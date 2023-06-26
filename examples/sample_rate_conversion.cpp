/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

constexpr size_t input_sr  = 96000;
constexpr size_t output_sr = 44100;
constexpr size_t len       = 96000 * 6;

int main()
{
    println(library_version());

    univector<fbase> swept_sine = swept(0.5, len);

    {
        auto r = resampler<fbase>(resample_quality::high, output_sr, input_sr);
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());
        r.process(resampled, swept_sine);

        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_high_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        plot_save("audio_high_quality", "audio_high_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::normal, output_sr, input_sr);
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());
        r.process(resampled, swept_sine);

        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_normal_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        plot_save("audio_normal_quality", "audio_normal_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::low, output_sr, input_sr);
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());
        r.process(resampled, swept_sine);

        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_low_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        plot_save("audio_low_quality", "audio_low_quality.wav", "");
    }

    {
        auto r = resampler<fbase>(resample_quality::draft, output_sr, input_sr);
        univector<fbase> resampled(len * output_sr / input_sr + r.get_delay());
        r.process(resampled, swept_sine);

        audio_writer_wav<fbase> writer(open_file_for_writing(KFR_FILEPATH("audio_draft_quality.wav")),
                                       audio_format{ 1, audio_sample_type::i32, output_sr });
        writer.write(resampled.data(), resampled.size());
        writer.close();

        plot_save("audio_draft_quality", "audio_draft_quality.wav", "");
    }

    println("SVG plots have been saved to svg directory");

    return 0;
}
