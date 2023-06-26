/*
 * ccv, part of KFR (https://www.kfr.dev)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

// Complex convolution filter examples

#define CMT_BASETYPE_F32

#include <chrono>
#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>

using namespace kfr;

int main()
{
    println(library_version());

    // low-pass filter
    univector<fbase, 1023> taps127;
    expression_handle<fbase> kaiser = to_handle(window_kaiser(taps127.size(), 3.0));
    fir_lowpass(taps127, 0.2, kaiser, true);

    // Create filters.
    size_t const block_size = 256;
    convolve_filter<complex<fbase>> conv_filter_complex(
        univector<complex<fbase>>(make_complex(taps127, zeros())), block_size);
    convolve_filter<fbase> conv_filter_real(taps127, block_size);

    // Create noise to filter.
    auto const size = 1024 * 100 + 33; // not a multiple of block_size
    univector<complex<fbase>> cnoise =
        make_complex(truncate(gen_random_range(random_init(1, 2, 3, 4), -1.f, +1.f), size),
                     truncate(gen_random_range(random_init(3, 4, 9, 8), -1.f, +1.f), size));
    univector<fbase> noise = truncate(gen_random_range(random_init(3, 4, 9, 8), -1.f, +1.f), size);

    // Filter results.
    univector<complex<fbase>> filtered_cnoise_ccv(size), filtered_cnoise_fir(size);
    univector<fbase> filtered_noise_ccv(size), filtered_noise_fir(size);

    // Complex filtering (time and compare).
    auto tic = std::chrono::high_resolution_clock::now();
    conv_filter_complex.apply(filtered_cnoise_ccv, cnoise);
    auto toc                    = std::chrono::high_resolution_clock::now();
    auto const ccv_time_complex = std::chrono::duration_cast<std::chrono::duration<float>>(toc - tic);
    tic                         = toc;
    filtered_cnoise_fir         = kfr::fir(cnoise, taps127);
    toc                         = std::chrono::high_resolution_clock::now();
    auto const fir_time_complex = std::chrono::duration_cast<std::chrono::duration<float>>(toc - tic);
    auto const cdiff            = rms(cabs(filtered_cnoise_fir - filtered_cnoise_ccv));

    // Real filtering (time and compare).
    tic = std::chrono::high_resolution_clock::now();
    conv_filter_real.apply(filtered_noise_ccv, noise);
    toc                      = std::chrono::high_resolution_clock::now();
    auto const ccv_time_real = std::chrono::duration_cast<std::chrono::duration<float>>(toc - tic);
    tic                      = toc;
    filtered_noise_fir       = kfr::fir(noise, taps127);
    toc                      = std::chrono::high_resolution_clock::now();
    auto const fir_time_real = std::chrono::duration_cast<std::chrono::duration<float>>(toc - tic);
    auto const diff          = rms(filtered_noise_fir - filtered_noise_ccv);

    println("complex: convolution_filter ", ccv_time_complex.count(), " fir ", fir_time_complex.count(),
            " diff=", cdiff);
    println("real: convolution_filter ", ccv_time_real.count(), " fir ", fir_time_real.count(),
            " diff=", diff);

    return 0;
}
