/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
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

TEST(sine_type)
{
    double ph = 0.0;
    using T   = decltype(sine(ph));
    static_assert(std::is_same_v<T, double>);
}

TEST(phasor)
{
    constexpr fbase sr       = 44100.0;
    univector<fbase, 100> v1 = sinenorm(phasor(15000, sr));
    univector<fbase, 100> v2 = sin(constants<fbase>::pi_s(2) * counter(0, 15000 / sr));
    CHECK(rms(v1 - v2) < 1.e-5);
}

template <typename E, typename T, size_t size>
void test_ir(E&& e, const univector<T, size>& test_vector)
{
    substitute(e, to_handle(unitimpulse<T>()));
    const univector<T, size> ir = e;
    println(absmaxof(ir - test_vector));
}

TEST(gen_sin)
{
    kfr::univector<kfr::fbase> x;
    constexpr size_t size = 132;
    kfr::fbase step       = kfr::c_pi<kfr::fbase> / (size + 1);
    kfr::univector<kfr::fbase> up;
    up = kfr::truncate(kfr::gen_sin<kfr::fbase>(kfr::c_pi<kfr::fbase> / 2, step), size);

    kfr::univector<kfr::fbase> up2(size);
    for (int i = 0; i < size; ++i)
    {
        up2[i] = std::sin(kfr::c_pi<kfr::fbase> / 2 + i * step);
    }
    CHECK(rms(up - up2) < 0.00001);
}

} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main(int argc, char* argv[])
{
    println(library_version());
    return testo::run_all(argc > 1 ? argv[1] : "", true);
}
#endif
