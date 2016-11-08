/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/io.hpp>

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
    CHECK(permute(numbers1, elements_t<0, 2, 1, 3, 4, 6, 5, 7>()) == vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 });
    CHECK(permute(numbers1, elements_t<0, 2, 1, 3>()) == vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 });

    CHECK(shuffle(numbers1, numbers2, elements_t<0, 8, 2, 10, 4, 12, 6, 14>()) ==
          vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 });
    CHECK(shuffle(numbers1, numbers2, elements_t<0, 8>()) == vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 });

    CHECK(blend(numbers1, numbers2, elements_t<0, 1, 1, 0, 1, 1, 0, 1>()) ==
          vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 });
    CHECK(blend(numbers1, numbers2, elements_t<0, 1, 1>()) ==
          vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 });

    CHECK(splitpairs(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 2, 4, 6, 1, 3, 5, 7));
    CHECK(splitpairs<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 1, 4, 5, 2, 3, 6, 7));

    CHECK(interleavehalfs(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 4, 1, 5, 2, 6, 3, 7));
    CHECK(interleavehalfs<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 1, 4, 5, 2, 3, 6, 7));

    CHECK(subadd(pack(0, 1, 2, 3, 4, 5, 6, 7), pack(10, 10, 10, 10, 10, 10, 10, 10)) ==
          pack(-10, 11, -8, 13, -6, 15, -4, 17));
    CHECK(addsub(pack(0, 1, 2, 3, 4, 5, 6, 7), pack(10, 10, 10, 10, 10, 10, 10, 10)) ==
          pack(10, -9, 12, -7, 14, -5, 16, -3));

    CHECK(broadcast<8>(1) == pack(1, 1, 1, 1, 1, 1, 1, 1));
    CHECK(broadcast<8>(1, 2) == pack(1, 2, 1, 2, 1, 2, 1, 2));
    CHECK(broadcast<8>(1, 2, 3, 4) == pack(1, 2, 3, 4, 1, 2, 3, 4));
    CHECK(broadcast<8>(1, 2, 3, 4, 5, 6, 7, 8) == pack(1, 2, 3, 4, 5, 6, 7, 8));

    CHECK(even(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 2, 4, 6));
    CHECK(odd(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(1, 3, 5, 7));

    CHECK(even<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 1, 4, 5));
    CHECK(odd<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(2, 3, 6, 7));

    CHECK(reverse(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(7, 6, 5, 4, 3, 2, 1, 0));
    CHECK(reverse<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(6, 7, 4, 5, 2, 3, 0, 1));
    CHECK(reverse<4>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(4, 5, 6, 7, 0, 1, 2, 3));

    CHECK(digitreverse4(pack(0.f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)) ==
          pack(0.f, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15));

    CHECK(dup(pack(0, 1, 2, 3)) == pack(0, 0, 1, 1, 2, 2, 3, 3));
    CHECK(duphalfs(pack(0, 1, 2, 3)) == pack(0, 1, 2, 3, 0, 1, 2, 3));
    CHECK(dupeven(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 0, 2, 2, 4, 4, 6, 6));
    CHECK(dupodd(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(1, 1, 3, 3, 5, 5, 7, 7));

    CHECK(inrange(pack(1, 2, 3), 1, 3) == make_mask<int>(true, true, true));
    CHECK(inrange(pack(1, 2, 3), 1, 2) == make_mask<int>(true, true, false));
    CHECK(inrange(pack(1, 2, 3), 1, 1) == make_mask<int>(true, false, false));

    // * Transpose matrix:
    const auto sixteen = enumerate<float, 16>();
    CHECK(transpose<4>(sixteen) == vec<float, 16>(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15));
}

TEST(concat)
{
    CHECK(concat(vec<f32, 1>{ 1 }, vec<f32, 2>{ 2, 3 }, vec<f32, 1>{ 4 }, vec<f32, 3>{ 5, 6, 7 }) //
          == vec<f32, 7>{ 1, 2, 3, 4, 5, 6, 7 });
}

TEST(split)
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

TEST(broadcast)
{
    CHECK(broadcast<5>(3.f) == vec<f32, 5>{ 3, 3, 3, 3, 3 });
    CHECK(broadcast<6>(1.f, 2.f) == vec<f32, 6>{ 1, 2, 1, 2, 1, 2 });
    CHECK(broadcast<6>(1.f, 2.f, 3.f) == vec<f32, 6>{ 1, 2, 3, 1, 2, 3 });
}

TEST(resize)
{
    CHECK(resize<5>(make_vector(3.f)) == vec<f32, 5>{ 3, 3, 3, 3, 3 });
    CHECK(resize<6>(make_vector(1.f, 2.f)) == vec<f32, 6>{ 1, 2, 1, 2, 1, 2 });
    CHECK(resize<6>(make_vector(1.f, 2.f, 3.f)) == vec<f32, 6>{ 1, 2, 3, 1, 2, 3 });
}

TEST(make_vector)
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

TEST(apply)
{
    CHECK(apply([](int x) { return x + 1; }, make_vector(1, 2, 3, 4, 5)) == make_vector(2, 3, 4, 5, 6));
    CHECK(apply(fn::sqr(), make_vector(1, 2, 3, 4, 5)) == make_vector(1, 4, 9, 16, 25));
}

TEST(zerovector)
{
    CHECK(zerovector<f32, 3>() == f32x3{ 0, 0, 0 });
    // CHECK(zerovector<i16, 3>() == i16x3{ 0, 0, 0 }); // clang 3.9 (trunk) crashes here
    CHECK(zerovector(f64x8{}) == f64x8{ 0, 0, 0, 0, 0, 0, 0, 0 });
}

TEST(allonesvector)
{
    CHECK(bitcast<u32>(constants<f32>::allones()) == 0xFFFFFFFFu);
    CHECK(bitcast<u64>(constants<f64>::allones()) == 0xFFFFFFFFFFFFFFFFull);

    CHECK(~allonesvector<f32, 3>() == f32x3{ 0, 0, 0 });
    CHECK(allonesvector<i16, 3>() == i16x3{ -1, -1, -1 });
    CHECK(allonesvector<u8, 3>() == u8x3{ 255, 255, 255 });
}

TEST(low_high)
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

#ifdef CMT_COMPILER_CLANG
TEST(matrix)
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
#endif

TEST(is_convertible)
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

    testo::assert_is_same<i32x4, common_type<i32x4>>();
    testo::assert_is_same<u32x4, common_type<i32x4, u32x4>>();
    testo::assert_is_same<f64x4, common_type<i32x4, u32x4, f64x4>>();

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

TEST(transcendental)
{
    CHECK(kfr::sin(1.0f) == 0.8414709848078965066525023216303f);
    CHECK(kfr::sin(1.0) == 0.8414709848078965066525023216303);

    CHECK(kfr::cos(1.0f) == 0.54030230586813971740093660744298f);
    CHECK(kfr::cos(1.0) == 0.54030230586813971740093660744298);

    CHECK(kfr::tan(1.0f) == 1.5574077246549022305069748074584f);
    CHECK(kfr::tan(1.0) == 1.5574077246549022305069748074584);

    CHECK(kfr::asin(0.45f) == 0.46676533904729636185033976030414f);
    CHECK(kfr::asin(0.45) == 0.46676533904729636185033976030414);

    CHECK(kfr::acos(0.45f) == 1.1040309877476002573809819313356f);
    CHECK(kfr::acos(0.45) == 1.1040309877476002573809819313356);

    CHECK(kfr::atan(0.45f) == 0.42285392613294071296648279098114f);
    CHECK(kfr::atan(0.45) == 0.42285392613294071296648279098114);

    CHECK(kfr::sinh(1.0f) == 1.1752011936438014568823818505956f);
    CHECK(kfr::sinh(1.0) == 1.1752011936438014568823818505956);

    CHECK(kfr::cosh(1.0f) == 1.5430806348152437784779056207571f);
    CHECK(kfr::cosh(1.0) == 1.5430806348152437784779056207571);

    CHECK(kfr::tanh(1.0f) == 0.76159415595576488811945828260479f);
    CHECK(kfr::tanh(1.0) == 0.76159415595576488811945828260479);

    CHECK(kfr::exp(0.75f) == 2.1170000166126746685453698198371f);
    CHECK(kfr::exp(0.75) == 2.1170000166126746685453698198371);

    CHECK(kfr::exp(-0.75f) == 0.47236655274101470713804655094327f);
    CHECK(kfr::exp(-0.75) == 0.47236655274101470713804655094327);

    CHECK(kfr::log(2.45f) == 0.89608802455663561677548191074382f);
    CHECK(kfr::log(2.45) == 0.89608802455663561677548191074382);
}

TEST(horner)
{
    CHECK(horner(pack(0, 1, 2, 3), 1, 2, 3) == pack(1, 6, 17, 34));
    CHECK(horner_odd(pack(0, 1, 2, 3), 1, 2, 3) == pack(0, 6, 114, 786));
    CHECK(horner_even(pack(0, 1, 2, 3), 1, 2, 3) == pack(1, 6, 57, 262));
}

TEST(test_stat)
{
    {
        univector<float, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(rms(a) == 3.316624790355399849115f);
        CHECK(product(a) == 120);
    }
    {
        univector<double, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(rms(a) == 3.316624790355399849115);
        CHECK(product(a) == 120);
    }
    {
        univector<int, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(product(a) == 120);
    }
    {
        univector<complex<float>, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == c32{ 15 });
        CHECK(mean(a) == c32{ 3 });
        CHECK(sumsqr(a) == c32{ 55 });
        CHECK(product(a) == c32{ 120 });
    }
}

int main()
{
    println(library_version());
    return testo::run_all("", true);
}
