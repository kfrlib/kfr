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

    const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                                "log_freq=False, horizontal=False, normalized_freq=True";

    univector<fbase, 64> output;
    output = window_hann(output.size());
    plot_save("window_hann", output, options + ", title='Hann window'");

    output = window_hamming(output.size());
    plot_save("window_hamming", output, options + ", title='Hamming window'");

    output = window_blackman(output.size());
    plot_save("window_blackman", output, options + ", title='Blackman window'");

    output = window_blackman_harris(output.size());
    plot_save("window_blackman_harris", output, options + ", title='Blackman-Harris window'");

    output = window_gaussian(output.size());
    plot_save("window_gaussian", output, options + ", title='Gaussian window'");

    output = window_triangular(output.size());
    plot_save("window_triangular", output, options + ", title='Triangular window'");

    output = window_bartlett(output.size());
    plot_save("window_bartlett", output, options + ", title='Bartlett window'");

    output = window_cosine(output.size());
    plot_save("window_cosine", output, options + ", title='Cosine window'");

    output = window_cosine_np(output.size());
    plot_save("window_cosine_np", output, options + ", title='Cosine window (numpy compatible)'");

    output = window_bartlett_hann(output.size());
    plot_save("window_bartlett_hann", output, options + ", title='Bartlett-Hann window'");

    output = window_bohman(output.size());
    plot_save("window_bohman", output, options + ", title='Bohman window'");

    output = window_lanczos(output.size());
    plot_save("window_lanczos", output, options + ", title='Lanczos window'");

    output = window_flattop(output.size());
    plot_save("window_flattop", output, options + ", title='Flat top window'");

    output = window_kaiser(output.size(), 2.5);
    plot_save("window_kaiser", output, options + ", title='Kaiser window'");

    output = window_planck_taper(output.size(), 0.1);
    plot_save("window_planck_taper", output, options + ", title='Planck-taper window'");

    println("SVG plots have been saved to svg directory");

    return 0;
}
