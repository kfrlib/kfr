# More about FFT/DFT

Fast Fourier Transform (FFT) can be used to perform:

* [Convolution (including convolution reverberation)](convolution.md)
* Cross-correlation and auto-correlation
* [Applying large FIR filters](fir.md)
* [Sample rate conversion](src.md)
* Spectrum visualization
* Large integer multiplication
* Wavelet transform
* and many other algorithms

Often FFT is the most efficient way to perform each of these algorithms.


## About KFR DFT implementation

KFR implementation of the FFT:

* is fully optimized for X86, X86-64, ARM and AARCH64 processors
* uses vector intrinsics (if available for cpu)
* supports both single- and double precision
* can cache internal data between calls to speed up plan creation
* can do forward and inverse FFT without a need to create two plans
* can be used for complex-to-complex, real-to-complex and complex-to-real 1D transforms
* doesn’t require measure FFT performance at runtime and to find an optimal configuration
* has special implementations for FFT sizes up to 256
* has no external dependencies
* is thread-safe, no global data
* is written in modern C++14
* is open source (GPL v2+ license)

