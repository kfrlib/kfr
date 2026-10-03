/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/audio.hpp>

using namespace kfr;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        println("Usage: ebu_test INPUT_FILE");
        println("Supported input formats: WAV, RF64, BW64, W64, FLAC, MP3, AIFF, CAF");
        return 1;
    }

    // Pick a decoder matching the file extension and open the input file.
    std::unique_ptr<audio_decoder> decoder = create_decoder_for_file(argv[1]);
    auto format                            = decoder->open(argv[1]);
    if (!format)
    {
        println("Error: cannot open input file: ", to_string(format.error()));
        return 2;
    }

    // Report the input file's layout, then reject channel counts outside the supported range.
    const size_t channels = format->channels;
    println("Input channels: ", channels);
    println("Input sample rate: ", format->sample_rate);
    println("Input bit depth: ", format->bit_depth);

    if (channels < 1 || channels > 8)
    {
        println("Unsupported number of channels: ", channels);
        return 1;
    }

    // Build the EBU R128 meter: default 100 ms packets (10 Hz refresh), one ebu_channel per speaker.
    ebu_r128<fbase> loudness(format->sample_rate, arrangement_speakers(arrangement_for_channels(channels)));

    // Scratch buffers: interleaved sink for the decoder, planar buffer for per-channel packet views.
    constexpr size_t chunk_size = 1 << 16;
    audio_data_interleaved input_chunk_interleaved(channels, chunk_size);
    audio_data input_chunk(channels, chunk_size);

    // M, S, I, RL, RH hold the current measurements; maxM/maxS track the running peaks.
    fbase M, S, I, RL, RH;
    fbase maxM = -HUGE_VALF, maxS = -HUGE_VALF;

    println("Processing...");
    fflush(stdout);
    for (;;)
    {
        // Pull the next block of decoded samples; end_of_file terminates the loop normally.
        const auto frames_read = decoder->read_to(input_chunk_interleaved);
        if (!frames_read)
        {
            if (frames_read.error() == audiofile_error::end_of_file)
                break;
            println("Error: cannot read input file: ", to_string(frames_read.error()));
            return 2;
        }

        // Convert interleaved -> planar and trim the buffer to the exact number of frames returned.
        input_chunk = input_chunk_interleaved.truncate(*frames_read);

        // Feed the meter one packet at a time, tracking peaks after each update.
        const size_t packet_size = loudness.packet_size();
        for (size_t i = 0; i + packet_size <= *frames_read; i += packet_size)
        {
            // Build per-channel packet views (zero-copy slices into the planar buffer).
            std::vector<univector_ref<fbase>> ch_refs;
            for (size_t ch = 0; ch < channels; ++ch)
            {
                ch_refs.push_back(input_chunk.channel(ch).slice(i, packet_size));
            }
            // Run K-weighting, accumulate momentary/short-term energy, and update integrated/LRA.
            loudness.process_packet(ch_refs);
            loudness.get_values(M, S, I, RL, RH);
            maxM = std::max(maxM, M);
            maxS = std::max(maxS, S);
        }
    }

    {
        // For file-based measurements, the signal should be followed by at least 1.5 s of silence
        // so the short-term and integrated loudness windows can settle. Push 15 silence packets
        // (~1.5 s at the default 100 ms packet size) to flush the buffers.
        std::vector<univector_dyn<fbase>> ch_silence(channels, univector_dyn<fbase>(loudness.packet_size()));
        for (size_t i = 0; i < 15; ++i)
            loudness.process_packet(ch_silence);
        // Re-read the final measurements: the silence tail is what makes I (and LRA) settle.
        loudness.get_values(M, S, I, RL, RH);
    }

    println(argv[1]);
    println("M = ", M);
    println("S = ", S);
    println("I = ", I);
    println("LRA = ", RH - RL);
    println("maxM = ", maxM);
    println("maxS = ", maxS);
    println();

    return 0;
}
