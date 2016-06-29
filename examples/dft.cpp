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

#include <kfr/dft/fft.hpp>
#include <kfr/dft/reference_dft.hpp>
#include <kfr/dsp/oscillators.hpp>
#include <kfr/dsp/units.hpp>
#include <kfr/expressions/basic.hpp>
#include <kfr/expressions/operators.hpp>
#include <kfr/expressions/reduce.hpp>
#include <kfr/math.hpp>
#include <kfr/misc/random.hpp>
#include <kfr/vec.hpp>

using namespace kfr;

int main(int argc, char** argv)
{
    println(library_version());

    // fft size
    const size_t size = 128;
    using float_type  = double;

    // initialize input & output buffers
    univector<complex<float_type>, size> in  = sin(linspace(0.0, c_pi<float_type, 2> * 4.0, size));
    univector<complex<float_type>, size> out = scalar(qnan);

    // initialize fft
    const dft_plan<float_type> dft(size);

    // allocate work buffer for fft (if needed)
    univector<u8> temp(dft.temp_size);

    // perform forward fft
    dft.execute(out, in, temp);

    // scale output
    out = out / size;

    // get magnitude and convert to decibels
    univector<float_type, size> dB = amp_to_dB(cabs(out));

    println("max  = ", max(dB));
    println("min  = ", min(dB));
    println("mean = ", mean(dB));
    println("rms  = ", rms(dB));

    println(in);
    println();
    println(dB);
    (void)argc;
    (void)argv;
    return 0;
}
