# The `vec` Type

[[`vec<T, N>`:nosig]] is KFR's fixed-size vector type. It represents a known number of
values and maps element-wise operations to the SIMD facilities selected for the
current target. The same source can use a single scalar lane, a native-width
register, or a larger vector assembled from several registers; the interface
does not change.

Include `<kfr/simd.hpp>` for this type and the SIMD operations used in the
examples below.

```c++
#include <kfr/simd.hpp>

using namespace kfr;
|||TEST_CASE("vec.md/intro")
|||{
f32x4 samples{ 1.f, 2.f, 3.f, 4.f };
f32x4 gain = 0.5f; // one value broadcast to every lane

const auto scaled = samples * gain;
|||CHECK_THAT(scaled, DeepMatcher(f32x4{ 0.5f, 1.f, 1.5f, 2.f }));
|||}
```

The type is a value type. A vector holds its elements directly, has a
compile-time width, and does not allocate memory. It is the right tool for a
small, fixed unit of data processed in parallel. For a run-time-sized sequence,
use [[`univector<T, Size>`:nosig]] or an expression instead.

## Element type and width

`vec` takes an element type and a top-level element count. Its element type can
be a supported arithmetic type, a [[`complex`:nosig]] value, or another `vec`. KFR
flattens compound elements internally so that they can still be processed as
SIMD data.

```c++
|||using namespace kfr;
|||TEST_CASE("vec.md/element type and width")
|||{
vec<i16, 8> pcm{};                         // eight signed 16-bit values
vec<complex<f32>, 4> spectrum{};           // four complex values
mat<f32, 3, 2> transform{};                // two rows, each with three values
vec<vec<f32, 2>, 3> same_shape_as_matrix{};
|||CHECK(pcm.size() == 8);
|||CHECK(spectrum.size() == 4);
|||CHECK(transform.size() == 2);
|||CHECK(same_shape_as_matrix.size() == 3);
|||}
```

[[`mat`:nosig]] is an alias for a nested vector: `mat<T, Columns, Rows>` has `Rows`
top-level elements, each a `vec<T, Columns>`. Consequently, the example above
is a two-row, three-column value. The vector's [[`vec<T, N>::size()`:nosig]] reports its
top-level count, while [[`vec<T, N>::scalar_size()`:nosig]] reports the total number of
deepest scalar components. This distinction matters for vectors of complex
values and nested vectors.

The public API is portable, but the native representation and alignment of a
`vec` follow the architecture selected when KFR is compiled. See [Portable
Layout and ABI Boundaries](portable_layout.md) when a vector crosses a target
or ABI boundary.

## Constructing vectors

Construct a vector with a broadcast value, an explicit lane list, another
compatible vector, or an index generator. A default-constructed `vec` leaves
its elements uninitialized, so initialize it deliberately before reading it.

```c++
|||using namespace kfr;
|||TEST_CASE("vec.md/constructing vectors")
|||{
const f32x4 silence(czeros);                 // { 0, 0, 0, 0 }
const f32x4 unit{ 1.f, 1.f, 1.f, 1.f };      // explicit lanes
const f32x4 half = 0.5f;                     // broadcast construction

const auto indices = vec<i32, 4>(from_lambda{}, [](size_t index) {
	return static_cast<i32>(index);
});

const vec<f64, 4> widened = unit;            // element conversion
const auto inferred = vec{ 1, 2, 3 };        // vec<int, 3>
|||CHECK_THAT(silence, DeepMatcher(f32x4{ 0.f, 0.f, 0.f, 0.f }));
|||CHECK_THAT(unit, DeepMatcher(f32x4{ 1.f, 1.f, 1.f, 1.f }));
|||CHECK_THAT(half, DeepMatcher(f32x4{ 0.5f, 0.5f, 0.5f, 0.5f }));
|||CHECK_THAT(indices, DeepMatcher(vec<i32, 4>{ 0, 1, 2, 3 }));
|||CHECK_THAT(widened, DeepMatcher(vec<f64, 4>{ 1.0, 1.0, 1.0, 1.0 }));
|||CHECK_THAT(inferred, DeepMatcher(vec<int, 3>{ 1, 2, 3 }));
|||}
```

The tags [[`czeros`:nosig]] and [[`cones`:nosig]] initialize the underlying bits to all
zeros and all ones. [[`czeros`:nosig]] is a numeric zero for the usual KFR scalar types.
[[`cones`:nosig]] is handy for masks and bitwise operations — for a floating-point
vector, though, an all-ones bit pattern is a NaN, not the numeric value `1.0`.

For compound element types, broadcasting repeats the complete compound value,
not just its first scalar component:

```c++
|||using namespace kfr;
|||TEST_CASE("vec.md/compound broadcast")
|||{
const vec<i32, 2> pair{ 10, 20 };
const vec<vec<i32, 2>, 3> repeated = pair;
// { { 10, 20 }, { 10, 20 }, { 10, 20 } }
|||CHECK_THAT(repeated, DeepMatcher(vec<vec<i32, 2>, 3>{ pair, pair, pair }));
|||}
```

Vectors with matching total widths can also be joined by the vector constructor
or with structural helpers such as [[`concat(const vec<T, Ns> &...)`:nosig]], [[`slice(const vec<T, N> &)`:nosig]], [[`lowhalf(const vec<T, N> &)`:nosig]],
and [[`highhalf(const vec<T, N> &)`:nosig]]:

```c++
|||using namespace kfr;
|||TEST_CASE("vec.md/join")
|||{
const vec<i32, 2> left{ 1, 2 };
const vec<i32, 2> right{ 3, 4 };
const vec<i32, 4> joined(left, right);
|||CHECK_THAT(joined, DeepMatcher(vec<i32, 4>{ 1, 2, 3, 4 }));
|||}
```

### Lane order follows memory order

KFR writes constructor arguments and indexes vector lanes in increasing memory
order: lane 0 is the first value stored. This is the opposite of the argument
order used by several Intel intrinsic constructors, whose arguments are listed
from the highest lane down to the lowest lane.

```c++
||||||||||
using namespace kfr;

const i32x4 kfr_values{ 1, 2, 3, 4 };
// Memory order: { 1, 2, 3, 4 }

// Intel intrinsic for comparison:
const __m128i intel_values = _mm_set_epi32(1, 2, 3, 4);
// Memory order: { 4, 3, 2, 1 }
||||||||||
```

Thus `kfr_values[0]` is `1`. When translating code from Intel intrinsics,
reverse the arguments of constructors such as `_mm_set_epi32` rather than
reversing KFR indexing or memory access.

## Names for common vector types

KFR provides concise aliases for common scalar types and widths. For example,
[[`f32x4`:nosig]], [[`f64x2`:nosig]], [[`i16x8`:nosig]], and [[`u32x4`:nosig]] name the corresponding
`vec` types. Aliases are available for 1, 2, 3, 4, 8, 16, 32, and 64 elements
of the fixed-width integer and floating-point types.

The [[`glsl_names`:nosig]] and [[`opencl_names`:nosig]] namespaces offer aliases such as
[[`glsl_names::vec4`:nosig]] and [[`opencl_names::float4`:nosig]] when code is shared with
those naming conventions. They are namespaced aliases; they do not introduce
GLSL or OpenCL names into the global namespace.

Use the alias that makes the data's meaning clearest. [[`f32x4`:nosig]] is convenient in
ordinary SIMD code, while `vec<complex<f32>, 4>` states explicitly that each
lane is one complex sample.

## Accessing lanes

Read lanes with [[`vec<T, N>::operator[](size_t)`:nosig]], [[`vec<T, N>::front()`:nosig]], [[`vec<T, N>::back()`:nosig]], or
[[`vec<T, N>::get()`:nosig]]. Indexing a non-const vector returns a lightweight mutable proxy
rather than a `T&`; it supports assignment and compound assignment while KFR
keeps the vector in its SIMD representation.

```c++
|||using namespace kfr;
|||TEST_CASE("vec.md/lane access")
|||{
i32x4 values{ 10, 20, 30, 40 };

const i32 first = values.front();
const i32 third = values[2];
const i32 last = values.get<3>();

values[1] = 25;
values.back() += 2;
values.set(0, 5);
|||CHECK(first == 10);
|||CHECK(third == 30);
|||CHECK(last == 40);
|||CHECK(values[1] == 25);
|||CHECK(values[3] == 42);
|||CHECK(values[0] == 5);
|||}
```

Use lane access for setup, control decisions, and the occasional boundary
case. Repeated scalar extraction and insertion can prevent the compiler from
keeping a hot calculation in vector form. Prefer whole-vector arithmetic,
[[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]], reductions, and shuffle operations inside performance-critical
loops.

## Compound values, flattening, and complex lanes

[[`vec<T, N>::flatten()`:nosig]] exposes the deepest scalar components of a compound vector;
[[`vec<T, N>::from_flatten(const vec<ST, SN> &)`:nosig]] reconstructs the original shape. KFR uses these
operations internally for compound SIMD values, but they are also useful when
an interface requires scalar components.

```c++
using namespace kfr;
|||TEST_CASE("vec.md/flatten")
|||{
const vec<complex<f32>, 2> bins{
	complex<f32>{ 1.f, 2.f },
	complex<f32>{ 3.f, 4.f },
};

const f32x4 interleaved = bins.flatten();
// { 1, 2, 3, 4 }: real, imaginary, real, imaginary

const auto restored = vec<complex<f32>, 2>::from_flatten(interleaved);
|||CHECK_THAT(interleaved, DeepMatcher(f32x4{ 1.f, 2.f, 3.f, 4.f }));
|||CHECK_THAT(restored, DeepMatcher(bins));
|||}
```

For complex vector processing, [[`make_complex(const vec<T1, N> &, const vec<T2, N> &)`:nosig]], [[`real(const vec<complex<T>, N> &)`:nosig]], [[`imag(const vec<complex<T>, N> &)`:nosig]],
[[`cconj(const T1 &)`:nosig]], [[`ccomp(const vec<T, N> &)`:nosig]], and [[`cdecom(const vec<complex<T>, N> &)`:nosig]] avoid manual component management.
The complex components are stored in real/imaginary order when flattened.

## Rearranging and loading data

[[`vec<T, N>::shuffle(csizes_t<indices...>)`:nosig]] creates a vector whose lanes are selected with compile-time
indices. It can select from one source vector or from two vectors treated as a
concatenated source.

```c++
using namespace kfr;
|||TEST_CASE("vec.md/shuffle")
|||{
const f32x4 input{ 1.f, 2.f, 3.f, 4.f };
const auto reversed = input.shuffle(csizes<3, 2, 1, 0>);
const auto duplicated_ends = input.shuffle(csizes<0, 0, 3, 3>);
|||CHECK_THAT(reversed, DeepMatcher(f32x4{ 4.f, 3.f, 2.f, 1.f }));
|||CHECK_THAT(duplicated_ends, DeepMatcher(f32x4{ 1.f, 1.f, 4.f, 4.f }));
|||}
```

[[`shufflevector(const vec<T, N> &, csizes_t<indices...>)`:nosig]] and [[`shufflevectors(const vec<T, N> &, const vec<T, N> &, csizes_t<indices...>)`:nosig]] provide the same operation as free
functions. [[`concat(const vec<T, Ns> &...)`:nosig]], [[`slice(const vec<T, N> &)`:nosig]], [[`lowhalf(const vec<T, N> &)`:nosig]], [[`highhalf(const vec<T, N> &)`:nosig]], and
[[`resize(const vec<T, N> &)`:nosig]] handle other composition and extraction tasks.

To bring contiguous data into a vector, use [[`read`:nosig]]; use [[`write`:nosig]] to
store it. The default form supports an ordinary valid pointer. The aligned
forms make an alignment promise and should only be used when the caller can
meet it.

```c++
|||using namespace kfr;
|||TEST_CASE("vec.md/read write")
|||{
const f32 input[] = { 1.f, 2.f, 3.f, 4.f };
f32 output[4];

const f32x4 x = read<4>(input);
write(output, x * 0.25f);
|||CHECK(output[0] == 0.25f);
|||CHECK(output[3] == 1.f);
|||}
```

The aligned forms, partial vectors, and non-contiguous access have additional
pointer-validity and alignment requirements; use the specific memory operation
that matches the layout you actually own.

## Vector width and portability

The width of a `vec` is always part of its C++ type. It need not be the native
width of the selected processor: `vec<f32, 3>`, `vec<f32, 4>`, and
`vec<f32, 11>` are all valid. [[`vector_width`:nosig]] reports KFR's current native
width for an element type, which is useful when a low-level routine wants to
choose its natural chunk size.

A `vec` cannot exceed 4096 bytes: `sizeof(T) * N` must be at most 4096. This
matches the minimum supported platform page size and ensures a full-width
access from a pointer aligned to the vector's byte width remains within one
page. The limit applies to the complete top-level element type, including
[[`complex`:nosig]] and nested `vec` elements.

Avoid encoding the assumed native CPU width into public interfaces. A fixed
`vec` type is excellent for implementation code compiled for one target; use
[[`portable_vec<T, N>`:nosig]] at target-independent KFR interface boundaries.

## See Also

- [Portable Layout and ABI Boundaries](portable_layout.md)
- [Arithmetic, Bitwise Operations, and Type Conversion](arithmetic.md)
