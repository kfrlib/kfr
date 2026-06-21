# How to Design and Apply FIR Filters with KFR

This guide explains how to design and apply [Finite Impulse Response (FIR)](dsp_glossary.md#fir-finite-impulse-response) filters using the KFR library. FIR filters are widely used in digital signal processing for applications such as audio equalization, noise reduction, and signal conditioning. They offer linear phase response (when symmetric taps are used) and are always stable, making them a robust choice for many filtering tasks. KFR supports the standard windowed-sinc design methods (low-pass, high-pass, band-pass, and band-stop) and provides efficient runtime and expression-based APIs for applying the filters to signals.

## Overview of FIR Filters in KFR

KFR represents an FIR filter as a tap vector wrapped in a `fir_params` object (the taps are reversed internally so the convolution can be computed as a dot product against the most recent samples). The taps are held alongside a ring-buffer delay line in a `fir_state` object, which is consumed either by the `fir` free function (returning a lazy expression) or by the `fir_filter` class (also available as the `filter_fir` alias), which implements the generic `filter<T>` interface.

### Supported FIR Filter Types

KFR provides the following windowed-sinc design functions, which compute the tap coefficients for a given window function:

- **Low-pass (`fir_lowpass`)**: Passes frequencies below the cutoff and attenuates frequencies above it.
- **High-pass (`fir_highpass`)**: Attenuates frequencies below the cutoff and passes frequencies above it. Implemented via spectral inversion of the low-pass prototype.
- **Band-pass (`fir_bandpass`)**: Passes frequencies within a specified band. Constructed as the difference of two low-pass prototypes.
- **Band-stop (`fir_bandstop`)**: Attenuates frequencies within a specified band (notch filter). Constructed as a low-pass minus a second low-pass.

### Design and Application Functions

KFR provides functions and classes to design and apply FIR filters:

- `fir_lowpass`, `fir_highpass`, `fir_bandpass`, `fir_bandstop`: Design the tap coefficients using the windowed-sinc method.
- `fir_params<T>`: Wraps the tap vector (taps are reversed internally).
- `fir_state<T, U>`: Holds the parameters and a ring-buffer delay line.
- `fir_filter<T, U>` (alias `filter_fir<T, U>`): Runtime filter implementing the `filter<U>` interface; the recommended way to apply an FIR filter when streaming or when an opaque `filter` interface is required.
- `fir(expr, fir_params{...})` / `fir(expr, std::ref(state))`: Free functions returning a lazy expression that applies the filter.
- `short_fir(expr, taps)`: Optimized expression for small filters (2..33 taps) using SIMD-friendly fixed-size vectors.
- `moving_sum(expr, length)`: Specialized moving-sum (rectangular window) filter.

### Applying Filters

The `fir_filter` class applies the designed FIR filter to input data, maintaining internal state (the delay line) for continuous processing. Filters can be applied to plain arrays, `univector` objects, or expressions. All the internal state is preserved between calls to `apply`. Use `filter.reset()` to clear the filter's internal state when processing a new, unrelated signal.

## Designing FIR Filters

To design an FIR filter, you first choose a window function (for example, `window_kaiser`), allocate a tap vector whose size determines the filter order (`order = size - 1`), and call one of the design functions (`fir_lowpass`, `fir_highpass`, `fir_bandpass`, or `fir_bandstop`). The resulting taps are then wrapped in a `fir_params` and passed to a `fir_state` or directly to the `fir_filter` constructor.

> [!note]
> Frequencies passed to the design functions are **normalized** to the sample rate (`frequency_Hz / samplerate_Hz`), where `0.5` corresponds to the Nyquist frequency.

### Example: Designing a Low-Pass FIR Filter

The following example designs a low-pass FIR filter with a cutoff of 0.2 (normalized) using a Kaiser window:

```c++
// Initialize window function
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps.size(), 3.0));

// Initialize taps
univector<float, 7> taps;
fir_lowpass(taps, 0.2, kaiser, true);

// Initialize filter and delay line
filter_fir<float> filter(taps);
```

See [Expression handles](handle.md) for details on `expression_handle` and `to_handle`.

### Example: Designing a Band-Pass FIR Filter

This example creates a band-pass FIR filter with lower and upper cutoffs of 0.2 and 0.45 (normalized), using a Kaiser window and normalizing the DC gain to unity:

```c++
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps.size(), 3.0));

univector<float, 7> taps;
fir_bandpass(taps, 0.2, 0.45, kaiser, true);

filter_fir<float> filter(taps);
```

### Example: Designing a High-Pass FIR Filter

```c++
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps.size(), 3.0));

univector<float, 15> taps; // odd number of taps
fir_highpass(taps, 0.3, kaiser, true);

filter_fir<float> filter(taps);
```

> [!note]
> For a high-pass FIR filter, the number of taps must be **odd**.
> FIR filters with an even number of taps (Type II filters) always have a zero at $z = -1$ (the Nyquist frequency) and cannot be used as high-pass filters, which require unity gain at Nyquist.

### Example: Designing a Band-Stop (Notch) FIR Filter

```c++
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps.size(), 3.0));

univector<float, 63> taps;
fir_bandstop(taps, 0.2, 0.3, kaiser, true);

filter_fir<float> filter(taps);
```

You can pass your own coefficients to the `fir_filter` constructor (or to `fir_params`) if you compute them with another method.

## Applying FIR Filters

Once the filter is designed and initialized, you can apply it to input data using the `fir_filter` class. The `apply` function supports various input and output formats, including plain arrays, `univector` objects, and expressions.

### Applying to a Plain Array (In-Place)

```c++
float data[256];
// Initialize filter as shown above
filter.apply(data); // Apply filter in-place, size is determined automatically
```

### Applying to a Plain Array (Input/Output)

```c++
float input[256];
float output[256]; // size must be the same
// Initialize filter as shown above
filter.apply(output, input); // Read from input, apply, write to output
```

### Applying Using a Pointer and Size (In-Place)

```c++
float* data;
size_t size;
// Initialize filter as shown above
filter.apply(data, size); // Apply in-place, size is explicit
```

### Applying Using Two Pointers and Size

```c++
float* output;
float* input;
size_t size;
// Initialize filter as shown above
filter.apply(output, input, size); // Read from input, write to output, size is explicit
```

### Applying to a `univector` (In-Place)

```c++
univector<float> data(256); // or univector<float, 1000>
// Initialize filter as shown above
filter.apply(data); // Apply in-place, size is determined automatically
```

### Applying to a `univector` (Input/Output)

```c++
univector<float> input(256);  // or univector<float, 1000>
univector<float> output(256); // or univector<float, 1000>
// Initialize filter as shown above
filter.apply(output, input); // size is determined automatically
```

### Applying to an Expression

```c++
univector<float, 1000> output;
auto input = counter();
// Initialize filter as shown above
filter.apply(output, input);
```

## Using the `fir` Free Function (Lazy Expressions)

Instead of the runtime `fir_filter` class, you can use the `fir` free function to obtain a lazy expression that is evaluated on demand. This is convenient when composing filter chains or when you only need to materialize the result once.

```c++
univector<float> taps(63);
fir_lowpass(taps, 0.2, to_handle(window_kaiser(taps.size(), 3.0)), true);

univector<float> input = counter();
univector<float> output = fir(input, fir_params<float>{ taps });
```

To reuse the same filter state across multiple calls (streaming), pass a `fir_state` by reference:

```c++
fir_state<float> state{ taps };

univector<float> block1 = fir(input_block1, std::ref(state));
univector<float> block2 = fir(input_block2, std::ref(state)); // state carries over
```

## Short FIR Filters

For filters with a small number of taps (2..33), use `short_fir`, which is optimized for SIMD using fixed-size vectors for the taps and delay line:

```c++
univector<float, 7> taps;
fir_lowpass(taps, 0.2, to_handle(window_kaiser(taps.size(), 3.0)), true);

univector<float> output = short_fir(input, taps);
```

## Moving-Sum (Rectangular Window) Filter

KFR also provides `moving_sum`, a specialized filter that computes the running sum over a sliding window of fixed length. It is numerically equivalent to an FIR filter with all taps equal to 1, but uses an incremental update for efficiency:

```c++
univector<float> output = moving_sum(input, 64); // 64-sample window
```

## Long Filters and FFT-Based Convolution

For long filters, the KFR DFT module provides [`convolve_filter`](convolution.md), which performs the same convolution math using FFT-based overlap-add and is significantly faster for large tap counts than the direct-form `fir_filter`.

## Visualizing Filter Response

To analyze the filter's frequency response, apply the filter to a unit impulse and use KFR's `plot_save` function to generate plots.

> [!note]
> Ensure python with numpy, scipy and matplotlib is installed.

Example:

```c++
univector<float, 63> taps;
fir_lowpass(taps, 0.2, to_handle(window_kaiser(taps.size(), 3.0)), true);

filter_fir<float> filter(taps);

// Apply to unit impulse
univector<float, 1024> output;
univector<float, 1024> impulse;
impulse[0] = 1;
filter.apply(output, impulse);

// Save frequency response plot
const std::string options = "phaseresp=True, log_freq=True, freq_dB_lim=(-160, 10), padwidth=8192";
plot_save("fir_lowpass_kaiser", output, options + ", title='Low-pass FIR filter, Kaiser window'");
```

## Best Practices

1. **Reuse `fir_state` / `fir_filter`**: Constructing the filter (and reversing the taps) is cheap, but reusing the same filter object across calls avoids reallocating the delay line and preserves state for streaming.
2. **Reset Filter State**: Use `filter.reset()` to clear the filter's internal state (the delay line) when processing new, unrelated signals.
3. **Frequency Normalization**: Frequencies passed to the design functions are normalized to the sample rate (`frequency_Hz / samplerate_Hz`). Ensure frequencies are in the range `0` to `0.5`.
4. **Odd Tap Count for High-Pass**: High-pass FIR filters require an odd number of taps; even-tap (Type II) filters cannot represent a high-pass response.
5. **Choose the Right Tool**: Use `short_fir` for 2..33 taps, `fir_filter` / `fir` for general direct-form filtering, and `convolve_filter` for long filters where FFT-based overlap-add is faster.
6. **Window Selection**: The window function controls the trade-off between transition width and stop-band attenuation. Kaiser (with the β parameter) is a good general-purpose choice.

## See Also

- [Convolution / `convolve_filter`](convolution.md)
- [IIR Filters](iir.md)
- [Biquad Filter Documentation](bq.md)
- [Filter Class Definition](auto/classes/kfr.filter.t.md)
- [Gallery with results of applying various FIR filters](fir_gallery.md)
