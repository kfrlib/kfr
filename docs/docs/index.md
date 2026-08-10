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
* [IIR](advanced/dsp_glossary.md#iir-infinite-impulse-response), [FIR](advanced/dsp_glossary.md#fir-finite-impulse-response) and [biquad](advanced/dsp_glossary.md#biquad-sos) filter design and application
* [Sample-rate conversion](dsp/src.md) with configurable quality and linear phase
* [Convolution](advanced/dsp_glossary.md#convolution) and [convolution reverb](advanced/dsp_glossary.md#convolution-reverb)
* Audio file reading and writing for many common formats
* Mathematical functions, statistics, random number generation and tensors
* A [SIMD](advanced/dsp_glossary.md#simd-single-instruction-multiple-data) abstraction layer that scales from scalar code up to AVX-512 and NEON

Everything is written in modern C++20 and is built with CMake. KFR has no
external runtime dependencies beyond a C++20-compatible standard library,
which makes it easy to drop into existing projects. See
[Installation](getting-started/installation.md) for build instructions and
[Basics](getting-started/basics.md) for the conventions used throughout the library.

## Proven in production

KFR is used in production across commercial, academic, and engineering
projects, including brain-computer interfaces, medical devices, digital music
software, satellite communications, industrial control systems, and robotics.
It runs on hardware from microcontrollers and smartphones to supercomputers.
A list of known commercial and open-source users is maintained at
[kfr.dev](https://www.kfr.dev/).

KFR is tested on all supported architectures and has more than 10 years of
continuous development and real-world use.

## Design goals

KFR is built around two ideas that are usually in tension: **performance** and
**flexibility**. Most signal-processing code is either fast but rigid, or
expressive but slow. KFR tries to give you both.

### Performance first

Every algorithm in KFR is implemented on top of an explicit [SIMD](advanced/dsp_glossary.md#simd-single-instruction-multiple-data) layer. Rather
than relying on the compiler's auto-vectorizer, KFR uses a
[[`vec<T, N>`:nosig]] type that
abstracts CPU-specific intrinsics and is specialized for SSE, AVX, AVX-512 and
NEON. Data containers such as [[`univector<T, Size>`:nosig]] allocate memory aligned to 64-byte
boundaries so that wide SIMD loads are always possible. Hot paths are
hand-tuned, and the [DFT](advanced/dsp_glossary.md#dft-vs-fft) implementation in particular is competitive with the
fastest general-purpose [FFT](advanced/dsp_glossary.md#dft-vs-fft) implementations available.

For cases where a single binary must run on many different CPUs, KFR provides a
[multiarchitecture](advanced/dsp_glossary.md#multiarchitecture-dispatch) mode that compiles several code paths and dispatches to the
best one at runtime. This is available for DFT, resampling, and FIR/IIR
filtering.

### Flexible and extensible

Performance never forces you into a fixed API. KFR is built on the
[Expression](expressions/expressions.md) concept: operations on arrays return lazy
[expression templates](advanced/dsp_glossary.md#expression-templates) that are only evaluated when their results are actually
needed. This lets you compose algorithms in a natural, readable way while the
library fuses operations together and vectorizes them as a whole.

The same [[`vec<T, N>`:nosig]] type works for any element type and any width. `vec<float, 1>`,
`vec<unsigned, 3>` and `vec<complex<float>, 11>` are all valid, which means you
can write generic code once and let KFR pick the most efficient representation
for the target hardware.

## What can KFR be used for?

Because KFR covers the full chain from raw numerics to audio file I/O, it is a
good fit for a wide variety of workloads:

* **Audio processing** — design filters, apply them to streaming audio, convert
	between sample rates, measure loudness to [EBU R128](advanced/dsp_glossary.md#ebu-r128), and read or write WAV,
	FLAC, AIFF, ALAC, MP3 and other formats. See
	[How to read or write an audio file](audio_io/read_audio.md) and
	[How to apply a FIR filter](dsp/fir.md).
* **Scientific computing** — perform [FFT](advanced/dsp_glossary.md#dft-vs-fft)-based spectral analysis, [convolve](advanced/dsp_glossary.md#convolution) large
	signals, work with multidimensional data through the [[`tensor<T, NDims>`:nosig]] type, and read
	or write `.npy` files for interop with the Python ecosystem.
* **Communications and measurement** — generate [oscillators](advanced/dsp_glossary.md#oscillator), apply [Goertzel](advanced/dsp_glossary.md#goertzel-algorithm)
	detection, design elliptic or Chebyshev filters with tight tolerances, and
	process real-time data through [ring buffers](advanced/dsp_glossary.md#ring-buffer).
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

int main()|||TEST_CASE("index.md/FFT example")
{
		// 256 complex samples of a complex exponential
		univector<complex<double>, 256> data =
				cexp(linspace(0, c_pi<double, 2>, 256) * make_complex(0, 1));

|||		univector<complex<double>, 256> original = data;

		// Forward FFT — returns a univector holding the spectrum
		univector<complex<double>, 256> freq = dft(data);

		// Inverse FFT back to the time domain
		data = idft(freq) / data.size(); // KFR does not scale automatically
|||		for (size_t i = 0; i < data.size(); ++i)
|||			CHECK(cabs(data[i] - original[i]) < 1e-9);
}
```

A few things worth noticing in this snippet:

* [[`univector<T, Size>`:nosig]] is KFR's main 1D container. With a size template argument it
	stores data inline like `std::array`; without one it allocates on the heap
	like `std::vector` but with SIMD-friendly alignment. See
	[Basics](getting-started/basics.md) for the full picture.
* KFR does not apply the conventional `1/N` scaling on the inverse transform,
	so we divide explicitly when we need to round-trip the data.

## Filters and signal processing

Filter design in KFR reads almost like a textbook. You pick an approximation
([Butterworth, Chebyshev I/II, elliptic or Bessel](advanced/dsp_glossary.md#analog-iir-filters-vs-digital-filters)), choose a band type, and
apply it to a signal:

```c++
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>

using namespace kfr;

int main()|||TEST_CASE("index.md/IIR example")
{
    // 8th-order elliptic lowpass at 1 kHz, sampled at 48 kHz
    // rp = 0.1 dB passband ripple, rs = 40 dB stopband attenuation
    // (mirrors scipy.signal.ellip's N, rp, rs arguments)
    zpk filt = iir_lowpass(elliptic(8, 0.1, 40.0), 1000, 48000);

    // Apply it to an impulse to obtain the impulse response
    univector<fbase, 1024> response = iir(unitimpulse(), filt);
|||    CHECK(std::isfinite(response[0]));
|||    // Lowpass impulse response should have decayed to near zero by the end
|||    CHECK(std::abs(response[response.size() - 1]) < 0.01);
}
```

The same [[`iir(E1 &&, const zpk &)`:nosig]] function accepts [biquad](advanced/dsp_glossary.md#biquad-sos) cascades, so you can mix designs freely.
For details and gallery examples, see [How to apply a Biquad filter](dsp/bq.md),
[IIR filters](dsp/iir.md) and [FIR filters](dsp/fir.md).

Sample-rate conversion is just as concise. The [[`resampler`:nosig]] supports several
quality levels and exposes its group delay so that you can align the output
with the input:

```c++
|||TEST_CASE("index.md/SRC example") {
|||univector<fbase, 96000> input = unitimpulse();
auto r = resampler<fbase>(resample_quality::high, 44100, 96000);
univector<fbase> out(input.size() * 44100 / 96000 + r.get_delay());
r.process(out, input);
|||CHECK(out.size() == input.size() * 44100 / 96000 + r.get_delay());
|||bool any_nonzero = false;
|||for (size_t i = 0; i < out.size(); ++i)
|||    if (std::abs(out[i]) > 1e-6) any_nonzero = true;
|||CHECK(any_nonzero);
||| }
```

See [Sample rate conversion](dsp/src.md) for the full guide.

## A closer look at FFT

The Fast Fourier Transform is one of the most-used algorithms in DSP, and it is
an area where KFR spends a lot of effort. The implementation has a few
properties that are worth highlighting for new users.

### Arbitrary sizes

KFR is not limited to powers of two. The [[`dft_plan<T>`:nosig]] accepts any positive size
and internally selects the best mix of mixed-radix, four-step and Bluestein's
algorithms. This means you can transform a 1000-point or 12345-point signal
without padding or windowing workarounds.

### Real and complex transforms

Both complex-to-complex and real-to-complex transforms are available. Real
transforms pack the output using either [CCS or Perm format](advanced/dsp_glossary.md#dft-real-data-layout), which roughly
halves both memory and computation. The format is documented in
[DFT data layout](dft/dft_layout.md).

### Plans and caching

Computing a transform requires twiddle factors and other precomputed data. KFR
exposes this as a [[`dft_plan<T>`:nosig]] object that you create once and reuse. Plans are
immutable after construction, so they can be shared freely between threads
without locking:

```c++
|||TEST_CASE("index.md/DFT plan example") {
const dft_plan<double> plan(1024);
univector<u8> temp(plan.temp_size);   // scratch buffer

univector<complex<double>, 1024> in, out;
// ... fill in ...
|||in = cexp(linspace(0, c_pi<double, 2>, 1024) * make_complex(0, 1));
|||univector<complex<double>, 1024> original = in;
plan.execute(out, in, temp);          // forward transform
plan.execute(in, out, temp, true);    // inverse transform
|||for (size_t i = 0; i < in.size(); ++i)
|||    CHECK(cabs(in[i] / 1024.0 - original[i]) < 1e-9);
|||}
```

For convenience, the high-level [[`dft(const univector<complex<T>, Tag> &)`:nosig]],
[[`idft(const univector<complex<T>, Tag> &)`:nosig]],
[[`realdft(const univector<T, Tag> &)`:nosig]], and
[[`irealdft(const univector<complex<T>, Tag> &)`:nosig]] functions
automatically cache plans in a thread-safe [[`dft_cache`:nosig]], so you get
the benefit of plan reuse even in short scripts.

See [DFT and FFT in KFR](dft/dft_introduction.md) and
[Fast Fourier Transform with KFR](dft/dft.md) for the complete API.

### Beyond the basic transform

Because the FFT is fast, it enables a family of higher-level operations that
KFR exposes directly:

* **[Convolution](advanced/dsp_glossary.md#convolution)** of long signals via overlap-add, used internally by the
	convolution filter. See [Convolution filter details](dft/convolution.md).
* **[Convolution reverb](advanced/dsp_glossary.md#convolution-reverb)** for applying impulse responses to audio. See
	[How to apply Convolution Reverb](dft/conv_reverb.md).
* **Discrete Cosine Transform** ([DCT](advanced/dsp_glossary.md#discrete-cosine-transform-dct)-II and its inverse DCT-III), useful for
	compression and spectral analysis.
* **Multidimensional DFT** for image and tensor processing.

## Where to go next

The rest of the documentation is organized as task-oriented guides. Good
follow-up reads after this introduction are:

1. [Installation](getting-started/installation.md) — set up KFR in your project.
2. [Basics](getting-started/basics.md) — [[`univector<T, Size>`:nosig]], [[`vec<T, N>`:nosig]], and the conventions used everywhere.
3. [Expressions](expressions/expressions.md) — the lazy evaluation model that powers KFR.
4. [How to apply Fast Fourier Transform](dft/dft.md) — the FFT guide.
5. [How to apply a FIR filter](dsp/fir.md) and [How to apply a Biquad filter](dsp/bq.md)
	 — filter design and application.

The full list of guides is available in the site navigation, and the generated
function reference covers every public symbol in the library.
