/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/version.hpp>

// print(), format()
#include <kfr/cometa/string.hpp>

#include <kfr/math.hpp>

// resample*
#include <kfr/dsp/sample_rate_conversion.hpp>

// file*
#include <kfr/io/audiofile.hpp>

// swept
#include <kfr/dsp/oscillators.hpp>

// plot_save()
#include <kfr/io/python_plot.hpp>

#include <iostream>

using namespace kfr;

constexpr size_t input_sr  = 96000;
constexpr size_t output_sr = 44100;
constexpr size_t len       = 96000 * 6;
constexpr fbase i32max     = 2147483647.0;

int main(int argc, char** argv)
{
    println(library_version());

    const std::string options = "phaseresp=False";

    univector<fbase> swept_sine = swept<fbase>(0.5, len);

    {
        auto r = resampler<fbase>(resample_quality::high, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.slice(0, destsize) * i32max, -i32max, +i32max);
        univector2d<i32> data  = { i32data };

        auto wr = sequential_file_writer("audio_high_quality.wav");
        audio_encode(wr, data, audioformat(data, output_sr));

        plot_save("audio_high_quality", "audio_high_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::normal, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.slice(0, destsize) * i32max, -i32max, +i32max);
        univector2d<i32> data  = { i32data };

        auto wr = sequential_file_writer("audio_normal_quality.wav");
        audio_encode(wr, data, audioformat(data, output_sr));

        plot_save("audio_normal_quality", "audio_normal_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::low, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.slice(0, destsize) * i32max, -i32max, +i32max);
        univector2d<i32> data  = { i32data };

        auto wr = sequential_file_writer("audio_low_quality.wav");
        audio_encode(wr, data, audioformat(data, output_sr));

        plot_save("audio_low_quality", "audio_low_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::draft, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.slice(0, destsize) * i32max, -i32max, +i32max);
        univector2d<i32> data  = { i32data };

        auto wr = sequential_file_writer("audio_draft_quality.wav");
        audio_encode(wr, data, audioformat(data, output_sr));

        plot_save("audio_draft_quality", "audio_draft_quality.wav", "");
    }

    return 0;
}
