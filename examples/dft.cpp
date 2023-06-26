/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>

using namespace kfr;

int main()
{
    println(library_version());

    // fft size
    const size_t size = 128;

    // initialize input & output buffers
    univector<complex<fbase>, size> in  = sin(linspace(0.0, c_pi<fbase, 2> * 4.0, size));
    univector<complex<fbase>, size> out = scalar(qnan);

    // initialize fft
    const dft_plan<fbase> dft(size);

    dft.dump();

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
    return 0;
}
