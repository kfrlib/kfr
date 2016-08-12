/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>

#include "testo/testo.hpp"
#include <kfr/dsp.hpp>
#include <kfr/math.hpp>

using namespace kfr;

TEST(test_basic)
{
    // How to make a vector:

    // * Use constructor
    const vec<double, 4> first{ 1, 2.5, -infinity, 3.1415926 };
    CHECK(first == vec<double, 4>{ 1, 2.5, -infinity, 3.1415926 });

    // * Use make_vector function
    const auto second = make_vector(-1, +1);
    CHECK(second == vec<int, 2>{ -1, 1 });

    // * Convert from vector of other type:
    const vec<int, 4> int_vector{ 10, 20, 30, 40 };
    const vec<double, 4> double_vector = cast<double>(int_vector);
    CHECK(double_vector == vec<double, 4>{ 10, 20, 30, 40 });

    // * Concat two vectors:
    const vec<int, 1> left_part{ 1 };
    const vec<int, 1> right_part{ 2 };
    const vec<int, 2> pair{ left_part, right_part };
    CHECK(pair == vec<int, 2>{ 1, 2 });

    // * Same, but using make_vector and concat:
    const vec<int, 2> pair2 = concat(make_vector(10), make_vector(20));
    CHECK(pair2 == vec<int, 2>{ 10, 20 });

    // * Repeat vector multiple times:
    const vec<short, 8> repeated = repeat<4>(make_vector<short>(0, -1));
    CHECK(repeated == vec<short, 8>{ 0, -1, 0, -1, 0, -1, 0, -1 });

    // * Use enumerate to generate sequence of numbers:
    const vec<int, 8> eight = enumerate<int, 8>();
    CHECK(eight == vec<int, 8>{ 0, 1, 2, 3, 4, 5, 6, 7 });

    // * Vectors can be of any length...
    const vec<int, 1> one{ 42 };
    const vec<int, 2> two = concat(one, make_vector(42));
    CHECK(two == vec<int, 2>{ 42, 42 });

    const vec<u8, 256> very_long_vector = repeat<64>(make_vector<u8>(1, 2, 4, 8));
    CHECK(slice<0, 17>(very_long_vector) ==
          vec<unsigned char, 17>{ 1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8, 1 });

    // * ...really any:
    using big_vector = vec<i16, 107>;
    big_vector v107  = enumerate<i16, 107>();
    CHECK(hadd(v107) == static_cast<short>(5671));

    using color       = vec<u8, 3>;
    const color green = cast<u8>(make_vector(0.0, 1.0, 0.0) * 255);
    CHECK(green == vec<unsigned char, 3>{ 0, 255, 0 });

    // Vectors support all standard operators:
    const auto op1    = make_vector(0, 1, 10, 100);
    const auto op2    = make_vector(20, 2, -2, 200);
    const auto result = op1 * op2 - 4;
    CHECK(result == vec<int, 4>{ -4, -2, -24, 19996 });

    // * Transform vector:
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    const vec<int, 8> numbers2 = enumerate<int, 8>() + 100;
    CHECK(odd(numbers1) == vec<int, 4>{ 1, 3, 5, 7 });
    CHECK(even(numbers2) == vec<int, 4>{ 100, 102, 104, 106 });

    // * The following command pairs are equivalent:
    CHECK(permute(numbers1, elements<0, 2, 1, 3, 4, 6, 5, 7>) == vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 });
    CHECK(permute(numbers1, elements<0, 2, 1, 3>) == vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 });

    CHECK(shuffle(numbers1, numbers2, elements<0, 8, 2, 10, 4, 12, 6, 14>) ==
          vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 });
    CHECK(shuffle(numbers1, numbers2, elements<0, 8>) == vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 });

    CHECK(blend(numbers1, numbers2, elements<0, 1, 1, 0, 1, 1, 0, 1>) ==
          vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 });
    CHECK(blend(numbers1, numbers2, elements<0, 1, 1>) == vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 });

    // * Transpose matrix:
    const auto sixteen = enumerate<float, 16>();
    CHECK(transpose<4>(sixteen) == vec<float, 16>{ 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 });
}

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

TEST(test_delay)
{
    const univector<float, 33> v1 = counter() + 100;
    const univector<float, 33> v2 = delay(v1);
    CHECK(v2[0] == 0);
    CHECK(v2[1] == 100);
    CHECK(v2[2] == 101);
    CHECK(v2[19] == 118);

    const univector<float, 33> v3 = delay(v1, csize<3>);
    CHECK(v3[0] == 0);
    CHECK(v3[1] == 0);
    CHECK(v3[2] == 0);
    CHECK(v3[3] == 100);
    CHECK(v3[4] == 101);
    CHECK(v3[19] == 116);

}

int main(int argc, char** argv) { return testo::run_all("", true); }
