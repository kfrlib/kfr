# Masks, Comparisons, and Branchless Selection

A KFR SIMD comparison produces a [[`mask`:nosig]], not a vector of ordinary C++
`bool` values. A mask carries one predicate per lane and can be combined,
reduced to one `bool`, or used by [[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]] to choose a result per lane.
This keeps a data-parallel calculation in vector form without branching once
for every element.

Include `<kfr/simd.hpp>` for the types and operations in this article.

```c++
#include <kfr/simd.hpp>

using namespace kfr;

|||TEST_CASE("masks.md/select-basic")
|||{
const f32x4 samples{ -1.5f, -0.25f, 0.75f, 1.25f };
const auto in_range = (samples >= -1.f) && (samples <= 1.f);

const auto limited = select(in_range, samples, 0.f);
|||CHECK_THAT(limited, DeepMatcher(f32x4{ 0.f, -0.25f, 0.75f, 0.f }));
|||}
```

`limited` is `{ 0, -0.25, 0.75, 0 }`. The comparison is performed for all
lanes, and [[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]] takes its first value where the corresponding mask lane is
true and its second value otherwise.

## Mask representation

[[`mask`:nosig]] is an alias for `vec<bit<T>, N>`. Each [[`bit<T>`:nosig]] holds a
predicate in the same storage shape as the corresponding scalar type:

- a canonical true value has every bit set;
- a canonical false value has every bit clear.

`bit<T>` can convert to `bool`, but it deliberately does not convert to the
underlying raw `T`. Converting to `bool` tests the high bit of the mask value.
Normal KFR comparisons and [[`make_mask`:nosig]] produce canonical true and false
lanes; treat other bit patterns as representation-level data rather than
application predicates.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/mask-representation")
|||{
const mask<f32, 4> active = make_mask<f32>(true, false, true, false);

const bool lane0 = active[0]; // true
const bool lane1 = active[1]; // false
|||CHECK(lane0 == true);
|||CHECK(lane1 == false);
|||}
```

[[`make_mask`:nosig]] takes one or more boolean arguments. The element type must be
specified explicitly, and the number of arguments determines the mask width.
Use the element type and lane count that match the vectors you will select or
combine.

[[`maskfor`:nosig]] obtains the associated mask type from a SIMD vector type — useful
in generic SIMD code, e.g. `maskfor<f32x4>` is `mask<f32, 4>`. It maps a
vector type to its mask type, not a scalar value to a mask.

## Comparisons produce masks

The comparison operators `==`, `!=`, `<`, `<=`, `>`, and `>=` work lane by
lane for vector/vector, vector/scalar, and scalar/vector operands. Scalar
operands are broadcast, and the operands use KFR's common-type promotion.
The result has the same lane count as the vector operands.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/comparisons")
|||{
const i32x4 counts{ 3, 7, 11, 15 };

const auto above_ten = counts > 10;
const auto valid = (counts >= 5) && (counts <= 12);
|||CHECK_THAT(above_ten, DeepMatcher(make_mask<i32>(false, false, true, true)));
|||CHECK_THAT(valid, DeepMatcher(make_mask<i32>(false, true, true, false)));
|||}
```

The named comparison helpers are [[`equal`:nosig]], [[`notequal`:nosig]], [[`less`:nosig]],
[[`greater`:nosig]], [[`lessorequal`:nosig]], and [[`greaterorequal`:nosig]]. They express the
same lane-wise comparisons and are useful when an operator would be unclear in
generic code.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/named-comparisons")
|||{
const f32x4 x{ 0.f, 0.25f, 0.5f, 0.75f };
const auto lower_half = less(x, 0.5f);
const auto endpoints = equal(x, 0.f) || equal(x, 0.75f);
|||CHECK_THAT(lower_half, DeepMatcher(make_mask<f32>(true, true, false, false)));
|||CHECK_THAT(endpoints, DeepMatcher(make_mask<f32>(true, false, false, true)));
|||}
```

The named helpers are SIMD helpers: their arguments must form a KFR vector
common type with an associated mask. Use normal scalar C++ comparisons for
scalar-only code.

## Combining predicates

Mask `!` and `~` both complement every lane. The operators `&`, `|`, `^`,
`&&`, and `||` combine matching mask lanes. These are vector operations, not
ordinary scalar Boolean operators:

- `&&` and `||` do **not** short-circuit;
- both operands have already been evaluated before the operator combines their
	lanes;
- use parentheses when combining comparison expressions.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/combining-predicates")
|||{
const f32x4 x{ -2.f, -0.5f, 0.5f, 2.f };
const auto in_band = (x >= -1.f) && (x <= 1.f);
const auto outside = !in_band;
|||CHECK_THAT(in_band, DeepMatcher(make_mask<f32>(false, true, true, false)));
|||CHECK_THAT(outside, DeepMatcher(make_mask<f32>(true, false, false, true)));
|||}
```

Masks being combined must have equal lane counts and compatible element storage
sizes — a mask holds one predicate per lane, not an arbitrary collection of
independent `bool` objects.

## Testing all or any lanes

[[`all`:nosig]] and [[`any`:nosig]] reduce a mask to one scalar `bool`. Use them when a
vector predicate must control an ordinary branch or report one condition for a
whole block.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/all-any")
|||{
const f32x4 x{ -0.5f, 0.0f, 0.5f, 1.0f };
const auto normal_range = inrange(x, -1.f, 1.f);

if (all(normal_range))
{
	// Every lane is within the inclusive range.
}

if (any(x < 0.f))
{
	// At least one lane is negative by numeric comparison.
}
|||CHECK(all(normal_range));
|||CHECK(any(x < 0.f));
|||}
```

`all(mask)` is the logical AND reduction of its lanes; `any(mask)` is the
logical OR reduction. They are the transition from a lane-wise predicate to one
scalar decision.

## Branchless lane selection

[[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]] is the vector equivalent of a conditional expression:

```c++
||||||||||||
select(mask, when_true, when_false)
||||||||||||
```

For each lane, it returns the corresponding value from `when_true` when the
mask is true and from `when_false` otherwise. The two values may be vectors or
scalars; scalar values are broadcast. Their common element type determines the
result element type.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/branchless-select")
|||{
const f32x4 samples{ -1.5f, -0.25f, 0.75f, 1.25f };
const auto negative = samples < 0.f;

const auto half_wave = select(negative, 0.f, samples);
const auto signed_unit = select(samples >= 0.f, 1.f, -1.f);
|||CHECK_THAT(half_wave, DeepMatcher(f32x4{ 0.f, 0.f, 0.75f, 1.25f }));
|||CHECK_THAT(signed_unit, DeepMatcher(f32x4{ -1.f, -1.f, 1.f, 1.f }));
|||}
```

[[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]] is branchless with respect to its lanes, but it does not defer its
value arguments: ordinary C++ evaluates `when_true` and `when_false` before
the call. Do not use it to avoid an expensive or unsafe computation in one
branch; make both arguments safe to evaluate first.

## Classification and range predicates

KFR provides lane-wise classification functions for vector values:

| Function | True lanes |
|----------|------------|
| [[`isnan`:nosig]] | Values that are not equal to themselves. |
| [[`isinf`:nosig]] | Positive or negative infinity. |
| [[`isfinite`:nosig]] | Values that are neither NaN nor infinite. |
| [[`isnegative`:nosig]] | Values whose representation has the sign bit set. |
| [[`ispositive`:nosig]] | Values whose sign bit is clear. |
| [[`iszero`:nosig]] | Values equal to zero. |
| [[`inrange`:nosig]] | Values in the inclusive range $[\mathrm{min}, \mathrm{max}]$. |

```c++
using namespace kfr;

|||TEST_CASE("masks.md/classification")
|||{
const f32x4 x{ -0.0f, 0.25f, 2.0f, -2.0f };

const auto usable = isfinite(x) && inrange(x, -1.f, 1.f);
const auto sign_set = isnegative(x);
const auto zero = iszero(x);
|||CHECK_THAT(usable, DeepMatcher(make_mask<f32>(true, true, false, false)));
|||CHECK_THAT(sign_set, DeepMatcher(make_mask<f32>(true, false, false, true)));
|||CHECK_THAT(zero, DeepMatcher(make_mask<f32>(true, false, false, false)));
|||}
```

[[`isnegative(const vec<T, N> &)`:nosig]] examines the sign bit, so it reports true for `-0.0f`. By
contrast, [[`iszero(const vec<T, N> &)`:nosig]] uses equality and reports true for both `-0.0f` and `+0.0f`.
[[`ispositive`:nosig]] is the complement of this sign-bit test; it means
*sign-bit clear*, not strictly numerically greater than zero. Use `x > 0` for
the latter condition.

The non-finite classification helpers are intended for floating-point vector
data. For integer data, use the comparisons that describe the actual condition
you need.

## A complete per-lane validation example

The following expression accepts finite samples in an inclusive range, replaces
the rest with zero, and separately reports whether the block contains any
rejected lanes.

```c++
using namespace kfr;

|||TEST_CASE("masks.md/complete-example")
|||{
const f32x4 input{ -1.5f, -0.25f, 0.5f, constants<f32>::infinity };
const auto accepted = isfinite(input) && inrange(input, -1.f, 1.f);
const f32x4 cleaned = select(accepted, input, 0.f);

const bool had_rejected_samples = any(!accepted);
|||CHECK_THAT(cleaned, DeepMatcher(f32x4{ 0.f, -0.25f, 0.5f, 0.f }));
|||CHECK(had_rejected_samples);
|||}
```

This separates the SIMD data path (`accepted` and `cleaned`) from the one
scalar decision (`had_rejected_samples`).

## See Also

- [The `vec` Type](vec.md)
- [Arithmetic, Bitwise Operations, and Type Conversion](arithmetic.md)
- [Constants, Special Values, and Numerical Representation](../core/constants_representation.md)
