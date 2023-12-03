/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */
#include "../numeric_tests.hpp"

#include <kfr/math/log_exp.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
KFR_AUTO_TEST_1(gamma, narrow, 2200, 321)
KFR_AUTO_TEST_1(exp, narrow, 4, 2)
KFR_AUTO_TEST_1(exp2, narrow, 5, 2)
KFR_AUTO_TEST_1(exp10, narrow, 40, 10)
KFR_AUTO_TEST_1(log, narrow, 2, 1)
KFR_AUTO_TEST_1(log2, narrow, 2, 1)
KFR_AUTO_TEST_1(log10, narrow, 3, 1)
KFR_AUTO_TEST_1(cbrt, narrow, 5, 1)

TEST(cbrt)
{
    CHECK(kfr::cbrt(27.f) == 3.f);
    CHECK(kfr::cbrt(-27.f) == -3.f);

    CHECK(kfr::root(32.f, 5.f) == 2.f);
    CHECK(kfr::root(-32.f, 5.f) == -2.f);
    CHECK(std::isnan(kfr::root(-32.f, 5.001f)));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
