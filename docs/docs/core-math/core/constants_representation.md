# Constants, Special Values, and Numerical Representation

KFR provides typed mathematical constants for ordinary numerical code,
including values such as $\pi$, logarithms, epsilon, infinities, and NaN.
It also provides representation helpers for algorithms that deliberately
operate on the bits of scalar or SIMD values—for example, to preserve a sign
bit or build a mask.

## Typed Mathematical and Machine Constants

[[`constants<T>`:nosig]] collects `constexpr` constants for the immediate
scalar element type of `T`. It works for both scalars and SIMD vectors: for
example, `constants<f32>` and `constants<f32x4>` both provide `f32` constants.
The class form is useful in generic code where the type is already a template
parameter.

| Member                            | Meaning                                                        |
|-----------------------------------|----------------------------------------------------------------|
| `pi`, `sqr_pi`, `recip_pi`        | $\pi$, $\pi^2$, and $1 / \pi$                                  |
| `e`                               | Euler's number                                                 |
| `log_2`, `log_10`                 | Natural logarithms of 2 and 10                                 |
| `recip_log_2`, `recip_log_10`     | $1 / \ln(2)$ and $1 / \ln(10)$                                 |
| `sqrt_2`                          | $\sqrt{2}$                                                     |
| `degtorad`, `radtodeg`            | Angle conversion factors                                       |
| `epsilon`                         | The spacing between 1 and the next representable value above 1 |
| `infinity`, `neginfinity`, `qnan` | IEEE-style floating-point special values                       |

`pi_s(m, d)` and `recip_pi_s(m, d)` return scaled values such as $2\pi$ or
$\pi/4$ without introducing an untyped literal into generic code.

For direct use in expressions, KFR also supplies inline variable templates.
They produce the immediate scalar subtype of `T`, so `c_pi<f32x4>` is an `f32`
value suitable for vector broadcasting.

| Variable template                                                     | Meaning                                                                                  |
|-----------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| [[`c_pi`:nosig]], [[`c_sqr_pi`:nosig]], [[`c_recip_pi`:nosig]]        | $\pi$, $\pi^2$, and $1 / \pi$, each optionally scaled by template parameters `m` and `d` |
| [[`c_e`:nosig]], [[`c_sqrt_2`:nosig]]                                 | $e$ and $\sqrt{2}$, optionally scaled                                                    |
| [[`c_log_2`:nosig]], [[`c_log_10`:nosig]]                             | $\ln(2)$ and $\ln(10)$                                                                   |
| [[`c_recip_log_2`:nosig]], [[`c_recip_log_10`:nosig]]                 | $1 / \ln(2)$ and $1 / \ln(10)$                                                           |
| [[`c_epsilon`:nosig]]                                                 | Machine epsilon                                                                          |
| [[`c_infinity`:nosig]], [[`c_neginfinity`:nosig]], [[`c_qnan`:nosig]] | Positive infinity, negative infinity, and quiet NaN                                      |

```c++
|||#include <kfr/base.hpp>
using namespace kfr;

constexpr f64 quarter_turn = c_pi<f64, 1, 2>;
constexpr f32 log2e = c_recip_log_2<f32>;
|||TEST_CASE("constants_representation.md/pi_and_log")
|||{
|||CHECK(quarter_turn == Catch::Approx(3.14159265358979 / 2));
|||CHECK(log2e == Catch::Approx(1.4426950408889634f));
|||}
```

The constants are rounded to the destination scalar type. They are not a
promise that a later transcendental calculation is exact; use an error bound
appropriate to the calculation and its input range.

## Epsilon, Infinity, NaN, and Signed Zero

`epsilon` is not a universal tolerance. It measures local precision near 1,
not the expected accumulated error of a transform, filter, or reduction. A
useful comparison normally scales its tolerance to the magnitude and number of
operations involved:

```c++
bool nearly_equal(f64 a, f64 b)
{
		const f64 scale = max(1.0, max(abs(a), abs(b)));
		return abs(a - b) <= constants<f64>::epsilon * 16 * scale;
}
|||TEST_CASE("constants_representation.md/nearly_equal")
|||{
|||CHECK(nearly_equal(1.0, 1.0 + constants<f64>::epsilon));
|||CHECK_FALSE(nearly_equal(1.0, 1.1));
|||}
```

`c_infinity<T>` and `c_neginfinity<T>` are useful sentinels for floating-point
min/max reductions. `infinity - infinity`, `0 * infinity`, and invalid
operations such as `sqrt(-1)` produce NaN, and a NaN compares unequal to every
value, including itself — use the finite/NaN predicates instead of `<`, `>`,
or `==` when an API needs to reject or repair non-finite samples.

Floating point has both `+0.0` and `-0.0`. They compare equal, but their sign
bits differ and operations such as reciprocal, phase, and complex functions
can observe that difference. Do not replace a bit-preserving operation with
`x < 0` when negative zero must be retained.

> [!note]
> The infinity and NaN constants describe floating-point representations.
> `std::numeric_limits` does not provide meaningful infinities or NaNs for
> ordinary integer types; use integer limits for integer sentinel values.

## Mantissa and Bit Masks

[[`c_mantissa_bits`:nosig]] exposes the mantissa-field width for a type with a
32- or 64-bit immediate subtype. [[`c_mantissa_mask`:nosig]] provides the
corresponding low-bit mask in an integer subtype. They are low-level tools, not
numeric values: apply the mask to an integer representation obtained with
[[`ubitcast`:nosig]] or [[`bitcast`:nosig]], rather than to a floating-point
value with arithmetic operators.

```c++
|||TEST_CASE("constants_representation.md/mantissa_mask")
|||{
const f32 value = 3.14159f;
const u32 fraction = ubitcast(value) & c_mantissa_mask<u32>;
|||CHECK(fraction != 0);
|||}
```

[[`special_constants`:nosig]] supplies masks for the immediate scalar
subtype of `T`:

| Function                                          | Bit pattern                                   |
|---------------------------------------------------|-----------------------------------------------|
| [[`special_scalar_constants<T>::allzeros`]]       | Every bit clear                               |
| [[`special_scalar_constants<T>::allones`]]        | Every bit set                                 |
| [[`special_scalar_constants<T>::highbitmask`]]    | Only the most significant bit set             |
| [[`special_scalar_constants<T>::invhighbitmask`]] | Every bit except the most significant bit set |

For unsigned integers, these have their literal bit-pattern meanings. For
signed integers, `allones()` has value `-1`. For `float` and `double`, the
masks are *not generally ordinary finite numbers*: `highbitmask()` is negative
zero, `allones()` is a NaN bit pattern, and `invhighbitmask()` is a NaN bit
pattern with the sign bit clear. Their intended use is KFR's bitwise SIMD
operations, such as clearing a floating-point sign bit to implement absolute
value. Do not use floating-point equality or arithmetic to inspect a mask.

The [[`bit`:nosig]] type represents a SIMD predicate in mask form. A true
`bit<T>` stores all ones and false stores all zeros; converting it back to
`bool` tests the high bit. This is compatible with vector comparison and
selection operations, but a `bit<T>` is deliberately not implicitly
convertible to its raw element type.

## Reinterpreting Bits

[[`bitcast`:nosig]] preserves the object representation while
changing the type. Scalar casts require equal source and destination sizes.
Vector casts preserve the total byte count and adjust the number of lanes when
the element size changes. These are representation conversions, not numeric
conversions:

```c++
|||TEST_CASE("constants_representation.md/bitcast")
|||{
const f32 negative_zero = -0.0f;
const u32 bits = ubitcast(negative_zero); // 0x80000000
const f32 restored = bitcast<f32>(bits);  // still -0.0f

const f32 magnitude = bitcast<f32>(
		ubitcast(negative_zero) & 0x7fffffffu); // +0.0f
|||CHECK(bits == 0x80000000u);
|||CHECK(restored == negative_zero);
|||CHECK(magnitude == 0.0f);
|||}
```

Convenience forms choose a destination category with the same storage shape:

- [[`ubitcast`:nosig]] reinterprets as the unsigned counterpart.
- [[`ibitcast`:nosig]] reinterprets as the signed counterpart.
- [[`fbitcast`:nosig]] reinterprets as the floating counterpart.
- [[`uibitcast`:nosig]] reinterprets as the unsigned counterpart for signed
	inputs and leaves an unsigned input category unchanged.
- [[`bitcast_anything`:nosig]] is the general equal-size helper used by
	low-level code, including non-SIMD types.

Bit reinterpretation can create NaNs, infinities, subnormals, or unsupported
encodings from otherwise ordinary integers. Treat the result as raw data until
it has been validated for the operation that will consume it. In particular,
avoid arithmetic on values produced solely as masks and do not assume a NaN's
payload will survive every operation or compiler optimization unchanged.

## `special_value` for Generic Test Data

[[`special_value`:nosig]] is a type-erased value source that converts to a
requested numeric type. It can hold a `special_constant` tag, an integer
literal, or a floating-point literal — handy in generic tests and test-data
generators that need to exercise the same cases across several scalar or
vector types.

The available tags are `default_constructed`, `min`, `max`, `neg_max`,
`lowest`, `epsilon`, `infinity`, `neg_infinity`, and `random_bits`. The latter
fills the destination object's storage with pseudo-random bits and can
therefore produce non-finite floating-point values. It is suited to robustness
tests, not signal generation.

```c++
|||TEST_CASE("constants_representation.md/special_value")
|||{
const special_value ceiling = special_constant::max;
const f32 scalar_limit = ceiling;
const f32x4 vector_limit = f32x4(scalar_limit); // broadcasts through the normal vector conversion

const special_value sample = 0.25;
const f64 fraction = sample;
|||CHECK(scalar_limit == std::numeric_limits<f32>::max());
|||CHECK(fraction == 0.25);
|||}
```

`special_value` converts through the immediate subtype of a compound type, so
it can supply corresponding scalar limits to vectors. Prefer named constants
and explicit values in application code; `special_value` is intended for
parameterized fixtures and boundary-case coverage.

## Representation and Platform Alignment

[[`platform`:nosig]] exposes compile-time properties of a KFR target CPU,
including SIMD register sizes and alignment requirements. Its
`native_vector_alignment_mask`, `maximum_vector_alignment_mask`, and
`native_cache_alignment_mask` members are each one less than their associated
power-of-two alignment. They support the usual alignment test or rounding
pattern:

```c++
constexpr size_t alignment = platform<>::native_vector_alignment;
constexpr size_t alignment_mask = platform<>::native_vector_alignment_mask;

bool is_aligned(const void* pointer)
{
		return (reinterpret_cast<uintptr_t>(pointer) & alignment_mask) == 0;
}
|||TEST_CASE("constants_representation.md/alignment")
|||{
|||alignas(alignment) f32 buffer[8]{};
|||CHECK(is_aligned(buffer));
|||CHECK(alignment_mask == alignment - 1);
|||}
```

Use KFR containers and read/write APIs where possible; they arrange suitable
storage and avoid making alignment an application concern. Platform masks
describe addresses and storage alignment, whereas `special_constants` masks
describe bits inside a value—the two should not be interchanged.

## See Also

- [Numeric, Compound, and Complex Types](types.md)
- [SIMD vectors](../simd/vec.md)
- [Expressions](../../expressions/expressions.md)
