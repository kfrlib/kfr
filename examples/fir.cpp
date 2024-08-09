/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#ifdef HAVE_DFT
#include <kfr/dft.hpp>
#endif
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

// Define a macro to check if Python is installed
#ifndef PYTHON_IS_INSTALLED
#define PYTHON_IS_INSTALLED 1
#endif

int main()
{
    // Print the version of the KFR library being used
    println(library_version());

    // --------------------------------------------------------------------------------------
    // --------------------- FIR filter design (using window functions) ---------------------
    // --------------------------------------------------------------------------------------

    // Define options for plotting DSP data
    const std::string options = "phaseresp=True";

    // Define filter coefficient buffers of different sizes
    univector<fbase, 15> taps15;
    univector<fbase, 127> taps127;
    univector<fbase, 8191> taps8191;

    // Prepare window function expressions, data is not processed at this point
    // Hann window for 15-point filter
    expression_handle<fbase> hann = to_handle(window_hann(taps15.size()));
    // Kaiser window with alpha = 3.0 for 127-point filter
    expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));
    // Blackman-Harris window for 8191-point filter
    expression_handle<fbase> blackman_harris = to_handle(window_blackman_harris(taps8191.size()));

    // Design a 15-point lowpass FIR filter with a cutoff frequency of 0.15 using Hann window
    fir_lowpass(taps15, 0.15, hann, true);
#if PYTHON_IS_INSTALLED
    // Plot the filter, frequency, and impulse response
    // internally plot_save calls Python (matplotlib and numpy packages used) to save SVG files
    plot_save("fir_lowpass_hann", taps15,
              options + ", phasearg='auto', title='15-point lowpass FIR, Hann window'");
#endif

    // Design a 127-point lowpass FIR filter with a cutoff frequency of 0.2 using the Kaiser window
    // The resulting coefficients are in taps127
    fir_lowpass(taps127, 0.2, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot the filter, frequency, and impulse response
    plot_save("fir_lowpass_kaiser", taps127,
              options + ", phasearg='auto', title='127-point lowpass FIR, Kaiser window (\\alpha=3.0)'");
#endif

    // Design a 127-point highpass FIR filter with a cutoff frequency of 0.2 using the Kaiser window
    // The resulting coefficients are in taps127
    fir_highpass(taps127, 0.2, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot the filter, frequency, and impulse response
    plot_save("fir_highpass_kaiser", taps127,
              options + ", phasearg='auto', title='127-point highpass FIR, Kaiser window (\\alpha=3.0)'");
#endif

    // Design a 127-point bandpass FIR filter with cutoff frequencies of 0.2 and 0.4 using the Kaiser window
    fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot the filter, frequency, and impulse response
    plot_save("fir_bandpass_kaiser", taps127,
              options + ", phasearg='auto', title='127-point bandpass FIR, Kaiser window (\\alpha=3.0)'");
#endif

    // Design a 127-point bandstop FIR filter with cutoff frequencies of 0.2 and 0.4 using the Kaiser window
    fir_bandstop(taps127, 0.2, 0.4, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot the filter, frequency, and impulse response
    plot_save("fir_bandstop_kaiser", taps127,
              options + ", phasearg='auto', title='127-point bandstop FIR, Kaiser window (\\alpha=3.0)'");
#endif

    // Design an 8191-point lowpass FIR filter with a cutoff frequency of 0.15 using the Blackman-Harris
    // window
    fir_lowpass(taps8191, 0.15, blackman_harris, true);
    // Initialize a buffer that is 8191+150 samples long and set it to zero
    univector<fbase, 8191 + 150> taps8191_150 = scalar(0);

    // Shift the filter coefficients by 150 samples
    taps8191_150.slice(150) = taps8191;

#if PYTHON_IS_INSTALLED
    // Plot the filter, frequency, and impulse response
    // phasearg is used to get the correct phase shift (offset by 150 samples)
    plot_save(
        "fir_lowpass_blackman", taps8191_150,
        options +
            ", phasearg=4095+150, title='8191-point lowpass FIR, Blackman-Harris window', padwidth=16384");
#endif

    // --------------------------------------------------------------------------------------
    // -------------------------- Using FIR filter as an expression -------------------------
    // --------------------------------------------------------------------------------------

    // Generate 10,000 samples of white noise
    univector<float> noise = truncate(gen_random_range(random_init(1, 2, 3, 4), -1.f, +1.f), 10000);

    // Apply the bandstop filter designed earlier to the noise
    univector<float> filtered_noise = fir(noise, fir_params{ taps127 });

#if PYTHON_IS_INSTALLED
    // Plot the original noise and the filtered noise
    plot_save("noise", noise, "title='Original noise', div_by_N=True");
    plot_save("filtered_noise", filtered_noise, "title='Filtered noise', div_by_N=True");
#endif

    // --------------------------------------------------------------------------------------
    // ------------------------------- FIR filter as a class --------------------------------
    // --------------------------------------------------------------------------------------

    // Redesign the 127-point bandpass FIR filter with cutoff frequencies of 0.2 and 0.4 using the Kaiser
    // window
    fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
    // Initialize an FIR filter class with float input/output and fbase-typed taps
    filter_fir<fbase, float> fir_filter(taps127);

    // Apply the FIR filter to the noise data
    univector<float> filtered_noise2;
    fir_filter.apply(filtered_noise2, noise);

#if PYTHON_IS_INSTALLED
    // Plot the results of the filtered noise
    plot_save("filtered_noise2", filtered_noise2, "title='Filtered noise 2', div_by_N=True");
#endif

#ifdef HAVE_DFT
    // --------------------------------------------------------------------------------------
    // ---------------------- Convolution filter (optimized using DFT) ----------------------
    // --------------------------------------------------------------------------------------

    // Initialize a convolution filter with float input/output and fbase-typed taps
    convolve_filter<fbase> conv_filter(taps127);

    // Apply the convolution filter to the noise data
    univector<fbase> filtered_noise3;
    conv_filter.apply(filtered_noise3, noise * fbase(1.0));

#if PYTHON_IS_INSTALLED
    // Plot the results, same as filtered_noise2
    plot_save("filtered_noise3", filtered_noise3, "title='Filtered noise 3', div_by_N=True");
#endif
#endif

    println("SVG plots have been saved to svg directory");

    return 0;
}
