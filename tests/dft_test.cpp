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
#include <kfr/io/tostring.hpp>
#include <set>

using namespace kfr;

namespace CMT_ARCH_NAME
{

TEST(print_vector_capacity)
{
    println("vector_capacity<float> = ", vector_capacity<float>);
    println("vector_capacity<double> = ", vector_capacity<double>);

    println("fft_config<float>::process_width = ", vector_capacity<float> / 16);
    println("fft_config<double>::process_width = ", vector_capacity<double> / 16);
}

#ifdef CMT_NATIVE_F64
constexpr ctypes_t<float, double> dft_float_types{};
#else
constexpr ctypes_t<float> dft_float_types{};
#endif

#if defined(CMT_ARCH_X86) && !defined(KFR_NO_PERF_TESTS)

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
    for (int size = 16; size <= 65536; size <<= 1)
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
#ifndef KFR_DFT_NO_NPo2
constexpr size_t dft_stopsize = 101;
#endif
#else
constexpr size_t fft_stopsize = 21;
#ifndef KFR_DFT_NO_NPo2
constexpr size_t dft_stopsize = 257;
#endif
#endif

TEST(fft_accuracy)
{
#ifdef DEBUG_DFT_PROGRESS
    testo::active_test()->show_progress = true;
#endif
    random_state gen = random_init(2247448713, 915890490, 864203735, 2982561);
    std::set<size_t> size_set;
    univector<size_t> sizes = truncate(counter(), fft_stopsize);
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

    testo::matrix(
        named("type") = dft_float_types, //
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
                CHECK(rms_diff_inplace <= min_prec2);
                const float_type rms_diff_outofplace = rms(cabs(refout - outo));
                CHECK(rms_diff_outofplace <= min_prec2);
            }

            if (is_even(size))
            {
                index_t csize            = dft_plan_real<float_type>::complex_size_for(size);
                univector<float_type> in = truncate(gen_random_range<float_type>(gen, -1.0, +1.0), size);

                univector<complex<float_type>> out    = truncate(dimensions<1>(scalar(qnan)), csize);
                univector<complex<float_type>> refout = truncate(dimensions<1>(scalar(qnan)), csize);
                const dft_plan_real<float_type> dft(size);
                univector<u8> temp(dft.temp_size);

                {
                    testo::scope s("real-direct");
                    reference_dft(refout.data(), in.data(), size);
                    dft.execute(out, in, temp);
                    float_type rms_diff_outofplace = rms(cabs(refout - out));
                    CHECK(rms_diff_outofplace <= min_prec);

                    univector<complex<float_type>> outi(csize);
                    outi = padded(make_univector(ptr_cast<complex<float_type>>(in.data()), size / 2),
                                  complex<float_type>{ 0.f });
                    dft.execute(outi.data(), ptr_cast<float_type>(outi.data()), temp.data());
                    float_type rms_diff_inplace = rms(cabs(refout - outi.truncate(csize)));
                    CHECK(rms_diff_inplace <= min_prec);
                }

                {
                    testo::scope s("real-inverse");
                    univector<float_type> out2(size, 0.f);
                    dft.execute(out2, out, temp);
                    out2                           = out2 / size;
                    float_type rms_diff_outofplace = rms(in - out2);
                    CHECK(rms_diff_outofplace <= min_prec);

                    univector<float_type> outi(2 * csize);
                    outi = make_univector(ptr_cast<float_type>(out.data()), 2 * csize);

                    dft.execute(outi.data(), ptr_cast<complex<float_type>>(outi.data()), temp.data());
                    outi                        = outi / size;
                    float_type rms_diff_inplace = rms(in - outi.truncate(size));
                    CHECK(rms_diff_inplace <= min_prec);
                }
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

template <typename T, index_t Dims, typename dft_type, typename dft_real_type>
static void test_dft_md_t(random_state& gen, shape<Dims> shape)
{
    index_t size = shape.product();
    testo::scope s(as_string("shape=", shape));

    const double min_prec = 0.000002 * std::log(size) * size;

    {
        const dft_type dft(shape);
#if DEBUG_DFT_PROGRESS
        dft.dump();
#endif
        univector<complex<T>> in = truncate(gen_random_range<T>(gen, -1.0, +1.0), size);
        for (bool inverse : { false, true })
        {
            testo::scope s(inverse ? "complex-inverse" : "complex-direct");
            univector<complex<T>> out    = in;
            univector<complex<T>> refout = out;
            univector<complex<T>> outo   = in;
            univector<u8> temp(dft.temp_size);

            reference_dft_md(refout.data(), in.data(), shape, inverse);
            dft.execute(outo.data(), in.data(), temp.data(), inverse);
            dft.execute(out.data(), out.data(), temp.data(), inverse);

            const T rms_diff_inplace = rms(cabs(refout - out));
            CHECK(rms_diff_inplace <= min_prec);
            const T rms_diff_outofplace = rms(cabs(refout - outo));
            CHECK(rms_diff_outofplace <= min_prec);
        }
    }

    if (is_even(shape.back()))
    {
        index_t csize   = dft_plan_md_real<float, Dims>::complex_size_for(shape).product();
        univector<T> in = truncate(gen_random_range<T>(gen, -1.0, +1.0), size);

        univector<complex<T>> out    = truncate(dimensions<1>(scalar(qnan)), csize);
        univector<complex<T>> refout = truncate(dimensions<1>(scalar(qnan)), csize);
        const dft_real_type dft(shape, true);
#if DEBUG_DFT_PROGRESS
        dft.dump();
#endif
        univector<u8> temp(dft.temp_size);

        {
            testo::scope s("real-direct");
            reference_dft_md(refout.data(), in.data(), shape);
            dft.execute(out.data(), in.data(), temp.data());
            T rms_diff_outofplace = rms(cabs(refout - out));
            CHECK(rms_diff_outofplace <= min_prec);

            univector<complex<T>> outi(csize);
            outi = padded(make_univector(ptr_cast<complex<T>>(in.data()), size / 2), complex<T>{ 0.f });
            dft.execute(outi.data(), ptr_cast<T>(outi.data()), temp.data());
            T rms_diff_inplace = rms(cabs(refout - outi));
            CHECK(rms_diff_inplace <= min_prec);
        }

        {
            testo::scope s("real-inverse");
            univector<T> out2(dft.real_out_size(), 0.f);
            dft.execute(out2.data(), out.data(), temp.data());
            out2                  = out2 / size;
            T rms_diff_outofplace = rms(in - out2.truncate(size));
            CHECK(rms_diff_outofplace <= min_prec);

            univector<T> outi(2 * csize);
            outi = make_univector(ptr_cast<T>(out.data()), 2 * csize);
            dft.execute(outi.data(), ptr_cast<complex<T>>(outi.data()), temp.data());
            outi               = outi / size;
            T rms_diff_inplace = rms(in - outi.truncate(size));
            CHECK(rms_diff_inplace <= min_prec);
        }
    }
}

template <typename T, index_t Dims>
static void test_dft_md(random_state& gen, shape<Dims> shape)
{
    {
        testo::scope s("compile-time dims");
        test_dft_md_t<T, Dims, dft_plan_md<T, Dims>, dft_plan_md_real<T, Dims>>(gen, shape);
    }
    {
        testo::scope s("runtime dims");
        test_dft_md_t<T, Dims, dft_plan_md<T, dynamic_shape>, dft_plan_md_real<T, dynamic_shape>>(gen, shape);
    }
}

TEST(dft_md)
{
    random_state gen = random_init(2247448713, 915890490, 864203735, 2982561);

    testo::matrix(named("type") = dft_float_types, //
                  [&gen](auto type)
                  {
                      using float_type = type_of<decltype(type)>;
                      test_dft_md<float_type>(gen, shape{ 120 });
                      test_dft_md<float_type>(gen, shape{ 2, 60 });
                      test_dft_md<float_type>(gen, shape{ 3, 40 });
                      test_dft_md<float_type>(gen, shape{ 4, 30 });
                      test_dft_md<float_type>(gen, shape{ 5, 24 });
                      test_dft_md<float_type>(gen, shape{ 6, 20 });
                      test_dft_md<float_type>(gen, shape{ 8, 15 });
                      test_dft_md<float_type>(gen, shape{ 10, 12 });
                      test_dft_md<float_type>(gen, shape{ 12, 10 });
                      test_dft_md<float_type>(gen, shape{ 15, 8 });
                      test_dft_md<float_type>(gen, shape{ 20, 6 });
                      test_dft_md<float_type>(gen, shape{ 24, 5 });
                      test_dft_md<float_type>(gen, shape{ 30, 4 });
                      test_dft_md<float_type>(gen, shape{ 40, 3 });
                      test_dft_md<float_type>(gen, shape{ 60, 2 });

                      test_dft_md<float_type>(gen, shape{ 2, 3, 24 });
                      test_dft_md<float_type>(gen, shape{ 12, 5, 2 });
                      test_dft_md<float_type>(gen, shape{ 5, 12, 2 });

                      test_dft_md<float_type>(gen, shape{ 2, 3, 2, 12 });
                      test_dft_md<float_type>(gen, shape{ 3, 4, 5, 2 });
                      test_dft_md<float_type>(gen, shape{ 5, 4, 3, 2 });

                      test_dft_md<float_type>(gen, shape{ 5, 2, 2, 3, 2 });
                      test_dft_md<float_type>(gen, shape{ 2, 5, 2, 2, 3 });

                      test_dft_md<float_type>(gen, shape{ 1, 120 });
                      test_dft_md<float_type>(gen, shape{ 120, 1 });
                      test_dft_md<float_type>(gen, shape{ 2, 1, 1, 60 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 10, 2, 1, 3 });

                      test_dft_md<float_type>(gen, shape{ 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4, 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4, 4, 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4, 4, 4, 4, 4, 4 });
                      test_dft_md<float_type>(gen, shape{ 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 });
#if defined NDEBUG
                      test_dft_md<float_type>(gen, shape{ 512, 512 });
                      test_dft_md<float_type>(gen, shape{ 32, 32, 32 });
                      test_dft_md<float_type>(gen, shape{ 8, 8, 8, 8 });
                      test_dft_md<float_type>(gen, shape{ 2, 2, 2, 2, 2, 2 });

                      test_dft_md<float_type>(gen, shape{ 1, 65536 });
                      test_dft_md<float_type>(gen, shape{ 2, 65536 });
                      test_dft_md<float_type>(gen, shape{ 3, 65536 });
                      test_dft_md<float_type>(gen, shape{ 4, 65536 });
                      test_dft_md<float_type>(gen, shape{ 65536, 1 });
                      test_dft_md<float_type>(gen, shape{ 65536, 2 });
                      test_dft_md<float_type>(gen, shape{ 65536, 3 });
                      test_dft_md<float_type>(gen, shape{ 65536, 4 });

                      test_dft_md<float_type>(gen, shape{ 1, 2 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 3 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 3, 4 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 3, 4, 5 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 3, 4, 5, 6 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 3, 4, 5, 6, 7 });
                      test_dft_md<float_type>(gen, shape{ 1, 2, 3, 4, 5, 6, 7, 8 });
                      test_dft_md<float_type>(gen, shape{ 2, 1 });
                      test_dft_md<float_type>(gen, shape{ 3, 2, 1 });
                      test_dft_md<float_type>(gen, shape{ 4, 3, 2, 1 });
                      test_dft_md<float_type>(gen, shape{ 5, 4, 3, 2, 1 });
                      test_dft_md<float_type>(gen, shape{ 6, 5, 4, 3, 2, 1 });
                      test_dft_md<float_type>(gen, shape{ 7, 6, 5, 4, 3, 2, 1 });
                      test_dft_md<float_type>(gen, shape{ 8, 7, 6, 5, 4, 3, 2, 1 });
#endif
                  });
}

} // namespace CMT_ARCH_NAME

#ifndef KFR_NO_MAIN
int main()
{
    println(library_version(), " running on ", cpu_runtime());

    return testo::run_all("", false);
}
#endif
