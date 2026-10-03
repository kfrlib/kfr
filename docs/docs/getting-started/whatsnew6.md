# What's new in KFR 6

* DFT performance has been improved up to 40% (backported to KFR 5.2.0 branch)
* C API for non x86 architectures
* DSP refactoring with easier initialization
* Multiarchitecture for resampling, FIR and IIR filters
* `matrix_transpose`: optimized matrix transpose (square/non-square, inplace/out-of-place, real/complex, scalar/vectors)
* CMake config file generation (`find_package(KFR CONFIG)` support, see [installation](installation.md))
* `.npy` format support (reading/writing, v1/v2, c/fortran order, real/complex, bigendian/littleendian)
* Multidimensional DFT: real/complex
* `inline_vector`
* Windows arm64 support
* Emscripten (wasm/wasm64) support

### Other changes

* CMake minimum version is 3.12
* Multidimensional reference DFT
* Easier cross compilation to ARM64 on x86_64 macOS
* Automated tests using GitHub Actions (previously Azure Pipelines)
* GCC 7 and 8: emulate missing avx-512 instrinsics
* `read_group` and `write_group`
* [❗breaking change] `reshape_may_copy` and `flatten_may_copy` in `tensor<>` allows copying by default
* `shape<>::transpose` function
* `tensor<>::transpose` function
* `convert_endianess`
* DFT, DSP and IO sources have been moved to `src/` directory
* Multiarchitecture is enabled by default
* `KFR_DFT_NO_NPo2` has been removed (assumed always enabled)
* Tests refactoring
* Some tests moved to `tests/internal/`
* [❗breaking change] Scalars are now passed by value in expressions (this fixes dangling references in some cases)
* Expression functions should return `expression_make_function` instead of `expression_function`
* `KFR_WITH_CLANG`
* `KFR_VERSION` CMake variable
* Functions to get module versions (`library_version_dft`, `library_version_dsp` etc)
* Exceptions are no longer enforced in MSVC
* `kfr::complex` removed (use `std::complex` instead). `KFR_STD_COMPLEX` cmake variable removed too
* `strides_for_shape` for fortran order
* AARCH and ARM emulation refactoring (dynamic libraries are now supported)
* `call_with_temp`
* `maximum_dims` is now 16 (was 8)
* `to_fmt`/`from_fmt` supports inplace
* `shape` refactoring: `rotate_left`, `rotate_right`, `remove_back`, `remove_front`
* temp argument can be `nullptr` for DFT (temporary buffer will be allocated on stack or heap)
* `dft_plan` and similar classes have now default and move constructors
* `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` is required for building C API
* `ci/run.sh` can now build in a directory outside source tree
* [❗breaking change]`graphics/color.hpp` and `graphics/geometry.hpp` have been removed
* Simpler `CMT_CVAL` macro
* `/Zc:lambda` is now required for building KFR in MSVC
* `println` for `string_view`
* MSVC internal compiler error fixed
* Complex vector operators fixed


For KFR5 changelog see [What's new in KFR 5](whatsnew5.md)
