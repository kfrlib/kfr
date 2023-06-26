/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <kfr/testo/testo.hpp>

#include <chrono>
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

#if defined(CMT_ARCH_X86)

static void full_barrier()
{
#ifdef CMT_COMPILER_GNU
    asm volatile("mfence" ::: "memory");
#else
    _ReadWriteBarrier();
#endif
}
static CMT_NOINLINE void dont_optimize(const void* in)
{
#ifdef CMT_COMPILER_GNU
    asm volatile("" : "+m"(in));
#else
    volatile uint8_t a = *reinterpret_cast<const uint8_t*>(in);
#endif
}

template <typename T>
static void perf_test_t(int size)
{
    print("[PERFORMANCE] DFT ", fmt<'s', 6>(type_name<T>()), " ", fmt<'d', 6>(size), "...");
    random_state gen1 = random_init(2247448713, 915890490, 864203735, 2982561);
    random_state gen2 = random_init(2982561, 2247448713, 915890490, 864203735);
    std::chrono::high_resolution_clock::duration duration(0);
    dft_plan<T> dft(size);
    univector<u8> tmp(dft.temp_size);
    uint64_t counter = 0;
    while (duration < std::chrono::seconds(1))
    {
        univector<complex<T>> data(size);
        data = make_complex(gen_random_range<T>(gen1, -1.0, +1.0), gen_random_range<T>(gen2, -1.0, +1.0));
        full_barrier();
        auto start = std::chrono::high_resolution_clock::now();
        dft.execute(data, data, tmp);

        full_barrier();
        duration += std::chrono::high_resolution_clock::now() - start;
        dont_optimize(data.data());
        ++counter;
    }
    double opspersecond = counter / (std::chrono::nanoseconds(duration).count() / 1'000'000'000.0);
    println(" ", fmt<'f', 12, 1>(opspersecond), " ops/second");
}

static void perf_test(int size)
{
    perf_test_t<float>(size);
    perf_test_t<double>(size);
}

TEST(test_performance)
{
    for (int size = 16; size <= 16384; size <<= 1)
    {
        perf_test(size);
    }

#ifndef KFR_DFT_NO_NPo2
    perf_test(210);
    perf_test(3150);
    perf_test(211);
    perf_test(3163);
#endif
}
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
    random_state gen = random_init(2247448713, 915890490, 864203735, 2982561);

    constexpr size_t size = 64;

    kfr::univector<float_type, size> in = gen_random_range<float_type>(gen, -1.0, +1.0);
    kfr::univector<kfr::complex<float_type>, size / 2 + 1> out = realdft(in);
    kfr::univector<float_type, size> rev                       = irealdft(out) / size;
    CHECK(rms(rev - in) <= 0.00001f);
}

TEST(fft_real_not_size_4N)
{
    kfr::univector<double, 6> in = counter();
    auto out = realdft(in);
    kfr::univector<kfr::complex<double>> expected { 
        15.0, { -3, 5.19615242}, {-3, +1.73205081}, -3.0 };
    CHECK(rms(cabs(out - expected)) <= 0.00001f);
    kfr::univector<double, 6> rev = irealdft(out) / 6;
    CHECK(rms(rev - in) <= 0.00001f);

    random_state gen = random_init(2247448713, 915890490, 864203735, 2982561);
    constexpr size_t size = 66;
    kfr::univector<double, size> in2 = gen_random_range<double>(gen, -1.0, +1.0);
    kfr::univector<kfr::complex<double>, size / 2 + 1> out2 = realdft(in2);
    kfr::univector<double, size> rev2                       = irealdft(out2) / size;
    CHECK(rms(rev2 - in2) <= 0.00001f);
}

TEST(fft_accuracy)
{
#ifdef DEBUG_DFT_PROGRESS
    testo::active_test()->show_progress = true;
#endif
    random_state gen = random_init(2247448713, 915890490, 864203735, 2982561);
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
#ifdef DEBUG_DFT_PROGRESS
    println(sizes);
#endif

    testo::matrix(named("type") = dft_float_types, //
                  named("size") = sizes, //
                  [&gen](auto type, size_t size)
                  {
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
                          double min_prec2 = dft.arblen ? 2 * min_prec : min_prec;
                          if (!inverse)
                          {
#if DEBUG_DFT_PROGRESS
                              dft.dump();
#endif
                          }
                          univector<u8> temp(dft.temp_size);

                          reference_dft(refout.data(), in.data(), size, inverse);
                          dft.execute(outo, in, temp, inverse);
                          dft.execute(out, out, temp, inverse);

                          const float_type rms_diff_inplace = rms(cabs(refout - out));
                          CHECK(rms_diff_inplace < min_prec2);
                          const float_type rms_diff_outofplace = rms(cabs(refout - outo));
                          CHECK(rms_diff_outofplace < min_prec2);
                      }

                      if (size >= 4 && is_poweroftwo(size))
                      {
                          univector<float_type> in =
                              truncate(gen_random_range<float_type>(gen, -1.0, +1.0), size);

                          univector<complex<float_type>> out    = truncate(dimensions<1>(scalar(qnan)), size);
                          univector<complex<float_type>> refout = truncate(dimensions<1>(scalar(qnan)), size);
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

    univector<float, size> refout = { 120.f, -51.79283109806667f,  0.f, -5.6781471211595695f,
                                      0.f,   -1.9843883778092053f, 0.f, -0.9603691873838152f,
                                      0.f,   -0.5308329190495176f, 0.f, -0.3030379000702155f,
                                      0.f,   -0.1584982220313824f, 0.f, -0.0494839805703826f };

    CHECK(rms(refout - out) < 0.00001f);

    plan.execute(outinv, in, tmp, true);

    univector<float, size> refoutinv = { 59.00747544192212f,  -65.54341437693878f,  27.70332758523579f,
                                         -24.56124678824279f, 15.546989102481612f,  -14.293082621965974f,
                                         10.08224348063459f,  -9.38097406470581f,   6.795411054455922f,
                                         -6.320715753372687f, 4.455202292297903f,   -4.0896421269390455f,
                                         2.580439536964837f,  -2.2695816108369176f, 0.9311870090070382f,
                                         -0.643618159997807f };

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
