/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main()
{
    // Print the version of the KFR library being used
    println(library_version());

    // Define options for plotting DSP data
    const std::string options = "phaseresp=True, log_freq=True, freq_dB_lim=(-160, 10), padwidth=8192";

    // Define an output univector with 1024 elements
    univector<fbase, 1024> output;

    // --------------------------------------------------------------------------------------
    // ---------------------------- 24th-Order Bessel Lowpass Filter ------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 24th-order Bessel lowpass filter with a cutoff frequency of 1 kHz and a sample rate of
        // 48 kHz
        zpk<fbase> filt = iir_lowpass(bessel<fbase>(24), 1000, 48000);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), filt);
    }
    plot_save("bessel_lowpass24", output, options + ", title='24th-order Bessel filter, lowpass 1 kHz'");

    // --------------------------------------------------------------------------------------
    // ---------------------------- 12th-Order Bessel Lowpass Filter ------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 12th-order Bessel lowpass filter with a cutoff frequency of 1 kHz and a sample rate of
        // 48 kHz
        zpk<fbase> filt = iir_lowpass(bessel<fbase>(12), 1000, 48000);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), filt);
    }
    plot_save("bessel_lowpass12", output, options + ", title='12th-order Bessel filter, lowpass 1 kHz'");

    // --------------------------------------------------------------------------------------
    // ----------------------------- 6th-Order Bessel Lowpass Filter ------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 6th-order Bessel lowpass filter with a cutoff frequency of 1 kHz and a sample rate of
        // 48 kHz
        zpk<fbase> filt = iir_lowpass(bessel<fbase>(6), 1000, 48000);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), filt);
    }
    plot_save("bessel_lowpass6", output, options + ", title='6th-order Bessel filter, lowpass 1 kHz'");

    // --------------------------------------------------------------------------------------
    // ------------------------ 24th-Order Butterworth Lowpass Filter -----------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 24th-order Butterworth lowpass filter with a cutoff frequency of 1 kHz and a sample rate
        // of 48 kHz
        zpk<fbase> filt = iir_lowpass(butterworth<fbase>(24), 1000, 48000);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), filt);
    }
    plot_save("butterworth_lowpass24", output,
              options + ", title='24th-order Butterworth filter, lowpass 1 kHz'");

    // --------------------------------------------------------------------------------------
    // ------------------------ 12th-Order Butterworth Lowpass Filter -----------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 12th-order Butterworth lowpass filter with a cutoff frequency of 1 kHz and a sample rate
        // of 48 kHz
        zpk<fbase> filt = iir_lowpass(butterworth<fbase>(12), 1000, 48000);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), filt);
    }
    plot_save("butterworth_lowpass12", output,
              options + ", title='12th-order Butterworth filter, lowpass 1 kHz'");

    // --------------------------------------------------------------------------------------
    // ------------------------ 12th-Order Butterworth Highpass Filter ----------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 12th-order Butterworth highpass filter with a cutoff frequency of 1 kHz and a sample rate
        // of 48 kHz
        zpk<fbase> filt = iir_highpass(butterworth<fbase>(12), 1000, 48000);

        // Convert the filter to second-order sections (SOS).
        // This is an expensive operation, so keep 'iir_params' if it is reused later
        iir_params<fbase> bqs = to_sos(filt);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), bqs);
    }
    plot_save("butterworth_highpass12", output,
              options + ", title='12th-order Butterworth filter, highpass 1 kHz'");

    // --------------------------------------------------------------------------------------
    // ---------------------- 12th-Order Butterworth Bandpass Filter ------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 12th-order Butterworth bandpass filter with a passband from 0.1 to 0.2 (normalized
        // frequency)
        zpk<fbase> filt = iir_bandpass(butterworth<fbase>(12), 0.1, 0.2);

        // Convert the filter to second-order sections (SOS).
        // This is an expensive operation, so keep 'iir_params' if it is reused later
        iir_params<fbase> bqs = to_sos(filt);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), bqs);
    }
    plot_save("butterworth_bandpass12", output,
              options + ", title='12th-order Butterworth filter, bandpass'");

    // --------------------------------------------------------------------------------------
    // ---------------------- 12th-Order Butterworth Bandstop Filter ------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 12th-order Butterworth bandstop filter with a stopband from 0.1 to 0.2 (normalized
        // frequency)
        zpk<fbase> filt = iir_bandstop(butterworth<fbase>(12), 0.1, 0.2);

        // Convert the filter to second-order sections (SOS).
        // This is an expensive operation, so keep 'iir_params' if it is reused later
        iir_params<fbase> bqs = to_sos(filt);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), bqs);
    }
    plot_save("butterworth_bandstop12", output,
              options + ", title='12th-order Butterworth filter, bandstop'");

    // --------------------------------------------------------------------------------------
    // ------------------------ 4th-Order Butterworth Bandpass Filter -----------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a 4th-order Butterworth bandpass filter with a passband from 0.005 to 0.9 (normalized
        // frequency)
        zpk<fbase> filt = iir_bandpass(butterworth<fbase>(4), 0.005, 0.9);

        // Convert the filter to second-order sections (SOS).
        // This is an expensive operation, so keep 'iir_params' if it is reused later
        iir_params<fbase> bqs = to_sos(filt);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), bqs);
    }
    plot_save("butterworth_bandpass4", output, options + ", title='4th-order Butterworth filter, bandpass'");

    // --------------------------------------------------------------------------------------
    // ------------------- 8th-Order Chebyshev Type I Lowpass Filter ------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create an 8th-order Chebyshev Type I lowpass filter with a cutoff frequency of 0.09 (normalized
        // frequency) and 2 dB ripple in the passband
        zpk<fbase> filt = iir_lowpass(chebyshev1<fbase>(8, 2), 0.09);

        // Convert the filter to second-order sections (SOS).
        // This is an expensive operation, so keep 'iir_params' if it is reused later
        iir_params<fbase> bqs = to_sos(filt);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), bqs);
    }
    plot_save("chebyshev1_lowpass8", output,
              options + ", title='8th-order Chebyshev Type I filter, lowpass'");

    // --------------------------------------------------------------------------------------
    // ------------------- 8th-Order Chebyshev Type II Lowpass Filter -----------------------
    // --------------------------------------------------------------------------------------
    {
        // Create an 8th-order Chebyshev Type II lowpass filter with a cutoff frequency of 0.09 (normalized
        // frequency) and 80 dB attenuation in the stopband
        zpk<fbase> filt = iir_lowpass(chebyshev2<fbase>(8, 80), 0.09);

        // Convert the filter to second-order sections (SOS).
        // This is an expensive operation, so keep 'iir_params' if it is reused later
        iir_params<fbase> bqs = to_sos(filt);

        // Apply the filter to a unit impulse signal to get the filter's impulse response
        output = iir(unitimpulse(), filt);
    }
    plot_save("chebyshev2_lowpass8", output,
              options + ", title='8th-order Chebyshev type II filter, lowpass'");

    println("SVG plots have been saved to svg directory");

    return 0;
}
