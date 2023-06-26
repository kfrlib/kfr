/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

cpu_t fir_sse2(univector<double, 0> data, univector<double, 4>& taps);
cpu_t fir_avx(univector<double, 0> data, univector<double, 4>& taps);

TEST(test_fir_sse2)
{
    univector<double, 8> data = counter();
    univector<double, 4> taps({ 0.5, 1.0, 1.0, 0.5 });
    cpu_t c = fir_sse2(data, taps);
    CHECK(c == cpu_t::sse2);
    CHECK(data[0] == 0);
    CHECK(data[1] == 0.5);
    CHECK(data[2] == 2);
    CHECK(data[3] == 4.5);
    CHECK(data[4] == 7.5);
    CHECK(data[5] == 10.5);
    CHECK(data[6] == 13.5);
    CHECK(data[7] == 16.5);
}

TEST(test_fir_avx)
{
    if (get_cpu() >= cpu_t::avx1)
    {
        univector<double, 8> data = counter();
        univector<double, 4> taps({ 0.5, 1.0, 1.0, 0.5 });
        cpu_t c = fir_avx(data, taps);
        CHECK(c == cpu_t::avx);
        CHECK(data[0] == 0);
        CHECK(data[1] == 0.5);
        CHECK(data[2] == 2);
        CHECK(data[3] == 4.5);
        CHECK(data[4] == 7.5);
        CHECK(data[5] == 10.5);
        CHECK(data[6] == 13.5);
        CHECK(data[7] == 16.5);
    }
    else
    {
        println("No AVX");
    }
}

int main() { return testo::run_all("", true); }
