/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

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
    constexpr fbase sr = 44100.0;
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

int main()
{
    println(library_version());
    return testo::run_all("", true);
}
