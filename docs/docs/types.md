# Data types

`univector` class is a base of all containers in KFR.

`univector` can have both static and dynamic size and can even hold only reference to an external data (just like `array_view` or `string_view`)

`univector<float>` is derived from `std::vector<float>` and contains all its member functions and constructors.

`univector<float, 10>` is derived from `std::array<float, 10>` and contains all its member functions and constructors.

`univector<float, 0>` is only reference to data and doesn’t contain any values.

Such universal template allows functions in KFR to get data in any format.

You can get subrange of an array using slice function:

```c++
univector<float, 100> v;
// ...
const float s1 = sum(v); // Sum all elements
const float s2 = sum(v.slice(2, 50)); // Sum 50 elements starting from 2
```
Result of the call to slice is always `univector<T, 0>`, a reference to external data.

The lifetime of the reference is limited to the lifetime of the original data.

!!! note
    `univector` class is also [Expression](expressions.md) and can be used whereever expression is required.

## Pass data to KFR functions

If you don't use `univector` for data representation, you can still pass the data to KFR functions and filters. 

Examples (`rms` is used as an example function that takes `univector`):

### `std::vector` or `std::array`
```c++
std::vector<float> data; // existing data, or std::array<N, float>

float val = rms(make_univector(data)); // No data copy
```

### Plain pointer
```c++
const float* data; // existing data
size_t size;       // 

float val = rms(make_univector(data, size)); // No data copy
```

### array
```c++
const float data[1024];

float val = rms(make_univector(data)); // No data copy
```


## Data Types

Unsigned:

``u8``, ``u16``, ``u32`` and ``u64``

Signed:

``i8``, ``i16``, ``i32`` and ``i64``

Floating point:

``f32`` and ``f64``

Complex:

``complex<f32>`` and ``complex<f64>``

Vector:

``vec<u8, 4>``, ``vec<f32, 3>``, ``vec<i64, 1>``, ``vec<complex<float>, 15>``, ``vec<u8, 256>``, ``vec<vec<int, 3>, 3>``

You are not limited to sizes of SIMD registers and basic types.