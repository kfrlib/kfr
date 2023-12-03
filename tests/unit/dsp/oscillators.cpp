/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp/oscillators.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

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
} // namespace CMT_ARCH_NAME
