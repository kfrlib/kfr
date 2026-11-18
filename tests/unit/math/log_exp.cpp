/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */
#include <kfr/test/test.hpp>

#include <kfr/math/gamma.hpp>
#include <kfr/math/log_exp.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("exp_nonfinite_mixed_lanes")
{
    const auto check = []<typename T>()
    {
        const auto result = kfr::exp(pack(-c_infinity<T>, T(0), c_infinity<T>, c_qnan<T>));
        CHECK(result[0] == T(0));
        CHECK(result[1] == T(1));
        CHECK(result[2] == c_infinity<T>);
        CHECK(std::isnan(result[3]));
    };
    check.operator()<float>();
    check.operator()<double>();
}

TEST_CASE("gamma_known_values")
{
    CHECK(kfr::gamma(0.5f) == Catch::Approx(1.772453850905516).epsilon(2e-4));
    CHECK(kfr::gamma(0.5) == Catch::Approx(1.772453850905516).epsilon(1e-12));
    const vec<f64, 4> inputs{ 1.0, 2.0, 3.0, 6.0 };
    const auto result = kfr::gamma(inputs);
    CHECK(result[0] == Catch::Approx(1.0).epsilon(1e-12));
    CHECK(result[1] == Catch::Approx(1.0).epsilon(1e-12));
    CHECK(result[2] == Catch::Approx(2.0).epsilon(1e-12));
    CHECK(result[3] == Catch::Approx(120.0).epsilon(1e-12));
}

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
