# Migration to KFR6

Here is migration guide to upgrade from KFR5.x to KFR6.

For upgrade from KFR4.x to KFR5 see [Upgrade to KFR 5](upgrade5.md)

## Multiarchitecture and C API

### Multiarchitecture is enabled by default

In KFR5 only DFT had support for multiarchitecture. This was controlled by the`KFR_ENABLE_DFT_MULTIARCH` CMake option (default `OFF`).

Since KFR6, multiarchitecture is controlled by the `KFR_ENABLE_MULTIARCH` (default `ON`) and is supported by more algorithms.

`KFR_ARCHS` CMake variable controls the list of the architectures for which the code will be generated.

```
cmake ... -DKFR_ARCHS=sse2;avx;avx2
```

This feature is now fully transparent to the users of the KFR library and doesn't require any changes to the source code.

### C API is supported on non-x86 platforms

On Linux and macOS, the following CMake option is required for building the C API:

```shell
-DCMAKE_POSITION_INDEPENDENT_CODE=ON
```

Note that this makes all static libraries built in KFR directory position-independent.

The C API requires that `KFR_ENABLE_MULTIARCH` be enabled.

### IIR, FIR filters and resampling now support multiarchitecture

## Name changes

| KFR5                            | KFR6                        |
|---------------------------------|-----------------------------|
| `biquad_params<T>`              | → `biquad_section<T>`       |
| `biquad_filter<T>`              | → `iir_filter<T>`           |
| `biquad(params, expression)`    | → `iir(expression, params)` |
| `expression_biquad`             | → `expression_iir`          |
| `biquad_blocks<T, N>`           | → `iir_params<T, N>`        |
| `std::vector<biquad_params<T>>` | → `iir_params<T>`           |
| `fir_taps<T>`                   | → `fir_params<T>`           |

The old names works in KFR6 but are marked deprecated.

## CMake config files

During install CMake config files are now generated.

```cmake
# CMAKE_PREFIX_PATH must contain path-to-kfr-install-dir/lib/cmake
find_package(KFR CONFIG REQUIRED)
target_link_libraries(main PRIVATE kfr kfr_dsp)
```

# Linking

It is important to link KFR libraries correctly when using the multiarchitecture feature.

The following methods automatically ensure that linking is correct:

* `add_subdirectory`.
* Installing KFR and using `find_package(KFR CONFIG)`

If you use different method please follow [these instructions](without_cmake.md).

## Other changes

### `kfr::complex` type removed in favor of using `std::complex`

The `KFR_STD_COMPLEX` CMake option and `KFR_STD_COMPLEX` macro are removed.

### `KFR_DFT_NO_NPo2` macro is no more used. Non-power of 2 DFT is always enabled

The `KFR_ENABLE_DFT_NP` CMake option is removed too.

### `tensor<>::reshape_may_copy` and `tensor<>::flatten_may_copy` change parameters default values

In KFR6 default behavior is to allow copying (as name suggests);
```c++
auto t = mytensor.reshape_may_copy(new_shape);
```

## Requirements

### Clang minimum version is now 11

Please update your build environment if you're using Clang 10 or older.

### CMake minimum version is now 3.12

Please update your build environment if you're using CMake 3.11 or older.

## KFR Internals

### DFT, DSP and IO sources have been moved to `src/` directory

### `expression_make_function` must be used for expression function prototypes

Before KFR6
```c++
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::sin, E1> sin(E1&& x);
```
KFR6
```c++
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sin, E1> sin(E1&& x);
```
