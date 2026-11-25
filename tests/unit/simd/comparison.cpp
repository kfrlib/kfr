/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/comparison.hpp>
#include <kfr/simd/operators.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("comparison binary functions")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return equal(x, y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x == y); });
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return notequal(x, y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x != y); });
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return less(x, y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x < y); });
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return greater(x, y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x > y); });
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return lessorequal(x, y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x <= y); });
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return greaterorequal(x, y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x >= y); });
}

TEST_CASE("scalar byte comparison masks preserve all lanes")
{
    auto check = []<typename T>()
    {
        const vec<T, 16> values{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
        const vec<T, 16> mixed{ 0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0 };
        const auto scalar_equal = [](T left, T right) { return maskbits<T>(left == right); };
        const auto all_true = apply(scalar_equal, values, values);
        const auto alternating = apply(scalar_equal, values, mixed);
        for (size_t index = 0; index < 16; ++index)
        {
            INFO("index = " << index << ", signed = " << std::is_signed_v<T>);
            CHECK(all_true[index] == maskbits<T>(true));
            CHECK(alternating[index] == maskbits<T>(index % 2 == 0));
        }
    };
    check.template operator()<i8>();
    check.template operator()<u8>();
}

TEST_CASE("comparison classification functions")
{
    const vec<f32, 8> x{ 0.0f,
                         -0.0f,
                         1.0f,
                         -1.0f,
                         constants<f32>::infinity,
                         -constants<f32>::infinity,
                         std::numeric_limits<f32>::quiet_NaN(),
                         2.0f };

    CHECK_THAT(ubitcast(isnan(x).asvec()), DeepMatcher(vec<u32, 8>{ 0, 0, 0, 0, 0, 0, ~u32(0), 0 }));
    CHECK_THAT(ubitcast(isinf(x).asvec()), DeepMatcher(vec<u32, 8>{ 0, 0, 0, 0, ~u32(0), ~u32(0), 0, 0 }));
    CHECK_THAT(ubitcast(isfinite(x).asvec()),
               DeepMatcher(vec<u32, 8>{ ~u32(0), ~u32(0), ~u32(0), ~u32(0), 0, 0, 0, ~u32(0) }));
    CHECK_THAT(ubitcast(isnegative(x).asvec()),
               DeepMatcher(vec<u32, 8>{ 0, ~u32(0), 0, ~u32(0), 0, ~u32(0), 0, 0 }));
    CHECK_THAT(ubitcast(ispositive(x).asvec()),
               DeepMatcher(vec<u32, 8>{ ~u32(0), 0, ~u32(0), 0, ~u32(0), 0, ~u32(0), ~u32(0) }));
    CHECK_THAT(ubitcast(iszero(x).asvec()), DeepMatcher(vec<u32, 8>{ ~u32(0), ~u32(0), 0, 0, 0, 0, 0, 0 }));
    CHECK_THAT(ubitcast(inrange(x, -1.0f, 1.0f).asvec()),
               DeepMatcher(vec<u32, 8>{ ~u32(0), ~u32(0), ~u32(0), ~u32(0), 0, 0, 0, 0 }));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
