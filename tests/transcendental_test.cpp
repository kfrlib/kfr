/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include "testo/testo.hpp"
#include <kfr/base.hpp>
#include <kfr/io.hpp>

#define MPFR_THREAD_LOCAL
#include "mpfr/mpfrplus.hpp"

using namespace kfr;

template <typename T>
double ulps(T test, const mpfr::number& ref)
{
    if (std::isnan(test) && ref.isnan())
        return 0;
    if (std::isinf(test) && ref.isinfinity())
        return (test < 0) == (ref < 0) ? 0 : NAN;
    return static_cast<double>(mpfr::abs(mpfr::number(test) - ref) /
                               mpfr::abs(mpfr::number(test) - std::nexttoward(test, HUGE_VALL)));
}

TEST(test_sin_cos)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(0.0, +c_pi<f64, 2>, 0.05),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::sin(x), mpfr::sin(x)) < 2.0);
                      CHECK(ulps(kfr::cos(x), mpfr::cos(x)) < 2.0);
                  });
}

TEST(test_log)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(0.0, 100.0, 0.5),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::log(x), mpfr::log(x)) < 2.0);
                  });
}

TEST(test_log2)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(0.0, 100.0, 0.5),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::log2(x), mpfr::log2(x)) < 3.0);
                  });
}

TEST(test_log10)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(0.0, 100.0, 0.5),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::log10(x), mpfr::log10(x)) < 3.0);
                  });
}

TEST(test_exp)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(-10, +10, 0.05),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::exp(x), mpfr::exp(x)) < 2.0);
                  });
}

TEST(test_exp2)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(-10, +10, 0.05),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::exp2(x), mpfr::exp2(x)) < 3.0);
                  });
}

TEST(test_exp10)
{
    testo::matrix(named("type") = ctypes<f32, f64>, named("value") = make_range(-10, +10, 0.05),
                  [](auto type, double value) {
                      using T = type_of<decltype(type)>;
                      const T x(value);
                      CHECK(ulps(kfr::exp10(x), mpfr::exp10(x)) < 3.0);
                  });
}

int main(int argc, char** argv)
{
    mpfr::scoped_precision p(128);
    return testo::run_all("");
}
