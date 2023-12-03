/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/dsp/mixdown.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(mixdown)
{
    CHECK_EXPRESSION(mixdown(counter(), counter() * 2 + 100), infinite_size,
                     [](size_t i) { return i + i * 2 + 100; });
}

TEST(mixdown_stereo)
{
    const univector<double, 21> left  = counter();
    const univector<double, 21> right = counter() * 2 + 100;
    univector<double, 21> mid;
    univector<double, 21> side;
    unpack(mid, side) = mixdown_stereo(left, right, matrix_sum_diff());

    CHECK_EXPRESSION(mid, 21, [](size_t i) { return i + i * 2.0 + 100.0; });
    CHECK_EXPRESSION(side, 21, [](size_t i) { return i - (i * 2.0 + 100.0); });
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
