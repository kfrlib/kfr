# Portable Layout and ABI Boundaries

[[`vec<T, N>`:nosig]] gives KFR efficient, target-aware SIMD values. That target awareness
is useful inside a calculation, but it matters at interfaces: the alignment and
native representation of a `vec` depend on the architecture selected for the
translation unit. Code compiled for different KFR targets should not make a
native `vec` part of its shared ABI by default.

[[`portable_vec<T, N>`:nosig]] is the corresponding target-independent KFR representation.
It has a fixed element array, a documented layout compatible with the matching
`vec`, and conversions at the boundary where SIMD work begins or ends.

Include `<kfr/simd.hpp>` for the types discussed here.

## Choose the representation for the boundary

Use a native [[`vec<T, N>`:nosig]] for local computation in code built for one KFR target:

```c++
|||#include <kfr/simd.hpp>
|||using namespace kfr;
|||TEST_CASE("portable_layout.md/native vec")
|||{
f32x4 gain{ 0.5f, 0.5f, 0.5f, 0.5f };
const auto result = gain * 2.f;
|||CHECK(result[0] == 1.0f);
|||}
```

Use [[`portable_vec<T, N>`:nosig]] when fixed vector data passes between KFR translation
units that may have been compiled for different architecture targets. Typical
examples include a portable dispatch interface, a target-neutral callback
signature, or data held while choosing a target-specific implementation.

```c++
#include <kfr/simd.hpp>

using namespace kfr;

portable_vec<f32, 4> process(portable_vec<f32, 4> input)
{
	// Convert at the target-specific processing boundary.
	f32x4 native = input;
	native *= 0.5f;

	// Convert back before returning through the portable interface.
	return native;
}

|||TEST_CASE("portable_layout.md/process")
|||{
|||const portable_vec<f32, 4> input{ 2.f, 4.f, 6.f, 8.f };
|||const portable_vec<f32, 4> result = process(input);
|||CHECK(result.elem[0] == 1.f);
|||CHECK(result.elem[3] == 4.f);
|||}
```

The conversion from `portable_vec` to `vec` is supported by the `vec`
constructor. A native vector converts back to the matching `portable_vec`; the
[[`to_vec(const portable_vec<T, N> &)`:nosig]] helper is also available when converting in the other direction.

## What `portable_vec` guarantees

`portable_vec` has a compile-time element type and count, just like `vec`. It
stores a public `elem` array of that type, offers [[`portable_vec<T, N>::operator[](size_t)`:nosig]],
`portable_vec::front`, `portable_vec::back`, and
`portable_vec::size`, and supports broadcast or explicit-element
construction.

```c++
|||TEST_CASE("portable_layout.md/portable_vec basics")
|||{
portable_vec<i32, 4> message{ 10, 20, 30, 40 };
message[2] = 99;

const portable_vec<f32, 4> silence = 0.f;
|||CHECK(message[2] == 99);
|||CHECK(silence[0] == 0.f);
|||}
```

KFR specifies that `portable_vec<T, N>` has layout compatible with the
matching `vec<T, N>`. Its alignment is calculated in the same way as the
matching `vec`, from the element size and element count. The type accepts
between 1 and 1024 elements. Its scalar element type must be a
KFR SIMD element type; compound element types, including complex and nested
vectors, are also supported through KFR's compound-type machinery.

Like `vec`, default construction does not initialize the contained values.
Construct with an explicit value, an explicit lane list, or value-initialization
when the object will be read before being written.

## Why the distinction is necessary

KFR's native [[`vec<T, N>`:nosig]] representation is selected by the SIMD backend.
The same source type can therefore have different internal representations in
translation units compiled for different targets. For example, `vec<f32, 8>`
can be one `__m256` on AVX2 but two `__m128` values on AVX. Passing such a
`vec` across that translation-unit boundary can cause undefined behavior even
though the source-level type is identical.

The API of `vec` remains source-portable, but its ABI is deliberately optimized
for the compilation target. `portable_vec` provides the target-independent
boundary representation while retaining layout compatibility with the matching
`vec`; convert at the boundary before native SIMD work begins, and convert back
before returning through the boundary.

`portable_vec` is not required for sharing a value between translation units.
Any memory containing the scalar elements can be used as the boundary
representation. Prefer suitably aligned storage when it is practical, and use
KFR's ordinary `read` and `write` operations to convert at the boundary:

```c++
|||TEST_CASE("portable_layout.md/write to shared memory")
|||{
vec<u8, 32> v{ 1 };
alignas(32) std::array<u8, 32> value;
write(value.data(), v);
|||CHECK(value[0] == 1);
|||}
```

The receiving translation unit can read the same bytes into its own native
`vec` representation. The shared memory must have sufficient size and a
compatible scalar representation; if the aligned form of `write` or `read` is
selected, the storage must also satisfy the corresponding alignment guarantee.

[[`platform<c>`:nosig]] exposes the selected target's vector traits, including
`platform::native_vector_alignment`,
`platform::maximum_vector_alignment`, and `platform::fast_unaligned`.
These are compile-time properties of the KFR target, not a run-time guarantee
about an arbitrary processor.

> [!note]
> `portable_vec` gives you a stable in-memory KFR layout, not a complete file
> or network format — a serialized protocol still needs an explicit byte
> order, scalar encoding, versioning policy, and compatibility rules.

## Passing and storing portable values

Passing a `portable_vec` by value is appropriate for a small fixed-size
interface value. For a larger structure or a bulk buffer, establish ownership,
alignment, lifetime, and byte-order rules for the enclosing API instead of
relying on an implementation detail of one member type.

At the entry point of target-specific work, convert once and keep the native
value in vector form for as much of the calculation as possible. Converting
back and forth for every operation defeats the purpose of the native type.

```c++
|||using namespace kfr;

portable_vec<f32, 4> apply_gain(portable_vec<f32, 4> input, float gain)
{
	auto samples = to_vec(input);
	samples *= gain;
	return samples;
}

|||TEST_CASE("portable_layout.md/apply_gain")
|||{
|||const portable_vec<f32, 4> input{ 2.f, 4.f, 6.f, 8.f };
|||const portable_vec<f32, 4> result = apply_gain(input, 0.5f);
|||CHECK(result.elem[0] == 1.f);
|||CHECK(result.elem[3] == 4.f);
|||}
```

Use the exact same element type and count on both sides. The compatible type
for `portable_vec<f32, 4>` is `vec<f32, 4>` (also spelled [[`f32x4`:nosig]]), not a
vector with a different scalar type or lane count.

## Alignment promises for memory access

The default [[`read`:nosig]] and [[`write`:nosig]] operations support ordinary valid
pointers without requiring a particular vector alignment. Their aligned forms
accept an `Aligned` template argument of `true`; this is a promise by the
caller that the pointer is suitably aligned for the operation.

```c++
|||TEST_CASE("portable_layout.md/aligned read write")
|||{
alignas(platform<>::native_vector_alignment) f32 source[4]{ 1, 2, 3, 4 };
alignas(platform<>::native_vector_alignment) f32 destination[4]{};

const f32x4 values = read<4, true>(source);
write<true>(destination, values * 0.5f);
|||CHECK(destination[0] == 0.5f);
|||CHECK(destination[3] == 2.f);
|||}
```

`true` does not repair an unaligned address. Do not select the aligned form
unless the storage declaration, allocator, or external API provides the
required guarantee. The data must also contain the requested number of
contiguous elements.

For arrays owned by KFR containers, see 
[Memory, Alignment, and Ownership](../core/memory.md). Gathers, scatters, and partial operations have separate
validity requirements from the contiguous operations shown here.

## Compact packed storage

[[`pkd_vec<T, N>`:nosig]] is a compact storage wrapper initialized from a `vec` or from
scalar values. It is useful when a local packed representation is required:

```c++
|||TEST_CASE("portable_layout.md/pkd_vec")
|||{
const i16x4 samples{ 10, 20, 30, 40 };
const pkd_vec<i16, 4> packed{ samples };
|||(void)packed;
|||}
```

[[`pkd_vec<T, N>`:nosig]] intentionally exposes a much smaller public interface than
`portable_vec`: it has no public `data` accessor or public conversion back to
`vec`. Treat it as a compact storage utility rather than a two-way
serialization API, and don't assume anything about packing, padding,
endianness, or cross-toolchain binary compatibility beyond what your own
external format specifies.

## Practical checklist

1. Use [[`vec<T, N>`:nosig]] for computation inside one selected KFR target.
2. Use [[`portable_vec<T, N>`:nosig]] for a fixed-width KFR value crossing a
	target-independent translation-unit interface.
3. Convert at the edge, not between every arithmetic operation.
4. Use aligned [[`read`:nosig]] or [[`write`:nosig]] only when the pointer satisfies the
	corresponding selected-target alignment requirement.
5. Define serialization separately; neither vector type replaces a complete
	portable on-disk or wire-format specification.

## See Also

- [The `vec` Type](vec.md)
- [Memory, Alignment, and Ownership](../core/memory.md)
