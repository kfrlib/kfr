/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/round.hpp>
#include <cmath>
#include <limits>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("floor")
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::floor(x); }, [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::floor(x)); });
}

TEST_CASE("ceil")
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::ceil(x); }, [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::ceil(x)); });
}

TEST_CASE("trunc")
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::trunc(x); }, [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::trunc(x)); });
}

TEST_CASE("round")
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::round(x); }, [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::round(x)); });
}

TEST_CASE("round half ties and adjacent values")
{
    const auto check = []<typename T>()
    {
        const T below = std::nextafter(T(0.5), T(0));
        const T above = std::nextafter(T(0.5), T(1));
        const vec<T, 8> input{ T(0.5), T(-0.5), T(2.5), T(-2.5), below, -below, above, -above };
        const auto rounded  = kfr::round(input);
        const auto integers = kfr::iround(input);
        for (size_t i = 0; i < 8; ++i)
        {
            const T expected = std::round(input[i]);
            CHECK(rounded[i] == expected);
            CHECK(std::signbit(rounded[i]) == std::signbit(expected));
            CHECK(integers[i] == static_cast<itype<T>>(expected));
            CHECK(kfr::round(input[i]) == expected);
        }
    };
    check.template operator()<f32>();
    check.template operator()<f64>();
}

TEST_CASE("trunc handles mixed finite and nonfinite lanes")
{
    const auto check = []<typename T>()
    {
        const T inf = std::numeric_limits<T>::infinity();
        const T nan = std::numeric_limits<T>::quiet_NaN();
        const vec<T, 8> input{ T(-0.5), T(1.75), inf, -inf, nan,
                               std::numeric_limits<T>::max(), -std::numeric_limits<T>::max(), T(-2.75) };
        const auto result = kfr::trunc(input);
        for (size_t i = 0; i < 8; ++i)
        {
            CAPTURE(i);
            const T expected = std::trunc(input[i]);
            if (std::isnan(expected))
                CHECK(std::isnan(result[i]));
            else
            {
                CHECK(result[i] == expected);
                CHECK(std::signbit(result[i]) == std::signbit(expected));
            }
        }
    };
    check.template operator()<f32>();
    check.template operator()<f64>();
}

template <typename T>
void check_fmod_regressions()
{
    const vec<T, 8> dividends{ T(5.5), T(-5.5), T(5.5), T(-5.5), T(0), T(-0.0), T(6), T(-6) };
    const vec<T, 8> divisors{ T(2), T(2), T(-2), T(-2), T(3), T(3), T(-3), T(3) };
    const auto result    = kfr::fmod(dividends, divisors);
    const auto remainder = rem(dividends, divisors);
    for (size_t i = 0; i < 8; ++i)
    {
        const T expected = std::fmod(dividends[i], divisors[i]);
        CHECK(result[i] == expected);
        CHECK(remainder[i] == expected);
        CHECK(std::signbit(result[i]) == std::signbit(expected));
        CHECK(std::signbit(remainder[i]) == std::signbit(expected));
    }
    CHECK(std::signbit(kfr::fmod(T(-6), T(3))));
    CHECK(std::signbit(kfr::fmod(T(-0.0), T(3))));

    const T divisor  = T(0.1);
    const T multiple = divisor * T(5);
    const T below    = std::nextafter(multiple, T(0));
    const T above    = std::nextafter(multiple, T(1));
    const T inf      = std::numeric_limits<T>::infinity();
    const vec<T, 8> xs{ below, -below, above, -above, T(1), T(1), T(1), T(1) };
    const vec<T, 8> ys{
        divisor, divisor, divisor, -divisor, divisor, std::numeric_limits<T>::min(), inf, T(0)
    };
    const auto f = kfr::fmod(xs, ys);
    const auto r = kfr::rem(xs, ys);
    for (size_t i = 0; i < 7; ++i)
    {
        const T expected = std::fmod(xs[i], ys[i]);
        CHECK(kfr::fmod(xs[i], ys[i]) == expected);
        CHECK(f[i] == expected);
        CHECK(r[i] == expected);
    }
    CHECK(std::isnan(f[7]));
    CHECK(std::isnan(r[7]));
}

TEST_CASE("fmod finite remainders and signed zeros")
{
    check_fmod_regressions<f32>();
    check_fmod_regressions<f64>();
}

TEST_CASE("fract")
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::fract(x); }, [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? 0 : static_cast<decltype(x)>(x - std::floor(x)); });
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
