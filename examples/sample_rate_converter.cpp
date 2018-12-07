/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

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

    const size_t output_sr = std::atol(argv[3]);

    audio_reader_wav<double> reader(open_file_for_reading(argv[1]));
    const size_t input_sr = reader.format().samplerate;
    univector<double> input_interleaved(reader.format().length * reader.format().channels);
    reader.read(input_interleaved.data(), input_interleaved.size());
    univector2d<double> input_channels(
        reader.format().channels, univector<double>(input_interleaved.size() / reader.format().channels));
    deinterleave(input_channels, input_interleaved);

    univector2d<double> output_channels;
    println("Input channels: ", reader.format().channels);
    println("Input sample rate: ", reader.format().samplerate);
    println("Input bit depth: ", audio_sample_bit_depth(reader.format().type));

    for (size_t ch = 0; ch < input_channels.size(); ++ch)
    {
        println("Processing ", ch, " of ", reader.format().channels);
        const univector<double>& input = input_channels[ch];
        auto r                   = resampler<double>(resample_quality::high, output_sr, input_sr, 1.0, 0.492);
        const size_t output_size = input.size() * output_sr / input_sr;
        univector<double> output(output_size);
        const size_t input_step = r.skip(r.get_delay(), input.slice());

        size_t input_pos  = input_step;
        size_t output_pos = 0;
        for (;;)
        {
            const size_t block_size = std::min(size_t(4096), output.size() - output_pos);
            if (block_size == 0)
                break;
            const size_t input_step =
                r.process(output.slice(output_pos, block_size).ref(), input.slice(input_pos));
            input_pos += input_step;
            output_pos += block_size;
        }

        output_channels.push_back(std::move(output));
    }

    univector<double> output_interleved = interleave(output_channels);

    audio_writer_wav<double> writer(
        open_file_for_writing(argv[2]),
        audio_format{ reader.format().channels, reader.format().type, kfr::fmax(output_sr) });
    writer.write(output_interleved.data(), output_interleved.size());

    return 0;
}
