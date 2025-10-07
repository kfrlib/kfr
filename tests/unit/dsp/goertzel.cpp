/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/test/test.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>

using namespace kfr;

namespace KFR_ARCH_NAME
{

TEST_CASE("goertzel")
{
    epsilon_scope<float> e(100);
    univector<float, 16> a;
    a = sinenorm(phasor(0.125f));

    float omega = c_pi<float, 2> * 0.125f;

    complex<float> c;
    process(goertzel(c, omega), a);
    CHECK_THAT(cabs(c), DeepMatcher(8.f));

    complex<float> cs[3];
    float omegas[3] = { omega, omega, omega };
    process(goertzel(cs, omegas), a);
    println(cs[0]);
    CHECK_THAT(cabs(cs[0]), DeepMatcher(8.f));
    CHECK_THAT(cabs(cs[1]), DeepMatcher(8.f));
    CHECK_THAT(cabs(cs[2]), DeepMatcher(8.f));
}

} // namespace KFR_ARCH_NAME
