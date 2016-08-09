/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>

#include "testo/testo.hpp"
#include <kfr/dsp/mixdown.hpp>
#include <kfr/math.hpp>

using namespace kfr;

TEST(vec_concat)
{
    CHECK(concat(vec<f32, 1>{ 1 }, vec<f32, 2>{ 2, 3 }, vec<f32, 1>{ 4 }, vec<f32, 3>{ 5, 6, 7 }) //
          == vec<f32, 7>{ 1, 2, 3, 4, 5, 6, 7 });
}

TEST(vec_split)
{
    vec<f32, 1> a1;
    vec<f32, 2> a23;
    vec<f32, 1> a4;
    vec<f32, 3> a567;
    split(vec<f32, 7>{ 1, 2, 3, 4, 5, 6, 7 }, a1, a23, a4, a567);
    CHECK(a1 == vec<f32, 1>{ 1 });
    CHECK(a23 == vec<f32, 2>{ 2, 3 });
    CHECK(a4 == vec<f32, 1>{ 4 });
    CHECK(a567 == vec<f32, 3>{ 5, 6, 7 });
}

TEST(vec_broadcast)
{
    CHECK(broadcast<5>(3.f) == vec<f32, 5>{ 3, 3, 3, 3, 3 });
    CHECK(broadcast<6>(1.f, 2.f) == vec<f32, 6>{ 1, 2, 1, 2, 1, 2 });
    CHECK(broadcast<6>(1.f, 2.f, 3.f) == vec<f32, 6>{ 1, 2, 3, 1, 2, 3 });
}

TEST(vec_resize)
{
    CHECK(resize<5>(make_vector(3.f)) == vec<f32, 5>{ 3, 3, 3, 3, 3 });
    CHECK(resize<6>(make_vector(1.f, 2.f)) == vec<f32, 6>{ 1, 2, 1, 2, 1, 2 });
    CHECK(resize<6>(make_vector(1.f, 2.f, 3.f)) == vec<f32, 6>{ 1, 2, 3, 1, 2, 3 });
}

TEST(vec_make_vector)
{
    const signed char ch = -1;
    CHECK(make_vector(1, 2, ch) == vec<i32, 3>{ 1, 2, -1 });
    const i64 v = -100;
    CHECK(make_vector(1, 2, v) == vec<i64, 3>{ 1, 2, -100 });
    CHECK(make_vector<i64>(1, 2, ch) == vec<i64, 3>{ 1, 2, -1 });
    CHECK(make_vector<f32>(1, 2, ch) == vec<f32, 3>{ 1, 2, -1 });

    CHECK(make_vector(f64x2{ 1, 2 }, f64x2{ 10, 20 }) ==
          vec<vec<f64, 2>, 2>{ f64x2{ 1, 2 }, f64x2{ 10, 20 } });
    CHECK(make_vector(1.f, f32x2{ 10, 20 }) == vec<vec<f32, 2>, 2>{ f32x2{ 1, 1 }, f32x2{ 10, 20 } });
}

TEST(vec_apply)
{
    CHECK(apply([](int x) { return x + 1; }, make_vector(1, 2, 3, 4, 5)) == make_vector(2, 3, 4, 5, 6));
    CHECK(apply(fn_sqr(), make_vector(1, 2, 3, 4, 5)) == make_vector(1, 4, 9, 16, 25));
}

#ifdef CMT_ARCH_SSE
TEST(vec_tovec)
{
    const __m128 x = _mm_set_ps(4.f, 3.f, 2.f, 1.f);
    CHECK(tovec(x) == vec<f32, 4>(1, 2, 3, 4));
    const __m128d y = _mm_set_pd(2.0, 1.0);
    CHECK(tovec(y) == vec<f64, 2>(1, 2));
    const simd<f64, 7> z{ 1, 2, 3, 4, 5, 6, 7 };
    CHECK(tovec(z) == vec<f64, 7>(1, 2, 3, 4, 5, 6, 7));
}
#endif

TEST(vec_zerovector)
{
    CHECK(zerovector<f32, 3>() == f32x3{ 0, 0, 0 });
    // CHECK(zerovector<i16, 3>() == i16x3{ 0, 0, 0 }); // clang 3.9 (trunk) crashes here
    CHECK(zerovector(f64x8{}) == f64x8{ 0, 0, 0, 0, 0, 0, 0, 0 });
}

TEST(vec_allonesvector)
{
    CHECK(~allonesvector<f32, 3>() == f32x3{ 0, 0, 0 });
    CHECK(allonesvector<i16, 3>() == i16x3{ -1, -1, -1 });
    CHECK(allonesvector<u8, 3>() == u8x3{ 255, 255, 255 });
}

TEST(vec_low_high)
{
    CHECK(low(vec<u8, 8>(1, 2, 3, 4, 5, 6, 7, 8)) == vec<u8, 4>(1, 2, 3, 4));
    CHECK(high(vec<u8, 8>(1, 2, 3, 4, 5, 6, 7, 8)) == vec<u8, 4>(5, 6, 7, 8));

    CHECK(low(vec<u8, 7>(1, 2, 3, 4, 5, 6, 7)) == vec<u8, 4>(1, 2, 3, 4));
    CHECK(high(vec<u8, 7>(1, 2, 3, 4, 5, 6, 7)) == vec<u8, 3>(5, 6, 7));

    CHECK(low(vec<u8, 6>(1, 2, 3, 4, 5, 6)) == vec<u8, 4>(1, 2, 3, 4));
    CHECK(high(vec<u8, 6>(1, 2, 3, 4, 5, 6)) == vec<u8, 2>(5, 6));

    CHECK(low(vec<u8, 5>(1, 2, 3, 4, 5)) == vec<u8, 4>(1, 2, 3, 4));
    CHECK(high(vec<u8, 5>(1, 2, 3, 4, 5)) == vec<u8, 1>(5));

    CHECK(low(vec<u8, 4>(1, 2, 3, 4)) == vec<u8, 2>(1, 2));
    CHECK(high(vec<u8, 4>(1, 2, 3, 4)) == vec<u8, 2>(3, 4));

    CHECK(low(vec<u8, 3>(1, 2, 3)) == vec<u8, 2>(1, 2));
    CHECK(high(vec<u8, 3>(1, 2, 3)) == vec<u8, 1>(3));

    CHECK(low(vec<u8, 2>(1, 2)) == vec<u8, 1>(1));
    CHECK(high(vec<u8, 2>(1, 2)) == vec<u8, 1>(2));
}

TEST(vec_conv)
{
    testo::assert_is_same<common_type<i32, f32>, f32>();
    testo::assert_is_same<common_type<f32, f64>, f64>();
    testo::assert_is_same<common_type<i32, f32x4>, f32x4>();
    testo::assert_is_same<common_type<f32x4, f64>, f32x4>();
    testo::assert_is_same<common_type<f32x4, f64x4>, f64x4>();

    testo::assert_is_same<decltype(min(1, 2)), int>();
    testo::assert_is_same<decltype(min(1, 2u)), unsigned int>();
    testo::assert_is_same<decltype(min(1, 2)), int>();
    testo::assert_is_same<decltype(min(pack(1), 2u)), i32x1>();
    testo::assert_is_same<decltype(min(2u, pack(1))), i32x1>();
    testo::assert_is_same<decltype(min(pack(1), pack(2u))), u32x1>();
    testo::assert_is_same<decltype(min(pack(1, 2, 3), pack(1.0, 2.0, 3.0))), f64x3>();
    testo::assert_is_same<decltype(min(pack(1.0, 2.0, 3.0), pack(1, 2, 3))), f64x3>();
}

TEST(vec_matrix)
{
    using i32x2x2 = vec<vec<int, 2>, 2>;
    const i32x2x2 m22{ i32x2{ 1, 2 }, i32x2{ 3, 4 } };
    CHECK(m22 * 10 == i32x2x2{ i32x2{ 10, 20 }, i32x2{ 30, 40 } });

    CHECK(m22 * i32x2{ -1, 100 } == i32x2x2{ i32x2{ -1, 200 }, i32x2{ -3, 400 } });

    i32x2 xy{ 10, 20 };
    i32x2x2 m{ i32x2{ 1, 2 }, i32x2{ 3, 4 } };
    xy = hadd(xy * m);
    CHECK(xy == i32x2{ 40, 120 });

    i32x2 xy2{ 10, 20 };
    xy2 = hadd(transpose(xy2 * m));
    CHECK(xy2 == i32x2{ 50, 110 });
}

TEST(vec_is_convertible)
{
    static_assert(std::is_convertible<float, f32x4>::value, "");
    static_assert(std::is_convertible<float, f64x8>::value, "");
    static_assert(std::is_convertible<float, u8x3>::value, "");

    static_assert(std::is_convertible<u16x4, i32x4>::value, "");
    static_assert(!std::is_convertible<u16x4, i32x3>::value, "");
    static_assert(!std::is_convertible<u16x1, u16x16>::value, "");

    static_assert(std::is_convertible<float, complex<float>>::value, "");
    static_assert(std::is_convertible<float, complex<double>>::value, "");
    static_assert(std::is_convertible<short, complex<double>>::value, "");

    static_assert(std::is_convertible<complex<float>, vec<complex<float>, 4>>::value, "");
    static_assert(!std::is_convertible<vec<complex<float>, 1>, vec<complex<float>, 4>>::value, "");

    static_assert(std::is_convertible<vec<complex<float>, 2>, vec<complex<double>, 2>>::value, "");
    static_assert(std::is_convertible<vec<vec<float, 4>, 2>, vec<vec<double, 4>, 2>>::value, "");

    CHECK(static_cast<f32x4>(4.f) == f32x4{ 4.f, 4.f, 4.f, 4.f });
    CHECK(static_cast<f64x8>(4.f) == f64x8{ 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0, 4.0 });
    CHECK(static_cast<u8x3>(4.f) == u8x3{ 4, 4, 4 });

    CHECK(static_cast<i32x4>(u16x4{ 1, 2, 3, 4 }) == i32x4{ 1, 2, 3, 4 });

    CHECK(static_cast<complex<float>>(10.f) == complex<float>{ 10.f, 0.f });
    CHECK(static_cast<complex<double>>(10.f) == complex<double>{ 10., 0. });
    CHECK(static_cast<complex<double>>(static_cast<short>(10)) == complex<double>{ 10., 0. });

    CHECK(static_cast<vec<complex<float>, 4>>(complex<float>{ 1.f, 2.f }) ==
          vec<complex<float>, 4>{ c32{ 1.f, 2.f }, c32{ 1.f, 2.f }, c32{ 1.f, 2.f }, c32{ 1.f, 2.f } });

    CHECK(static_cast<vec<complex<double>, 2>>(vec<complex<float>, 2>{ c32{ 1.f, 2.f }, c32{ 1.f, 2.f } }) ==
          vec<complex<double>, 2>{ c64{ 1., 2. }, c64{ 1., 2. } });

    CHECK(static_cast<vec<vec<double, 4>, 2>>(vec<vec<float, 4>, 2>{
              vec<float, 4>{ 1.f, 2.f, 3.f, 4.f }, vec<float, 4>{ 11.f, 22.f, 33.f, 44.f } }) ==
          vec<vec<double, 4>, 2>{ vec<double, 4>{ 1., 2., 3., 4. }, vec<double, 4>{ 11., 22., 33., 44. } });
}

TEST(vec_pack_expr)
{
    const univector<float, 20> v1 = 1 + counter();
    const univector<float, 20> v2 = v1 * 11;
    const univector<f32x2, 20> v3 = pack(v1, v2);
    CHECK(v3[0] == f32x2{ 1, 11 });
    CHECK(v3[1] == f32x2{ 2, 22 });
    CHECK(v3[18] == f32x2{ 19, 209 });
    CHECK(v3[19] == f32x2{ 20, 220 });

    const univector<f32x2, 20> v4 = bind_expression(fn_reverse(), v3);
    CHECK(v4[0] == f32x2{ 11, 1 });
    CHECK(v4[1] == f32x2{ 22, 2 });
    CHECK(v4[18] == f32x2{ 209, 19 });
    CHECK(v4[19] == f32x2{ 220, 20 });
}

int main(int argc, char** argv) { return testo::run_all("", true); }
