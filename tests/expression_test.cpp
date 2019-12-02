/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/cometa/function.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(pack)
{
    static_assert(is_same<vec<f32x2, 1>, invoke_result<fn::reverse, vec<f32x2, 1>>>);
    const univector<float, 21> v1 = 1 + counter();
    const univector<float, 21> v2 = v1 * 11;

    CHECK_EXPRESSION(pack(v1, v2), 21, [](float i) { return f32x2{ 1 + i, (1 + i) * 11 }; });

    CHECK_EXPRESSION(bind_expression(fn::reverse(), pack(v1, v2)), 21, [](float i) {
        return f32x2{ (1 + i) * 11, 1 + i };
    });
}

TEST(adjacent)
{
    CHECK_EXPRESSION(adjacent(fn::mul(), counter()), infinite_size,
                     [](size_t i) { return i > 0 ? i * (i - 1) : 0; });
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

TEST(rebind)
{
    auto c_minus_two  = counter() - 2;
    auto four_minus_c = rebind(c_minus_two, 4, counter());
    CHECK_EXPRESSION(c_minus_two, infinite_size, [](size_t i) { return i - 2; });
    CHECK_EXPRESSION(four_minus_c, infinite_size, [](size_t i) { return 4 - i; });
}

TEST(test_arg_access)
{
    univector<float> v1(10);
    v1                       = counter();
    auto e1                  = std::move(v1) + 10;
    std::get<0>(e1.args)[0]  = 100;
    std::get<1>(e1.args).val = 1;

    CHECK_EXPRESSION(e1, 10, [](size_t i) { return (i == 0 ? 100 : i) + 1; });
}

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
    auto expr = 100 * placeholder<float>();
    CHECK_EXPRESSION(expr, infinite_size, [](size_t) { return 0.f; });
    substitute(expr, to_pointer(counter<float>()));
    CHECK_EXPRESSION(expr, infinite_size, [](size_t i) { return 100.f * i; });
}

TEST(placeholders_pointer)
{
    expression_pointer<float> expr = to_pointer(10 * placeholder<float>());
    CHECK_EXPRESSION(expr, infinite_size, [](size_t) { return 0.f; });
    substitute(expr, to_pointer(counter<float>()));
    CHECK_EXPRESSION(expr, infinite_size, [](size_t i) { return 10.f * i; });
}

TEST(univector_assignment)
{
    univector<int> x = truncate(counter(), 10);
    CHECK(x.size() == 10u);

    univector<int> y;
    y = truncate(counter(), 10);
    CHECK(y.size() == 10u);
}

TEST(size_calc)
{
    auto a = counter();
    CHECK(a.size() == infinite_size);
    auto b = slice(counter(), 100);
    CHECK(b.size() == infinite_size);
    auto c = slice(counter(), 100, 1000);
    CHECK(c.size() == 1000u);
    auto d = slice(c, 100);
    CHECK(d.size() == 900u);
}

TEST(reverse)
{
    CHECK_EXPRESSION(reverse(truncate(counter(), 21)), 21, [](size_t i) { return 20 - i; });
}

TEST(mix)
{
    CHECK_EXPRESSION(mix(sequence(0, 0.5f, 1, 0.5f), counter(), counter() * 10), infinite_size, [](size_t i) {
        return mix(std::array<float, 4>{ 0, 0.5f, 1, 0.5f }[i % 4], i, i * 10);
    });
}

TEST(expression_mask)
{
    univector<float> x(100);
    univector<float> y(100);
    x = select(x > y, 0.5f, 0.1f) * (y - x) + x;
}

constexpr inline size_t fast_range_sum(size_t stop) { return stop * (stop + 1) / 2; }

TEST(partition)
{
    {
        univector<double, 385> output = zeros();
        auto result                   = partition(output, counter(), 5, 1);
        CHECK(result.count == 5u);
        CHECK(result.chunk_size == 80u);

        result(0);
        CHECK(sum(output) >= fast_range_sum(80 - 1));
        result(1);
        CHECK(sum(output) >= fast_range_sum(160 - 1));
        result(2);
        CHECK(sum(output) >= fast_range_sum(240 - 1));
        result(3);
        CHECK(sum(output) >= fast_range_sum(320 - 1));
        result(4);
        CHECK(sum(output) == fast_range_sum(385 - 1));
    }

    {
        univector<double, 385> output = zeros();
        auto result                   = partition(output, counter(), 5, 160);
        CHECK(result.count == 3u);
        CHECK(result.chunk_size == 160u);

        result(0);
        CHECK(sum(output) >= fast_range_sum(160 - 1));
        result(1);
        CHECK(sum(output) >= fast_range_sum(320 - 1));
        result(2);
        CHECK(sum(output) == fast_range_sum(385 - 1));
    }
}
} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version());

    return testo::run_all("", true);
}
#endif
