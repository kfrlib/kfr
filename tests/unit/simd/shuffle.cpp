/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/io.hpp>
#include <kfr/simd/shuffle.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(concat)
{
    CHECK(concat(vec<f32, 1>{ 1 }, vec<f32, 2>{ 2, 3 }, vec<f32, 1>{ 4 }, vec<f32, 3>{ 5, 6, 7 }) //
          == vec<f32, 7>{ 1, 2, 3, 4, 5, 6, 7 });
}

TEST(reverse)
{
    CHECK(reverse(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(7, 6, 5, 4, 3, 2, 1, 0));
    CHECK(reverse<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(6, 7, 4, 5, 2, 3, 0, 1));
    CHECK(reverse<4>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(4, 5, 6, 7, 0, 1, 2, 3));
}

TEST(shuffle)
{
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    const vec<int, 8> numbers2 = enumerate<int, 8, 100>();
    CHECK(shuffle(numbers1, numbers2, elements_t<0, 8, 2, 10, 4, 12, 6, 14>()) ==
          vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 });
    CHECK(shuffle(numbers1, numbers2, elements_t<0, 8>()) == vec<int, 8>{ 0, 100, 2, 102, 4, 104, 6, 106 });
}

TEST(permute)
{
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    CHECK(permute(numbers1, elements_t<0, 2, 1, 3, 4, 6, 5, 7>()) == vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 });
    CHECK(permute(numbers1, elements_t<0, 2, 1, 3>()) == vec<int, 8>{ 0, 2, 1, 3, 4, 6, 5, 7 });
}

TEST(blend)
{
    const vec<int, 8> numbers1 = enumerate<int, 8>();
    const vec<int, 8> numbers2 = enumerate<int, 8, 100>();
    CHECK(blend(numbers1, numbers2, elements_t<0, 1, 1, 0, 1, 1, 0, 1>()) ==
          vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 });
    CHECK(blend(numbers1, numbers2, elements_t<0, 1, 1>()) ==
          vec<int, 8>{ 0, 101, 102, 3, 104, 105, 6, 107 });
}

TEST(duplicate_shuffle)
{
    CHECK(dup(pack(0, 1, 2, 3)) == pack(0, 0, 1, 1, 2, 2, 3, 3));
    CHECK(duphalves(pack(0, 1, 2, 3)) == pack(0, 1, 2, 3, 0, 1, 2, 3));
    CHECK(dupeven(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 0, 2, 2, 4, 4, 6, 6));
    CHECK(dupodd(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(1, 1, 3, 3, 5, 5, 7, 7));
}

TEST(split_interleave)
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

    CHECK(splitpairs(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 2, 4, 6, 1, 3, 5, 7));
    CHECK(splitpairs<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 1, 4, 5, 2, 3, 6, 7));

    CHECK(interleavehalves(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 4, 1, 5, 2, 6, 3, 7));
    CHECK(interleavehalves<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 1, 4, 5, 2, 3, 6, 7));
}

TEST(zip)
{
    CHECK(zip(pack(1, 2, 3, 4), pack(10, 20, 30, 40)) ==
          pack(pack(1, 10), pack(2, 20), pack(3, 30), pack(4, 40)));

    CHECK(zip(pack(1, 2, 3, 4), pack(10, 20, 30, 40), pack(111, 222, 333, 444), pack(-1, -2, -3, -4)) ==
          pack(pack(1, 10, 111, -1), pack(2, 20, 222, -2), pack(3, 30, 333, -3), pack(4, 40, 444, -4)));
}

TEST(column)
{
    CHECK(column<1>(pack(pack(0, 1), pack(2, 3), pack(4, 5), pack(6, 7))) == pack(1, 3, 5, 7));

    CHECK(column<0>(pack(pack(0., 1.), pack(2., 3.), pack(4., 5.), pack(6., 7.))) == pack(0., 2., 4., 6.));
}

TEST(broadcast)
{
    CHECK(broadcast<8>(1) == pack(1, 1, 1, 1, 1, 1, 1, 1));
    CHECK(broadcast<8>(1, 2) == pack(1, 2, 1, 2, 1, 2, 1, 2));
    CHECK(broadcast<8>(1, 2, 3, 4) == pack(1, 2, 3, 4, 1, 2, 3, 4));
    CHECK(broadcast<8>(1, 2, 3, 4, 5, 6, 7, 8) == pack(1, 2, 3, 4, 5, 6, 7, 8));

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

TEST(zerovector)
{
    CHECK(zerovector<f32, 3>() == f32x3{ 0, 0, 0 });
    // CHECK(zerovector<i16, 3>() == i16x3{ 0, 0, 0 }); // clang 3.9 (trunk) crashes here
    CHECK(zerovector(f64x8{}) == f64x8{ 0, 0, 0, 0, 0, 0, 0, 0 });
}

TEST(allonesvector)
{
    CHECK(bitcast<u32>(special_constants<f32>::allones()) == 0xFFFFFFFFu);
    CHECK(bitcast<u64>(special_constants<f64>::allones()) == 0xFFFFFFFFFFFFFFFFull);

    CHECK(allonesvector<i16, 3>() == i16x3{ -1, -1, -1 });
    CHECK(allonesvector<u8, 3>() == u8x3{ 255, 255, 255 });
}

TEST(transpose)
{
    const auto sixteen = enumerate<float, 16>();
    CHECK(transpose<4>(sixteen) == vec<float, 16>(0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15));
}

TEST(odd_even)
{
    CHECK(even(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 2, 4, 6));
    CHECK(odd(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(1, 3, 5, 7));

    CHECK(even<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(0, 1, 4, 5));
    CHECK(odd<2>(pack(0, 1, 2, 3, 4, 5, 6, 7)) == pack(2, 3, 6, 7));
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
TEST(enumerate)
{
    CHECK(enumerate(vec_shape<int, 4>{}, 4) == vec{ 0, 4, 8, 12 });
    CHECK(enumerate(vec_shape<int, 8>{}, 3) == vec{ 0, 3, 6, 9, 12, 15, 18, 21 });
    CHECK(enumerate(vec_shape<int, 7>{}, 3) == vec{ 0, 3, 6, 9, 12, 15, 18 });
}


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

    CHECK(subadd(pack(0, 1, 2, 3, 4, 5, 6, 7), pack(10, 10, 10, 10, 10, 10, 10, 10)) ==
          pack(-10, 11, -8, 13, -6, 15, -4, 17));
    CHECK(addsub(pack(0, 1, 2, 3, 4, 5, 6, 7), pack(10, 10, 10, 10, 10, 10, 10, 10)) ==
          pack(10, -9, 12, -7, 14, -5, 16, -3));

    CHECK(digitreverse4(pack(0.f, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15)) ==
          pack(0.f, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15));

    CHECK(inrange(pack(1, 2, 3), 1, 3) == make_mask<int>(true, true, true));
    CHECK(inrange(pack(1, 2, 3), 1, 2) == make_mask<int>(true, true, false));
    CHECK(inrange(pack(1, 2, 3), 1, 1) == make_mask<int>(true, false, false));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
