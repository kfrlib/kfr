/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>

#include <kfr/cometa/string.hpp>
#include <kfr/math.hpp>
#include <kfr/version.hpp>

#include "testo/testo.hpp"

using namespace kfr;

TEST(test)
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

int main(int /*argc*/, char** /*argv*/)
{
    println(library_version());

    return testo::run_all("", true);
}
