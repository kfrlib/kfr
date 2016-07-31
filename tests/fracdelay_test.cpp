/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/io/tostring.hpp>
#include <kfr/version.hpp>

#include <kfr/base/reduce.hpp>

#include <tuple>

#include "testo/testo.hpp"
#include <kfr/dsp/fracdelay.hpp>

using namespace kfr;

TEST(test_fracdelay)
{
    univector<double, 5> a({ 1, 2, 3, 4, 5 });
    univector<double, 5> b = fracdelay(a, 0.5);
    CHECK(rms(b - univector<double>({ 0.5, 1.5, 2.5, 3.5, 4.5 })) < c_epsilon<double> * 5);

    b = fracdelay(a, 0.1);
    CHECK(rms(b - univector<double>({ 0.9, 1.9, 2.9, 3.9, 4.9 })) < c_epsilon<double> * 5);

    b = fracdelay(a, 0.0);
    CHECK(rms(b - univector<double>({ 1, 2, 3, 4, 5 })) < c_epsilon<double> * 5);

    b = fracdelay(a, 1.0);
    CHECK(rms(b - univector<double>({ 0, 1, 2, 3, 4 })) < c_epsilon<double> * 5);
}

int main(int argc, char** argv)
{
    println(library_version());

    return testo::run_all("", true);
}
