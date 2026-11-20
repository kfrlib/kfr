/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/base/generators.hpp>
#include <kfr/base/math_expressions.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/base/univector.hpp>
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
TEST_CASE("phasor_normalized_preserves_precision")
{
    constexpr double step = 0.125 + 0x1p-40;
    const auto ramp = phasor<double>(step);
    static_assert(std::is_same_v<value_type_of<decltype(ramp)>, double>);
    univector<double, 2> samples = ramp;
    CHECK(samples[1] == step);
}

TEST_CASE("phasor_step")
{
    CHECK(phasor_step<double>(0.75, 1.0, 4.0, 2) == 0.25);
    CHECK(phasor_step<double>(0.25, -1.0, 4.0, 2) == 0.75);
    CHECK(phasor_step<double>(0.25, 1.0, 4.0, 0) == 0.25);
    const double phase = phasor_step<double>(0.125, 3.0, 16.0, 5);
    univector<double, 6> whole = phasor<double>(3.0, 16.0, 0.125);
    CHECK(phase == whole[5]);
}

TEST_CASE("trianglenorm_preserves_offset_for_large_phases")
{
    constexpr double cycles = 0x1p52;
    CHECK(trianglenorm(cycles) == 0.0);
    CHECK(trianglenorm(-cycles) == 0.0);
    CHECK(trianglenorm(cycles / 2 + 0.5) == 0.0);
    CHECK(trianglenorm(0.25) == 1.0);
    CHECK(trianglenorm(0.75) == -1.0);
}
} // namespace KFR_ARCH_NAME
