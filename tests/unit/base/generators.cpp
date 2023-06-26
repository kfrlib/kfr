/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/generators.hpp>
#include <kfr/base/math_expressions.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(test_gen_expj)
{
    univector<cbase> v = truncate(gen_expj(0.f, constants<float>::pi_s(2) * 0.1f), 1000);
    CHECK(rms(cabs(
              v.slice(990) -
              univector<cbase>({ cbase(1., +0.00000000e+00), cbase(0.80901699, +5.87785252e-01),
                                 cbase(0.30901699, +9.51056516e-01), cbase(-0.30901699, +9.51056516e-01),
                                 cbase(-0.80901699, +5.87785252e-01), cbase(-1., +1.22464680e-16),
                                 cbase(-0.80901699, -5.87785252e-01), cbase(-0.30901699, -9.51056516e-01),
                                 cbase(0.30901699, -9.51056516e-01), cbase(0.80901699, -5.87785252e-01) }))) <
          0.00006); // error here depends on vector width
    // In most cases error is much lower (less than 0.00001)
}

} // namespace CMT_ARCH_NAME
