/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/math/min_max.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(min)
{
    test_function2(test_catogories::all, [](auto x, auto y) { return kfr::min(x, y); },
                   [](auto x, auto y) -> common_type<decltype(x), decltype(y)> { return x <= y ? x : y; });
}

TEST(max)
{
    test_function2(test_catogories::all, [](auto x, auto y) { return kfr::max(x, y); },
                   [](auto x, auto y) -> common_type<decltype(x), decltype(y)> { return x >= y ? x : y; });
}

TEST(absmin)
{
    test_function2(test_catogories::all, [](auto x, auto y) { return kfr::absmin(x, y); },
                   [](auto x, auto y) -> common_type<decltype(x), decltype(y)> {
                       x = x >= 0 ? x : -x;
                       y = y >= 0 ? y : -y;
                       return x <= y ? x : y;
                   });
}

TEST(absmax)
{
    test_function2(test_catogories::all, [](auto x, auto y) { return kfr::absmax(x, y); },
                   [](auto x, auto y) -> common_type<decltype(x), decltype(y)> {
                       x = x >= 0 ? x : -x;
                       y = y >= 0 ? y : -y;
                       return x >= y ? x : y;
                   });
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
