# KFR

## Features

* All code in the library is optimized for Intel, AMD (SSE2, SSE3, SSE4.x, AVX and AVX2 and AVX512) and ARM (NEON) processors
* Mathematical and statistical functions
* Template expressions (See examples)
* All data types are supported including complex numbers
* All vector lengths are also supported. `vec<float,1>`, `vec<unsigned,3>`, `vec<complex<float>, 11>` all are valid vector types in KFR
* Most of the standard library functions are re-implemented to support vector of any length and data type and to support expressions 
* Runtime cpu detection

### DSP/Audio algorithms

* [FFT](dft.md)
* [Convolution](convolution.md)
* [FIR filtering](fir.md)
* [FIR filter design using the window method](fir.md)
* [Resampling with configurable quality](src.md) (See resampling.cpp from Examples directory)
* Goertzel algorithm
* Fractional delay
* [Biquad filtering](bq.md)
* [Biquad design functions](bq.md)
* Oscillators: Sine, Square, Sawtooth, Triangle
* Window functions: Triangular, Bartlett, Cosine, Hann, Bartlett-Hann, Hamming, Bohman, Blackman, Blackman-Harris, Kaiser, Flattop, Gaussian, Lanczos, Rectangular
* [Audio file reading/writing](read_audio.md)
* Pseudorandom number generator
* Sorting
* Ring (Circular) buffer
* Simple waveshaper
* Fast incremental sine/cosine generation
* [EBU R 128](ebur128.md)

## Installation

[GitHub:kfrlib/kfr/blob/master/README.md](https://github.com/kfrlib/kfr/blob/master/README.md#usage)

## Compiler support

Xcode | Visual Studio | Clang | GCC | Intel Compiler
----- | ------------- | ----- | --- | --------------
8.3+  | 2017          | 4+    | 7+  | Experimental

Tested on macOS, Windows (MinGW, MSYS and MSVC), Linux, iOS, Android.

## Architecture support

x86, x86_64 | ARM, ARM64 |
----- | -------------
Scalar, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX512  | Scalar, NEON, NEON64
