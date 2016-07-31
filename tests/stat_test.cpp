/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/io/tostring.hpp>
#include <kfr/version.hpp>

#include <kfr/math.hpp>

#include <kfr/base/reduce.hpp>

#include <tuple>

#include "testo/testo.hpp"

using namespace kfr;

TEST(test_stat)
{
    {
        univector<float, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(rms(a) == 3.316624790355399849115f);
        CHECK(product(a) == 120);
    }
    {
        univector<double, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(rms(a) == 3.316624790355399849115);
        CHECK(product(a) == 120);
    }
    {
        univector<int, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == 15);
        CHECK(mean(a) == 3);
        CHECK(minof(a) == 1);
        CHECK(maxof(a) == 5);
        CHECK(sumsqr(a) == 55);
        CHECK(product(a) == 120);
    }
    {
        univector<complex<float>, 5> a({ 1, 2, 3, 4, 5 });
        CHECK(sum(a) == c32{ 15 });
        CHECK(mean(a) == c32{ 3 });
        CHECK(sumsqr(a) == c32{ 55 });
        CHECK(product(a) == c32{ 120 });
    }
}

int main(int argc, char** argv)
{
    println(library_version());

    return testo::run_all("", true);
}
