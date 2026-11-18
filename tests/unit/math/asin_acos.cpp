/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2026 Dan Casarin
 * See LICENSE.txt for details
 */
#include <kfr/test/test.hpp>

#include <kfr/math/asin_acos.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
TEST_CASE("acos_near_one")
{
    const vec<f32, 2> x(1.f - 0x1p-14f, 1.f - 0x1p-15f);
    const auto result = kfr::acos(x);
    for (size_t i = 0; i < x.size(); ++i)
        CHECK(result[i] == Catch::Approx(std::acos(x[i])).epsilon(1e-6));
}
} // namespace KFR_ARCH_NAME

} // namespace kfr
