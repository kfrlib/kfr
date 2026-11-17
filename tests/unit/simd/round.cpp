/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/round.hpp>
#include <cmath>

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

template <typename T>
void check_fmod_regressions()
{
    const vec<T, 8> dividends{ T(5.5), T(-5.5), T(5.5), T(-5.5),
                               T(0), T(-0.0), T(6), T(-6) };
    const vec<T, 8> divisors{ T(2), T(2), T(-2), T(-2), T(3), T(3), T(-3), T(3) };
    const auto result = kfr::fmod(dividends, divisors);
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

    const T divisor = T(0.1);
    const T below_multiple = std::nextafter(divisor * T(3), T(0));
    CHECK(std::signbit(kfr::fmod(below_multiple, divisor)) == false);
    CHECK(std::signbit(kfr::fmod(-below_multiple, divisor)) == true);
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
