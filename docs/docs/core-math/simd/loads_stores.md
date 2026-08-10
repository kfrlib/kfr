# SIMD Loads, Stores, Gathers, and Scatters

KFR memory operations move data between ordinary C++ storage and a fixed-size
[[`vec`:nosig]]. Prefer a contiguous [[`read`:nosig]] or [[`write`:nosig]] when the data is
contiguous. Use grouped, gathered, or strided operations only for layouts that
need them: they have stricter validity requirements and are usually costlier.

Include `<kfr/simd.hpp>` for these APIs.

```c++
#include <kfr/simd.hpp>

using namespace kfr;
|||TEST_CASE("loads_stores.md/contiguous read write")
|||{
const f32 input[4]{ 1.f, 2.f, 3.f, 4.f };
f32 output[4];

const f32x4 samples = read<4>(input);
write(output, samples * 0.5f);
|||CHECK(output[0] == 0.5f);
|||CHECK(output[3] == 2.0f);
|||}
```

KFR follows memory order: `read<4>(input)` produces `{ input[0], input[1],
input[2], input[3] }`; [[`write(T *, const vec<T, N> &)`:nosig]] stores lanes in that same order.

## Contiguous loads and stores

[[`read`:nosig]] loads a fixed number of contiguous top-level objects, and
[[`write`:nosig]] stores the matching vector:

```c++
||||||||||||
read<N>(source);
write(destination, value);
||||||||||||
```

`N` is the vector's top-level element count. Thus [[`read(const T *)`:nosig]]<4> with a
`complex<f32>*` loads four complex objects, not four individual floating-point
components.

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/complex read")
|||{
const complex<f32> bins[]{
	{ 1.f, 2.f }, { 3.f, 4.f }, { 5.f, 6.f }, { 7.f, 8.f },
};

const vec<complex<f32>, 4> spectrum = read<4>(bins);
|||CHECK_THAT(spectrum, DeepMatcher(vec<complex<f32>, 4>{ bins[0], bins[1], bins[2], bins[3] }));
|||}
```

For a load, the pointer must designate at least `N` valid contiguous objects.
For a store, it must designate `N` writable contiguous objects. KFR performs no
bounds checks. An ordinary unaligned load is still a full-width load, so it is
not safe at the tail of a shorter buffer.

The member forms have the same semantics:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/member read write")
|||{
|||const f32 input[4]{ 1.f, 2.f, 3.f, 4.f };
|||f32 output[4]{};
f32x4 value{ input };
value.write(output);
|||CHECK_THAT(value, DeepMatcher(f32x4{ 1.f, 2.f, 3.f, 4.f }));
|||CHECK(output[0] == 1.f);
|||}
```

## Alignment contracts

The default forms use unaligned access and accept any otherwise valid pointer.
Passing `true` as the aligned template argument makes a compile-time promise:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/aligned read write")
|||{
alignas(platform<>::native_vector_alignment) f32 source[4]{ 1, 2, 3, 4 };
alignas(platform<>::native_vector_alignment) f32 destination[4]{};

const f32x4 value = read<4, true>(source);
write<true>(destination, value);
|||CHECK_THAT(value, DeepMatcher(f32x4{ 1.f, 2.f, 3.f, 4.f }));
|||CHECK(destination[0] == 1.f);
|||}
```

With `true`, the pointer must meet the selected target's required vector
alignment. This can be greater than `alignof(f32)` and changes with the KFR
target. A derived address such as `source + 1` generally loses that guarantee.

The `vec` constructor and member store use [[`caligned`:nosig]] and [[`cunaligned`:nosig]]
tags:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/caligned read write")
|||{
|||alignas(platform<>::native_vector_alignment) f32 source[4]{ 1, 2, 3, 4 };
|||alignas(platform<>::native_vector_alignment) f32 destination[4]{};
const f32x4 value{ source, caligned };
value.write(destination, caligned);
|||CHECK(destination[0] == 1.f);
|||}
```

These tags choose an overload at compile time. They do not inspect or correct
the address at run time. Use aligned access only when the allocation or
interface guarantees the alignment for every accessed address.

## Grouped contiguous transfers

[[`read_group`:nosig]] and [[`write_group`:nosig]] transfer repeated contiguous chunks with
gaps between chunks. Their template arguments are `count`, `N`, and optional
`group`. Each of the `count` chunks contains `group * N` contiguous elements;
the run-time `stride` is measured in groups of `group` elements.

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/read_group write_group")
|||{
const i32 values[]{ 0, 1, 2, 3, 4, 5, 6, 7 };
const auto selected = read_group<2, 1, 2>(values, 2);
// { 0, 1, 4, 5 }

i32 result[8]{};
write_group<2, 1, 2>(result, 2, selected);
// Writes to result[0..1] and result[4..5]
|||CHECK_THAT(selected, DeepMatcher(vec<i32, 4>{ 0, 1, 4, 5 }));
|||CHECK(result[0] == 0);
|||CHECK(result[1] == 1);
|||CHECK(result[4] == 4);
|||CHECK(result[5] == 5);
|||}
```

For chunk index $i$, the chunk begins at:

$$
\texttt{base} + i \cdot \texttt{group} \cdot \texttt{stride}.
$$

Therefore, `stride == 1` means adjacent chunks regardless of `group`. Each
chunk must be fully valid and contiguous. With aligned access, every chunk
address—not only the first one—must satisfy the alignment contract.

## Gathers

A gather constructs a vector from non-contiguous locations. Every selected
address must be valid to read; KFR does not check indices.

Use [[`gather`:nosig]] with a small fixed set of run-time indices:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/gather runtime indices")
|||{
const i32 values[]{ 10, 20, 30, 40, 50, 60, 70 };
const auto picked = gather(values, 6, 1, 4);
// { 70, 20, 50 }
|||CHECK_THAT(picked, DeepMatcher(vec<i32, 3>{ 70, 20, 50 }));
```

When the pattern itself is fixed, indices can be template arguments:

```c++
using namespace kfr;

const auto picked2 = gather<6, 1, 4>(values);
// { 70, 20, 50 }
|||CHECK_THAT(picked2, DeepMatcher(vec<i32, 3>{ 70, 20, 50 }));
```

For one run-time index per output lane, use a [[`u32`:nosig]] vector of indices:

```c++
using namespace kfr;

const u32x4 indices{ 3, 0, 6, 1 };
const auto reordered = gather(values, indices);
// { 40, 10, 70, 20 }
|||CHECK_THAT(reordered, DeepMatcher(vec<i32, 4>{ 40, 10, 70, 20 }));
|||}
```

This index-vector overload specifically takes `vec<u32, N>`.

### Gathering contiguous groups

The group-offset overload of [[`gather`:nosig]] reads a contiguous group at each
offset lane. Offsets are measured in groups, not individual elements.

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/gather contiguous groups")
|||{
const i32 values[]{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
const i32x2 offsets{ 0, 3 };
const auto pairs = gather<2>(values, offsets);
// { 0, 1, 6, 7 }
|||CHECK_THAT(pairs, DeepMatcher(vec<i32, 4>{ 0, 1, 6, 7 }));
|||}
```

For offset lane $i$, `gather<groupsize>` begins at
`base + groupsize * offsets[i]` and reads `groupsize` contiguous objects.

## Strided access

[[`gather_stride`:nosig]] and [[`scatter_stride`:nosig]] address regularly spaced data. In
the compile-time-stride gather form, the stride is expressed in elements:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/gather_stride compile-time")
|||{
const i32 values[]{ 10, 20, 30, 40, 50, 60 };
const auto every_second = gather_stride<3, 2>(values);
// { 10, 30, 50 }
|||CHECK_THAT(every_second, DeepMatcher(vec<i32, 3>{ 10, 30, 50 }));
|||}
```

The run-time-stride form has a `groupsize` template argument; its stride is in
groups, and each group is contiguous:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/gather_stride run-time")
|||{
const i32 values[]{ 0, 1, 2, 3, 4, 5, 6, 7 };
const auto frames = gather_stride<2, 2>(values, 2);
// { 0, 1, 4, 5 }
|||CHECK_THAT(frames, DeepMatcher(vec<i32, 4>{ 0, 1, 4, 5 }));
|||}
```

The grouped run-time-stride form supports any number of groups and any positive
`groupsize`. Each group is read or written contiguously, and the next group
starts at `groupsize * stride` elements after the base group. Use explicit
[[`read(const T *)`:nosig]]/[[`write(T *, const vec<T, N> &)`:nosig]] calls when a
contiguous layout is simpler or more efficient.

## Scatters

The offset-vector [[`scatter`:nosig]] stores sequential groups from a vector at
non-contiguous destination offsets. Offsets are again measured in groups.

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/scatter")
|||{
i32 result[10]{};
const i32x2 offsets{ 0, 3 };

scatter<2>(result, offsets, vec<i32, 4>{ 10, 11, 12, 13 });
// result[0..1] = { 10, 11 }
// result[6..7] = { 12, 13 }
|||CHECK(result[0] == 10);
|||CHECK(result[1] == 11);
|||CHECK(result[6] == 12);
|||CHECK(result[7] == 13);
|||}
```

Overlapping or duplicate offsets are not rejected. Stores occur in sequence,
so a later group can overwrite earlier data. Every destination group must be
writable and fully in bounds.

[[`scatter_stride`:nosig]] follows the same group and stride conventions as
[[`gather_stride`:nosig]].

## `stride_pointer`

[[`stride_pointer<T, groupsize>`:nosig]] packages a pointer and a run-time group stride as a
lightweight read adaptor. Its [[`stride_pointer<const T, groupsize>::read(csize_t<N>)`:nosig]] operation delegates to
the grouped [[`gather_stride`:nosig]] API.

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/stride_pointer read")
|||{
const i32 values[]{ 0, 1, 2, 3, 4, 5, 6, 7 };
stride_pointer<const i32, 2> frames{ values, 2 };

const auto two_frames = frames.read<4>();
// { 0, 1, 4, 5 }
|||CHECK_THAT(two_frames, DeepMatcher(vec<i32, 4>{ 0, 1, 4, 5 }));
|||}
```

The `N` in `read<N>` and `write` is the total number of top-level elements,
not the number of groups. It must be a multiple of `groupsize`; each group is
read or written contiguously. The stored `stride` is measured in groups, so
the next group begins `groupsize * stride` elements after the previous base
group. Every accessed group must be valid (and writable for `write`), and KFR
does not perform bounds checks. The current implementation supports any
number of groups; a zero stride is also accepted and makes all groups overlap.

For example, a mutable pointer can write the same layout directly:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/stride_pointer write")
|||{
i32 result[6]{};
stride_pointer<i32, 2> frames{ result, 2 };
frames.write(vec<i32, 4>{ 10, 11, 20, 21 });
// result[0..1] = { 10, 11 }
// result[4..5] = { 20, 21 }
|||CHECK(result[0] == 10);
|||CHECK(result[1] == 11);
|||CHECK(result[4] == 20);
|||CHECK(result[5] == 21);
|||}
```

## Partial reads, writes, masks, and buffer tails

[[`partial_read`:nosig]] and [[`partial_write`:nosig]] handle a final incomplete vector without
accessing past the logical end of a buffer:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/partial read write")
|||{
const f32 input[]{ 1.f, 2.f, 3.f };
f32 output[]{ 0.f, 0.f, 0.f };

const f32x4 tail = partial_read<4>(input, 3);
partial_write(output, tail * 0.5f, 3);
|||CHECK(output[0] == 0.5f);
|||CHECK(output[2] == 1.5f);
|||}
```

`count` must be greater than zero. Values greater than the vector width are
clamped to that width. A partial read defines the first `count` lanes; the
remaining lanes are unspecified. A partial write changes only the first
`count` destination objects.

These operations are always slower than [[`read`:nosig]] and [[`write`:nosig]].
Use them only for a loop tail, not the main vector loop body. They select a
smaller complete load or store width at run time, so only the requested prefix
is accessed.

The aligned forms require `N` and `N * sizeof(T)` to be powers of two, plus a
pointer aligned to `N * sizeof(T)`. For an aligned partial read, KFR may load
the entire vector; for an aligned partial write, KFR may read the complete
vector, replace the leading lanes, and write it back. This is safe because a
`vec` is limited to 4096 bytes and the full-width alignment keeps the access
within one minimum-sized page. Consequently, the entire vector starting at the
aligned pointer must be readable for `partial_read<..., true>` and readable and
writable for `partial_write<true>`.

[[`partial_mask`:nosig]] creates a *value vector* with leading all-one bit patterns
and trailing zero bit patterns:

```c++
using namespace kfr;
|||TEST_CASE("loads_stores.md/partial_mask")
|||{
const auto raw = partial_mask<f32, 4>(2);
const mask<f32, 4> first_two{ raw };
|||const f32x4 selected = select(first_two, f32x4{ 1, 2, 3, 4 }, f32x4{ 0 });
|||CHECK_THAT(selected, DeepMatcher(f32x4{ 1.f, 2.f, 0.f, 0.f }));
|||}
```

`first_two` selects the first two lanes when used with [[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]]. The count
must be in the range 0 through `N`; there is no run-time bounds check. The
public implementation supports widths up to 32 lanes.

For floating-point `T`, an all-one value is a bit pattern, not numerical
`1.0f`. Treat `partial_mask` results as mask bits, not arithmetic data.

Masking a full-width [[`read`:nosig]] or [[`write`:nosig]] does not make it safe
at the end of a buffer: all lanes are accessed before the mask can be applied.
Use `partial_read`/`partial_write`, process a scalar tail, arrange accessible
padded storage, or use an explicitly tail-safe algorithm. For a vector loop
with a smaller-width tail, [[`block_process`:nosig]] can select the largest
complete blocks and finish with scalar blocks:

```c++
|||TEST_CASE("loads_stores.md/block_process tail")
|||{
|||using namespace kfr;
|||const size_t count = 6;
|||const f32 input[count]{ 1.f, 2.f, 3.f, 4.f, 5.f, 6.f };
|||f32 output[count]{};
block_process(count, csizes<4, 1>, [&](size_t offset, auto width)
{
	constexpr size_t N = KFR_CVAL(width);
	write(output + offset, read<N>(input + offset) * 0.5f);
});
|||CHECK(output[0] == 0.5f);
|||CHECK(output[5] == 3.0f);
|||}
```

The final `1` ensures that the tail is processed one element at a time, so no
full-width access extends beyond the buffer.

## Memory-access checklist

1. Prefer contiguous [[`read`:nosig]] and [[`write`:nosig]] whenever possible.
2. Ensure every accessed object exists; KFR memory operations do not check
	bounds.
3. Use aligned forms only when every accessed address satisfies the selected
	target's alignment requirement.
4. Remember the units: grouped offsets and run-time group strides are in
	groups, but the compile-time [[`gather_stride`:nosig]] stride is in elements.
5. Use `partial_read`/`partial_write` or scalar code for an unpadded loop tail;
	do not rely on a mask to protect an out-of-bounds full-width load or store.

## See Also

- [The `vec` Type](vec.md)
- [Portable Layout and ABI Boundaries](portable_layout.md)
- [Masks, Comparisons, and Branchless Selection](masks.md)
