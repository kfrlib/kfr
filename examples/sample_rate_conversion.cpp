/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

// Define constants for input and output sample rates and the length of the signal
constexpr size_t input_sr  = 96000; // Input sample rate (96 kHz)
constexpr size_t output_sr = 44100; // Output sample rate (44.1 kHz)
constexpr size_t len       = 96000 * 6; // Length of the signal (6 seconds at 96 kHz)

int main()
{
    // Print the version of the KFR library being used
    println(library_version());

    // Generate a swept sine wave signal with a duration of 'len' samples
    univector<fbase> swept_sine = swept(0.5, len);

    // --------------------------------------------------------------------------------------
    // ----------------------------- High Quality Resampling --------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a high-quality resampler from input_sr to output_sr
        auto r = resampler<fbase>(resample_quality::high, output_sr, input_sr);

        // Create a buffer for the resampled signal, taking the resampler delay into account
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());

        // Perform the resampling process
        r.process(resampled, swept_sine);

        // Write the resampled signal to a WAV file
        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_high_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        // Save a plot of the high-quality resampled audio
        plot_save("audio_high_quality", "audio_high_quality.wav", "");
    }

    // --------------------------------------------------------------------------------------
    // ----------------------------- Normal Quality Resampling ------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a normal-quality resampler from input_sr to output_sr
        auto r = resampler<fbase>(resample_quality::normal, output_sr, input_sr);

        // Create a buffer for the resampled signal, taking the resampler delay into account
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());

        // Perform the resampling process
        r.process(resampled, swept_sine);

        // Write the resampled signal to a WAV file
        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_normal_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        // Save a plot of the normal-quality resampled audio
        plot_save("audio_normal_quality", "audio_normal_quality.wav", "");
    }

    // --------------------------------------------------------------------------------------
    // ----------------------------- Low Quality Resampling ---------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a low-quality resampler from input_sr to output_sr
        auto r = resampler<fbase>(resample_quality::low, output_sr, input_sr);

        // Create a buffer for the resampled signal, taking the resampler delay into account
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());

        // Perform the resampling process
        r.process(resampled, swept_sine);

        // Write the resampled signal to a WAV file
        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_low_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        // Save a plot of the low-quality resampled audio
        plot_save("audio_low_quality", "audio_low_quality.wav", "");
    }

    // --------------------------------------------------------------------------------------
    // ----------------------------- Draft Quality Resampling -------------------------------
    // --------------------------------------------------------------------------------------
    {
        // Create a draft-quality resampler from input_sr to output_sr
        auto r = resampler<fbase>(resample_quality::draft, output_sr, input_sr);

        // Create a buffer for the resampled signal, taking the resampler delay into account
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());

        // Perform the resampling process
        r.process(resampled, swept_sine);

        // Write the resampled signal to a WAV file
        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_draft_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        // Save a plot of the draft-quality resampled audio
        plot_save("audio_draft_quality", "audio_draft_quality.wav", "");
    }

    println("SVG plots have been saved to svg directory");

    return 0;
}
