/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(goertzel)
{
    testo::epsilon_scope<float> e(100);
    univector<float, 16> a;
    a = sinenorm(phasor(0.125f));

    float omega = c_pi<float, 2> * 0.125f;

    complex<float> c;
    process(goertzel(c, omega), a);
    CHECK(cabs(c) == 8.f);

    complex<float> cs[3];
    float omegas[3] = { omega, omega, omega };
    process(goertzel(cs, omegas), a);
    println(cs[0]);
    CHECK(cabs(cs[0]) == 8.f);
    CHECK(cabs(cs[1]) == 8.f);
    CHECK(cabs(cs[2]) == 8.f);
}

} // namespace CMT_ARCH_NAME
