/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

constexpr size_t input_sr  = 96000;
constexpr size_t output_sr = 44100;
constexpr size_t len       = 96000 * 6;
constexpr fbase i32max     = 2147483647.0;

int main()
{
    println(library_version());

    const std::string options = "phaseresp=False";

    univector<fbase> swept_sine = swept(0.5, len);

    {
        auto r = resampler<fbase>(resample_quality::high, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.truncate(destsize) * i32max, -i32max, +i32max);
        {
            audio_writer_wav<i32> writer(open_file_for_writing(KFR_FILEPATH("audio_high_quality.wav")),
                                         audio_format{ 2, audio_sample_type::i32, output_sr });
            writer.write(i32data.data(), i32data.size());
        }

        plot_save("audio_high_quality", "audio_high_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::normal, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.truncate(destsize) * i32max, -i32max, +i32max);
        {
            audio_writer_wav<i32> writer(open_file_for_writing(KFR_FILEPATH("audio_normal_quality.wav")),
                                         audio_format{ 2, audio_sample_type::i32, output_sr });
            writer.write(i32data.data(), i32data.size());
        }

        plot_save("audio_normal_quality", "audio_normal_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::low, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.truncate(destsize) * i32max, -i32max, +i32max);
        {
            audio_writer_wav<i32> writer(open_file_for_writing(KFR_FILEPATH("audio_low_quality.wav")),
                                         audio_format{ 2, audio_sample_type::i32, output_sr });
            writer.write(i32data.data(), i32data.size());
        }

        plot_save("audio_low_quality", "audio_low_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::draft, output_sr, input_sr, 1.0, 0.496);
        univector<fbase> resampled(len * output_sr / input_sr);

        const size_t destsize = r(resampled.data(), swept_sine);

        univector<i32> i32data = clamp(resampled.truncate(destsize) * i32max, -i32max, +i32max);
        {
            audio_writer_wav<i32> writer(open_file_for_writing(KFR_FILEPATH("audio_draft_quality.wav")),
                                         audio_format{ 2, audio_sample_type::i32, output_sr });
            writer.write(i32data.data(), i32data.size());
        }

        plot_save("audio_draft_quality", "audio_draft_quality.wav", "");
    }

    return 0;
}
