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
struct safe_negation
{
    template <typename T>
    bool operator()(ctype_t<T>, special_value value) const
    {
        using U = subtype<T>;
        return !std::is_integral<U>::value || !std::is_signed<U>::value ||
               value.get<U>() != std::numeric_limits<U>::min();
    }
};

struct safe_addition
{
    template <typename T>
    bool operator()(ctype_t<T>, special_value left, special_value right) const
    {
        using U = subtype<T>;
        if (!std::is_integral<U>::value || !std::is_signed<U>::value)
            return true;

        const U x = left.get<U>();
        const U y = right.get<U>();
        return !(y > U(0) && x > std::numeric_limits<U>::max() - y) &&
               !(y < U(0) && x < std::numeric_limits<U>::min() - y);
    }
};

struct safe_subtraction
{
    template <typename T>
    bool operator()(ctype_t<T>, special_value left, special_value right) const
    {
        using U = subtype<T>;
        if (!std::is_integral<U>::value || !std::is_signed<U>::value)
            return true;

        const U x = left.get<U>();
        const U y = right.get<U>();
        return !(y > U(0) && x < std::numeric_limits<U>::min() + y) &&
               !(y < U(0) && x > std::numeric_limits<U>::max() + y);
    }
};

struct safe_multiplication
{
    template <typename T>
    bool operator()(ctype_t<T>, special_value left, special_value right) const
    {
        using U = subtype<T>;
        if (!std::is_integral<U>::value || !std::is_signed<U>::value)
            return true;

        const U x   = left.get<U>();
        const U y   = right.get<U>();
        const U min = std::numeric_limits<U>::min();
        const U max = std::numeric_limits<U>::max();

        if (x == U(0) || y == U(0))
            return true;
        if (x == U(-1))
            return y != min;
        if (y == U(-1))
            return x != min;
        if (x > U(0))
            return y > U(0) ? x <= max / y : y >= min / x;
        return y > U(0) ? x >= min / y : x >= max / y;
    }
};

TEST_CASE("neg")
{
    test_function1(
        test_catogories::vectors, [](auto x) -> decltype(x) { return -x; },
        [](auto x) -> decltype(x) { return -x; }, safe_negation{});
}

TEST_CASE("copysign preserves magnitude and copies only the sign bit")
{
    const vec<u32, 8> xbits{ 0x40400000u, 0xc0200000u, 0u, 0x80000000u,
                             0x7f800000u, 0xff800000u, 0x7fc12345u, 0xffc12345u };
    const vec<u32, 8> ybits{ 0x80000000u, 0u, 0x80000000u, 0u,
                             0x80000000u, 0u, 0x80000000u, 0u };
    const vec<u32, 8> expected{ 0xc0400000u, 0x40200000u, 0x80000000u, 0u,
                                0xff800000u, 0x7f800000u, 0xffc12345u, 0x7fc12345u };
    CHECK_THAT(ubitcast(copysign(bitcast<f32>(xbits), bitcast<f32>(ybits))), DeepMatcher(expected));

    const vec<u64, 3> dx{ 0xc004000000000000ull, 0x7ff8000000001234ull, 0x8000000000000000ull };
    const vec<u64, 3> dy{ 0ull, 0x8000000000000000ull, 0ull };
    const vec<u64, 3> de{ 0x4004000000000000ull, 0xfff8000000001234ull, 0ull };
    CHECK_THAT(ubitcast(copysign(bitcast<f64>(dx), bitcast<f64>(dy))), DeepMatcher(de));

    const vec<u32, 1> ix{ 0x80000025u };
    const vec<u32, 1> iy{ 0u };
    CHECK_THAT(copysign(ix, iy), DeepMatcher(vec<u32, 1>{ 0x25u }));

    CHECK(kfr::copysign(3.f, -1.f) == -3.f);
    CHECK(kfr::copysign(-2.5, 1.0) == 2.5);
    CHECK(ubitcast(kfr::copysign(0.f, -1.f)) == 0x80000000u);
    CHECK(ubitcast(kfr::copysign(-0.0, 1.0)) == 0ull);
    CHECK(ubitcast(kfr::copysign(bitcast_anything<f32>(u32{ 0x7fc12345u }), -1.f)) == 0xffc12345u);
    CHECK(ubitcast(kfr::copysign(bitcast_anything<f64>(u64{ 0xfff8000000001234ull }), 1.0)) ==
          0x7ff8000000001234ull);
    CHECK(kfr::copysign(u32{ 0x80000025u }, u32{ 0 }) == u32{ 0x25u });
    CHECK(kfr::copysign(i32{ 37 }, i32{ -1 }) == std::numeric_limits<i32>::min() + 37);
}

TEST_CASE("alternating negation handles integer and floating-point lanes")
{
    const vec<i32, 5> signed_values{ 1, -2, std::numeric_limits<i32>::min(), 0, 7 };
    CHECK_THAT(negeven(signed_values),
               DeepMatcher(vec<i32, 5>{ -1, -2, std::numeric_limits<i32>::min(), 0, -7 }));
    CHECK_THAT(negodd(signed_values),
               DeepMatcher(vec<i32, 5>{ 1, 2, std::numeric_limits<i32>::min(), 0, 7 }));

    const vec<u8, 5> unsigned_values{ 1, 2, 128, 255, 0 };
    CHECK_THAT(negeven(unsigned_values), DeepMatcher(vec<u8, 5>{ 255, 2, 128, 255, 0 }));
    CHECK_THAT(negodd(unsigned_values), DeepMatcher(vec<u8, 5>{ 1, 254, 128, 1, 0 }));
    const vec<i64, 1> single{ std::numeric_limits<i64>::min() };
    CHECK_THAT(negeven(single), DeepMatcher(single));
    CHECK_THAT(negodd(single), DeepMatcher(single));
    const vec<i8, 16> bytes{ 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6, -6, 7, -7, -128, -128 };
    CHECK_THAT(negeven(bytes),
               DeepMatcher(vec<i8, 16>{ -1, -1, -2, -2, -3, -3, -4, -4, -5, -5, -6, -6, -7, -7, -128, -128 }));
    CHECK_THAT(negodd(bytes),
               DeepMatcher(vec<i8, 16>{ 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, -128, -128 }));

    const vec<u32, 5> bits{ 0u, 0x80000000u, 0x7fc12345u, 0xff800000u, 0x3f800000u };
    const auto floats = bitcast<f32>(bits);
    CHECK_THAT(ubitcast(negeven(floats)),
               DeepMatcher(vec<u32, 5>{ 0x80000000u, 0x80000000u, 0xffc12345u, 0xff800000u, 0xbf800000u }));
    CHECK_THAT(ubitcast(negodd(floats)),
               DeepMatcher(vec<u32, 5>{ 0u, 0u, 0x7fc12345u, 0x7f800000u, 0x3f800000u }));
    const vec<u64, 2> dbits{ 0x8000000000000000ull, 0x7ff8000000001234ull };
    const auto doubles = bitcast<f64>(dbits);
    CHECK_THAT(ubitcast(negeven(doubles)), DeepMatcher(vec<u64, 2>{ 0ull, 0x7ff8000000001234ull }));
    CHECK_THAT(ubitcast(negodd(doubles)),
               DeepMatcher(vec<u64, 2>{ 0x8000000000000000ull, 0xfff8000000001234ull }));
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
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x + y; },
        safe_addition{});
}

TEST_CASE("sub")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x - y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x - y; },
        safe_subtraction{});
}

TEST_CASE("mul")
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x * y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x * y; },
        safe_multiplication{});
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
        CHECK_THAT((unsigned_value << shift), DeepMatcher(vec<u8, 16>(u8(u8(0x81) << shift))));
        CHECK_THAT((unsigned_value >> shift), DeepMatcher(vec<u8, 16>(u8(u8(0x81) >> shift))));
        CHECK_THAT((signed_left_value << shift), DeepMatcher(vec<i8, 16>(i8(u8(1u << shift)))));
        CHECK_THAT((signed_value >> shift), DeepMatcher(vec<i8, 16>(i8(-1))));
    }
}

TEST_CASE("signed 64-bit right shift keeps the signed result type")
{
    const vec<i64, 2> value(i64(-3));
    CHECK_THAT((value >> unsigned(1)), DeepMatcher(vec<i64, 2>(i64(-2))));
}

TEST_CASE("integer arithmetic wraps at the element width")
{
    const vec<u8, 16> umax_value((u8)255);
    const vec<u8, 16> one((u8)1);
    CHECK_THAT((umax_value + one), DeepMatcher(vec<u8, 16>((u8)0)));
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
