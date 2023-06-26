/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>
#include <kfr/simd/horizontal.hpp>
#include <kfr/simd/operators.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(neg)
{
    test_function1(
        test_catogories::vectors, [](auto x) -> decltype(x) { return -x; },
        [](auto x) -> decltype(x) { return -x; });
}

TEST(bnot)
{
    test_function1(
        test_catogories::vectors, [](auto x) -> decltype(x) { return ~x; },
        [](auto x) -> decltype(x)
        {
            utype<decltype(x)> u = ~ubitcast(x);
            return bitcast<decltype(x)>(u);
        });
}

TEST(add)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x + y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x + y; });
}

TEST(sub)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x - y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> { return x - y; });
}

TEST(mul)
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

TEST(div)
{
    test_function2(
        test_catogories::vectors,
        [](auto x, auto y)
        { return is_safe_division<subtype<decltype(x)>>(x.front(), y.front()) ? x / y : 0; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return is_safe_division(x, y) ? x / y : 0;
        });
}
struct not_f
{
    template <typename T>
    constexpr bool operator()(ctype_t<T>) const
    {
        return !is_f_class<subtype<T>>;
    }
};
TEST(mod)
{
    test_function2(
        test_catogories::vectors,
        [](auto x, auto y)
        { return is_safe_division<subtype<decltype(x)>>(x.front(), y.front()) ? x % y : 0; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return is_safe_division(x, y) ? x % y : 0;
        },
        fn_return_constant<bool, true>{}, not_f{});
}

TEST(bor)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x | y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            using T = std::common_type_t<decltype(x), decltype(y)>;
            return bitcast<T>(static_cast<utype<T>>(ubitcast(T(x)) | ubitcast(T(y))));
        });
}

TEST(bxor)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x ^ y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            using T = std::common_type_t<decltype(x), decltype(y)>;
            return bitcast<T>(static_cast<utype<T>>(ubitcast(T(x)) ^ ubitcast(T(y))));
        });
}

TEST(band)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return x & y; },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)>
        {
            using T = std::common_type_t<decltype(x), decltype(y)>;
            return bitcast<T>(static_cast<utype<T>>(ubitcast(T(x)) & ubitcast(T(y))));
        });
}

TEST(shl)
{
    testo::matrix(
        named("type") = test_catogories::types(test_catogories::vectors), named("value1") = special_values(),
        named("shift") = std::vector<unsigned>{ 1, 2, 7, 8, 9, 15, 16, 31, 32, 63, 64 },
        [&](auto type, special_value value, unsigned shift)
        {
            using T = typename decltype(type)::type;
            if (shift < sizeof(subtype<T>))
            {
                const T x(value);
                CHECK(std::is_same<decltype(x << shift), T>::value);
                CHECK((x << shift) == apply(
                                          [=](auto x) -> decltype(x) {
                                              return bitcast<decltype(x)>(
                                                  static_cast<uitype<decltype(x)>>(uibitcast(x) << shift));
                                          },
                                          x));
                CHECK((x << broadcast<T::scalar_size()>(utype<subtype<T>>(shift))) ==
                      apply(
                          [=](auto x) -> decltype(x) {
                              return bitcast<decltype(x)>(
                                  static_cast<uitype<decltype(x)>>(uibitcast(x) << shift));
                          },
                          x));
            }
        });
}

TEST(shr)
{
    testo::matrix(
        named("type") = test_catogories::types(test_catogories::vectors), named("value1") = special_values(),
        named("shift") = std::vector<unsigned>{ 1, 2, 7, 8, 9, 15, 16, 31, 32, 63, 64 },
        [&](auto type, special_value value, unsigned shift)
        {
            using T = typename decltype(type)::type;
            if (shift < sizeof(subtype<T>))
            {
                const T x(value);
                CHECK(std::is_same<decltype(x << shift), T>::value);
                CHECK((x >> shift) == apply(
                                          [=](auto x) -> decltype(x) {
                                              return bitcast<decltype(x)>(
                                                  static_cast<uitype<decltype(x)>>(uibitcast(x) >> shift));
                                          },
                                          x));
                CHECK((x >> broadcast<T::scalar_size()>(utype<subtype<T>>(shift))) ==
                      apply(
                          [=](auto x) -> decltype(x) {
                              return bitcast<decltype(x)>(
                                  static_cast<uitype<decltype(x)>>(uibitcast(x) >> shift));
                          },
                          x));
            }
        });
}

TEST(eq)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x == y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return maskbits<subtype<decltype(x)>>(x == y);
        });
}

TEST(ne)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x != y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return maskbits<subtype<decltype(x)>>(x != y);
        });
}

TEST(ge)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x >= y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return maskbits<subtype<decltype(x)>>(x >= y);
        });
}

TEST(le)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x <= y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return maskbits<subtype<decltype(x)>>(x <= y);
        });
}

TEST(gt)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x > y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return maskbits<subtype<decltype(x)>>(x > y);
        });
}

TEST(lt)
{
    test_function2(
        test_catogories::vectors, [](auto x, auto y) { return (x < y).asvec(); },
        [](auto x, auto y) -> std::common_type_t<decltype(x), decltype(y)> {
            return maskbits<subtype<decltype(x)>>(x < y);
        });
}

TEST(horner)
{
    CHECK(horner(pack(0, 1, 2, 3), 1, 2, 3) == pack(1, 6, 17, 34));
    CHECK(horner_odd(pack(0, 1, 2, 3), 1, 2, 3) == pack(0, 6, 114, 786));
    CHECK(horner_even(pack(0, 1, 2, 3), 1, 2, 3) == pack(1, 6, 57, 262));
}

TEST(matrix)
{
    using i32x2x2 = vec<vec<int, 2>, 2>;
    const i32x2x2 m22{ i32x2{ 1, 2 }, i32x2{ 3, 4 } };
    CHECK(m22 * 10 == i32x2x2{ i32x2{ 10, 20 }, i32x2{ 30, 40 } });

    CHECK(m22 * i32x2{ -1, 100 } == i32x2x2{ i32x2{ -1, 200 }, i32x2{ -3, 400 } });

    CHECK(vec{ vec{ 1, 1 }, vec{ 1, 1 } } * vec{ -1, 100 } == vec{ vec{ -1, 100 }, vec{ -1, 100 } });
    CHECK(vec{ vec{ 1, 1 }, vec{ 1, 1 }, vec{ 1, 1 } } * vec{ -1, 100 } ==
          vec{ vec{ -1, 100 }, vec{ -1, 100 }, vec{ -1, 100 } });

    i32x2 xy{ 10, 20 };
    i32x2x2 m{ i32x2{ 1, 2 }, i32x2{ 3, 4 } };
    xy = hadd(xy * m);
    CHECK(xy == i32x2{ 40, 120 });

    i32x2 xy2{ 10, 20 };
    xy2 = hadd(transpose(xy2 * m));
    CHECK(xy2 == i32x2{ 50, 110 });
}

TEST(apply)
{
    CHECK(apply([](int x) { return x + 1; }, make_vector(1, 2, 3, 4, 5)) == make_vector(2, 3, 4, 5, 6));
    CHECK(apply(fn::sqr(), make_vector(1, 2, 3, 4, 5)) == make_vector(1, 4, 9, 16, 25));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
