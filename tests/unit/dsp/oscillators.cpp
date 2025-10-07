/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/test/test.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp/oscillators.hpp>

using namespace kfr;

namespace KFR_ARCH_NAME
{

TEST_CASE("sine_type")
{
    double ph = 0.0;
    using T   = decltype(sine(ph));
    static_assert(std::is_same_v<T, double>);
}

TEST_CASE("phasor")
{
    constexpr fbase sr       = 44100.0;
    univector<fbase, 100> v1 = sinenorm(phasor(15000, sr));
    univector<fbase, 100> v2 = sin(constants<fbase>::pi_s(2) * counter(0, 15000 / sr));
    CHECK(rms(v1 - v2) < 1.e-5);
}
} // namespace KFR_ARCH_NAME
