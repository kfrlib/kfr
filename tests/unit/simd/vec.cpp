/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include <bit>
#include <kfr/simd/vec.hpp>

#include <kfr/io/tostring.hpp>

namespace kfr
{

static_assert(std::is_same_v<i32x4, std::common_type_t<i32x4, i32x4>>);
static_assert(std::is_same_v<i32x4, std::common_type_t<i32x4>>);
static_assert(std::is_same_v<u32x4, std::common_type_t<i32x4, u32x4>>);
static_assert(std::is_same_v<f64x4, std::common_type_t<i32x4, u32x4, f64x4>>);

inline namespace KFR_ARCH_NAME
{

TEST_CASE("mask_op")
{
    mask<float, 4> m = make_mask<float>(true, false, true, false);

    CHECK_THAT(m, DeepMatcher(make_mask<float>(true, false, true, false)));

    m ^= vec<float, 4>(1, 2, 3, 4) < 3;

    CHECK_THAT(m, DeepMatcher(make_mask<float>(false, true, true, false)));

    m |= vec<float, 4>(1, 2, 3, 4) < 3;

    CHECK_THAT(m, DeepMatcher(make_mask<float>(true, true, true, false)));

    m &= vec<float, 4>(1, 2, 3, 4) < 3;

    CHECK_THAT(m, DeepMatcher(make_mask<float>(true, true, false, false)));

    m = ~m;

    CHECK_THAT(m, DeepMatcher(make_mask<float>(false, false, true, true)));
}
TEST_CASE("cones")
{
    CHECK_THAT((vec<int, 2>(cones)), DeepMatcher(vec<int, 2>(-1, -1)));
    CHECK_THAT((vec<float, 2>(cones)), DeepMatcher(vec<f32, 2>(bitcast<f32>(-1), bitcast<f32>(-1))));
}
TEST_CASE("vec_broadcast")
{
    CHECK_THAT(static_cast<f32x4>(4.f), DeepMatcher(f32x4{ 4.f, 4.f, 4.f, 4.f }));
    CHECK_THAT(static_cast<f64x8>(4.f), DeepMatcher(f64x8{ 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0 }));
    CHECK_THAT(static_cast<u8x3>(4.f), DeepMatcher(u8x3{ 4, 4, 4 }));
}
template <typename Tout, typename Tin>
bool is_in_range_of(Tin x)
{
    return (is_f_class<Tin> && is_f_class<Tout>) || static_cast<Tin>(static_cast<Tout>(x)) == x;
}

TEST_CASE("cast")
{

    CHECK_THAT(static_cast<i32x4>(u16x4{ 1, 2, 3, 4 }), DeepMatcher(i32x4{ 1, 2, 3, 4 }));

    CHECK_THAT((static_cast<vec<vec<double, 4>, 2>>(vec<vec<float, 4>, 2>{
                   vec<float, 4>{ 1.f, 2.f, 3.f, 4.f }, vec<float, 4>{ 11.f, 22.f, 33.f, 44.f } })),
               DeepMatcher(vec<vec<double, 4>, 2>{ vec<double, 4>{ 1., 2., 3., 4. },
                                                   vec<double, 4>{ 11., 22., 33., 44. } }));

    static_assert(std::is_convertible_v<float, f32x4>, "");
    static_assert(std::is_convertible_v<float, f64x8>, "");
    static_assert(std::is_convertible_v<float, u8x3>, "");

    static_assert(std::is_convertible_v<u16x4, i32x4>, "");
    static_assert(!std::is_convertible_v<u16x4, i32x3>, "");
    static_assert(!std::is_convertible_v<u16x1, u16x16>, "");

    static_assert(std::is_convertible_v<float, vecx<float, 2>>, "");
    static_assert(std::is_convertible_v<float, vecx<float, 2, 2>>, "");

    static_assert(std::is_same_v<decltype(broadcastto<f64>(f32x4x4(1))), f64x4x4>, "");
    static_assert(std::is_same_v<decltype(broadcastto<f64>(f32x4(1))), f64x4>, "");
    static_assert(std::is_same_v<decltype(broadcastto<f64>(f32(1))), f64>, "");

    // N/A static_assert(std::is_same_v<decltype(broadcastto<f64x4>(f32x4x4(1))), f64x4x4>, "");
    static_assert(std::is_same_v<decltype(broadcastto<f64x4>(f32x4(1))), f64x4x4>, "");
    static_assert(std::is_same_v<decltype(broadcastto<f64x4>(f32(1))), f64x4>, "");

    // N/A static_assert(std::is_same_v<decltype(promoteto<f64>(f32x4x4(1))), f64x4>, "");
    static_assert(std::is_same_v<decltype(promoteto<f64>(f32x4(1))), f64x4>, "");

    static_assert(std::is_same_v<decltype(promoteto<f64x4>(f32x4x4(1))), f64x4x4>, "");
    static_assert(std::is_same_v<decltype(promoteto<f64x4>(f32x4(1))), f64x4x4>, "");

    CHECK_THAT((cast<vecx<float, 2, 2>>(123.f)),
               DeepMatcher(vec{ vec{ 123.f, 123.f }, vec{ 123.f, 123.f } }));

    CHECK_THAT((promoteto<vecx<float, 2>>(vecx<float, 4>{ 1.f, 2.f, 3.f, 4.f })),
               DeepMatcher(vec{ vec{ 1.f, 1.f }, vec{ 2.f, 2.f }, vec{ 3.f, 3.f }, vec{ 4.f, 4.f } }));

    {
        INFO("target_type = u8");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<u8>(x); },
            [](auto x) -> u8 { return static_cast<u8>(x); }, [](auto t, special_value x)
            { return is_in_range_of<u8>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = i8");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<i8>(x); },
            [](auto x) -> i8 { return static_cast<i8>(x); }, [](auto t, special_value x)
            { return is_in_range_of<i8>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = u16");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<u16>(x); },
            [](auto x) -> u16 { return static_cast<u16>(x); }, [](auto t, special_value x)
            { return is_in_range_of<u16>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = i16");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<i16>(x); },
            [](auto x) -> i16 { return static_cast<i16>(x); }, [](auto t, special_value x)
            { return is_in_range_of<i16>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = u32");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<u32>(x); },
            [](auto x) -> u32 { return static_cast<u32>(x); }, [](auto t, special_value x)
            { return is_in_range_of<u32>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = i32");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<i32>(x); },
            [](auto x) -> i32 { return static_cast<i32>(x); }, [](auto t, special_value x)
            { return is_in_range_of<i32>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = u64");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<u64>(x); },
            [](auto x) -> u64 { return static_cast<u64>(x); }, [](auto t, special_value x)
            { return is_in_range_of<u64>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = i64");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<i64>(x); },
            [](auto x) -> i64 { return static_cast<i64>(x); }, [](auto t, special_value x)
            { return is_in_range_of<i64>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = f32");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<f32>(x); },
            [](auto x) -> f32 { return static_cast<f32>(x); }, [](auto t, special_value x)
            { return is_in_range_of<f32>(x.get<subtype<typename decltype(t)::type>>()); });
    }
    {
        INFO("target_type = f64");
        test_function1(
            test_catogories::all, [](auto x) { return kfr::broadcastto<f64>(x); },
            [](auto x) -> f64 { return static_cast<f64>(x); }, [](auto t, special_value x)
            { return is_in_range_of<f64>(x.get<subtype<typename decltype(t)::type>>()); });
    }
}

TEST_CASE("unaligned_read")
{
    test_matrix(named("type") = numeric_vector_types<vec>,
                [](auto type)
                {
                    using T = typename decltype(type)::type;
#if defined(_MSC_VER) && !defined(__clang__)
                    // workaround for MSVC
                    using Tsub = typename T::value_type;
#else                   
                      using Tsub                = subtype<T>;
#endif

                    constexpr static size_t N = T::size();
                    Tsub data[N * 2];
                    for (size_t i = 0; i < std::size(data); i++)
                    {
                        data[i] = static_cast<Tsub>(i);
                    }

                    for (size_t i = 0; i < N; i++)
                    {
                        INFO(as_string("i = ", i));
                        CHECK_THAT((read<N, false>(data + i)),
                                   DeepMatcher((enumerate<Tsub, N>() + static_cast<Tsub>(i))));
                    }
                });
}

TEST_CASE("mask_broadcast")
{
    CHECK_THAT((mask<i32, 4>(mask<f32, 4>(true, false, true, false)).asvec()),
               DeepMatcher(vec<i32, 4>(-1, 0, -1, 0)));
    CHECK_THAT((mask<i32, 4>(mask<f32, 4>(true)).asvec()), DeepMatcher(vec<i32, 4>(-1, -1, -1, -1)));
    CHECK_THAT((mask<i32, 4>(mask<i32, 4>(true)).asvec()), DeepMatcher(vec<i32, 4>(-1, -1, -1, -1)));
    CHECK_THAT((mask<i32, 4>(mask<u32, 4>(true)).asvec()), DeepMatcher(vec<i32, 4>(-1, -1, -1, -1)));
}

TEST_CASE("masks")
{
    mask<float, 4> m = make_mask<float>(false, true, false, true);
    vec<float, 4> v  = m.asvec();
    CHECK(bit<float>(m[0]) == false);
    CHECK(bit<float>(m[1]) == true);
    CHECK(bit<float>(m[2]) == false);
    CHECK(bit<float>(m[3]) == true);
    CHECK(std::bit_cast<uint32_t>(float(v[0])) == 0x00000000u);
    CHECK(std::bit_cast<uint32_t>(float(v[1])) == 0xffffffffu);
    CHECK(std::bit_cast<uint32_t>(float(v[2])) == 0x00000000u);
    CHECK(std::bit_cast<uint32_t>(float(v[3])) == 0xffffffffu);

    CHECK(bitcast_anything<std::array<int32_t, 1>>(mask<i32, 1>{ true }) == std::array<int32_t, 1>{ -1 });
    CHECK(bitcast_anything<std::array<int32_t, 2>>(mask<i32, 2>{ true, true }) ==
          std::array<int32_t, 2>{ -1, -1 });
    CHECK(bitcast_anything<std::array<int32_t, 4>>(mask<i32, 4>{ true, true, true, true }) ==
          std::array<int32_t, 4>{ -1, -1, -1, -1 });

    CHECK_THAT(
        (bitcast_anything<u8x16>(mask<f32, 4>{ true, true, true, true })),
        DeepMatcher(u8x16{ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }));
    CHECK_THAT(
        (bitcast_anything<u8x16>(bitcast<bit<u8>>(mask<i32, 4>{ true, true, true, true }))),
        DeepMatcher(u8x16{ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }));
    CHECK_THAT(
        (bitcast_anything<u8x16>(bitcast<bit<u8>>(mask<f32, 4>{ true, true, true, true }))),
        DeepMatcher(u8x16{ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 }));
}

TEST_CASE("vec_deduction")
{
    vec v{ 1, 2, 3 };
    static_assert(std::is_same_v<decltype(v), vec<int, 3>>);
}

TEST_CASE("vec2d_element")
{
    auto v = vec{ vec{ 1, 2 }, vec{ 3, 4 } };
    CHECK(v[0][0] == 1);
    CHECK(v[0][1] == 2);
    CHECK(v[1][0] == 3);
    CHECK(v[1][1] == 4);
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
