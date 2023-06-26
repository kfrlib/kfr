/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp/biquad.hpp>
#include <kfr/dsp/biquad_design.hpp>
#include <kfr/dsp/special.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main()
{
    println(library_version());

    const std::string options = "phaseresp=True";

    univector<fbase, 128> output;
    {
        biquad_params<fbase> bq[] = { biquad_notch(0.1, 0.5), biquad_notch(0.2, 0.5), biquad_notch(0.3, 0.5),
                                      biquad_notch(0.4, 0.5) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_notch", output, options + ", title='Four Biquad Notch filters'");

    {
        biquad_params<fbase> bq[] = { biquad_lowpass(0.2, 0.9) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_lowpass", output, options + ", title='Biquad Low pass filter (0.2, 0.9)'");

    {
        biquad_params<fbase> bq[] = { biquad_highpass(0.3, 0.1) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_highpass", output, options + ", title='Biquad High pass filter (0.3, 0.1)'");

    {
        biquad_params<fbase> bq[] = { biquad_peak(0.3, 0.5, +9.0) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_peak", output, options + ", title='Biquad Peak filter (0.2, 0.5, +9)'");

    {
        biquad_params<fbase> bq[] = { biquad_peak(0.3, 3.0, -2.0) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_peak2", output, options + ", title='Biquad Peak filter (0.3, 3, -2)'");

    {
        biquad_params<fbase> bq[] = { biquad_lowshelf(0.3, -1.0) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_lowshelf", output, options + ", title='Biquad low shelf filter (0.3, -1)'");

    {
        biquad_params<fbase> bq[] = { biquad_highshelf(0.3, +9.0) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_highshelf", output, options + ", title='Biquad high shelf filter (0.3, +9)'");

    {
        biquad_params<fbase> bq[] = { biquad_bandpass(0.25, 0.2) };
        output                    = biquad(bq, unitimpulse());
    }
    plot_save("biquad_bandpass", output, options + ", title='Biquad band pass (0.25, 0.2)'");

    {
        biquad_params<fbase> bq[] = { biquad_bandpass(0.25, 0.2) };
        std::vector<fbase> data(output.size(), 0.f);
        data[0] = 1.f;
        output  = biquad(bq, make_univector(data));
    }
    plot_save("biquad_bandpass_stdvector", output, options + ", title='Biquad band pass (0.25, 0.2)'");

    {
        biquad_params<fbase> bq[] = { biquad_bandpass(0.25, 0.2) };
        fbase data[output.size()] = { 0 }; // .size() is constexpr
        data[0]                   = 1.f;
        output                    = biquad(bq, make_univector(data));
    }
    plot_save("biquad_bandpass_carray", output, options + ", title='Biquad band pass (0.25, 0.2)'");

    {
        // filter initialization
        biquad_params<fbase> bq[]       = { biquad_lowpass(0.2, 0.9) };
        expression_filter<fbase> filter = to_filter(biquad(bq, placeholder<fbase>()));

        // prepare data
        output = unitimpulse();

        // apply filter
        filter.apply(output);
    }
    plot_save("biquad_custom_filter_lowpass", output,
              options + ", title='Biquad Low pass filter (0.2, 0.9) (using expression_filter)'");

    {
        // filter initialization
        biquad_params<fbase> bq[] = { biquad_lowpass(0.2, 0.9) };
        biquad_filter<fbase> filter(bq);

        // prepare data
        output = unitimpulse();

        // apply filter
        filter.apply(output);
    }
    plot_save("biquad_filter_lowpass", output,
              options + ", title='Biquad Low pass filter (0.2, 0.9) (using biquad_filter)'");

    println("SVG plots have been saved to svg directory");

    return 0;
}
