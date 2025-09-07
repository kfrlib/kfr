/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/test/test.hpp>

#include <kfr/math/sqrt.hpp>

using namespace kfr;

namespace KFR_ARCH_NAME
{

TEST_CASE("intrin_sqrt")
{
    assert_is_same<decltype(kfr::sqrt(9)), fbase>();
    assert_is_same<decltype(kfr::intr::sqrt(9)), fbase>();
    assert_is_same<decltype(kfr::sqrt(make_vector(9))), vec<fbase, 1>>();
    assert_is_same<decltype(kfr::sqrt(make_vector(9, 25))), vec<fbase, 2>>();
    CHECK(kfr::sqrt(9) == fbase(3.0));
    CHECK(kfr::sqrt(2) == fbase(1.4142135623730950488));
    CHECK(std::isnan(kfr::sqrt(-9)));
    CHECK_THAT(kfr::sqrt(make_vector(9)), DeepMatcher{ make_vector<fbase>(3.0) });
    CHECK(std::isnan(kfr::sqrt(make_vector(-9))[0]));
    test_matrix(named("type") = float_vector_types<vec>, named("value") = std::vector<int>{ 0, 2, 65536 },
                [](auto type, int value)
                {
                    using T = typename decltype(type)::type;
                    const T x(value);
                    CHECK_THAT(kfr::sqrt(x),
                               DeepMatcher{ apply([](auto x) -> decltype(x) { return std::sqrt(x); }, x) });
                });
}

} // namespace KFR_ARCH_NAME
