/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

#include <complex>
#include <numeric>

using namespace kfr;

namespace CMT_ARCH_NAME
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
                      using T = typename decltype(type)::type;

                      ebu_test_stereo<T>(sample_rate, { { -23.f, 20.f, 1000.f } }, -23.f, -23.f, -23.f, NAN);
                      ebu_test_stereo<T>(sample_rate, { { -33.f, 20.f, 1000.f } }, -33.f, -33.f, -33.f, NAN);
                  });
}

TEST(ebu_stereo_3_4_and_5)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = typename decltype(type)::type;

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
                      using T = typename decltype(type)::type;

                      ebu_test_multichannel<T>(sample_rate, { { -28.f, -24.f, -30.f, 20.f, 1000.f } }, NAN,
                                               NAN, -23.f, NAN);
                  });
}

TEST(ebu_stereo_9)
{
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
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
    testo::matrix(named("type")        = ctypes_t<float, double>{},
                  named("sample_rate") = std::vector<int>{ 44100, 48000 }, [](auto type, int sample_rate) {
                      using T = typename decltype(type)::type;

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

TEST(note_to_hertz)
{
    testo::eplison_scope<void> eps(1000);
    CHECK(kfr::note_to_hertz(60) == fbase(261.6255653005986346778499935233));
    CHECK(kfr::note_to_hertz(pack(60)) == pack(fbase(261.6255653005986346778499935233)));

    CHECK(kfr::note_to_hertz(69) == fbase(440.0));
    CHECK(kfr::note_to_hertz(pack(69)) == pack(fbase(440)));
}

TEST(hertz_to_note)
{
    testo::eplison_scope<void> eps(1000);
    CHECK(kfr::hertz_to_note(261.6255653005986346778499935233) == fbase(60));
    CHECK(kfr::hertz_to_note(pack(261.6255653005986346778499935233)) == pack(fbase(60)));

    CHECK(kfr::hertz_to_note(440) == fbase(69));
    CHECK(kfr::hertz_to_note(pack(440)) == pack(fbase(69)));
}

TEST(amp_to_dB)
{
    testo::eplison_scope<void> eps(1000);

    CHECK(kfr::amp_to_dB(fbase(2.0)) == fbase(6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(-2.0)) == fbase(6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(1.0)) == fbase(0));
    CHECK(kfr::amp_to_dB(fbase(-1.0)) == fbase(0));
    CHECK(kfr::amp_to_dB(fbase(0.5)) == fbase(-6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(-0.5)) == fbase(-6.0205999132796239042747778944899));
    CHECK(kfr::amp_to_dB(fbase(0.0)) == fbase(-HUGE_VAL));
}

TEST(dB_to_amp)
{
    testo::eplison_scope<void> eps(1000);

    CHECK(kfr::dB_to_amp(fbase(-HUGE_VAL)) == fbase(0.0));
    CHECK(kfr::dB_to_amp(fbase(0.0)) == fbase(1.0));
    CHECK(kfr::dB_to_amp(fbase(6.0205999132796239042747778944899)) == fbase(2.0));
    CHECK(kfr::dB_to_amp(fbase(-6.0205999132796239042747778944899)) == fbase(0.5));
}

TEST(delay)
{
    const univector<float, 33> v1 = counter() + 100;
    CHECK_EXPRESSION(delay(v1), 33, [](size_t i) { return i < 1 ? 0.f : (i - 1) + 100.f; });

    CHECK_EXPRESSION(delay<3>(v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });

    delay_state<float, 3> state1;
    CHECK_EXPRESSION(delay(state1, v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });

    delay_state<float, 3, tag_dynamic_vector> state2;
    CHECK_EXPRESSION(delay(state2, v1), 33, [](size_t i) { return i < 3 ? 0.f : (i - 3) + 100.f; });
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

TEST(phasor)
{
    constexpr fbase sr       = 44100.0;
    univector<fbase, 100> v1 = sinenorm(phasor(15000, sr));
    univector<fbase, 100> v2 = sin(constants<fbase>::pi_s(2) * counter(0, 15000 / sr));
    CHECK(rms(v1 - v2) < 1.e-5);
}

TEST(fir)
{
#ifdef CMT_COMPILER_MSVC
    // testo::matrix causes error in MSVC
    {
        using T = float;

        const univector<T, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
        const univector<T, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

        CHECK_EXPRESSION(fir(data, taps), 100, [&](size_t index) -> T {
            T result = 0;
            for (size_t i = 0; i < taps.size(); i++)
                result += data.get(index - i, 0) * taps[i];
            return result;
        });

        CHECK_EXPRESSION(short_fir(data, taps), 100, [&](size_t index) -> T {
            T result = 0;
            for (size_t i = 0; i < taps.size(); i++)
                result += data.get(index - i, 0) * taps[i];
            return result;
        });
    }
    {
        using T = double;

        const univector<T, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
        const univector<T, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

        CHECK_EXPRESSION(fir(data, taps), 100, [&](size_t index) -> T {
            T result = 0;
            for (size_t i = 0; i < taps.size(); i++)
                result += data.get(index - i, 0) * taps[i];
            return result;
        });

        CHECK_EXPRESSION(short_fir(data, taps), 100, [&](size_t index) -> T {
            T result = 0;
            for (size_t i = 0; i < taps.size(); i++)
                result += data.get(index - i, 0) * taps[i];
            return result;
        });
    }
#else
    testo::matrix(named("type") = ctypes_t<float
#ifdef CMT_NATIVE_F64
                                           ,
                                           double
#endif
                                           >{},
                  [](auto type) {
                      using T = typename decltype(type)::type;

                      const univector<T, 100> data =
                          counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5);
                      const univector<T, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

                      CHECK_EXPRESSION(fir(data, taps), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < taps.size(); i++)
                              result += data.get(index - i, 0) * taps[i];
                          return result;
                      });

                      fir_state<T> state(taps.ref());

                      CHECK_EXPRESSION(fir(state, data), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < taps.size(); i++)
                              result += data.get(index - i, 0) * taps[i];
                          return result;
                      });

                      CHECK_EXPRESSION(short_fir(data, taps), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < taps.size(); i++)
                              result += data.get(index - i, 0) * taps[i];
                          return result;
                      });

                      short_fir_state<9, T> state2(taps);

                      CHECK_EXPRESSION(short_fir<taps.size()>(state2, data), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < taps.size(); i++)
                              result += data.get(index - i, 0) * taps[i];
                          return result;
                      });

                      CHECK_EXPRESSION(moving_sum<taps.size()>(data), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < taps.size(); i++)
                              result += data.get(index - i, 0);
                          return result;
                      });

                      moving_sum_state<T, 131> msstate1;

                      CHECK_EXPRESSION(moving_sum(msstate1, data), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < msstate1.delayline.size(); i++)
                              result += data.get(index - i, 0);
                          return result;
                      });

                      moving_sum_state<T> msstate2(133);

                      CHECK_EXPRESSION(moving_sum(msstate2, data), 100, [&](size_t index) -> T {
                          T result = 0;
                          for (size_t i = 0; i < msstate2.delayline.size(); i++)
                              result += data.get(index - i, 0);
                          return result;
                      });
                  });
#endif
}

#ifdef CMT_NATIVE_F64
TEST(fir_different)
{
    const univector<float, 100> data = counter() + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5f);
    //    const univector<double, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };
    const univector<double, 4> taps{ 1, 2, 3, 4 };

    CHECK_EXPRESSION(fir(data, taps), 100, [&](size_t index) -> float {
        double result = 0.0;
        for (size_t i = 0; i < taps.size(); i++)
            result += data.get(index - i, 0.0) * taps[i];
        return float(result);
    });

    CHECK_EXPRESSION(short_fir(data, taps), 100, [&](size_t index) -> float {
        double result = 0.0;
        for (size_t i = 0; i < taps.size(); i++)
            result += data.get(index - i, 0.0) * taps[i];
        return float(result);
    });
}
#endif

#ifdef KFR_STD_COMPLEX
template <typename T>
inline std::complex<T> to_std(const std::complex<T>& c)
{
    return c;
}
template <typename T>
inline std::complex<T> from_std(const std::complex<T>& c)
{
    return c;
}
#else
template <typename T>
inline std::complex<T> to_std(const kfr::complex<T>& c)
{
    return { c.real(), c.imag() };
}

template <typename T>
inline kfr::complex<T> from_std(const std::complex<T>& c)
{
    return { c.real(), c.imag() };
}
#endif

TEST(fir_complex)
{
    const univector<complex<float>, 100> data =
        counter() * complex<float>{ 0.f, 1.f } + sequence(1, 2, -10, 100) + sequence(0, -7, 0.5f);
    const univector<float, 6> taps{ 1, 2, -2, 0.5, 0.0625, 4 };

    CHECK_EXPRESSION(fir(data, taps), 100, [&](size_t index) -> complex<float> {
        std::complex<float> result = 0.0;
        for (size_t i = 0; i < taps.size(); i++)
            result = result + to_std(data.get(index - i, 0.0)) * taps[i];
        return from_std(result);
    });

    CHECK_EXPRESSION(short_fir(data, taps), 100, [&](size_t index) -> complex<float> {
        std::complex<float> result = 0.0;
        for (size_t i = 0; i < taps.size(); i++)
            result = result + to_std(data.get(index - i, 0.0)) * taps[i];
        return from_std(result);
    });
}

template <typename E, typename T, size_t size>
void test_ir(E&& e, const univector<T, size>& test_vector)
{
    substitute(e, to_pointer(unitimpulse<T>()));
    const univector<T, size> ir = e;
    println(absmaxof(ir - test_vector));
}

template <typename T, typename... Ts, univector_tag Tag>
inline const univector<T, Tag>& choose_array(const univector<T, Tag>& array, const univector<Ts, Tag>&...)
{
    return array;
}

template <typename T, typename T2, typename... Ts, univector_tag Tag, KFR_ENABLE_IF(!is_same<T, T2>)>
inline const univector<T, Tag>& choose_array(const univector<T2, Tag>&, const univector<Ts, Tag>&... arrays)
{
    return choose_array<T>(arrays...);
}

TEST(biquad_lowpass1)
{
    testo::matrix(named("type") = ctypes_t<float, double>{}, [](auto type) {
        using T = typename decltype(type)::type;

        const biquad_params<T> bq = biquad_lowpass<T>(0.1, 0.7);

        constexpr size_t size = 32;

        const univector<float, size> test_vector_f32{
            +0x8.9bce2p-7,  +0xd.8383ep-6,  +0x8.f908dp-5,  +0xe.edc21p-6,  +0x9.ae104p-6,  +0x9.dcc24p-7,
            +0xd.50584p-9,  -0xf.2668p-13,  -0xd.09ca1p-10, -0xe.15995p-10, -0xa.b90d2p-10, -0xc.edea4p-11,
            -0xb.f14eap-12, -0xc.2cb44p-14, +0xb.4a4dep-15, +0xb.685dap-14, +0xa.b181fp-14, +0xf.0cb2bp-15,
            +0x8.695d6p-15, +0xd.bedd4p-17, +0xf.5474p-20,  -0xd.bb266p-19, -0x9.63ca1p-18, -0xf.ca567p-19,
            -0xa.5231p-19,  -0xa.9e934p-20, -0xe.ab52p-22,  +0xa.3c4cp-26,  +0xd.721ffp-23, +0xe.ccc1ap-23,
            +0xb.5f248p-23, +0xd.d2c9ap-24,
        };

        const univector<double, size> test_vector_f64{
            +0x8.9bce2bf3663e8p-7,  +0xd.8384010fdf1dp-6,   +0x8.f908e7a36df6p-5,   +0xe.edc2332a6d0bp-6,
            +0x9.ae104af1da9ap-6,   +0x9.dcc235ef68e7p-7,   +0xd.5057ee425e05p-9,   -0xf.266e42a99aep-13,
            -0xd.09cad73642208p-10, -0xe.1599f32a83dp-10,   -0xa.b90d8910a117p-10,  -0xc.edeaabb890948p-11,
            -0xb.f14edbb55383p-12,  -0xc.2cb39b86f2dap-14,  +0xb.4a506ecff055p-15,  +0xb.685edfdb55358p-14,
            +0xa.b182e32f8e298p-14, +0xf.0cb3dfd894b2p-15,  +0x8.695df725b4438p-15, +0xd.beddc3606b9p-17,
            +0xf.547004d20874p-20,  -0xd.bb29b25b49b6p-19,  -0x9.63cb9187da1dp-18,  -0xf.ca588634fc618p-19,
            -0xa.52322d320da78p-19, -0xa.9e9420154e4p-20,   -0xe.ab51f7b0335ap-22,  +0xa.3c6479980e1p-26,
            +0xd.7223836599fp-23,   +0xe.ccc47ddd18678p-23, +0xb.5f265b1be1728p-23, +0xd.d2cb83f8483f8p-24,
        };

        const univector<T, size> ir = biquad(bq, unitimpulse<T>());

        CHECK(absmaxof(choose_array<T>(test_vector_f32, test_vector_f64) - ir) == 0);
    });
}

TEST(biquad_lowpass2)
{
    testo::matrix(named("type") = ctypes_t<float, double>{}, [](auto type) {
        using T = typename decltype(type)::type;

        const biquad_params<T> bq = biquad_lowpass<T>(0.45, 0.2);

        constexpr size_t size = 32;

        const univector<float, size> test_vector_f32{
            +0x8.ce416p-4,  +0x8.2979p-4,   -0x8.a9d04p-7,  +0xe.aeb3p-11,  +0x8.204f8p-13, -0x8.20d78p-12,
            +0x8.3379p-12,  -0xf.83d81p-13, +0xe.8b5c4p-13, -0xd.9ddadp-13, +0xc.bedfcp-13, -0xb.ee123p-13,
            +0xb.2a9e5p-13, -0xa.73ac4p-13, +0x9.c86f6p-13, -0x9.2828p-13,  +0x8.92229p-13, -0x8.05b7p-13,
            +0xf.048ffp-14, -0xe.0e849p-14, +0xd.28384p-14, -0xc.50a9p-14,  +0xb.86e56p-14, -0xa.ca0b6p-14,
            +0xa.19476p-14, -0x9.73d38p-14, +0x8.d8f64p-14, -0x8.48024p-14, +0xf.80aa2p-15, -0xe.82ad8p-15,
            +0xd.94f22p-15, -0xc.b66d9p-15,
        };

        const univector<double, size> test_vector_f64{
            +0x8.ce416c0d31e88p-4,  +0x8.2978efe51dafp-4,   -0x8.a9d088b81da6p-7,   +0xe.aeb56c029358p-11,
            +0x8.20492639873ap-13,  -0x8.20d4e21aab538p-12, +0x8.3376b2d53b4a8p-12, -0xf.83d3d1c17343p-13,
            +0xe.8b584f0dd5ac8p-13, -0xd.9dd740ceaacf8p-13, +0xc.bedc85e7a621p-13,  -0xb.ee0f472bf8968p-13,
            +0xb.2a9baed1fe6cp-13,  -0xa.73a9d1670f4ep-13,  +0x9.c86d29d297798p-13, -0x9.2825f4d894088p-13,
            +0x8.9220a956d651p-13,  -0x8.05b539fdd79e8p-13, +0xf.048cb5194cfa8p-14, -0xe.0e819fa128938p-14,
            +0xd.2835957d684cp-14,  -0xc.50a69c2a8dc18p-14, +0xb.86e33bbaf3cbp-14,  -0xa.ca097058af2cp-14,
            +0xa.1945ad1703dcp-14,  -0x9.73d1eef7d8b68p-14, +0x8.d8f4df1bb3efp-14,  -0x8.48010323c6f7p-14,
            +0xf.80a7f5baeeb2p-15,  -0xe.82ab94bb68a8p-15,  +0xd.94f05f80af008p-15, -0xc.b66c0799b21a8p-15,
        };

        const univector<T, size> ir = biquad(bq, unitimpulse<T>());

        CHECK(absmaxof(choose_array<T>(test_vector_f32, test_vector_f64) - ir) == 0);
    });
}

TEST(resampler_test)
{
    const int in_sr  = 44100;
    const int out_sr = 48000;
    const int freq   = 100;
    auto resampler   = sample_rate_converter<fbase>(resample_quality::draft, out_sr, in_sr);
    double delay     = resampler.get_fractional_delay();
    univector<fbase> out(out_sr / 10);
    univector<fbase> in  = truncate(sin(c_pi<fbase> * phasor<fbase>(freq, in_sr, 0)), in_sr / 10);
    univector<fbase> ref = truncate(
        sin(c_pi<fbase> * phasor<fbase>(freq, out_sr, -delay * (static_cast<double>(freq) / out_sr))),
        out_sr / 10);
    resampler.process(out, in);

    CHECK(rms(slice(out - ref, static_cast<size_t>(ceil(delay * 2)))) < 0.005f);
}
TEST(resampler_test_complex)
{
    using type = complex<fbase>;
    const int in_sr  = 44100;
    const int out_sr = 48000;
    const int freq   = 100;
    auto resampler   = sample_rate_converter<type>(resample_quality::draft, out_sr, in_sr);
    double delay     = resampler.get_fractional_delay();
    univector<type> out(out_sr / 10);
    univector<type> in  = truncate(sin(c_pi<fbase> * phasor<fbase>(freq, in_sr, 0)), in_sr / 10);
    univector<type> ref = truncate(
        sin(c_pi<fbase> * phasor<fbase>(freq, out_sr, -delay * (static_cast<double>(freq) / out_sr))),
        out_sr / 10);
    resampler.process(out, in);

    CHECK(rms(cabs(slice(out - ref, static_cast<size_t>(ceil(delay * 2))))) < 0.005f);
}
} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main(int argc, char* argv[])
{
    println(library_version());
    return testo::run_all(argc > 1 ? argv[1] : "", true);
}
#endif
