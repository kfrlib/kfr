/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */


#include <kfr/base/generators.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/dsp/goertzel.hpp>
#include <kfr/dsp/oscillators.hpp>
#include <kfr/math/complex_math.hpp>
#include <kfr/simd/complex.hpp>
#include <kfr/simd/constants.hpp>

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

TEST_CASE("goertzel_reset")
{
    const float omega = c_pi<float, 2> * 0.125f;

    SECTION("single bin")
    {
        complex<float> result;
        auto expression = goertzel(result, omega);
        expression.q0  = 1.f;
        expression.q1  = 2.f;
        expression.q2  = 3.f;

        reset(expression);

        CHECK(expression.q0 == 0.f);
        CHECK(expression.q1 == 0.f);
        CHECK(expression.q2 == 0.f);
        CHECK(expression.coeff == 2.f * cos(omega));
    }

    SECTION("parallel bins")
    {
        complex<float> result[2];
        const float omegas[2] = { omega, omega };
        auto expression = goertzel(result, omegas);
        expression.q0  = vec<float, 2>{ 1.f, 2.f };
        expression.q1  = vec<float, 2>{ 3.f, 4.f };
        expression.q2  = vec<float, 2>{ 5.f, 6.f };

        reset(expression);

        for (size_t i = 0; i < 2; ++i)
        {
            CHECK(expression.q0[i] == 0.f);
            CHECK(expression.q1[i] == 0.f);
            CHECK(expression.q2[i] == 0.f);
            CHECK(expression.coeff[i] == 2.f * cos(omega));
        }
    }
}

} // namespace KFR_ARCH_NAME
