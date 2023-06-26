/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */
#include "../../numeric_tests.hpp"

#include <kfr/math/sin_cos.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
KFR_AUTO_TEST_1(sin, narrow, 7, 1)
KFR_AUTO_TEST_1(cos, narrow, 2, 1)
} // namespace CMT_ARCH_NAME
} // namespace kfr
