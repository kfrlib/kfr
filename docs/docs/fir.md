# How to apply a FIR filter

## Filter initialization (window method)

```c++ linenums="1"
// Initialize window function
expression_pointer<fbase> kaiser = to_pointer(window_kaiser(taps.size(), 3.0));

// Initialize taps
univector<float, 7> taps;
fir_bandpass(taps, 0.2, 0.45, kaiser, true);

// Initialize filter and delay line
filter_fir<float> filter(taps);
```

You can pass your own coefficients to `filter_fir` constructor or use another method to calculate coefficients for the filter.

Use `apply` function to apply the filter to audio.

All the internal state (delay line etc) is preserved between calls to apply. Use `reset` to reset filter internal state.

!!! note
    Note that for high pass FIR filter the number of taps must be odd.
    FIR filters with even number of taps (Type II filters) always have a zero at z=−1 (Nyquist frequency) and can't be used as high pass filters which require 1 at the Nyquist frequency.

## How to apply a FIR filter to a plain array (inplace)

```c++ linenums="1"
float* data[256];
filter.apply(data); // apply inplace, size is determined automatically
```

## ... to a plain array

```c++ linenums="1"
float* output[256];
float* input[256]; // size must be same
filter.apply(output, inplace); // read from input, apply, write to output, 
                               // size is determined automatically
```

## ... using a pointer and size (inplace)

```c++ linenums="1"
float* data;
size_t size;
filter.apply(data, size); // apply inplace, size is explicit
```

## ... using two pointers and size

```c++ linenums="1"
float* output;
float* input;
size_t size;
filter.apply(output, input, size); // read from input, apply, write to output, 
                                   // size is explicit
```

## ... to `univector` (inplace)

```c++ linenums="1"
univector<float> data; // or univector<float, 1000>
filter.apply(data); // apply inplace, size is determined automatically
```

## ... to `univector`

```c++ linenums="1"
univector<float> output; // or univector<float, 1000>
univector<float> input; // or univector<float, 1000>
filter.apply(output, input); // size is determined automatically
```

## .. to expression

```c++ linenums="1"
univector<float, 1000> output;
auto input = counter();
filter.apply(output, input);
```

[convolve_filter](convolution.md) is numerically equal to FIR filter but uses DFT internally for better performance.

See also [Filter class definition](auto/filter.md)

[See also a gallery with results of applying various FIR filters](fir_gallery.md)