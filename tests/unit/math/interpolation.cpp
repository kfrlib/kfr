/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */

#include <kfr/math/interpolation.hpp>
#include <kfr/test/test.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("cubic_interpolation_hits_inner_samples")
{
    CHECK(kfr::cubic(0.0, -3.0, 2.0, 5.0, 6.0) == 2.0);
    CHECK(kfr::cubic(1.0, -3.0, 2.0, 5.0, 6.0) == 5.0);
    CHECK(kfr::catmullrom(0.0, -3.0, 2.0, 5.0, 6.0) == 2.0);
    CHECK(kfr::catmullrom(1.0, -3.0, 2.0, 5.0, 6.0) == 5.0);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
