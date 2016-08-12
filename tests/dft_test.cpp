/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

// library_version()
#include <kfr/version.hpp>

#include <tuple>

#include "testo/testo.hpp"
#include <kfr/base/basic_expressions.hpp>
#include <kfr/base/random.hpp>
#include <kfr/base/reduce.hpp>
#include <kfr/cometa/string.hpp>
#include <kfr/dft/fft.hpp>
#include <kfr/dft/conv.hpp>
#include <kfr/dft/reference_dft.hpp>
#include <kfr/io/tostring.hpp>
#include <kfr/math.hpp>
#include <kfr/version.hpp>

using namespace kfr;

#ifdef KFR_NATIVE_F64
constexpr ctypes_t<float, double> float_types{};
#else
constexpr ctypes_t<float> float_types{};
#endif

TEST(test_convolve)
{
    univector<fbase, 5> a({ 1, 2, 3, 4, 5 });
    univector<fbase, 5> b({ 0.25, 0.5, 1.0, 0.5, 0.25 });
    univector<fbase> c = convolve(a, b);
    CHECK(c.size() == 9);
    CHECK(rms(c - univector<fbase>({ 0.25, 1., 2.75, 5., 7.5, 8.5, 7.75, 3.5, 1.25 })) < 0.0001);
}

TEST(fft_accuracy)
{
    testo::active_test()->show_progress = true;
    random_bit_generator gen(2247448713, 915890490, 864203735, 2982561);

    testo::matrix(named("type")       = float_types, //
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
