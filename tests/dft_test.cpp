/**
 * KFR (http://kfrlib.com)
 * Copyright (C) 2016  D Levin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include <kfr/io.hpp>
#include <set>

using namespace kfr;

namespace CMT_ARCH_NAME
{

#ifdef CMT_NATIVE_F64
constexpr ctypes_t<float, double> dft_float_types{};
#else
constexpr ctypes_t<float> dft_float_types{};
#endif

TEST(test_convolve)
{
    univector<fbase, 5> a({ 1, 2, 3, 4, 5 });
    univector<fbase, 5> b({ 0.25, 0.5, 1.0, -2.0, 1.5 });
    univector<fbase> c = convolve(a, b);
    CHECK(c.size() == 9u);
    CHECK(rms(c - univector<fbase>({ 0.25, 1., 2.75, 2.5, 3.75, 3.5, 1.5, -4., 7.5 })) < 0.0001);
}

TEST(test_complex_convolve)
{
    univector<complex<fbase>, 5> a({ 1, 2, 3, 4, 5 });
    univector<complex<fbase>, 5> b({ 0.25, 0.5, 1.0, -2.0, 1.5 });
    univector<complex<fbase>> c = convolve(a, b);
    CHECK(c.size() == 9u);
    CHECK(rms(cabs(c - univector<complex<fbase>>({ 0.25, 1., 2.75, 2.5, 3.75, 3.5, 1.5, -4., 7.5 }))) <
          0.0001);
}

TEST(test_convolve_filter)
{
    univector<fbase, 5> a({ 1, 2, 3, 4, 5 });
    univector<fbase, 5> b({ 0.25, 0.5, 1.0, -2.0, 1.5 });
    univector<fbase, 5> dest;
    convolve_filter<fbase> filter(a);
    filter.apply(dest, b);
    CHECK(rms(dest - univector<fbase>({ 0.25, 1., 2.75, 2.5, 3.75 })) < 0.0001);
}

TEST(test_complex_convolve_filter)
{
    univector<complex<fbase>, 5> a({ 1, 2, 3, 4, 5 });
    univector<complex<fbase>, 5> b({ 0.25, 0.5, 1.0, -2.0, 1.5 });
    univector<complex<fbase>, 5> dest;
    convolve_filter<complex<fbase>> filter(a);
    filter.apply(dest, b);
    CHECK(rms(cabs(dest - univector<complex<fbase>>({ 0.25, 1., 2.75, 2.5, 3.75 }))) < 0.0001);
    filter.apply(dest, b);
    CHECK(rms(cabs(dest - univector<complex<fbase>>({ 0.25, 1., 2.75, 2.5, 3.75 }))) > 0.0001);
    filter.reset();
    filter.apply(dest, b);
    CHECK(rms(cabs(dest - univector<complex<fbase>>({ 0.25, 1., 2.75, 2.5, 3.75 }))) < 0.0001);
}

TEST(test_correlate)
{
    univector<fbase, 5> a({ 1, 2, 3, 4, 5 });
    univector<fbase, 5> b({ 0.25, 0.5, 1.0, -2.0, 1.5 });
    univector<fbase> c = correlate(a, b);
    CHECK(c.size() == 9u);
    CHECK(rms(c - univector<fbase>({ 1.5, 1., 1.5, 2.5, 3.75, -4., 7.75, 3.5, 1.25 })) < 0.0001);
}

TEST(test_complex_correlate)
{
    univector<complex<fbase>, 5> a({ 1, 2, 3, 4, 5 });
    univector<complex<fbase>, 5> b({ 0.25, 0.5, 1.0, -2.0, 1.5 });
    univector<complex<fbase>> c = correlate(a, b);
    CHECK(c.size() == 9u);
    CHECK(rms(cabs(c - univector<fbase>({ 1.5, 1., 1.5, 2.5, 3.75, -4., 7.75, 3.5, 1.25 }))) < 0.0001);
}

#if defined CMT_ARCH_ARM || !defined NDEBUG
constexpr size_t fft_stopsize = 12;
constexpr size_t dft_stopsize = 101;
#else
constexpr size_t fft_stopsize = 20;
constexpr size_t dft_stopsize = 257;
#endif

TEST(fft_real)
{
    using float_type = double;
    random_bit_generator gen(2247448713, 915890490, 864203735, 2982561);

    constexpr size_t size = 64;

    kfr::univector<float_type, size> in = gen_random_range<float_type>(gen, -1.0, +1.0);
    kfr::univector<kfr::complex<float_type>, size / 2 + 1> out = realdft(in);
    kfr::univector<float_type, size> rev                       = irealdft(out) / size;
    CHECK(rms(rev - in) <= 0.00001f);
}

TEST(fft_accuracy)
{
    testo::active_test()->show_progress = true;
    random_bit_generator gen(2247448713, 915890490, 864203735, 2982561);
    std::set<size_t> size_set;
    univector<size_t> sizes = truncate(1 + counter(), fft_stopsize - 1);
    sizes                   = round(pow(2.0, sizes));

#ifndef KFR_DFT_NO_NPo2
    univector<size_t> sizes2 = truncate(2 + counter(), dft_stopsize - 2);
    for (size_t s : sizes2)
    {
        if (std::find(sizes.begin(), sizes.end(), s) == sizes.end())
            sizes.push_back(s);
    }
#endif
    println(sizes);

    testo::matrix(named("type") = dft_float_types, //
                  named("size") = sizes, //
                  [&gen](auto type, size_t size) {
                      using float_type      = type_of<decltype(type)>;
                      const double min_prec = 0.000001 * std::log(size) * size;

                      for (bool inverse : { false, true })
                      {
                          testo::scope s(inverse ? "complex-inverse" : "complex-direct");
                          univector<complex<float_type>> in =
                              truncate(gen_random_range<float_type>(gen, -1.0, +1.0), size);
                          univector<complex<float_type>> out    = in;
                          univector<complex<float_type>> refout = out;
                          univector<complex<float_type>> outo   = in;
                          const dft_plan<float_type> dft(size);
                          if (!inverse)
                          {
                              dft.dump();
                          }
                          univector<u8> temp(dft.temp_size);

                          reference_dft(refout.data(), in.data(), size, inverse);
                          dft.execute(outo, in, temp, inverse);
                          dft.execute(out, out, temp, inverse);

                          const float_type rms_diff_inplace = rms(cabs(refout - out));
                          CHECK(rms_diff_inplace < min_prec);
                          const float_type rms_diff_outofplace = rms(cabs(refout - outo));
                          CHECK(rms_diff_outofplace < min_prec);
                      }

                      if (size >= 4 && is_poweroftwo(size))
                      {
                          univector<float_type> in =
                              truncate(gen_random_range<float_type>(gen, -1.0, +1.0), size);

                          univector<complex<float_type>> out    = truncate(scalar(qnan), size);
                          univector<complex<float_type>> refout = truncate(scalar(qnan), size);
                          const dft_plan_real<float_type> dft(size);
                          univector<u8> temp(dft.temp_size);

                          testo::scope s("real-direct");
                          reference_fft(refout.data(), in.data(), size);
                          dft.execute(out, in, temp);
                          float_type rms_diff =
                              rms(cabs(refout.truncate(size / 2 + 1) - out.truncate(size / 2 + 1)));
                          CHECK(rms_diff < min_prec);

                          univector<float_type> out2(size, 0.f);
                          s.text = "real-inverse";
                          dft.execute(out2, out, temp);
                          out2     = out2 / size;
                          rms_diff = rms(in - out2);
                          CHECK(rms_diff < min_prec);
                      }
                  });
}

TEST(dct)
{
    constexpr size_t size = 16;
    dct_plan<float> plan(size);
    univector<float, size> in = counter();
    univector<float, size> out;
    univector<float, size> outinv;
    univector<u8> tmp(plan.temp_size);
    plan.execute(out, in, tmp, false);

    univector<float, size> refout = { 120., -51.79283109806667,  0., -5.6781471211595695,
                                      0.,   -1.9843883778092053, 0., -0.9603691873838152,
                                      0.,   -0.5308329190495176, 0., -0.3030379000702155,
                                      0.,   -0.1584982220313824, 0., -0.0494839805703826 };

    CHECK(rms(refout - out) < 0.00001f);

    plan.execute(outinv, in, tmp, true);

    univector<float, size> refoutinv = { 59.00747544192212,  -65.54341437693878,  27.70332758523579,
                                         -24.56124678824279, 15.546989102481612,  -14.293082621965974,
                                         10.08224348063459,  -9.38097406470581,   6.795411054455922,
                                         -6.320715753372687, 4.455202292297903,   -4.0896421269390455,
                                         2.580439536964837,  -2.2695816108369176, 0.9311870090070382,
                                         -0.643618159997807 };

    CHECK(rms(refoutinv - outinv) < 0.00001f);
}
} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version(), " running on ", cpu_runtime());

    return testo::run_all("", false);
}
#endif
