/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/dsp.hpp>
#include <kfr/io.hpp>
#include <kfr/testo/testo.hpp>

using namespace kfr;

TEST(resampler_test)
{
    const int in_sr  = 44100;
    const int out_sr = 48000;
    const int freq   = 100;
    auto resampler   = sample_rate_converter<fbase>(resample_quality::draft, out_sr, in_sr);
    double delay     = resampler.get_fractional_delay();
    univector<fbase> out(out_sr / 10);
    univector<fbase> in  = truncate(sin(c_pi<fbase> * phasor<fbase>(freq, in_sr, 0)), in_sr / 10);
    univector<fbase> ref = truncate(
        sin(c_pi<fbase> * phasor<fbase>(freq, out_sr, -delay * (static_cast<double>(freq) / out_sr))),
        out_sr / 10);
    resampler.process(out, in);

    ASSERT(rms(slice(out - ref, ceil(delay * 2))) < 0.0001);
}

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version());

    return testo::run_all("", true);
}
#endif
