/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */
#include "../numeric_tests.hpp"

#include <kfr/math/log_exp.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
KFR_AUTO_TEST_1(gamma, narrow, 2200, 321)
KFR_AUTO_TEST_1(exp, narrow, 4, 2)
KFR_AUTO_TEST_1(exp2, narrow, 5, 2)
KFR_AUTO_TEST_1(exp10, narrow, 40, 10)
KFR_AUTO_TEST_1(log, narrow, 2, 1)
KFR_AUTO_TEST_1(log2, narrow, 2, 1)
KFR_AUTO_TEST_1(log10, narrow, 3, 1)
KFR_AUTO_TEST_1(cbrt, narrow, 5, 1)

TEST_CASE("gamma_finite_after_intermediate_overflow")
{
    const vec<f32, 2> xf(5.f, 30.f);
    const auto yf = kfr::gamma(xf);
    CHECK(yf[0] == Catch::Approx(std::tgamma(5.f)).epsilon(0.001));
    CHECK(yf[1] == Catch::Approx(std::tgamma(30.f)).epsilon(0.001));

    const vec<f64, 2> xd(5.0, 150.0);
    const auto yd = kfr::gamma(xd);
    CHECK(yd[0] == Catch::Approx(std::tgamma(5.0)).epsilon(1e-10));
    CHECK(yd[1] == Catch::Approx(std::tgamma(150.0)).epsilon(1e-10));
}

TEST_CASE("cbrt")
{
    CHECK(kfr::cbrt(27.f) == 3.f);
    CHECK(kfr::cbrt(-27.f) == -3.f);

    CHECK(kfr::root(32.f, 5.f) == 2.f);
    CHECK(kfr::root(-32.f, 5.f) == -2.f);
    CHECK(std::isnan(kfr::root(-32.f, 5.001f)));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
