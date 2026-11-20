/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <algorithm>
#include <array>
#include <limits>
#include <vector>
#include <kfr/dsp/ebu.hpp>
#include <kfr/dsp/oscillators.hpp>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4244))

namespace kfr
{
inline namespace KFR_ARCH_NAME
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
    ebu_r128<T> loudness(sample_rate,
                         std::initializer_list<speaker_type>{ speaker_type::Left, speaker_type::Right });

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
        INFO(as_string("M = ", fmt<'f', -1, 2>(M)));
        CHECK(std::abs(M - refM) < 0.05f);
    }
    if (!std::isnan(refS))
    {
        INFO(as_string("S = ", fmt<'f', -1, 2>(S)));
        CHECK(std::abs(S - refS) < 0.05f);
    }
    if (!std::isnan(refI))
    {
        INFO(as_string("I = ", fmt<'f', -1, 2>(I)));
        CHECK(std::abs(I - refI) < 0.05f);
    }
    if (!std::isnan(refLRA))
    {
        INFO(as_string("LRA = ", fmt<'f', -1, 2>((RH - RL))));
        CHECK(std::abs((RH - RL) - refLRA) < 0.05f);
    }
}

template <typename T>
static void ebu_test_multichannel(int sample_rate,
                                  const std::initializer_list<TestFragmentMultichannel>& fragments, T refM,
                                  T refS, T refI, T refLRA)
{
    ebu_r128<T> loudness(sample_rate, std::initializer_list<speaker_type>{
                                          speaker_type::Left, speaker_type::Right, speaker_type::Center,
                                          speaker_type::LeftSurround, speaker_type::RightSurround });

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
        INFO(as_string("M = ", fmt<'f', -1, 2>(M)));
        CHECK(std::abs(M - refM) < 0.1f);
    }
    if (!std::isnan(refS))
    {
        INFO(as_string("S = ", fmt<'f', -1, 2>(S)));
        CHECK(std::abs(S - refS) < 0.1f);
    }
    if (!std::isnan(refI))
    {
        INFO(as_string("I = ", fmt<'f', -1, 2>(I)));
        CHECK(std::abs(I - refI) < 0.1f);
    }
    if (!std::isnan(refLRA))
    {
        INFO(as_string("LRA = ", fmt<'f', -1, 2>((RH - RL))));
        CHECK(std::abs((RH - RL) - refLRA) < 0.1f);
    }
}

TEST_CASE("ebu_stereo_1_and_2")
{
    test_matrix(named("type")        = ctypes_t<float, double>{},
                named("sample_rate") = std::vector<int>{ 44100, 48000 },
                [](auto type, int sample_rate)
                {
                    using T = typename decltype(type)::type;

                    ebu_test_stereo<T>(sample_rate, { { -23.f, 20.f, 1000.f } }, -23.f, -23.f, -23.f, NAN);
                    ebu_test_stereo<T>(sample_rate, { { -33.f, 20.f, 1000.f } }, -33.f, -33.f, -33.f, NAN);
                });
}

TEST_CASE("ebu_stereo_3_4_and_5")
{
    test_matrix(
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

TEST_CASE("ebu_multichannel_6")
{
    test_matrix(named("type")        = ctypes_t<float, double>{},
                named("sample_rate") = std::vector<int>{ 44100, 48000 },
                [](auto type, int sample_rate)
                {
                    using T = typename decltype(type)::type;

                    ebu_test_multichannel<T>(sample_rate, { { -28.f, -24.f, -30.f, 20.f, 1000.f } }, NAN, NAN,
                                             -23.f, NAN);
                });
}

TEST_CASE("ebu_stereo_9")
{
    test_matrix(named("type")        = ctypes_t<float, double>{},
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

TEST_CASE("ebu_stereo_12")
{
    test_matrix(
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

TEST_CASE("ebu_lra_1_2_3_and_4")
{
    test_matrix(named("type")        = ctypes_t<float, double>{},
                named("sample_rate") = std::vector<int>{ 44100, 48000 },
                [](auto type, int sample_rate)
                {
                    using T = typename decltype(type)::type;

                    ebu_test_stereo<T>(sample_rate, { { -20.f, 20.f, 1000.f }, { -30.f, 20.f, 1000.f } }, NAN,
                                       NAN, NAN, 10.f);

                    ebu_test_stereo<T>(sample_rate, { { -20.f, 20.f, 1000.f }, { -15.f, 20.f, 1000.f } }, NAN,
                                       NAN, NAN, 5.f);

                    ebu_test_stereo<T>(sample_rate, { { -40.f, 20.f, 1000.f }, { -20.f, 20.f, 1000.f } }, NAN,
                                       NAN, NAN, 20.f);

                    ebu_test_stereo<T>(sample_rate,
                                       { { -50.f, 20.f, 1000.f },
                                         { -35.f, 20.f, 1000.f },
                                         { -20.f, 20.f, 1000.f },
                                         { -35.f, 20.f, 1000.f },
                                         { -50.f, 20.f, 1000.f } },
                                       NAN, NAN, NAN, 15.f);
                });
}
TEST_CASE("ebu_integrated_gates_and_reset")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    integrated_vec<T> integrated;

                    CHECK(std::isinf(integrated.get()));
                    CHECK(integrated.get() < T(0));
                    for (int i = 0; i < 100; ++i)
                        integrated.push(loudness_to_energy(T(-70.5)));
                    CHECK(std::isinf(integrated.get()));
                    CHECK(integrated.get() < T(0));
                    integrated.push(loudness_to_energy(T(-69.5)));
                    CHECK(std::abs(integrated.get() - T(-69.5)) < T(0.11));

                    integrated.reset();
                    CHECK(std::isinf(integrated.get()));
                    CHECK(integrated.get() < T(0));
                    integrated.push(loudness_to_energy(T(-20)));
                    CHECK(std::abs(integrated.get() - T(-20)) < T(0.11));
                    integrated.push(loudness_to_energy(T(-28)));
                    // Equal-duration -20 and -28 LUFS blocks average to -22.37 LUFS in energy.
                    CHECK(std::abs(integrated.get() - T(-22.37)) < T(0.11));
                    integrated.push(loudness_to_energy(T(-40)));
                    CHECK(std::abs(integrated.get() - T(-22.37)) < T(0.11));

                    integrated.reset();
                    integrated.push(loudness_to_energy(T(-10)));
                    for (int i = 0; i < 1000; ++i)
                        integrated.push(loudness_to_energy(T(-35)));
                    // The relative gate must use block counts, not an unweighted mean of occupied bins.
                    const T expected = energy_to_loudness((loudness_to_energy(T(-10)) +
                                                           T(1000) * loudness_to_energy(T(-35))) /
                                                          T(1001));
                    CHECK(std::abs(integrated.get() - expected) < T(0.11));

                    integrated.reset();
                    integrated.push(std::numeric_limits<T>::quiet_NaN());
                    integrated.push(std::numeric_limits<T>::infinity());
                    CHECK(std::isinf(integrated.get()));
                    integrated.push(loudness_to_energy(T(-23.01)));
                    integrated.push(loudness_to_energy(T(-23.04)));
                    const T same_bin_expected = energy_to_loudness(
                        (loudness_to_energy(T(-23.01)) + loudness_to_energy(T(-23.04))) / T(2));
                    CHECK(std::abs(integrated.get() - same_bin_expected) < T(0.011));

                    integrated.reset();
                    integrated.push(loudness_to_energy(T(29.98)));
                    CHECK(std::abs(integrated.get() - T(29.98)) < T(0.02));
                    integrated.reset();
                    integrated.push(loudness_to_energy(T(30)));
                    CHECK(std::abs(integrated.get() - T(30)) < T(0.02));
                    integrated.reset();
                    integrated.push(loudness_to_energy(T(32)));
                    integrated.push(loudness_to_energy(T(38)));
                    // Values above the histogram range saturate at +30 LUFS.
                    CHECK(std::abs(integrated.get() - T(30)) < T(0.02));
                });
}

TEST_CASE("ebu_reference_stereo_tone")
{
    // A 0.5-peak stereo 1 kHz sine at 48 kHz measures approximately -6.0 LUFS
    // with FFmpeg's EBU R128 filter; compare after the meter has settled.
    ebu_r128<double> meter(48000, std::initializer_list<speaker_type>{ speaker_type::Left, speaker_type::Right });
    univector<double> packet(meter.packet_size());
    packet = 0.5 * sinenorm(phasor<double>(1000.0, 48000.0));
    for (int i = 0; i < 40; ++i)
        meter.process_packet({ packet, packet });
    double momentary, shortterm, integrated, low, high;
    meter.get_values(momentary, shortterm, integrated, low, high);
    INFO(as_string("M=", momentary, " S=", shortterm, " I=", integrated));
    CHECK(std::abs(momentary - (-6.0)) < 0.1);
    CHECK(std::abs(integrated - (-6.0)) < 0.1);
}

TEST_CASE("ebu_complete_gating_windows")
{
    const std::array channels{ speaker_type::Left, speaker_type::Right };
    ebu_r128<double> meter(48000, channels);
    univector<double> packet(meter.packet_size());
    packet = 0.5 * sinenorm(phasor<double>(1000.0, 48000.0));
    double momentary, shortterm, integrated, low, high;
    for (int i = 0; i < 3; ++i)
        meter.process_packet({ packet, packet });
    meter.get_values(momentary, shortterm, integrated, low, high);
    CHECK(std::isinf(integrated));
    CHECK(integrated < 0);
    meter.process_packet({ packet, packet });
    meter.get_values(momentary, shortterm, integrated, low, high);
    CHECK(std::abs(integrated + 6.02) < 0.1);
    CHECK(low == -70);
    CHECK(high == -70);
    for (int i = 4; i < 29; ++i)
        meter.process_packet({ packet, packet });
    meter.get_values(momentary, shortterm, integrated, low, high);
    CHECK(low == -70);
    CHECK(high == -70);
    meter.process_packet({ packet, packet });
    meter.get_values(momentary, shortterm, integrated, low, high);
    CHECK(std::abs(low + 6.02) < 0.1);
    CHECK(std::abs(high + 6.02) < 0.1);
}

TEST_CASE("ebu_histogram_gate_boundary")
{
    integrated_vec<double> integrated;
    for (int i = 0; i < 10000; ++i)
    {
        integrated.push(loudness_to_energy(-30.09));
        integrated.push(loudness_to_energy(-30.01));
    }
    for (int i = 0; i < 2000; ++i)
        integrated.push(loudness_to_energy(-10.039));

    // The gate is near -30.04 LUFS. With 0.01 LU bins, the lower group is
    // excluded and the upper group is retained, rather than dropping both.
    const double expected = energy_to_loudness(
        (10000.0 * loudness_to_energy(-30.01) + 2000.0 * loudness_to_energy(-10.039)) / 12000.0);
    CHECK(std::abs(integrated.get() - expected) < 0.02);
}

TEST_CASE("ebu_lra_gating_percentiles_and_reset")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    lra_vec<T> range;
                    T low, high;
                    range.get(low, high);
                    CHECK(low == T(-70));
                    CHECK(high == T(-70));
                    range.push(loudness_to_energy(T(-25)));
                    range.get(low, high);
                    CHECK(std::abs(low - T(-25)) < T(0.02));
                    CHECK(std::abs(high - T(-25)) < T(0.02));
                    range.reset();
                    for (int i = 0; i < 100; ++i)
                        range.push(loudness_to_energy(T(-70.5)));
                    for (int i = 0; i < 20; ++i)
                        range.push(loudness_to_energy(T(-69.5)));
                    range.get(low, high);
                    CHECK(std::abs(low - T(-69.5)) < T(0.11));
                    CHECK(std::abs(high - T(-69.5)) < T(0.11));
                    range.reset();

                    // The -55 LUFS blocks pass the absolute gate but not the relative gate;
                    // the -40 and -12 LUFS tails remain outside the 10th and 95th percentiles.
                    for (int i = 0; i < 20; ++i)
                        range.push(loudness_to_energy(T(-55)));
                    for (int i = 0; i < 3; ++i)
                        range.push(loudness_to_energy(T(-40)));
                    for (int i = 0; i < 20; ++i)
                        range.push(loudness_to_energy(T(-35)));
                    for (int i = 0; i < 60; ++i)
                        range.push(loudness_to_energy(T(-25)));
                    for (int i = 0; i < 20; ++i)
                        range.push(loudness_to_energy(T(-15)));
                    for (int i = 0; i < 3; ++i)
                        range.push(loudness_to_energy(T(-12)));
                    range.get(low, high);
                    CHECK(std::abs(low - T(-35)) < T(0.11));
                    CHECK(std::abs(high - T(-15)) < T(0.11));
                    CHECK(std::abs((high - low) - T(20)) < T(0.21));

                    for (int i = 0; i < 200; ++i)
                        range.push(loudness_to_energy(T(-5)));
                    range.get(low, high);
                    CHECK(std::abs(high - T(-5)) < T(0.11));
                    range.reset();
                    range.get(low, high);
                    CHECK(low == T(-70));
                    CHECK(high == T(-70));
                    range.push(loudness_to_energy(T(-28)));
                    range.push(loudness_to_energy(T(-28)));
                    range.get(low, high);
                    CHECK(std::abs(low - T(-28)) < T(0.11));
                    CHECK(std::abs(high - T(-28)) < T(0.11));

                    range.reset();
                    for (int i = 0; i < 100; ++i)
                        range.push(loudness_to_energy(T(-10)));
                    for (int i = 0; i < 1000; ++i)
                        range.push(loudness_to_energy(T(-35)));
                    range.get(low, high);
                    CHECK(std::abs(low - T(-35)) < T(0.11));
                    CHECK(std::abs(high - T(-10)) < T(0.11));

                    range.reset();
                    range.push(loudness_to_energy(T(29)));
                    range.push(loudness_to_energy(T(35)));
                    range.get(low, high);
                    CHECK(std::abs(low - T(29)) < T(0.02));
                    CHECK(std::abs(high - T(30)) < T(0.02));
                });
}

TEST_CASE("ebu_silence")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    ebu_r128<T> loudness(8000, std::initializer_list<speaker_type>{ speaker_type::Left });
                    univector<T> silence(loudness.packet_size(), T(0));
                    for (int i = 0; i < 40; ++i)
                        loudness.process_packet({ silence });
                    T M, S, I, RL, RH;
                    loudness.get_values(M, S, I, RL, RH);
                    CHECK(std::isinf(M));
                    CHECK(M < T(0));
                    CHECK(std::isinf(S));
                    CHECK(S < T(0));
                    CHECK(std::isinf(I));
                    CHECK(I < T(0));
                    CHECK(RL == T(-70));
                    CHECK(RH == T(-70));
                });
}

TEST_CASE("ebu_silence_packets_match_zero_input")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    const std::array speakers{ speaker_type::Left, speaker_type::RightSurround };
                    ebu_r128<T> zeros_meter(48000, speakers), silence_meter(48000, speakers);
                    univector<T> tone(zeros_meter.packet_size());
                    univector<T> silence(zeros_meter.packet_size(), T(0));
                    tone = T(0.3) * sinenorm(phasor<T>(T(1000), 48000));
                    const auto compare = [&]
                    {
                        T a, b, c, d, e, x, y, z, u, v;
                        zeros_meter.get_values(a, b, c, d, e);
                        silence_meter.get_values(x, y, z, u, v);
                        CHECK((a == x || (std::isnan(a) && std::isnan(x))));
                        CHECK((b == y || (std::isnan(b) && std::isnan(y))));
                        CHECK((c == z || (std::isnan(c) && std::isnan(z))));
                        CHECK(d == u);
                        CHECK(e == v);
                    };
                    for (int i = 0; i < 40; ++i)
                    {
                        zeros_meter.process_packet({ tone, tone });
                        silence_meter.process_packet({ tone, tone });
                    }
                    silence_meter.process_silence_packets(0);
                    compare();
                    for (int i = 0; i < 15; ++i)
                    {
                        zeros_meter.process_packet({ silence, silence });
                        silence_meter.process_silence_packets(1);
                        compare();
                    }
                    zeros_meter.stop();
                    silence_meter.stop();
                    for (int i = 0; i < 3; ++i)
                        zeros_meter.process_packet({ silence, silence });
                    silence_meter.process_silence_packets(3);
                    compare();
                    zeros_meter.start();
                    silence_meter.start();
                    zeros_meter.reset();
                    silence_meter.reset();
                    silence_meter.process_silence_packets(0);
                    compare();
                    zeros_meter.process_packet({ silence, silence });
                    silence_meter.process_silence_packets(1);
                    compare();
                    zeros_meter.process_packet({ tone, tone });
                    silence_meter.process_packet({ tone, tone });
                    compare();
                });
}

TEST_CASE("ebu_reset_matches_new_meter")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    const std::array channels{ speaker_type::Left };
                    ebu_r128<T> reused(8000, channels), fresh(8000, channels);
                    univector<T> previous(reused.packet_size(), T(0.75));
                    univector<T> next(reused.packet_size(), T(0));
                    for (int i = 0; i < 17; ++i)
                        reused.process_packet({ previous });
                    reused.reset();
                    for (int i = 0; i < 40; ++i)
                    {
                        reused.process_packet({ next });
                        fresh.process_packet({ next });
                        T rm, rs, ri, rl, rh, fm, fs, fi, fl, fh;
                        reused.get_values(rm, rs, ri, rl, rh);
                        fresh.get_values(fm, fs, fi, fl, fh);
                        CHECK(rm == fm);
                        CHECK(rs == fs);
                        CHECK(ri == fi);
                        CHECK(rl == fl);
                        CHECK(rh == fh);
                    }
                });
}

TEST_CASE("ebu_long_repeated_packets_and_reset")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    constexpr int sample_rate = 8000;
                    ebu_r128<T> loudness(sample_rate,
                                         std::initializer_list<speaker_type>{ speaker_type::Left });
                    univector<T> packet(loudness.packet_size());
                    packet = dB_to_amp(T(-23)) * sinenorm(phasor<T>(T(1000), sample_rate));
                    for (int i = 0; i < 600; ++i)
                        loudness.process_packet({ packet });
                    T M, S, I, RL, RH;
                    loudness.get_values(M, S, I, RL, RH);
                    const T initial_M = M, initial_S = S, initial_I = I, initial_RL = RL, initial_RH = RH;
                    CHECK(std::isfinite(initial_I));
                    CHECK(std::abs(initial_RH - initial_RL) < T(0.21));

                    // Reuse one 100 ms packet for 20 more minutes without allocating a long signal.
                    for (int i = 0; i < 12000; ++i)
                        loudness.process_packet({ packet });
                    loudness.get_values(M, S, I, RL, RH);
                    CHECK(std::abs(M - initial_M) < T(0.11));
                    CHECK(std::abs(S - initial_S) < T(0.11));
                    CHECK(std::abs(I - initial_I) < T(0.11));
                    CHECK(std::abs(RL - initial_RL) < T(0.11));
                    CHECK(std::abs(RH - initial_RH) < T(0.11));

                    loudness.reset();
                    packet = dB_to_amp(T(-33)) * sinenorm(phasor<T>(T(1000), sample_rate));
                    for (int i = 0; i < 1200; ++i)
                        loudness.process_packet({ packet });
                    loudness.get_values(M, S, I, RL, RH);
                    CHECK(std::abs(M - (initial_M - T(10))) < T(0.15));
                    CHECK(std::abs(S - (initial_S - T(10))) < T(0.15));
                    CHECK(std::abs(I - (initial_I - T(10))) < T(0.15));
                    CHECK(std::abs(RL - (initial_RL - T(10))) < T(0.15));
                    CHECK(std::abs(RH - (initial_RH - T(10))) < T(0.15));
                });
}
} // namespace KFR_ARCH_NAME

} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
