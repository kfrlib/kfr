# How to Apply a Biquad Filter

## Filter Initialization

Biquad filters, also known as second-order section (SOS) filters, are widely used in digital signal processing for their versatility and efficiency. The KFR library provides functions to design and apply biquad filters with various frequency response characteristics.

To initialize a biquad filter, you first create a `biquad_section` to hold the filter coefficients, then use `iir_params` to configure the filter, and finally instantiate a `filter_iir` object to apply the filter.

```c++
// Initialize biquad section for a low-pass filter
// 0.2 is the normalized cutoff frequency (frequency_Hz / samplerate_Hz)
// 0.707 is the Q factor
biquad_section<float> bq = biquad_lowpass<float>(0.2, 0.707);

// Initialize filter parameters
iir_params<float> params(bq);

// Initialize filter
filter_iir<float> filter(params);
```

### Supported Biquad Filter Types

KFR supports the following biquad filter types, each with specific parameters:

- **Low-pass**: `biquad_lowpass(frequency, Q)`  
  Attenuates frequencies above the cutoff. Parameters: normalized frequency, Q factor.
- **High-pass**: `biquad_highpass(frequency, Q)`  
  Attenuates frequencies below the cutoff. Parameters: normalized frequency, Q factor.
- **Band-pass**: `biquad_bandpass(frequency, Q)`  
  Passes frequencies within a band. Parameters: normalized frequency, Q factor.
- **Notch**: `biquad_notch(frequency, Q)`  
  Attenuates frequencies around the center frequency. Parameters: normalized frequency, Q factor.
- **Peak**: `biquad_peak(frequency, Q, gain)`  
  Boosts or cuts frequencies around the center frequency. Parameters: normalized frequency, Q factor, gain (dB).
- **Low-shelf**: `biquad_lowshelf(frequency, gain)`  
  Boosts or cuts frequencies below the cutoff. Parameters: normalized frequency, gain (dB).
- **High-shelf**: `biquad_highshelf(frequency, gain)`  
  Boosts or cuts frequencies above the cutoff. Parameters: normalized frequency, gain (dB).

You can pass your own coefficients to the `biquad_section` constructor or use the provided functions to calculate coefficients based on the desired filter characteristics.

### Applying the Filter

Use the `apply` function to process audio data with the biquad filter. The filter maintains its internal state (e.g., delay lines) between calls to `apply`. Use `reset` to clear the filter's internal state if needed.

## How to Apply a Biquad Filter to a Plain Array (In-Place)

```c++
float data[256];
filter.apply(data); // Apply in-place, size is determined automatically
```

## ... to a Plain Array

```c++
float output[256];
float input[256]; // Size must be the same
filter.apply(output, input); // Read from input, apply, write to output
```

## ... Using a Pointer and Size (In-Place)

```c++
float* data;
size_t size;
filter.apply(data, size); // Apply in-place, size is explicit
```

## ... Using Two Pointers and Size

```c++
float* output;
float* input;
size_t size;
filter.apply(output, input, size); // Read from input, apply, write to output
```

## ... to `univector` (In-Place)

```c++
univector<float> data; // or univector<float, 1000>
filter.apply(data); // Apply in-place, size is determined automatically
```

## ... to `univector`

```c++
univector<float> output; // or univector<float, 1000>
univector<float> input; // or univector<float, 1000>
filter.apply(output, input); // Size is determined automatically
```

## ... to Expression

```c++
univector<float, 1000> output;
auto input = counter();
filter.apply(output, input);
```

## Multiple Biquad Sections

For more complex filtering, you can cascade multiple biquad sections by passing an array of `biquad_section` objects to `iir_params`. The following example creates a filter with two cascaded low-pass sections:

```c++
// Initialize multiple biquad sections
std::vector<biquad_section<float>> sections = {
    biquad_lowpass<float>(0.2, 0.707),
    biquad_lowpass<float>(0.3, 0.707)
};

// Initialize filter parameters
iir_params<float> params(sections);

// Initialize filter
filter_iir<float> filter(params);
```

> [!note]
> The number of biquad sections is limited to `maximum_biquad_count` (defined as `maximum_iir_order / 2` in `biquad.hpp`). Ensure the number of sections does not exceed this limit.
> Currently, `maximum_iir_order` is set to 128, allowing up to 64 biquad sections.

## Zero-Phase Filtering

To achieve zero-phase filtering (minimal phase distortion), use the `filtfilt` function, which applies the filter in both forward and backward directions:

```c++
univector<float> data;
iir_params<float> params(biquad_lowpass<float>(0.2, 0.707));
filtfilt(data, params); // Apply forward and backward filtering in-place
```

## See Also

- [Filter Class Definition](auto/classes/kfr.filter.t.md)
- [FIR Filter Documentation](fir.md)
- [Gallery with results of applying various Biquad filters](bq_gallery.md)
