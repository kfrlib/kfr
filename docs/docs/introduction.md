# Introduction to KFR

Welcome to KFR, a modern C++ framework for digital signal processing, audio
work, and scientific computing. This article is the recommended starting point
for new users. It explains what KFR is, the design principles behind it, the
problems it solves, and how to begin using it in your own projects.

After reading this page you should be comfortable with the core ideas of the
library and ready to dive into the more specialized guides referenced below.

## What is KFR?

KFR is a header-and-library C++ framework that provides high-performance
building blocks for signal processing. It covers a wide range of functionality,
including:

* Fast Fourier Transforms and Discrete Cosine Transforms of arbitrary size
* [IIR](dsp_glossary.md#iir-infinite-impulse-response), [FIR](dsp_glossary.md#fir-finite-impulse-response) and [biquad](dsp_glossary.md#biquad-sos) filter design and application
* [Sample-rate conversion](src.md) with configurable quality and linear phase
* [Convolution](dsp_glossary.md#convolution) and [convolution reverb](dsp_glossary.md#convolution-reverb)
* Audio file reading and writing for many common formats
* Mathematical functions, statistics, random number generation and tensors
* A [SIMD](dsp_glossary.md#simd-single-instruction-multiple-data) abstraction layer that scales from scalar code up to AVX-512 and NEON

Everything is written in modern C++20 and is built with CMake. KFR has no
external runtime dependencies beyond a C++20-compatible standard library,
which makes it easy to drop into existing projects. See
[Installation](installation.md) for build instructions and
[Basics](basics.md) for the conventions used throughout the library.

## Proven in production

The **KFR library** is a high-performance computing framework used across
commercial, academic, and engineering industries. Its practical applications
span brain-computer interfaces, medical equipment, digital music, satellite
communications, industrial control systems, and robotics.

Because the software scales efficiently, it is deployed on hardware ranging
from microcomputers and smartphones to supercomputers, operating across
terrestrial, aerial, and aerospace environments. KFR's user base ranges from
independent developers to major scientific collaborations and Fortune 500
companies.

KFR is extensively tested on all supported architectures and has more than
10 years of history of continuous development and real-world deployment.

## Design goals

KFR is built around two ideas that are usually in tension: **performance** and
**flexibility**. Most signal-processing code is either fast but rigid, or
expressive but slow. KFR tries to give you both.

### Performance first

Every algorithm in KFR is implemented on top of an explicit [SIMD](dsp_glossary.md#simd-single-instruction-multiple-data) layer. Rather
than relying on the compiler's auto-vectorizer, KFR uses a `vec<T, N>` type that
abstracts CPU-specific intrinsics and is specialized for SSE, AVX, AVX-512 and
NEON. Data containers such as `univector` allocate memory aligned to 64-byte
boundaries so that wide SIMD loads are always possible. Hot paths are
hand-tuned, and the [DFT](dsp_glossary.md#dft-vs-fft) implementation in particular is competitive with the
fastest general-purpose [FFT](dsp_glossary.md#dft-vs-fft) implementations available.

For cases where a single binary must run on many different CPUs, KFR provides a
[multiarchitecture](dsp_glossary.md#multiarchitecture-dispatch) mode that compiles several code paths and dispatches to the
best one at runtime. This is available for DFT, resampling, and FIR/IIR
filtering.

### Flexible and extensible

Performance never forces you into a fixed API. KFR is built on the
[Expression](expressions.md) concept: operations on arrays return lazy
[expression templates](dsp_glossary.md#expression-templates) that are only evaluated when their results are actually
needed. This lets you compose algorithms in a natural, readable way while the
library fuses operations together and vectorizes them as a whole.

The same `vec` type works for any element type and any width. `vec<float, 1>`,
`vec<unsigned, 3>` and `vec<complex<float>, 11>` are all valid, which means you
can write generic code once and let KFR pick the most efficient representation
for the target hardware.

## What can KFR be used for?

Because KFR covers the full chain from raw numerics to audio file I/O, it is a
good fit for a wide variety of workloads:

* **Audio processing** — design filters, apply them to streaming audio, convert
  between sample rates, measure loudness to [EBU R128](dsp_glossary.md#ebu-r128), and read or write WAV,
  FLAC, AIFF, ALAC, MP3 and other formats. See
  [How to read or write an audio file](read_audio.md) and
  [How to apply a FIR filter](fir.md).
* **Scientific computing** — perform [FFT](dsp_glossary.md#dft-vs-fft)-based spectral analysis, [convolve](dsp_glossary.md#convolution) large
  signals, work with multidimensional data through the `tensor` type, and read
  or write `.npy` files for interop with the Python ecosystem.
* **Communications and measurement** — generate [oscillators](dsp_glossary.md#oscillator), apply [Goertzel](dsp_glossary.md#goertzel-algorithm)
  detection, design elliptic or Chebyshev filters with tight tolerances, and
  process real-time data through [ring buffers](dsp_glossary.md#ring-buffer).
* **Embedded and cross-platform DSP** — build the same code for x86, ARM,
  AArch64 and RISC-V, with runtime dispatch picking the best implementation per
  device.
* **Education and prototyping** — the expression-based API makes it easy to
  write short, math-like programs that still run at full speed.

## A first taste

The canonical KFR "hello world" is a forward FFT. With the high-level helper
functions, the whole transform is a single call:

```c++
#include <kfr/base.hpp>
#include <kfr/dft.hpp>

using namespace kfr;

int main()
{
    // 256 complex samples of a complex exponential
    univector<complex<double>, 256> data =
        cexp(linspace(0, c_pi<double, 2>, 256) * make_complex(0, 1));

    // Forward FFT — returns a univector holding the spectrum
    univector<complex<double>, 256> freq = dft(data);

    // Inverse FFT back to the time domain
    data = idft(freq) / data.size(); // KFR does not scale automatically
}
```

A few things worth noticing in this snippet:

* `univector` is KFR's main 1D container. With a size template argument it
  stores data inline like `std::array`; without one it allocates on the heap
  like `std::vector` but with SIMD-friendly alignment. See
  [Basics](basics.md) for the full picture.
* KFR does not apply the conventional `1/N` scaling on the inverse transform,
  so we divide explicitly when we need to round-trip the data.

## Filters and signal processing

Filter design in KFR reads almost like a textbook. You pick an approximation
([Butterworth, Chebyshev I/II, elliptic or Bessel](dsp_glossary.md#analog-iir-filters-vs-digital-filters)), choose a band type, and
apply it to a signal:

```c++
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>

using namespace kfr;

int main() {
    // 8th-order elliptic [lowpass](dsp_glossary.md#filter-band-types) at 1 kHz, sampled at 48 kHz
    // rp = 0.1 dB passband ripple, rs = 40 dB stopband attenuation
    // (mirrors scipy.signal.ellip's N, rp, rs arguments)
    [zpk](dsp_glossary.md#zero-pole-gain-zpk) filt = iir_lowpass(elliptic(8, 0.1, 40.0), 1000, 48000);

    // Apply it to an impulse to obtain the impulse response
    univector<fbase, 1024> response = iir(unitimpulse(), filt);
}
```

The same `iir` function accepts [biquad](dsp_glossary.md#biquad-sos) cascades, so you can mix designs freely.
For details and gallery examples, see [How to apply a Biquad filter](bq.md),
[IIR filters](iir.md) and [FIR filters](fir.md).

Sample-rate conversion is just as concise. The resampler supports several
quality levels and exposes its group delay so that you can align the output
with the input:

```c++
auto r = resampler<fbase>(resample_quality::high, 44100, 96000);
univector<fbase> out(input.size() * 44100 / 96000 + r.get_delay());
r.process(out, input);
```

See [Sample rate conversion](src.md) for the full guide.

## A closer look at FFT

The Fast Fourier Transform is one of the most-used algorithms in DSP, and it is
an area where KFR spends a lot of effort. The implementation has a few
properties that are worth highlighting for new users.

### Arbitrary sizes

KFR is not limited to powers of two. The `dft_plan` accepts any positive size
and internally selects the best mix of mixed-radix, four-step and Bluestein's
algorithms. This means you can transform a 1000-point or 12345-point signal
without padding or windowing workarounds.

### Real and complex transforms

Both complex-to-complex and real-to-complex transforms are available. Real
transforms pack the output using either [CCS or Perm format](dsp_glossary.md#dft-real-data-layout), which roughly
halves both memory and computation. The format is documented in
[DFT data layout](dft_format.md).

### Plans and caching

Computing a transform requires twiddle factors and other precomputed data. KFR
exposes this as a `dft_plan<T>` object that you create once and reuse. Plans are
immutable after construction, so they can be shared freely between threads
without locking:

```c++
const dft_plan<double> plan(1024);
univector<u8> temp(plan.temp_size);   // scratch buffer

univector<complex<double>, 1024> in, out;
// ... fill in ...
plan.execute(out, in, temp);          // forward transform
plan.execute(out, in, temp, true);    // inverse transform
```

For convenience, the high-level `dft`, `idft`, `realdft` and `irealdft`
functions automatically cache plans in a thread-safe `dft_cache`, so you get
the benefit of plan reuse even in short scripts.

For more control, KFR also exposes a low-level FFT interface through
`dft_plan::execute`. This path performs no
internal allocations, has minimal overhead, and is the one to reach for when
you need predictable memory behavior or want to reuse buffers across many
transforms. See [How to apply Fast Fourier Transform](dft.md) and
[More about FFT/DFT](dft2.md) for the complete API.

### Beyond the basic transform

Because the FFT is fast, it enables a family of higher-level operations that
KFR exposes directly:

* **[Convolution](dsp_glossary.md#convolution)** of long signals via overlap-add, used internally by the
  convolution filter. See [Convolution filter details](convolution.md).
* **[Convolution reverb](dsp_glossary.md#convolution-reverb)** for applying impulse responses to audio. See
  [How to apply Convolution Reverb](conv_reverb.md).
* **Discrete Cosine Transform** ([DCT](dsp_glossary.md#discrete-cosine-transform-dct)-II and its inverse DCT-III), useful for
  compression and spectral analysis.
* **Multidimensional DFT** for image and tensor processing.

## Where to go next

The rest of the documentation is organized as task-oriented guides. Good
follow-up reads after this introduction are:

1. [Installation](installation.md) — set up KFR in your project.
2. [Basics](basics.md) — `univector`, `vec` and the conventions used everywhere.
3. [Expressions](expressions.md) — the lazy evaluation model that powers KFR.
4. [How to apply Fast Fourier Transform](dft.md) — the FFT guide.
5. [How to apply a FIR filter](fir.md) and [How to apply a Biquad filter](bq.md)
   — filter design and application.

The full list of guides is available on the [index](index.md) page, and the
generated function reference covers every public symbol in the library.
