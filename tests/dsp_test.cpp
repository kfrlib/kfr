/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/io/tostring.hpp>

#include "testo/testo.hpp"
#include <kfr/dsp.hpp>
#include <kfr/math.hpp>

using namespace kfr;

TEST(delay)
{
    const univector<float, 33> v1 = counter() + 100;
    const univector<float, 33> v2 = delay(v1);
    CHECK(v2[0] == 0);
    CHECK(v2[1] == 100);
    CHECK(v2[2] == 101);
    CHECK(v2[19] == 118);

    const univector<float, 33> v3 = delay(v1, csize<3>);
    CHECK(v3[0] == 0);
    CHECK(v3[1] == 0);
    CHECK(v3[2] == 0);
    CHECK(v3[3] == 100);
    CHECK(v3[4] == 101);
    CHECK(v3[19] == 116);
}

TEST(fracdelay)
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

TEST(mixdown)
{
    univector<double, 20> ch1 = counter();
    univector<double, 20> ch2 = counter() * 2 + 100;
    univector<double, 20> mix = mixdown(ch1, ch2);
    CHECK(mix[0] == 100);
    CHECK(mix[1] == 103);
    CHECK(mix[19] == 157);
}

TEST(mixdown_stereo)
{
    const univector<double, 21> left  = counter();
    const univector<double, 21> right = counter() * 2 + 100;
    univector<double, 21> mid;
    univector<double, 21> side;
    pack(mid, side) = mixdown_stereo(left, right, matrix_sum_diff());

    CHECK(mid[0] == 100);
    CHECK(side[0] == -100);
    CHECK(mid[1] == 103);
    CHECK(side[1] == -101);
    CHECK(mid[20] == 160);
    CHECK(side[20] == -120);
}

int main(int argc, char** argv) { return testo::run_all("", true); }
