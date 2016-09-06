/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main(int argc, char** argv)
{
    println(library_version());

    const std::string options = "phaseresp=False";

    univector<fbase, 15> taps15;
    univector<fbase, 127> taps127;
    univector<fbase, 8191> taps8191;

    expression_pointer<fbase> hann = to_pointer(window_hann(taps15.size()));

    expression_pointer<fbase> kaiser = to_pointer(window_kaiser(taps127.size(), 3.0));

    expression_pointer<fbase> blackman_harris = to_pointer(window_blackman_harris(taps8191.size()));

    fir_lowpass(taps15, 0.15, hann, true);
    plot_save("fir_lowpass_hann", taps15, options + ", title='15-point lowpass FIR, Hann window'");

    fir_lowpass(taps127, 0.2, kaiser, true);
    plot_save("fir_lowpass_kaiser", taps127,
              options + ", title=r'127-point lowpass FIR, Kaiser window ($\\alpha=3.0$)'");

    fir_highpass(taps127, 0.2, kaiser, true);
    plot_save("fir_highpass_kaiser", taps127,
              options + ", title=r'127-point highpass FIR, Kaiser window ($\\alpha=3.0$)'");

    fir_bandpass(taps127, 0.2, 0.4, kaiser, true);
    plot_save("fir_bandpass_kaiser", taps127,
              options + ", title=r'127-point bandpass FIR, Kaiser window ($\\alpha=3.0$)'");

    fir_bandstop(taps127, 0.2, 0.4, kaiser, true);
    plot_save("fir_bandstop_kaiser", taps127,
              options + ", title=r'127-point bandstop FIR, Kaiser window ($\\alpha=3.0$)'");

    fir_lowpass(taps8191, 0.15, blackman_harris, true);
    plot_save("fir_lowpass_blackman", taps8191,
              options + ", title='8191-point lowpass FIR, Blackman-Harris window'");

    return 0;
}
