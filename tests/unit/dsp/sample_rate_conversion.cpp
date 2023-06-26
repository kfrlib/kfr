/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base/math_expressions.hpp>
#include <kfr/dsp/oscillators.hpp>
#include <kfr/dsp/sample_rate_conversion.hpp>
#include <kfr/math/sin_cos.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

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

    CHECK(rms(slice(out - ref, static_cast<size_t>(ceil(delay * 2)))) < 0.005f);
}
TEST(resampler_test_complex)
{
    using type       = complex<fbase>;
    const int in_sr  = 44100;
    const int out_sr = 48000;
    const int freq   = 100;
    auto resampler   = sample_rate_converter<type>(resample_quality::draft, out_sr, in_sr);
    double delay     = resampler.get_fractional_delay();
    univector<type> out(out_sr / 10);
    univector<type> in  = truncate(sin(c_pi<fbase> * phasor<fbase>(freq, in_sr, 0)), in_sr / 10);
    univector<type> ref = truncate(
        sin(c_pi<fbase> * phasor<fbase>(freq, out_sr, -delay * (static_cast<double>(freq) / out_sr))),
        out_sr / 10);
    resampler.process(out, in);

    CHECK(rms(cabs(slice(out - ref, static_cast<size_t>(ceil(delay * 2))))) < 0.005f);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
