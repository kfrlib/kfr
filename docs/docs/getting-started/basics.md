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
#include <kfr/audio.hpp> // Audio encoding/decoding
```

## Types

### SIMD vector

[[`vec`:nosig]] is a template class that contains 1 or more elements and lets functions operate them in a machine-efficient way.

The class synopsis:

```c++
||||||||||||
template <typename T, size_t N>
struct alignas(...) vec
{
    using value_type = T;
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
||||||||||||
```

!!! note
    The class implementation is specific to the target cpu, so [[`vec`:nosig]] class definition resides in `kfr::KFR_ARCH_NAME` namespace. For avx2 architecture it's `kfr::avx2`. The architecture namespace is declared inline, you should not use it directly, [[`vec`:nosig]] as treated by compiler as an alias for `kfr::KFR_ARCH_NAME::vec`.


You can omit the template parameters and let compiler deduce them for you:
```c++
|||#include <kfr/base.hpp>
|||using namespace kfr;
|||static void some_function(vec<int, 3>) {}
|||TEST_CASE("basics.md/vec deduction")
|||{
vec x{ 10, 5, 2.5, 1.25 }; // vec<double, 4>

some_function(vec{ 3, 2, 1 }); // vec<int, 3>
|||static_assert(std::is_same_v<decltype(x), vec<double, 4>>);
|||CHECK_THAT(x, DeepMatcher(vec<double, 4>{ 10, 5, 2.5, 1.25 }));
|||}
```

Vectors can be nested. `vec<vec<float, 2>, 2>` is a valid declaration.

#### Using [[`vec`:nosig]] directly

[[`vec`:nosig]] is the building block of all KFR SIMD operations. While most KFR functions accept [[`univector`:nosig]], [[`tensor`:nosig]] or any [expression](../expressions/expressions.md) and operate on [[`vec`:nosig]] internally, you can also use [[`vec`:nosig]] directly for low-level, register-sized computations.

```c++
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("basics.md/vec directly")
|||{
// Construct from scalars (size and type deduced)
vec<float, 4> a{ 1, 2, 3, 4 };
vec<float, 4> b{ 10, 20, 30, 40 };

// Broadcast a scalar to all lanes
vec<float, 4> c = 0.5f;            // { 0.5, 0.5, 0.5, 0.5 }

// Element-wise arithmetic maps to a single SIMD instruction
vec<float, 4> sum  = a + b;        // { 11, 22, 33, 44 }
vec<float, 4> prod = a * b;        // { 10, 40, 90, 160 }
|||CHECK_THAT(sum, DeepMatcher(vec<float, 4>{ 11, 22, 33, 44 }));
|||CHECK_THAT(prod, DeepMatcher(vec<float, 4>{ 10, 40, 90, 160 }));

// Accessing individual elements (goes through a proxy)
float first = a.front();
a[2]        = 100.0f;
|||CHECK(first == 1.0f);
|||CHECK(a[2] == 100.0f);

// Concatenating smaller vectors into a larger one
vec<float, 2> lo{ 1, 2 };
vec<float, 2> hi{ 3, 4 };
vec<float, 4> ab = concat(lo, hi); // { 1, 2, 3, 4 }
|||CHECK_THAT(ab, DeepMatcher(vec<float, 4>{ 1, 2, 3, 4 }));

// Implicit element-type conversion
vec<double, 4> d = a;              // int/float -> double, widened
|||CHECK_THAT(d, DeepMatcher(vec<double, 4>{ 1, 2, 100, 4 }));
|||}
```

[[`vec`:nosig]] is also the type returned by KFR expression primitives such as `read<N>(ptr)`, `gather`, `scatter`, etc. When you write generic code that should operate on a single SIMD register, prefer taking/returning [[`vec`]] rather than a raw pointer.

!!! tip
    [[`vec`:nosig]] size `N` should usually match the native SIMD width for the element type (e.g. 4 `double`s or 8 `float`s on AVX2), or be a small integer multiple of it (`2N`, `4N`, ...). As long as the total width fits within the target's overall register file (check [[`vector_capacity`:nosig]]<T>), no overhead is introduced — KFR simply unrolls the operation over the available registers. Sizes that are not a multiple of the native width, or that exceed the register file, may require extra shuffling or spilling.

### 1D array

[[`univector`:nosig]] is a template class that, based on the template parameter, may hold data in heap like `std::vector`, in its storage like `std::array` or hold pointer to external data like `std::span`.

#### [[`univector<T, tag_dynamic_vector>`]]

This specialization holds data in heap. Memory is automatically aligned.

[[`univector<T, tag_dynamic_vector>`]] is derived from `std::vector<T>` but with KFR own allocator that provides alignment for memory allocation and contains all member functions and constructors from `std::vector<T, ...>`.

#### [[`univector<T, Size>`]]

This specialization holds data in its storage. [[`univector<T, Size>`]] is derived from `std::array<T>` and contains all member functions and constructors from `std::array<T>` but is properly aligned.

`Size` must not be zero.

#### [[`univector<T, tag_array_ref>`]]

This specialization works like `std::span` from C++20 and holds only pointer and size to external memory. Data alignment is preserved and cannot be enforced.

For all specializations data is always contiguous in memory.

#### Alignment

For SIMD operations to be effective, data should be aligned to 16, 32 or 64 bytes boundary. Default STL allocator cannot provide such alignment, so holding data in `std::vector<T>` with default allocator may be suboptimal.

KFR has its own STL-compatible allocator [[`data_allocator`:nosig]] that aligns memory to 64-bytes boundary. Using it with STL containers may increase performance.

!!! note
    Define `KFR_USE_STD_ALLOCATION` macro to make [[`data_allocator`:nosig]] an alias for `std::allocator`. This makes [[`univector`:nosig]]s interchangeable with `std::vector`s

!!! warning
    Alignment is only guaranteed for memory **allocated** by a container. It is **not** guaranteed when a container wraps an external pointer (e.g. `univector<T, 0>`, [[`tensor<T, NDims>`]] constructed from `T*`, [[`audio_data<IsInterleaved>`]] constructed from external buffers) or when you obtain a subrange via `slice`/`truncate`/`operator()`. In those cases KFR will still work correctly, but unaligned loads/stores may be emitted, which can be slower on some targets.

#### Passing 1D data to KFR functions

Many KFR functions, such as DFT, receive and return data through [[`univector`:nosig]] class. If it's possible, use [[`univector`:nosig]] in your code as a storage for all data that may be passed to KFR functions. But if you already have data and need to pass it to KFR, you may use [[`make_univector`:nosig]] function that constructs [[`univector`:nosig]]<T, 0> from the pointer and the size or from a STL-compatible container (if `data()` and `size()` is defined).

```c++
|||#include <kfr/base.hpp>
|||using namespace kfr;
|||TEST_CASE("basics.md/make_univector from container")
|||{
std::vector<float> data; // existing data, or std::array<N, float>

float val = rms(make_univector(data)); // No data copy
|||(void)val;
|||}
```

```c++
|||TEST_CASE("basics.md/make_univector from pointer")
|||{
|||std::vector<float> storage{ 1, 2, 3, 4 };
const float* data; // existing data
size_t size;       // 
|||data = storage.data();
|||size = storage.size();

float val = rms(make_univector(data, size)); // No data copy
|||CHECK(val > 0);
|||}
```

```c++
|||TEST_CASE("basics.md/make_univector from array")
|||{
const float data[1024] = {};

float val = rms(make_univector(data)); // No data copy
|||(void)val;
|||}
```

#### Slice

You can get subrange of an array using [[`univector_base<T, Class, true>::slice(size_t, size_t)`:nosig:noscope]] function defined in all specializations of [[`univector`:nosig]] class. 

```c++
univector<float, 100> v;
// ...
const float s1 = sum(v); // Sum all elements
const float s2 = sum(v.slice(2, 50)); // Sum 50 elements starting from 2
```

Result of the call to [[`univector_base<T, Class, true>::slice(size_t, size_t)`:nosig:noscope]] is always [[`univector_ref`]], a reference to external data.
Note that the lifetime of the reference is limited to the lifetime of the original data.

!!! note
    [[`univector`:nosig]] class is also an [Expression](../expressions/expressions.md) and can be used wherever an expression is required. This means you can pass a [[`univector`:nosig]] directly to any KFR function expecting an expression argument, and you can assign expressions back into a [[`univector`:nosig]]:
    ```c++
    univector<float> x = counter(0, 0.5f, 1.0f); // fill from an expression
    univector<float> y = sin(x);                  // element-wise sin
    ```

### Tensor (Multidimensional array)

[[`tensor`:nosig]] is a class that holds or references multidimensional data and provides 
a way to access individual elements and perform complex operations on the data.

The number of elements in each axis of the array is defined by its _shape_.
The number of dimensions is fixed at compile time.

Tensor class synopsis:

```c++
||||||||||||

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
||||||||||||
```

Iteration is always goes from the first axis to the last axis.

By default the last axis is contiguous in memory but it can be changed with custom `strides`.

```c++
tensor<double, 1> t1{ 1, 2, 3, 4, 5, 6 };
tensor<double, 2> t2{ {1, 2}, {3, 4}, {5, 6} };
tensor<double, 3> t3{ {{1}, {2}}, {{3}, {4}}, {{5}, {6}} };
// Memory layout for all these tensors is: 1, 2, 3, 4, 5, 6
```

> [!IMPORTANT]
> `const`-qualified tensors are writable. This makes it possible to pass a writable subrange to a function without converting it to an lvalue.

Tensor behaves like a shared pointer to memory (possibly allocated outside [[`tensor`:nosig]] class, see [Constructing tensor from external data](#constructing-tensor-from-external-data)) with automatic reference counting. Copy and assignment increments internal counter and the internal pointer still references the original data. 

_Important_: Writing to one shared copy will modify all other copies of the this tensor too.
To get a deep copy call the [[`tensor<T, NDims>::copy`:nosig:noscope]] member function:

```c++
|||#include <kfr/base.hpp>
|||using namespace kfr;
|||TEST_CASE("basics.md/tensor copy")
|||{
|||tensor<float, 2> other(shape{ 2, 2 }, 1.0f);
tensor<float, 2> t = other;
t = t.copy();
|||CHECK_THAT(t, DeepMatcher(other));
|||}
```

#### Reshaping

[[`tensor<T, NDims>::reshape`:nosig:noscope]] and [[`tensor<T, NDims>::flatten`:nosig:noscope]] functions perform reshaping and return new tensor that shares data with the original tensor.

Not every tensor may be reshaped to any shape. The total number of elements must be same before and after reshaping.

Also, to be able to share data the original tensor must be contiguous. If this requirement isn't meet, [[`tensor<T, NDims>::reshape`:nosig:noscope]] and [[`tensor<T, NDims>::flatten`:nosig:noscope]] functions throw [[`logic_error`:nosig]] exception. 
There are variants of these functions called [[`tensor<T, NDims>::reshape_may_copy`:nosig:noscope]] and [[`tensor<T, NDims>::flatten_may_copy`:nosig:noscope]] that return a new tensor that does not share data with the original tensor in that cases.

#### Slicing

To slice the original array the special value constructed by [[`trange`:nosig]], [[`tstart`:nosig]], [[`tstop`:nosig]] or [[`tall`:nosig]] functions should be passed to tensor's [[`tensor<T, NDims>::operator()`:nosig]].

```c++
||||||||||||
constexpr tensor_range trange(std::optional<signed_index_t> start = std::nullopt,
                              std::optional<signed_index_t> stop  = std::nullopt,
                              std::optional<signed_index_t> step  = std::nullopt)
{
    return { start, stop, step };
}
||||||||||||
```

If `start` is nullopt, the slice starts from the first element (or the last one if step is negative). If `stop` is nullopt, the slice ends at the last element (or the first one if step is negative).
If `step` is nullopt or omitted, the step will be equal to 1.

[[`tstart`:nosig]](start) and [[`tstart`:nosig]](start, step) are equivalents of calling [[`trange`:nosig]](start, nullopt, nullopt) and [[`trange`:nosig]](start, nullopt, step) and used to return the range starting from the `start` along the given axis.

[[`tstop`:nosig]](stop) and [[`tstop`:nosig]](stop, step) are equivalents of calling [[`trange`:nosig]](nullopt, stop, nullopt) and [[`trange`:nosig]](nullopt, stop, step) and used to return the range stopping at the `stop` along the given axis.

[[`tall`:nosig]]() is equivalent of [[`trange`:nosig]](nullopt, nullopt, nullopt) and used to return the whole range of the given axis.

Examples:

```c++
|||TEST_CASE("basics.md/tensor slicing")
|||{
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
tensor<double, 2> t2 = t1(tstart(2), trange(2, 4));
// t2 =
// {{22, 23},
//  {32, 33},
//  {42, 43},
//  {52, 53},
//  {62, 63},
//  {72, 73}}
|||CHECK_THAT(t2, DeepMatcher(tensor<double, 2>{ { 22, 23 }, { 32, 33 }, { 42, 43 }, { 52, 53 }, { 62, 63 }, { 72, 73 } }));
|||}
```

#### Constructing tensor from external data

A [[`tensor`:nosig]] can either own its memory (allocated cache-aligned) or reference external storage. To take ownership of an existing container without copying its data, use [[`tensor_from_container`:nosig]], which moves the container into a [[`memory_finalizer`:nosig]] so the underlying buffer stays alive as long as the [[`tensor`:nosig]] does:

```c++
tensor<float, 1> fn(std::vector<float>&& v)
{
    tensor<float, 1> t = tensor_from_container(std::move(v));
    // no data copy is performed. v is being moved to finalizer
    // and tensor references original vector data
    return t;
}
```

For full control (e.g. wrapping a buffer allocated by another library), construct a [[`tensor`:nosig]] directly from a pointer, shape, strides and a [[`memory_finalizer`:nosig]]. The finalizer is a `std::shared_ptr` whose destructor releases the storage:

```c++
|||static void* some_lib_alloc(size_t n) { return ::operator new(n); }
|||static void some_lib_free(void* p) { ::operator delete(p); }
|||TEST_CASE("basics.md/tensor from foreign buffer")
|||{
// Wrap a foreign buffer and free it with a custom callback when the tensor is destroyed.
float* foreign = static_cast<float*>(some_lib_alloc(1024 * sizeof(float)));
tensor<float, 1> t(foreign, shape<1>{ 1024 },
                   make_memory_finalizer([foreign] { some_lib_free(foreign); }));
|||CHECK(t.data() == foreign);
|||}
```

!!! note
    [[`tensor`:nosig]] uses reference counting for its finalizer, so copying a tensor only increments the counter — the underlying buffer is shared. Call [[`tensor<T, NDims>::copy`:nosig]]() for a deep copy.

### Audio data

[[`audio_data`:nosig]] (defined in `<kfr/audio/data.hpp>`) is a container tailored for **multi-channel audio**. It simplifies audio I/O and inter-channel processing by bundling the sample buffers together with channel count, frame count and an optional position.

```c++
||||||||||||
template <bool IsInterleaved = false>
struct audio_data
{
    uint32_t channels;                 // number of channels
    chan<fbase*, IsInterleaved> data;  // channel pointers (planar) or single buffer (interleaved)
    size_t   size;                     // frames per channel
    size_t   capacity;                 // allocated capacity per channel
    int64_t  position;                 // position of the first sample
    std::shared_ptr<void> deallocator; // optional ownership/finalizer
    // ...
};

using audio_data_planar       = audio_data<false>;
using audio_data_interleaved  = audio_data<true>;
||||||||||||
```

Key properties:

- **Floating point only.** [[`audio_data`:nosig]] always stores samples as [[`fbase`:nosig]] (`float` or `double`, depending on platform support and `KFR_BASETYPE_F32`). Integer PCM formats are handled by the [[`samples_load`:nosig]] / [[`samples_store`:nosig]] helpers (see below), which convert to/from [[`fbase`:nosig]].
- **Planar or interleaved.** The `IsInterleaved` template parameter selects the layout. [[`audio_data_planar`:nosig]] keeps one pointer per channel; [[`audio_data_interleaved`:nosig]] keeps a single contiguous buffer with samples interleaved (`L0 R0 L1 R1 ...`). A converting constructor lets you copy between the two layouts.
- **Sample conversion.** [[`samples_load`:nosig]] / [[`samples_store`:nosig]] move data between [[`fbase`:nosig]] buffers and any PCM sample type (`i16`, `i24`, `i32`, `f32`, `f64`), with optional byte swapping and [[`audio_quantization`:nosig]] (bit depth + dithering). The [[`audio_sample_type`:nosig]]-based overloads dispatch on a runtime tag.
- **Audio I/O.** All KFR audio readers/writers ([[`decode_audio_file`:nosig]], [[`encode_audio_file`:nosig]], ...) produce or consume [[`audio_data`:nosig]]. The accompanying [[`audiofile_format`:nosig]] carries container, codec, endianness, bit depth, sample rate, channel count, speaker arrangement and a [[`metadata_map`:nosig]] of arbitrary key/value pairs.
- **Ownership.** Like [[`tensor`:nosig]], [[`audio_data`:nosig]] may own its storage (allocated cache-aligned) or reference an external buffer via a custom deallocator (`std::shared_ptr<void>`). Copying is cheap — only the pointers and the shared deallocator are copied.
- **Expressions.** [[`audio_data`:nosig]].[[`audio_data<IsInterleaved>::channel`:noscope]] returns a [[`univector_ref`:nosig]]<fbase> (planar) or a [[`strided_channel`:nosig]]<fbase> (interleaved), both of which are [expressions](../expressions/expressions.md). This lets you apply any KFR expression per channel:

```c++
|||TEST_CASE("basics.md/audio_data channel expression")
|||{
audio_data_planar audio = decode_audio_file("input.wav").value();|||audio_data_planar audio(2, 256, fbase(1.0));

for (size_t ch = 0; ch < audio.channel_count(); ++ch)
{
    univector_ref<fbase> in = audio.channel(ch);
    // Apply an expression and write back
    in = in * 0.5f + sin(in * 3.14159f);
}
|||CHECK(std::isfinite(audio.channel(0)[0]));
|||}
```

#### Constructing [[`audio_data`:nosig]]

```c++
|||TEST_CASE("basics.md/audio_data construction")
|||{
// Allocate a 2-channel, 44100-frame planar buffer (zero-initialized storage)
audio_data_planar a(2, 44100);

// Allocate and fill with a constant
audio_data_planar b(2, 44100, 0.0f);
|||CHECK(b.channel(0)[0] == 0.0f);

// Wrap an external interleaved buffer (no ownership)
fbase external[2 * 1024];
audio_data_interleaved ref(external, 2, 1024);

// Wrap an external buffer and take ownership via a custom deallocator
fbase* buf = aligned_allocate<fbase>(2 * 1024, 64);
audio_data_interleaved owned(buf, 2, 1024, [buf] { aligned_deallocate(buf); });
|||CHECK(owned.channel_count() == 2);
|||}
```

#### Slicing and appending

[[`audio_data`:nosig]] provides [[`audio_data<IsInterleaved>::slice`:nosig:noscope]], [[`audio_data<IsInterleaved>::truncate`:nosig:noscope]], [[`audio_data<IsInterleaved>::slice_past_end`:nosig:noscope]], [[`audio_data<IsInterleaved>::append`:nosig:noscope]], [[`audio_data<IsInterleaved>::prepend`:nosig:noscope]] (with both same-layout and opposite-layout overloads), [[`audio_data<IsInterleaved>::resize(size_t)`:nosig:noscope]], [[`audio_data<IsInterleaved>::reserve`:nosig:noscope]] and [[`audio_data<IsInterleaved>::fill`:nosig:noscope]], mirroring the convenience of [[`univector`:nosig]] but operating on every channel at once.

```c++
|||TEST_CASE("basics.md/audio_data slicing and appending")
|||{
|||int sample_rate = 44100;
audio_data_planar a = decode_audio_file("song.wav").value();|||audio_data_planar a(2, sample_rate, fbase(1.0));

// First 10 seconds (assuming a.sample_rate is known from audiofile_format)
audio_data_planar intro = a.slice(0, 10 * sample_rate);|||audio_data_planar intro = a.slice(0, a.size);

// Concatenate two files with the same layout
audio_data_planar combined;
combined.append(a);
combined.append(other);|||combined.append(intro);
|||CHECK(combined.size == a.size + intro.size);
|||}
```

### Choosing the right container

KFR provides three main data containers. Pick the one that best matches your data shape and ownership needs:

| Container              | Dimensionality   | Element types                         | Layouts / Ownership                                                                                          | Expressions                                              |
|------------------------|------------------|---------------------------------------|--------------------------------------------------------------------------------------------------------------|----------------------------------------------------------|
| [[`univector`:nosig]]  | 1D               | any integer / float / boolean         | static (`std::array`-like), dynamic (`std::vector`-like) or span-like reference, all under one template name | yes (1D)                                                 |
| [[`tensor`:nosig]]     | N-D (fixed)      | any                                   | owns or references; custom strides; custom deallocator; can capture any container                            | yes                                                      |
| [[`audio_data`:nosig]] | multi-channel 1D | [[`fbase`:nosig]] only (float/double) | planar or interleaved (template flag); owns or references; custom deallocator                                | yes (via [[`audio_data<IsInterleaved>::channel`:noscope]]) |

**Use [[`univector`:nosig]]** for the simplest 1D case. A single template name covers static, dynamic and reference storage, which makes it ideal for writing generic functions that should accept any 1D buffer. It supports any integer, floating point or boolean element type and is a full [expression](../expressions/expressions.md), so it composes with all KFR math/DSP primitives.

**Use [[`tensor`:nosig]]** when you need a powerful multidimensional container. It can own its data or reference it (with custom strides and a custom deallocator), and it can capture ownership of any STL-compatible container via [[`tensor_from_container`:nosig]]. Expressions work across all axes. However, SIMD acceleration is applied along a single axis. Use it for matrices, spectrograms, batched FFTs and similar workloads.

**Use [[`audio_data`:nosig]]** to simplify multi-channel audio processing and I/O. It supports only floating-point samples ([[`fbase`:nosig]]), but ships with sample conversion helpers that handle any bit depth, channel count and endianness. Audio I/O functions read into and write from [[`audio_data`:nosig]], and the accompanying [[`audiofile_format`:nosig]] carries metadata such as sample rate, speaker arrangement and a `std::map` of custom key/value pairs. Like [[`tensor`:nosig]], it supports custom deallocation and optional ownership, and it exposes template expressions through its `channel()` member.

!!! note
    All three containers allocate **cache-aligned** memory (64-byte boundary) when they own their storage. However, none of them *guarantee* alignment in general, because any of them can wrap a user-supplied pointer, and the `slice` / `truncate` / `operator()` family of functions may return sub-buffers whose start address is not aligned. KFR always produces correct results on unaligned data, but for maximum throughput prefer owned storage and operate on whole buffers when possible.
