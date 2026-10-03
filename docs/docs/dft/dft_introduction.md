# DFT and FFT in KFR

The [Discrete Fourier Transform (DFT)](../advanced/dsp_glossary.md#dft-vs-fft)
turns a signal into its frequency-domain representation. Its fast form, the
FFT, is one of the foundations of audio processing, spectral analysis, image
processing, and many numerical algorithms. KFR provides FFTs for both everyday
signal-processing code and performance-sensitive applications that need direct
control over memory and execution.

KFR's FFT implementation is competitive with the fastest available
General-purpose FFT libraries. It uses hand-tuned SIMD kernels and
size-specific implementations where they matter, without requiring a runtime
measurement phase to choose a configuration. The same API supports single- and
double-precision data, forward and inverse transforms, in-place processing,
and real or complex signals.

This page introduces the DFT facilities available in KFR and points to the
right guide for each task.

## Choosing an FFT interface

Most applications should start with [Fast Fourier Transform with KFR](dft.md).
It covers the general [[`dft_plan<T>`:nosig]] API and the convenience
[[`dft(const univector<complex<T>, Tag> &)`:nosig]] functions. This is the
flexible choice: it accepts arbitrary positive transform lengths, including
lengths that are not powers of two, and selects an appropriate mixed-radix,
four-step, or Bluestein algorithm internally.

For a fixed power-of-two length where memory must be controlled by the caller,
use the [low-level ngFFT API](ngfft.md). Its [[`ngfft_plan<T>`:nosig]] holds no
owned storage, takes caller-provided twiddle factors, and needs no scratch
buffer while executing. That smaller execution path can be advantageous in
real-time and embedded workloads.

Both APIs support complex-to-complex transforms. They also provide real
transforms, which take advantage of the symmetry in the spectrum of a real
signal to reduce the amount of storage and computation. Neither API applies
inverse scaling automatically: a forward transform followed by an inverse
transform returns the original signal multiplied by its length $N$.

## From transforms to DSP algorithms

An FFT is often an intermediate step rather than the final result. The DFT
module provides several algorithms that build on it:

- [DFT data layout](dft_layout.md) describes the CCS and Perm packed-spectrum
  formats used by real transforms.
- [FFT-based sample rate conversion](dft_resampler.md) applies a frequency-domain
  low-pass filter with overlap-save processing. It is useful for power-of-two
  resampling ratios and long filters.
- [Convolution filter details](convolution.md) explains KFR's partitioned
  overlap-add convolution for long FIR filters and streaming signals.
- [Convolution reverb](conv_reverb.md) shows how to apply that convolution
  machinery to an impulse response.

These techniques are useful beyond audio: convolution and correlation,
spectrum visualization, frequency-domain filtering, and multidimensional
signal analysis all rely on the same transform building block.

## Performance and portability

KFR maps FFT work onto its explicit SIMD layer instead of depending solely on
a compiler's auto-vectorizer. Optimized paths cover x86 and x86-64 processors
with SSE through AVX-512, ARM and AArch64 processors with NEON, and RISC-V
processors with the RVV vector extension. The library also has scalar-capable
fallbacks where SIMD is unavailable.

When multiarchitecture support is enabled, one binary can include optimized
paths for several CPUs and dispatch to the best available implementation at
runtime. Plans can be reused across calls, avoiding repeated setup work for a
fixed transform size.

## Where to start

1. Read [Fast Fourier Transform with KFR](dft.md) for complex and real 1D
   transforms, plans, caching, and multidimensional DFTs.
2. Read [DFT data layout](dft_layout.md) before inspecting or modifying a
   packed real spectrum.
3. Choose the [low-level ngFFT API](ngfft.md) when a power-of-two transform
   needs caller-owned storage and no execution scratch buffer.
4. Continue with [FFT-based sample rate conversion](dft_resampler.md),
   [Convolution filter details](convolution.md), or [Convolution reverb](conv_reverb.md)
   when the transform is part of a larger DSP operation.
