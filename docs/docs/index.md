# KFR

## Features

* All code in the library is optimized for Intel, AMD (SSE2, SSE3, SSE4.x, AVX and AVX2 and AVX512) and ARM (NEON) processors
* Mathematical and statistical functions
* Template expressions (See examples)
* All data types are supported including complex numbers
* All vector lengths are also supported. `vec<float,1>`, `vec<unsigned,3>`, `vec<complex<float>, 11>` all are valid vector types in KFR
* Most of the standard library functions are re-implemented to support vector of any length and data type and to support expressions 
* Runtime cpu detection
* C API: DFT, real DFT, DCT, FIR and IIR filters and convolution, memory allocation
  * Built for SSE2, SSE4.1, AVX, AVX2, AVX512, x86 and x86_64, architecture is selected at runtime
  * Can be used with any compiler and any language with ability to call C functions
  * [Prebuilt Windows binaries](https://github.com/kfrlib/kfr/releases)

### DSP/Audio algorithms

* [FFT](dft.md)
* DCT
* [Convolution](convolution.md)
* [FIR filtering](fir.md)
* [FIR filter design using the window method](fir.md)
* [Resampling with configurable quality](src.md) (See resampling.cpp from Examples directory)
* IIR filter design
  * Butterworth
  * Chevyshev I and Chevyshev II
  * Bessel
  * Low pass, high pass, band pass, band stop
  * Convert arbitrary filter from Z, P, K format to SOS suitable for biquad function
* Goertzel algorithm
* Fractional delay
* [Biquad filtering](bq.md)
* [Biquad design functions](bq.md)
* Oscillators: Sine, Square, Sawtooth, Triangle
* Window functions: Triangular, Bartlett, Cosine, Hann, Bartlett-Hann, Hamming, Bohman, Blackman, Blackman-Harris, Kaiser, Flattop, Gaussian, Lanczos, Rectangular
* [Audio file reading/writing (wav, flac, mp3)](read_audio.md)
* Pseudorandom number generator
* Sorting
* Color space conversion (sRGB, XYZ, Lab, LCH)
* Ring (Circular) buffer
* Simple waveshaper
* Fast incremental sine/cosine generation
* [EBU R 128](ebur128.md)

## Installation

[GitHub:kfrlib/kfr/blob/master/README.md](https://github.com/kfrlib/kfr/blob/master/README.md#usage)

## Compiler support

Xcode | Visual Studio | Clang | GCC | Intel Compiler
----- | ------------- | ----- | --- | --------------
9+    | 2017          | 6+    | 7+  | Experimental

Tested on macOS, Windows (MinGW, MSYS and MSVC), Linux, iOS, Android.

## Architecture support

x86, x86_64 | ARM, ARM64 |
----- | -------------
Scalar, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, AVX, AVX2, AVX512  | Scalar, NEON, NEON64
