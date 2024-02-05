# Expressions

In KFR, all data is represented through _Expression_ concept.
Expression is a virtual multidimensional array consisting of elements that can be read (Input expression) or written to (Output expression).
Expression can have specific size or have infinite size in any dimension. Its size may be known at compile time or at runtime only.

The number of dimensions must be known at compile time.

Normally, expressions do not own any data and can be seen as _data generators_ with any algorithm under the hood. But classes owning data (`tensor` and `univector`) provide expression interface as well. Since KFR5 you can make expression from any user defined or `std` type.

Expressions can refer to other expressions as its arguments.
prvalue expressions are captured by value and moved to expression storage.
lvalue expressions are captured by reference. 
The latter may cause dangling references if the resulting expression is used outside of its arguments scope. As always, `std::move` forces variable to be captured by value.

The following function creates Expression that represents a virtual 3-dimensional array with elements starting from 0 at $(0,0,0)$ index
and incremented by $1$, $10$ and $100$ along each axis.
```
counter(0, 1, 10, 100)
```

To get a single element from Expression, call `get_element` function. It takes the expression itself and the index.
```
get_element(counter(0, 1, 10, 100), {1, 2, 3}) == 321
```

Calling functions or operators on expressions or arrays of data is performed lazily in KFR.
This allows better optimization and does not require saving temporary data.

Internally a C++ technique called [Expression templates](https://en.wikipedia.org/wiki/Expression_templates) is used but expressions processing is explicitly vectorized in KFR. You can control some aspects of vectorization.

## Functions and operators

For example, subtracting one univector from another produces expression, not univector:

```c++
univector<int, 5> x{1, 2, 3, 4, 5};
univector<int, 5> y{0, 0, 1, 10, -5};

auto z = x - y; // z is of type expression_function<...>, not univector. 
                // This only constructs an expression and does not perform any calculation
```

You should assign expression to a univector (or tensor) to get the data:

```c++
univector<int, 5> x{1, 2, 3, 4, 5};
univector<int, 5> y{0, 0, 1, 10, -5};    

univector<int, 5> z = x - y;
```

!!! note
    When an expression is assigned to a `univector` variable, expression is evaluated in `process` function and values are being written to the target storage.

Same applies to calling KFR functions on univectors, this doesn't calculate values immediately. Instead, new expression will be created.

```c++
univector<float, 5> x{1, 2, 3, 4, 5};
sqrt(x);                                // only constructs an expression
univector<float, 5> values = sqrt(x);   // constructs an expression and writes data to univector
```

Element type of an input expressions can be determined by using `expression_value_type<Expr>` (In KFR4 it was `value_type_of<Expr>`). Since KFR5 all expressions have their type specified.

### Writing custom expressions

To turn a user-defined class into expression, you should define
`kfr::expression_traits<Class>` and provide typedefs and function to work with it.

Input expressions must implement `get_elements` function to retrieve elements at given index.

Output expressions must implement `set_elements` function to set its elements.

Example of input expression:

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
    // element type for expression
    using value_type             = T;

    // number of dimensions, must be known at compile time
    constexpr static size_t dims = 2;

    // function to retrieve shape (size) of matrix, runtime version
    constexpr static shape<2> get_shape(const identity_matrix<T, Size>& self) { return { Size, Size }; }

    // function to retrieve shape (size) of matrix, compile time version
    // if the size is unknown at compile time the function must be still defined
    // but return undefined_size for every axis with unknown size
    constexpr static shape<2> get_shape() { return { Size, Size }; }
};

template <typename T, index_t Size, index_t Axis, size_t N>
vec<T, N> get_elements(const identity_matrix<T, Size>& self, const shape<2>& index,
                       const axis_params<Axis, N>& sh)
{
    // identity matrix expression returns 1 if col==row, 0 otherwise
    // indices<A> returns indices for given axis taking vectorization into account
    // select is a SIMD-enabled ternary operator function
    return select(indices<0>(index, sh) == indices<1>(index, sh), 1, 0);
}

}

int main()
{
    using namespace kfr;
    // Create identity_matrix expression with 9 cols/rows,
    // Render it to tensor class (trender)
    // Convert to string with maximum 16 columns of values
    // And 0 dimensions collapsed (to_string).
    println(trender(identity_matrix<float, 9>{}).to_string(16, 0));
}
```

**Output**:

```c++
{{1, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 1, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 1, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 1, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 1, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 1, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 1, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 1, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 1}}
```

Expression may be defined in a compact form using only a single class:

```c++
template <typename T, index_t Size>
struct identity_matrix : expression_traits_defaults
{
    using value_type             = T;

    constexpr static size_t dims = 2;
    constexpr static shape<2> get_shape(const identity_matrix& self) { return { Size, Size }; }
    constexpr static shape<2> get_shape() { return { Size, Size }; }

    template <index_t Axis, size_t N>
    friend vec<T, N> get_elements(const identity_matrix& self, const shape<2>& index,
                                  const axis_params<Axis, N>& sh)
    {
        return select(indices<0>(index, sh) == indices<1>(index, sh), 1, 0);
    }
};

```

The same class with the size defined at runtime.

```c++
template <typename T>
struct identity_matrix : expression_traits_defaults
{
    using value_type             = T;

    constexpr static size_t dims = 2;
    constexpr static shape<2> get_shape(const identity_matrix& self) { return { self.size, self.size }; }
    // undefined_size means size is not known at compile time
    constexpr static shape<2> get_shape() { return { undefined_size, undefined_size }; }

    template <index_t Axis, size_t N>
    friend vec<T, N> get_elements(const identity_matrix& self, const shape<2>& index,
                                  const axis_params<Axis, N>& sh)
    {
        return select(indices<0>(index, sh) == indices<1>(index, sh), 1, 0);
    }

    index_t size;
};

```

### Reducing functions

Reducing functions accept 1D [Expression](expressions.md) and produce scalar.

Some of the reducing functions are:
`sum`, `rms`, `mean`, `dotproduct`, `product`, `sumsqr`.

Some of reducing functions have the same names as corresponding regular functions but with `of` suffix to distinguish them: 
`minof`, `maxof`, `absmaxof`, `absminof`
