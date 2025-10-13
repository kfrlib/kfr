# How to Design and Apply IIR Filters with KFR

This guide explains how to design and apply Infinite Impulse Response (IIR) filters using the KFR library. IIR filters are powerful tools in digital signal processing, offering efficient frequency-selective filtering for applications like audio processing, noise reduction, and signal analysis. KFR supports several IIR filter types, including Butterworth, Chebyshev Type I, Chebyshev Type II, Bessel, and Elliptic filters, with functions to create low-pass, high-pass, bandpass, and bandstop filters.

## Overview of IIR Filters in KFR

KFR provides a robust framework for designing IIR filters in zero-pole-gain (ZPK) form and converting them to second-order sections (SOS) for stable implementation. The library includes functions to design filters with specific characteristics and apply them to signals using the `filter_iir` class.

### Supported IIR Filter Types

KFR supports the following IIR filter design functions, which mirror the behavior of their counterparts in SciPy:

- **Butterworth (`butterworth`)**: Maximally flat passband with a smooth frequency response.
- **Bessel (`bessel`)**: Linear phase response, minimizing phase distortion.
- **Chebyshev Type I (`chebyshev1`)**: Steep roll-off with passband ripple, controlled by the ripple parameter (dB).
- **Chebyshev Type II (`chebyshev2`)**: Steep roll-off with stopband ripple, controlled by the attenuation parameter (dB).
- **Elliptic (`elliptic`)**: Sharpest roll-off with ripple in both passband and stopband.

### Filter Design Functions

KFR provides functions to design filters in ZPK form based on frequency specifications:

- `iir_lowpass`: Designs a low-pass filter that attenuates frequencies above the cutoff.
- `iir_highpass`: Designs a high-pass filter that attenuates frequencies below the cutoff.
- `iir_bandpass`: Designs a bandpass filter that passes frequencies within a specified band.
- `iir_bandstop`: Designs a bandstop filter that attenuates frequencies within a specified band.

### Applying Filters

The `filter_iir` class applies the designed IIR filter to input data, maintaining internal state (e.g., delay lines) for continuous processing. Filters can be applied to arrays, `univector` objects, or expressions.

## Designing IIR Filters

To design an IIR filter, you first create a ZPK representation using one of the filter type functions (`butterworth`, `bessel`, `chebyshev1`, `chebyshev2`, or `elliptic`). Then, use the appropriate design function (`iir_lowpass`, `iir_highpass`, `iir_bandpass`, or `iir_bandstop`) to specify the frequency characteristics. Finally, convert the ZPK form to SOS for application.

### Example: Designing a Butterworth Low-Pass Filter

The following example designs a 4th-order Butterworth low-pass filter with a cutoff frequency of 500 Hz and a sampling rate of 48 kHz:

```c++
// Design a 4th-order Butterworth low-pass filter
zpk filt = iir_lowpass(butterworth(4), 500, 48000);

// Convert to second-order sections (SOS)
iir_params<float> params = to_sos<float>(filt);

// Initialize the filter
filter_iir<float> filter(params);
```

### Example: Designing a Chebyshev Type I Bandpass Filter

This example creates an 8th-order Chebyshev Type I bandpass filter with a passband from 1000 Hz to 2000 Hz, 2 dB passband ripple, and a sampling rate of 48 kHz:

```c++
// Design an 8th-order Chebyshev Type I bandpass filter
zpk filt = iir_bandpass(chebyshev1(8, 2), 1000, 2000, 48000);

// Convert to second-order sections
iir_params<float> params = to_sos<float>(filt);

// Initialize the filter
filter_iir<float> filter(params);
```

### Example: Designing a Bessel High-Pass Filter

This example designs a 6th-order Bessel high-pass filter with a cutoff frequency of 1000 Hz and a sampling rate of 48 kHz:

```c++
// Design a 6th-order Bessel high-pass filter
zpk filt = iir_highpass(bessel(6), 1000, 48000);

// Convert to second-order sections
iir_params<float> params = to_sos<float>(filt);

// Initialize the filter
filter_iir<float> filter(params);
```

### Notes on Elliptic Filters

> [!note]
> Elliptic filters require the Boost library.

```c++
// Design a 10th-order Elliptic low-pass filter
zpk filt = iir_lowpass(elliptic(10, 2, 60), 1000, 48000);

// Convert to second-order sections
iir_params<float> params = to_sos<float>(filt);

// Initialize the filter
filter_iir<float> filter(params);
```

## Applying IIR Filters

Once the filter is designed and initialized, you can apply it to input data using the `filter_iir` class. The `apply` function supports various input and output formats, including plain arrays, `univector` objects, and expressions.

### Applying to a Plain Array (In-Place)

```c++
float data[256];
// Initialize filter as shown above
filter.apply(data); // Apply filter in-place
```

### Applying to a Plain Array (Input/Output)

```c++
float input[256];
float output[256];
// Initialize filter as shown above
filter.apply(output, input); // Read from input, write to output
```

### Applying to a `univector` (In-Place)

```c++
univector<float> data(256);
// Initialize filter as shown above
filter.apply(data); // Apply filter in-place
```

### Applying to a `univector` (Input/Output)

```c++
univector<float> input(256);
univector<float> output(256);
// Initialize filter as shown above
filter.apply(output, input); // Read from input, write to output
```

### Applying to an Expression

```c++
univector<float, 256> output;
auto input = counter(); // Example expression
// Initialize filter as shown above
filter.apply(output, input); // Apply to expression
```

## Zero-Phase Filtering with `filtfilt`

For applications requiring minimal phase distortion, use the `filtfilt` function to apply the filter in both forward and backward directions:

```c++
// Design a 4th-order Butterworth low-pass filter
zpk filt = iir_lowpass(butterworth(4), 500, 48000);

// Convert to second-order sections
iir_params<float> params = to_sos<float>(filt);

// Apply zero-phase filtering to a signal
univector<float> data(1024);
data[512] = 1; // Unit impulse
filtfilt(data, params); // Apply forward-backward filtering in-place
```

## Cascading Multiple Filters

To create higher-order filters, KFR automatically cascades multiple second-order sections when converting from ZPK to SOS. The number of sections is limited by `maximum_iir_order / 2` (default: 64 sections, as `maximum_iir_order` is 128). Ensure your filter order does not exceed this limit.

Example of a high-order filter:

```c++
// Design a 24th-order Butterworth low-pass filter
zpk filt = iir_lowpass(butterworth(24), 1000, 48000);

// Convert to second-order sections
iir_params<float> params = to_sos<float>(filt);

// Initialize the filter
filter_iir<float> filter(params);

// Apply to data
univector<float> data(1024);
filter.apply(data);
```

## Visualizing Filter Response

To analyze the filter's frequency response, apply the filter to a unit impulse and use KFR's `plot_save` function to generate plots. 

> [!note]
> Ensure python with numpy, scipy and matplotlib is installed.

Example:

```c++
// Design a 12th-order Bessel low-pass filter
zpk filt = iir_lowpass(bessel(12), 1000, 48000);

// Convert to second-order sections
iir_params<float> params = to_sos<float>(filt);

// Apply to unit impulse
univector<float, 1024> output = iir(unitimpulse(), filt);

// Save frequency response plot
const std::string options = "phaseresp=True, log_freq=True, freq_dB_lim=(-160, 10), padwidth=8192";
plot_save("bessel_lowpass12", output, options + ", title='12th-order Bessel filter, lowpass 1 kHz'");
```

## Best Practices

1. **Reuse `iir_params`**: Converting ZPK to SOS (`to_sos`) is computationally expensive. Store and reuse `iir_params` if applying the same filter multiple times.
2. **Reset Filter State**: Use `filter.reset()` to clear the filter's internal state (e.g., delay lines) when processing new, unrelated signals.
4. **Frequency Normalization**: For bandpass and bandstop filters, frequencies are typically normalized (frequency / sampling rate). Ensure frequencies are in the correct range (0 to 0.5 for normalized frequencies).
5. **Order Limits**: Keep filter orders reasonable to avoid numerical instability and respect the `maximum_iir_order` limit (default: 128).

## See Also

- [Biquad Filter Documentation](bq.md)
- [Filter Class Definition](auto/filter.md)
- [Gallery with results of applying various IIR filters](iir_gallery.md)
