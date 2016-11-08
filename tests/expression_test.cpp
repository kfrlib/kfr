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

TEST(pack)
{
    const univector<float, 21> v1 = 1 + counter();
    const univector<float, 21> v2 = v1 * 11;

    CHECK_EXPRESSION(pack(v1, v2), 21, [](size_t i) { return f32x2{ 1 + i, (1 + i) * 11 }; });

    CHECK_EXPRESSION(bind_expression(fn::reverse(), pack(v1, v2)), 21, [](size_t i) {
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
    static_assert(is_infinite<decltype(padded(counter()))>::value, "");
    static_assert(is_infinite<decltype(padded(truncate(counter(), 100)))>::value, "");

    CHECK_EXPRESSION(padded(truncate(counter(), 6), -1), infinite_size,
                     [](size_t i) { return i >= 6 ? -1 : i; });
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

TEST(test_arg_replace)
{
    univector<float, 10> v1 = counter();
    univector<float, 10> v2 = -counter();
    auto e1              = to_pointer(v1) * 10;
    std::get<0>(e1.args) = to_pointer(v2);

    CHECK_EXPRESSION(e1, 10, [](size_t i) { return i * -10.0; });
}

TEST(size_calc)
{
    auto a = counter();
    CHECK(a.size() == infinite_size);
    auto b = slice(counter(), 100);
    CHECK(b.size() == infinite_size);
    auto c = slice(counter(), 100, 1000);
    CHECK(c.size() == 1000);
    auto d = slice(c, 100);
    CHECK(d.size() == 900);
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

constexpr inline size_t fast_range_sum(size_t stop) { return stop * (stop + 1) / 2; }

TEST(partition)
{
    {
        univector<double, 400> output = zeros();
        auto result = partition(output, counter(), 5, 1);
        CHECK(result.count == 5);
        CHECK(result.chunk_size == 80);

        result(0);
        CHECK(sum(output) >= fast_range_sum(80 - 1));
        result(1);
        CHECK(sum(output) >= fast_range_sum(160 - 1));
        result(2);
        CHECK(sum(output) >= fast_range_sum(240 - 1));
        result(3);
        CHECK(sum(output) >= fast_range_sum(320 - 1));
        result(4);
        CHECK(sum(output) == fast_range_sum(400 - 1));
    }

    {
        univector<double, 400> output = zeros();
        auto result = partition(output, counter(), 5, 160);
        CHECK(result.count == 3);
        CHECK(result.chunk_size == 160);

        result(0);
        CHECK(sum(output) >= fast_range_sum(160 - 1));
        result(1);
        CHECK(sum(output) >= fast_range_sum(320 - 1));
        result(2);
        CHECK(sum(output) == fast_range_sum(400 - 1));
    }
}

int main()
{
    println(library_version());

    return testo::run_all("", true);
}
