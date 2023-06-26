/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include "../../numeric_tests.hpp"

#include <kfr/math/hyperbolic.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
KFR_AUTO_TEST_1(sinh, narrow, 114, 2.5)
KFR_AUTO_TEST_1(cosh, narrow, 7, 2.5)
KFR_AUTO_TEST_1(tanh, narrow, 45, 1)
KFR_AUTO_TEST_1(coth, narrow, 85, 1)
} // namespace CMT_ARCH_NAME

} // namespace kfr
