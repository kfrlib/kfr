/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <chrono>
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

// Example: Command line sample rate converter

using namespace kfr;

int main(int argc, char** argv)
{
    println(library_version());
    if (argc < 4)
    {
        println("Usage: sample_rate_converter <INPUT_FILE> <OUTPUT_FILE> <TARGET_SAMPLE_RATE>");
        println("Supported formats: WAV/W64, 16, 24, 32-bit PCM, 32, 64-bit IEEE");
        return 1;
    }

    // Get output sample rate from the command line
    const size_t output_sr = std::atol(argv[3]);

    // Initialize WAV reader and get file sample rate
    audio_reader_wav<double> reader(open_file_for_reading(argv[1]));
    const size_t channels = reader.format().channels;
    const size_t input_sr = static_cast<size_t>(reader.format().samplerate);

    println("Input channels: ", channels);
    println("Input sample rate: ", reader.format().samplerate);
    println("Input bit depth: ", audio_sample_bit_depth(reader.format().type));

    // Initialize WAV writer
    audio_writer_wav<double> writer(open_file_for_writing(argv[2]),
                                    audio_format{ channels, reader.format().type, kfr::fmax(output_sr) });

    std::vector<samplerate_converter<double>> resamplers(channels);
    for (size_t ch = 0; ch < channels; ++ch)
    {
        resamplers[ch] = resampler<double>(resample_quality::high, output_sr, input_sr);
    }
    auto& resampler0 = resamplers.front();

    constexpr size_t output_chunk_size = 16384;
    univector2d<double> output_chunk(channels, univector<double>(output_chunk_size));
    univector<double> output_chunk_interleaved(output_chunk_size * channels);

    const size_t input_delay_compensation = resampler0.input_size_for_output(resampler0.get_delay());
    const size_t input_chunk_size = output_chunk_size * input_sr / output_sr + 1 + input_delay_compensation;
    univector<double> input_chunk_interleaved(input_chunk_size * channels);
    univector2d<double> input_chunk(channels, univector<double>(input_chunk_size));

    bool first_chunk = true;
    bool last_chunk  = false;
    std::chrono::high_resolution_clock::duration resampling_time{};
    // Process audio in chunks
    println("Resampling...");
    fflush(stdout);
    for (;;)
    {
        const size_t frames_to_read =
            resampler0.input_size_for_output(output_chunk_size + (first_chunk ? resampler0.get_delay() : 0));

        // Read channels of audio
        const size_t samples_read = reader.read(input_chunk_interleaved.truncate(frames_to_read * channels));
        const size_t frames_read  = samples_read / channels;
        deinterleave(input_chunk, input_chunk_interleaved.truncate(samples_read));

        size_t frames_to_write = output_chunk_size;
        if (frames_read < frames_to_read)
        {
            last_chunk      = true;
            frames_to_write = resampler0.output_size_for_input(frames_read) + resampler0.get_delay();
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
            auto&& output = output_chunk[ch].truncate(frames_to_write).ref();
            auto&& input  = input_chunk[ch].truncate(frames_read);
            if (first_chunk)
            {
                // Skip the first r.get_delay() samples (FIR filter delay).
                r.skip(r.get_delay(), input);
            }

            // Process new block of audio
            r.process(output, input);
        }
        resampling_time += std::chrono::high_resolution_clock::now() - t1;
        interleave(output_chunk_interleaved.truncate(frames_to_write * channels).ref(), output_chunk);

        // Write audio
        writer.write(output_chunk_interleaved.truncate(frames_to_write * channels));
        first_chunk = false;
        if (last_chunk)
            break;
    }
    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(resampling_time).count() / 1e9;
    double length   = reader.format().length / reader.format().samplerate;
    println("done in ", duration, " seconds", " (", fmt<'f', 4, 1>(length / duration), "x real-time)");

    return 0;
}
