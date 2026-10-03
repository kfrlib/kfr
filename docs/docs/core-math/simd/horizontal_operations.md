# Horizontal Operations and Small Vector Algorithms

Most SIMD arithmetic is *vertical*: each output lane depends on the matching
input lanes. A horizontal operation combines lanes and returns one scalar
result. Use horizontal reductions for a small fixed vector that is already in
registers, such as a dot product, channel-frame statistic, or a small sorting
network input.

Include `<kfr/simd.hpp>` for the APIs in this article.

```c++
#include <kfr/simd.hpp>

using namespace kfr;

|||TEST_CASE("horizontal_operations.md/hsum")
|||{
const f32x4 values{ 1.f, 2.f, 3.f, 4.f };
const f32 total = hsum(values); // 10
|||CHECK(total == 10.f);
|||}
```

For reductions over a run-time-sized sequence, use KFR's [expression reductions](../../expressions/statistics.md)
instead. These functions operate on one fixed-size [[`vec`:nosig]].

## The generic `horizontal` reduction

[[`horizontal`:nosig]] reduces a vector with a KFR reduction function:

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/horizontal")
|||{
const i32x4 values{ 3, 7, 2, 5 };
const i32 minimum = horizontal(values, fn::min());
|||CHECK(minimum == 2);
|||}
```

The reduction is structurally a balanced tree, not a scalar left-to-right fold.
For four lanes and reduction $r$, the shape is:

$$
r(r(x_0, x_1), r(x_2, x_3)).
$$

For a width that is not a power of two, KFR pads to the next power of two with
the reduction's identity value, then uses the same tree. Use KFR reducers such
as `fn::add()`, `fn::mul()`, `fn::min()`, and `fn::max()` for generic
reductions; a simple binary-only lambda does not supply the identity needed for
all widths.

The tree structure is important for floating-point work. Floating addition and
multiplication are not associative, so the result need not match a scalar
left-to-right loop bit for bit. The functions do not use compensated summation
or a wider accumulator.

## Sums and products

[[`hadd`:nosig]] and [[`hsum`:nosig]] are aliases: each sums every lane. [[`hmul`:nosig]] and
[[`hproduct`:nosig]] are likewise aliases that multiply every lane.

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/sums_and_products")
|||{
const i32x4 values{ 1, 2, 3, 4 };

const i32 sum_a = hadd(values);     // 10
const i32 sum_b = hsum(values);     // 10
const i32 product_a = hmul(values); // 24
const i32 product_b = hproduct(values); // 24
|||CHECK(sum_a == 10);
|||CHECK(sum_b == 10);
|||CHECK(product_a == 24);
|||CHECK(product_b == 24);
|||}
```

The result type is the vector's element type. Integer sums and products do not
automatically promote to a wider accumulator, so select a sufficiently wide
element type before reducing when overflow is possible.

## Dot product, average, and RMS

[[`hdot`:nosig]], [[`havg`:nosig]], and [[`hrms`:nosig]] compose common arithmetic reductions:

$$
\begin{aligned}
\operatorname{hdot}(x, y) &= \operatorname{hadd}(x \cdot y), \\
\operatorname{havg}(x) &= \operatorname{hadd}(x) / N, \\
\operatorname{hrms}(x) &= \sqrt{\operatorname{hadd}(x \cdot x) / N}.
\end{aligned}
$$

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/dot_avg_rms")
|||{
const f32x4 x{ 1.f, 2.f, 3.f, 4.f };
const f32x4 y{ 2.f, 0.f, -1.f, 3.f };

const f32 dot = hdot(x, y); // 11
const f32 average = havg(x); // 2.5
const f32 rms = hrms(x); // sqrt(7.5)
|||CHECK(dot == 11.f);
|||CHECK(average == 2.5f);
|||CHECK_THAT(rms, Catch::Matchers::WithinAbs(std::sqrt(7.5f), 1e-5f));
|||}
```

`hdot` multiplies in the element type before summing. `hrms` squares in the
element type before summing. Use floating-point vectors for RMS calculations
unless you have accounted for integer multiplication overflow.

[[`havg`:nosig]] also retains the element type. Integer vectors therefore use integer
division, which truncates a non-integral mean:

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/havg_integer")
|||{
const i32x4 values{ 1, 2, 3, 4 };
const i32 average = havg(values); // 2, not 2.5
|||CHECK(average == 2);
|||}
```

Convert to `f32`, `f64`, or the selected [[`fbase`:nosig]] type before reducing when
a fractional result is required.

## Horizontal minimum and maximum

[[`hmin`:nosig]] and [[`hmax`:nosig]] return the smallest and largest lane:

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/hmin_hmax")
|||{
const i32x4 values{ 4, -3, 8, 1 };

const i32 smallest = hmin(values); // -3
const i32 largest = hmax(values);  // 8
|||CHECK(smallest == -3);
|||CHECK(largest == 8);
|||}
```

For non-power-of-two widths, KFR pads the reduction with an identity value that
does not affect an ordinary minimum or maximum. These operations delegate to
the selected platform `min`/`max` implementation, so don't rely on a
particular NaN ordering or signed-zero tie behavior across targets.

## Bitwise reductions

[[`hbitwiseand`:nosig]], [[`hbitwiseor`:nosig]], and [[`hbitwisexor`:nosig]] reduce an integer or
bit-representation vector with the corresponding bitwise operation.

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/bitwise_reductions")
|||{
const u32x4 flags{ 0b0111u, 0b0011u, 0b0101u, 0b0001u };

const u32 common = hbitwiseand(flags); // 0b0001
const u32 combined = hbitwiseor(flags); // 0b0111
|||CHECK(common == 0b0001u);
|||CHECK(combined == 0b0111u);
|||}
```

For predicates, use [[`all`:nosig]] and [[`any`:nosig]] on a [[`mask`:nosig]] rather than
manually reducing its raw representation.

## Sorting a small fixed vector

[[`sort`:nosig]] returns a new vector in ascending order; [[`sortdesc`:nosig]] returns a
new vector in descending order. Neither changes the input vector.

```c++
using namespace kfr;

|||TEST_CASE("horizontal_operations.md/sort")
|||{
const i32x4 input{ 1000, 1, 2, -10 };

const auto ascending = sort(input);
// { -10, 1, 2, 1000 }

const auto descending = sortdesc(input);
// { 1000, 2, 1, -10 }
|||CHECK_THAT(ascending, DeepMatcher(i32x4{ -10, 1, 2, 1000 }));
|||CHECK_THAT(descending, DeepMatcher(i32x4{ 1000, 2, 1, -10 }));
|||}
```

These are fixed-size SIMD sorting operations, not replacements for a
general-purpose container sort. They do not take a comparator and do not
promise stability. Use power-of-two vector widths of at least two; that is the
practical domain supported by the current implementation.

As with [[`hmin`:nosig]] and [[`hmax`:nosig]], do not rely on a prescribed order for NaNs
or equal signed-zero values. If a data format needs a total order, define and
apply that ordering before sorting.

## Numerical considerations

Horizontal operations are compact and efficient, but they preserve the
underlying arithmetic type and its limits:

1. **Floating-point order matters.** A tree reduction can round differently
	from a sequential loop. Avoid assuming bitwise-identical results across a
	changed target, compiler mode, or hand-written accumulation order.
2. **Integers do not widen automatically.** Sums, products, dot products, and
	RMS intermediate squares can overflow in the original element type.
3. **RMS can overflow before square root.** Scale or convert integer samples to
	floating point when needed.
4. **Min/max special values need a policy.** Filter or handle NaNs explicitly
	when their presence is meaningful to the application.

## Choosing a reduction

| Need | Operation |
|------|-----------|
| Sum or product | [[`hsum`:nosig]] / [[`hproduct`:nosig]] |
| Dot product | [[`hdot`:nosig]] |
| Arithmetic mean | [[`havg`:nosig]] |
| Root-mean-square level | [[`hrms`:nosig]] |
| Smallest or largest lane | [[`hmin`:nosig]] / [[`hmax`:nosig]] |
| Bit flags across lanes | [[`hbitwiseand`:nosig]], [[`hbitwiseor`:nosig]], or [[`hbitwisexor`:nosig]] |
| Predicate reduction | [[`all`:nosig]] or [[`any`:nosig]] |
| Ascending or descending small-vector order | [[`sort`:nosig]] / [[`sortdesc`:nosig]] |

## See Also

- [The `vec` Type](vec.md)
- [Arithmetic, Bitwise Operations, and Type Conversion](arithmetic.md)
- [Masks, Comparisons, and Branchless Selection](masks.md)
