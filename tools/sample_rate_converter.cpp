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
    const size_t input_sr = reader.format().samplerate;

    // Read channels of audio
    univector2d<double> input_channels = reader.read_channels(reader.format().length);

    // Prepare conversion
    univector2d<double> output_channels;
    println("Input channels: ", reader.format().channels);
    println("Input sample rate: ", reader.format().samplerate);
    println("Input bit depth: ", audio_sample_bit_depth(reader.format().type));

    for (size_t ch = 0; ch < input_channels.size(); ++ch)
    {
        println("Processing ", ch, " of ", reader.format().channels);
        const univector<double>& input = input_channels[ch];

        // Initialize resampler
        auto r = resampler<double>(resample_quality::high, output_sr, input_sr);

        // Calculate output size and initialize output buffer
        const size_t output_size = input.size() * output_sr / input_sr;
        univector<double> output(output_size);

        // Skip the first r.get_delay() samples (FIR filter delay). Returns new input pos
        size_t input_pos = r.skip(r.get_delay(), input.slice());

        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
        size_t output_pos                                         = 0;
        for (;;)
        {
            const size_t block_size = std::min(size_t(16384), output.size() - output_pos);
            if (block_size == 0)
                break;

            // Process new block of audio
            input_pos += r.process(output.slice(output_pos, block_size).ref(), input.slice(input_pos));
            output_pos += block_size;
        }

        std::chrono::high_resolution_clock::duration time =
            std::chrono::high_resolution_clock::now() - start_time;
        const double duration = static_cast<double>(output.size()) / output_sr;
        println("time: ",
                fmt<'f', 6, 2>(std::chrono::duration_cast<std::chrono::microseconds>(time).count() /
                               duration * 0.001),
                "ms per 1 second of audio");

        // Place buffer to the list of output channels
        output_channels.push_back(std::move(output));
    }

    // Initialize WAV writer
    audio_writer_wav<double> writer(
        open_file_for_writing(argv[2]),
        audio_format{ reader.format().channels, reader.format().type, kfr::fmax(output_sr) });

    // Write audio
    writer.write_channels(output_channels);

    return 0;
}
