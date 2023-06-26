/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/dsp/ebu.hpp>
#include <kfr/dsp/oscillators.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

struct TestFragment
{
    float gain; // dB
    float duration; // seconds
    float frequency; // Hz
};

struct TestFragmentMultichannel
{
    float gain_L_R; // dB
    float gain_C; // dB
    float gain_Ls_Rs; // dB
    float duration; // seconds
    float frequency; // Hz
};

template <typename T>
static void ebu_test_stereo(int sample_rate, const std::initializer_list<TestFragment>& fragments, T refM,
                            T refS, T refI, T refLRA)
{
    ebu_r128<T> loudness(sample_rate, { Speaker::Left, Speaker::Right });

    size_t total_length = 0;
    for (const TestFragment& f : fragments)
    {
        total_length += static_cast<size_t>(f.duration * sample_rate);
    }

    univector<T> left_right(total_length);
    size_t pos = 0;
    for (const TestFragment& f : fragments)
    {
        const size_t len           = static_cast<size_t>(f.duration * sample_rate);
        left_right.slice(pos, len) = dB_to_amp(f.gain) * sinenorm(phasor<float>(f.frequency, sample_rate));
        pos += len;
    }

    for (size_t i = 0; i < total_length / loudness.packet_size(); i++)
    {
        loudness.process_packet({ left_right.slice(i * loudness.packet_size(), loudness.packet_size()),
                                  left_right.slice(i * loudness.packet_size(), loudness.packet_size()) });
    }
    T M, S, I, RL, RH;
    loudness.get_values(M, S, I, RL, RH);
    if (!std::isnan(refM))
    {
        testo::scope s(as_string("M = ", fmt<'f', -1, 2>(M)));
        CHECK(std::abs(M - refM) < 0.05f);
    }
    if (!std::isnan(refS))
    {
        testo::scope s(as_string("S = ", fmt<'f', -1, 2>(S)));
        CHECK(std::abs(S - refS) < 0.05f);
    }
    if (!std::isnan(refI))
    {
        testo::scope s(as_string("I = ", fmt<'f', -1, 2>(I)));
        CHECK(std::abs(I - refI) < 0.05f);
    }
    if (!std::isnan(refLRA))
    {
        testo::scope s(as_string("LRA = ", fmt<'f', -1, 2>((RH - RL))));
        CHECK(std::abs((RH - RL) - refLRA) < 0.05f);
    }
}

template <typename T>
static void ebu_test_multichannel(int sample_rate,
                                  const std::initializer_list<TestFragmentMultichannel>& fragments, T refM,
                                  T refS, T refI, T refLRA)
{
    ebu_r128<T> loudness(sample_rate, { Speaker::Left, Speaker::Right, Speaker::Center, Speaker::LeftSurround,
                                        Speaker::RightSurround });

    size_t total_length = 0;
    for (const TestFragmentMultichannel& f : fragments)
    {
        total_length += static_cast<size_t>(f.duration * sample_rate);
    }

    univector<T> left_right(total_length);
    univector<T> center(total_length);
    univector<T> surround(total_length);
    size_t pos = 0;
    for (const TestFragmentMultichannel& f : fragments)
    {
        const size_t len = static_cast<size_t>(f.duration * sample_rate);
        left_right.slice(pos, len) =
            dB_to_amp(f.gain_L_R) * sinenorm(phasor<float>(f.frequency, sample_rate));
        center.slice(pos, len) = dB_to_amp(f.gain_C) * sinenorm(phasor<float>(f.frequency, sample_rate));
        surround.slice(pos, len) =
            dB_to_amp(f.gain_Ls_Rs) * sinenorm(phasor<float>(f.frequency, sample_rate));
        pos += len;
    }

    for (size_t i = 0; i < total_length / loudness.packet_size(); i++)
    {
        loudness.process_packet({ left_right.slice(i * loudness.packet_size(), loudness.packet_size()),
                                  left_right.slice(i * loudness.packet_size(), loudness.packet_size()),
                                  center.slice(i * loudness.packet_size(), loudness.packet_size()),
                                  surround.slice(i * loudness.packet_size(), loudness.packet_size()),
                                  surround.slice(i * loudness.packet_size(), loudness.packet_size()) });
    }
    T M, S, I, RL, RH;
    loudness.get_values(M, S, I, RL, RH);
    if (!std::isnan(refM))
    {
        testo::scope s(as_string("M = ", fmt<'f', -1, 2>(M)));
        CHECK(std::abs(M - refM) < 0.05f);
    }
    if (!std::isnan(refS))
    {
        testo::scope s(as_string("S = ", fmt<'f', -1, 2>(S)));
        CHECK(std::abs(S - refS) < 0.05f);
    }
    if (!std::isnan(refI))
    {
        testo::scope s(as_string("I = ", fmt<'f', -1, 2>(I)));
        CHECK(std::abs(I - refI) < 0.05f);
    }
    if (!std::isnan(refLRA))
    {
        testo::scope s(as_string("LRA = ", fmt<'f', -1, 2>((RH - RL))));
        CHECK(std::abs((RH - RL) - refLRA) < 0.05f);
    }
}

TEST(ebu_stereo_1_and_2)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 },
                  [](auto type, int sample_rate)
                  {
                      using T = typename decltype(type)::type;

                      ebu_test_stereo<T>(sample_rate, { { -23.f, 20.f, 1000.f } }, -23.f, -23.f, -23.f, NAN);
                      ebu_test_stereo<T>(sample_rate, { { -33.f, 20.f, 1000.f } }, -33.f, -33.f, -33.f, NAN);
                  });
}

TEST(ebu_stereo_3_4_and_5)
{
    testo::matrix(
        named("type") = ctypes_t<float, double>{}, named("sample_rate") = std::vector<int>{ 44100, 48000 },
        [](auto type, int sample_rate)
        {
            using T = typename decltype(type)::type;

            ebu_test_stereo<T>(sample_rate,
                               { { -36.f, 10.f, 1000.f }, { -23.f, 60.f, 1000.f }, { -36.f, 10.f, 1000.f } },
                               NAN, NAN, -23.f, NAN);
            ebu_test_stereo<T>(sample_rate,
                               { { -72.f, 10.f, 1000.f },
                                 { -36.f, 10.f, 1000.f },
                                 { -23.f, 60.f, 1000.f },
                                 { -36.f, 10.f, 1000.f },
                                 { -72.f, 10.f, 1000.f } },
                               NAN, NAN, -23.f, NAN);
        });
}

TEST(ebu_multichannel_6)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 },
                  [](auto type, int sample_rate)
                  {
                      using T = typename decltype(type)::type;

                      ebu_test_multichannel<T>(sample_rate, { { -28.f, -24.f, -30.f, 20.f, 1000.f } }, NAN,
                                               NAN, -23.f, NAN);
                  });
}

TEST(ebu_stereo_9)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 },
                  [](auto type, int sample_rate)
                  {
                      using T = typename decltype(type)::type;

                      ebu_test_stereo<T>(sample_rate,
                                         { { -20.f, 1.34f, 1000.f },
                                           { -30.f, 1.66f, 1000.f },
                                           { -20.f, 1.34f, 1000.f },
                                           { -30.f, 1.66f, 1000.f },
                                           { -20.f, 1.34f, 1000.f },
                                           { -30.f, 1.66f, 1000.f },
                                           { -20.f, 1.34f, 1000.f },
                                           { -30.f, 1.66f, 1000.f },
                                           { -20.f, 1.34f, 1000.f },
                                           { -30.f, 1.66f, 1000.f } },
                                         NAN, -23.f, NAN, NAN);
                  });
}

TEST(ebu_stereo_12)
{
    testo::matrix(
        named("type") = ctypes_t<float, double>{}, named("sample_rate") = std::vector<int>{ 44100, 48000 },
        [](auto type, int sample_rate)
        {
            using T = typename decltype(type)::type;

            ebu_test_stereo<T>(sample_rate,
                               { { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f },
                                 { -30.f, 0.22f, 1000.f }, { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f },
                                 { -20.f, 0.18f, 1000.f }, { -30.f, 0.22f, 1000.f } },
                               -23.f, NAN, NAN, NAN);
        });
}

TEST(ebu_lra_1_2_3_and_4)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 },
                  [](auto type, int sample_rate)
                  {
                      using T = typename decltype(type)::type;

                      ebu_test_stereo<T>(sample_rate, { { -20.f, 20.f, 1000.f }, { -30.f, 20.f, 1000.f } },
                                         NAN, NAN, NAN, 10.f);

                      ebu_test_stereo<T>(sample_rate, { { -20.f, 20.f, 1000.f }, { -15.f, 20.f, 1000.f } },
                                         NAN, NAN, NAN, 5.f);

                      ebu_test_stereo<T>(sample_rate, { { -40.f, 20.f, 1000.f }, { -20.f, 20.f, 1000.f } },
                                         NAN, NAN, NAN, 20.f);

                      ebu_test_stereo<T>(sample_rate,
                                         { { -50.f, 20.f, 1000.f },
                                           { -35.f, 20.f, 1000.f },
                                           { -20.f, 20.f, 1000.f },
                                           { -35.f, 20.f, 1000.f },
                                           { -50.f, 20.f, 1000.f } },
                                         NAN, NAN, NAN, 15.f);
                  });
}
} // namespace CMT_ARCH_NAME

} // namespace kfr
