/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016-2022 Fractalium Ltd
 * See LICENSE.txt for details
 */

#include <kfr/base/pointer.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/base/generators.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
TEST(to_pointer)
{
    auto e1 = to_pointer(counter<float>());

    CHECK_EXPRESSION(e1, infinite_size, [](size_t i) { return static_cast<float>(i); });

    auto e2 = to_pointer(gen_linear(0.f, 1.f));

    CHECK_EXPRESSION(e2, infinite_size, [](size_t i) { return static_cast<float>(i); });
}

TEST(test_arg_replace)
{
    univector<float, 10> v1 = counter();
    univector<float, 10> v2 = -counter();
    auto e1                 = to_pointer(v1) * 10;
    std::get<0>(e1.args)    = to_pointer(v2);

    CHECK_EXPRESSION(e1, 10, [](size_t i) { return i * -10.0; });
}

TEST(placeholders)
{
    auto expr1 = placeholder<float>();
    CHECK_EXPRESSION(expr1, infinite_size, [](size_t) { return 0.f; });
    auto expr2 = 100 * placeholder<float>();
    CHECK_EXPRESSION(expr2, infinite_size, [](size_t) { return 0.f; });
    substitute(expr2, to_pointer(counter<float>()));
    CHECK_EXPRESSION(expr2, infinite_size, [](size_t i) { return 100.f * i; });
}

TEST(placeholders_pointer)
{
    expression_pointer<float> expr = to_pointer(10 * placeholder<float>());
    CHECK_EXPRESSION(expr, infinite_size, [](size_t) { return 0.f; });
    substitute(expr, to_pointer(counter<float>()));
    CHECK_EXPRESSION(expr, infinite_size, [](size_t i) { return 10.f * i; });
}
    
} // namespace CMT_ARCH_NAME
} // namespace kfr
