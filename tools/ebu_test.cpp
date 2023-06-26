/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        println("Usage: ebu_test INPUT_IN_F32_RAW_FORMAT CHANNEL_NUMBER");
        return 1;
    }

    // Prepare
    FILE* f                  = fopen(argv[1], "rb");
    const int channel_number = atoi(argv[2]);
    if (channel_number < 1 || channel_number > 6)
    {
        println("Incorrect number of channels");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    uintmax_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size % (sizeof(float) * channel_number))
    {
        println("Incorrect file size");
        return 1;
    }

    // Read file
    const size_t length = size / (sizeof(float) * channel_number);
    univector<float> interleaved(size / sizeof(float));
    size_t read_len = fread(interleaved.data(), 1, size, f);
    if (read_len != size)
    {
        println("Can't read file");
        return 1;
    }

    // Deinterleave
    univector<univector<float>> data(channel_number, univector<float>(length));
    for (size_t ch = 0; ch < channel_number; ++ch)
    {
        for (size_t i = 0; i < length; ++i)
        {
            data[ch][i] = interleaved[i * channel_number + ch];
        }
    }

    std::vector<Speaker> speakers;
    switch (channel_number)
    {
    case 1:
        speakers = { Speaker::Mono };
        break;
    case 2:
        speakers = { Speaker::Left, Speaker::Right };
        break;
    case 3:
        speakers = { Speaker::Left, Speaker::Right, Speaker::Center };
        break;
    case 4:
        speakers = { Speaker::Left, Speaker::Right, Speaker::LeftSurround, Speaker::RightSurround };
        break;
    case 5:
        speakers = { Speaker::Left, Speaker::Right, Speaker::Center, Speaker::LeftSurround,
                     Speaker::RightSurround };
        break;
    case 6:
        speakers = { Speaker::Left,         Speaker::Right,         Speaker::Center,
                     Speaker::LeftSurround, Speaker::RightSurround, Speaker::Lfe };
        break;
    }

    ebu_r128<float> loudness(48000, speakers);

    float M, S, I, RL, RH;
    float maxM = -HUGE_VALF, maxS = -HUGE_VALF;
    for (size_t i = 0; i < length / loudness.packet_size(); i++)
    {
        std::vector<univector_ref<float>> channels;
        for (size_t ch = 0; ch < channel_number; ++ch)
        {
            channels.push_back(data[ch].slice(i * loudness.packet_size(), loudness.packet_size()));
        }
        loudness.process_packet(channels);
        loudness.get_values(M, S, I, RL, RH);
        maxM = std::max(maxM, M);
        maxS = std::max(maxS, S);
    }

    {
        // For file-based measurements, the signal should be followed by at least 1.5 s of silence
        std::vector<univector_dyn<float>> channels(channel_number,
                                                   univector_dyn<float>(loudness.packet_size()));
        for (size_t i = 0; i < 15; ++i)
            loudness.process_packet(channels);
        float dummyM, dummyS, dummyI;
        loudness.get_values(dummyM, dummyS, dummyI, RL, RH);
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
