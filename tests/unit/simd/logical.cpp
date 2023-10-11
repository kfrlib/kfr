/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/simd/logical.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(logical_all)
{
    CHECK(all(mask<f32, 4>{ true, true, true, true }) == true);
    CHECK(all(mask<f32, 4>{ true, false, true, false }) == false);
    CHECK(all(mask<f32, 4>{ false, true, false, true }) == false);
    CHECK(all(mask<f32, 4>{ false, false, false, false }) == false);
}
TEST(logical_any)
{
    CHECK(any(mask<f32, 4>{ true, true, true, true }) == true);
    CHECK(any(mask<f32, 4>{ true, false, true, false }) == true);
    CHECK(any(mask<f32, 4>{ false, true, false, true }) == true);
    CHECK(any(mask<f32, 4>{ false, false, false, false }) == false);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
