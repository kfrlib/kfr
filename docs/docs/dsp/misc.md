# Audio Utilities: Peak Normalization, Stereo Mixing, and Sample Conversion

This article collects small DSP and audio-buffer operations that commonly sit around a processing chain: peak normalization, stereo and mid/side transforms, sample-format conversion, and planar/interleaved layout conversion. These operations are independent of the filters and transforms described elsewhere in the DSP documentation.

## Peak normalization

[[`absmaxof(const E1 &)`:nosig]] returns the largest sample magnitude in a finite expression:

$$
\operatorname{peak}(x) = \max_i |x_i|.
$$

Dividing by that value gives sample-peak normalization to a linear peak of $1.0$ (0 dBFS for conventional normalized floating-point audio):

```c++
|||#include <kfr/dsp.hpp>
|||using namespace kfr;
|||TEST_CASE("dsp/misc.md: normalize a peak")
|||{
|||univector<float> audio{ -0.25f, 0.5f, -1.0f };
const float peak = absmaxof(audio);
if (peak > 0.0f)
	audio /= peak;
|||CHECK(absmaxof(audio) == 1.0f);
|||}
```

Guard the operation. Reducing an empty expression is invalid, and dividing silent audio by a zero peak produces invalid floating-point values. This is peak normalization, not RMS or loudness normalization; it does not make signals equally loud.

To normalize to another peak, multiply the result or include the target in the scale factor:

```c++
|||TEST_CASE("dsp/misc.md: normalize to a target peak")
|||{
|||univector<float> audio{ -0.25f, 0.5f, -1.0f };
constexpr float target_peak = 0.8f;
const float peak = absmaxof(audio);
if (peak > 0.0f)
	audio *= target_peak / peak;
|||CHECK(absmaxof(audio) == target_peak);
|||}
```

### Linked stereo normalization

Use one shared peak across channels to preserve their relative level and stereo balance. [[`concatenate(Arg1 &&, Arg2 &&)`:nosig]] creates a lazy sequence over both channels without copying them:

```c++
|||TEST_CASE("dsp/misc.md: normalize linked stereo")
|||{
|||univector<float> left{ -0.5f, 0.25f };
|||univector<float> right{ 0.75f, -0.125f };
const float peak = absmaxof(concatenate(left, right));
if (peak > 0.0f)
{
	left /= peak;
	right /= peak;
}
|||CHECK(absmaxof(concatenate(left, right)) == 1.0f);
|||}
```

Normalizing the channels independently can change a deliberately asymmetric stereo image. As with a single channel, validate or sanitize NaN and infinity values before applying a normalization policy if input data may contain them.

## Stereo mixing and mid/side conversion

[[`mixdown_stereo(Left &&, Right &&, const f64x2x2 &)`:nosig]] returns a lazy expression of paired output samples. A $2\times2$ coefficient matrix maps left and right inputs to two output channels:

$$
\begin{aligned}
y_0 &= m_{00}L + m_{01}R, \\
y_1 &= m_{10}L + m_{11}R.
\end{aligned}
$$

Use [[`unpack`:nosig]] to write those two output components to channel buffers.

### L/R to M/S

[[`matrix_halfsum_halfdiff()`:nosig]] implements the conventional level-preserving mid/side encoding:

$$
M = \frac{L+R}{2}, \qquad S = \frac{L-R}{2}.
$$

```c++
|||TEST_CASE("dsp/misc.md: encode left-right as mid-side")
|||{
|||univector<float> left{ 0.8f, 0.4f };
|||univector<float> right{ 0.2f, -0.4f };
|||univector<float> mid(left.size());
|||univector<float> side(left.size());
unpack(mid, side) =
	mixdown_stereo(left, right, matrix_halfsum_halfdiff());
|||CHECK(mid[0] == 0.5f);
|||CHECK(side[1] == 0.4f);
|||}
```

### M/S to L/R

[[`matrix_sum_diff()`:nosig]] computes its first output as the sum and its second as the difference. Applied to mid/side inputs, it reconstructs left/right:

$$
L = M + S, \qquad R = M - S.
$$

```c++
|||TEST_CASE("dsp/misc.md: decode mid-side as left-right")
|||{
|||univector<float> mid{ 0.5f, 0.0f };
|||univector<float> side{ 0.3f, 0.4f };
|||univector<float> left(mid.size());
|||univector<float> right(mid.size());
unpack(left, right) =
	mixdown_stereo(mid, side, matrix_sum_diff());
|||CHECK(left[0] == 0.8f);
|||CHECK(right[1] == -0.4f);
|||}
```

`matrix_sum_diff()` applied to left/right inputs instead produces $L+R$ and $L-R$; it is not a safe, level-preserving downmix matrix. Keep headroom when using unscaled sums. The half-sum/half-difference and sum/difference pair is reversible for floating-point buffers; integer buffers can lose precision in the half-scaled step.

### Stereo to mono and custom matrices

For an averaged mono downmix, which preserves the level of identical left and right channels, use:

```c++
|||TEST_CASE("dsp/misc.md: average a stereo downmix")
|||{
|||univector<float> left{ 0.5f, -0.5f };
|||univector<float> right{ 0.5f, 0.25f };
|||univector<float> mono(left.size());
mono = (left + right) * 0.5f;
|||CHECK(mono[0] == 0.5f);
|||}
```

An unscaled sum is also available through simple expression arithmetic or [[`mixdown(E &&...)`:nosig]], but identical in-phase channels gain 6 dB and can exceed normalized sample range:

```c++
|||TEST_CASE("dsp/misc.md: sum a stereo downmix")
|||{
|||univector<float> left{ 0.5f, -0.5f };
|||univector<float> right{ 0.5f, 0.25f };
|||univector<float> mono(left.size());
mono = left + right;
|||CHECK(mono[0] == 1.0f);
|||}
```

Custom two-output transforms use an `f64x2x2` matrix. Each row contains the coefficients of one output channel:

```c++
|||TEST_CASE("dsp/misc.md: apply a custom stereo matrix")
|||{
|||univector<float> left{ 1.0f, 0.0f };
|||univector<float> right{ 0.0f, 1.0f };
|||univector<float> out_left(left.size());
|||univector<float> out_right(left.size());
const f64x2x2 matrix{
	f64x2{ 0.8, 0.2 },
	f64x2{ 0.2, 0.8 },
};
unpack(out_left, out_right) = mixdown_stereo(left, right, matrix);
|||CHECK(out_left[0] == 0.8f);
|||CHECK(out_right[1] == 0.8f);
|||}
```

Input and output channels should have matching lengths. `mixdown_stereo` is lazy, so the transform is evaluated when its result is assigned or otherwise materialized.

## Audio sample conversion

[[`convert_sample(const Tin &)`:nosig]] and the buffer [[`convert(Tout *, const Tin *, size_t)`:nosig]] overload rescale and clamp between KFR's native audio sample formats. Integer formats use the symmetric full-scale range $\pm(2^{N-1}-1)$, while floating-point formats use a scale of one:

| Runtime format | C++ storage type | Nominal full-scale range |
| --- | --- | --- |
| `audio_sample_type::i16` | `i16` / `int16_t` | $-32767$ to $32767$ |
| `audio_sample_type::i24` | `i24` | $-8388607$ to $8388607$ |
| `audio_sample_type::i32` | `i32` / `int32_t` | $-2147483647$ to $2147483647$ |
| `audio_sample_type::f32` | `f32` / `float` | $-1$ to $1$ nominally |
| `audio_sample_type::f64` | `f64` / `double` | $-1$ to $1$ nominally |

[[`audio_sample_type`:nosig]] identifies a format at runtime. [[`audio_sample_bit_depth(audio_sample_type)`:nosig]] returns its bit depth, and [[`audio_sample_is_float(audio_sample_type)`:nosig]] reports whether it is floating point. `i24` is KFR's packed three-byte sample type; use `i24`, not an `int32_t` buffer, for 24-bit sample storage.

### Converting known formats

When source and destination types are known at compile time, `size` is the number of individual samples:

```c++
|||TEST_CASE("dsp/misc.md: convert known sample formats")
|||{
const float input[] = { 0.0f, 0.25f, -0.25f, 1.0f, -1.0f };
int16_t output[std::size(input)];

convert(output, input, std::size(input));
|||CHECK(output[3] == 32767);
|||CHECK(output[4] == -32767);
|||}
```

For a scalar conversion, specify only the destination type when the source can be deduced:

```c++
|||TEST_CASE("dsp/misc.md: convert one sample to PCM24")
|||{
const i24 pcm24 = convert_sample<i24>(1.0f);
|||CHECK(static_cast<i32>(pcm24) == 8388607);
|||}
```

Conversion to an integer format clamps out-of-range floating-point input before storing it. It does not add dither, so use an audio quantization/dithering stage when that is required. Same-type conversion returns its input unchanged.

### Converting a runtime-selected format

If a decoder or external API reports the input format at runtime, keep the destination type fixed:

```c++
|||TEST_CASE("dsp/misc.md: convert a runtime-selected input format")
|||{
|||const int16_t encoded_samples[] = { 0, 32767, -32767 };
|||const size_t sample_count = std::size(encoded_samples);
|||const audio_sample_type input_type = audio_sample_type::i16;
const void* input = encoded_samples;
univector<float> output(sample_count);

if (input_type != audio_sample_type::unknown)
	convert(output.data(), input, input_type, sample_count);
|||CHECK(output[1] == 1.0f);
|||}
```

The reverse form selects the destination format at runtime:

```c++
|||TEST_CASE("dsp/misc.md: convert to a runtime-selected output format")
|||{
|||const size_t sample_count = 3;
|||univector<float> samples{ 0.0f, 1.0f, -1.0f };
univector<float> input = samples;
std::vector<std::byte> output(sample_count * 2); // i16 storage

convert(output.data(), audio_sample_type::i16, input.data(), sample_count);
|||CHECK(reinterpret_cast<const int16_t*>(output.data())[1] == 32767);
|||}
```

The pointer must refer to storage matching the declared runtime format. Invalid or `unknown` runtime formats have no fallback conversion. Use separate non-overlapping source and destination buffers; the conversion functions do not promise in-place or overlapping-buffer behavior.

## Interleaving and deinterleaving

[[`interleave(Tout *, const Tin **, size_t, size_t)`:nosig]] converts planar channel buffers to channel-interleaved storage. [[`deinterleave(Tout **, const Tin *, size_t, size_t)`:nosig]] performs the inverse. Both can convert the sample type while changing layout.

For $C$ channels and $N$ frames, the functions use:

$$
\begin{aligned}
	ext{interleaved}[iC + ch] &= \text{planar}[ch][i], \\
	ext{planar}[ch][i] &= \text{interleaved}[iC + ch].
\end{aligned}
$$

The final `size` parameter means samples per channel, or audio frames; it is not the total interleaved sample count.

### Planar stereo to interleaved PCM16

```c++
|||TEST_CASE("dsp/misc.md: interleave planar stereo PCM16")
|||{
|||univector<float> left{ 0.0f, 1.0f };
|||univector<float> right{ 0.5f, -1.0f };
const float* channels[] = { left.data(), right.data() };
univector<int16_t> interleaved(left.size() * 2);

interleave(interleaved.data(), channels, 2, left.size());
|||CHECK(interleaved[1] == 16383);
|||CHECK(interleaved[3] == -32767);
|||}
```

### Interleaved PCM24 to planar float

```c++
|||TEST_CASE("dsp/misc.md: deinterleave PCM24 to planar float")
|||{
|||const size_t frame_count = 2;
|||const i24 pcm24_interleaved[] = { convert_sample<i24>(0.0f), convert_sample<i24>(0.5f),
|||                                  convert_sample<i24>(1.0f), convert_sample<i24>(-1.0f) };
|||univector<float> left(frame_count);
|||univector<float> right(frame_count);
float* channels[] = { left.data(), right.data() };

deinterleave(channels, pcm24_interleaved, 2, frame_count);
|||CHECK(left[1] == 1.0f);
|||CHECK(right[1] == -1.0f);
|||}
```

Here `pcm24_interleaved` has type `const i24*`. For a source type known only at runtime, convert it to a known typed buffer first, then interleave or deinterleave it.

Convenience overloads also work with rectangular `univector2d` planar buffers:

```c++
|||TEST_CASE("dsp/misc.md: interleave a planar univector")
|||{
|||univector<float> left{ 0.0f, 1.0f };
|||univector<float> right{ 0.5f, -1.0f };
univector2d<float> planar;
planar.push_back(left);
planar.push_back(right);

univector<float> interleaved = interleave(planar);
|||CHECK(interleaved.size() == 4);
|||CHECK(interleaved[2] == 1.0f);
|||}
```

All planar channels must have the same frame count, and an explicit destination buffer must be sized for exactly `channels * frames` samples. These layout helpers do not validate mismatched shapes, insufficient capacity, null pointers, or overlap.

## Waveforms and test signals

[[`phasor(std::type_identity_t<T>, std::type_identity_t<T>, std::type_identity_t<T>)`:nosig]] creates a lazy phase ramp in normalized cycles $[0,1)$. Its three-argument form accepts frequency and sample rate in Hz, plus an initial phase in cycles. The one-argument [[`phasor(std::type_identity_t<T>)`:nosig]] form interprets its frequency as cycles per sample.

```c++
|||TEST_CASE("dsp/misc.md: generate a 440 Hz sine wave")
|||{
constexpr float sample_rate = 48000.0f;
univector<float, 480> sine_440 =
	sinenorm(phasor<float>(440.0f, sample_rate));
|||CHECK(sine_440[0] == 0.0f);
|||CHECK(sine_440[120] > 0.5f);
|||}
```

The waveform functions operate element-wise on phases. The radian forms are [[`sine(E1 &&)`:nosig]], [[`square(E1 &&)`:nosig]], [[`triangle(E1 &&)`:nosig]], [[`sawtooth(E1 &&)`:nosig]], and [[`isawtooth(E1 &&)`:nosig]]. Their normalized-cycle counterparts are [[`sinenorm(E1 &&)`:nosig]], [[`squarenorm(E1 &&)`:nosig]], [[`trianglenorm(E1 &&)`:nosig]], [[`sawtoothnorm(E1 &&)`:nosig]], and [[`isawtoothnorm(E1 &&)`:nosig]]. The normalized forms wrap their phase into $[0,1)$; use the radian forms when the input is already in radians.

```c++
|||TEST_CASE("dsp/misc.md: generate normalized waveforms")
|||{
auto phase = phasor<float>(0.01f); // 0.01 cycles per sample
univector<float, 256> triangle_wave = trianglenorm(phase);
univector<float, 256> saw_wave = sawtoothnorm(phase);
|||CHECK(triangle_wave[0] == 0.0f);
|||CHECK(abs(saw_wave[100] + 1.0f) < 1e-5f);
|||}
```

[[`unitimpulse()`:nosig]] produces one at index zero and zero afterward, which is useful for measuring impulse responses. [[`jaehne(std::type_identity_t<T>, size_t)`:nosig]] and [[`swept(std::type_identity_t<T>, size_t)`:nosig]] create finite swept-sine test signals with an explicit amplitude and sample count:

```c++
|||TEST_CASE("dsp/misc.md: generate impulse and swept test signals")
|||{
univector<float, 1024> impulse = unitimpulse<float>();
univector<float> test_chirp = jaehne<float>(0.5f, 48000);
univector<float> test_sweep = swept<float>(0.5f, 48000);
|||CHECK(impulse[0] == 1.0f);
|||CHECK(impulse[1] == 0.0f);
|||CHECK(test_chirp.size() == 48000);
|||CHECK(test_sweep.size() == 48000);
|||}
```

These APIs generate mathematical waveforms and test signals. The headers do not establish an anti-aliasing strategy such as BLEP/BLAMP correction or oversampling. In particular, discontinuous square and sawtooth waves, and the corners in triangle waves, can alias; choose a separately designed anti-aliased oscillator when that is a requirement.

## Waveshaping and frequency weighting

The waveshaper functions return lazy, memoryless expressions. [[`waveshaper_hardclip(E1 &&, double)`:nosig]] clamps samples to a symmetric threshold. [[`waveshaper_tanh(E1 &&, double)`:nosig]] applies a normalized hyperbolic-tangent curve, while [[`waveshaper_saturate_I(E1 &&, double)`:nosig]] and [[`waveshaper_saturate_II(E1 &&, double)`:nosig]] apply normalized Type-I and Type-II saturation curves. [[`saturate_I(E1 &&)`:nosig]] and [[`saturate_II(E1 &&)`:nosig]] expose the underlying odd-symmetric bounded curves directly.

```c++
|||TEST_CASE("dsp/misc.md: apply built-in waveshapers")
|||{
|||univector<float> audio{ -2.0f, -0.25f, 0.25f, 2.0f };
audio = waveshaper_hardclip(audio, 0.8);
audio = waveshaper_tanh(audio, 2.5);
audio = waveshaper_saturate_I(audio, 3.0);
|||CHECK(absmaxof(audio) <= 1.0f);
|||}
```

[[`waveshaper_poly(E1 &&, fbase, fbase, Cs...)`:nosig]] evaluates an odd polynomial $c_1x+c_3x^3+c_5x^5+\ldots$, allowing a custom memoryless curve:

```c++
// x - 0.25x³ + 0.05x⁵
|||TEST_CASE("dsp/misc.md: apply a polynomial waveshaper")
|||{
|||univector<float> audio{ -1.0f, 0.0f, 1.0f };
audio = waveshaper_poly(audio, 1.0, -0.25, 0.05);
|||CHECK(audio[0] == -0.8f);
|||CHECK(audio[1] == 0.0f);
|||CHECK(audio[2] == 0.8f);
|||}
```

These simple nonlinearities are useful for experiments and controlled effects, but they are not an analogue-circuit model and do not provide anti-aliasing. Avoid a zero drive value for the normalized `tanh` and saturation wrappers because their normalization divides by the response at that drive.

[[`aweighting(E1 &&)`:nosig]], [[`bweighting(E1 &&)`:nosig]], and [[`cweighting(E1 &&)`:nosig]] evaluate A-, B-, and C-weighting **frequency-response gains**. Their input is frequency in Hz and their output is a linear magnitude, normalized to one at 1 kHz—not a dB value and not a time-domain weighting filter. Apply them to a matching frequency axis or spectrum magnitude:

```c++
|||TEST_CASE("dsp/misc.md: apply A-weighting gains")
|||{
univector<float> frequencies = linspace(20.0f, 20000.0f, 512, false, ctrue);
univector<float> a_gain = aweighting(frequencies);
|||univector<float> magnitude(frequencies.size(), 1.0f);
univector<float> weighted_magnitude = magnitude * a_gain;
|||CHECK(a_gain[0] < 1.0f);
|||CHECK(weighted_magnitude.size() == frequencies.size());
|||}
```

## See also

- [Audio buffer management](../audio_io/read_audio.md)
- [Audio format support](../audio_io/file_support.md)
- [Window functions](window_functions.md)
