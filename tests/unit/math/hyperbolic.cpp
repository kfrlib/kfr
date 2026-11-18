/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/test/test.hpp>

#include <kfr/math/hyperbolic.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("sinh preserves tiny finite inputs")
{
    const vec<f32, 4> xf{ 1e-10f, -1e-10f, 0.01f, -0.01f };
    const vec<f64, 4> xd{ 1e-20, -1e-20, 0.01, -0.01 };
    const auto yf = kfr::sinh(xf);
    const auto yd = kfr::sinh(xd);
    for (size_t i = 0; i < 4; ++i)
    {
        CHECK(std::abs(yf[i] / std::sinh(xf[i]) - 1) < 3e-7f);
        CHECK(std::abs(yd[i] / std::sinh(xd[i]) - 1) < 5e-16);
    }
}

TEST_CASE("hyperbolic functions preserve small lanes")
{
    test_matrix(named("type") = ctypes_t<float, double>{},
                [](auto type)
                {
                    using T = typename decltype(type)::type;
                    const vec<T, 4> x{ T(1e-10), T(-1e-10), T(0.0125), T(-0.0125) };
                    const auto c      = kfr::cosh(x);
                    const auto t      = kfr::tanh(x);
                    const auto q      = kfr::coth(x);
                    const auto sc     = kfr::sinhcosh(x);
                    const auto cs     = kfr::coshsinh(x);
                    const T tolerance = T(4) * std::numeric_limits<T>::epsilon();
                    for (size_t i = 0; i < x.size(); ++i)
                    {
                        const T sref = std::sinh(x[i]);
                        const T cref = std::cosh(x[i]);
                        const T tref = std::tanh(x[i]);
                        CHECK(std::abs(c[i] / cref - 1) < tolerance);
                        CHECK(std::abs(t[i] / tref - 1) < tolerance);
                        CHECK(std::abs(q[i] * tref - 1) < tolerance);
                        CHECK(std::abs(sc[i] / (i % 2 == 0 ? sref : cref) - 1) < tolerance);
                        CHECK(std::abs(cs[i] / (i % 2 == 0 ? cref : sref) - 1) < tolerance);
                    }
                });
}

TEST_CASE("tanh and coth saturate for large finite inputs")
{
    const vec<f32, 4> x{ 100.0f, -100.0f, 1.0f, -1.0f };
    const auto t = kfr::tanh(x);
    const auto c = kfr::coth(x);
    for (size_t i = 0; i < 4; ++i)
    {
        CHECK(std::abs(t[i] - std::tanh(x[i])) < 1e-6f);
        CHECK(std::abs(c[i] - 1.0f / std::tanh(x[i])) < 1e-6f);
    }
}
} // namespace KFR_ARCH_NAME

} // namespace kfr
