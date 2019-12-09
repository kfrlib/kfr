/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/simd/vec.hpp>

#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(mask_op)
{
    mask<float, 4> m = make_mask<float>(true, false, true, false);

    CHECK(m == make_mask<float>(true, false, true, false));

    m ^= vec<float, 4>(1, 2, 3, 4) < 3;

    CHECK(m == make_mask<float>(false, true, true, false));

    m |= vec<float, 4>(1, 2, 3, 4) < 3;

    CHECK(m == make_mask<float>(true, true, true, false));

    m &= vec<float, 4>(1, 2, 3, 4) < 3;

    CHECK(m == make_mask<float>(true, true, false, false));

    m = ~m;

    CHECK(m == make_mask<float>(false, false, true, true));
}
TEST(cones)
{
    CHECK(vec<int, 2>(cones) == vec<int, 2>(-1, -1));
    CHECK(vec<float, 2>(cones) == vec<f32, 2>(bitcast<f32>(-1), bitcast<f32>(-1)));
}
TEST(vec_broadcast)
{
    CHECK(static_cast<f32x4>(4.f) == f32x4{ 4.f, 4.f, 4.f, 4.f });
    CHECK(static_cast<f64x8>(4.f) == f64x8{ 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0 });
    CHECK(static_cast<u8x3>(4.f) == u8x3{ 4, 4, 4 });
}
template <typename Tout, typename Tin>
bool is_in_range_of(Tin x)
{
    return (is_f_class<Tin> && is_f_class<Tout>) || static_cast<Tin>(static_cast<Tout>(x)) == x;
}

TEST(cast)
{
    testo::assert_is_same<i32x4, kfr::common_type<i32x4>>();
    testo::assert_is_same<u32x4, kfr::common_type<i32x4, u32x4>>();
    testo::assert_is_same<f64x4, kfr::common_type<i32x4, u32x4, f64x4>>();

    CHECK(static_cast<i32x4>(u16x4{ 1, 2, 3, 4 }) == i32x4{ 1, 2, 3, 4 });

    CHECK(static_cast<vec<vec<double, 4>, 2>>(vec<vec<float, 4>, 2>{
              vec<float, 4>{ 1.f, 2.f, 3.f, 4.f }, vec<float, 4>{ 11.f, 22.f, 33.f, 44.f } }) ==
          vec<vec<double, 4>, 2>{ vec<double, 4>{ 1., 2., 3., 4. }, vec<double, 4>{ 11., 22., 33., 44. } });

    static_assert(is_convertible<float, f32x4>, "");
    static_assert(is_convertible<float, f64x8>, "");
    static_assert(is_convertible<float, u8x3>, "");

    static_assert(is_convertible<u16x4, i32x4>, "");
    static_assert(!is_convertible<u16x4, i32x3>, "");
    static_assert(!is_convertible<u16x1, u16x16>, "");

    static_assert(is_same<decltype(innercast<f64>(f32x4x4(1))), f64x4x4>, "");
    static_assert(is_same<decltype(innercast<f64>(f32x4(1))), f64x4>, "");
    static_assert(is_same<decltype(innercast<f64>(f32(1))), f64>, "");

    // N/A static_assert(is_same<decltype(innercast<f64x4>(f32x4x4(1))), f64x4x4>, "");
    static_assert(is_same<decltype(innercast<f64x4>(f32x4(1))), f64x4x4>, "");
    static_assert(is_same<decltype(innercast<f64x4>(f32(1))), f64x4>, "");

    // N/A static_assert(is_same<decltype(elemcast<f64>(f32x4x4(1))), f64x4>, "");
    static_assert(is_same<decltype(elemcast<f64>(f32x4(1))), f64x4>, "");

    static_assert(is_same<decltype(elemcast<f64x4>(f32x4x4(1))), f64x4x4>, "");
    static_assert(is_same<decltype(elemcast<f64x4>(f32x4(1))), f64x4x4>, "");

    testo::scope s("");
    s.text = ("target_type = u8");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<u8>(x); },
        [](auto x) -> u8 { return static_cast<u8>(x); },
        [](auto t, special_value x) { return is_in_range_of<u8>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = i8");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<i8>(x); },
        [](auto x) -> i8 { return static_cast<i8>(x); },
        [](auto t, special_value x) { return is_in_range_of<i8>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = u16");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<u16>(x); },
        [](auto x) -> u16 { return static_cast<u16>(x); },
        [](auto t, special_value x) { return is_in_range_of<u16>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = i16");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<i16>(x); },
        [](auto x) -> i16 { return static_cast<i16>(x); },
        [](auto t, special_value x) { return is_in_range_of<i16>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = u32");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<u32>(x); },
        [](auto x) -> u32 { return static_cast<u32>(x); },
        [](auto t, special_value x) { return is_in_range_of<u32>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = i32");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<i32>(x); },
        [](auto x) -> i32 { return static_cast<i32>(x); },
        [](auto t, special_value x) { return is_in_range_of<i32>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = u64");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<u64>(x); },
        [](auto x) -> u64 { return static_cast<u64>(x); },
        [](auto t, special_value x) { return is_in_range_of<u64>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = i64");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<i64>(x); },
        [](auto x) -> i64 { return static_cast<i64>(x); },
        [](auto t, special_value x) { return is_in_range_of<i64>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = f32");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<f32>(x); },
        [](auto x) -> f32 { return static_cast<f32>(x); },
        [](auto t, special_value x) { return is_in_range_of<f32>(x.get<subtype<typename decltype(t)::type>>()); });
    s.text = ("target_type = f64");
    test_function1(
        test_catogories::all, [](auto x) { return kfr::innercast<f64>(x); },
        [](auto x) -> f64 { return static_cast<f64>(x); },
        [](auto t, special_value x) { return is_in_range_of<f64>(x.get<subtype<typename decltype(t)::type>>()); });
}

TEST(unaligned_read)
{
    testo::matrix(named("type") = numeric_vector_types<vec>, [](auto type) {
            using T = typename decltype(type)::type;
        using Tsub                = subtype<T>;
        constexpr static size_t N = T::size();
        Tsub data[N * 2];
        for (size_t i = 0; i < arraysize(data); i++)
        {
            data[i] = static_cast<Tsub>(i);
        }

        for (size_t i = 0; i < N; i++)
        {
            testo::scope sc(as_string("i = ", i));
            CHECK(read<N, false>(data + i) == (enumerate<Tsub, N>() + static_cast<Tsub>(i)));
        }
    });
}

TEST(mask_broadcast)
{
    CHECK(mask<i32, 4>(mask<f32, 4>(true, false, true, false)).asvec() == vec<i32, 4>(-1, 0, -1, 0));
    CHECK(mask<i32, 4>(mask<f32, 4>(true)).asvec() == vec<i32, 4>(-1, -1, -1, -1));
    CHECK(mask<i32, 4>(mask<i32, 4>(true)).asvec() == vec<i32, 4>(-1, -1, -1, -1));
    CHECK(mask<i32, 4>(mask<u32, 4>(true)).asvec() == vec<i32, 4>(-1, -1, -1, -1));
}

TEST(masks)
{
    mask<float, 4> m = make_mask<float>(false, true, false, true);
    vec<float, 4> v  = m.asvec();
    CHECK(bit<float>(m[0]) == false);
    CHECK(bit<float>(m[1]) == true);
    CHECK(bit<float>(m[2]) == false);
    CHECK(bit<float>(m[3]) == true);
    CHECK(float(v[0]) == maskbits<float>(false));
    CHECK(float(v[1]) == maskbits<float>(true));
    CHECK(float(v[2]) == maskbits<float>(false));
    CHECK(float(v[3]) == maskbits<float>(true));
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
