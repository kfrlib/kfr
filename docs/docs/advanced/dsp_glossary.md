# DSP Glossary

This glossary explains the signal-processing terms that appear throughout the
KFR codebase and documentation. It is intended as a quick reference, not a
textbook — for in-depth treatment, consult a dedicated DSP source.

## Filters and filter design

### IIR (Infinite Impulse Response)

A class of digital filters whose output depends on both current and past inputs
*and* past outputs (feedback). IIR filters are computationally cheap and can
achieve steep rolloffs with few coefficients, but they generally have non-linear
phase and can become unstable if coefficients are poorly chosen. In KFR, IIR
filters are built from an approximation (Butterworth, Chebyshev I/II, elliptic,
Bessel) and a band type, and applied through the `iir` function. See
[IIR filters](../dsp/iir.md).

### FIR (Finite Impulse Response)

A class of digital filters whose output depends only on current and past inputs
(no feedback). FIR filters are always stable and can be designed with exactly
linear phase, at the cost of more coefficients than an equivalent IIR filter.
KFR applies FIR filters through the `fir` function and supports FFT-based
overlap-add convolution for long kernels. See [FIR filters](../dsp/fir.md).

### Analog IIR filters vs Digital filters

"Analog" IIR filters refer to the classical continuous-time prototypes
(Butterworth, Chebyshev, elliptic, Bessel) defined on the Laplace *s*-plane.
KFR designs these prototypes as zero-pole-gain (`zpk`) objects and then maps
them to the discrete *z*-domain through a bilinear transform so they can run on
sampled data. The term "digital filter" covers the resulting discrete-time
filters that operate on sample sequences.

### Zero-Pole-Gain (ZPK)

A representation of a filter as a set of zeros `z`, poles `p`, and a gain `k`.
It is the natural form produced by classical analog approximations and is what
KFR's `zpk` type holds. For an analog prototype on the Laplace *s*-plane the
transfer function is

$$H(s) = k \cdot \frac{\prod_i (s - q_i)}{\prod_i (s - p_i)},$$

where $q_i$ are the zeros and $p_i$ the poles. After the bilinear transform maps
the prototype to the discrete *z*-domain, the same factored form holds in $z$
with the transformed zeros and poles.

ZPK is convenient for design and inspection; for application it is typically
converted to a biquad cascade.

### Biquad (SOS)

A second-order IIR section — the smallest useful IIR building block, described
by six coefficients: three feedforward ($b_0, b_1, b_2$) and three feedback
($a_0, a_1, a_2$), of which five are independent once $a_0$ is normalized to 1:

$$H(z) = \frac{b_0 + b_1 z^{-1} + b_2 z^{-2}}{a_0 + a_1 z^{-1} + a_2 z^{-2}}.$$

A "biquad cascade" or
"SOS" (second-order sections) is a chain of biquads that implements a
higher-order filter in a numerically stable way. KFR accepts biquad cascades
directly through the same `iir` function used for `zpk` designs. See
[Biquad filters](../dsp/bq.md).

### Filter band types

* **Lowpass** — passes frequencies below a cutoff, attenuates those above.
* **Highpass** — passes frequencies above a cutoff, attenuates those below.
* **Bandpass** — passes a range of frequencies between two cutoffs.
* **Bandstop / band-reject** — attenuates a range of frequencies between two
  cutoffs.
* **Notch** — a narrow bandstop filter targeting a single frequency (e.g. 50/60
  Hz mains hum). Often implemented as a special-case biquad.
* **Allpass** — passes all magnitudes unchanged but shifts phase; used for
  phase equalization and delay compensation.

### Frequency conventions in filter design

KFR's filter design APIs accept two different frequency conventions:

* **IIR design** (`iir_lowpass`, `iir_highpass`, `iir_bandpass`,
  `iir_bandstop`) takes an explicit sampling frequency `fs` whose default is
  `2.0`. If `fs` is omitted, the cutoff frequency is interpreted as
  **normalized to Nyquist**, so the valid range is `0..1` (0 = DC, 1 =
  Nyquist). Pass a real `fs` in Hz to specify cutoffs directly in Hz.
* **FIR design** always uses frequency **normalized to the sampling
  frequency** (cycles per sample), so the valid range is `0..0.5` (0 = DC,
  0.5 = Nyquist). There is no `fs` parameter.

## Frequency

### Frequency (Hz)

The number of cycles of a periodic signal per unit time, measured in **hertz**
(Hz = cycles per second). For a sinusoid $x(t) = \sin(2\pi f t)$, $f$ is the
ordinary frequency in Hz. In sampled systems the highest representable
frequency is the **Nyquist frequency** $f_s/2$, where $f_s$ is the sampling
rate in samples per second.

### Normalized frequency

A dimensionless ratio of a signal frequency to a reference frequency (usually
the sampling rate $f_s$). Normalized frequency lets DSP math be written without
committing to a specific sample rate. The two common conventions are:

* **Cycles per sample** — $f' = f / f_s$, ranging over $[0, \tfrac{1}{2}]$ for
  real signals (Nyquist at $\tfrac{1}{2}$). Some toolboxes instead normalize by
  $f_s/2$, giving a range of $[0, 1]$ in *half-cycles per sample*.
* **Frequency bins** — $f \cdot N / f_s$, used when sampling the spectrum at
  $N$ points; the Nyquist bin sits at index $N/2$.

For example, with $f = 1\text{ kHz}$ and $f_s = 44100\text{ Hz}$, the
cycles-per-sample value is $1000/44100 \approx 0.02268$.

### Angular frequency

The rate of phase rotation, denoted $\omega$ and measured in **radians per
second** (rad/s). It relates to ordinary frequency by $\omega = 2\pi f$. In
discrete-time systems it is normalized to **radians per sample** as
$\omega' = \omega / f_s = 2\pi f / f_s$, ranging over $[0, \pi]$ for real
signals with the Nyquist frequency at $\pi$. This is the form that appears in
the DFT kernel $e^{-j\omega' n}$ and in KFR's filter and oscillator APIs.

## Sample rate conversion

### Polyphase sample rate conversion

A technique for resampling signals efficiently by splitting a FIR filter into
multiple phases (sub-filters), one per output sample phase. Each output sample
is produced by convolving the input with the phase matching its fractional
position. This avoids recomputing the full kernel for every sample and is the
method used by KFR's `samplerate_converter`. See [Sample rate conversion](../dsp/src.md).

### Window-sinc method

A FIR filter design method where the ideal (sinc) impulse response of a
brick-wall filter is truncated and shaped by a window function. It produces
linear-phase filters with predictable tradeoffs between transition width and
stopband attenuation. KFR uses this approach for some of its resampler quality
levels.

## Convolution

### Convolution

The operation $y[n] = \sum_k x[k] \cdot h[n-k]$ that combines an input signal
with a filter's impulse response. Direct convolution is $O(N \cdot M)$; for
long kernels KFR uses FFT-based overlap-add, which reduces the cost to roughly
$O(N \log M)$ — the signal is processed in blocks on the order of the kernel
length $M$, at $O(M \log M)$ per block, which also keeps latency bounded and
makes streaming possible. See [Convolution filter details](../dft/convolution.md) and
[How to apply Convolution Reverb](../dft/conv_reverb.md).

### Convolution reverb

An audio effect that convolves an input signal with a recorded impulse
response (e.g. of a concert hall), simulating the acoustics of that space.
Because impulse responses are typically thousands of samples long, this is
implemented via FFT-based fast convolution with partitioned overlap-add.

## Transforms

### DFT vs FFT

The **Discrete Fourier Transform** (DFT) is the mathematical operation that
maps a length-$N$ sequence to its frequency representation:

$$X[k] = \sum_{n=0}^{N-1} x[n] \, e^{-j 2\pi k n / N}.$$

The **Fast Fourier Transform** (FFT) is any algorithm that computes the DFT in
$O(N \log N)$ time instead of the naive $O(N^2)$. Because $O(N \log N)$
algorithms exist for *every* transform length — mixed-radix for composite sizes,
Rader's or Bluestein's algorithm otherwise — a DFT is in practice always
computed by a fast algorithm, and the two terms are used interchangeably
throughout KFR's documentation and API (`dft_plan`, `dft`, `idft`). KFR supports
arbitrary sizes via mixed-radix, four-step, and Bluestein's algorithms — not just
powers of two.

### DFT real data layout

Real-to-complex transforms pack the output into roughly half the memory of a
full complex spectrum, exploiting the Hermitian symmetry of real inputs. KFR
supports two packing formats (see [DFT data layout](../dft/dft_layout.md)):

* **CCS (Complex Conjugate-Symmetric)** — stores $N/2 + 1$ complex values. The
  DC and Nyquist bins land at indices 0 and $N/2$ and their imaginary parts are
  identically zero, since Hermitian symmetry $X[N-k] = X[k]^*$ forces those two
  bins to be real. The remaining slots hold the positive-frequency complex
  values.
* **Perm (Permutation)** — stores the Nyquist bin's real part in the imaginary
  slot of index 0, so the packed output is exactly $N/2$ complex samples with no
  wasted slots.

### Discrete Cosine Transform (DCT)

A transform related to the DFT that operates on real, often even-symmetric data.
KFR implements DCT-II (the form used in audio/image compression) and its
inverse DCT-III.

## Audio I/O and measurement

### Bit depth

The number of bits used to represent each sample in a PCM audio file. Higher bit
depth means more quantization levels and lower quantization noise. KFR's audio
readers and writers support common depths (8-, 16-, 24-, 32-bit integer, and
32/64-bit float) and convert transparently to/from the floating-point
representation used internally.

### SNR (Signal-to-Noise Ratio)

The ratio between the power of a signal and the power of background noise,
usually expressed in decibels: $\text{SNR} = 10 \log_{10}(P_\text{signal} /
P_\text{noise})$. In audio it quantifies the dynamic range a format or
processing chain can carry; for example, 16-bit PCM has a theoretical SNR of
about 96 dB.

### Latency

The delay introduced by a processing block between input and output. In KFR,
latency comes from IIR group delay, the pre-ringing and group delay of
linear-phase FIR filters and resampler kernels, FFT block framing, and
overlap-add convolution. The resampler
exposes its delay via `get_delay()` so outputs can be time-aligned with inputs.

### EBU R128

A standard from the European Broadcasting Union for measuring loudness of audio
programmes in LUFS (Loudness Units Full Scale). It defines integrated,
short-term, and momentary loudness and a target level of −23 LUFS for broadcast.
KFR provides an EBU R128 meter; see the EBU example.

## Windows and analysis

### Window function

A function multiplied point-wise with a block of samples before an FFT to
reduce spectral leakage caused by the block's finite extent. Common windows
include Hann, Hamming, Blackman, and Kaiser, each trading main-lobe width
against side-lobe level. KFR provides a range of windows; see the
[window example](../dsp/examples/window_gallery.md).

### Goertzel algorithm

An efficient way to evaluate a single DFT bin (a single frequency) without
computing the full transform. It is useful for tone detection — for example,
DTMF decoding — when only one or a few frequencies are of interest. KFR exposes
this through its Goertzel detection API.

### Oscillator

A generator that produces a periodic signal — typically a sine, cosine, or
complex exponential — at a specified frequency and sample rate. KFR's
`sinewave`, `cosinewave`, and `cexp` generators are used for test signals,
modulation, and as building blocks in synthesis.

### Ring buffer

A circular buffer used to feed samples into a streaming filter or out to a
consumer that uses a different block size or runs on a different thread or
timing schedule. It decouples producer and consumer timing, but does not
reconcile differing sample rates — that requires resampling, since a sustained
rate mismatch would eventually overflow or starve the buffer. KFR provides ring
buffers with lock-free
single-producer / single-consumer semantics for real-time audio and
communications paths.

## SIMD and performance

### SIMD (Single Instruction, Multiple Data)

A CPU feature that applies one instruction to several data elements in parallel.
KFR abstracts SSE, AVX, AVX-512, and NEON behind its `vec<T, N>` type so that
generic code is vectorized without intrinsics. See [Basics](../getting-started/basics.md).

### Multiarchitecture dispatch

A build mode where several SIMD code paths are compiled into one binary and
the best one is selected at runtime based on CPU features. KFR uses this for
DFT, resampling, and FIR/IIR filtering so a single binary can run optimally
across diverse hardware.

### Expression templates

KFR's lazy-evaluation model: operations on `univector` return expression
objects that are only evaluated when their results are consumed, allowing the
library to fuse and vectorize chains of operations as a whole. See
[Expressions](../expressions/expressions.md).
