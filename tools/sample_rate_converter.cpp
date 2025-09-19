/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <chrono>
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/audio.hpp>

// Example: Command line sample rate converter

using namespace kfr;

int main(int argc, char** argv)
{
    println(library_version());
    if (argc < 4)
    {
        println("Usage: sample_rate_converter <INPUT_FILE> <OUTPUT_FILE> <TARGET_SAMPLE_RATE>");
        println("Supported input formats: WAV, RF64, BW64, W64, FLAC, MP3, AIFF, CAF");
        println("Output format: WAV/RF64, 16, 24, 32-bit PCM, 32, 64-bit IEEE");
        return 1;
    }

    // Get output sample rate from the command line
    const size_t output_sr = std::atol(argv[3]);

    std::unique_ptr<audio_decoder> decoder = create_decoder_for_file(argv[1]);
    auto format                            = decoder->open(argv[1]);
    if (!format)
    {
        println("Error: cannot open input file: ", to_string(format.error()));
        return 2;
    }

    const size_t channels = format->channels;
    const size_t input_sr = static_cast<size_t>(format->sample_rate);

    println("Input channels: ", channels);
    println("Input sample rate: ", format->sample_rate);
    println("Input bit depth: ", format->bit_depth);

    std::unique_ptr<audio_encoder> encoder =
        create_wave_encoder({ {}, /* .switch_to_rf64_if_over_4gb = */ true });
    audiofile_format out_format{};
    if (format->codec == audiofile_codec::ieee_float || format->codec == audiofile_codec::lpcm)
        out_format = *format; // copy input format if available

    out_format.sample_rate = static_cast<uint32_t>(output_sr);
    out_format.container   = audiofile_container::wave;
    out_format.endianness  = audiofile_endianness::little;
    auto opened            = encoder->open(argv[2], out_format);
    if (!opened)
    {
        println("Error: cannot open output file: ", to_string(opened.error()));
        return 2;
    }

    std::vector<samplerate_converter<fbase>> resamplers(channels);
    for (size_t ch = 0; ch < channels; ++ch)
    {
        resamplers[ch] = resampler<fbase>(resample_quality::high, output_sr, input_sr);
    }
    auto& resampler0 = resamplers.front();

    constexpr size_t output_chunk_size = 16384;
    audio_data output_chunk(channels, output_chunk_size);
    audio_data_interleaved output_chunk_interleaved(channels, output_chunk_size);

    const size_t input_delay_compensation = resampler0.input_size_for_output(resampler0.get_delay());
    const size_t input_chunk_size = output_chunk_size * input_sr / output_sr + 1 + input_delay_compensation;
    audio_data_interleaved input_chunk_interleaved(channels, input_chunk_size);
    audio_data input_chunk(channels, input_chunk_size);

    bool first_chunk = true;
    std::chrono::high_resolution_clock::duration resampling_time{};
    // Process audio in chunks
    println("Resampling...");
    fflush(stdout);
    for (;;)
    {
        const size_t frames_to_read =
            resampler0.input_size_for_output(output_chunk_size + (first_chunk ? resampler0.get_delay() : 0));

        // Read channels of audio
        const auto frames_read = decoder->read_to(input_chunk_interleaved.truncate(frames_to_read));
        if (!frames_read)
        {
            if (frames_read.error() == audiofile_error::end_of_file)
                break;
            println("Error: cannot read input file: ", to_string(frames_read.error()));
            return 2;
        }
        // Deinterleave
        input_chunk = input_chunk_interleaved.truncate(*frames_read);

        size_t frames_to_write = output_chunk_size;
        if (*frames_read < frames_to_read)
        {
            frames_to_write = resampler0.output_size_for_input(*frames_read) + resampler0.get_delay();
        }
        if (frames_to_write <= resampler0.get_delay())
        {
            println("Error: input file is too short for resampling");
            return 2;
        }

        const std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        for (size_t ch = 0; ch < channels; ++ch)
        {
            auto& r       = resamplers[ch];
            auto&& output = output_chunk.channel(ch).truncate(frames_to_write).ref();
            auto&& input  = input_chunk.channel(ch).truncate(*frames_read);
            if (first_chunk)
            {
                // Skip the first r.get_delay() samples (FIR filter delay).
                r.skip(r.get_delay(), input);
            }

            // Process new block of audio
            r.process(output, input);
        }
        resampling_time += std::chrono::high_resolution_clock::now() - t1;
        output_chunk_interleaved = output_chunk.slice(0, frames_to_write);

        // Write audio
        auto written = encoder->write(output_chunk_interleaved);
        if (!written)
        {
            println("Error: cannot write to output file: ", to_string(written.error()));
            return 2;
        }
        first_chunk = false;
    }
    auto closed = encoder->close();
    if (!closed)
    {
        println("Error: cannot finalize output file: ", to_string(closed.error()));
        return 2;
    }
    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(resampling_time).count() / 1e9;
    double length   = double(*closed) / format->sample_rate;
    println("done in ", duration, " seconds", " (", fmt<'f', 4, 1>(length / duration), "x real-time)");

    return 0;
}
