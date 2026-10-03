# Expression sources and adaptors

This page covers the small expression building blocks in `kfr/base.hpp`.
Most sources are infinite by default. This is convenient for signals and
procedural data, but a dynamic container needs a finite extent. Use
[[`truncate(Arg &&, std::type_identity_t<shape<Dims>>)`:nosig]],
[[`render(Expr &&, size_t, size_t)`:nosig]], or
[[`trender(const E &, shape<Traits::dims>)`:nosig]] to establish that bound.

## Constant and generated sources

### Scalar constants

[[`scalar(T)`:nosig]] wraps one explicit scalar expression. It has rank zero,
so it broadcasts wherever a larger expression expects a value.
[[`zeros()`:nosig]] and [[`ones()`:nosig]] are scalar expressions holding zero
and one; their element type defaults to `fbase` and can be specified:

```c++
|||#include <kfr/base.hpp>
|||using namespace kfr;
|||TEST_CASE("sources.md/scalar constants")
|||{
univector<int, 4> zeroes = zeros<int>();
univector<float, 4> gains = ones<float>() * 0.5f;
|||CHECK_THAT(zeroes, DeepMatcher(univector<int, 4>{ 0, 0, 0, 0 }));
|||CHECK_THAT(gains, DeepMatcher(univector<float, 4>{ 0.5f, 0.5f, 0.5f, 0.5f }));
|||}
```

### Counters

[[`counter(T)`:nosig]] is an infinite 1D arithmetic source beginning at
`start` with a step of one. The multi-axis form
[[`counter(T, Arg, Args...)`:nosig]] accepts one step per axis. Its value is

$$
start + step_0 i_0 + step_1 i_1 + \cdots
$$

```c++
|||TEST_CASE("sources.md/counters")
|||{
auto samples = render(counter(10, 2), 4);
// { 10, 12, 14, 16 }

auto grid = trender(truncate(counter(0, 10, 1), shape{ 2, 3 }));
// {{0, 1, 2}, {10, 11, 12}}
|||CHECK_THAT(samples, DeepMatcher(univector<int>{ 10, 12, 14, 16 }));
|||CHECK_THAT(grid, DeepMatcher(tensor<int, 2>{ { 0, 1, 2 }, { 10, 11, 12 } }));
|||}
```

### Lambda-backed expressions

[[`lambda(Fn &&, cbool_t<RandomAccess>)`:nosig]] creates an infinite
expression from a callable. Specify its value type and rank as template
arguments. The callable may receive an index `shape<Dims>`, an index plus
vectorization parameters, or no arguments. An index-only lambda is usually the
clearest choice:

```c++
|||TEST_CASE("sources.md/lambda-backed expressions")
|||{
auto checkerboard = lambda<int, 2>([](shape<2> index)
{
    return (index[0] + index[1]) % 2;
});

auto image = trender(checkerboard, shape{ 4, 6 });
|||CHECK(image(0, 0) == 0);
|||CHECK(image(0, 1) == 1);
|||CHECK(image(1, 1) == 0);
|||}
```

[[`lambda_generator(Fn &&)`:nosig]] has the same calling forms but declares
that it is not random-access. Use it for stateful generation where values must
be consumed in traversal order. Bound it before materializing it, just as with
other infinite generators.

### Repeating and evenly spaced sequences

[[`sequence(const Ts &...)`:nosig]] repeats a supplied value list forever:

```c++
|||TEST_CASE("sources.md/repeating sequences")
|||{
auto pattern = render(sequence(0.0f, 0.5f, 1.0f), 7);
// { 0, 0.5, 1, 0, 0.5, 1, 0 }
|||CHECK_THAT(pattern, DeepMatcher(univector<float>{ 0.f, 0.5f, 1.f, 0.f, 0.5f, 1.f, 0.f }));
|||}
```

[[`linspace(T1, T2, size_t, bool, cbool_t<truncated>)`:nosig]] produces evenly
spaced values between `start` and `stop`. If `endpoint` is true, `stop` is the
last of `size` values; otherwise it is excluded. Its final `ctrue` argument
makes the result finite; without it, the source remains infinite after the
requested interval.

```c++
|||TEST_CASE("sources.md/linspace")
|||{
auto inclusive = render(linspace(0.0, 1.0, 5, true, ctrue));
// { 0, 0.25, 0.5, 0.75, 1 }

auto exclusive = render(linspace(0.0, 1.0, 4, false, ctrue));
// { 0, 0.25, 0.5, 0.75 }
|||CHECK_THAT(inclusive, DeepMatcher(univector<double>{ 0.0, 0.25, 0.5, 0.75, 1.0 }));
|||CHECK_THAT(exclusive, DeepMatcher(univector<double>{ 0.0, 0.25, 0.5, 0.75 }));
|||}
```

[[`symmlinspace(T, size_t, cbool_t<truncated>)`:nosig]] is the symmetric form,
spanning `[-symsize, +symsize]` with endpoints included:

```c++
|||TEST_CASE("sources.md/symmlinspace")
|||{
auto positions = render(symmlinspace(3.0, 4, ctrue));
// { -3, -1, 1, 3 }
|||CHECK_THAT(positions, DeepMatcher(univector<double>{ -3.0, -1.0, 1.0, 3.0 }));
|||}
```

[[`arange(T, cbool_t<truncated>)`:nosig]] and
[[`arange(T, T, T, cbool_t<truncated>)`:nosig]] are always finite. They produce
`[0, stop)` or `[start, stop)` with a given step:

```c++
|||TEST_CASE("sources.md/arange")
|||{
auto indices = render(arange(2, 10, 3));
// { 2, 5, 8 }
|||CHECK_THAT(indices, DeepMatcher(univector<float>{ 2, 5, 8 }));
|||}
```

Use a nonzero step that moves from `start` toward `stop`.

## Selecting and extending an expression

### `slice` and `truncate`

[[`slice(Arg &&, std::type_identity_t<shape<Dims>>, std::type_identity_t<shape<Dims>>)`:nosig]]
creates a rank-preserving view starting at `start`, with an optional maximum
`size`. Its actual extent is clamped to the source. It forwards reads and,
where possible, writes to its operand.

[[`truncate(Arg &&, std::type_identity_t<shape<Dims>>)`:nosig]] is a slice from
the origin. It is the standard way to turn an infinite generator into a finite
expression:

```c++
|||TEST_CASE("sources.md/slice and truncate")
|||{
auto middle = render(slice(counter(), 100, 4));
// { 100, 101, 102, 103 }

univector<float> finite = truncate(counter<float>(), 256);
|||CHECK_THAT(middle, DeepMatcher(univector<int>{ 100, 101, 102, 103 }));
|||CHECK(finite.size() == 256);
|||}
```

For multidimensional sources, pass shapes:

```c++
|||TEST_CASE("sources.md/multidimensional slice")
|||{
auto block = slice(counter(0, 10, 1), shape{ 2, 3 }, shape{ 2, 4 });
|||CHECK_THAT(trender(block), DeepMatcher(tensor<int, 2>{ { 23, 24, 25, 26 }, { 33, 34, 35, 36 } }));
|||}
```

### Padding and reversal

[[`padded(Arg &&, T)`:nosig]] extends an expression to an infinite source.
Requests inside the original shape read from the operand; requests outside it
return the fill value.

```c++
|||TEST_CASE("sources.md/padded")
|||{
auto padded_signal = render(padded(truncate(counter(), 3), -1), 6);
// { 0, 1, 2, -1, -1, -1 }
|||CHECK_THAT(padded_signal, DeepMatcher(univector<int>{ 0, 1, 2, -1, -1, -1 }));
|||}
```

[[`reverse(Arg &&)`:nosig]] reverses the final axis. Its operand must support
random access, and its shape is unchanged:

```c++
|||TEST_CASE("sources.md/reverse")
|||{
auto backwards = render(reverse(truncate(counter(), 5)));
// { 4, 3, 2, 1, 0 }
|||CHECK_THAT(backwards, DeepMatcher(univector<int>{ 4, 3, 2, 1, 0 }));
|||}
```

## Changing shape and rank

[[`reshape(Arg &&, const shape<OutDims> &)`:nosig]] exposes the same logical
sequence under another shape. It maps through row-major flat indices, so a
1D sequence `{0, 1, 2, 3, 4, 5}` reshaped to `{2, 3}` reads as
`{{0, 1, 2}, {3, 4, 5}}`. Supply a shape with a compatible total element count:

```c++
|||TEST_CASE("sources.md/reshape")
|||{
auto matrix = trender(reshape(truncate(counter(), 6), shape{ 2, 3 }));
|||CHECK_THAT(matrix, DeepMatcher(tensor<int, 2>{ { 0, 1, 2 }, { 3, 4, 5 } }));
|||}
```

[[`fixshape(Arg &&, const fixed_shape_t<ShapeValues...> &)`:nosig]] is
intentionally different. It supplies a compile-time shape declaration but does
not remap elements. It is primarily useful when a generic algorithm benefits
from a statically known extent:

```c++
|||TEST_CASE("sources.md/fixshape")
|||{
auto values = truncate(counter<float>(), 16);
auto fixed = fixshape(values, fixed_shape<16>);
|||static_assert(expression_dims<decltype(fixed)> == 1);
|||CHECK(get_shape(fixed).front() == 16);
|||}
```

[[`dimensions(E1 &&)`:nosig]] raises an expression to a specified rank by
prepending infinite, broadcast axes. It does not duplicate storage:

```c++
|||TEST_CASE("sources.md/dimensions")
|||{
auto constant_line = truncate(dimensions<1>(scalar(1)), 5);
// { 1, 1, 1, 1, 1 }
|||CHECK_THAT(render(constant_line), DeepMatcher(univector<int>{ 1, 1, 1, 1, 1 }));
|||}
```

## Joining and routing expressions

[[`concatenate(Arg1 &&, Arg2 &&)`:nosig]] joins same-rank, same-value-type
expressions along axis zero by default. A template axis selects another axis;
the three-argument overload joins three inputs. Non-concatenated extents are
limited to their common region.

```c++
|||TEST_CASE("sources.md/concatenate")
|||{
auto joined = render(concatenate(
    truncate(counter(5, 0), 3),
    truncate(counter(10, 0), 2)));
// { 5, 5, 5, 10, 10 }
|||CHECK_THAT(joined, DeepMatcher(univector<int>{ 5, 5, 5, 10, 10 }));
|||}
```

[[`pack(Args &&...)`:nosig]] combines aligned scalar expressions into a
vector-valued expression. It is useful for assembling interleaved channels.
[[`unpack(E &&...)`:nosig]] is the output counterpart: it accepts vector values
and distributes their lanes to several scalar destinations.

```c++
|||TEST_CASE("sources.md/pack and unpack")
|||{
univector<float> left  = truncate(counter(0, 1), 3);
univector<float> right = truncate(counter(100, 1), 3);

// Each value is a two-lane vector: {left[i], right[i]}.
auto stereo = pack(left, right);
|||univector<f32x2> stereo_values = render(stereo);
|||CHECK(all(stereo_values[0] == f32x2{ 0, 100 }));
|||CHECK(all(stereo_values[1] == f32x2{ 1, 101 }));
|||CHECK(all(stereo_values[2] == f32x2{ 2, 102 }));

// Writing a packed expression writes its lanes back to the channels.
pack(left, right) *= broadcast<2>(10.0f);
|||CHECK_THAT(left, DeepMatcher(univector<float>{ 0, 10, 20 }));
|||CHECK_THAT(right, DeepMatcher(univector<float>{ 1000, 1010, 1020 }));
|||}
```

## Stateful and diagnostic adaptors

[[`adjacent(Fn &&, E1 &&)`:nosig]] calls `fn(current, previous)` for each
value. The previous value begins as zero. It is stateful and not random-access,
so construct a fresh expression for each independent traversal:

```c++
|||TEST_CASE("sources.md/adjacent")
|||{
auto products = render(adjacent(fn::mul(), counter()), 5);
// { 0, 0, 2, 6, 12 }
|||CHECK_THAT(products, DeepMatcher(univector<int>{ 0, 0, 2, 6, 12 }));
|||}
```

[[`trace(E1 &&)`:nosig]] returns the same values as its input while printing
requested vector blocks and their indices. It is a debugging aid rather than a
stable presentation format: what is printed depends on traversal and
vectorization.

```c++
|||TEST_CASE("sources.md/trace")
|||{
render(trace(counter()), 16); // prints the evaluated blocks to the console
|||}
```
