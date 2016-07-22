/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/io/tostring.hpp>
#include <kfr/version.hpp>

#include <kfr/expressions/reduce.hpp>

#include <tuple>

#include "testo/testo.hpp"

using namespace kfr;

TEST(test_stat)
{
    {
        univector<float, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(native::sum(a) == 15);
        CHECK(native::mean(a) == 3);
        CHECK(native::min(a) == 1);
        CHECK(native::max(a) == 5);
        CHECK(native::sumsqr(a) == 55);
        CHECK(native::rms(a) == 3.316624790355399849115f);
        CHECK(native::product(a) == 120);
    }
    {
        univector<double, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(native::sum(a) == 15);
        CHECK(native::mean(a) == 3);
        CHECK(native::min(a) == 1);
        CHECK(native::max(a) == 5);
        CHECK(native::sumsqr(a) == 55);
        CHECK(native::rms(a) == 3.316624790355399849115);
        CHECK(native::product(a) == 120);
    }
    {
        univector<int, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(native::sum(a) == 15);
        CHECK(native::mean(a) == 3);
        CHECK(native::min(a) == 1);
        CHECK(native::max(a) == 5);
        CHECK(native::sumsqr(a) == 55);
        CHECK(native::product(a) == 120);
    }
}

int main(int argc, char** argv)
{
    println(library_version());

    return testo::run_all("", true);
}
