/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/version.hpp>

#include <kfr/io/tostring.hpp>

// print(), format()
#include <kfr/cometa/string.hpp>

#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/random.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/dft/fft.hpp>
#include <kfr/dft/reference_dft.hpp>
#include <kfr/dsp/oscillators.hpp>
#include <kfr/dsp/units.hpp>
#include <kfr/math.hpp>

using namespace kfr;

int main(int argc, char** argv)
{
    println(library_version());

    // fft size
    const size_t size = 128;

    // initialize input & output buffers
    univector<complex<fbase>, size> in  = sin(linspace(0.0, c_pi<fbase, 2> * 4.0, size));
    univector<complex<fbase>, size> out = scalar(qnan);

    // initialize fft
    const dft_plan<fbase> dft(size);

    // allocate work buffer for fft (if needed)
    univector<u8> temp(dft.temp_size);

    // perform forward fft
    dft.execute(out, in, temp);

    // scale output
    out = out / size;

    // get magnitude and convert to decibels
    univector<fbase, size> dB = amp_to_dB(cabs(out));

    println("max  = ", maxof(dB));
    println("min  = ", minof(dB));
    println("mean = ", mean(dB));
    println("rms  = ", rms(dB));

    println(in);
    println();
    println(dB);
    (void)argc;
    (void)argv;
    return 0;
}
