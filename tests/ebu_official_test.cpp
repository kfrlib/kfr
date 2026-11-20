/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <span>
#include <string>
#include <vector>
#include <kfr/audio/decoder.hpp>
#include <kfr/dsp/ebu.hpp>
#include <kfr/test/test.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
struct official_loudness_result
{
    double momentary, shortterm, integrated, range;
    std::vector<double> momentary_windows;
    std::vector<double> shortterm_windows;
};

static official_loudness_result measure_official_file(const std::string& path, size_t expected_channels,
                                                       size_t observe_step_packets = 0,
                                                       size_t observe_start_packets = 0,
                                                       size_t observe_end_packets = 0)
{
    INFO(path);
    auto decoder = create_wave_decoder();
    REQUIRE(decoder);
    const auto format = decoder->open(path);
    REQUIRE(format);
    REQUIRE(format->channels == expected_channels);
    REQUIRE(format->sample_rate == 48000);

    constexpr std::array stereo{ speaker_type::Left, speaker_type::Right };
    constexpr std::array surround5{ speaker_type::Left, speaker_type::Right, speaker_type::Center,
                                     speaker_type::LeftSurround, speaker_type::RightSurround };
    constexpr std::array surround6{ speaker_type::Left, speaker_type::Right, speaker_type::Center,
                                     speaker_type::Lfe, speaker_type::LeftSurround, speaker_type::RightSurround };
    std::span<const speaker_type> speakers = stereo;
    if (expected_channels == 5)
        speakers = surround5;
    else if (expected_channels == 6)
        speakers = surround6;

    ebu_r128<double> meter(format->sample_rate, speakers, 5);
    const size_t packet_size = meter.packet_size();
    audio_data_interleaved decoded(expected_channels, packet_size);
    std::vector<univector<double>> channels(expected_channels, univector<double>(packet_size));
    std::vector<univector_ref<double>> views;
    for (auto& channel : channels)
        views.emplace_back(channel);
    size_t buffered = 0, packets = 0;
    double M, S, I, low, high;
    official_loudness_result result{};
    for (;;)
    {
        const auto read = decoder->read_to(decoded);
        if (!read)
        {
            REQUIRE(read.error() == audiofile_error::end_of_file);
            break;
        }
        if (*read == 0)
            break;
        for (size_t frame = 0; frame < *read; ++frame)
        {
            for (size_t ch = 0; ch < expected_channels; ++ch)
                channels[ch][buffered] = decoded.data[frame * expected_channels + ch];
            if (++buffered != packet_size)
                continue;
            meter.process_packet(views);
            buffered = 0;
            ++packets;
            if (observe_step_packets && packets >= observe_start_packets &&
                (!observe_end_packets || packets <= observe_end_packets) && packets % observe_step_packets == 0)
            {
                meter.get_values(M, S, I, low, high);
                result.momentary_windows.push_back(M);
                result.shortterm_windows.push_back(S);
            }
        }
    }
    if (buffered)
    {
        for (auto& channel : channels)
            std::fill(channel.begin() + buffered, channel.end(), 0.0);
        meter.process_packet(views);
        ++packets;
    }
    REQUIRE(packets > 0);
    meter.get_values(result.momentary, result.shortterm, result.integrated, low, high);
    result.range = high - low;
    return result;
}

TEST_CASE("ebu_official_test_signals")
{
    const char* root = std::getenv("LOUDNESS_TEST_SIGNALS");
    if (!root || !*root)
        SKIP("Set LOUDNESS_TEST_SIGNALS to the EBU loudness test set v05 directory");
    const auto path = [root](const std::string& name) { return std::string(root) + "/" + name; };
    const auto check = [](double value, double expected, double tolerance)
    {
        INFO(as_string("measured=", value, ", expected=", expected));
        CHECK(std::isfinite(value));
        CHECK(std::abs(value - expected) <= tolerance);
    };
    struct expected_file
    {
        const char* name;
        size_t channels;
        double momentary, shortterm, integrated, range;
    };
    constexpr double na = std::numeric_limits<double>::quiet_NaN();
    const expected_file files[] = {
        { "seq-3341-1-16bit.wav", 2, -23, -23, -23, na },
        { "seq-3341-2-16bit.wav", 2, -33, -33, -33, na },
        { "seq-3341-3-16bit-v02.wav", 2, na, na, -23, na },
        { "seq-3341-4-16bit-v02.wav", 2, na, na, -23, na },
        { "seq-3341-5-16bit-v02.wav", 2, na, na, -23, na },
        { "seq-3341-6-5channels-16bit.wav", 5, na, na, -23, na },
        { "seq-3341-6-6channels-WAVEEX-16bit.wav", 6, na, na, -23, na },
        { "seq-3341-7_seq-3342-5-24bit.wav", 2, na, na, -23, 5 },
        { "seq-3341-2011-8_seq-3342-6-24bit-v02.wav", 2, na, na, -23, 15 },
        { "seq-3342-1-16bit.wav", 2, na, na, na, 10 },
        { "seq-3342-2-16bit.wav", 2, na, na, na, 5 },
        { "seq-3342-3-16bit.wav", 2, na, na, na, 20 },
        { "seq-3342-4-16bit.wav", 2, na, na, na, 15 },
    };
    for (const auto& file : files)
    {
        INFO(file.name);
        const auto result = measure_official_file(path(file.name), file.channels);
        if (!std::isnan(file.momentary)) check(result.momentary, file.momentary, 0.1);
        if (!std::isnan(file.shortterm)) check(result.shortterm, file.shortterm, 0.1);
        if (!std::isnan(file.integrated)) check(result.integrated, file.integrated, 0.1);
        if (!std::isnan(file.range)) check(result.range, file.range, 1.0);
    }

    // Check every 100 ms after the stated settling time, including the final packet.
    for (const auto& item : { std::pair{ "seq-3341-9-24bit.wav", true },
                              std::pair{ "seq-3341-12-24bit.wav", false } })
    {
        INFO(item.first);
        const auto result = measure_official_file(path(item.first), 2, 5, item.second ? 150 : 50);
        const auto& values = item.second ? result.shortterm_windows : result.momentary_windows;
        REQUIRE(!values.empty());
        for (double value : values) check(value, -23, 0.1);
    }

    for (int i = 1; i <= 20; ++i)
    {
        const auto short_name = "seq-3341-10-" + std::to_string(i) + "-24bit.wav";
        const auto moment_name = "seq-3341-13-" + std::to_string(i) +
                                 (i <= 2 ? "-24bit.wav" : "-24bit.wav.wav");
        for (const auto& item : { std::pair{ short_name, true }, std::pair{ moment_name, false } })
        {
            INFO(item.first);
            const auto result = measure_official_file(path(item.first), 2, 1);
            const auto& values = item.second ? result.shortterm_windows : result.momentary_windows;
            REQUIRE(!values.empty());
            check(*std::max_element(values.begin(), values.end()), -23, 0.1);
        }
    }

    // The live-meter files contain consecutive 6 s and 0.8 s segments.
    for (const auto& item : { std::pair{ "seq-3341-11-24bit.wav", true },
                              std::pair{ "seq-3341-14-24bit.wav.wav", false } })
    {
        INFO(item.first);
        const auto result = measure_official_file(path(item.first), 2, 1);
        const auto& values = item.second ? result.shortterm_windows : result.momentary_windows;
        const size_t segment_packets = item.second ? 300 : 40;
        REQUIRE(values.size() >= 20 * segment_packets);
        for (size_t i = 0; i < 20; ++i)
        {
            INFO(as_string("segment ", i));
            const auto first = values.begin() + i * segment_packets;
            check(*std::max_element(first, first + segment_packets), -38.0 + i, 0.1);
        }
    }
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
