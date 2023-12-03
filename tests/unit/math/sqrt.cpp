/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/math/sqrt.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(intrin_sqrt)
{
    testo::assert_is_same<decltype(kfr::sqrt(9)), fbase>();
    testo::assert_is_same<decltype(kfr::intrinsics::sqrt(9)), fbase>();
    testo::assert_is_same<decltype(kfr::sqrt(make_vector(9))), vec<fbase, 1>>();
    testo::assert_is_same<decltype(kfr::sqrt(make_vector(9, 25))), vec<fbase, 2>>();
    CHECK(kfr::sqrt(9) == fbase(3.0));
    CHECK(kfr::sqrt(2) == fbase(1.4142135623730950488));
    CHECK(kfr::sqrt(-9) == fbase(qnan));
    CHECK(kfr::sqrt(make_vector(9)) == make_vector<fbase>(3.0));
    CHECK(kfr::sqrt(make_vector(-9)) == make_vector<fbase>(qnan));
    testo::matrix(named("type") = float_vector_types<vec>, named("value") = std::vector<int>{ 0, 2, 65536 },
                  [](auto type, int value)
                  {
                      using T = typename decltype(type)::type;
                      const T x(value);
                      CHECK(kfr::sqrt(x) == apply([](auto x) -> decltype(x) { return std::sqrt(x); }, x));
                  });
}


}
