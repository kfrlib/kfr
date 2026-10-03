# Tensors and multidimensional expressions

[[`tensor<T, NDims>`:nosig]] is KFR's container for data with a fixed number of
dimensions. It is useful for matrices, images, volumes, and any other result
whose shape matters as much as its values. A tensor normally owns aligned
storage, but it can also be a view of memory managed elsewhere. In either case,
it participates in KFR as both an input and an output [expression](expressions.md).

The rank is part of the type, while the extent of each axis is chosen at run
time. In the following examples, `image` is always two-dimensional, although
its width and height are stored in its [[`tensor<T, NDims>::shape()`:nosig:noscope]]:

```c++
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("expressions/tensor.md/fixed-rank")
|||{
tensor<float, 2> image(shape{ 480, 640 });
tensor<float, 3> volume(shape{ 32, 64, 64 });
|||    CHECK(image.shape() == shape{ 480, 640 });
|||    CHECK(volume.shape() == shape{ 32, 64, 64 });
|||}
```

Rank zero represents one scalar value. It is most often produced by evaluating
a scalar expression:

```c++
|||TEST_CASE("expressions/tensor.md/scalar")
|||{
auto answer = trender(scalar(42)); // tensor<int, 0>
|||    CHECK(answer.to_string() == "42");
|||}
```

`tensor<T, dynamic_shape>` is not implemented, so select a fixed rank when
writing the type. As with other KFR containers, operators normally build a lazy
expression. The expression is evaluated when it is assigned to a tensor,
constructed as a tensor, or passed to
[[`process(Out &&, In &&, shape<outdims>, shape<outdims>, csize_t<gw>)`:nosig]]
or [[`trender(const E &)`:nosig]].

## Creating tensors

The usual way to create a tensor is to provide a [[`shape<dims>`:nosig]]. This
allocates 64-byte-aligned, packed row-major storage, ready for normal KFR
expression processing:

```c++
|||TEST_CASE("expressions/tensor.md/allocation")
|||{
tensor<float, 2> uninitialized(shape{ 2, 3 });
tensor<float, 2> zeroes(shape{ 2, 3 }, 0.0f);
tensor<int, 2> values(shape{ 2, 3 }, { 1, 2, 3, 4, 5, 6 });
|||    CHECK(uninitialized.shape() == shape{ 2, 3 });
|||    CHECK(zeroes(1, 2) == 0.0f);
|||    CHECK(values(1, 2) == 6);
|||}
```

Small tensors are often clearest as initializer lists. KFR has nested-list
constructors through rank four, and infers the shape from the first nested
list:

```c++
|||TEST_CASE("expressions/tensor.md/nested-lists")
|||{
tensor<int, 1> line{ 1, 2, 3 };
tensor<int, 2> grid{ { 1, 2, 3 }, { 4, 5, 6 } };
tensor<int, 3> cube{ { { 1, 2 } }, { { 3, 4 } } };
|||    CHECK(line.shape() == shape{ 3 });
|||    CHECK(grid(1, 2) == 6);
|||    CHECK(cube(1, 0, 1) == 4);
|||}
```

Nested lists should be rectangular. A flat list given with an explicit shape
must contain exactly as many elements as
[[`tensor<T, NDims>::size()`:nosig:noscope]] reports; otherwise construction
reports a logic error.

A tensor can also materialize an expression directly. The expression rank must
match the tensor rank, and the constructor allocates enough storage for the
expression's runtime shape:

```c++
|||TEST_CASE("expressions/tensor.md/materialize-expression")
|||{
tensor<float, 2> ramp = trender(
    lambda<float, 2>([](shape<2> index)
    {
        return float(10 * index[0] + index[1]);
    }),
    shape{ 2, 3 });

tensor<float, 2> squared = ramp * ramp;
|||    CHECK(ramp(1, 2) == 12.0f);
|||    CHECK(squared(1, 2) == 144.0f);
|||}
```

When the result type is not important to spell out, use [[`trender(const E &)`:nosig]]
to deduce both the value type and rank:

```c++
|||TEST_CASE("expressions/tensor.md/deduced-render")
|||{
|||    const tensor<float, 2> ramp{ { 0, 1, 2 }, { 10, 11, 12 } };
auto result = trender(ramp * 0.5f + 1.0f);
|||    CHECK(result(1, 2) == 7.0f);
|||}
```

The no-size form of [[`trender(const E &)`:nosig]] requires a finite
expression. Give the size overload a shape to materialize an infinite source,
such as [[`counter(T)`:nosig]]; KFR uses the requested shape as an upper bound
on every axis:

```c++
|||TEST_CASE("expressions/tensor.md/bounded-source")
|||{
auto block = trender(counter(0.0f, 4.0f, 1.0f), shape{ 2, 4 });
|||    CHECK(block == tensor<float, 2>{ { 0, 1, 2, 3 }, { 4, 5, 6, 7 } });
|||}
```

## Shape, layout, and strides

A shape stores one extent for each axis.
[[`tensor<T, NDims>::size()`:nosig:noscope]] is the product of those extents,
and [[`tensor<T, NDims>::empty()`:nosig:noscope]] is true when that product is
zero. Shapes, indices, and strides use KFR's `index_t` type.

A normally allocated tensor uses **row-major** layout, so its final axis varies
fastest. For a tensor with shape `{ 2, 3, 4 }`, the row-major strides are
`{ 12, 4, 1 }`. The element at `(i, j, k)` is therefore at offset

$$
\operatorname{offset}(i, j, k) = 12i + 4j + k.
$$

More generally, tensors use their stored strides to convert an index to a
memory offset:

$$
\operatorname{offset}(i_0, \ldots, i_{D-1}) =
\sum_a i_a \cdot \operatorname{stride}_a.
$$

```c++
|||TEST_CASE("expressions/tensor.md/row-major-offset")
|||{
tensor<int, 2> matrix(shape{ 2, 3 });
// shape: {2, 3}; strides: {3, 1}

matrix(1, 2) = 42; // offset 1 * 3 + 2
|||    CHECK(matrix[5] == 42);
|||}
```

[[`shape<Dims>::to_flat`:nosig]] and [[`shape<Dims>::from_flat`:nosig]] perform
the corresponding row-major conversions for logical indices. They describe a
shape, whereas tensor element access always uses the tensor's actual strides.

### Column-major and other packed layouts

The allocating `(shape, strides)` constructors support layouts that fill one
compact storage block without gaps. This includes row-major order, column-major
order, and any other complete permutation of the axes. It does **not** include
padded rows, overlapping axes, zero strides, reverse strides, or arbitrary
strided layouts; those require the external-memory constructor described below.

For example, a 2-by-3 column-major tensor has strides `{ 1, 2 }`:

```c++
|||TEST_CASE("expressions/tensor.md/column-major")
|||{
tensor<int, 2> column_major(shape{ 2, 3 }, shape{ 1, 2 });
column_major(1, 2) = 12; // physical offset 1 + 2 * 2
|||    CHECK_FALSE(column_major.is_contiguous());
|||    CHECK(column_major[5] == 12);
|||}
```

The data is still packed, but [[`tensor<T, NDims>::is_contiguous`:nosig:noscope]]
means *canonical row-major* contiguity. It is therefore false for this
column-major tensor, and [[`tensor<T, NDims>::contiguous_begin()`:nosig:noscope]] cannot be used. Indexed access,
logical iteration, copying, and expression evaluation all continue to work.

## Ownership and external memory

A tensor handle stores a pointer, shape, strides, and a shared
[[`memory_finalizer`:nosig]]. Copying a tensor, or assigning one non-const
tensor lvalue to another, copies that handle. It does not copy the elements.
Slices, transposes, and view reshapes follow the same rule, so they all share
the original storage and finalizer.

```c++
|||TEST_CASE("expressions/tensor.md/shared-handle")
|||{
tensor<int, 2> original{ { 1, 2 }, { 3, 4 } };
auto alias = original;       // another handle for the same data
alias(0, 0) = 10;            // original(0, 0) is now also 10

auto independent = original.copy(); // separate packed allocation
|||    CHECK(original(0, 0) == 10);
|||    CHECK(independent(0, 0) == 10);
|||    independent(0, 0) = 0;
|||    CHECK(original(0, 0) == 10);
|||}
```

This is also why a `const tensor` is a writable handle: KFR uses const handles
for output expressions and views. Constness here does not make the referenced
elements immutable.

### Non-owning and lifetime-managed views

The pointer constructors create a tensor view. Passing a null finalizer makes
the view non-owning, which is appropriate when the caller already controls the
storage lifetime:

```c++
|||TEST_CASE("expressions/tensor.md/non-owning-view")
|||{
std::vector<float> samples(6);
tensor<float, 2> view(samples.data(), shape{ 2, 3 }, nullptr);
view(1, 2) = 1.0f; // writes samples[5]
|||    CHECK(samples[5] == 1.0f);
|||}
```

The vector must remain alive and must not reallocate while the view is used.
For a strided view, the caller must also provide enough memory for every offset
that the shape and strides can reach. This is how to represent a padded layout:

```c++
|||TEST_CASE("expressions/tensor.md/padded-view")
|||{
std::array<float, 8> padded{};
tensor<float, 2> rows(padded.data(), shape{ 2, 3 }, shape{ 4, 1 }, nullptr);
// The logical rows occupy offsets {0, 1, 2} and {4, 5, 6}.
|||    rows(1, 2) = 1.0f;
|||    CHECK(padded[6] == 1.0f);
|||}
```

To make a view keep its storage alive, provide a finalizer. A finalizer is
shared by all related tensor views, and [[`make_memory_finalizer`:nosig]] calls
its function when the last handle is destroyed:

```c++
|||TEST_CASE("expressions/tensor.md/lifetime-managed-view")
|||{
float* raw = new float[6];
tensor<float, 2> owned_view(
    raw, shape{ 2, 3 },
    make_memory_finalizer([raw] { delete[] raw; }));
|||    owned_view(1, 2) = 1.0f;
|||    CHECK(owned_view(1, 2) == 1.0f);
|||}
```

[[`tensor_from_container`:nosig]] is a convenient special case for a movable,
contiguous container. It creates a one-dimensional view and stores the container
inside the tensor's finalizer:

```c++
|||TEST_CASE("expressions/tensor.md/container-view")
|||{
std::vector<float> samples{ 1, 2, 3, 4 };
auto signal = tensor_from_container(std::move(samples));
|||    CHECK(signal.shape() == shape{ 4 });
|||    CHECK(signal(3) == 4.0f);
|||}
```

The function takes its argument by value. Supplying an rvalue moves the
container into the finalizer; supplying an lvalue first copies that container.
The container must provide `data()`, `size()`, and `value_type`.

For advanced ownership code, [[`tensor<T, NDims>::data()`:nosig:noscope]]
exposes the base pointer, [[`tensor<T, NDims>::finalizer()`:nosig:noscope]]
returns the shared lifetime handle, and
[[`tensor<T, NDims>::allocate(size_t)`:nosig:noscope]] /
[[`tensor<T, NDims>::deallocate(T *)`:nosig:noscope]] expose KFR's aligned
allocation functions.

## Indexing and slicing

Passing one integer per axis returns an element reference:

```c++
|||TEST_CASE("expressions/tensor.md/element-access")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };
matrix(1, 2) = 60;
|||    CHECK(matrix(1, 2) == 60);
|||}
```

[[`tensor<T, NDims>::access(const shape_type &)`:nosig:noscope]] performs the
same stride-based access. Full-rank indexing is intentionally low-level and
does not check bounds. `operator[]` is lower level still: it indexes directly
from [[`tensor<T, NDims>::data()`:nosig:noscope]] and is not a logical row-major
index for a strided tensor.

Partial indexing produces a tensor view. An integer selector removes its axis,
while a [[`tensor_range`:nosig]] selector retains it. Any trailing axes that
are omitted are selected in full:

```c++
|||TEST_CASE("expressions/tensor.md/partial-indexing")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };

auto first_row  = matrix(0, tall());  // tensor<int, 1>: {1, 2, 3}
auto middle_col = matrix(tall(), 1);  // tensor<int, 1>: {2, 5}
auto top_rows   = matrix(tstop(1));   // tensor<int, 2>: {{1, 2, 3}}

first_row = first_row * 10; // updates the first row in matrix
|||    CHECK_THAT(first_row, DeepMatcher(tensor<int, 1>{ 10, 20, 30 }));
|||    CHECK_THAT(middle_col, DeepMatcher(tensor<int, 1>{ 20, 5 }));
|||    CHECK_THAT(top_rows, DeepMatcher(tensor<int, 2>{ { 10, 20, 30 } }));
|||}
```

These views are lightweight and share data with the source. They keep the
source storage alive through its finalizer, even when the original tensor
handle itself has gone out of scope.

### Selecting ranges

[[`tensor_range`:nosig]] is the range descriptor used by tensor slicing. For a
nonzero positive step it follows the usual half-open convention. Negative
endpoints are interpreted from the end of the axis, and nonzero-step endpoints
are clamped to the valid range.

| Factory | Meaning |
| --- | --- |
| [[`tall`:nosig]] | Select the complete axis. |
| [[`tstart`:nosig]]`(start, step = 1)` | Select from the inclusive start through the end. |
| [[`tstop`:nosig]]`(stop, step = 1)` | Select from the beginning through the exclusive stop. |
| [[`tstep`:nosig]]`(step = 1)` | Select the complete axis with the given step. |
| [[`trange`:nosig]]`(start, stop, step)` | Select an explicit range; each component may be omitted. |

```c++
|||TEST_CASE("expressions/tensor.md/ranges")
|||{
tensor<int, 1> values{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
const auto _ = std::nullopt;

auto head    = values(tstop(3));         // {0, 1, 2}
auto tail    = values(tstart(-3));       // {7, 8, 9}
auto middle  = values(trange(3, 7));     // {3, 4, 5, 6}
auto reverse = values(trange(_, _, -1)); // {9, 8, ..., 0}
auto every_2 = values(tstep(2));         // {0, 2, 4, 6, 8}
|||    CHECK_THAT(head, DeepMatcher(tensor<int, 1>{ 0, 1, 2 }));
|||    CHECK_THAT(tail, DeepMatcher(tensor<int, 1>{ 7, 8, 9 }));
|||    CHECK_THAT(middle, DeepMatcher(tensor<int, 1>{ 3, 4, 5, 6 }));
|||    CHECK_THAT(reverse, DeepMatcher(tensor<int, 1>{ 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }));
|||    CHECK_THAT(every_2, DeepMatcher(tensor<int, 1>{ 0, 2, 4, 6, 8 }));
|||}
```

A step becomes part of the view's stride, so steps naturally compose:

```c++
|||TEST_CASE("expressions/tensor.md/composed-steps")
|||{
tensor<int, 1> values{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
auto every_4 = values(tstep(2))(tstep(2)); // {0, 4, 8}
|||    CHECK_THAT(every_4, DeepMatcher(tensor<int, 1>{ 0, 4, 8 }));
|||}
```

A negative step creates a reverse view. A zero step is different: it creates a
zero-stride view that repeats the selected element. Writing through that view
writes the same source element repeatedly:

```c++
|||TEST_CASE("expressions/tensor.md/zero-step")
|||{
tensor<int, 1> values{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
auto repeated = values(trange(3, 6, 0)); // {3, 3, 3}
repeated = 9;                            // repeatedly assigns values[3]
|||    CHECK_THAT(repeated, DeepMatcher(tensor<int, 1>{ 9, 9, 9 }));
|||    CHECK(values(3) == 9);
|||}
```

For a zero-step range, the stop must not precede the start, and a nonempty
range must start inside the axis. Invalid ranges report a logic error. Unlike
ordinary ranges, the explicit stop is not clamped because it specifies the end
of the repetition count.

There is also a direct same-rank slice overload that accepts start and exclusive
stop shapes. It is useful for code that already has multidimensional indices,
but it performs no ordering or bounds validation:

```c++
|||TEST_CASE("expressions/tensor.md/shape-slice")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };
auto block = matrix(shape{ 0, 1 }, shape{ 2, 3 }); // shape {2, 2}
|||    CHECK_THAT(block, DeepMatcher(tensor<int, 2>{ { 2, 3 }, { 5, 6 } }));
|||}
```

## Iteration and formatting

[[`tensor<T, NDims>::begin`:nosig:noscope]] and
[[`tensor<T, NDims>::end`:nosig:noscope]] traverse logical elements in row-major
index order. They work with normal tensors and with column-major, stepped,
transposed, or reverse views, so range-based loops do the natural thing:

```c++
|||static void use(int) {}
|||TEST_CASE("expressions/tensor.md/iteration")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };
for (int& value : matrix)
    value *= 2;

for (int value : matrix.transpose())
    use(value); // 2, 8, 4, 10, 6, 12
|||    CHECK_THAT(matrix, DeepMatcher(tensor<int, 2>{ { 2, 4, 6 }, { 8, 10, 12 } }));
|||}
```

The ordinary iterators are forward iterators. If an algorithm needs raw pointer
iterators, use [[`tensor<T, NDims>::contiguous_begin()`:nosig:noscope]] and
[[`tensor<T, NDims>::contiguous_end()`:nosig:noscope]] only after checking
[[`tensor<T, NDims>::is_contiguous()`:nosig:noscope]]. They require canonical
row-major storage and report a logic error for other layouts or views. The
[[`tensor<T, NDims>::contiguous_begin_unsafe()`:nosig:noscope]] and
[[`tensor<T, NDims>::contiguous_end_unsafe()`:nosig:noscope]] variants skip
that check. [[`tensor<T, NDims>::is_last_contiguous()`:nosig:noscope]] is a
weaker test that only says the final axis has stride one.

[[`tensor<T, NDims>::nested_begin()`:nosig:noscope]] and
[[`tensor<T, NDims>::nested_end()`:nosig:noscope]] offer iteration over the
first axis. For a rank greater than one, dereferencing returns a rank-one-lower
tensor **by value**. The returned value is still a shared-data view, so it can
be used to update a row or other first-axis slice:

```c++
|||TEST_CASE("expressions/tensor.md/nested-iteration")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };
for (auto it = matrix.nested_begin(); it != matrix.nested_end(); ++it)
{
    auto row = *it;
    row = row + 100;
}
|||    CHECK_THAT(matrix, DeepMatcher(tensor<int, 2>{ { 101, 102, 103 }, { 104, 105, 106 } }));
|||}
```

[[`tensor<T, NDims>::iterate`:nosig:noscope]] is useful when both the value and
its logical index are needed. [[`tensor<T, NDims>::to_string`:nosig:noscope]]
formats scalar and multidimensional tensors, with options for limiting columns
and dimensions or choosing delimiters.

```c++
|||TEST_CASE("expressions/tensor.md/indexed-iteration")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };
matrix.iterate([](int& value, shape<2> index)
{
    value += 10 * index[0] + index[1];
});
|||    CHECK_THAT(matrix, DeepMatcher(tensor<int, 2>{ { 1, 3, 5 }, { 14, 16, 18 } }));
|||}
```

## Views, transpose, and reshaping

[[`tensor<T, NDims>::transpose`:nosig:noscope]] returns a view rather than
moving elements. It reverses **all** axes and their strides. For a matrix this
is the familiar transpose; for a tensor with shape `{ 2, 3, 4 }`, it produces a
view with shape `{ 4, 3, 2 }`:

```c++
|||TEST_CASE("expressions/tensor.md/transpose")
|||{
tensor<int, 2> matrix{ { 1, 2, 3 }, { 4, 5, 6 } };
auto transposed = matrix.transpose(); // shape {3, 2}, same storage
|||    CHECK(transposed.shape() == shape{ 3, 2 });
|||    CHECK(transposed(2, 1) == 6);
|||}
```

Use [[`matrix_transpose`:nosig]] when a physical packed-buffer permutation is
needed. That operation is distinct from `tensor::transpose()`, which changes
only view metadata.

[[`tensor<T, NDims>::reshape`:nosig:noscope]] and
[[`tensor<T, NDims>::flatten`:nosig:noscope]] also return shared-data views.
They require the same number of elements and canonical row-major contiguity:

```c++
|||TEST_CASE("expressions/tensor.md/reshape")
|||{
tensor<int, 1> values{ 0, 1, 2, 3, 4, 5 };
auto matrix = values.reshape(shape{ 2, 3 }); // shared storage
auto flat = matrix.flatten();                // shared rank-one view
|||    CHECK(matrix(1, 2) == 5);
|||    CHECK_THAT(flat, DeepMatcher(tensor<int, 1>{ 0, 1, 2, 3, 4, 5 }));
|||}
```

Use [[`tensor<T, NDims>::reshape_may_copy`:nosig:noscope]] or
[[`tensor<T, NDims>::flatten_may_copy`:nosig:noscope]] when the input may be a
transpose or another non-contiguous view. These operations preserve logical
row-major iteration order and make a packed copy when necessary, unless
`allow_copy` is false:

```c++
|||TEST_CASE("expressions/tensor.md/flatten-may-copy")
|||{
tensor<int, 2> matrix{ { 0, 1, 2 }, { 3, 4, 5 } };
auto packed = matrix.transpose().flatten_may_copy();
// {0, 3, 1, 4, 2, 5}
|||    CHECK_THAT(packed, DeepMatcher(tensor<int, 1>{ 0, 3, 1, 4, 2, 5 }));
|||}
```

[[`tensor<T, NDims>::copy()`:nosig:noscope]] always produces an independent
packed row-major tensor, making it the simplest way to detach from shared
storage or pack an existing view.

The free [[`reshape(Arg &&, const shape<OutDims> &)`:nosig]] expression adaptor
has a different purpose. It is lazy and remaps row-major expression indices
during evaluation; it does not create a shared-data tensor view.

## Assignment and member operations

Assigning an expression to a tensor evaluates it into the tensor's existing
logical shape. KFR's usual final-axis-aligned broadcasting rules apply to tensor
destinations and views:

```c++
|||TEST_CASE("expressions/tensor.md/broadcasting")
|||{
tensor<float, 2> row(shape{ 1, 3 }, { 1, 2, 3 });
tensor<float, 2> column(shape{ 2, 1 }, { 10, 20 });
tensor<float, 2> sum = row + column; // shape {2, 3}

sum(tall(), 0) = 0.0f; // assign through a strided column view
|||    CHECK_THAT(sum, DeepMatcher(tensor<float, 2>{ { 0, 12, 13 }, { 0, 22, 23 } }));
|||}
```

Ordinary assignment between non-const tensor lvalues changes the destination
handle so that it shares the source. Use
[[`tensor<T, NDims>::assign(const tensor<T, NDims> &)`:nosig:noscope]] when the
destination storage must stay in place and only its values should change; this
member checks that the shapes agree.
[[`tensor<T, NDims>::assign(const T &)`:nosig:noscope]] fills every logical
element.

The following member functions operate in logical row-major order, including
when the source is a strided view:

| Member | Behavior |
| --- | --- |
| [[`tensor<T, NDims>::copy()`:nosig:noscope]] | Returns an independent, packed row-major copy. |
| [[`tensor<T, NDims>::copy_from(const tensor<T, NDims> &)`:nosig:noscope]] | Copies logical values into this tensor. |
| [[`tensor<T, NDims>::map(Fn &&)`:nosig:noscope]] / [[`tensor<T, NDims>::unary(Fn &&)`:nosig:noscope]] | Return a new packed tensor with `fn` applied to each value. |
| [[`tensor<T, NDims>::unary_inplace(Fn &&)`:nosig:noscope]] | Applies `fn` through this tensor or view. |
| [[`tensor<T, NDims>::reduce(Fn &&, T)`:nosig:noscope]] | Folds logical values as `fn(element, accumulator)`. |
| [[`tensor<T, NDims>::binary(const tensor<U, dims> &, Fn &&)`:nosig:noscope]] | Returns a packed tensor from pairwise values. |
| [[`tensor<T, NDims>::binary_inplace(const tensor<U, dims> &, Fn &&)`:nosig:noscope]] | Replaces each left value with `fn(left, right)`. |
| [[`tensor<T, NDims>::astype()`:nosig:noscope]] | Returns a packed tensor with each value converted using `static_cast<U>`. |
| [[`tensor<T, NDims>::to_array()`:nosig:noscope]] | Returns logical values in a `std::array`; `N` must equal `size()`. |

The mapping, unary, binary, and conversion members return new packed tensors,
while the in-place forms preserve the current view and write through it. The
reduction member has the perhaps unusual element-first call order shown in the
table, so use a clearly ordered function when that matters.

The binary and copy-from members expect compatible same-shaped or same-sized
operands but do not check that requirement. Use the expression operators
instead when broadcasting is wanted, and do not call these members with
mismatched tensors.

```c++
|||TEST_CASE("expressions/tensor.md/member-operations")
|||{
tensor<int, 2> matrix{ { 1, 2 }, { 3, 4 } };
auto squares = matrix.map([](int x) { return x * x; });
auto total = squares.reduce(std::plus<>{}, 0); // 30

matrix.unary_inplace([](int x) { return -x; });
auto floats = matrix.astype<float>();
|||    CHECK(total == 30);
|||    CHECK_THAT(matrix, DeepMatcher(tensor<int, 2>{ { -1, -2 }, { -3, -4 } }));
|||    CHECK_THAT(floats, DeepMatcher(tensor<float, 2>{ { -1, -2 }, { -3, -4 } }));
|||}
```

## Expression processing and SIMD access

Tensors implement KFR's expression read/write protocol, so a view can usually
be used wherever an ordinary tensor can. [[`process`:nosig]] and
[[`trender`:nosig]] normally advance along the final axis, which is the best
case for normal row-major storage. When the selected axis has stride one, KFR
uses contiguous SIMD loads and stores. For a strided axis it uses gathers and
scatters instead.

This means that slices and transposes remain valid expression inputs and
outputs, although their memory access can be less efficient. Algorithms that
need a different traversal direction can explicitly select an `Axis`; the
associated [[`axis_params<Axis, N>`:nosig]] describes that axis and the number
of SIMD lanes in the processed block.

## Requirements and checked errors

Tensor deliberately exposes low-level storage and indexing operations, so some
contracts remain the caller's responsibility. The following table distinguishes
those preconditions from operations that report a logic error themselves.

| Operation | Requirement or checked error |
| --- | --- |
| Allocating custom strides | The layout must be packed row-major, column-major, or an axis permutation. |
| External pointer and strides | The pointer must cover every stride-derived offset and remain alive through the finalizer. |
| Element access and integer slices | Coordinates must be in bounds; these operations do not check them. |
| Flat initializer list | It must contain exactly `size()` values. |
| `contiguous_begin/end` | `is_contiguous()` must be true, meaning canonical row-major layout. |
| `reshape` / `flatten` | The element count must match and the tensor must be canonical row-major contiguous. |
| `reshape_may_copy` / `flatten_may_copy` | The element count must match; a non-contiguous input needs `allow_copy == true`. |
| `assign(tensor)` | The two shapes must be equal. |
| `binary`, `binary_inplace`, `copy_from` | The caller must supply compatible tensors; these members do not validate them. |
| Zero-step `trange` | The stop cannot precede the start, and a nonempty range must start within its axis. |

For one-dimensional stored data, [`univector`](univector.md) is usually the
simpler container. Choose a tensor when rank, shape-aware broadcasting,
slicing, or multidimensional strided views are part of the problem.
