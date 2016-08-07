/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/dsp.hpp>
#include <kfr/io/tostring.hpp>
#include <kfr/version.hpp>

using namespace kfr;

cpu_t fir_avx(univector<double, 0> data, univector<double, 4>& taps)
{
    println(library_version());
    data = short_fir(data, taps);
    return cpu_t::native;
}
