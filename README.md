# KFR - Fast, modern C++ DSP framework

<p align="center">
  <img width="300" height="auto" src="img/KFR1.png">
</p>

![Build](https://img.shields.io/github/actions/workflow/status/kfrlib/kfr/build.yml?style=flat-square&label=Build)
![Test](https://img.shields.io/github/actions/workflow/status/kfrlib/kfr/test.yml?style=flat-square&label=Test)

![License](https://img.shields.io/github/license/kfrlib/kfr.svg?style=flat-square&label=License)
![Release](https://img.shields.io/github/release-date/kfrlib/kfr?style=flat-square&label=Latest+release)

https://www.kfrlib.com

KFR is an open source C++ DSP framework that contains high performance building blocks for DSP, audio, scientific and other applications. It is distributed under dual GPLv2/v3 and [commercial license](https://kfrlib.com/purchase).

:star2: **New**: Explore benchmark results from the LIGO, Virgo, and KAGRA collaborations, comparing KFR performance against FFTW for signal processing in gravitational-wave research: https://ar5iv.labs.arxiv.org/html/2503.14292

## KFR 7

KFR 7 is currently in development.

What is already available in the `main` branch:

* Elliptic filter design
* Zero-Phase IIR Filter (`filtfilt`)
* Audio encoding/decoding with support for:
    * Wave (WAV)
    * W64
    * RF64/BW64
    * AIFF
    * FLAC
    * Apple CAF
    * ALAC
    * MP3 (decoding only)
    * Raw PCM
    * decoding AAC and other formats using MediaFoundation (Windows only)   
* Basic RISC-V support
    * Only CPUs with VLEN >= 128 are supported
    * Linux only, clang required
* C++20
* New optimized functions
* Performance improvements
* Broader GCC support
* Tests are migrated to Catch2 framework
* [More to come...](https://github.com/kfrlib/kfr/issues/256)

## Our other projects

<div align="center">

[**🟣 Brisk**](https://github.com/brisklib/brisk) is a cross-platform C++20 GUI framework featuring MVVM architecture, reactive capabilities, and scalable, accelerated GPU rendering. *(GPL/Commercial)*

[**🟢 CxxDox**](https://github.com/kfrlib/cxxdox) — C++ documentation generator. *(MIT)*

</div>

## KFR Installation

Compiler support:

![Clang 11+](https://img.shields.io/badge/Clang-11%2B-brightgreen.svg?style=flat-square)
![GCC 7+](https://img.shields.io/badge/GCC-7%2B-brightgreen.svg?style=flat-square)
![MSVC 2019](https://img.shields.io/badge/MSVC-2019%2B-brightgreen.svg?style=flat-square)
![Xcode 12+](https://img.shields.io/badge/Xcode-12%2B-brightgreen.svg?style=flat-square)

KFR has no external dependencies except for a C++20-compatible standard C++ library. CMake is used as the build system.

Clang is highly recommended and proven to provide the best performance for KFR. You can use Clang as a drop-in replacement for both GCC on Linux and MSVC on Windows. On macOS, Clang is the default compiler and included in the official Xcode toolchain.

_Note_: ARM/AArch64 support and building the DFT module currently requires Clang due to internal compiler errors and a lack of some optimizations in GCC and MSVC.

:arrow_right: See [Installation](docs/docs/installation.md) docs for more details

## Features

:star2: — new in KFR6

### FFT/DFT
* Optimized DFT implementation for any size (non-power of two sizes are supported)
* DFT performance is on par with the most performant implementation currently available [See Benchmarks](#benchmark-results)
* Real forward and inverse DFT
* :star2: Multidimensional complex and real DFT
* Discrete Cosine Transform type II (and its inverse, also called DCT type III)
* Convolution using FFT
* Convolution filter

:arrow_right: See also [How to apply FFT](docs/docs/dft.md) with KFR

### DSP

* IIR filter design
  * Elliptic
  * Butterworth
  * Chebyshev type I and II
  * Bessel
  * Lowpass, highpass, bandpass and bandstop filters
  * Conversion of arbitrary filter from {Z, P, K} to SOS format (suitable for biquad function and filter)
* Biquad filter [See Benchmarks](#benchmark-results)
* Simple biquad filter design
* FIR filter design using window method
* Loudness measurement according to EBU R128
* Window functions: Triangular, Bartlett, Cosine, Hann, Bartlett-Hann, Hamming, Bohman, Blackman, Blackman-Harris, Kaiser, Flattop, Gaussian, Lanczos, Rectangular
* Sample rate conversion with configurable quality and linear phase
* Oscillators, fast incremental sine/cosine generation,  Goertzel algorithm, fractional delay

### Base

* Tensors (multidimensional arrays)
* :star2: .npy support (reading/writing)
* :star2: Matrix transpose
* Statistical functions
* Random number generation
* Template expressions (See examples)
* Ring (Circular) buffer
* :star2: Windows arm64 support
* :star2: Emscripten (wasm/wasm64) support

### Math

* Mathematical functions such as `sin`, `log` and `cosh` built on top of SIMD primitives
* Most of the standard library functions are re-implemented to support vector of any length and data type

### SIMD

* `vec<T, N>` class and related functions that abstracts cpu-specific intrinsics
* All code in the library is optimized for Intel, AMD (SSE2, SSE3, SSE4.x, AVX and AVX2 and AVX512) and ARM, AArch64 (NEON) processors
* All data types are supported including complex numbers
* All vector lengths are also supported. `vec<float,1>`, `vec<unsigned,3>`, `vec<complex<float>, 11>` all are valid vector types in KFR
* Sorting

### IO

* Audio file reading/writing
    * Wave (WAV)
    * W64
    * RF64/BW64
    * AIFF
    * FLAC
    * Apple CAF
    * ALAC
    * MP3 (decoding only)
    * Raw PCM
    * decoding AAC and other formats using MediaFoundation (Windows only)

### Multiarchitecture

The multiarchitecture mode enables building algorithms for multiple architectures with runtime dispatch to detect the CPU of the target machine and select the best code path

* :star2: Multiarchitecture for DFT, resampling, FIR and IIR filters.

### C API

C API is available and includes a subset of KFR features including FFT and filter processing.

* :star2: Support for non x86 systems.

## Benchmark results

### DFT

Powers of 2, from 16 to 16777216 (*Higher is better*)

![FFT Performance](img/powers2_double_16_16777216.png)

Prime numbers from 17 to 127 (*Higher is better*)

![DFT Performance](img/primes_double_17_127.png)

Small numbers from 18 to 119 (*Higher is better*)

![DFT Performance](img/composite_double_18_119.png)

Random sizes from 120 to 30720000 (*Higher is better*)

![DFT Performance](img/extra_double_120_30720000.png)

See [fft benchmark](https://github.com/kfrlib/fft-benchmark) for details about benchmarking process.


### Biquad

(*Higher is better*)

![Biquad Performance](img/biquad.svg)

## Documentation

KFR 6 (latest)
https://www.kfrlib.com/docs/latest/

KFR 5
https://www.kfrlib.com/docs/v5/

Built with
* [cxxdox - generates markdown docs from C++](https://github.com/kfrlib/cxxdox)
* [mkdocs - static documentation generator](https://www.mkdocs.org/)
* [mkdocs-material - material theme for mkdocs](https://squidfunk.github.io/mkdocs-material/)

## Branches

`dev` - current development version. Pull requests should go to `dev`.

`main` - current stable version passing all compiler/architecture tests.

`v5` - previous version of KFR (no new features, fixes only)

## License

KFR is dual-licensed, available under both commercial and open-source GPL 2+ license.

If you want to use KFR in a commercial product or a closed-source project, you need to [purchase a Commercial License](https://kfrlib.com/purchase-license).
