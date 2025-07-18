# Changelog

## 6.3.0

#### Added

- Progressive DFT execution

#### Changed

- Matrix transpose performance has been improved by 10-30%
- Optimize scalar to expression assignment for msvc

## 6.2.0

#### Added

- Android x86 and x86_64 builds
- Linux arm and aarch64 (arm64) builds

#### Changed

- Documentation update

#### Fixed

- Minor fixes to `small_buffer` and `npy.hpp`.

## 6.1.1

#### Changed

- DFT performance has been improved by 30-80% for ARM and ARM64 cpus
- DFT benchmark is now enabled for non-x86 builds

## 6.1.0

#### Added

- C API now supports Multidimensional DFT

#### Changed

- Documentation update
- Update to latest CxxDox
- `Dims` template parameter in `dft_plan_md` and `dft_plan_md_real` now defaults to `dynamic_shape`

## 6.0.4

#### Fixed

- Clang 19 compatibility fix

## 6.0.3

#### Changed

- Added more Doxygen documentation for filter functions (@Jalmenara)
- Examples now include detailed comments

#### Fixed

- Fixed stateless parameter
- Fixed build with non-MSVC on Win32 (@jcelerier)
- Resolved Android building issues
- Removed deprecated atomic initialization

## 6.0.2

#### Added

- Windows arm64 support
- Emscripten (wasm/wasm64) support

#### Changed

- `complex_size` now takes dft_pack_format as parameter

## 6.0.0

- DFT performance has been improved up to 40% (backported to KFR 5.2.0 branch)
- C API for non x86 architectures
- DSP refactoring with easier initialization
- Multiarchitecture for resampling, FIR and IIR filters
- `matrix_transpose`: optimized matrix transpose (square/non-square, inplace/out-of-place, real/complex, scalar/vectors)
- CMake config file generation (`find_package(KFR CONFIG)` support, see [installation](installation.md))
- `.npy` format support (reading/writing, v1/v2, c/fortran order, real/complex, bigendian/littleendian)
- Multidimensional DFT: real/complex
- `inline_vector`

#### Other changes

- CMake minimum version is 3.12
- Multidimensional reference DFT
- Easier cross compilation to ARM64 on x86_64 macOS
- Automated tests using GitHub Actions (previously Azure Pipelines)
- GCC 7 and 8: emulate missing avx-512 instrinsics
- `read_group` and `write_group`
- [❗breaking change] `reshape_may_copy` and `flatten_may_copy` in `tensor<>` allows copying by default
- `shape<>::transpose` function
- `tensor<>::transpose` function
- `convert_endianess`
- DFT, DSP and IO sources have been moved to `src/` directory
- Multiarchitecture is enabled by default
- `KFR_DFT_NO_NPo2` has been removed (assumed always enabled)
- Tests refactoring
- Some tests moved to `tests/internal/`
- [❗breaking change] Scalars are now passed by value in expressions (this fixes dangling references in some cases)
- Expression functions should return `expression_make_function` instead of `expression_function`
- `KFR_WITH_CLANG`
- `KFR_VERSION` CMake variable
- Functions to get module versions (`library_version_dft`, `library_version_dsp` etc)
- Exceptions are no longer enforced in MSVC
- `kfr::complex` removed (use `std::complex` instead). `KFR_STD_COMPLEX` cmake variable removed too
- `strides_for_shape` for fortran order
- AARCH and ARM emulation refactoring (dynamic libraries are now supported)
- `call_with_temp`
- `maximum_dims` is now 16 (was 8)
- `to_fmt`/`from_fmt` supports inplace
- `shape` refactoring: `rotate_left`, `rotate_right`, `remove_back`, `remove_front`
- temp argument can be `nullptr` for DFT (temporary buffer will be allocated on stack or heap)
- `dft_plan` and similar classes have now default and move constructors
- `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` is required for building C API
- `ci/run.sh` can now build in a directory outside source tree
- [❗breaking change]`graphics/color.hpp` and `graphics/geometry.hpp` have been removed
- Simpler `CMT_CVAL` macro
- `/Zc:lambda` is now required for building KFR in MSVC
- `println` for `string_view`
- MSVC internal compiler error fixed
- Complex vector operators fixed

## 5.2.0

2023-11-27

#### Changed

- The performance of DFT has been increased up to 40% compared to KFR 5 on x86 and x86_64
in single and double precision, inplace and out of place processing.

#### Added

- `KFR_NO_PERF_TESTS` define can now disable performance tests
- `CMT_CVAL` for extracting constexpr-enabled value from `cval_t`
- `fft_algorithm_selection` to select FFT algorithm for given FFT size.

#### Fixed

- Warnings in Clang 10 https://github.com/kfrlib/kfr/issues/198
- `std::is_pov` is peprecated in C++20 https://github.com/kfrlib/kfr/issues/190
- DFT sizes 0 and 1 were not processed correctly https://github.com/kfrlib/kfr/issues/195
- Internal compiler error in Visual Studio Compiler 19.37 https://github.com/kfrlib/kfr/issues/194
- Goertzel issue https://github.com/kfrlib/kfr/issues/121
- Bug in `nearest_real_or_complex` https://github.com/kfrlib/kfr/issues/137
- Fixed operators in `KFR_STD_COMPLEX` mode
- Testo library: typo in `epsilon_scope`
- Fix ambiguities with std::identity (c++20)
- Force linking in correct order for multi-architecture binaries

## 5.1.0

2023-10-11

#### Added

- Tukey window function
- Subscript operator for `vec<>`
- Unary operators for `vec<>`

#### Fixed

- Inverse DCT has been fixed
- Allow C API to be built on non-x86 systems
- Tensor iteration range has been fixed
- `transpose(vec<vec<>>)` has been fixed
- `vec<bit<>>` bug on GCC and MSVC
- Internal constant `is_pod` has been removed
- `root` and `cbrt` have been fixed for negative values
- Fixed numerous warnings in MSVC

## 5.0.3

2023-06-26

#### Added

- Planck-taper window function

#### Fixed

- Sample rate conversion: automatic zero padding 
- DFT: incorrect result of real dft when input size != 4N https://github.com/kfrlib/kfr/pull/141
- Add options for installing libraries and headers https://github.com/kfrlib/kfr/pull/182

## 5.0.2

2023-01-25

#### Performance

- ARM/ARM64 performance has been improved up to 2 times in various usage scenarios, including DFT.

#### Fixed

- Fix sine and other functions not accepting scalar references
- Fix possible name conflict with fmt

## 5.0.1

2022-12-06

#### Changed

- Documentation updates

#### Fixed

- MSVC fixes
- Ambiguous assign operator fixes

## 5.0.0

2022-11-30

#### Added

- New `tensor<T, dims>` class for multidimensional data (like numpy's nparray)
- Histogram computation
- Normal (gaussian) distribution for random number generator
- All builtin expressions support multiple dimensions
- Exception support (may be configured to call user-supplied function or std::abort)
- [changes required] CMake variables now have `KFR_` prefix
- Template parameter deduction for `vec`, so `vec{1, 2}` is the same as `vec<int, 2>{1, 2}`
- [changes required] `random_state` is now architecture-agnostic and defined in `kfr` namespace
- All expression classes have been moved from `kfr::CMT_ARCH_NAME::internal` to `kfr::CMT_ARCH_NAME` namespace
- `expression_traits<T>` introduced to support interpreting any object as kfr expression
- [changes required] User-defined expressions should be rewritten to be used in KFR5
- Out-of-class assign operators for all input & output expressions
- `round.hpp`, `clamp.hpp`, `select.hpp`, `sort.hpp`, `saturation.hpp`, `min_max.hpp`, `logical.hpp`, `abs.hpp` headers have been moved to `simd` module
- `state_holder.hpp` has been moved to `base` module
- All code related to expressions have been moved to `base` module
- `vec<T, N>::front()` and `vec<T, N>::front()` are now writable
- `set_elements` functions for output expressions like `get_elements` for input expressions

#### Changed

- Documentation updates

## 4.3.1

2022-11-23

#### Fixed

- C++20 compatibility fixes

## 4.3.0

2022-10-14

#### Changed
* Compile times improved and memory usage reduced for MSVC and GCC
* `cxxdox` version updated
* Tests for latest Clang, Azure Pipelines images are updated
* .editorconfig file

#### Fixed
* Fixed incompatibility with latest GCC
* Fixed various Internal Compiler Error in latest MSVC2019
* Fixed tests for Clang 14
* Fixed bugs in gather/scatter and read/write functions

## 4.2.1

2021-04-29

#### Fixed

- AVX512 intrinsics in MSVC and GCC
- `carg`
- Python 3.7 compatibility

#### Changed

- Global constants are now inline

## 4.2.0

2020-02-17

#### Added

- `ENABLE_DFT_MULTIARCH` cmake option can be used to build `kfr_dft` with multiple architectures support (x86/x86_64 only)
- `config.h` is generated during install step with all `#define`s needed for correct usage of installed libraries

#### Changed

- `CMAKE_INSTALL_PREFIX` is reset to empty on Win32 (can be overriden in cmake command line)
- C API binary is now installed using install command (`make install`, `ninja install` or `cmake --build . --target install`)

## 4.1.0

2020-03-04

#### Added

- MSVC2019 support (16.4.4 is required)
- Stateless short_fir expression (thanks to https://github.com/slarew)
- `biquad_filter` constructor taking `std::vector<biquad_params<T>>` (thanks to https://github.com/BenjaminNavarro)
- `cabssqr` function (thanks to https://github.com/slarew)
- `moving_sum` expression (thanks to https://github.com/slarew)
- `gen_expj` generator (thanks to https://github.com/slarew)

#### Changed
- `cdirect_t{}` is now allowed in real dft plan methods for compatibility
- complex support for `convolve_filter` (thanks to https://github.com/slarew)

#### Fixed

- GCC debug build (thanks to https://github.com/BenjaminNavarro)
- `is_invocable_r_impl` fallback for missing C++17 feature
- `std::complex` compatibility (thanks to https://github.com/slarew)
- Various CI fixes

## 4.0.0

2019-12-05

#### Added

- IIR filter design
  - Butterworth
  - Chebyshev type I and II
  - Bessel
  - Lowpass, highpass, bandpass and bandstop filters
  - Conversion of arbitrary filter from Z,P,K to SOS format (suitable for biquad function and filter)
- Discrete Cosine Transform type II and III
- cmake uninstall target
- C API: 
  - DFT
  - real DFT
  - DCT
  - FIR
  - IIR
  - Convolution
  - Aligned memory allocation
  - Built for SSE2, SSE4.1, AVX, AVX2, AVX512, x86 and x86_64, architecture is selected at runtime
- New vector based types: 
  - color
  - rectangle
  - point
  - size
  - border
  - geometric vector
  - 2D matrix
- Color space conversion:
  - sRGB
  - linear RGB
  - XYZ
  - Lab
  - LCH
  - HSV
- MP3 audio reading
- `aligned_reallocate` function
- `zip` and `column` functions
- `vec<vec<vec<T>>>` support
- New example: `iir.cpp`
- `biquad` that gets `std::vector` of `biquad_param`s
- `csqr` function
- `factorial` function
- `isreal` function
- `make_complex` function for expressions
- New optimization technique for vector element shuffle
- `std::get<>` support for `vec<>`
- C++17 structured bindings support for `vec<>`
- comparison operators for `cvals_t`
- compile time `cminof`, `cmaxof` functions
- `vector_width_for` constant to get vector width for specific architecture
- make_univector for containers and arrays
- univector: copy constructor optimization
- Custom `assertion_failed`

#### Changed

- C++17 compiler is required. Some of C++17 library features may be missing, in this case KFR uses custom implementation
- `cast` function now works with both vectors and expressions (expression versions was previously called `convert`)
- Memory alignment can be up to 32768
- New implementation for `cometa::function` using shared pointer
- `memory.hpp` allocation functions has been moved to cometa
- `cconj` has been moved to simd module (instead of `math`)
- dr_libs version updated
- All global constants are now `inline constexpr`
- enums moved out of architecture namespace
- `special_constant::undefined` removed

#### Fixed

- Fixed real DFT with raw poitners
- Bug with generators 
- Fixed grid issues in dspplot
- Wrong index in `concatenate`
- `csqrt` function
- Fixed NaNs in `amp_to_dB`
- GCC 9 support
- Workaround for Clang 8.0 FMA code generator bug
- Flat top window
- SSE4.2 detection (previously detected as SSE4.1)

#### Notes

* MSVC support is limited to MSVC2017 due to ICE in MSVC2019. Once fixed, support will be added
* DFT is limited to Clang due to ICE in MSVC and broken AVX optimization in GCC 8 and 9. Once fixed, support will be added

## 3.0.9

2019-04-02

#### Added

- `reduce` supports different types and containers other than `univector`
- Assignment operators for `univector`: `+=`, `*=` etc
- `concatenate` function to concatenate two expressions sequentially
- Audio file IO: `read_channels`/`write_channels` to read channels data directly without interleaving/deinterleaving
- `as_string`: support for `std::vector`

#### Changed

- `expression_scalar`: support for `vec<T>`

#### Fixed

- CPU detection in cmake subdirectory
- MSVC 2017 32-bit intrinsics

## 3.0.8

2019-03-15

#### Added

- Ability to pass `random_bit_generator` by reference
- Tests for iOS ARM and ARM64

#### Changed

- `kfr::complex` is placed in `kfr` namespace

#### Fixed

## 3.0.7

2019-03-13

#### Added

- Detected CPU is now saved to CMake cache
- Linux/AArch64 tests have been added to CI

#### Changed

- `mask<>` is now a specialization of `vec<>`. This allows using many `vec` functions for masks
- `short_fir` performance has been increased by around 50%-60%

#### Fixed

- Fixed the bug with infinitely loading in Intellisense

## 3.0.6

2019-03-07

#### Added

- Android arm & arm64 tests have been added to CI

#### Fixed

- Fixed Android support (thanks to https://github.com/Jman420)
- Fixed building cpu detection tool in CMake subdirectory (thanks to https://github.com/Jman420)

## 3.0.5

2019-02-21

#### Added

- DFT speeds have been improved by up to 15% on most modern cpus
- Support for MSVC 2017
- Support for GCC 7.3
- Support for GCC 8.2
- Support for resampling complex vectors (Thanks to https://github.com/ermito)
- Tests for various math functions no longer depend on MPFR

#### Changed

- Testo now allocates much less memory during long tests (x3 less than previously)

#### Fixed

- Building generators (Thanks to https://github.com/ermito)

## 3.0.4

2019-01-08

#### Added

#### Changed

- `KFR_READCYCLECOUNTER` may be redefined to point to any function returning (pseudo-)random value
- Ability to disable random number initialization functions

#### Fixed

## 3.0.3

2018-12-27

#### Added

- Partial compatibility for Visual Studio 2017
- Support for `KFR_USE_STD_ALLOCATION`
- `univector` support for `abstract_reader`/`abstract_writer`

#### Changed

#### Fixed

- Paths in `CMakeLists.txt`

## 3.0.2

2018-12-19

#### Added

- More documentation
- dspplot: `freqticks` parameter
- dspplot: `freq_lim` parameter
- dspplot: `freq_dB_lim` parameter
- dspplot: `phasearg` parameter
- More tests for Sample Rate Converter

#### Changed

- Sample Rate Converter: now computes the transition width of the filter. This makes cutoff frequency more precise
- Sample Rate Converter: now uses Kaiser window with different &beta; parameter for different quality. This leads to a better balance between transition width and sidelobes attenuation.
- Sample Rate Converter: `quality` parameter is now passed as runtime parameter rather than compile time parameter
- `fir.cpp` example has been extended to include examples of applying FIR filters

#### Fixed

- `amp_to_dB` function
- 24-bit audio reading

## 3.0.1

2018-11-28

#### Added

- WAV file reading/writing
- FLAC file reading
- Audio sample conversion
- Interleaving/deinterleaving
- sample_rate_converter example
- Sample Rate Converter: `process`() function

#### Changed

- Assignment to an empty vector resizes it
- New documentation
- KFR IO is now built separately (only needed for audio file support)

#### Fixed

- Resampler bug: sequential calls to `resampler::operator()` may fail

### 3.0.0

#### Added

- Optimized non-power of two DFT implementation
- Full AVX-512 support
- EBU R128
- Ability to include KFR as a subdirectory in cmake project
- Number of automatic tests has been increased
- C API for DFT
- Partial GCC 8.x support
- Ability to make sized linspace
- `fraction` type

#### Changed

- GPL version changed from 3 to 2+
- Refactoring of DFT code
- KFR DFT is now built separately
- FIR filter now supports different tap and sample types (thanks to @fbbdev)

#### Fixed

- Various fixes for KFR functions
- FFT with size=128 on architectures with SSE only
- Small kfr::complex type fixes
