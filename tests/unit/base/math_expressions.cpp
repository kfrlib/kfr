/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/math_expressions.hpp>
#include <kfr/base/univector.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(complex_basic_expressions)
{
    const univector<c32, 15> uv1 = zeros();
    CHECK(uv1[0] == c32{ 0, 0 });
    CHECK(uv1[1] == c32{ 0, 0 });
    CHECK(uv1[2] == c32{ 0, 0 });
    CHECK(uv1[14] == c32{ 0, 0 });
    const univector<c32, 15> uv2 = ones();
    CHECK(uv2[0] == c32{ 1, 0 });
    CHECK(uv2[1] == c32{ 1, 0 });
    CHECK(uv2[2] == c32{ 1, 0 });
    CHECK(uv2[14] == c32{ 1, 0 });
    const univector<c32, 15> uv3 = counter();
    CHECK(uv3[0] == c32{ 0, 0 });
    CHECK(uv3[1] == c32{ 1, 0 });
    CHECK(uv3[2] == c32{ 2, 0 });
    CHECK(uv3[14] == c32{ 14, 0 });
}

TEST(complex_function_expressions)
{
    const univector<c32, 4> uv1 = sqr(counter());
    CHECK(uv1[0] == c32{ 0, 0 });
    CHECK(uv1[1] == c32{ 1, 0 });
    CHECK(uv1[2] == c32{ 4, 0 });
    CHECK(uv1[3] == c32{ 9, 0 });

    const univector<c32, 4> uv2 = uv1 * 2.f;
    CHECK(uv2[0] == c32{ 0, 0 });
    CHECK(uv2[1] == c32{ 2, 0 });
    CHECK(uv2[2] == c32{ 8, 0 });
    CHECK(uv2[3] == c32{ 18, 0 });

    const univector<f32, 4> uv3 = real(uv2);
    CHECK(uv3[0] == 0.f);
    CHECK(uv3[1] == 2.f);
    CHECK(uv3[2] == 8.f);
    CHECK(uv3[3] == 18.f);
    testo::assert_is_same<c32, expression_value_type<decltype(uv2)>>();
    testo::assert_is_same<f32, expression_value_type<decltype(uv3)>>();
    testo::assert_is_same<f32, expression_value_type<decltype(real(uv2))>>();
}
} // namespace CMT_ARCH_NAME
