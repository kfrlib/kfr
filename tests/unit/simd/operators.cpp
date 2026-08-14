/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>
#include <kfr/simd/horizontal.hpp>
#include <kfr/simd/operators.hpp>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4146))

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("neg")
{
    test_function1(
        test_catogories::vectors, [](auto x) -> decltype(x) { return -x; },
        [](auto x) -> decltype(x) { return -x; });
}

TEST_CASE("bnot")
{
    test_function1(
        test_catogories::vectors, [](auto x) -> decltype(x) { return ~x; },
        [](auto x) -> decltype(x)
        {
            utype<decltype(x)> u = ~ubitcast(x);
            return bitcast<decltype(x)>(u);
        });
}

TEST_CASE("add")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x + y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x + y; });
}

TEST_CASE("sub")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x - y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x - y; });
}

TEST_CASE("mul")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x * y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x * y; });
}

template <typename T>
inline bool is_safe_division(T x, T y)
{
    return y != T(0) && !(std::is_signed<T>::value && x == std::numeric_limits<T>::min() && y == T(-1));
}

TEST_CASE("div")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y)
        { return is_safe_division<subtype<decltype(x)>>(x.front(), y.front()) ? x / y : 0; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return is_safe_division(x, y) ? x / y : 0; });
}
struct not_f
{
    template <typename T>
    constexpr bool operator()(ctype_t<T>) const
    {
        return !is_f_class<subtype<T>>;
    }
};
TEST_CASE("mod")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y)
        { return is_safe_division<subtype<decltype(x)>>(x.front(), y.front()) ? x % y : 0; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return is_safe_division(x, y) ? x % y : 0; }, fn_return_constant<bool, true>{}, not_f{});
}

TEST_CASE("bor")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x | y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            using T = std::common_type_t<decltype(x), decltype(y)>;
            return bitcast<T>(static_cast<utype<T>>(ubitcast(T(x)) | ubitcast(T(y))));
        });
}

TEST_CASE("bxor")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x ^ y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            using T = std::common_type_t<decltype(x), decltype(y)>;
            return bitcast<T>(static_cast<utype<T>>(ubitcast(T(x)) ^ ubitcast(T(y))));
        });
}

TEST_CASE("band")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x & y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            using T = std::common_type_t<decltype(x), decltype(y)>;
            return bitcast<T>(static_cast<utype<T>>(ubitcast(T(x)) & ubitcast(T(y))));
        });
}

TEST_CASE("shl")
{
    test_matrix(
        named("type") = test_catogories::types(test_catogories::vectors), named("value1") = special_values(),
        named("shift") = std::vector<unsigned>{ 1, 2, 7, 8, 9, 15, 16, 31, 32, 63, 64 },
        [&](auto type, special_value value, unsigned shift)
        {
            using T = typename decltype(type)::type;
            if (shift < sizeof(subtype<T>) * 8)
            {
                const T x(value);
                CHECK(std::is_same<decltype(x << shift), T>::value);
                CHECK_THAT((x << shift), DeepMatcher(apply(
                                             [=](auto x) -> decltype(x)
                                             {
                                                 return bitcast<decltype(x)>(
                                                     static_cast<uitype<decltype(x)>>(uibitcast(x) << shift));
                                             },
                                             x)));
                CHECK_THAT((x << broadcast<T::scalar_size()>(utype<subtype<T>>(shift))),
                           DeepMatcher(apply(
                               [=](auto x) -> decltype(x)
                               {
                                   return bitcast<decltype(x)>(
                                       static_cast<uitype<decltype(x)>>(uibitcast(x) << shift));
                               },
                               x)));
            }
        });
}

TEST_CASE("shr")
{
    test_matrix(
        named("type") = test_catogories::types(test_catogories::vectors), named("value1") = special_values(),
        named("shift") = std::vector<unsigned>{ 1, 2, 7, 8, 9, 15, 16, 31, 32, 63, 64 },
        [&](auto type, special_value value, unsigned shift)
        {
            using T = typename decltype(type)::type;
            if (shift < sizeof(subtype<T>) * 8)
            {
                const T x(value);
                CHECK(std::is_same<decltype(x >> shift), T>::value);
                CHECK_THAT((x >> shift), DeepMatcher(apply(
                                             [=](auto x) -> decltype(x)
                                             {
                                                 return bitcast<decltype(x)>(
                                                     static_cast<uitype<decltype(x)>>(uibitcast(x) >> shift));
                                             },
                                             x)));
                CHECK_THAT((x >> broadcast<T::scalar_size()>(utype<subtype<T>>(shift))),
                           DeepMatcher(apply(
                               [=](auto x) -> decltype(x)
                               {
                                   return bitcast<decltype(x)>(
                                       static_cast<uitype<decltype(x)>>(uibitcast(x) >> shift));
                               },
                               x)));
            }
        });
}

TEST_CASE("byte shifts preserve lane semantics")
{
    // The byte implementations use 16-bit shifts internally. These values
    // exercise both bytes of every 16-bit word and negative signed operands.
    const vec<u8, 16> unsigned_value(u8(0x81));
    const vec<i8, 16> signed_value(i8(-1));
    const vec<i8, 16> signed_left_value(i8(1));

    for (unsigned shift = 0; shift < 8; ++shift)
    {
        CHECK_THAT((unsigned_value << shift),
                   DeepMatcher(vec<u8, 16>(u8(u8(0x81) << shift))));
        CHECK_THAT((unsigned_value >> shift),
                   DeepMatcher(vec<u8, 16>(u8(u8(0x81) >> shift))));
        CHECK_THAT((signed_left_value << shift),
               DeepMatcher(vec<i8, 16>(i8(u8(1u << shift)))));
        CHECK_THAT((signed_value >> shift), DeepMatcher(vec<i8, 16>(i8(-1))));
    }

    // A signed byte must remain sign-filled once the shift reaches the lane
    // width. This specifically catches implementations that derive the sign
    // mask by shifting a 16-bit 0x8080 pattern.
    CHECK_THAT((signed_value >> unsigned(8)), DeepMatcher(vec<i8, 16>(i8(-1))));
}

TEST_CASE("signed 64-bit right shift keeps the signed result type")
{
    const vec<i64, 2> value(i64(-3));
    CHECK_THAT((value >> unsigned(1)), DeepMatcher(vec<i64, 2>(i64(-2))));
}

TEST_CASE("integer division and modulo by zero are defined")
{
    const vec<i32, 4> value(7);
    const vec<i32, 4> zero(0);
    CHECK_THAT((value / zero), DeepMatcher(vec<i32, 4>(0)));
    CHECK_THAT((value % zero), DeepMatcher(vec<i32, 4>(0)));
}

TEST_CASE("integer arithmetic wraps at the element width")
{
    const vec<u8, 16> umax_value((u8)255);
    const vec<u8, 16> one((u8)1);
    CHECK_THAT((umax_value + one), DeepMatcher(vec<u8, 16>((u8)0)));

    const vec<i32, 4> imax_value(std::numeric_limits<i32>::max());
    CHECK_THAT((imax_value + vec<i32, 4>(1)), DeepMatcher(vec<i32, 4>(std::numeric_limits<i32>::min())));
}

TEST_CASE("integer shifts at or above the element width produce zero")
{
    const vec<u8, 16> value((u8)0xff);
    CHECK_THAT((value << unsigned(8)), DeepMatcher(vec<u8, 16>((u8)0)));
    CHECK_THAT((value >> unsigned(8)), DeepMatcher(vec<u8, 16>((u8)0)));
}

TEST_CASE("eq")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x == y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x == y); });
}

TEST_CASE("ne")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x != y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x != y); });
}

TEST_CASE("ge")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x >= y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x >= y); });
}

TEST_CASE("le")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x <= y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x <= y); });
}

TEST_CASE("gt")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x > y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x > y); });
}

TEST_CASE("lt")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x < y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        { return maskbits<subtype<decltype(x)>>(x < y); });
}

TEST_CASE("horner")
{
    CHECK_THAT((horner(pack(0, 1, 2, 3), 1, 2, 3)), DeepMatcher(pack(1, 6, 17, 34)));
    CHECK_THAT((horner_odd(pack(0, 1, 2, 3), 1, 2, 3)), DeepMatcher(pack(0, 6, 114, 786)));
    CHECK_THAT((horner_even(pack(0, 1, 2, 3), 1, 2, 3)), DeepMatcher(pack(1, 6, 57, 262)));
}

TEST_CASE("matrix")
{
    using i32x2x2 = vec<vec<int, 2>, 2>;
    const i32x2x2 m22{ i32x2{ 1, 2 }, i32x2{ 3, 4 } };
    CHECK_THAT((m22 * 10), DeepMatcher(i32x2x2{ i32x2{ 10, 20 }, i32x2{ 30, 40 } }));

    CHECK_THAT((m22 * i32x2{ -1, 100 }), DeepMatcher(i32x2x2{ i32x2{ -1, 200 }, i32x2{ -3, 400 } }));

    CHECK_THAT((vec{ vec{ 1, 1 }, vec{ 1, 1 } } * vec{ -1, 100 }),
               DeepMatcher(vec{ vec{ -1, 100 }, vec{ -1, 100 } }));
    CHECK_THAT((vec{ vec{ 1, 1 }, vec{ 1, 1 }, vec{ 1, 1 } } * vec{ -1, 100 }),
               DeepMatcher(vec{ vec{ -1, 100 }, vec{ -1, 100 }, vec{ -1, 100 } }));

    i32x2 xy{ 10, 20 };
    i32x2x2 m{ i32x2{ 1, 2 }, i32x2{ 3, 4 } };
    xy = hadd(xy * m);
    CHECK_THAT((xy), DeepMatcher(i32x2{ 40, 120 }));

    i32x2 xy2{ 10, 20 };
    xy2 = hadd(transpose(xy2 * m));
    CHECK_THAT((xy2), DeepMatcher(i32x2{ 50, 110 }));
}

TEST_CASE("apply")
{
    CHECK_THAT((apply([](int x) { return x + 1; }, make_vector(1, 2, 3, 4, 5))),
               DeepMatcher(make_vector(2, 3, 4, 5, 6)));
    CHECK_THAT((apply(fn::sqr(), make_vector(1, 2, 3, 4, 5))), DeepMatcher(make_vector(1, 4, 9, 16, 25)));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
