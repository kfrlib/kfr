/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp/biquad.hpp>
#include <kfr/dsp/biquad_design.hpp>
#include <kfr/dsp/special.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main()
{
    // Print the version of the KFR library being used
    println(library_version());

    // Define options for plotting DSP data
    const std::string options = "phaseresp=True";

    // Define a buffer for storing the filter output
    univector<fbase, 128> output;

    // --------------------------------------------------------------------------------------
    // ------------------------- Biquad Notch Filters Example -------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize an array of biquad notch filters with different center frequencies
        // biquad_notch(frequency, Q) where the frequency is relative to the sample rate
        biquad_section<fbase> bq[] = {
            biquad_notch(0.1, 0.5),
            biquad_notch(0.2, 0.5),
            biquad_notch(0.3, 0.5),
            biquad_notch(0.4, 0.5),
        };
        // Apply the biquad filters to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter responses
    plot_save("biquad_notch", output, options + ", title='Four Biquad Notch filters'");

    // --------------------------------------------------------------------------------------
    // ------------------------- Biquad Lowpass Filter Example ------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad lowpass filter with specific parameters
        // biquad_lowpass(frequency, Q) where the frequency is relative to the sample rate
        biquad_section<fbase> bq[] = { biquad_lowpass(0.2, 0.9) };
        // Apply the biquad lowpass filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_lowpass", output, options + ", title='Biquad Low pass filter (0.2, 0.9)'");

    // --------------------------------------------------------------------------------------
    // ------------------------- Biquad Highpass Filter Example -----------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad highpass filter with specific parameters
        // biquad_highpass(frequency, Q) where the frequency is relative to the sample rate
        biquad_section<fbase> bq[] = { biquad_highpass(0.3, 0.1) };
        // Apply the biquad highpass filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_highpass", output, options + ", title='Biquad High pass filter (0.3, 0.1)'");

    // --------------------------------------------------------------------------------------
    // -------------------------- Biquad Peak Filter Example --------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad peak filter with specific parameters
        // biquad_peak(frequency, Q, gain) where the frequency is relative to the sample rate and the gain is
        // in decibels
        biquad_section<fbase> bq[] = { biquad_peak(0.3, 0.5, +9.0) };
        // Apply the biquad peak filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_peak", output, options + ", title='Biquad Peak filter (0.2, 0.5, +9)'");

    // --------------------------------------------------------------------------------------
    // -------------------------- Biquad Peak Filter Example (2) ----------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize another biquad peak filter with different parameters
        // biquad_peak(frequency, Q, gain) where the frequency is relative to the sample rate and the gain is
        // in decibels
        biquad_section<fbase> bq[] = { biquad_peak(0.3, 3.0, -2.0) };
        // Apply the biquad peak filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_peak2", output, options + ", title='Biquad Peak filter (0.3, 3, -2)'");

    // --------------------------------------------------------------------------------------
    // -------------------------- Biquad Low Shelf Filter Example ---------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad low shelf filter with specific parameters
        // biquad_lowshelf(frequency, gain) where the frequency is relative to the sample rate and the gain is
        // in decibels
        biquad_section<fbase> bq[] = { biquad_lowshelf(0.3, -1.0) };
        // Apply the biquad low shelf filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_lowshelf", output, options + ", title='Biquad low shelf filter (0.3, -1)'");

    // --------------------------------------------------------------------------------------
    // -------------------------- Biquad High Shelf Filter Example --------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad high shelf filter with specific parameters
        // biquad_highshelf(frequency, gain) where the frequency is relative to the sample rate and the gain
        // is in decibels
        biquad_section<fbase> bq[] = { biquad_highshelf(0.3, +9.0) };
        // Apply the biquad high shelf filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_highshelf", output, options + ", title='Biquad high shelf filter (0.3, +9)'");

    // --------------------------------------------------------------------------------------
    // ------------------------- Biquad Bandpass Filter Example -----------------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad bandpass filter with specific parameters
        // biquad_bandpass(frequency, Q) where the frequency is relative to the sample rate
        biquad_section<fbase> bq[] = { biquad_bandpass(0.25, 0.2) };
        // Apply the biquad bandpass filter to a unit impulse signal and store the result in 'output'
        output = iir(unitimpulse(), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_bandpass", output, options + ", title='Biquad band pass (0.25, 0.2)'");

    // --------------------------------------------------------------------------------------
    // ----------------- Biquad Bandpass Filter Example with std::vector --------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad bandpass filter with specific parameters
        biquad_section<fbase> bq[] = { biquad_bandpass(0.25, 0.2) };
        // Create a std::vector for the input data and set the first element to 1 (unit impulse)
        std::vector<fbase> data(output.size(), 0.f);
        data[0] = 1.f;
        // Apply the biquad bandpass filter to the input data and store the result in 'output'
        output = iir(make_univector(data), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_bandpass_stdvector", output, options + ", title='Biquad band pass (0.25, 0.2)'");

    // --------------------------------------------------------------------------------------
    // ------------------- Biquad Bandpass Filter Example with C array ----------------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad bandpass filter with specific parameters
        biquad_section<fbase> bq[] = { biquad_bandpass(0.25, 0.2) };
        // Create a C array for the input data and set the first element to 1 (unit impulse)
        fbase data[output.size()] = { 0 }; // .size() is constexpr
        data[0]                   = 1.f;
        // Apply the biquad bandpass filter to the input data and store the result in 'output'
        output = iir(make_univector(data), iir_params{ bq });
    }
    // Save the plot of the filter response
    plot_save("biquad_bandpass_carray", output, options + ", title='Biquad band pass (0.25, 0.2)'");

    // --------------------------------------------------------------------------------------
    // ----------------- Custom Biquad Lowpass Filter using expression_filter ---------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad lowpass filter with specific parameters
        biquad_section<fbase> bq[] = { biquad_lowpass(0.2, 0.9) };
        // Create a type-erased expression filter for the biquad lowpass filter
        expression_filter<fbase> filter = to_filter(iir(placeholder<fbase>(), iir_params{ bq }));

        // Prepare a unit impulse signal
        output = unitimpulse();

        // Apply the expression filter to the unit impulse signal
        filter.apply(output);
    }
    // Save the plot of the filter response
    plot_save("biquad_custom_filter_lowpass", output,
              options + ", title='Biquad Low pass filter (0.2, 0.9) (using expression_filter)'");

    // --------------------------------------------------------------------------------------
    // ---------------------- Custom Biquad Lowpass Filter using iir_filter -----------------
    // --------------------------------------------------------------------------------------
    {
        // Initialize a biquad lowpass filter with specific parameters
        biquad_section<fbase> bq[] = { biquad_lowpass(0.2, 0.9) };
        // Create an IIR filter for the biquad lowpass filter
        iir_filter<fbase> filter(bq);

        // Prepare a unit impulse signal
        output = unitimpulse();

        // Apply the IIR filter to the unit impulse signal
        filter.apply(output);
    }
    // Save the plot of the filter response
    plot_save("biquad_filter_lowpass", output,
              options + ", title='Biquad Low pass filter (0.2, 0.9) (using iir_filter)'");

    println("SVG plots have been saved to svg directory");

    return 0;
}
