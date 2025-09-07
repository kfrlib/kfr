/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#define KFR_ENABLE_EXPR_CMP

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

TEST_CASE("expression_mask")
{
    univector<float> x(100);
    univector<float> y(100);
    x = select(x > y, 0.5f, 0.1f) * (y - x) + x;
}

TEST_CASE("mix")
{
    CHECK_EXPRESSION(mix(sequence(0, 0.5f, 1, 0.5f), counter(), counter() * 10), infinite_size, [](size_t i)
                     { return mix(std::array<float, 4>{ 0, 0.5f, 1, 0.5f }[i % 4], i, i * 10); });
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
