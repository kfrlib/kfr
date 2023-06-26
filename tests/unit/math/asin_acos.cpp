/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */
#include "../../numeric_tests.hpp"

#include <kfr/math/asin_acos.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
KFR_AUTO_TEST_1(asin, narrow, 6, 1)
KFR_AUTO_TEST_1(acos, narrow, 800, 1)
} // namespace CMT_ARCH_NAME

} // namespace kfr
