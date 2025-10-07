/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2025 Dan Casarin
 * See LICENSE.txt for details
 */

#include "../numeric_tests.hpp"

#include <kfr/math/atan.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
KFR_AUTO_TEST_1(atan, narrow, 2, 1)
KFR_AUTO_TEST_2(atan2, narrow, 2, 1)
} // namespace KFR_ARCH_NAME
} // namespace kfr
