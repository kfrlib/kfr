/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/math/min_max.hpp>

#include <kfr/io.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(min)
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::min(x, y); },
        [](auto x, auto y) -> common_type<decltype(x), decltype(y)> { return x <= y ? x : y; });
}

TEST(max)
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::max(x, y); },
        [](auto x, auto y) -> common_type<decltype(x), decltype(y)> { return x >= y ? x : y; });
}

struct IsNotMinInt
{
    template <typename T>
    bool operator()(ctype_t<T>, identity<T> x, identity<T> y) const
    {
        return is_floating_point<T> || is_unsigned<T> ||
               (x != std::numeric_limits<T>::min() && y != std::numeric_limits<T>::min());
    }
    template <typename T, size_t N>
    bool operator()(ctype_t<vec<T, N>>, identity<T> x, identity<T> y) const
    {
        return operator()(ctype<T>, x, y);
    }
};

TEST(absmin)
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::absmin(x, y); },
        [](auto x, auto y) -> common_type<decltype(x), decltype(y)> {
            x = x >= 0 ? x : -x;
            y = y >= 0 ? y : -y;
            return x <= y ? x : y;
        },
        IsNotMinInt{});
}

TEST(absmax)
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::absmax(x, y); },
        [](auto x, auto y) -> common_type<decltype(x), decltype(y)> {
            x = x >= 0 ? x : -x;
            y = y >= 0 ? y : -y;
            return x >= y ? x : y;
        },
        IsNotMinInt{});
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
