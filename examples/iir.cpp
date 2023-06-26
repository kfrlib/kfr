/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main()
{
    println(library_version());

    constexpr size_t maxorder = 32;

    const std::string options = "phaseresp=True, log_freq=True, freq_dB_lim=(-160, 10), padwidth=8192";

    univector<fbase, 1024> output;
    {
        zpk<fbase> filt                       = iir_lowpass(bessel<fbase>(24), 1000, 48000);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("bessel_lowpass24", output, options + ", title='24th-order Bessel filter, lowpass 1khz'");

    {
        zpk<fbase> filt                       = iir_lowpass(bessel<fbase>(12), 1000, 48000);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("bessel_lowpass12", output, options + ", title='12th-order Bessel filter, lowpass 1khz'");

    {
        zpk<fbase> filt                       = iir_lowpass(bessel<fbase>(6), 1000, 48000);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("bessel_lowpass6", output, options + ", title='6th-order Bessel filter, lowpass 1khz'");

    {
        zpk<fbase> filt                       = iir_lowpass(butterworth<fbase>(24), 1000, 48000);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("butterworth_lowpass24", output,
              options + ", title='24th-order Butterworth filter, lowpass 1khz'");

    {
        zpk<fbase> filt                       = iir_lowpass(butterworth<fbase>(12), 1000, 48000);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("butterworth_lowpass12", output,
              options + ", title='12th-order Butterworth filter, lowpass 1khz'");

    {
        zpk<fbase> filt                       = iir_highpass(butterworth<fbase>(12), 1000, 48000);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("butterworth_highpass12", output,
              options + ", title='12th-order Butterworth filter, highpass 1khz'");

    {
        zpk<fbase> filt                       = iir_bandpass(butterworth<fbase>(12), 0.1, 0.2);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("butterworth_bandpass12", output,
              options + ", title='12th-order Butterworth filter, bandpass'");

    {
        zpk<fbase> filt                       = iir_bandstop(butterworth<fbase>(12), 0.1, 0.2);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("butterworth_bandstop12", output,
              options + ", title='12th-order Butterworth filter, bandstop'");

    {
        zpk<fbase> filt                       = iir_bandpass(butterworth<fbase>(4), 0.005, 0.9);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("butterworth_bandpass4", output, options + ", title='4th-order Butterworth filter, bandpass'");

    {
        zpk<fbase> filt                       = iir_lowpass(chebyshev1<fbase>(8, 2), 0.09);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("chebyshev1_lowpass8", output,
              options + ", title='8th-order Chebyshev type I filter, lowpass'");

    {
        zpk<fbase> filt                       = iir_lowpass(chebyshev2<fbase>(8, 80), 0.09);
        std::vector<biquad_params<fbase>> bqs = to_sos(filt);
        output                                = biquad<maxorder>(bqs, unitimpulse());
    }
    plot_save("chebyshev2_lowpass8", output,
              options + ", title='8th-order Chebyshev type II filter, lowpass'");

    println("SVG plots have been saved to svg directory");

    return 0;
}
