# Writing custom expressions

KFR expressions are ordinary C++ types paired with a small traits and access
protocol. Implement that protocol when an algorithm should consume a custom
data source directly, when a procedural source should compose with KFR
operators, or when external storage should be a KFR destination without a
copy.

An input expression supplies values through `get_elements`. An output
expression accepts values through `set_elements`. Both functions operate on
SIMD blocks, not individual scalar accesses. A type that supplies both is an
[[`input_output_expression`:nosig]].

This page contains two examples:

1. `identity_matrix` is a finite, read-only 2D expression whose size is part
   of its type.
2. `strided_span` is a runtime-sized 1D view that can be read from or written
   to.

## The expression contract

To give a type KFR expression semantics, specialize [[`expression_traits<T>`:nosig]]
for that type (or derive the type itself from
[[`expression_traits_defaults`:nosig]]). The traits provide the following
information:

| Member | Purpose |
| --- | --- |
| `value_type` | Element type returned or stored by the expression. |
| `dims` | Number of axes. It must be a compile-time constant. |
| `get_shape(const E&)` | Actual extent of a particular expression object. |
| `get_shape()` | Compile-time extent information. Use [[`undefined_size`:nosig]] for an extent known only from an object, and [[`infinite_size`:nosig]] for an unbounded axis. |

Derive a traits specialization, or the expression class itself, from
[[`expression_traits_defaults`:nosig]] to use the defaults:

* `random_access` is `true`, meaning the same index can be requested in any
  order and produces the same value.
* `explicit_operand` is `true`, meaning the type is a proper expression
  operand rather than an implicitly converted scalar.

Set `random_access` to `false` for stateful streams. A generator expression,
for example, must be evaluated in traversal order and cannot be safely reread
at an arbitrary index.

The access functions are found by argument-dependent lookup. Put a free
function in the expression type's namespace, or define it as a friend in the
class. Do not add overloads to an unrelated namespace.

### Block access

KFR calls an input expression as follows:

```c++
||||||||||||
template <index_t Axis, size_t N>
vec<T, N> get_elements(E& self, const shape<Dims>& index,
                       const axis_params<Axis, N>& axis);
||||||||||||
```

`index` identifies the first requested element. `Axis` says which coordinate advances
between lanes, and `N` is the block width. The lane indices are:

$$
index_a + i \quad \text{when } a = Axis, \qquad index_a \quad \text{otherwise},
$$

for lane $i$. [[`indices(const shape<Dims> &, axis_params<VecAxis, N>)`:nosig]]
builds those lane-index vectors and avoids manual lane loops for expressions
that can calculate all lanes at once.

For a mutable destination, add the corresponding store function:

```c++
||||||||||||
template <index_t Axis, size_t N>
void set_elements(E& self, const shape<Dims>& index,
                  const axis_params<Axis, N>& axis,
                  const std::type_identity_t<vec<T, N>>& values);
||||||||||||
```

KFR guarantees that calls to `get_elements` and `set_elements` use a
power-of-two `N`.

KFR's output-expression check uses a single-element `set_elements` call; the
implementation should nevertheless support every axis and block width that
the expression is expected to process.

## Example: a fixed-size identity matrix

An identity matrix is a useful example because it has no backing storage. Its
value at row $r$ and column $c$ is simply $1$ when $r=c$, otherwise $0$.
`Size` is a template parameter, so both forms of `get_shape` can report a
fully known shape.

```c++
#include <kfr/all.hpp>

namespace kfr
{
template <typename T, index_t Size>
struct identity_matrix
{
};

template <typename T, index_t Size>
struct expression_traits<identity_matrix<T, Size>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 2;

    constexpr static shape<2> get_shape(const identity_matrix<T, Size>&) { return { Size, Size }; }
    constexpr static shape<2> get_shape() { return { Size, Size }; }
};

template <typename T, index_t Size, index_t Axis, size_t N>
vec<T, N> get_elements(const identity_matrix<T, Size>&, const shape<2>& index,
                       const axis_params<Axis, N>& axis)
{
    return select(indices<0>(index, axis) == indices<1>(index, axis), T{ 1 }, T{ 0 });
}
} // namespace kfr
```

`indices<0>` produces the row coordinate for every lane, and `indices<1>`
does the same for the column coordinate. Only the coordinate selected by
`Axis` changes across lanes. The element-wise comparison and [[`select(const mask<T1, N> &, const T2 &, const T3 &)`:nosig]]
therefore produce a correct vector whether KFR traverses rows (`Axis == 0`) or
columns (`Axis == 1`); no separate scalar fallback is needed.

The expression can be used wherever a 2D input is accepted:

```c++
|||using namespace kfr;
|||TEST_CASE("expressions/custom.md - fixed-size identity_matrix")
|||{
auto identity = trender(identity_matrix<float, 3>{});
// {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}
|||CHECK(identity == tensor<float, 2>{ { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } });

tensor<float, 2> scaled(shape{ 3, 3 });
scaled = identity_matrix<float, 3>{} * 2.0f;
// {{2, 0, 0}, {0, 2, 0}, {0, 0, 2}}
|||CHECK(scaled == tensor<float, 2>{ { 2, 0, 0 }, { 0, 2, 0 }, { 0, 0, 2 } });
|||}
```

### Runtime sizes

If the matrix size belongs to an object rather than its type, the instance
shape reports that value. The static shape cannot promise an extent, so it
uses [[`undefined_size`:nosig]]:

```c++
|||namespace kfr
|||{
template <typename T>
struct identity_matrix_dynamic : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 2;

    constexpr static shape<2> get_shape(const identity_matrix_dynamic& self)
    {
        return { self.size, self.size };
    }
    constexpr static shape<2> get_shape()
    {
        return { undefined_size, undefined_size };
    }

    template <index_t Axis, size_t N>
    friend vec<T, N> get_elements(const identity_matrix_dynamic&, const shape<2>& index,
                                  const axis_params<Axis, N>& axis)
    {
        return select(indices<0>(index, axis) == indices<1>(index, axis), T{ 1 }, T{ 0 });
    }

    index_t size;
};
|||} // namespace kfr
|||TEST_CASE("expressions/custom.md - runtime-sized identity_matrix")
|||{
|||    kfr::identity_matrix_dynamic<float> m;
|||    m.size = 2;
|||    CHECK(kfr::get_element(m, kfr::shape<2>{ 0, 0 }) == 1.0f);
|||    CHECK(kfr::get_element(m, kfr::shape<2>{ 0, 1 }) == 0.0f);
|||    CHECK(kfr::get_element(m, kfr::shape<2>{ 1, 1 }) == 1.0f);
|||}
```

This compact style derives the expression itself from
[[`expression_traits_defaults`:nosig]]. It is equivalent to writing a separate
`expression_traits` specialization and keeps a small procedural expression in
one definition.

## Example: a mutable strided view

The next example adapts a non-contiguous 1D range. It demonstrates a
runtime-sized shape, vector gather/scatter for non-unit strides, and `set_elements`
for output expressions. The view does not own the underlying memory; its
caller must keep that memory valid.

```c++
|||namespace kfr
|||{
template <typename T>
struct strided_span : expression_traits_defaults
{
    using value_type             = std::remove_const_t<T>;
    constexpr static size_t dims = 1;

    T* data;
    index_t size;
    index_t stride;

    constexpr strided_span(T* data, index_t size, index_t stride)
        : data(data), size(size), stride(stride)
    {
    }

    constexpr static shape<1> get_shape(const strided_span& self)
    {
        return { self.size };
    }
    constexpr static shape<1> get_shape()
    {
        return { undefined_size };
    }

    template <index_t Axis, size_t N>
    friend vec<value_type, N> get_elements(const strided_span& self,
                                           const shape<1>& index,
                                           const axis_params<Axis, N>&)
    {
        static_assert(Axis == 0);
        const T* first = self.data + index[0] * self.stride;
        if (self.stride == 1)
            return read<N>(first);
        return gather_stride<N>(first, self.stride);
    }

    template <index_t Axis, size_t N>
        requires(!std::is_const_v<T>)
    friend void set_elements(strided_span& self, const shape<1>& index,
                             const axis_params<Axis, N>&,
                             const std::type_identity_t<vec<value_type, N>>& values)
    {
        static_assert(Axis == 0);
        T* first = self.data + index[0] * self.stride;
        if (self.stride == 1)
            write(first, values);
        else
            scatter_stride(first, values, self.stride);
    }
};
|||} // namespace kfr
```

The `requires` clause omits `set_elements` when `T` is const. Consequently,
`strided_span<const float>` is an [[`input_expression`:nosig]], while
`strided_span<float>` is an [[`input_output_expression`:nosig]]. This follows
the same constness rule as KFR's container views.

```c++
|||using namespace kfr;
|||TEST_CASE("expressions/custom.md - strided_span")
|||{
float interleaved[] = { 1, 10, 2, 20, 3, 30, 4, 40 };

strided_span<float> left{ interleaved, 4, 2 };
strided_span<float> right{ interleaved + 1, 4, 2 };

process(right, left * 10.0f);
// interleaved: {1, 10, 2, 20, 3, 30, 4, 40}
|||CHECK(interleaved[0] == 1);
|||CHECK(interleaved[1] == 10);
|||CHECK(interleaved[2] == 2);
|||CHECK(interleaved[3] == 20);

process(left, left + 0.5f);
// interleaved: {1.5, 10, 2.5, 20, 3.5, 30, 4.5, 40}
|||CHECK(interleaved[0] == 1.5f);
|||CHECK(interleaved[1] == 10);
|||CHECK(interleaved[2] == 2.5f);
|||CHECK(interleaved[3] == 20);
|||CHECK(interleaved[4] == 3.5f);
|||CHECK(interleaved[5] == 30);
|||CHECK(interleaved[6] == 4.5f);
|||CHECK(interleaved[7] == 40);
|||}
```

The example reads and writes the left channel without constructing a temporary
`univector`. The stores are scattered because consecutive logical elements are
two physical elements apart. A real view type should define its range and
aliasing rules clearly, especially when source and destination can overlap.

## State and pass hooks

Most custom expressions only need traits and access functions. Stateful
expressions may additionally provide ADL-visible `begin_pass(expr, start, stop)` and `end_pass(expr, start, stop)` hooks. [[`process(Out &&, In &&, shape<outdims>, shape<outdims>, csize_t<gw>)`:nosig]] calls these around an evaluation pass, allowing an expression to reset, reserve, or finalize state.

Use such expressions with care: traversal order, SIMD width, and splitting a
range into several passes can affect state consumption. Mark a stateful stream
as `random_access = false`, construct a fresh instance for an independent
pass, and do not assume that requesting an index twice returns the same value.

## Verification checklist

Before exposing a custom expression from a library, verify the following:

1. `value_type`, rank, and both shape functions agree.
2. The runtime shape never exceeds the storage or generated domain.
3. `get_elements` advances exactly the axis named by `axis_params`.
4. A writable expression implements `set_elements` with the same index and
   axis interpretation.
5. Non-contiguous storage uses gather/scatter or an equivalent correct
   implementation.
6. A non-owning expression documents the lifetime of its referenced storage.
7. Stateful expressions declare `random_access = false` and handle each pass
   deliberately.

Use [[`get_element(E &&, shape<Dims>)`:nosig]] for focused scalar checks and
[[`render(Expr &&)`:nosig]] or [[`trender(const E &)`:nosig]] to compare a
complete finite expression against an expected container in tests.

## See also

- [Expression fundamentals](fundamentals.md) — expression shapes, composition,
  broadcasting, and evaluation.
- [Tensors and multidimensional expressions](tensor.md) — KFR's owning and
  strided multidimensional expression container.
- [Sources and adaptors](sources.md) — built-in procedural expressions and
  expression views.
