/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/io.hpp>
#include <kfr/simd/shuffle.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("concat")
{
    CHECK_THAT((concat(vec<f32, 1>{ 1 }, vec<f32, 2>{ 2, 3 }, vec<f32, 1>{ 4 }, vec<f32, 3>{ 5, 6, 7 })),
               DeepMatcher(vec<f32, 7>{ 1, 2, 3, 4, 5, 6, 7 }));
}

TEST_CASE("concat12")
{
    vec<float, 6> a0{ 1, 11, 2, 22, 3, 33 };
    vec<float, 6> a1{ 4, 44, 5, 55, 6, 66 };
    vec<float, 12> r = concat(a0, a1);
    CHECK_THAT((r), DeepMatcher(pack<float>(1, 11, 2, 22, 3, 33, 4, 44, 5, 55, 6, 66)));
}

TEST_CASE("concat18")
{
    vec<float, 6> a0{ 1, 11, 2, 22, 3, 33 };
    vec<float, 6> a1{ 4, 44, 5, 55, 6, 66 };
    vec<float, 6> a2{ 7, 77, 8, 88, 9, 99 };
    vec<float, 18> r = concat(a0, a1, a2);
    CHECK_THAT((r), DeepMatcher(pack<float>(1, 11, 2, 22, 3, 33, 4, 44, 5, 55, 6, 66, 7, 77, 8, 88, 9, 99)));
}

TEST_CASE("split18")
{
    vec<float, 18> r{ 1, 11, 2, 22, 3, 33, 4, 44, 5, 55, 6, 66, 7, 77, 8, 88, 9, 99 };
    CHECK_THAT((slice<0, 6>(r)), DeepMatcher(pack<float>(1, 11, 2, 22, 3, 33)));
    CHECK_THAT((slice<6, 6>(r)), DeepMatcher(pack<float>(4, 44, 5, 55, 6, 66)));
    CHECK_THAT((slice<12, 6>(r)), DeepMatcher(pack<float>(7, 77, 8, 88, 9, 99)));
}

TEST_CASE("reverse")
{
    CHECK_THAT((reverse(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(7, 6, 5, 4, 3, 2, 1, 0)));
    CHECK_THAT((reverse<2>(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(6, 7, 4, 5, 2, 3, 0, 1)));
    CHECK_THAT((reverse<4>(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(4, 5, 6, 7, 0, 1, 2, 3)));
}

TEST_CASE("shuffle")
{
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    const vec<int, 8> numbers2 = enumerate<int, 8, 100>();
    CHECK_THAT((shuffle(numbers1, numbers2, elements_t<0, 8, 2, 10, 4, 12, 6, 14>())),
               DeepMatcher(vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 }));
    CHECK_THAT((shuffle(numbers1, numbers2, elements_t<0, 8>())),
               DeepMatcher(vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 }));
}

TEST_CASE("permute")
{
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    CHECK_THAT((permute(numbers1, elements_t<0, 2, 1, 3, 4, 6, 5, 7>())),
               DeepMatcher(vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 }));
    CHECK_THAT((permute(numbers1, elements_t<0, 2, 1, 3>())),
               DeepMatcher(vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 }));
}

TEST_CASE("blend")
{
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    const vec<int, 8> numbers2 = enumerate<int, 8, 100>();
    CHECK_THAT((blend(numbers1, numbers2, elements_t<0, 1, 1, 0, 1, 1, 0, 1>())),
               DeepMatcher(vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 }));
    CHECK_THAT((blend(numbers1, numbers2, elements_t<0, 1, 1>())),
               DeepMatcher(vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 }));
}

TEST_CASE("duplicate_shuffle")
{
    CHECK_THAT((dup(pack(0, 1, 2, 3))), DeepMatcher(pack(0, 0, 1, 1, 2, 2, 3, 3)));
    CHECK_THAT((duphalves(pack(0, 1, 2, 3))), DeepMatcher(pack(0, 1, 2, 3, 0, 1, 2, 3)));
    CHECK_THAT((dupeven(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(0, 0, 2, 2, 4, 4, 6, 6)));
    CHECK_THAT((dupodd(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(1, 1, 3, 3, 5, 5, 7, 7)));
}

TEST_CASE("split_interleave")
{
    vec<f32, 1> a1;
    vec<f32, 2> a23;
    vec<f32, 1> a4;
    vec<f32, 3> a567;
    split(vec<f32, 7>{ 1, 2, 3, 4, 5, 6, 7 }, a1, a23, a4, a567);
    CHECK_THAT((a1), DeepMatcher(vec<f32, 1>{ 1 }));
    CHECK_THAT((a23), DeepMatcher(vec<f32, 2>{ 2, 3 }));
    CHECK_THAT((a4), DeepMatcher(vec<f32, 1>{ 4 }));
    CHECK_THAT((a567), DeepMatcher(vec<f32, 3>{ 5, 6, 7 }));

    CHECK_THAT((splitpairs(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(0, 2, 4, 6, 1, 3, 5, 7)));
    CHECK_THAT((splitpairs<2>(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(0, 1, 4, 5, 2, 3, 6, 7)));

    CHECK_THAT((interleavehalves(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(0, 4, 1, 5, 2, 6, 3, 7)));
    CHECK_THAT((interleavehalves<2>(pack(0, 1, 2, 3, 4, 5, 6, 7))),
               DeepMatcher(pack(0, 1, 4, 5, 2, 3, 6, 7)));
}

TEST_CASE("zip")
{
    CHECK_THAT((zip(pack(1, 2, 3, 4), pack(10, 20, 30, 40))),
               DeepMatcher(pack(pack(1, 10), pack(2, 20), pack(3, 30), pack(4, 40))));

    CHECK_THAT((zip(pack(1, 2, 3, 4), pack(10, 20, 30, 40), pack(111, 222, 333, 444), pack(-1, -2, -3, -4))),
               DeepMatcher(pack(pack(1, 10, 111, -1), pack(2, 20, 222, -2), pack(3, 30, 333, -3),
                                pack(4, 40, 444, -4))));
}

TEST_CASE("column")
{
    CHECK_THAT((column<1>(pack(pack(0, 1), pack(2, 3), pack(4, 5), pack(6, 7)))),
               DeepMatcher(pack(1, 3, 5, 7)));

    CHECK_THAT((column<0>(pack(pack(0., 1.), pack(2., 3.), pack(4., 5.), pack(6., 7.)))),
               DeepMatcher(pack(0., 2., 4., 6.)));
}

TEST_CASE("broadcast")
{
    CHECK_THAT((broadcast<8>(1)), DeepMatcher(pack(1, 1, 1, 1, 1, 1, 1, 1)));
    CHECK_THAT((broadcast<8>(1, 2)), DeepMatcher(pack(1, 2, 1, 2, 1, 2, 1, 2)));
    CHECK_THAT((broadcast<8>(1, 2, 3, 4)), DeepMatcher(pack(1, 2, 3, 4, 1, 2, 3, 4)));
    CHECK_THAT((broadcast<8>(1, 2, 3, 4, 5, 6, 7, 8)), DeepMatcher(pack(1, 2, 3, 4, 5, 6, 7, 8)));

    CHECK_THAT((broadcast<5>(3.f)), DeepMatcher(vec<f32, 5>{ 3, 3, 3, 3, 3 }));
    CHECK_THAT((broadcast<6>(1.f, 2.f)), DeepMatcher(vec<f32, 6>{ 1, 2, 1, 2, 1, 2 }));
    CHECK_THAT((broadcast<6>(1.f, 2.f, 3.f)), DeepMatcher(vec<f32, 6>{ 1, 2, 3, 1, 2, 3 }));
}

TEST_CASE("resize")
{
    CHECK_THAT((resize<5>(make_vector(3.f))), DeepMatcher(vec<f32, 5>{ 3, 3, 3, 3, 3 }));
    CHECK_THAT((resize<6>(make_vector(1.f, 2.f))), DeepMatcher(vec<f32, 6>{ 1, 2, 1, 2, 1, 2 }));
    CHECK_THAT((resize<6>(make_vector(1.f, 2.f, 3.f))), DeepMatcher(vec<f32, 6>{ 1, 2, 3, 1, 2, 3 }));
}

TEST_CASE("make_vector")
{
    const signed char ch = -1;
    CHECK_THAT((make_vector(1, 2, ch)), DeepMatcher(vec<i32, 3>{ 1, 2, -1 }));
    const i64 v = -100;
    CHECK_THAT((make_vector(1, 2, v)), DeepMatcher(vec<i64, 3>{ 1, 2, -100 }));
    CHECK_THAT((make_vector<i64>(1, 2, ch)), DeepMatcher(vec<i64, 3>{ 1, 2, -1 }));
    CHECK_THAT((make_vector<f32>(1, 2, ch)), DeepMatcher(vec<f32, 3>{ 1, 2, -1 }));

    CHECK_THAT((make_vector(f64x2{ 1, 2 }, f64x2{ 10, 20 })),
               DeepMatcher(vec<vec<f64, 2>, 2>{ f64x2{ 1, 2 }, f64x2{ 10, 20 } }));
    CHECK_THAT((make_vector(1.f, f32x2{ 10, 20 })),
               DeepMatcher(vec<vec<f32, 2>, 2>{ f32x2{ 1, 1 }, f32x2{ 10, 20 } }));
}

TEST_CASE("zerovector")
{
    CHECK_THAT((zerovector<f32, 3>()), DeepMatcher(f32x3{ 0, 0, 0 }));
    // CHECK_THAT((zerovector<i16, 3>() ), DeepMatcher( i16x3{ 0, 0, 0 }); // clang 3.9 (trunk)) crashes here
    CHECK_THAT((zerovector(f64x8{})), DeepMatcher(f64x8{ 0, 0, 0, 0, 0, 0, 0, 0 }));
}

TEST_CASE("allonesvector")
{
    CHECK(bitcast<u32>(special_constants<f32>::allones()) == 0xFFFFFFFFu);
    CHECK(bitcast<u64>(special_constants<f64>::allones()) == 0xFFFFFFFFFFFFFFFFull);

    CHECK_THAT((allonesvector<i16, 3>()), DeepMatcher(i16x3{ -1, -1, -1 }));
    CHECK_THAT((allonesvector<u8, 3>()), DeepMatcher(u8x3{ 255, 255, 255 }));
}

TEST_CASE("transpose")
{
    const auto sixteen = enumerate<float, 16>();
    CHECK_THAT((transpose<4>(sixteen)),
               DeepMatcher(vec<float, 16>(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15)));
}

TEST_CASE("odd_even")
{
    CHECK_THAT((even(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(0, 2, 4, 6)));
    CHECK_THAT((odd(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(1, 3, 5, 7)));

    CHECK_THAT((even<2>(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(0, 1, 4, 5)));
    CHECK_THAT((odd<2>(pack(0, 1, 2, 3, 4, 5, 6, 7))), DeepMatcher(pack(2, 3, 6, 7)));
}

TEST_CASE("low_high")
{
    CHECK_THAT((low(vec<u8, 8>(1, 2, 3, 4, 5, 6, 7, 8))), DeepMatcher(vec<u8, 4>(1, 2, 3, 4)));
    CHECK_THAT((high(vec<u8, 8>(1, 2, 3, 4, 5, 6, 7, 8))), DeepMatcher(vec<u8, 4>(5, 6, 7, 8)));

    CHECK_THAT((low(vec<u8, 7>(1, 2, 3, 4, 5, 6, 7))), DeepMatcher(vec<u8, 4>(1, 2, 3, 4)));
    CHECK_THAT((high(vec<u8, 7>(1, 2, 3, 4, 5, 6, 7))), DeepMatcher(vec<u8, 3>(5, 6, 7)));

    CHECK_THAT((low(vec<u8, 6>(1, 2, 3, 4, 5, 6))), DeepMatcher(vec<u8, 4>(1, 2, 3, 4)));
    CHECK_THAT((high(vec<u8, 6>(1, 2, 3, 4, 5, 6))), DeepMatcher(vec<u8, 2>(5, 6)));

    CHECK_THAT((low(vec<u8, 5>(1, 2, 3, 4, 5))), DeepMatcher(vec<u8, 4>(1, 2, 3, 4)));
    CHECK_THAT((high(vec<u8, 5>(1, 2, 3, 4, 5))), DeepMatcher(vec<u8, 1>(5)));

    CHECK_THAT((low(vec<u8, 4>(1, 2, 3, 4))), DeepMatcher(vec<u8, 2>(1, 2)));
    CHECK_THAT((high(vec<u8, 4>(1, 2, 3, 4))), DeepMatcher(vec<u8, 2>(3, 4)));

    CHECK_THAT((low(vec<u8, 3>(1, 2, 3))), DeepMatcher(vec<u8, 2>(1, 2)));
    CHECK_THAT((high(vec<u8, 3>(1, 2, 3))), DeepMatcher(vec<u8, 1>(3)));

    CHECK_THAT((low(vec<u8, 2>(1, 2))), DeepMatcher(vec<u8, 1>(1)));
    CHECK_THAT((high(vec<u8, 2>(1, 2))), DeepMatcher(vec<u8, 1>(2)));
}
TEST_CASE("enumerate")
{
    CHECK_THAT((enumerate(vec_shape<int, 4>{}, 4)), DeepMatcher(vec{ 0, 4, 8, 12 }));
    CHECK_THAT((enumerate(vec_shape<int, 8>{}, 3)), DeepMatcher(vec{ 0, 3, 6, 9, 12, 15, 18, 21 }));
    CHECK_THAT((enumerate(vec_shape<int, 7>{}, 3)), DeepMatcher(vec{ 0, 3, 6, 9, 12, 15, 18 }));
}

TEST_CASE("test_basic")
{
    // How to make a vector:

    // * Use constructor
    const vec<double, 4> first{ 1, 2.5, -infinity, 3.1415926 };
    CHECK_THAT((first), DeepMatcher(vec<double, 4>{ 1, 2.5, -infinity, 3.1415926 }));

    // * Use make_vector function
    const auto second = make_vector(-1, +1);
    CHECK_THAT((second), DeepMatcher(vec<int, 2>{ -1, 1 }));

    // * Convert from vector of other type:
    const vec<int, 4> int_vector{ 10, 20, 30, 40 };
    const vec<double, 4> double_vector = cast<double>(int_vector);
    CHECK_THAT((double_vector), DeepMatcher(vec<double, 4>{ 10, 20, 30, 40 }));

    // * Concat two vectors:
    const vec<int, 1> left_part{ 1 };
    const vec<int, 1> right_part{ 2 };
    const vec<int, 2> pair{ left_part, right_part };
    CHECK_THAT((pair), DeepMatcher(vec<int, 2>{ 1, 2 }));

    // * Same, but using make_vector and concat:
    const vec<int, 2> pair2 = concat(make_vector(10), make_vector(20));
    CHECK_THAT((pair2), DeepMatcher(vec<int, 2>{ 10, 20 }));

    // * Repeat vector multiple times:
    const vec<short, 8> repeated = repeat<4>(make_vector<short>(0, -1));
    CHECK_THAT((repeated), DeepMatcher(vec<short, 8>{ 0, -1, 0, -1, 0, -1, 0, -1 }));

    // * Use enumerate to generate sequence of numbers:
    const vec<int, 8> eight = enumerate<int, 8>();
    CHECK_THAT((eight), DeepMatcher(vec<int, 8>{ 0, 1, 2, 3, 4, 5, 6, 7 }));

    // * Vectors can be of any length...
    const vec<int, 1> one{ 42 };
    const vec<int, 2> two = concat(one, make_vector(42));
    CHECK_THAT((two), DeepMatcher(vec<int, 2>{ 42, 42 }));

    const vec<u8, 256> very_long_vector = repeat<64>(make_vector<u8>(1, 2, 4, 8));
    CHECK_THAT((slice<0, 17>(very_long_vector)),
               DeepMatcher(vec<unsigned char, 17>{ 1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8, 1, 2, 4, 8, 1 }));

    // * ...really any:
    using big_vector = vec<i16, 107>;
    big_vector v107  = enumerate<i16, 107>();
    CHECK_THAT((hadd(v107)), DeepMatcher(static_cast<short>(5671)));

    using color       = vec<u8, 3>;
    const color green = cast<u8>(make_vector(0.0, 1.0, 0.0) * 255);
    CHECK_THAT((green), DeepMatcher(vec<unsigned char, 3>{ 0, 255, 0 }));

    // Vectors support all standard operators:
    const auto op1    = make_vector(0, 1, 10, 100);
    const auto op2    = make_vector(20, 2, -2, 200);
    const auto result = op1 * op2 - 4;
    CHECK_THAT((result), DeepMatcher(vec<int, 4>{ -4, -2, -24, 19996 }));

    // * Transform vector:
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    const vec<int, 8> numbers2 = enumerate<int, 8>() + 100;
    CHECK_THAT((odd(numbers1)), DeepMatcher(vec<int, 4>{ 1, 3, 5, 7 }));
    CHECK_THAT((even(numbers2)), DeepMatcher(vec<int, 4>{ 100, 102, 104, 106 }));

    CHECK_THAT((subadd(pack(0, 1, 2, 3, 4, 5, 6, 7), pack(10, 10, 10, 10, 10, 10, 10, 10))),
               DeepMatcher(pack(-10, 11, -8, 13, -6, 15, -4, 17)));
    CHECK_THAT((addsub(pack(0, 1, 2, 3, 4, 5, 6, 7), pack(10, 10, 10, 10, 10, 10, 10, 10))),
               DeepMatcher(pack(10, -9, 12, -7, 14, -5, 16, -3)));

    CHECK_THAT((digitreverse4(pack(0.f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15))),
               DeepMatcher(pack(0.f, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15)));

    CHECK_THAT((inrange(pack(1, 2, 3), 1, 3)), DeepMatcher(make_mask<int>(true, true, true)));
    CHECK_THAT((inrange(pack(1, 2, 3), 1, 2)), DeepMatcher(make_mask<int>(true, true, false)));
    CHECK_THAT((inrange(pack(1, 2, 3), 1, 1)), DeepMatcher(make_mask<int>(true, false, false)));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
