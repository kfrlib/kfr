# Expression handles

KFR expressions are normally fully typed at compile time: the concrete type of an
expression tree is determined by the operations used to build it, and that type is
propagated through every function call. This gives the optimizer full visibility into
the expression graph but makes it impossible to store an expression of unknown type
behind a stable interface — for example, to pass it across a translation unit, store
it in a container, or expose it in a C API.

An **expression handle** is a type-erased reference to an expression. It erases the
concrete expression type while preserving its value type `T` and dimensionality
`Dims`, and exposes the standard expression operations (`get_shape`, `get_elements`,
`set_elements`, `begin_pass`/`end_pass`, and placeholder substitution) through a
virtual table.

Handles are declared in `kfr/base/handle.hpp`.

## `expression_handle`

```c++
template <typename T, index_t Dims = 1>
struct expression_handle;
```

A handle stores three things:

| Member     | Description                                                             |
|------------|-------------------------------------------------------------------------|
| `instance` | Pointer to the concrete expression object.                              |
| `vtable`   | Pointer to a static `expression_vtable<T, Dims>` for the instance type. |
| `resource` | Optional `shared_ptr<expression_resource>` that owns the expression.    |

When `resource` is null the handle is **non-owning** and the caller is responsible
for keeping the referenced expression alive. When `resource` is set the handle is
**owning** and keeps the expression alive through a shared resource.

A handle is a regular KFR expression. `expression_traits<expression_handle<T, Dims>>`
provides `value_type`, `dims`, and `get_shape`, and the ADL `get_elements` /
`set_elements` / `begin_pass` / `end_pass` overloads forward to the concrete
expression through the vtable.

### Block size splitting

The vtable stores callbacks for power-of-two block sizes from `1` up to
`maximum_expression_width<T>` (twice the widest native vector for the highest
available CPU target). Requests for larger blocks are split recursively into two
half-size reads/writes, so a handle can serve any vector width regardless of the
concrete expression's native width.

## Creating a handle

### `to_handle` (non-owning)

```c++
template <typename E, typename T = expression_value_type<E>,
          index_t Dims = expression_dims<E>>
expression_handle<T, Dims> to_handle(E& expr);
```

Wraps an existing lvalue expression. The returned handle does **not** extend the
lifetime of `expr`; the caller must keep `expr` alive for as long as the handle is
used.

```c++
auto src = counter<float>();
auto h   = to_handle(src);          // h references src, does not own it

univector<float, 10> out = h * 2.f; // ok, src is still in scope
```

!!! warning
    Binding a handle to a temporary or a local that goes out of scope yields a
    dangling handle. Use the owning overload (below) for temporaries.

### `to_handle` (owning)

```c++
template <typename E, typename T = expression_value_type<E>,
          index_t Dims = expression_dims<E>>
expression_handle<T, Dims> to_handle(E&& expr);
```

Takes ownership of an rvalue expression by moving it into a heap-allocated
`expression_resource_impl<E`. Pass an rvalue (typically via `std::move`) to select
this overload. The returned handle keeps the moved expression alive as long as the
handle (or a copy of it) exists.

```c++
auto h = to_handle(counter<float>());   // owning: the counter lives in the handle

univector<float, 10> out = h;
```

## Using a handle as an expression

A handle can be used anywhere an expression of the same `T` and `Dims` is expected.
It supports the usual operations: arithmetic, function calls, assignment to
`univector`/`tensor`, and so on.

```c++
expression_handle<float> h = to_handle(counter<float>());

univector<float, 8> out = h * 10.f;     // reads through the vtable
```

Because the concrete type is erased, the compiler cannot inline through the handle
boundary. Use handles where type erasure is required (ABI boundaries, storage,
runtime polymorphism) and direct expressions where maximum performance is needed.

## Placeholders

A **placeholder** is an expression with a deferred input slot that can be bound to a
concrete expression at runtime. Placeholders let you build an expression graph once
and rebind its input later, without rebuilding the graph.

```c++
template <typename T, index_t Dims = 1, size_t Key = 0>
struct expression_placeholder;
```

`Key` is a compile-time integer that distinguishes several placeholders within the
same graph. An unbound placeholder has infinite size and reads as zeros.

### Creating a placeholder

```c++
template <typename T, index_t Dims = 1, size_t Key = 0>
expression_placeholder<T, Dims, Key> placeholder(csize_t<Key> = csize_t<Key>{});
```

```c++
auto in = placeholder<float>();           // unbound, Key = 0
auto graph = 100 * in;                    // build a graph referencing the placeholder
```

### Binding a placeholder with `substitute`

```c++
template <typename T, index_t Dims, size_t Key = 0>
bool substitute(expression_placeholder<T, Dims, Key>& expr,
                expression_handle<T, Dims> new_handle,
                csize_t<Key> = csize_t<Key>{});
```

Replaces the placeholder's stored handle with `new_handle`. The `Key` template
argument selects which placeholder to bind when several exist in the same graph.

```c++
auto in    = placeholder<float>();
auto graph = 100 * in;

substitute(graph, to_handle(counter<float>()));   // bind in to counter<float>

univector<float, 5> out = graph;                  // out = {0, 100, 200, 300, 400}
```

`substitute` is overloaded to walk composite expressions recursively: when called on
an `expression_with_arguments<Args...>`, it visits each argument and substitutes the
first matching placeholder. It can also be called on an `expression_handle`, in which
case it delegates to `expression_handle::substitute` through the vtable (only
`Key == 0` is supported for handles, since the concrete expression type is hidden).

```c++
// Substitute through a handle that wraps an expression containing a placeholder
expression_handle<float> h = to_handle(10 * placeholder<float>());
substitute(h, to_handle(counter<float>()));

univector<float, 5> out = h;   // out = {0, 10, 20, 30, 40}
```

## Examples

### Wrapping an expression in a handle

```c++
// Non-owning: references the counter
auto c = counter<float>();
auto h = to_handle(c);

// Owning: moves the counter into the handle
auto h2 = to_handle(counter<float>());
```

### Replacing an argument at runtime

A handle stored inside a composite expression can be swapped out after the graph is
built by reassigning the corresponding argument:

```c++
univector<float, 10> v1 = counter();
univector<float, 10> v2 = -counter();

auto e = to_handle(v1) * 10;          // e references v1
std::get<0>(e.args) = to_handle(v2);  // now e references v2

// e now computes v2 * 10 == -10 * index
```

### Building a deferred graph with placeholders

```c++
auto in    = placeholder<float>();
auto graph = 100 * in;

// Bind later, possibly multiple times
substitute(graph, to_handle(counter<float>()));
univector<float, 5> a = graph;        // {0, 100, 200, 300, 400}

substitute(graph, to_handle(gen_linear(0.f, 1.f)));
univector<float, 5> b = graph;        // {0, 100, 200, 300, 400} (same values)
```

### Type-erased graph behind a handle

```c++
expression_handle<float> h = to_handle(10 * placeholder<float>());

// The concrete type of "10 * placeholder<float>()" is now hidden behind h.
// The placeholder can still be substituted through the vtable.
substitute(h, to_handle(counter<float>()));

univector<float, 5> out = h;          // {0, 10, 20, 30, 40}
```

## API reference

### `expression_handle`

| Member / Function                                         | Description                                                                                                                                           |
|-----------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| `expression_handle()`                                     | Constructs an empty (invalid) handle.                                                                                                                 |
| `expression_handle(instance, vtable, resource = nullptr)` | Constructs a handle referencing `instance` with the given `vtable`. If `resource` is set, the handle owns the expression.                             |
| `operator bool()`                                         | `true` if the handle references a valid instance.                                                                                                     |
| `substitute(new_handle)`                                  | Replaces the placeholder inside the referenced expression with `new_handle`. Returns `true` if a placeholder was found. Only `Key == 0` is supported. |

### `expression_placeholder`

| Member / Function          | Description                                                            |
|----------------------------|------------------------------------------------------------------------|
| `expression_placeholder()` | Default constructor; creates an unbound placeholder.                   |
| `handle`                   | The `expression_handle<T, Dims>` currently bound, or empty if unbound. |

### Free functions

| Function                                      | Description                                                                       |
|-----------------------------------------------|-----------------------------------------------------------------------------------|
| `to_handle(E& expr)`                          | Returns a non-owning handle referencing `expr`.                                   |
| `to_handle(E&& expr)`                         | Returns an owning handle that moves `expr` into heap storage.                     |
| `placeholder<T, Dims, Key>()`                 | Creates an unbound `expression_placeholder`.                                      |
| `substitute(placeholder, new_handle, key)`    | Binds a placeholder to `new_handle`.                                              |
| `substitute(expr_with_args, new_handle, key)` | Recursively substitutes the first matching placeholder in a composite expression. |
| `substitute(handle, new_handle, key)`         | Substitutes through a handle via its vtable (`Key == 0` only).                    |

### `expression_vtable`

Internal virtual table that stores function pointers for shape query, placeholder
substitution, pass begin/end, and `get_elements`/`set_elements` for each axis and
power-of-two block size. One static instance is created per concrete expression type
when `to_handle` is called; users do not normally interact with it directly.

| Constant | Description                                                                              |
|----------|------------------------------------------------------------------------------------------|
| `Nsizes` | Number of supported power-of-two block sizes (`1 + ilog2(maximum_expression_width<T>)`). |
| `Nmax`   | Largest supported block size (`1 << Nsizes`).                                            |



