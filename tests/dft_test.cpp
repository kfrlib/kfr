/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/version.hpp>

#include <tuple>

#include "testo/testo.hpp"
#include <kfr/cometa/string.hpp>
#include <kfr/dft/fft.hpp>
#include <kfr/dft/reference_dft.hpp>
#include <kfr/expressions/basic.hpp>
#include <kfr/expressions/operators.hpp>
#include <kfr/expressions/reduce.hpp>
#include <kfr/io/tostring.hpp>
#include <kfr/math.hpp>
#include <kfr/misc/random.hpp>
#include <kfr/version.hpp>

using namespace kfr;

TEST(fft_accuracy)
{
    testo::active_test()->show_progress = true;
    random_bit_generator gen(2247448713, 915890490, 864203735, 2982561);

    testo::matrix(named("type")       = ctypes<float, double>, //
                  named("inverse")    = std::make_tuple(false, true), //
                  named("log2(size)") = make_range(1, 21), //
                  [&gen](auto type, bool inverse, size_t log2size) {
                      using float_type  = type_of<decltype(type)>;
                      const size_t size = 1 << log2size;

                      univector<complex<float_type>> in =
                          typed<float_type>(gen_random_range(gen, -1.0, +1.0), size * 2);
                      univector<complex<float_type>> out    = in;
                      univector<complex<float_type>> refout = out;
                      const dft_plan<float_type> dft(size);
                      univector<u8> temp(dft.temp_size);

                      reference_dft(refout.data(), in.data(), size, inverse);
                      dft.execute(out, out, temp, inverse);

                      const float_type rms_diff = rms(cabs(refout - out));
                      const double ops          = log2size * 100;
                      const double epsilon      = std::numeric_limits<float_type>::epsilon();
                      CHECK(rms_diff < epsilon * ops);
                  });
}

int main(int argc, char** argv)
{
    println(library_version());

    return testo::run_all("", true);
}
