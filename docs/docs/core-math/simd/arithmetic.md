# Arithmetic, Bitwise Operations, and Type Conversion

KFR applies most SIMD arithmetic lane by lane. When two vectors have the same
width, each result lane is computed from the values at the corresponding lane
index. Scalars mix naturally with vectors by broadcasting to the vector width.
The same functions also work with supported compound values, including complex
numbers and nested vectors.

Include `<kfr/simd.hpp>` for the SIMD operations in this article.

```c++
#include <kfr/simd.hpp>

using namespace kfr;

|||TEST_CASE("arithmetic.md/basic")
|||{
f32x4 samples{ 1.f, 2.f, 3.f, 4.f };
const auto scaled = samples * 0.5f;
const auto shifted = scaled + 1.f;
|||CHECK_THAT(shifted, DeepMatcher(f32x4{ 1.5f, 2.f, 2.5f, 3.f }));
|||}
```

Ordinary arithmetic follows the selected scalar type's usual rules. It does
not provide overflow detection or saturation. Use the explicit saturation
operations where clamping is required.

## Element-wise arithmetic

[[`vec<T, N>`:nosig]] supports unary minus and the familiar binary arithmetic operators:
`+`, `-`, `*`, `/`, and `%`, including their compound assignments. Vector and
scalar operands can be mixed in either order.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/element-wise")
|||{
const i32x4 counts{ 1, 2, 3, 4 };
const auto offset = counts + 10;
const auto doubled = 2 * counts;

f32x4 samples{ 0.25f, 0.5f, 0.75f, 1.f };
samples *= 0.5f;
const auto normalized = samples / 0.25f;
|||CHECK_THAT(offset, DeepMatcher(i32x4{ 11, 12, 13, 14 }));
|||CHECK_THAT(doubled, DeepMatcher(i32x4{ 2, 4, 6, 8 }));
|||CHECK_THAT(normalized, DeepMatcher(f32x4{ 0.5f, 1.f, 1.5f, 2.f }));
|||}
```

For mixed element types, KFR bases the result on `std::common_type_t` and
preserves the vector shape. A scalar mixed with a vector produces a vector;
same-width vectors produce a same-width vector. See [Numeric, Compound, and
Complex Types](../core/types.md) for KFR's vector and complex promotion rules.

Named helpers [[`add(const T1 &, const T2 &, const Ts &...)`:nosig]], [[`sub(const T1 &, const T2 &)`:nosig]], [[`mul(const T1 &, const T2 &, const Ts &...)`:nosig]], [[`div(const T1 &, const T2 &)`:nosig]], [[`mod(const T1 &, const T2 &)`:nosig]], and
[[`neg(const T1 &)`:nosig]] express the same basic operations in generic code. [[`sqr(const T1 &)`:nosig]],
[[`cub(const T1 &)`:nosig]], and [[`pow2(const T &)`:nosig]] through [[`pow5(const T &)`:nosig]] express common small powers
without repeating the multiplication sequence by hand.

> [!warning]
> Division by zero, signed integer overflow, and invalid shift counts are not
> made safe by vectorization. Validate inputs where the corresponding scalar
> operation requires it.

## Fused helpers and polynomial evaluation

[[`fmadd(const T1 &, const T2 &, const T3 &)`:nosig]] computes $x y + z$ and [[`fmsub(const T1 &, const T2 &, const T3 &)`:nosig]] computes $x y - z$ lane by
lane. They provide a clear way to write multiply-add expressions; whether the
target uses a hardware fused instruction is determined by the selected backend
and compiler settings.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/fused-helpers")
|||{
const f32x4 x{ -1.f, -0.5f, 0.5f, 1.f };
const auto y = fmadd(x, 2.f, 1.f); // x * 2 + 1

const auto polynomial = horner(x, 1.f, -2.f, 3.f);
// 3*x*x - 2*x + 1
|||CHECK_THAT(y, DeepMatcher(f32x4{ -1.f, 0.f, 2.f, 3.f }));
|||CHECK_THAT(polynomial, DeepMatcher(3.f * x * x - 2.f * x + 1.f));
|||}
```

[[`horner(const T1 &, const Ts &...)`:nosig]] evaluates coefficients in ascending degree order:

$$
\operatorname{horner}(x, c_0, c_1, \ldots, c_n)
= c_0 + c_1x + \cdots + c_nx^n.
$$

[[`horner_even(const T1 &, const Ts &...)`:nosig]] and [[`horner_odd(const T1 &, const Ts &...)`:nosig]] specialize the same pattern for
polynomials containing only even or only odd powers.

[[`mix(const T1 &, const T2 &, const T3 &)`:nosig]] interpolates from `x` to `y` with a control value in $[0, 1]$:

$$
\operatorname{mix}(c, x, y) = x + (y - x)c.
$$

[[`mixs(const T1 &, const T2 &, const T3 &)`:nosig]] accepts a signed control value in $[-1, 1]$ and maps it to the
range used by [[`mix(const T1 &, const T2 &, const T3 &)`:nosig]].

## Saturating addition and subtraction

Use [[`satadd(const T1 &, const T2 &)`:nosig]] and [[`satsub(const T1 &, const T2 &)`:nosig]] when an addition or subtraction must clamp
at the limits of the result type rather than wrap or overflow.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/saturating")
|||{
const u8x4 a{ 250, 2, 10, 0 };
const u8x4 b{ 10, 3, 20, 1 };

const auto sum = satadd(a, b);  // { 255, 5, 30, 1 }
const auto difference = satsub(u8x4{ 2, 0, 30, 1 }, b);
// { 0, 0, 10, 0 }
|||CHECK_THAT(sum, DeepMatcher(u8x4{ 255, 5, 30, 1 }));
|||CHECK_THAT(difference, DeepMatcher(u8x4{ 0, 0, 10, 0 }));
|||}
```

Saturation is explicit. Ordinary `+` and `-` retain their normal type and
backend semantics, so do not substitute them when clipping behavior is part of
the algorithm.

## Bitwise operations and shifts

The operators `~`, `&`, `|`, `^`, `<<`, and `>>` act lane by lane, as do the
compound assignment forms. The corresponding named operations are
[[`bitwisenot(const T1 &)`:nosig]], [[`bitwiseand(const T1 &, const T2 &)`:nosig]], [[`bitwiseandnot(const T1 &, const T2 &)`:nosig]], [[`bitwiseor(const T1 &, const T2 &)`:nosig]],
[[`bitwisexor(const T1 &, const T2 &)`:nosig]], [[`shl(const T1 &, const T2 &)`:nosig]], and [[`shr(const T1 &, const T2 &)`:nosig]]. [[`rol(const T1 &, const T2 &)`:nosig]] and [[`ror(const T1 &, const T2 &)`:nosig]] rotate
bits left or right within each lane.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/bitwise")
|||{
const u32x4 flags{ 0b0011u, 0b0101u, 0b1001u, 0b1111u };
const auto low_two = bitwiseand(flags, u32x4{ 0b0011u });
const auto cleared = bitwiseandnot(flags, u32x4{ 0b0100u });
const auto shifted = flags << 1;
|||CHECK_THAT(low_two, DeepMatcher(u32x4{ 0b0011u, 0b0001u, 0b0001u, 0b0011u }));
|||CHECK_THAT(cleared, DeepMatcher(u32x4{ 0b0011u, 0b0001u, 0b1001u, 0b1011u }));
|||CHECK_THAT(shifted, DeepMatcher(u32x4{ 0b0110u, 0b1010u, 0b10010u, 0b11110u }));
|||}
```

Bitwise functions are available for floating-point vectors as well, but they
operate on representation bits, not on numerical floating-point values. This
is useful for operations such as sign-bit manipulation. It can also create
NaNs, infinities, or other unusual floating-point encodings, so keep such code
separate from normal numerical arithmetic.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/sign-bit")
|||{
const f32x4 values{ -1.f, 2.f, -3.f, 4.f };
const auto words = ubitcast(values);
const auto magnitudes = bitcast<f32>(
	bitwiseand(words, u32x4{ 0x7fffffffu }));
|||CHECK_THAT(magnitudes, DeepMatcher(f32x4{ 1.f, 2.f, 3.f, 4.f }));
|||}
```

The example clears the IEEE-754 sign bit. It is a representation operation;
[[`abs(const T1 &)`:nosig]] is the clearer choice when the objective is simply numerical absolute
value.

## Numeric conversion

[[`cast(const vec<Tin, N> &)`:nosig]] converts values lane by lane while preserving the vector's top-level
shape. It is the right operation when the numerical value, rather than the
underlying bits, must be converted.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/numeric-conversion")
|||{
const i32x4 counts{ 10, 20, 30, 40 };
const f32x4 values = cast<f32>(counts);
const f64x4 precise = cast<f64>(values);
|||CHECK_THAT(values, DeepMatcher(f32x4{ 10.f, 20.f, 30.f, 40.f }));
|||CHECK_THAT(precise, DeepMatcher(f64x4{ 10.0, 20.0, 30.0, 40.0 }));
|||}
```

The converting `vec` constructor supports the same common equal-width cases.
[[`promoteto(const vec<Tin, N> &)`:nosig]] and [[`broadcastto(const vec<Tin, N> &)`:nosig]] are lower-level helpers used by generic
code to preserve or expand compound shapes during promotion. Prefer [[`cast(const vec<Tin, N> &)`:nosig]] in
application code unless the distinction is required by a shape-generic
algorithm.

Conversion is not validation. A conversion to a narrower integer or an
integer conversion from a non-finite or out-of-range floating-point value
needs an application-level range policy. Use [[`floor(const T1 &)`:nosig]], [[`ceil(const T1 &)`:nosig]],
[[`round(const T1 &)`:nosig]], or [[`trunc(const T1 &)`:nosig]] first when an explicit rounding step is required.
The integer-returning forms [[`ifloor(const T1 &)`:nosig]], [[`iceil(const T1 &)`:nosig]], [[`iround(const T1 &)`:nosig]], and
[[`itrunc(const T1 &)`:nosig]] make that result category explicit.

## Bit reinterpretation

[[`bitcast(const vec<From, N> &)`:nosig]] preserves the object representation instead of converting a
numerical value. Scalar source and destination types must have the same size.
For vectors, the total byte count is preserved, so changing the element size
can change the lane count.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/bit-reinterpretation")
|||{
const f32x4 values{ -0.f, 1.f, 2.f, 3.f };
const u32x4 words = bitcast<u32>(values);
const f32x4 restored = bitcast<f32>(words);
|||CHECK_THAT(restored, DeepMatcher(values));
|||}
```

[[`ubitcast(const vec<From, N> &)`:nosig]], [[`ibitcast(const vec<From, N> &)`:nosig]], [[`fbitcast(const vec<From, N> &)`:nosig]], and [[`uibitcast(const vec<From, N> &)`:nosig]] select a
same-storage-shape destination category automatically. `vec::frombits` is
the corresponding static vector member when the source and destination have
the same total byte count.

Use [[`bitcast(const vec<From, N> &)`:nosig]] only for format-level work: masks, sign/exponent fields, hashing,
or an interface that explicitly specifies raw bits — reach for [[`cast(const vec<Tin, N> &)`:nosig]]
when you actually want a numeric conversion. See 
[Constants, Special Values, and Numerical Representation](../core/constants_representation.md) for floating-point masks and special
values.

## Complex arithmetic

KFR's [[`complex`:nosig]] values work as compound vector elements. Arithmetic on a
vector of complex values is complex arithmetic per lane, and the result keeps
the same vector shape. Operations use complex arithmetic independently in each
lane. This distinction is especially important for multiplication and
division, which are performed differently for vectors of complex values than
for vectors of scalar values.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/complex-components")
|||{
const vec<complex<f32>, 2> bins{
	complex<f32>{ 1.f, 2.f },
	complex<f32>{ 3.f, -1.f },
};

const auto conjugated = cconj(bins);
const auto real_parts = real(bins);
const auto imaginary_parts = imag(bins);
|||CHECK_THAT(real_parts, DeepMatcher(f32x2{ 1.f, 3.f }));
|||CHECK_THAT(imaginary_parts, DeepMatcher(f32x2{ 2.f, -1.f }));
|||CHECK(conjugated[0] == complex<f32>{ 1.f, -2.f });
|||CHECK(conjugated[1] == complex<f32>{ 3.f, 1.f });
|||}
```

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/complex-arithmetic")
|||{
const vec<complex<f32>, 2> a{
	complex<f32>{ 1.f, 2.f },
	complex<f32>{ 3.f, 1.f },
};
const vec<complex<f32>, 2> b{
	complex<f32>{ 2.f, -1.f },
	complex<f32>{ 1.f, 2.f },
};

const auto sums = a + b;
const auto differences = a - b;
const auto products = a * b;
const auto quotients = a / b;
|||CHECK(sums[0] == complex<f32>{ 3.f, 1.f });
|||CHECK(products[0] == complex<f32>{ 4.f, 3.f });
|||}
```

Here, `products` and `quotients` perform complex multiplication and division
per lane. `sums` and `differences` apply the usual component-wise complex
addition and subtraction independently to each lane.

[[`make_complex(T1, T2)`:nosig]] constructs complex scalar or vector values; [[`cconj(const T1 &)`:nosig]],
[[`real(const complex<T> &)`:nosig]], [[`imag(const complex<T> &)`:nosig]], [[`ccomp(const vec<T, N> &)`:nosig]], and [[`cdecom(const vec<complex<T>, N> &)`:nosig]] cover common component
operations.

## Comparisons belong to masks

Vector comparisons produce a [[`mask`:nosig]], not a single `bool`. Feed the mask to
[[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]] for lane-wise conditional values or reduce it with [[`any(const mask<T, N> &)`:nosig]] or
[[`all(const mask<T, N> &)`:nosig]] when a scalar decision is needed.

```c++
using namespace kfr;

|||TEST_CASE("arithmetic.md/comparisons")
|||{
const f32x4 samples{ 0.1f, 0.8f, -0.2f, 1.f };
const auto loud = abs(samples) > 0.5f;
const auto clipped = select(loud, 0.5f, samples);

if (any(loud))
{
	// At least one lane was above the threshold.
}
|||CHECK_THAT(clipped, DeepMatcher(f32x4{ 0.1f, 0.5f, -0.2f, 0.5f }));
|||}
```

Do not treat `&&` and `||` on masks as scalar short-circuit operations. They
form lane-wise mask combinations.

## See Also

- [The `vec` Type](vec.md)
- [Constants, Special Values, and Numerical Representation](../core/constants_representation.md)
