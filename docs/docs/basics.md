# Basics

To include all KFR modules use `kfr/all.hpp` header. Note that DFT and IO modules need linking appropriate static libraries, `kfr_dft` and `kfr_io`.

```c++
#include <kfr/all.hpp>
```

Alternatively, you can include only the modules you need:

```c++
#include <kfr/base.hpp> // Functions, expressions
#include <kfr/dft.hpp> // DFT/DCT and convolution
#include <kfr/dsp.hpp> // DSP, Filters etc
#include <kfr/io.hpp> // Audio file reading
```

## Types

### SIMD vector

`vec` is a template class that contains 1 or more elements and lets functions operate them in a machine-efficient way.

The class synopsis:

```c++
template <typename T, size_t N>
struct alignas(...) vec
{
    using value_type = T;

    // Constructors
    // broadcast scalar to all elements
    vec(T broadcasting);
    // initialize all elements 
    template <typename... Ts>
    vec(Ts... elements);
    // concatenate vectors (sum(Ns...) == N)
    template <size_t... Ns>
    vec(const vec<T, Ns>&... concatenation);
    // implicit element type conversion
    template <typename U>
    vec(const vec<U, N>& conversion);
    ...

    // element is a proxy class to get/set specific element
    struct element
    {
    };

    // Functions to access elements, const-versions omitted
    // access by index
    element operator[](size_t index);
    // access first element
    element front();
    // access last element
    element back();

    // return size
    size_t size() const;
};
template <typename... T>
vec(T&&...) -> vec<std::common_type_t<T...>, sizeof...(T)>;
```

!!! note
    The class implementation is specific to the target cpu, so `vec` class definition resides in `kfr::CMT_ARCH_NAME` namespace. For avx2 architecture it's `kfr::avx2`. The architecture namespace is declared inline, you should not use it directly, `kfr::vec` as treated by compiler as an alias for `kfr::CMT_ARCH_NAME::vec`.


Use can omit the template parameters and let compiler deduce them for you:
```c++
vec x{ 10, 5, 2.5, 1.25 }; // vec<double, 4>

some_function(vec{ 3, 2, 1 }); // vec<int, 3>
```

Vectors can be nested. `vec<vec<float, 2>, 2>` is a valid declaration.

### 1D array

`univector` is a template class that, based on the template parameter, may hold data in heap like `std::vector`, in its storage like `std::array` or hold pointer to external data like `std::span`.

#### `univector<T>`

This specialization holds data in heap. Memory is automatically aligned.

`univector<T>` is derived from `std::vector<T>` but with KFR own allocator that provides alignment for memory allocation and contains all member functions and constructors from `std::vector<T, ...>`.

#### `univector<T, Size>`

This specialization holds data in its storage. `univector<T, Size>` is derived from `std::array<T>`and contains all member functions and constructors from `std::array<T>` but is properly aligned.

`Size` must not be zero.

#### `univector<T, 0>`

This specialization works like `std::span` from C++20 and holds only pointer and size to external memory. Data alignment is preserved and cannot be enforced.

For all specializations data is always contiguous in memory.

#### Alignment

For SIMD operations to be effective, data should be aligned to 16, 32 or 64 bytes boundary. Default STL allocator cannot provide such alignment, so holding data in `std::vector<T>` with defalt allocator may be suboptimal.

KFR has its own STL-compatible allocator `kfr::data_allocator` that aligns memory to 64-bytes boundary. Using it with STL containers may increase performance.

!!! note
    Define `KFR_USE_STD_ALLOCATION` macro to make `data_allocator` an alias for `std::allocator`. This makes `univector`s interchangeable with `std::vector`s

#### Passing 1D data to KFR functions

Many KFR functions, such as DFT, receive and return data through `univector` class. If it's possible, use `univector` in your code as a storage for all data that may be passed to KFR functions. But if you already have data and need to pass it to KFR, you may use `make_univector` function that constructs `univector<T, 0>` from the pointer and the size or from a STL-compatible container (if `data()` and `size()` is defined).

```c++
std::vector<float> data; // existing data, or std::array<N, float>

float val = rms(make_univector(data)); // No data copy
```

```c++
const float* data; // existing data
size_t size;       // 

float val = rms(make_univector(data, size)); // No data copy
```

```c++
const float data[1024];

float val = rms(make_univector(data)); // No data copy
```

#### Slice

You can get subrange of an array using `slice` function defined in all specializations of `univector` class. 

```c++
univector<float, 100> v;
// ...
const float s1 = sum(v); // Sum all elements
const float s2 = sum(v.slice(2, 50)); // Sum 50 elements starting from 2
```

Result of the call to `slice` is always `univector<T, 0>`, a reference to external data.
Not tat the lifetime of the reference is limited to the lifetime of the original data.

!!! note
    `univector` class is also [Expression](expressions.md) and can be used whereever expression is required.

### Tensor (Multidimensional array)

`tensor` is a class that holds or references multidimensional data and provides 
a way to access individual elements and perform complex operations on the data.

The number of elements in each axis of the array is defined by its _shape_.
The number of dimensions is fixed at compile time.

Tensor class synopsis:

```c++

struct memory_finalizer;

// T is the element type
// Dims is the number of dimensions
template <typename T, index_t Dims>
struct tensor
{
    using value_type = T;
    using shape_type = shape<Dims>;

    // iterates through flattened array
    struct tensor_iterator;
    
    // iterates nested arrays
    struct nested_iterator;

    // construct from external pointer, shape, strides and finalizer
    tensor(T* data, const shape_type& shape, const shape_type& strides,
           memory_finalizer finalizer);

    // construct from external pointer, shape and finalizer with default strides
    tensor(T* data, const shape_type& shape, memory_finalizer finalizer);

    // construct from shape and allocate memory
    tensor(const shape_type& shape);

    // construct from shape, strides and allocate memory
    tensor(const shape_type& shape, const shape_type& strides);

    // construct from shape, allocate memory and fill with value
    tensor(const shape_type& shape, T value);

    // construct from shape, strides, allocate memory and fill with value
    tensor(const shape_type& shape, const shape_type& strides, T value);
    
    // construct from shape, allocate memory and fill with flat list
    tensor(const shape_type& shape, const std::initializer_list<T>& values);
    
    // initialize with braced list. defined for 1D tensor only
    template <typename U>
    tensor(const std::initializer_list<U>& values);

    // initialize with nested braced list. defined for 2D tensor only
    template <typename U>
    tensor(const std::initializer_list<std::initializer_list<U>>& values);
    
    // initialize with nested braced list. defined for 3D tensor only
    template <typename U>
    tensor(const std::initializer_list<std::initializer_list<std::initializer_list<U>>>& values)

    // shape of tensor
    shape_type shape() const;
    // strides
    shape_type strides() const;
    
    pointer data() const;
    size_type size() const;
    bool empty() const;
    tensor_iterator begin() const;
    tensor_iterator end() const;

    // access individual element by index
    value_type& access(const shape_type& index) const;

    // access individual element by list of indices
    value_type& operator()(size_t... index) const;
    
    // return subrange, individual axis or slice
    template <typename... Index>
    tensor<T, ...> operator()(const Index&...) const;

    // return flattened array, see Reshaping below
    tensor<T, 1> flatten() const;
    // return reshaped array, see Reshaping below
    template <index_t dims>
    tensor<T, dims> reshape(const shape<dims>& new_shape) const;

    // convert multidimensional tensor to string
    template <typename Fmt = void>
    std::string to_string(int max_columns = 16, int max_dimensions = INT_MAX, std::string separator = ", ",
                          std::string open = "{", std::string close = "}") const;
};
```

Iteration is always goes from the first axis to the last axis.

By default the last axis is contiguous in memory but it can be changed with custom `strides`.

```c++
tensor<double, 1> t1{ 1, 2, 3, 4, 5, 6 };
tensor<double, 2> t2{ {1, 2}, {3, 4}, {5, 6} };
tensor<double, 3> t3{ {{1}, {2}}, {{3}, {4}}, {{5}, {6}} };
// Memory layout for all these tensors is: 1, 2, 3, 4, 5, 6
```

!!! important `const`-qualified tensors are writable. This is to make it possible to pass a writable subrange to function without converting to lvalue.

Tensor behaves like a shared pointer to memory (possibly allocated outside tensor class, see [Constructing tensor from external data](#constructing-tensor-from-external-data)) with automatic reference counting. Copy and assignment increments internal counter and the internal pointer still references the original data. 

_Important_: Writing to one shared copy will modify all other copies of the this tensor too.
To get a deep copy call the `copy` member function:

```c++
tensor<float, 2> t = other;
t = t.copy();
```

#### Reshaping

`reshape` and `flatten` functions perform reshaping and return new tensor that shares data with the original tensor.

Not every tensor may be reshaped to any shape. The total number of elements must be same before and after reshaping.

Also, to be able to share data the original tensor must be contiguous. If this requirement isn't meet, `reshape` and `flatten` functions throw `kfr::logic_error` exception. 
There are variants of these functions called `reshape_may_copy` and `flatten_may_copy` that return a new tensor that does not share data with the original tensor in that cases.

#### Slicing

To slice the original array the special value constructed by `trange`, `tstart`, `tstop` or `tall` functions should be passed to tensor's `operator()`.

```c++
constexpr tensor_range trange(std::optional<signed_index_t> start = std::nullopt,
                              std::optional<signed_index_t> stop  = std::nullopt,
                              std::optional<signed_index_t> step  = std::nullopt)
{
    return { start, stop, step };
}
```

If `start` is nullopt, the slice starts from the first element (or the last one if step is negative). If `stop` is nullopt, the slice ends at the last element (or the first one if step is negative).
If `step` is nullopt or omitted, the step will be equal to 1.

`tstart(start)` and `tstart(start, step)` are equivalents of calling `trange(start, nullopt, nullopt)` and `trange(start, nullopt, step)` and used to return the range starting from the `start` along the given axis.

`tstop(stop)` and `tstop(stop, step)` are equivalents of calling `trange(nullopt, stop, nullopt)` and `trange(nullopt, stop, step)` and used to return the range stopping at the `stop` along the given axis.

`tall()` is equivalent of `trange(nullopt, nullopt, nullopt)` and used to return the whole range of the given axis.

Examples:

```c++
tensor<double, 2> t1(shape{ 8, 6 });
// initialize tensor
t1 = counter(0, 10, 1);
// t1 =
// {{ 0,  1,  2,  3,  4,  5,  6,  7},
//  {10, 11, 12, 13, 14, 15, 16, 17},
//  {20, 21, 22, 23, 24, 25, 26, 27},
//  {30, 31, 32, 33, 34, 35, 36, 37},
//  {40, 41, 42, 43, 44, 45, 46, 47},
//  {50, 51, 52, 53, 54, 55, 56, 57}}

// slice tensor
tensor<double, 2> t2 = t(tstart(2), trange(2, 4));
// t2 =
// {{22, 23, 24},
//  {32, 33, 34},
//  {42, 43, 44},
//  {52, 53, 54}}
```

#### Constructing tensor from external data

```c++
tensor<float, 1> fn(std::vector<float>&& v)
{
    tensor<float, 1> t = tensor_from_container(std::move(v));
    // no data copy is performed. v is being moved to finalizer
    // and tensor references original vector data
    return t;
}
```

