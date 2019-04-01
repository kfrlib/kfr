# Changelog

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
