# Statistics and histograms

KFR reductions turn a finite expression of any rank into one scalar. They are
useful for signal levels, extrema, correlation, and summary statistics without
first materializing an intermediate container. Histograms use the same
expression traversal model, either collecting a complete result immediately or
collecting counts as values pass through an expression.

## Reductions

[[`reduce`:nosig]] is the general reduction primitive. It applies a transform to
each input value, combines transformed values with a binary reduction function,
and optionally applies a finalizer to the accumulated result:

```c++
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("expressions/statistics.md/reduce")
|||{
univector<float> samples = { -0.5f, 0.25f, 1.0f, -0.75f };

const auto cube = [](auto x) { return x * x * x; };
const float sum_of_cubes = reduce(samples, fn::add(), cube);
const float average_cube = reduce(samples, fn::add(), cube, fn::final_mean());
|||    CHECK(sum_of_cubes == 0.46875f);
|||    CHECK(average_cube == 0.1171875f);
|||}
```

The reducer must have an identity value and should be associative for useful
results. KFR accumulates vector blocks and performs a final horizontal
reduction, rather than making a left-to-right scalar fold. This is why ordinary
arithmetic reductions are a good fit, while an order-dependent custom operation
is not.

A finalizer can accept either the accumulated value alone or the accumulated
value and the number of processed elements. [[`final_mean`:nosig]] divides an
accumulated sum by that count, and [[`final_rootmean`:nosig]] computes the
square root after that division. They are the finalizers used by [[`mean`:nosig]]
and [[`rms`:nosig]], respectively.

### Built-in reductions

The following functions reduce a finite input expression:

| Function | Result |
| --- | --- |
| [[`sum`:nosig]] | $\sum_i x_i$ |
| [[`mean`:nosig]] | $\frac{1}{N}\sum_i x_i$ |
| [[`minof`:nosig]] / [[`maxof`:nosig]] | Smallest / greatest value |
| [[`absminof`:nosig]] / [[`absmaxof`:nosig]] | Smallest / greatest magnitude; the result is the magnitude, not the original signed value |
| [[`sumsqr`:nosig]] | $\sum_i x_i^2$ |
| [[`rms`:nosig]] | $\sqrt{\frac{1}{N}\sum_i x_i^2}$ |
| [[`product`:nosig]] | $\prod_i x_i$ |
| [[`dotproduct`:nosig]] | $\sum_i x_i y_i$ |
| [[`variance`:nosig]] | Population variance, with denominator $N$ |
| [[`stddev`:nosig]] | Population standard deviation, $\sqrt{\operatorname{variance}(x)}$ |

For example, a common signal-analysis pass can be expressed directly:

```c++
|||TEST_CASE("expressions/statistics.md/signal-analysis")
|||{
univector<float> samples = { -0.5f, 0.25f, 1.0f, -0.75f };
univector<float> left = { 1.0f, 2.0f, 3.0f, 4.0f };
univector<float> right = { 4.0f, 3.0f, 2.0f, 1.0f };

const float peak   = absmaxof(samples);
const float level  = rms(samples);
const float energy = sumsqr(samples);

// The inputs combine lazily; no product vector is allocated.
const float correlation = dotproduct(left, right);
|||    CHECK(peak == 1.0f);
|||    CHECK(level == Catch::Approx(0.6846532f));
|||    CHECK(energy == 1.875f);
|||    CHECK(correlation == 20.0f);
|||}
```

[[`variance`:nosig]] uses a shifted-data calculation with the first value as a
reference. This improves the calculation when values have a large common
offset, but it still computes population rather than sample variance. For
nonempty input $k = x_0$, its result is

$$
\operatorname{variance}(x) =
\frac{1}{N}\sum_i(x_i-k)^2 -
\left(\frac{1}{N}\sum_i(x_i-k)\right)^2.
$$

### Finite expressions, types, and precision

Reductions need a finite traversal. Infinite sources such as [[`counter`:nosig]]
or random generators must be bounded first, for example with
[[`truncate`:nosig]]:

```c++
|||TEST_CASE("expressions/statistics.md/bounded-source")
|||{
const float first_1024_rms = rms(truncate(counter<float>(), 1024));
|||    CHECK(first_1024_rms == Catch::Approx(590.773620605f));
|||}
```

Do not reduce an empty expression. In particular, mean and RMS divide by zero,
and variance reads the first element.

The standard helpers calculate in the expression's element type. Integer mean
therefore uses integer division; integer squares, sums, products, and dot
products can overflow. Convert to a wider or floating-point expression before
reducing if that is required:

```c++
|||TEST_CASE("expressions/statistics.md/wider-accumulator")
|||{
univector<int16_t> pcm = { -32768, 0, 16384, 32767 };
const double level = rms(cast<double>(pcm));
|||    CHECK(level == Catch::Approx(24575.6666694924060721f));
|||}
```

Floating-point addition and multiplication are not associative. Since KFR
combines SIMD blocks and lanes in an implementation-selected tree, the final
bit pattern can differ from a scalar loop, and may also differ when the
vectorization or traversal changes. The independent intermediate lane
accumulators also make the longest accumulation sequence roughly the number of
lanes shorter than in a scalar loop, which generally improves numerical
accuracy. This is normal floating-point accumulation behavior; use a fixed
processing configuration when reproducibility at that level matters.

## Histograms

[[`histogram_data<Bins, TCount>`:nosig]] stores regular bin counts and separate
counters for underflow and overflow. `Bins` selects the bin-count form:

* `histogram_data<0>` has a runtime bin count, supplied to its constructor or
  to the factory that creates it.
* `histogram_data<Bins>` has `Bins` regular bins fixed at compile time.

Counts are `uint32_t` by default and can be changed with `TCount`. Use
[[`histogram_data<Bins, TCount>::size`:nosig]] for the number of regular bins,
`operator[]` for an individual bin,
[[`histogram_data<Bins, TCount>::values`:nosig]] for a view of all regular
bins, and [[`histogram_data<Bins, TCount>::below`:nosig]] /
[[`histogram_data<Bins, TCount>::above`:nosig]] for values outside the
representable range. [[`histogram_data<Bins, TCount>::total`:nosig]] includes
all values, including those two outside counters.

### Mapping values to bins

Integer and floating-point data use deliberately different domains:

* An integer value $i$ in $[0, B-1]$ increments bin $i$. Negative values
  increment `below()`, and values greater than or equal to $B$ increment
  `above()`.
* A floating-point value is expected in the inclusive interval $[0, 1]$.
  Values below zero and above one increment the outside counters. In-range
  values use equal-width bins: bin $j$ represents $[j/B, (j+1)/B)$, except
  that the final bin also includes `1.0`. Equivalently, KFR uses
  $\min(\lfloor Bx \rfloor, B-1)$. Define `KFR_HISTOGRAM_OLD` before including
  KFR to retain the previous nearest-bin mapping.

Choose a positive bin count. KFR does not use zero bins as a valid histogram.

[[`histogram(E &&, size_t)`]] traverses a finite expression immediately
and returns the completed data object. The
[[`histogram(E &&)`]] overload selects a compile-time bin count:

```c++
|||TEST_CASE("expressions/statistics.md/histogram")
|||{
univector<int> codes = { 0, 1, 1, 3, -1, 5 };
auto code_counts = histogram<4>(codes);
// bins: { 1, 2, 0, 1 }, below: 1, above: 1

univector<float> normalized = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
auto levels = histogram(normalized, 32); // 32 runtime-selected bins
|||    CHECK(code_counts[0] == 1);
|||    CHECK(code_counts[1] == 2);
|||    CHECK(code_counts[2] == 0);
|||    CHECK(code_counts[3] == 1);
|||    CHECK(code_counts.below() == 1);
|||    CHECK(code_counts.above() == 1);
|||    CHECK(levels.total() == normalized.size());
|||}
```

### Collecting while an expression flows

[[`histogram_expression(E &&, size_t)`:nosig]] wraps an expression and returns
the original values unchanged while updating its `data` member. Its
[[`histogram_expression(E &&)`:nosig]] overload selects a compile-time bin
count. It is a useful side effect in an otherwise lazy pipeline:

```c++
|||TEST_CASE("expressions/statistics.md/flowing-histogram")
|||{
auto source = counter<float>();
auto observed = histogram_expression<16>(source * 0.5f + 0.5f);
sink(truncate(observed, 4096)); // evaluates 4096 values and fills observed.data

const auto& counts = observed.data;
|||    CHECK(counts.total() == 4096);
|||}
```

Like any lazy expression, the wrapper changes nothing until an evaluation point
such as [[`sink`:nosig]], assignment, or [[`render`:nosig]] traverses it. Its
histogram is cumulative: evaluating the same wrapper again adds another pass of
counts. Construct a fresh wrapper when a separate histogram is wanted.

## See also

- [Expression fundamentals](fundamentals.md), including finite materialization
  and [[`sink`:nosig]]
- [Sources and adaptors](sources.md)
