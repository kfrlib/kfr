/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/simd_expressions.hpp>
#include <kfr/base/univector.hpp>
#include <kfr/io/tostring.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

TEST(linspace)
{
    testo::eplison_scope<> eps(10);
    CHECK_EXPRESSION(linspace(0.0, 1.0, 5, true, ctrue), { 0.0, 0.25, 0.50, 0.75, 1.0 });
    CHECK_EXPRESSION(linspace(0.0, 1.0, 4, false, ctrue), { 0.0, 0.25, 0.50, 0.75 });
    CHECK(get_shape(linspace(0.0, 1.0, 5, true, cfalse)) == shape{ infinite_size });
    CHECK_EXPRESSION(linspace(0.0, 1.0, 4, false, ctrue), { 0.0, 0.25, 0.50, 0.75 });
    CHECK_EXPRESSION(symmlinspace(3.0, 4, ctrue), { -3.0, -1.00, 1.00, 3.00 });

    CHECK_EXPRESSION(linspace(1, 21, 4, false, ctrue), { 1, 6, 11, 16 });
    CHECK_EXPRESSION(linspace(1, 21, 4, true, ctrue), { 1, 7.66666667f, 14.3333333f, 21 });
}

TEST(counter_shape)
{
    CHECK(get_shape(1) == shape{});
    CHECK(get_shape(counter()) == shape{ infinite_size });
    CHECK(get_shape(counter() + 1) == shape{ infinite_size });
    CHECK(get_shape(counter(0, 1, 1)) == shape{ infinite_size, infinite_size });
}

TEST(pack)
{
    static_assert(std::is_same_v<vec<f32x2, 1>, std::invoke_result_t<fn::reverse, vec<f32x2, 1>>>);
    const univector<float, 21> v1 = 1 + counter();
    const univector<float, 21> v2 = v1 * 11;

    CHECK_EXPRESSION(pack(v1, v2), 21, [](float i) { return f32x2{ 1 + i, (1 + i) * 11 }; });

    CHECK_EXPRESSION(bind_expression(fn::reverse(), pack(v1, v2)), 21,
                     [](float i) {
                         return f32x2{ (1 + i) * 11, 1 + i };
                     });
}

TEST(adjacent)
{
    CHECK_EXPRESSION(adjacent(fn::mul(), counter()), infinite_size,
                     [](size_t i) { return i > 0 ? i * (i - 1) : 0; });
}

TEST(dimensions)
{
    static_assert(expression_dims<decltype(scalar(0))> == 0);
    static_assert(expression_dims<decltype(dimensions<1>(scalar(0)))> == 1);

    static_assert(get_shape<decltype(scalar(0))>() == shape{});
    static_assert(get_shape<decltype(dimensions<1>(scalar(0)))>() == shape{ infinite_size });
    static_assert(get_shape<decltype(dimensions<2>(dimensions<1>(scalar(0))))>() ==
                  shape{ infinite_size, infinite_size });
    CHECK_EXPRESSION(truncate(dimensions<1>(scalar(1)), 10), { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
}

TEST(padded)
{
    static_assert(is_infinite<decltype(padded(counter()))>, "");
    static_assert(is_infinite<decltype(padded(truncate(counter(), 100)))>, "");

    CHECK_EXPRESSION(padded(truncate(counter(), 6), -1), infinite_size,
                     [](size_t i) { return i >= 6 ? -1 : i; });

    CHECK_EXPRESSION(padded(truncate(counter(), 0), -1), infinite_size, [](size_t i) { return -1; });

    CHECK_EXPRESSION(padded(truncate(counter(), 501), -1), infinite_size,
                     [](size_t i) { return i >= 501 ? -1 : i; });
}

TEST(concatenate)
{
    CHECK_EXPRESSION(concatenate(truncate(counter(5, 0), 5), truncate(counter(10, 0), 5)),
                     { 5, 5, 5, 5, 5, 10, 10, 10, 10, 10 });
}

TEST(rebind)
{
    auto c_minus_two  = counter() - 2;
    auto four_minus_c = rebind(c_minus_two, 4, counter());
    CHECK_EXPRESSION(counter(), infinite_size, [](size_t i) { return i; });
    CHECK_EXPRESSION(c_minus_two, infinite_size, [](size_t i) { return i - 2; });
    CHECK_EXPRESSION(four_minus_c, infinite_size, [](size_t i) { return 4 - i; });
}

TEST(test_arg_access)
{
    univector<float> v1(10);
    v1                      = counter();
    auto e1                 = std::move(v1) + 10;
    std::get<0>(e1.args)[0] = 100;
    std::get<1>(e1.args)    = 1;

    CHECK_EXPRESSION(e1, 10, [](size_t i) { return (i == 0 ? 100 : i) + 1; });
}

TEST(size_calc)
{
    auto a = counter();
    CHECK(get_shape(a) == shape{ infinite_size });
    auto b = slice(counter(), 100);
    CHECK(get_shape(b) == shape{ infinite_size });
    auto c = slice(counter(), 100, 1000);
    CHECK(get_shape(c) == shape{ 1000 });
    auto d = slice(c, 100);
    CHECK(get_shape(d) == shape{ 900 });
}

TEST(reverse_expression)
{
    CHECK_EXPRESSION(reverse(truncate(counter(), 21)), 21, [](size_t i) { return 20 - i; });
}

TEST(sequence)
{
    CHECK_EXPRESSION(sequence(0, 0.5f, 1, 0.5f), infinite_size,
                     [](size_t i) {
                         return std::array<float, 4>{ 0, 0.5f, 1, 0.5f }[i % 4];
                     });
}

TEST(assign_expression)
{
    univector<float> f = truncate(counter(0, 1), 10);
    f *= 10;
    CHECK_EXPRESSION(f, { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 });

    univector<float> a = truncate(counter(0, 1), 10);
    univector<float> b = truncate(counter(100, 1), 10);
    pack(a, b) *= broadcast<2>(10.f);
    CHECK_EXPRESSION(a, { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 });
    CHECK_EXPRESSION(b, { 1000, 1010, 1020, 1030, 1040, 1050, 1060, 1070, 1080, 1090 });

    static_assert(std::is_same_v<std::common_type_t<f32x2x2, f32x2x2>, f32x2x2>);
    static_assert(
        std::is_same_v<std::common_type_t<vec<vec<double, 2>, 1>, vec<double, 2>>, vec<vec<double, 2>, 1>>);
}

TEST(trace) { render(trace(counter()), 44); }

TEST(get_element) { CHECK(get_element(counter(0, 1, 10, 100), { 1, 2, 3 }) == 321); }

} // namespace CMT_ARCH_NAME
} // namespace kfr
