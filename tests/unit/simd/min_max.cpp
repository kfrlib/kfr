/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/min_max.hpp>

#include <kfr/io/tostring.hpp>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4146))

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("min")
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::min(x, y); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x <= y ? x : y; });
}

TEST_CASE("max")
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::max(x, y); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x >= y ? x : y; });
}

TEST_CASE("min max and scalar apply preserve all byte lanes")
{
    auto check = []<typename T>()
    {
        const vec<T, 16> x{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        const vec<T, 16> y{ 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
        const vec<T, 16> z{ 1 };
        const auto scalar_min = [](T a, T b) { return a <= b ? a : b; };
        const auto scalar_max = [](T a, T b) { return a >= b ? a : b; };
        const auto min_result = kfr::min(x, y);
        const auto max_result = kfr::max(x, y);
        const auto min_ref    = apply(scalar_min, x, y);
        const auto max_ref    = apply(scalar_max, x, y);
        const auto with_three = apply([](T a, T b, T c) { return static_cast<T>(a + b + c); }, x, y, z);
        for (size_t i = 0; i < 16; ++i)
        {
            INFO("index = " << i << ", signed = " << std::is_signed_v<T>);
            CHECK(min_result[i] == scalar_min(x[i], y[i]));
            CHECK(max_result[i] == scalar_max(x[i], y[i]));
            CHECK(min_ref[i] == scalar_min(x[i], y[i]));
            CHECK(max_ref[i] == scalar_max(x[i], y[i]));
            CHECK(with_three[i] == static_cast<T>(x[i] + y[i] + z[i]));
        }
    };
    check.template operator()<i8>();
    check.template operator()<u8>();
}

TEST_CASE("scalar apply byte lanes with a converted result")
{
    const i32x16 x{ -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7 };
    const auto convert = [](i32 value) { return static_cast<i8>(value * 2); };
    const auto result = apply(convert, x);
    for (size_t i = 0; i < 16; ++i)
        CHECK(result[i] == convert(x[i]));
}

struct IsNotMinInt
{
    template <typename T>
    bool operator()(ctype_t<T>, std::type_identity_t<T> x, std::type_identity_t<T> y) const
    {
        return std::is_floating_point_v<T> || std::is_unsigned_v<T> ||
               (x != std::numeric_limits<T>::min() && y != std::numeric_limits<T>::min());
    }
    template <typename T, size_t N>
    bool operator()(ctype_t<vec<T, N>>, std::type_identity_t<T> x, std::type_identity_t<T> y) const
    {
        return operator()(kfr::ctype<T>, x, y);
    }
};

TEST_CASE("absmin")
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::absmin(x, y); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            const auto abs_x = x >= 0 ? x : -x;
            const auto abs_y = y >= 0 ? y : -y;
            return abs_x <= abs_y ? abs_x : abs_y;
        },
        IsNotMinInt{});
}

TEST_CASE("absmax")
{
    test_function2(
        test_catogories::all, [](auto x, auto y) { return kfr::absmax(x, y); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            const auto abs_x = x >= 0 ? x : -x;
            const auto abs_y = y >= 0 ? y : -y;
            return abs_x >= abs_y ? abs_x : abs_y;
        },
        IsNotMinInt{});
}
} // namespace KFR_ARCH_NAME
} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
