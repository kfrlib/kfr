# Designing IIR Filters with KFR

[Infinite impulse response (IIR)](../advanced/dsp_glossary.md#iir-infinite-impulse-response) filters provide sharp frequency-selective responses with relatively few coefficients. KFR designs them from analog lowpass prototypes, represents the intermediate result as zeros, poles, and gain, then converts the digital design to numerically robust [second-order sections (SOS)](../advanced/dsp_glossary.md#biquad-sos) for processing.

This article covers prototype and ZPK design. See [Biquad filters](bq.md) for SOS coefficient containers, streaming state, `iir_filter`, and forward-backward filtering.

## IIR design workflow

The normal workflow has three steps:

1. Create a normalized analog lowpass prototype such as [[`butterworth(int)`:nosig]].
2. Transform that prototype to the required digital response with [[`iir_lowpass(const zpk &, double, double)`:nosig]], [[`iir_highpass(const zpk &, double, double)`:nosig]], [[`iir_bandpass(const zpk &, double, double, double)`:nosig]], or [[`iir_bandstop(const zpk &, double, double, double)`:nosig]].
3. Convert the result to SOS with [[`to_sos(const zpk &)`:nosig]] and apply the sections.

```c++
#include <kfr/dsp.hpp>

using namespace kfr;

|||TEST_CASE("dsp/iir.md/design lowpass")
|||{
const zpk design = iir_lowpass(butterworth(4), 1000.0, 48000.0);
const iir_params<float> sections = to_sos<float>(design);

univector<float> input{ 0.0f, 1.0f, 0.5f, -0.5f, 0.0f };
univector<float> filtered = iir(input, sections);
|||CHECK(filtered.size() == input.size());
|||}
```

The final call to `iir` returns a lazy expression. For repeated buffer-by-buffer processing, construct an [[`iir_filter<T>`:nosig]] from the same `sections`; see [Stateful filtering](filters.md) for shared state, reset, buffer, expression, and multichannel guidance.

## The `zpk` representation

[[`zpk`:nosig]] represents a linear time-invariant filter with zero locations `z`, pole locations `p`, and gain `k`:

$$
H(s) = k \frac{\prod_i(s-z_i)}{\prod_i(s-p_i)}.
$$

The prototype functions return analog ZPK filters with a normalized lowpass cutoff of $1$ rad/s. The high-level `iir_*` functions transform those analog filters into a digital ZPK design. This representation is compatible with SciPy's `scipy.signal` ZPK convention and is useful for inspecting or transforming a design before it becomes a cascade of biquads.

## Choosing an analog prototype

KFR provides the following normalized analog lowpass prototypes:

| Prototype | Function | Characteristics |
| --- | --- | --- |
| Butterworth | [[`butterworth(int)`:nosig]] | Maximally flat passband magnitude and a smooth roll-off. |
| Bessel | [[`bessel(int)`:nosig]] | Phase-normalized Bessel/Thomson response with maximally flat group delay near the passband; useful when transient shape matters. |
| Chebyshev Type I | [[`chebyshev1(int, double)`:nosig]] | Steeper transition than Butterworth for a given order, with passband ripple `rp` in dB. |
| Chebyshev Type II | [[`chebyshev2(int, double)`:nosig]] | Flat passband with stopband ripple; `rs` is the minimum stopband attenuation in dB. |
| Elliptic | [[`elliptic(int, double, double)`:nosig]] | The sharpest transition for a given order, with ripple in both passband (`rp`, dB) and stopband (`rs`, dB). |

[[`butterworth(int)`:nosig]] and [[`bessel(int)`:nosig]] use tables for orders 1 through 24. Unsupported orders return a gain-only filter, so validate the requested order in application code. The Chebyshev designers calculate positive orders directly; an order of zero returns a gain-only filter. `rp` and `rs` are positive dB values and are not range-checked by the public API.

Elliptic support is optional. [[`elliptic(int, double, double)`:nosig]] is declared only when KFR is built with `KFR_HAVE_ELLIPTIC`; it requires Boost.Math's elliptic functions. Guard code that uses it with the same feature macro when supporting builds without Boost.

```c++
|||TEST_CASE("dsp/iir.md/analog prototypes")
|||{
const zpk smooth = butterworth(6);
const zpk low_ringing = bessel(6);
const zpk ripple_passband = chebyshev1(6, 0.5); // 0.5 dB passband ripple
const zpk ripple_stopband = chebyshev2(6, 60.0); // 60 dB stopband attenuation
|||}
```

## Specifying the digital response

The four `iir_*` functions accept an analog prototype and return a **digital** [[`zpk`:nosig]]:

| Response | Function | Frequency arguments |
| --- | --- | --- |
| Lowpass | [[`iir_lowpass(const zpk &, double, double)`:nosig]] | `prototype, cutoff, fs` |
| Highpass | [[`iir_highpass(const zpk &, double, double)`:nosig]] | `prototype, cutoff, fs` |
| Bandpass | [[`iir_bandpass(const zpk &, double, double, double)`:nosig]] | `prototype, lower_edge, upper_edge, fs` |
| Bandstop | [[`iir_bandstop(const zpk &, double, double, double)`:nosig]] | `prototype, lower_edge, upper_edge, fs` |

With an explicit `fs`, frequencies are in Hz:

```c++
|||TEST_CASE("dsp/iir.md/specify response in hertz")
|||{
const zpk highpass = iir_highpass(bessel(6), 1000.0, 48000.0);
const zpk bandpass = iir_bandpass(chebyshev1(4, 1.0), 1000.0, 2000.0, 48000.0);
|||}
```

If `fs` is omitted, it defaults to `2.0`, so frequencies are normalized to **Nyquist**, not to the sample rate: `0.0` is DC, `1.0` is Nyquist, and `0.1` is 10% of Nyquist. This differs from the single-biquad helpers described in [Biquad filters](bq.md), which use cycles per sample and therefore place Nyquist at `0.5`.

```c++
|||TEST_CASE("dsp/iir.md/specify normalized response")
|||{
// Passes frequencies between 10% and 30% of Nyquist.
const zpk bandpass = iir_bandpass(butterworth(4), 0.1, 0.3);
|||}
```

Keep edges within the valid range and ensure `lower_edge < upper_edge`. The functions do not validate the sample rate, edge ordering, or frequencies near and beyond Nyquist.

### What the transformations do

A lowpass prototype is scaled to the requested cutoff for lowpass designs, inverted about the cutoff for highpass designs, or converted to a band response for bandpass and bandstop designs. Band transformations use the warped lower and upper edges to derive an analog centre frequency

$$
\omega_0 = \sqrt{\omega_\text{low}\omega_\text{high}}
$$

and bandwidth

$$
B = \omega_\text{high} - \omega_\text{low}.
$$

A bandpass or bandstop transformation doubles the prototype order. For example, a fourth-order prototype produces an eighth-order bandpass or bandstop filter, which converts to four SOS sections.

## Prewarping and the bilinear transform

Analog prototypes cannot be mapped directly to equally spaced digital frequencies. The bilinear (Tustin) transform maps the stable analog $s$-plane to the stable digital $z$-plane with

$$
s = 2f_s \frac{z - 1}{z + 1}.
$$

That mapping warps frequency. KFR compensates at the requested edges before applying the analog transformation:

$$
\omega_\text{warped} = 4\tan\left(\pi\frac{f}{f_s}\right).
$$

Internally, KFR normalizes the bilinear stage to $f_s=2$; the combined calculation is equivalent to conventional prewarping with the caller's physical sample rate. The practical result is that a requested cutoff or band edge is placed correctly in the final digital response rather than merely approximated at low frequencies.

The lower-level analog transformations and bilinear conversion are implementation utilities. Use the four public `iir_*` functions for ordinary filter design.

## Converting ZPK to executable SOS

[[`to_sos(const zpk &)`:nosig]] converts a digital ZPK design to dynamic [[`iir_params<T, filters>`:nosig]] containing real-valued biquad sections. It pairs conjugate poles and zeros into second-order sections, uses $a_0=1$ coefficients, and folds the overall ZPK gain into the first emitted section. An odd-order design is padded into $\lceil N/2 \rceil$ sections; a gain-only ZPK becomes one flat section.

```c++
|||TEST_CASE("dsp/iir.md/convert to sos")
|||{
const zpk design = iir_bandstop(butterworth(4), 50.0, 70.0, 48000.0);
const iir_params<float> sections = to_sos<float>(design);
|||}
```

Use `float` or `double` for the `to_sos` coefficient type. Converting once and retaining `iir_params` avoids repeating root pairing and coefficient generation for each signal or processing block.

The executable cascade accepts at most [[`maximum_biquad_count`:nosig]] sections: currently 64, corresponding to [[`maximum_iir_order`:nosig]] of 128. `to_sos` itself does not impose the limit, but applying a dynamic cascade does. Account for order doubling when designing bandpass or bandstop filters.

## Applying the resulting cascade

Pass the SOS result to [[`iir(E1 &&, iir_params<T, filters>)`:nosig]] for a lazy expression, or construct an [[`iir_filter<T>`:nosig]] for streaming. The common [Stateful filtering](filters.md) guide covers state continuity, buffer and expression input, reset behavior, and multichannel processing.

The convenience overload [[`iir(E1 &&, const zpk &)`:nosig]] accepts a ZPK design directly:

```c++
|||TEST_CASE("dsp/iir.md/apply zpk cascade")
|||{
const zpk design = iir_lowpass(butterworth(4), 1000.0, 48000.0);
univector<float> input{ 0.0f, 1.0f, 0.5f, -0.5f, 0.0f };
univector<float> response = iir(input, design);
|||CHECK(response.size() == input.size());
|||}
```

It converts the ZPK to SOS each time it is called. Prefer an explicit [[`to_sos(const zpk &)`:nosig]] conversion when processing the design more than once. For biquad-specific look-ahead behavior, state sharing, and `filtfilt`, see [Biquad filters](bq.md).

## Example: reusable Chebyshev bandpass

This example designs an eighth-order Chebyshev Type I prototype with 1 dB passband ripple, transforms it to a 1–2 kHz digital bandpass at 48 kHz, and retains the SOS cascade for streaming. The band transformation doubles the final filter order to 16, producing eight SOS sections.

```c++
|||TEST_CASE("dsp/iir.md/reusable Chebyshev bandpass")
|||{
constexpr double sample_rate = 48000.0;

const zpk design = iir_bandpass(chebyshev1(8, 1.0), 1000.0, 2000.0, sample_rate);
const iir_params<float> sections = to_sos<float>(design);

univector<float> audio_block{ 0.0f, 1.0f, 0.5f, -0.5f, 0.0f };
iir_filter<float> bandpass_filter(sections);
bandpass_filter.apply(audio_block);
|||CHECK(audio_block.size() == 5);
|||}
```

## Visualizing a response

Apply a design to a unit impulse to obtain its impulse response, then pass that response to [[`plot_save(const std::string &, const T &, const std::string &)`:nosig]].

```c++
|||TEST_CASE("dsp/iir.md/visualize response")
|||{
const zpk design = iir_lowpass(bessel(12), 1000.0, 48000.0);
const iir_params<float> sections = to_sos<float>(design);

univector<float, 1024> response = iir(unitimpulse(), sections);
||||||||||||
plot_save("bessel_lowpass12", response,
          "phaseresp=True, log_freq=True, freq_dB_lim=(-160, 10), padwidth=8192");
||||||||||||
|||CHECK(response.size() == 1024);
|||}
```

`plot_save` requires the optional Python plotting dependencies; see [Plotting DSP data](../audio_io/plot.md).

## See also

- [Biquad filters](bq.md)
- [Gallery with IIR responses](examples/iir_gallery.md)
