# FIR filters code & examples

The examples below use [expression handles](../../expressions/handle.md) (`expression_handle` / `to_handle`)
to pass window functions to the FIR design routines.

## Bandpass, 127, Kaiser 

Code
```c++ linenums="1"
|||#include <kfr/dsp.hpp>
|||using namespace kfr;
|||
|||TEST_CASE("dsp/examples/fir_gallery.md/bandpass Kaiser")
|||{
univector<fbase, 127> taps127;
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));

// Fill taps127 with the band pass FIR filter coefficients using kaiser window and cutoff=0.2 and 0.4
fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
// Plot filter, frequency and impulse response
|||CHECK(taps127.size() == 127);
|||CHECK(taps127[63] > fbase(0));
||||||||||||
plot_save("fir_bandpass_kaiser", taps127,
            options + ", phasearg='auto', title=r'127-point bandpass FIR, Kaiser window ($\\alpha=3.0$)'");
||||||||||||
|||}
```
Result

![fir_bandpass_kaiser.svg](../../img/fir_bandpass_kaiser.svg)

## Bandstop, 127, Kaiser

Code
```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/bandstop Kaiser")
|||{
univector<fbase, 127> taps127;
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));

// Fill taps127 with the band stop FIR filter coefficients using kaiser window and cutoff=0.2 and 0.4
fir_bandstop(taps127, 0.2, 0.4, kaiser, true);
// Show filter, frequency and impulse response
|||CHECK(taps127.size() == 127);
|||CHECK(taps127[63] > fbase(0));
||||||||||||
plot_save("fir_bandstop_kaiser", taps127,
            options + ", phasearg='auto', title=r'127-point bandstop FIR, Kaiser window ($\\alpha=3.0$)'");
||||||||||||
|||}
```
Result

![fir_bandstop_kaiser.svg](../../img/fir_bandstop_kaiser.svg)

## Highpass, 127, Kaiser

Code
```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/highpass Kaiser")
|||{
univector<fbase, 127> taps127;
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));

// Fill taps127 with the high pass FIR filter coefficients using kaiser window and cutoff=0.2
fir_highpass(taps127, 0.2, kaiser, true);
// Plot filter, frequency and impulse response
|||CHECK(taps127.size() == 127);
|||CHECK(taps127[63] > fbase(0));
||||||||||||
plot_save("fir_highpass_kaiser", taps127,
            options + ", phasearg='auto', title=r'127-point highpass FIR, Kaiser window ($\\alpha=3.0$)'");
||||||||||||
|||}
```
Result

![fir_highpass_kaiser.svg](../../img/fir_highpass_kaiser.svg)

## Lowpass, 8192, Blackman-Harris

Code
```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/long lowpass Blackman-Harris")
|||{
univector<fbase, 8191> taps8191;
expression_handle<fbase> blackman_harris = to_handle(window_blackman_harris(taps8191.size()));

// Fill taps8191 with the low pass FIR filter coefficients using blackman harris window and cutoff=0.15
fir_lowpass(taps8191, 0.15, blackman_harris, true);

// Plot filter, frequency and impulse response, pass phasearg to get correct phase shift (phasearg=offset
// to unit impulse in samples)
|||CHECK(taps8191.size() == 8191);
|||CHECK(taps8191[4095] > fbase(0));
||||||||||||
plot_save(
    "fir_lowpass_blackman", taps8191,
    options +
        ", title='8191-point lowpass FIR, Blackman-Harris window', padwidth=16384");
||||||||||||
|||}
```
Result

![fir_lowpass_blackman.svg](../../img/fir_lowpass_blackman.svg)

## Lowpass, 15, Hann

Code
```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/short lowpass Hann")
|||{
univector<fbase, 15> taps15;
expression_handle<fbase> hann = to_handle(window_hann(taps15.size()));

// Fill taps15 with the low pass FIR filter coefficients using hann window and cutoff=0.15
fir_lowpass(taps15, 0.15, hann, true);
// Plot filter, frequency and impulse response
// plot_save calls python (matplotlib and numpy must be installed) and saves SVG file
|||CHECK(taps15.size() == 15);
|||CHECK(taps15[7] > fbase(0));
||||||||||||
plot_save("fir_lowpass_hann", taps15,
            options + ", phasearg='auto', title='15-point lowpass FIR, Hann window'");
||||||||||||
|||}
```
Result
![fir_lowpass_hann.svg](../../img/fir_lowpass_hann.svg)

## Lowpass, 127, Kaiser

Code

```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/lowpass Kaiser")
|||{
univector<fbase, 127> taps127;
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));

// Fill taps127 with the low pass FIR filter coefficients using kaiser window and cutoff=0.2
fir_lowpass(taps127, 0.2, kaiser, true);
// Plot filter, frequency and impulse response
|||CHECK(taps127.size() == 127);
|||CHECK(taps127[63] > fbase(0));
||||||||||||
plot_save("fir_lowpass_kaiser", taps127,
            options + ", phasearg='auto', title=r'127-point lowpass FIR, Kaiser window ($\\alpha=3.0$)'");
||||||||||||
|||}
```
Result
![fir_lowpass_kaiser.svg](../../img/fir_lowpass_kaiser.svg)

## Bandstop, 127, Kaiser (using `filter_fir<>`)

```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/bandstop filter_fir")
|||{
univector<fbase, 127> taps127;
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));
fir_bandstop(taps127, 0.2, 0.4, kaiser, true);
// Initialize FIR filter with float input/output and fbase taps
filter_fir<fbase, float> fir_filter(taps127);

// Apply to univector, static array, data by pointer or anything
|||univector<float> noise = truncate(gen_random_range<float>(-1.0f, 1.0f), 512);
univector<float> filtered_noise;
fir_filter.apply(filtered_noise, noise);

// Plot results
|||CHECK(filtered_noise.size() == noise.size());
||||||||||||
plot_save("filtered_noise", filtered_noise, "title='Filtered noise', div_by_N=True");
||||||||||||
|||}
```

Result
![filtered_noise.svg](../../img/filtered_noise.svg)

## Bandpass, 127, Kaiser (using `filter_fir<>`)

```c++ linenums="1"
|||TEST_CASE("dsp/examples/fir_gallery.md/bandpass filter_fir")
|||{
univector<fbase, 127> taps127;
expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));
fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
// Initialize FIR filter with float input/output and fbase taps
filter_fir<fbase, float> fir_filter(taps127);

// Apply to univector, static array, data by pointer or anything
|||univector<float> noise = truncate(gen_random_range<float>(-1.0f, 1.0f), 512);
univector<float> filtered_noise2;
fir_filter.apply(filtered_noise2, noise);

// Plot results
|||CHECK(filtered_noise2.size() == noise.size());
||||||||||||
plot_save("filtered_noise2", filtered_noise2, "title='Filtered noise 2', div_by_N=True");
||||||||||||
|||}
```

Result
![filtered_noise.svg](../../img/filtered_noise2.svg)
