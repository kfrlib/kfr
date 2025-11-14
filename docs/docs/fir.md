# How to Apply an FIR Filter

## Filter Initialization (Window Method)

```c++ linenums="1"
// Initialize window function
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps.size(), 3.0));

// Initialize taps
// 0.2 is the lower cutoff frequency (normalized to Sample rate, frequency_Hz / samplerate_Hz)
// 0.45 is the upper cutoff frequency (normalized to Sample rate, frequency_Hz / samplerate_Hz)
// true means to normalize the gain at DC (0 Hz) to 1.0
univector<float, 7> taps;
fir_bandpass(taps, 0.2, 0.45, kaiser, true);

// Initialize filter and delay line
filter_fir<float> filter(taps);
```

KFR supports following filter types:

* Low-pass: [[fir_lowpass]]
* High-pass: [[fir_highpass]]
* Band-pass: [[fir_bandpass]]
* Band-stop: [[fir_bandstop]]

You can pass your own coefficients to the [[fir_filter]] constructor or use another method to calculate coefficients for the filter.

Use the [[filter<T>::apply]] function to apply the filter to audio.

All the internal state (delay line, etc.) is preserved between calls to apply. Use [[filter<T>::reset]] to reset the filter's internal state.

> [!note]
> Note that for a high-pass FIR filter, the number of taps must be odd.
> FIR filters with an even number of taps (Type II filters) always have a zero at z=−1 (Nyquist frequency) and cannot be used as high-pass filters, which require 1 at the Nyquist frequency.

## How to Apply an FIR Filter to a Plain Array (In-Place)

```c++ linenums="1"
float* data[256];
filter.apply(data); // apply in-place, size is determined automatically
```

## ... to a Plain Array

```c++ linenums="1"
float* output[256];
float* input[256]; // size must be the same
filter.apply(output, input); // read from input, apply, write to output, 
                            // size is determined automatically
```

## ... Using a Pointer and Size (In-Place)

```c++ linenums="1"
float* data;
size_t size;
filter.apply(data, size); // apply in-place, size is explicit
```

## ... Using Two Pointers and Size

```c++ linenums="1"
float* output;
float* input;
size_t size;
filter.apply(output, input, size); // read from input, apply, write to output, 
                                  // size is explicit
```

## ... to `univector` (In-Place)

```c++ linenums="1"
univector<float> data; // or univector<float, 1000>
filter.apply(data); // apply in-place, size is determined automatically
```

## ... to `univector`

```c++ linenums="1"
univector<float> output; // or univector<float, 1000>
univector<float> input; // or univector<float, 1000>
filter.apply(output, input); // size is determined automatically
```

## ... to Expression

```c++ linenums="1"
univector<float, 1000> output;
auto input = counter();
filter.apply(output, input);
```

[convolve_filter](convolution.md) is numerically equivalent to an FIR filter but uses DFT internally for better performance.

See also [Filter Class Definition](auto/classes/kfr.filter.t.md)

[See also a gallery with results of applying various FIR filters](fir_gallery.md)
