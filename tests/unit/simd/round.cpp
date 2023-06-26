/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/round.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(floor)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::floor(x); },
        [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::floor(x)); });
}

TEST(ceil)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::ceil(x); },
        [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::ceil(x)); });
}

TEST(trunc)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::trunc(x); },
        [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::trunc(x)); });
}

TEST(round)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::round(x); },
        [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? x : static_cast<decltype(x)>(std::round(x)); });
}

TEST(fract)
{
    test_function1(
        test_catogories::all, [](auto x) { return kfr::fract(x); },
        [](auto x) -> decltype(x)
        { return std::is_integral<decltype(x)>::value ? 0 : static_cast<decltype(x)>(x - std::floor(x)); });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
