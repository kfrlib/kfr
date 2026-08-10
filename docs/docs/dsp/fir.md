# Designing and Applying FIR Filters with KFR

[Finite impulse response (FIR)](../advanced/dsp_glossary.md#fir-finite-impulse-response) filters are widely used for equalization, noise reduction, resampling, and general signal conditioning. They have no feedback path, so they are unconditionally stable. When their taps are symmetric, they also have linear phase. KFR provides windowed-sinc coefficient designers and several ways to apply the resulting filter: lazy expressions for composition, a SIMD-oriented short-filter expression, and a runtime streaming filter.

For a tap sequence $h[0], \ldots, h[M-1]$, KFR's FIR operations implement the causal convolution

$$
y[n] = \sum_{i=0}^{M-1} x[n-i]h[i].
$$

Here `h[0]` multiplies the current input sample. Samples before the beginning of a new filter state are zero.

## FIR filters in KFR

The following types and functions cover the usual FIR workflows:

- [[`fir_params<T>`:nosig]] stores a generic FIR's coefficients in its internal representation.
- [[`fir_state<T, U>`:nosig]] combines the parameters with the ring-buffer delay line used for streaming.
- [[`fir(E1 &&, fir_params<T>)`:nosig]] returns a general, lazy FIR expression with owned state; [[`fir(E1 &&, std::reference_wrapper<fir_state<T, U>>)`:nosig]] uses caller-owned state.
- [[`short_fir(E1 &&, const univector<T, TapCount> &)`:nosig]] is a SIMD-friendly expression for fixed-size filters from 2 through 33 taps.
- [[`fir_filter<T, U>`:nosig]] (also available as [[`filter_fir`:nosig]]) is a runtime `filter` object for processing buffers or expressions over successive calls.
- [[`moving_sum(E1 &&, size_t)`:nosig]] is the efficient special case with every tap equal to one.

All FIR expressions are stateful, sequential, one-dimensional expressions. See [Stateful filtering](filters.md) for evaluation order, block continuity, state ownership, reset behavior, initial history, and multichannel processing.

## Designing FIR coefficients

KFR's coefficient helpers use the windowed-sinc method. Allocate the destination tap vector first; its size $N$ determines a filter order of $N-1$. Then supply a window through an [[`expression_handle<T, Dims>`:nosig]], normally created with [[`to_handle(E &)`:nosig]]. The window controls the trade-off between transition width and stopband attenuation.

All design frequencies are normalized to the sample rate:

$$
f_\text{normalized} = \frac{f_\text{Hz}}{f_s}.
$$

Thus $0.5$ is Nyquist; for example, a 4 kHz cutoff at 48 kHz is `4000.0f / 48000.0f`. Use cutoffs and band edges between DC and Nyquist, with the lower band edge before the upper one. The helpers do not perform range validation.

| Response | Coefficient helper                                                                                                                  | Parameters                                        |
|----------|-------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------|
| Lowpass  | [[`fir_lowpass(univector<T, Tag> &, std::type_identity_t<T>, const expression_handle<T> &, bool)`:nosig]]                           | `taps, cutoff, window, normalize`                 |
| Highpass | [[`fir_highpass(univector<T, Tag> &, std::type_identity_t<T>, const expression_handle<T> &, bool)`:nosig]]                          | `taps, cutoff, window, normalize`                 |
| Bandpass | [[`fir_bandpass(univector<T, Tag> &, std::type_identity_t<T>, std::type_identity_t<T>, const expression_handle<T> &, bool)`:nosig]] | `taps, lower_edge, upper_edge, window, normalize` |
| Bandstop | [[`fir_bandstop(univector<T, Tag> &, std::type_identity_t<T>, std::type_identity_t<T>, const expression_handle<T> &, bool)`:nosig]] | `taps, lower_edge, upper_edge, window, normalize` |

Equivalent overloads accept an `univector_ref` when the output is a view rather than an owning vector. Lowpass is the basic windowed-sinc prototype; highpass is its spectral inversion. Bandpass and bandstop combine two lowpass prototypes.

### Example: Kaiser-windowed lowpass

This 63-tap design has a 4 kHz cutoff at 48 kHz. The `true` argument requests normalization.

```c++
#include <kfr/dsp.hpp>

using namespace kfr;

|||TEST_CASE("dsp/fir.md/Kaiser-windowed lowpass")
|||{
constexpr float sample_rate = 48000.0f;
univector<float, 63> taps;
auto kaiser_window = to_handle(window_kaiser<float>(taps.size(), 6.0f));

fir_lowpass(taps, 4000.0f / sample_rate, kaiser_window, true);
|||CHECK(taps.size() == 63);
|||}
```

[[`window_kaiser(size_t, std::type_identity_t<T>, ctype_t<T>)`:nosig]] is a useful general-purpose window. Its beta parameter controls the transition-width versus sidelobe-attenuation trade-off. See [Window functions](window_functions.md) for alternatives.

### Tap count, symmetry, and normalization

For all four designers, odd tap counts have an actual centre sample. KFR assigns that centre coefficient analytically rather than evaluating `sinc(0)`: `2*cutoff` for lowpass, `1 - 2*cutoff` for highpass, `2*(upper - lower)` for bandpass, and `1 - 2*(upper - lower)` for bandstop. The remaining coefficients are symmetric about that centre when the supplied window is symmetric.

Use an odd tap count for highpass and bandstop filters. An even-length symmetric FIR is a Type II filter and always has a zero at Nyquist, so it cannot provide a normal high-frequency passband. Odd-length lowpass and bandpass filters are also convenient because they have an unambiguous centre tap, but are not required by the API.

When `normalize` is `true`, KFR applies the designer's response-specific gain compensation after windowing. In particular, lowpass and bandstop designs are scaled to unity DC gain. Leave it `false` when a later stage supplies the desired gain or when preserving the unscaled windowed-sinc coefficients is important.

### Other response examples

```c++
|||TEST_CASE("dsp/fir.md/other FIR responses")
|||{
univector<float, 63> highpass_taps;
univector<float, 63> bandpass_taps;
univector<float, 63> bandstop_taps;
auto kaiser_window = to_handle(window_kaiser<float>(highpass_taps.size(), 6.0f));

fir_highpass(highpass_taps, 0.10f, kaiser_window, true);
fir_bandpass(bandpass_taps, 0.10f, 0.25f, kaiser_window, true);
fir_bandstop(bandstop_taps, 0.10f, 0.25f, kaiser_window, true);
|||CHECK(highpass_taps.size() == bandpass_taps.size());
|||}
```

The three vectors have the same size here only to reuse the window. Each design can choose a tap count appropriate to its transition-width and attenuation requirements.

## Parameters, tap ordering, and state

Pass coefficients to [[`fir_params<T>`:nosig]] in **natural order**: `{ h[0], h[1], ... }`. The constructor reverses them internally so that the generic implementation can calculate dot products against the newest samples first.

```c++
|||TEST_CASE("dsp/fir.md/FIR parameter ordering")
|||{
univector<float, 3> taps{ 0.25f, 0.5f, 0.25f };
fir_params<float> params{ taps }; // input order is h[0], h[1], h[2]
|||CHECK(params.taps.size() == taps.size());
|||}
```

Do not reverse coefficients before constructing `fir_params`; that would reverse their time order twice. A [[`fir_state<T, U>`:nosig]] constructed from an existing `fir_params` takes its already-reversed taps as-is. A state constructed directly from an ordinary container performs the normal `fir_params` reversal.

`T` is the coefficient type, while `U` is the sample type. They can differ. In particular, real coefficients can filter complex samples:

```c++
|||TEST_CASE("dsp/fir.md/real taps complex samples")
|||{
univector<float, 3> taps{ 0.25f, 0.5f, 0.25f };
|||univector<complex<float>> complex_input{ { 1.0f, 0.0f }, { 0.5f, 0.5f } };
fir_state<float, complex<float>> state{ taps };
univector<complex<float>> output = fir(complex_input, std::ref(state));
|||CHECK(output.size() == complex_input.size());
|||}
```

The general FIR, short FIR, and runtime [[`fir_filter<T, U>`:nosig]] all support this real-tap/complex-sample use case.

### Owned and referenced state

[[`fir(E1 &&, fir_params<T>)`:nosig]] owns a fresh FIR state and is suitable for a finite signal. To retain history across blocks, pass an externally owned [[`fir_state<T, U>`:nosig]] with [[`fir(E1 &&, std::reference_wrapper<fir_state<T, U>>)`:nosig]]. See [Stateful filtering](filters.md) for the ownership, lifetime, reset, and initial-history rules shared by KFR filters.

## Short FIR filters

[[`short_fir(E1 &&, const univector<T, TapCount> &)`:nosig]] is specialized for filters with a fixed, compile-time tap count from 2 through 33. It stores taps and recent samples in SIMD-friendly fixed-size vectors and pads internally where needed. Supply taps in the same natural order as general FIR filters:

```c++
|||TEST_CASE("dsp/fir.md/short FIR")
|||{
|||univector<float> input{ 1, 2, 3, 4 };
univector<float, 7> taps{ 0.02f, 0.08f, 0.20f, 0.40f, 0.20f, 0.08f, 0.02f };
univector<float> output = short_fir(input, taps);
|||CHECK(output.size() == input.size());
|||}
```

For streaming, retain a [[`short_fir_state<tapcount, T, U>`:nosig]] and use the referenced overload. The state type's first template argument is the padded internal count, not always the visible tap count. For example, six taps use `short_fir_state<9, float>`:

```c++
|||TEST_CASE("dsp/fir.md/streaming short FIR")
|||{
univector<float, 6> taps{ 1, 2, -2, 0.5f, 0.0625f, 4 };
short_fir_state<9, float> state{ taps };

|||univector<float> input_block1{ 1, 2, 3 };
|||univector<float> input_block2{ 4, 5, 6 };
univector<float> output1 = short_fir(input_block1, std::ref(state));
univector<float> output2 = short_fir(input_block2, std::ref(state));
|||CHECK(output1.size() == input_block1.size());
|||CHECK(output2.size() == input_block2.size());
|||}
```

Use generic [[`fir(E1 &&, fir_params<T>)`:nosig]] instead when the tap count is only known at runtime or is outside this range.

## Moving sum

[[`moving_sum(E1 &&, size_t)`:nosig]] is a rectangular-window FIR: it returns the sum of the most recent `length` samples. Rather than calculate a complete dot product for each sample, it maintains a running sum by adding the incoming sample and subtracting the sample that leaves the window.

```c++
|||TEST_CASE("dsp/fir.md/moving sum")
|||{
|||univector<float> input{ 1, 2, 3, 4 };
univector<float> window_sum = moving_sum(input, 64);
|||CHECK(window_sum.size() == input.size());
|||}
```

This is a sum, not an average. Divide by the length if a moving average is needed. Use a positive window length.

For continuous blocks, retain [[`moving_sum_state<U, Tag>`:nosig]] and share it by reference:

```c++
|||TEST_CASE("dsp/fir.md/streaming moving sum")
|||{
moving_sum_state<float> state{ 64 };
|||univector<float> input_block1{ 1, 2, 3 };
|||univector<float> input_block2{ 4, 5, 6 };
univector<float> sum1 = moving_sum(input_block1, std::ref(state));
univector<float> sum2 = moving_sum(input_block2, std::ref(state));
|||CHECK(sum1.size() == input_block1.size());
|||CHECK(sum2.size() == input_block2.size());
|||}
```

## Runtime filtering with `fir_filter`

[[`fir_filter<T, U>`:nosig]] owns a [[`fir_state<T, U>`:nosig]] and implements the generic [[`filter<T>`:nosig]] interface. It is the practical choice for callback-style processing and for code that needs a runtime filter object. [[`filter_fir`:nosig]] is an alias for the same type.

```c++
|||TEST_CASE("dsp/fir.md/runtime FIR filter")
|||{
|||univector<float, 3> taps{ 0.25f, 0.5f, 0.25f };
filter_fir<float> filter{ taps };

|||univector<float> audio_block1{ 1, 2, 3 };
|||univector<float> audio_block2{ 4, 5, 6 };
filter.apply(audio_block1);
filter.apply(audio_block2);

// After the final input block, emit the remaining causal FIR response.
univector<float> tail(taps.size() - 1);
filter.apply_zeros(tail);
|||CHECK(tail.size() == taps.size() - 1);
|||}
```

For an input with $N$ samples and $M$ taps, append the $M - 1$ samples generated by [[`filter<T>::apply_zeros`:nosig]] after the final input block to obtain the complete $N + M - 1$ causal convolution result. Do not reset the filter before draining it.

`fir_filter` overrides [[`filter<T>::reset`:nosig]] to clear its delay line and ring-buffer cursor. Replacing coefficients through `set_taps` or `set_params` also starts with zero history. See [Stateful filtering](filters.md) for the shared `apply` and `apply_zeros` overloads, buffer sizing, expression input, state, and multichannel rules.

When multiarchitecture support is enabled, `fir_filter` can select an implementation for the current CPU at runtime. The free expression functions are still useful for composition and fixed-type pipelines.

## Choosing direct FIR or FFT convolution

Use [[`short_fir(E1 &&, const univector<T, TapCount> &)`:nosig]] for fixed filters of 2 to 33 taps, and general [[`fir(E1 &&, fir_params<T>)`:nosig]] or [[`fir_filter<T, U>`:nosig]] for other direct-form filters. Direct FIR has no block-convolution buffering and is generally the simplest option for short or moderate tap counts.

For a long, fixed impulse response, [[`convolve_filter<T>`:nosig]] uses FFT overlap-add convolution and is significantly faster at large tap counts. There is no universal tap-count crossover: it depends on the sample type, CPU, input block size, and latency constraints. Measure the target workload rather than relying on a fixed threshold. FFT convolution has block processing and overlap state, so it is most appropriate when its throughput benefit outweighs its buffering and latency trade-offs. See [Convolution filter details](../dft/convolution.md).

## Visualizing a response

An impulse response is a convenient way to inspect a designed filter. Apply the filter to an impulse, then use [[`plot_save(const std::string &, const T &, const std::string &)`:nosig]] to create a frequency-response plot.

```c++
|||TEST_CASE("dsp/fir.md/visualize response")
|||{
univector<float, 63> taps;
fir_lowpass(taps, 0.2f, to_handle(window_kaiser<float>(taps.size(), 6.0f)), true);

filter_fir<float> filter{ taps };
univector<float, 1024> impulse = unitimpulse();
univector<float, 1024> response;
filter.apply(response, impulse);

||||||||||
plot_save("fir_lowpass", response,
          "phaseresp=True, log_freq=True, freq_dB_lim=(-160, 10), padwidth=8192");
||||||||||
|||CHECK(response.size() == impulse.size());
|||}
```

`plot_save` requires the optional Python plotting dependencies; see [Plotting DSP data](../audio_io/plot.md).

## See also

- [Convolution filter details](../dft/convolution.md)
- [IIR filters](iir.md)
- [Biquad filters](bq.md)
- [Gallery with FIR responses](examples/fir_gallery.md)
