# Expression fundamentals

This page expands on the expression model introduced in
[Expressions and containers](expressions.md). It defines the shape and
access rules that apply while composing, evaluating, or implementing an
expression.

## Shapes and expression traits

Every expression has a value type and a rank (`dims`) known at compile time.
Its extent in each axis can be fixed, discovered at run time, or infinite. A
[[`shape<dims>`:nosig]] holds those extents; `shape<0>` represents a scalar.
KFR uses row-major axis order, so the final axis is normally contiguous and is
the one processed with SIMD vectors.

[[`expression_traits<T>`:nosig]] is the customization point for an expression
type. It supplies its `value_type`, its compile-time `dims`, and both static and
runtime forms of `get_shape`:

```c++
||||||||||||
using value_type = float;
constexpr static size_t dims = 1;

constexpr static shape<1> get_shape(const my_expression& self)
{
    return { self.size };
}
constexpr static shape<1> get_shape()
{
    return { undefined_size };
}
||||||||||||
```

Deriving the traits from [[`expression_traits_defaults`:nosig]] provides the
usual defaults. A fixed-size source can return its size from both overloads;
use `undefined_size` for a runtime-only extent. `infinite_size` marks an
unbounded axis. The rank itself must always be known at compile time.

The [[`input_expression`:nosig]] concept describes a source that can be read.
The [[`output_expression`:nosig]] concept describes a destination that can be
written. A type satisfying both is an
[[`input_output_expression`:nosig]]. These concepts are normally useful in
constraints for a generic function rather than as types to instantiate.

## Querying expression properties

The expression-trait helpers make an expression's compile-time properties
available without referring to `expression_traits<T>` directly:

* [[`expression_value_type`:nosig]] is the scalar value type produced by
  the expression.
* [[`expression_dims`:nosig]] is its compile-time rank.
* [[`expression_random_access`:nosig]] reports whether the expression can
  be read at arbitrary indices. Stateful generators may report `false`.

[[`get_shape(T &&)`:nosig]] returns the shape of a particular expression
instance, including extents known only at run time. The type-only
[[`get_shape()`:nosig]] overload returns its static shape, using
`undefined_size` for axes whose extents depend on the instance.

```c++
#include <kfr/all.hpp>

using namespace kfr;

template <input_expression E>
void inspect(const E& expression)
{
    using value_type = expression_value_type<E>;
    constexpr size_t dims = expression_dims<E>;
    constexpr bool random_access = expression_random_access<E>;

    const shape<dims> extent = get_shape(expression);
|||    CHECK(extent[0] == 5);
}
|||TEST_CASE("fundamentals.md/inspect")
|||{
|||    univector<float, 5> data{ 1, 2, 3, 4, 5 };
|||    inspect(data);
|||}
```

## Broadcasting in a lazy pipeline

As shown in the overview, operators and KFR functions compose expressions
without normally evaluating their arguments immediately:

```c++
|||TEST_CASE("fundamentals.md/broadcasting")
|||{
univector<float, 5> input{ 1, 2, 3, 4, 5 };
auto expression = sqrt(input * 0.5f + 1.0f); // no output storage yet

univector<float, 5> output = expression;    // evaluates here
|||    CHECK(output[0] == sqrt(1.5f));
|||}
```

The resulting expression combines all three operations, so KFR can process the
values as one vectorized operation without allocating intermediate vectors.
`process`, assignment, [[`render(Expr &&)`:nosig]], and
[[`trender(const E &)`:nosig]] are common evaluation points.

Scalars participate as zero-dimensional expressions. They broadcast across any
number of axes, so `input * 0.5f` applies the same value to every element. A
size-one axis broadcasts too. For example, shapes `{ 1, 5 }` and `{ 5, 1 }`
combine to `{ 5, 5 }`; shapes are compared from their final axes.

Use [[`scalar(T)`:nosig]] when an explicit scalar expression is useful, for
example when assigning a scalar to a tensor view:

```c++
|||TEST_CASE("fundamentals.md/scalar-tensor")
|||{
tensor<float, 2> image(shape{ 8, 8 });
image = scalar(0.0f);
image(trange(2, 6), trange(2, 6)) = scalar(1.0f);
|||    CHECK(image(3, 3) == 1.0f);
|||    CHECK(image(0, 0) == 0.0f);
|||}
```

## Lifetime of composed expressions

A composed expression stores temporary expression arguments by value and lvalue
expression arguments by reference. This avoids copies, but every referenced
operand must remain alive until evaluation:

```c++
|||TEST_CASE("fundamentals.md/lifetime")
|||{
univector<float> samples = { 1, 2, 3 };
auto scaled = samples * 2.0f; // scaled refers to samples
|||    univector<float> result = scaled;
|||    CHECK(result[0] == 2.0f);
|||}
```

Do not return an expression that refers to a local container:

```c++
auto invalid_expression()
{
    univector<float> local = { 1, 2, 3 };
    return local + 1.0f; // dangling reference after the return
}
```

<!-- Not called: the function above is shown to illustrate a dangling-reference
pitfall, not to be executed. -->

Materialize the result before `local` is destroyed, or move an owned operand
into the composed expression when that is appropriate:

```c++
auto make_expression()
{
    univector<float> local = { 1, 2, 3 };
    return std::move(local) + 1.0f; // local is captured by value
}
|||TEST_CASE("fundamentals.md/make_expression")
|||{
|||    univector<float> result = make_expression();
|||    CHECK(result[0] == 2.0f);
|||}
```

When a concrete, storable expression type is required, see
[Expression handles](handle.md).

## Custom expression types

Custom expressions implement a SIMD-oriented `get_elements` interface and,
when writable, a matching `set_elements` interface. These ADL-discovered
functions are implementation details of an expression type rather than normal
application-level calls. Use [[`get_element(E &&, shape<Dims>)`:nosig]] when a
single value must be inspected.

[Writing custom expressions](custom.md) covers the complete contract,
vectorization axes, stateful expressions, and read-only and writable examples.

## Evaluating with `process`

[[`process(Out &&, In &&, shape<outdims>, shape<outdims>, csize_t<gw>)`:nosig]]
reads an input expression and writes an output expression. With no explicit
range, it processes the region both expressions can represent. The source may
have fewer axes or extent-one axes, in which case it broadcasts to the output.

```c++
|||TEST_CASE("fundamentals.md/process")
|||{
univector<double, 6> values;
process(values, counter(0.0, 0.5));
// values: { 0, 0.5, 1, 1.5, 2, 2.5 }
|||    CHECK(values[1] == 0.5);

tensor<double, 2> grid(shape{ 3, 4 });
process(grid, counter(100.0, 1.0, 10.0));
// grid(row, column) = 100 + row + 10 * column
|||    CHECK(grid(1, 2) == 121.0);
|||}
```

The optional `start` and `size` parameters select a region of the output. The
function returns the stop index of the processed region. Template parameters
can select a vector width and axis when the default final-axis traversal is not
suitable.

## Materializing or discarding results

Use the result form that matches the expression rank and required lifetime:

* [[`render(Expr &&)`]] materializes a finite 1D expression into a
  dynamic `univector`.
* [[`render(Expr &&, size_t, size_t)`]] materializes a 1D range, which
  is how to read a finite portion of an infinite generator.
* [[`render(Expr &&, csize_t<Size>)`]] materializes into a fixed-size
  `univector`.
* [[`trender(const E &)`]] materializes a finite expression of any rank
  into a `tensor`. Its size overload is suitable for bounded portions of an
  infinite expression.
* [[`sink(E &&)`]] traverses a finite expression and discards the
  produced values. It is useful for expressions with state or side effects
  where no result container is needed.

```c++
|||TEST_CASE("fundamentals.md/materialize")
|||{
auto first_five = render(counter(), 5); // { 0, 1, 2, 3, 4 }

auto matrix = trender(
    truncate(counter(0, 10, 1), shape{ 2, 3 }));
// {{0, 1, 2}, {10, 11, 12}}

sink(truncate(counter(), 1024)); // forces traversal, retains no values
|||    CHECK(first_five[4] == 4);
|||    CHECK(matrix(1, 2) == 12);
|||}
```

