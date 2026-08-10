# Designing and Applying Biquad Filters with KFR

A [biquad](../advanced/dsp_glossary.md#biquad-sos) is a second-order IIR filter section. It is small enough for efficient real-time processing, but can describe the lowpass, highpass, equalizer, and shelving responses used in many audio and measurement applications. Several sections can be cascaded as a second-order-section (SOS) filter to make a higher-order IIR filter without evaluating one large, numerically fragile polynomial.

KFR provides coefficient helpers for common single-biquad responses and a common SOS processing API. Use [[`biquad_section<T>`:nosig]] to hold one section, [[`iir_params<T, filters>`:nosig]] to hold a cascade, and either [[`iir(E1 &&, iir_params<T, filters>)`:nosig]] or [[`iir_filter<T>`:nosig]] to apply it.

## Biquad sections and coefficients

[[`biquad_section<T>`:nosig]] stores the six coefficients of

$$
H(z) = \frac{b_0 + b_1z^{-1} + b_2z^{-2}}
            {a_0 + a_1z^{-1} + a_2z^{-2}}.
$$

Its explicit constructor takes the denominator first, followed by the numerator:

```c++
#include <kfr/dsp.hpp>

using namespace kfr;

|||TEST_CASE("dsp/bq.md/section-coefficients")
|||{
biquad_section<float> section{
    2.0f, -1.0f, 0.25f,
    0.5f, 0.25f, 0.125f
};
|||    CHECK(section.a0 == 2.0f);
|||    CHECK(section.b2 == 0.125f);
|||}
```

The default-constructed section is a pass-through: $a_0=b_0=1$ and all other coefficients are zero. This is also the padding used for unused slots in a fixed-size cascade.

### Normalizing supplied coefficients

The SIMD SOS processor stores $a_1$, $a_2$, $b_0$, $b_1$, and $b_2$; it assumes $a_0=1$. Coefficients returned by KFR's design helpers already satisfy that requirement. When importing coefficients from another source, normalize the denominator before creating [[`iir_params<T, filters>`:nosig]]:

```c++
|||TEST_CASE("dsp/bq.md/normalize-imported-coefficients")
|||{
biquad_section<float> imported{ 2.0f, -1.0f, 0.5f, 1.0f, 0.5f, 0.25f };
auto params = iir_params{ imported.normalized_a0() };
|||    CHECK(params.a1[0] == -0.5f);
|||    CHECK(params.b0[0] == 0.5f);
|||}
```

[[`biquad_section<T>::normalized_a0`:nosig]] divides $a_1$, $a_2$, and all numerator coefficients by $a_0$, then sets $a_0$ to one. [[`biquad_section<T>::normalized_b0`:nosig]] keeps the denominator unchanged, sets $b_0$ to one, and divides $b_1$ and $b_2$ by the original $b_0$. [[`biquad_section<T>::normalized_all`:nosig]] applies both operations. Normalizing $b_0$ changes the section gain, so use it only when that gain convention is intended; it is not needed by [[`iir(E1 &&, iir_params<T, filters>)`:nosig]].

## Designing one biquad

The response-kind enumeration [[`biquad_type`:nosig]] names the available categories: lowpass, highpass, bandpass, bandstop, peak, notch, low shelf, and high shelf. It is descriptive; choose a coefficient helper directly rather than passing this enum to a generic design function.

All helpers use a normalized frequency in **cycles per sample**:

$$
f_\text{normalized} = \frac{f_\text{Hz}}{f_s}.
$$

Therefore $0.5$ is Nyquist. At a 48 kHz sample rate, 1 kHz is `1000.0f / 48000.0f`. This convention differs from the higher-level ZPK IIR design functions when their sample-rate argument is omitted; see [IIR filters](iir.md) for that API.

For helpers that accept it, `Q` is the quality factor: larger values make the response more selective around its cutoff or centre frequency. `gain` is in dB; a positive value boosts and a negative value cuts. Choose frequencies strictly between DC and Nyquist and use positive, finite `Q` values. The coefficient helpers do not validate these inputs.

| Response   | Helper                       | Parameters and behaviour                                                                |
|------------|------------------------------|-----------------------------------------------------------------------------------------|
| All-pass   | [[`biquad_allpass`:nosig]]   | `frequency, Q`. Preserves magnitude while changing phase around the selected frequency. |
| Lowpass    | [[`biquad_lowpass`:nosig]]   | `frequency, Q`. Passes lower frequencies and attenuates higher ones.                    |
| Highpass   | [[`biquad_highpass`:nosig]]  | `frequency, Q`. Attenuates lower frequencies, including DC, and passes higher ones.     |
| Bandpass   | [[`biquad_bandpass`:nosig]]  | `frequency, Q`. Passes a band around its centre frequency.                              |
| Notch      | [[`biquad_notch`:nosig]]     | `frequency, Q`. Rejects a narrow band around its centre frequency.                      |
| Peak       | [[`biquad_peak`:nosig]]      | `frequency, Q, gain`. A peaking EQ that boosts or cuts around the centre frequency.     |
| Low shelf  | [[`biquad_lowshelf`:nosig]]  | `frequency, gain`. Boosts or cuts the low-frequency side of the transition.             |
| High shelf | [[`biquad_highshelf`:nosig]] | `frequency, gain`. Boosts or cuts the high-frequency side of the transition.            |

The shelf helpers have no `Q` parameter; their transition shape is fixed by their design. `biquad_type::bandstop` identifies a band-reject response, while the corresponding single-section helper is named [[`biquad_notch`:nosig]].

### Example: a lowpass section

This produces a second-order lowpass at 1 kHz for 48 kHz audio. $Q \approx 0.707$ is a common Butterworth-like choice for a two-pole lowpass.

```c++
#include <kfr/dsp.hpp>

using namespace kfr;

|||TEST_CASE("dsp/bq.md/lowpass-section")
|||{
constexpr float sample_rate = 48000.0f;
auto lowpass = biquad_lowpass<float>(1000.0f / sample_rate, 0.707f);

univector<float> input = { 0.0f, 1.0f, 0.5f, -0.5f, 0.0f };
univector<float> filtered = iir(input, iir_params{ lowpass });
|||    CHECK(filtered.size() == input.size());
|||}
```

### Example: a two-band EQ

Sections are evaluated in their stored order. This example cuts a narrow 50 Hz interference component and then applies a high-shelf boost:

```c++
|||TEST_CASE("dsp/bq.md/two-band-eq")
|||{
constexpr float fs = 48000.0f;

std::vector<biquad_section<float>> sections{
    biquad_notch<float>(50.0f / fs, 12.0f),
    biquad_highshelf<float>(6000.0f / fs, 3.0f), // 3 dB boost
};

iir_params<float> params(sections);
univector<float> input = { 0.0f, 0.5f, 1.0f, 0.5f, 0.0f };
univector<float> filtered = iir(input, params);
|||    CHECK(filtered.size() == input.size());
|||}
```

## Cascades, parameters, and limits

[[`iir_params<T, filters>`:nosig]] is the coefficient container consumed by the SOS processing functions. A fixed-size `iir_params<T, filters>` has its section count in the type and is useful when the cascade size is known at compile time. The default form, `iir_params<T>`, is a dynamic container of [[`biquad_section<T>`:nosig]] values and is generally the convenient choice for designed or user-configured filters.

Both forms can be built from one section or a container of sections. A dynamic cascade is dispatched internally to a fixed-size SIMD implementation, rounding its section count up to the next power of two and padding extra slots with pass-through sections.

KFR supports at most [[`maximum_biquad_count`:nosig]] sections: currently 64, derived from the [[`maximum_iir_order`:nosig]] of 128. A dynamic cascade containing 33 through 64 sections is processed by the 64-section specialization; 65 sections are rejected. A fixed [[`iir_state<T, filters>`:nosig]] similarly requires between one and 64 sections.

## Applying a cascade with `iir`

[[`iir(E1 &&, iir_params<T, filters>)`:nosig]] returns a lazy expression. Passing parameters by value creates an expression with owned, initially zero state:

```c++
|||TEST_CASE("dsp/bq.md/apply-cascade")
|||{
auto params = iir_params{ biquad_lowpass<float>(1000.0f / 48000.0f, 0.707f) };
univector<float> input = { 0.0f, 1.0f, 0.5f, -0.5f, 0.0f };
univector<float> output = iir(input, params);
|||    CHECK(output.size() == input.size());
|||}
```

See [Stateful filtering](filters.md) for expression evaluation, block continuity, owned versus referenced state, reset behavior, initial history, and multichannel processing.

### Sharing state between blocks

[[`iir_state<T, filters>`:nosig]] combines fixed-size parameters and the delay registers needed by the cascade. Pass it through `std::ref` to the state-sharing overload of [[`iir(E1 &&, std::reference_wrapper<iir_state<T, filters>>)`:nosig]] when blocks must form one continuous signal:

```c++
|||TEST_CASE("dsp/bq.md/share-state-between-blocks")
|||{
using params_type = iir_params<float, 1>;

params_type params{ biquad_highpass<float>(20.0f / 48000.0f, 0.707f) };
iir_state state{ params };

univector<float> input_block1 = { 0.0f, 1.0f, 0.5f, 0.0f };
univector<float> input_block2 = { -0.5f, -1.0f, -0.5f, 0.0f };
univector<float> output1 = iir(input_block1, std::ref(state));
univector<float> output2 = iir(input_block2, std::ref(state)); // continues block 1
|||    CHECK(output1.size() == input_block1.size());
|||    CHECK(output2.size() == input_block2.size());
|||}
```

The state is mutable; its ownership and lifetime rules are described in [Stateful filtering](filters.md). To begin an unrelated signal, construct a fresh state, or replace it with a newly constructed `iir_state` using the same parameters.

### Look-ahead and alignment

For a cascade of more than one section, KFR processes sections efficiently by looking ahead `filters - 1` samples within each evaluation block. The public [[`iir(E1 &&, iir_params<T, filters>)`:nosig]] overloads return the compensated [[`expression_biquads`:nosig]] form. It absorbs that look-ahead internally, including at a block boundary, so output index $i$ remains aligned with input index $i$. This is the normal choice.

[[`expression_biquads_l`:nosig]] is the uncompensated look-ahead alias. It retains the speed-oriented priming behaviour but does not restore the input/output time alignment: its output is shifted earlier by `filters - 1` samples, and its first `filters - 1` outputs are a priming transient. Use it only when the caller will realign the result or when that shift is explicitly acceptable. Neither variant removes the IIR filter's actual frequency-dependent phase response; “aligned” here refers to sample indexing and the implementation's look-ahead latency.

## Streaming with `iir_filter`

[[`iir_filter<T>`:nosig]] is a runtime filter object built from dynamic [[`iir_params<T, filters>`:nosig]]. It is the practical interface for callback-style audio or other buffer-by-buffer processing. Its internal filter expression retains delay state between `apply` calls. When KFR is built with multiarchitecture support, its implementation can select the appropriate SIMD specialization for the current CPU at runtime.

```c++
|||TEST_CASE("dsp/bq.md/streaming-iir-filter")
|||{
std::vector<biquad_section<float>> sections{
    biquad_lowshelf<float>(150.0f / 48000.0f, 4.0f),
    biquad_highpass<float>(30.0f / 48000.0f, 0.707f),
};

iir_filter<float> filter{ iir_params<float>(sections) };
univector<float> audio_block1 = { 0.0f, 1.0f, 0.5f, 0.0f };
univector<float> audio_block2 = { -0.5f, -1.0f, -0.5f, 0.0f };
filter.apply(audio_block1); // in place
filter.apply(audio_block2); // continues from audio_block1
|||    CHECK(audio_block1.size() == 4);
|||    CHECK(audio_block2.size() == 4);
|||}
```

See [Stateful filtering](filters.md) for `apply` overloads, expression input, buffer sizing, reset behavior, and multichannel processing. Call [[`filter<T>::reset`:nosig]] to clear the wrapped IIR delay state while preserving its coefficients.

## Forward-backward filtering with `filtfilt`

[[`filtfilt`:nosig]] applies one forward pass over a `univector`'s stored samples, then evaluates a second pass through a reversed lazy view of that same vector. It does not reverse the vector in memory. The forward and backward passes cancel the filter's phase response, making it useful for offline analysis when phase distortion is unacceptable.

```c++
|||TEST_CASE("dsp/bq.md/forward-backward-filtering")
|||{
auto section = biquad_lowpass<float>(1000.0f / 48000.0f, 0.707f);
iir_params<float> params(section);

univector<float> input = { 0.0f, 1.0f, 0.5f, -0.5f, 0.0f };
univector<float> recording = input;
filtfilt(recording, params);
|||    CHECK(recording.size() == input.size());
|||}
```

It only accepts a `univector` and modifies it in place. Each pass starts with a new filter expression. Unlike some forward-backward filtering implementations, KFR does not extend or pad the endpoints, so startup and ending transients can remain near the signal edges. Also, applying the response twice squares its magnitude response; it is not a drop-in replacement for a single-pass filter in real-time processing.

## Removing DC offset

[[`dcremove`:nosig]] is a concise expression helper for DC removal. It designs a one-section, second-order Butterworth highpass using the cutoff and sampling rate specified in Hz:

```c++
|||TEST_CASE("dsp/bq.md/remove-dc-offset")
|||{
constexpr double fs = 48000.0;
univector<float> samples = { 0.5f, 1.0f, 0.5f, 1.0f, 0.5f };
univector<float> corrected = dcremove(samples, 10.0, fs);
|||    CHECK(corrected.size() == samples.size());
|||}
```

The resulting expression uses the same stateful IIR mechanism as [[`iir(E1 &&, iir_params<T, filters>)`:nosig]]. Materializing one expression is suitable for a finite buffer. For continuous blocks, retain an [[`iir_state<T, filters>`:nosig]] built from [[`biquad_highpass`:nosig]] and use the state-sharing `iir` overload instead. The older `dcremove(expression, cutoff)` overload, which accepts a normalized cutoff without a sample rate, is deprecated.

## See also

- [IIR filters](iir.md) for Butterworth, Chebyshev, Bessel, and elliptic filters designed from analog prototypes.
- [FIR filters](fir.md) for finite impulse response filters and linear-phase designs.
- [Gallery with biquad responses](examples/bq_gallery.md).
