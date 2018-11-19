/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

#include <numeric>

using namespace kfr;

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
        CHECK(std::abs(M - refM) < 0.05f);
    if (!std::isnan(refS))
        CHECK(std::abs(S - refS) < 0.05f);
    if (!std::isnan(refI))
        CHECK(std::abs(I - refI) < 0.05f);
    if (!std::isnan(refLRA))
        CHECK(std::abs((RH - RL) - refLRA) < 0.05f);
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
        CHECK(std::abs(M - refM) < 0.05f);
    if (!std::isnan(refS))
        CHECK(std::abs(S - refS) < 0.05f);
    if (!std::isnan(refI))
        CHECK(std::abs(I - refI) < 0.05f);
    if (!std::isnan(refLRA))
        CHECK(std::abs((RH - RL) - refLRA) < 0.05f);
}

TEST(ebu_stereo_1_and_2)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = type_of<decltype(type)>;

                      ebu_test_stereo<T>(sample_rate, { { -23.f, 20.f, 1000.f } }, -23.f, -23.f, -23.f, NAN);
                      ebu_test_stereo<T>(sample_rate, { { -33.f, 20.f, 1000.f } }, -33.f, -33.f, -33.f, NAN);
                  });
}

TEST(ebu_stereo_3_4_and_5)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = type_of<decltype(type)>;

                      ebu_test_stereo<T>(
                          sample_rate,
                          { { -36.f, 10.f, 1000.f }, { -23.f, 60.f, 1000.f }, { -36.f, 10.f, 1000.f } }, NAN,
                          NAN, -23.f, NAN);
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
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = type_of<decltype(type)>;

                      ebu_test_multichannel<T>(sample_rate, { { -28.f, -24.f, -30.f, 20.f, 1000.f } }, NAN,
                                               NAN, -23.f, NAN);
                  });
}

TEST(ebu_stereo_9)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = type_of<decltype(type)>;

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
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = type_of<decltype(type)>;

                      ebu_test_stereo<T>(
                          sample_rate,
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
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = type_of<decltype(type)>;

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

TEST(delay)
{
    const univector<float, 33> v1 = counter() + 100;
    CHECK_EXPRESSION(delay(v1), 33, [](size_t i) { return i < 1 ? 0.f : (i - 1) + 100.f; });

    CHECK_EXPRESSION(delay<3>(v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });
}

TEST(fracdelay)
{
    univector<double, 5> a({ 1, 2, 3, 4, 5 });
    univector<double, 5> b = fracdelay(a, 0.5);
    CHECK(rms(b - univector<double>({ 0.5, 1.5, 2.5, 3.5, 4.5 })) < constants<double>::epsilon * 5);

    b = fracdelay(a, 0.1);
    CHECK(rms(b - univector<double>({ 0.9, 1.9, 2.9, 3.9, 4.9 })) < constants<double>::epsilon * 5);

    b = fracdelay(a, 0.0);
    CHECK(rms(b - univector<double>({ 1, 2, 3, 4, 5 })) < constants<double>::epsilon * 5);

    b = fracdelay(a, 1.0);
    CHECK(rms(b - univector<double>({ 0, 1, 2, 3, 4 })) < constants<double>::epsilon * 5);
}

TEST(mixdown)
{
    CHECK_EXPRESSION(mixdown(counter(), counter() * 2 + 100), infinite_size,
                     [](size_t i) { return i + i * 2 + 100; });
}

#ifdef CMT_COMPILER_CLANG
TEST(mixdown_stereo)
{
    const univector<double, 21> left  = counter();
    const univector<double, 21> right = counter() * 2 + 100;
    univector<double, 21> mid;
    univector<double, 21> side;
    unpack(mid, side) = mixdown_stereo(left, right, matrix_sum_diff());

    CHECK_EXPRESSION(mid, 21, [](size_t i) { return i + i * 2.0 + 100.0; });
    CHECK_EXPRESSION(side, 21, [](size_t i) { return i - (i * 2.0 + 100.0); });
}
#endif

TEST(phasor)
{
    constexpr fbase sr       = 44100.0;
    univector<fbase, 100> v1 = sinenorm(phasor(15000, sr));
    univector<fbase, 100> v2 = sin(constants<fbase>::pi_s(2) * counter(0, 15000 / sr));
    CHECK(rms(v1 - v2) < 1.e-5);
}

TEST(fir)
{
    const univector<double, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
    const univector<double, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

    CHECK_EXPRESSION(fir(data, taps), 100, [&](size_t index) -> double {
        double result = 0.0;
        for (size_t i = 0; i < taps.size(); i++)
            result += data.get(index - i, 0.0) * taps[i];
        return result;
    });

    CHECK_EXPRESSION(short_fir(data, taps), 100, [&](size_t index) -> double {
        double result = 0.0;
        for (size_t i = 0; i < taps.size(); i++)
            result += data.get(index - i, 0.0) * taps[i];
        return result;
    });
}

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version());
    return testo::run_all("", true);
}
#endif
