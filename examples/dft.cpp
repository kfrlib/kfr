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
    // Print the version of the KFR library being used
    println(library_version());

    // Define the size of the Fast Fourier Transform (FFT)
    const size_t size = 128;

    // Initialize input and output buffers
    // 'in' buffer is filled with a sine wave spanning 4 cycles over the given range
    // 'out' buffer is initialized with 'qnan' (quiet NaN) to represent uninitialized state
    univector<complex<fbase>, size> in  = sin(linspace(0.0, c_pi<fbase, 2> * 4.0, size));
    univector<complex<fbase>, size> out = scalar(qnan);

    // Create an FFT plan for the defined size
    const dft_plan<fbase> dft(size);

    // Dump the details of the FFT plan (for debugging or information purposes)
    dft.dump();

    // Allocate a temporary buffer for the FFT computation if needed
    univector<u8> temp(dft.temp_size);

    // Perform the forward FFT on the input buffer 'in', store the result in the 'out' buffer
    dft.execute(out, in, temp);

    // Scale the output of the FFT by dividing by the size
    out = out / size;

    // Convert the amplitude of the FFT output to decibels (dB)
    // 'cabs' computes the magnitude of the complex numbers in 'out'
    // 'amp_to_dB' converts the amplitude values to decibel scale
    univector<fbase, size> dB = amp_to_dB(cabs(out));

    // Print the maximum, minimum, mean, and root mean square (RMS) of the dB values
    println("max  = ", maxof(dB));
    println("min  = ", minof(dB));
    println("mean = ", mean(dB));
    println("rms  = ", rms(dB));

    // Print the input buffer 'in'
    println(in);
    println();
    // Print the dB values
    println(dB);
    return 0;
}
