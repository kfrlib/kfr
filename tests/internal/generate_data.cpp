/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */
#define _USE_MATH_DEFINES

#include "mpfr/mpfrplus.hpp"
#include <kfr/cometa.hpp>
#include <kfr/cometa/ctti.hpp>
#include <kfr/cometa/function.hpp>
#include <kfr/io/file.hpp>
#include <random>

constexpr size_t points      = 10000;
constexpr size_t points_2arg = 100;

using namespace kfr;

using testo::test_data_entry;

template <typename T>
struct range_sampler
{
    double min;
    double max;
    T operator()(size_t i, size_t num) { return static_cast<T>(min + (max - min) * i / (points - 1)); }
};

template <typename T>
struct fuzz_sampler
{
    std::mt19937_64 rnd{ 12345 };
    T operator()(size_t i, size_t num) { return bitcast_anything<T>(static_cast<utype<T>>(rnd())); }
};

template <typename T, typename Sampler>
void generate_table(const std::shared_ptr<file_writer<test_data_entry<T, 1>>>& writer,
                    cometa::function<mpfr::number(const mpfr::number&)> func, Sampler&& sampler)
{
    for (size_t i = 0; i < points; i++)
    {
        test_data_entry<T, 1> entry;
        entry.arguments[0] = sampler(i, points);
        entry.result       = static_cast<T>(func(entry.arguments[0]));
        writer->write(entry);
    }
}

template <typename T, typename Sampler>
void generate_table(const std::shared_ptr<file_writer<test_data_entry<T, 2>>>& writer,
                    cometa::function<mpfr::number(const mpfr::number&, const mpfr::number&)> func,
                    Sampler&& sampler)
{
    for (size_t i = 0; i < points_2arg; i++)
    {
        for (size_t j = 0; j < points_2arg; j++)
        {
            test_data_entry<T, 2> entry;
            entry.arguments[0] = sampler(i, points_2arg);
            entry.arguments[1] = sampler(j, points_2arg);
            entry.result       = static_cast<T>(func(entry.arguments[0], entry.arguments[1]));
            writer->write(entry);
        }
    }
}

template <int args, typename Func>
void generate_test(cint_t<args>, const char* name, const Func& func, double min, double max)
{
    generate_table(open_file_for_writing<test_data_entry<float, args>>(as_string(name, "_float_narrow")),
                   func, range_sampler<float>{ min, max });
    generate_table(open_file_for_writing<test_data_entry<double, args>>(as_string(name, "_double_narrow")),
                   func, range_sampler<double>{ min, max });

    generate_table(open_file_for_writing<test_data_entry<float, args>>(as_string(name, "_float_fuzz")), func,
                   fuzz_sampler<float>{});
    generate_table(open_file_for_writing<test_data_entry<double, args>>(as_string(name, "_double_fuzz")),
                   func, fuzz_sampler<double>{});
}

int main()
{
    using num = mpfr::number;
    mpfr::scoped_precision prec(512);

    generate_test(
        cint<1>, "sin", [](const num& x) { return mpfr::sin(x); }, 0, M_PI * 2);
    generate_test(
        cint<1>, "cos", [](const num& x) { return mpfr::cos(x); }, 0, M_PI * 2);
    generate_test(
        cint<1>, "tan", [](const num& x) { return mpfr::tan(x); }, 0, M_PI);

    generate_test(
        cint<1>, "asin", [](const num& x) { return mpfr::asin(x); }, 0, 1);
    generate_test(
        cint<1>, "acos", [](const num& x) { return mpfr::acos(x); }, 0, 1);
    generate_test(
        cint<1>, "atan", [](const num& x) { return mpfr::atan(x); }, 0, 1);
    generate_test(
        cint<2>, "atan2", [](const num& x, const num& y) { return mpfr::atan2(x, y); }, 0, 10);

    generate_test(
        cint<1>, "sinh", [](const num& x) { return mpfr::sinh(x); }, 0, 10 * 2);
    generate_test(
        cint<1>, "cosh", [](const num& x) { return mpfr::cosh(x); }, 0, 10 * 2);
    generate_test(
        cint<1>, "tanh", [](const num& x) { return mpfr::tanh(x); }, 0, 10 * 2);
    generate_test(
        cint<1>, "coth", [](const num& x) { return mpfr::coth(x); }, 0, 10 * 2);

    generate_test(
        cint<1>, "gamma", [](const num& x) { return mpfr::gamma(x); }, 0, 10);

    generate_test(
        cint<1>, "log", [](const num& x) { return mpfr::log(x); }, 0, 100);
    generate_test(
        cint<1>, "log2", [](const num& x) { return mpfr::log2(x); }, 0, 100);
    generate_test(
        cint<1>, "log10", [](const num& x) { return mpfr::log10(x); }, 0, 100);

    generate_test(
        cint<1>, "exp", [](const num& x) { return mpfr::exp(x); }, -10, 10);
    generate_test(
        cint<1>, "exp2", [](const num& x) { return mpfr::exp2(x); }, -10, 10);
    generate_test(
        cint<1>, "exp10", [](const num& x) { return mpfr::exp10(x); }, -10, 10);

    generate_test(
        cint<1>, "cbrt", [](const num& x) { return mpfr::cbrt(x); }, 0, 1000);
}
