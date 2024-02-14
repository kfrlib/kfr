# More about FFT/DFT

The Fast Fourier Transform (FFT) can be used to perform:

* [Convolution (including convolution reverberation)](convolution.md)
* Cross-correlation and auto-correlation
* [Applying large FIR filters](fir.md)
* [Sample rate conversion](src.md)
* Spectrum visualization
* Large integer multiplication
* Wavelet transform
* and many other algorithms

Often, FFT is the most efficient way to perform each of these algorithms.

## About KFR DFT implementation

The KFR implementation of the FFT:

* is fully optimized for X86, X86-64, ARM and AARCH64 (ARM64) processors
* uses vector intrinsics (if available for the cpu)
* supports both single- and double-precision
* supports in-place processing
* supports multidimensional FFT
* can cache internal data between calls to speed up plan creation
* can do forward and inverse FFT without the need to create two plans
* can be used for complex-to-complex, real-to-complex and complex-to-real transforms
* doesn’t require measuring FFT performance at runtime to find an optimal configuration
* has special implementations for FFT sizes up to 1024
* has no external dependencies
* is thread-safe, with no global data
* is written in modern C++17
* is open source (GPL v2+ license, commercial license is availalble for closed source projects, see https://kfr.dev )
