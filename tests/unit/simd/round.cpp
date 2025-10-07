/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/round.hpp>

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

TEST_CASE("fract")
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::fract(x); }, [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? 0 : static_cast<decltype(x)>(x - std::floor(x)); });
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
