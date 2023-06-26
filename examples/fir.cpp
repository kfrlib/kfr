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

#ifndef PYTHON_IS_INSTALLED
#define PYTHON_IS_INSTALLED 1
#endif

int main()
{
    // Show KFR version
    println(library_version());

    // --------------------------------------------------------------------------------------
    // --------------------- FIR filter design (using window functions) ---------------------
    // --------------------------------------------------------------------------------------

    // Options for dspplot
    const std::string options = "phaseresp=True";

    univector<fbase, 15> taps15;
    univector<fbase, 127> taps127;
    univector<fbase, 8191> taps8191;

    // Prepare window functions (only expression saved here, not data)
    expression_handle<fbase> hann = to_handle(window_hann(taps15.size()));

    expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));

    expression_handle<fbase> blackman_harris = to_handle(window_blackman_harris(taps8191.size()));

    // Fill taps15 with the low pass FIR filter coefficients using hann window and cutoff=0.15
    fir_lowpass(taps15, 0.15, hann, true);
#if PYTHON_IS_INSTALLED
    // Plot filter, frequency and impulse response
    // plot_save calls python (matplotlib and numpy must be installed) and saves SVG file
    plot_save("fir_lowpass_hann", taps15,
              options + ", phasearg='auto', title='15-point lowpass FIR, Hann window'");
#endif

    // Fill taps127 with the low pass FIR filter coefficients using kaiser window and cutoff=0.2
    fir_lowpass(taps127, 0.2, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot filter, frequency and impulse response
    plot_save("fir_lowpass_kaiser", taps127,
              options + ", phasearg='auto', title=r'127-point lowpass FIR, Kaiser window ($\\alpha=3.0$)'");
#endif

    // Fill taps127 with the high pass FIR filter coefficients using kaiser window and cutoff=0.2
    fir_highpass(taps127, 0.2, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot filter, frequency and impulse response
    plot_save("fir_highpass_kaiser", taps127,
              options + ", phasearg='auto', title=r'127-point highpass FIR, Kaiser window ($\\alpha=3.0$)'");
#endif

    // Fill taps127 with the band pass FIR filter coefficients using kaiser window and cutoff=0.2 and 0.4
    fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Plot filter, frequency and impulse response
    plot_save("fir_bandpass_kaiser", taps127,
              options + ", phasearg='auto', title=r'127-point bandpass FIR, Kaiser window ($\\alpha=3.0$)'");
#endif

    // Fill taps127 with the band stop FIR filter coefficients using kaiser window and cutoff=0.2 and 0.4
    fir_bandstop(taps127, 0.2, 0.4, kaiser, true);
#if PYTHON_IS_INSTALLED
    // Show filter, frequency and impulse response
    plot_save("fir_bandstop_kaiser", taps127,
              options + ", phasearg='auto', title=r'127-point bandstop FIR, Kaiser window ($\\alpha=3.0$)'");
#endif

    // Fill taps8191 with the low pass FIR filter coefficients using blackman harris window and cutoff=0.15
    fir_lowpass(taps8191, 0.15, blackman_harris, true);
    univector<fbase, 8191 + 150> taps8191_150 = scalar(0);

    // Shift by 150 samples
    taps8191_150.slice(150) = taps8191;

#if PYTHON_IS_INSTALLED
    // Plot filter, frequency and impulse response, pass phasearg to get correct phase shift (phasearg=offset
    // to unit impulse in samples)
    plot_save(
        "fir_lowpass_blackman", taps8191_150,
        options +
            ", phasearg=4095+150, title='8191-point lowpass FIR, Blackman-Harris window', padwidth=16384");
#endif

    // --------------------------------------------------------------------------------------
    // -------------------------- Using FIR filter as an expression -------------------------
    // --------------------------------------------------------------------------------------

    // Prepare 10000 samples of white noise
    univector<float> noise = truncate(gen_random_range(random_init(1, 2, 3, 4), -1.f, +1.f), 10000);

    // Apply band stop filter
    univector<float> filtered_noise = fir(noise, taps127);

#if PYTHON_IS_INSTALLED
    // Plot results
    plot_save("noise", noise, "title='Original noise', div_by_N=True");
    plot_save("filtered_noise", filtered_noise, "title='Filtered noise', div_by_N=True");
#endif

    // --------------------------------------------------------------------------------------
    // ------------------------------- FIR filter as a class --------------------------------
    // --------------------------------------------------------------------------------------

    fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
    // Initialize FIR filter with float input/output and fbase taps
    filter_fir<fbase, float> fir_filter(taps127);

    // Apply to univector, static array, data by pointer or anything
    univector<float> filtered_noise2;
    fir_filter.apply(filtered_noise2, noise);

#if PYTHON_IS_INSTALLED
    // Plot results
    plot_save("filtered_noise2", filtered_noise2, "title='Filtered noise 2', div_by_N=True");
#endif

#ifdef HAVE_DFT
    // --------------------------------------------------------------------------------------
    // ---------------------- Convolution filter (optimized using DFT) ----------------------
    // --------------------------------------------------------------------------------------

    // Initialize FIR filter with float input/output and fbase taps
    convolve_filter<fbase> conv_filter(taps127);

    // Apply to univector, static array, data by pointer or anything
    univector<fbase> filtered_noise3;
    conv_filter.apply(filtered_noise3, noise * fbase(1.0));

#if PYTHON_IS_INSTALLED
    // Plot results, same as filtered_noise2
    plot_save("filtered_noise3", filtered_noise3, "title='Filtered noise 3', div_by_N=True");
#endif
#endif

    println("SVG plots have been saved to svg directory");

    return 0;
}
