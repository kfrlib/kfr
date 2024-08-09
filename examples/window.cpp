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
    // Print the version of the KFR library being used
    println(library_version());

    // Define options for plotting DSP data
    const std::string options = "freqresp=True, dots=True, padwidth=1024, "
                                "log_freq=False, horizontal=False, normalized_freq=True";

    // Declare a univector of type fbase with a size of 64 to hold the window function output
    univector<fbase, 64> output;

    // Generate a Hann window function and save the plot
    output = window_hann(output.size());
    plot_save("window_hann", output, options + ", title='Hann window'");

    // Generate a Hamming window function and save the plot
    output = window_hamming(output.size());
    plot_save("window_hamming", output, options + ", title='Hamming window'");

    // Generate a Blackman window function and save the plot
    output = window_blackman(output.size());
    plot_save("window_blackman", output, options + ", title='Blackman window'");

    // Generate a Blackman-Harris window function and save the plot
    output = window_blackman_harris(output.size());
    plot_save("window_blackman_harris", output, options + ", title='Blackman-Harris window'");

    // Generate a Gaussian window function and save the plot
    output = window_gaussian(output.size());
    plot_save("window_gaussian", output, options + ", title='Gaussian window'");

    // Generate a Triangular window function and save the plot
    output = window_triangular(output.size());
    plot_save("window_triangular", output, options + ", title='Triangular window'");

    // Generate a Bartlett window function and save the plot
    output = window_bartlett(output.size());
    plot_save("window_bartlett", output, options + ", title='Bartlett window'");

    // Generate a Cosine window function and save the plot
    output = window_cosine(output.size());
    plot_save("window_cosine", output, options + ", title='Cosine window'");

    // Generate a Cosine window function compatible with numpy and save the plot
    output = window_cosine_np(output.size());
    plot_save("window_cosine_np", output, options + ", title='Cosine window (numpy compatible)'");

    // Generate a Bartlett-Hann window function and save the plot
    output = window_bartlett_hann(output.size());
    plot_save("window_bartlett_hann", output, options + ", title='Bartlett-Hann window'");

    // Generate a Bohman window function and save the plot
    output = window_bohman(output.size());
    plot_save("window_bohman", output, options + ", title='Bohman window'");

    // Generate a Lanczos window function and save the plot
    output = window_lanczos(output.size());
    plot_save("window_lanczos", output, options + ", title='Lanczos window'");

    // Generate a Flat top window function and save the plot
    output = window_flattop(output.size());
    plot_save("window_flattop", output, options + ", title='Flat top window'");

    // Generate a Kaiser window function with a beta value of 2.5 and save the plot
    output = window_kaiser(output.size(), 2.5);
    plot_save("window_kaiser", output, options + ", title='Kaiser window'");

    // Generate a Planck-taper window function with an epsilon value of 0.1 and save the plot
    output = window_planck_taper(output.size(), 0.1);
    plot_save("window_planck_taper", output, options + ", title='Planck-taper window'");

    println("SVG plots have been saved to svg directory");

    return 0;
}
