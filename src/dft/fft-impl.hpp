/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "dft-fft.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif
#if CMT_HAS_WARNING("-Wunused-lambda-capture")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wunused-lambda-capture")
#endif
#if CMT_HAS_WARNING("-Wpass-failed")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpass-failed")
#endif

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

constexpr bool inline always_br2 = true;

template <typename T>
inline std::bitset<DFT_MAX_STAGES> fft_algorithm_selection;

template <>
inline std::bitset<DFT_MAX_STAGES> fft_algorithm_selection<float>{ (1ull << 15) - 1 };

template <>
inline std::bitset<DFT_MAX_STAGES> fft_algorithm_selection<double>{ 0 };

template <typename T>
constexpr bool inline use_autosort(size_t log2n)
{
    return fft_algorithm_selection<T>[log2n];
}

#define KFR_AUTOSORT_FOR_128D
#define KFR_AUTOSORT_FOR_256D
#define KFR_AUTOSORT_FOR_512
#define KFR_AUTOSORT_FOR_1024
#define KFR_AUTOSORT_FOR_2048

#ifdef CMT_ARCH_AVX
template <>
KFR_INTRINSIC vec<float, 32> ctranspose<4, float, 32>(const vec<float, 32>& v16)
{
    cvec<float, 4> r0, r1, r2, r3;
    split(v16, r0, r1, r2, r3);
    const __m256d t0 = _mm256_unpacklo_pd(_mm256_castps_pd(r0.v), _mm256_castps_pd(r1.v));
    const __m256d t1 = _mm256_unpacklo_pd(_mm256_castps_pd(r2.v), _mm256_castps_pd(r3.v));
    const __m256d t2 = _mm256_unpackhi_pd(_mm256_castps_pd(r0.v), _mm256_castps_pd(r1.v));
    const __m256d t3 = _mm256_unpackhi_pd(_mm256_castps_pd(r2.v), _mm256_castps_pd(r3.v));
    r0.v             = _mm256_castpd_ps(_mm256_permute2f128_pd(t0, t1, 0x20));
    r1.v             = _mm256_castpd_ps(_mm256_permute2f128_pd(t2, t3, 0x20));
    r2.v             = _mm256_castpd_ps(_mm256_permute2f128_pd(t0, t1, 0x31));
    r3.v             = _mm256_castpd_ps(_mm256_permute2f128_pd(t2, t3, 0x31));
    return concat(r0, r1, r2, r3);
}
#endif

template <>
KFR_INTRINSIC vec<float, 64> ctranspose<8, float, 64>(const vec<float, 64>& v32)
{
    cvec<float, 4> a0, a1, a2, a3, a4, a5, a6, a7;
    split(v32, a0, a1, a2, a3, a4, a5, a6, a7);
    cvec<float, 16> even = concat(a0, a2, a4, a6);
    cvec<float, 16> odd  = concat(a1, a3, a5, a7);
    even                 = ctranspose<4>(even);
    odd                  = ctranspose<4>(odd);
    return concat(even, odd);
}

template <>
KFR_INTRINSIC vec<float, 64> ctranspose<4, float, 64>(const vec<float, 64>& v32)
{
    cvec<float, 16> lo, hi;
    split(v32, lo, hi);
    lo = ctranspose<4>(lo);
    hi = ctranspose<4>(hi);
    cvec<float, 4> a0, a1, a2, a3, a4, a5, a6, a7;
    split(lo, a0, a1, a2, a3);
    split(hi, a4, a5, a6, a7);
    return concat(a0, a4, a1, a5, a2, a6, a3, a7);
}

namespace intrinsics
{

template <size_t width, bool inverse, typename T>
KFR_INTRINSIC cvec<T, width> radix4_apply_twiddle(csize_t<width>, cfalse_t /*split_format*/, cbool_t<inverse>,
                                                  const cvec<T, width>& w, const cvec<T, width>& tw)
{
    cvec<T, width> ww  = w;
    cvec<T, width> tw_ = tw;
    cvec<T, width> b1  = ww * dupeven(tw_);
    ww                 = swap<2>(ww);

    if constexpr (inverse)
    {
        ww = addsub(b1, ww * dupodd(tw_));
    }
    else
    {
        ww = subadd(b1, ww * dupodd(tw_));
    }
    return ww;
}

template <size_t width, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC void radix4_body(size_t N, csize_t<width>, cfalse_t, cfalse_t, cfalse_t, cbool_t<use_br2>,
                               cbool_t<inverse>, cbool_t<aligned>, complex<T>* out, const complex<T>* in,
                               const complex<T>* twiddle)
{
    const size_t N4 = N / 4;
    cvec<T, width> w1, w2, w3;

    cvec<T, width> sum02, sum13, diff02, diff13;

    cvec<T, width> a0, a1, a2, a3;
    a0    = cread<width, aligned>(in + 0);
    a2    = cread<width, aligned>(in + N4 * 2);
    sum02 = a0 + a2;

    a1    = cread<width, aligned>(in + N4);
    a3    = cread<width, aligned>(in + N4 * 3);
    sum13 = a1 + a3;

    cwrite<width, aligned>(out, sum02 + sum13);
    w2 = sum02 - sum13;
    cwrite<width, aligned>(out + N4 * (use_br2 ? 1 : 2),
                           radix4_apply_twiddle(csize_t<width>(), cfalse, cbool_t<inverse>(), w2,
                                                cread<width, true>(twiddle + width)));
    diff02 = a0 - a2;
    diff13 = a1 - a3;
    if constexpr (inverse)
    {
        diff13 = (diff13 ^ broadcast<width * 2, T>(T(), -T()));
        diff13 = swap<2>(diff13);
    }
    else
    {
        diff13 = swap<2>(diff13);
        diff13 = (diff13 ^ broadcast<width * 2, T>(T(), -T()));
    }

    w1 = diff02 + diff13;

    cwrite<width, aligned>(out + N4 * (use_br2 ? 2 : 1),
                           radix4_apply_twiddle(csize_t<width>(), cfalse, cbool_t<inverse>(), w1,
                                                cread<width, true>(twiddle + 0)));
    w3 = diff02 - diff13;
    cwrite<width, aligned>(out + N4 * 3, radix4_apply_twiddle(csize_t<width>(), cfalse, cbool_t<inverse>(),
                                                              w3, cread<width, true>(twiddle + width * 2)));
}

template <size_t width, bool inverse, typename T>
KFR_INTRINSIC cvec<T, width> radix4_apply_twiddle(csize_t<width>, ctrue_t /*split_format*/, cbool_t<inverse>,
                                                  const cvec<T, width>& w, const cvec<T, width>& tw)
{
    vec<T, width> re1, im1, twre, twim;
    split<T, 2 * width>(w, re1, im1);
    split<T, 2 * width>(tw, twre, twim);

    const vec<T, width> b1re = re1 * twre;
    const vec<T, width> b1im = im1 * twre;
    if constexpr (inverse)
        return concat(b1re + im1 * twim, b1im - re1 * twim);
    else
        return concat(b1re - im1 * twim, b1im + re1 * twim);
}

template <size_t width, bool splitout, bool splitin, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC void radix4_body(size_t N, csize_t<width>, ctrue_t, cbool_t<splitout>, cbool_t<splitin>,
                               cbool_t<use_br2>, cbool_t<inverse>, cbool_t<aligned>, complex<T>* out,
                               const complex<T>* in, const complex<T>* twiddle)
{
    const size_t N4 = N / 4;
    cvec<T, width> w1, w2, w3;
    constexpr bool read_split  = !splitin;
    constexpr bool write_split = !splitout;

    vec<T, width> re0, im0, re1, im1, re2, im2, re3, im3;

    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 0), re0, im0);
    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 2), re2, im2);
    const vec<T, width> sum02re = re0 + re2;
    const vec<T, width> sum02im = im0 + im2;

    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 1), re1, im1);
    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 3), re3, im3);
    const vec<T, width> sum13re = re1 + re3;
    const vec<T, width> sum13im = im1 + im3;

    cwrite_split<width, aligned, write_split>(out, concat(sum02re + sum13re, sum02im + sum13im));
    w2 = concat(sum02re - sum13re, sum02im - sum13im);
    cwrite_split<width, aligned, write_split>(
        out + N4 * (use_br2 ? 1 : 2), radix4_apply_twiddle(csize_t<width>(), ctrue, cbool_t<inverse>(), w2,
                                                           cread<width, true>(twiddle + width)));

    const vec<T, width> diff02re = re0 - re2;
    const vec<T, width> diff02im = im0 - im2;
    const vec<T, width> diff13re = re1 - re3;
    const vec<T, width> diff13im = im1 - im3;

    (inverse ? w1 : w3) = concat(diff02re - diff13im, diff02im + diff13re);
    (inverse ? w3 : w1) = concat(diff02re + diff13im, diff02im - diff13re);

    cwrite_split<width, aligned, write_split>(
        out + N4 * (use_br2 ? 2 : 1), radix4_apply_twiddle(csize_t<width>(), ctrue, cbool_t<inverse>(), w1,
                                                           cread<width, true>(twiddle + 0)));
    cwrite_split<width, aligned, write_split>(
        out + N4 * 3, radix4_apply_twiddle(csize_t<width>(), ctrue, cbool_t<inverse>(), w3,
                                           cread<width, true>(twiddle + width * 2)));
}

template <typename T>
CMT_NOINLINE cvec<T, 1> calculate_twiddle(size_t n, size_t size)
{
    if (n == 0)
    {
        return make_vector(static_cast<T>(1), static_cast<T>(0));
    }
    else if (n == size / 4)
    {
        return make_vector(static_cast<T>(0), static_cast<T>(-1));
    }
    else if (n == size / 2)
    {
        return make_vector(static_cast<T>(-1), static_cast<T>(0));
    }
    else if (n == size * 3 / 4)
    {
        return make_vector(static_cast<T>(0), static_cast<T>(1));
    }
    else
    {
        fbase kth  = c_pi<fbase, 2> * (n / static_cast<fbase>(size));
        fbase tcos = +kfr::cos(kth);
        fbase tsin = -kfr::sin(kth);
        return make_vector(static_cast<T>(tcos), static_cast<T>(tsin));
    }
}

template <typename T, size_t width>
KFR_INTRINSIC void initialize_twiddles_impl(complex<T>*& twiddle, size_t nn, size_t nnstep, size_t size,
                                            bool split_format)
{
    static_assert(width > 0, "width cannot be zero");
    vec<T, 2 * width> result = T();
    CMT_LOOP_UNROLL
    for (size_t i = 0; i < width; i++)
    {
        const cvec<T, 1> r = calculate_twiddle<T>(nn + nnstep * i, size);
        result[i * 2]      = r[0];
        result[i * 2 + 1]  = r[1];
    }
    if (split_format)
        ref_cast<cvec<T, width>>(twiddle[0]) = splitpairs(result);
    else
        ref_cast<cvec<T, width>>(twiddle[0]) = result;
    twiddle += width;
}

template <typename T, size_t width>
CMT_NOINLINE void initialize_twiddles(complex<T>*& twiddle, size_t stage_size, size_t size, bool split_format)
{
    static_assert(width > 0, "width cannot be zero");
    const size_t count = stage_size / 4;
    size_t nnstep      = size / stage_size;
    DFT_ASSERT(width <= count);
    CMT_LOOP_NOUNROLL
    for (size_t n = 0; n < count; n += width)
    {
        initialize_twiddles_impl<T, width>(twiddle, n * nnstep * 1, nnstep * 1, size, split_format);
        initialize_twiddles_impl<T, width>(twiddle, n * nnstep * 2, nnstep * 2, size, split_format);
        initialize_twiddles_impl<T, width>(twiddle, n * nnstep * 3, nnstep * 3, size, split_format);
    }
}

#ifdef KFR_NO_PREFETCH
#define KFR_PREFETCH(addr)                                                                                   \
    do                                                                                                       \
    {                                                                                                        \
        (void)(addr);                                                                                        \
    } while (0)
#else

#if defined CMT_ARCH_SSE
#ifdef CMT_COMPILER_GNU
#define KFR_PREFETCH(addr) __builtin_prefetch(::kfr::ptr_cast<void>(addr), 0, _MM_HINT_T0);
#else
#define KFR_PREFETCH(addr) _mm_prefetch(::kfr::ptr_cast<char>(addr), _MM_HINT_T0);
#endif
#else
#define KFR_PREFETCH(addr) __builtin_prefetch(::kfr::ptr_cast<void>(addr));
#endif
#endif

template <size_t size = 1, typename T>
KFR_INTRINSIC void prefetch_one(const complex<T>* in)
{
    KFR_PREFETCH(in);
    if constexpr (sizeof(complex<T>) * size > 64)
        KFR_PREFETCH(in + 64);
    if constexpr (sizeof(complex<T>) * size > 128)
        KFR_PREFETCH(in + 128);
    if constexpr (sizeof(complex<T>) * size > 192)
        KFR_PREFETCH(in + 192);
}

template <size_t size = 1, typename T>
KFR_INTRINSIC void prefetch_four(size_t stride, const complex<T>* in)
{
    prefetch_one<size>(in);
    prefetch_one<size>(in + stride);
    prefetch_one<size>(in + stride * 2);
    prefetch_one<size>(in + stride * 3);
}

template <size_t size = 1, typename T>
KFR_INTRINSIC void prefetch_eight(size_t stride, const complex<T>* in)
{
    prefetch_one<size>(in);
    prefetch_one<size>(in + stride);
    prefetch_one<size>(in + stride * 2);
    prefetch_one<size>(in + stride * 3);
    prefetch_one<size>(in + stride * 4);
    prefetch_one<size>(in + stride * 5);
    prefetch_one<size>(in + stride * 6);
    prefetch_one<size>(in + stride * 7);
}

template <typename Ntype, size_t width, bool splitout, bool splitin, bool prefetch, bool use_br2,
          bool inverse, bool aligned, typename T>
KFR_INTRINSIC cfalse_t radix4_pass(Ntype N, size_t blocks, csize_t<width>, cbool_t<splitout>,
                                   cbool_t<splitin>, cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>,
                                   cbool_t<aligned>, complex<T>* out, const complex<T>* in,
                                   const complex<T>*& twiddle)
{
    static_assert(width > 0, "width cannot be zero");
    constexpr static size_t prefetch_cycles = 8;
    const auto N4                           = N / csize_t<4>();
    const auto N43                          = N4 * csize_t<3>();
    CMT_ASSUME(blocks > 0);
    CMT_ASSUME(N > 0);
    CMT_ASSUME(N4 > 0);
    DFT_ASSERT(width <= N4);
    CMT_LOOP_NOUNROLL for (size_t b = 0; b < blocks; b++)
    {
        CMT_LOOP_NOUNROLL
        for (size_t n2 = 0; n2 < N4;)
        {
            if constexpr (prefetch)
                prefetch_four<width>(N4, in + width * prefetch_cycles);
            radix4_body(N, csize_t<width>(), cbool_t<(splitout || splitin)>(), cbool_t<splitout>(),
                        cbool_t<splitin>(), cbool_t<use_br2>(), cbool_t<inverse>(), cbool_t<aligned>(), out,
                        in, twiddle + n2 * 3);
            in += width;
            out += width;
            n2 += width;
        }
        in += N43;
        out += N43;
    }
    twiddle += N43;
    return {};
}

template <size_t width, bool splitout, bool splitin, bool prefetch, bool use_br2, bool inverse, typename T>
KFR_INTRINSIC void radix2_autosort_pass(size_t N, size_t stride, csize_t<width>, cbool_t<splitout>,
                                        cbool_t<splitin>, cbool_t<use_br2>, cbool_t<prefetch>,
                                        cbool_t<inverse>, complex<T>* out, const complex<T>* in,
                                        const complex<T>*& twiddle)

{
    for (size_t n = 0; n < stride; n++)
    {
        const cvec<T, 1> a = cread<1>(in + n);
        const cvec<T, 1> b = cread<1>(in + n + stride);
        cwrite<1>(out + n, a + b);
        cwrite<1>(out + n + stride, a - b);
    }
}

template <size_t N, bool inverse, bool split_format, typename T>
KFR_INTRINSIC void radix4_butterfly(cvec<T, N> a0, cvec<T, N> a1, cvec<T, N> a2, cvec<T, N> a3,
                                    cvec<T, N>& w0, cvec<T, N>& w1, cvec<T, N>& w2, cvec<T, N>& w3)
{
    if constexpr (split_format)
    {
        const cvec<T, N> sum02  = a0 + a2;
        const cvec<T, N> diff02 = a0 - a2;
        const cvec<T, N> sum13  = a1 + a3;
        cvec<T, N> diff13       = a1 - a3;
        vec<T, N> diff13re, diff13im, diff02re, diff02im;
        split(diff02, diff02re, diff02im);
        split(diff13, diff13re, diff13im);
        w0 = sum02 + sum13;
        w2 = sum02 - sum13;

        (inverse ? w1 : w3) = concat(diff02re - diff13im, diff02im + diff13re);
        (inverse ? w3 : w1) = concat(diff02re + diff13im, diff02im - diff13re);
    }
    else
    {
        const cvec<T, N> sum02  = a0 + a2;
        const cvec<T, N> diff02 = a0 - a2;
        const cvec<T, N> sum13  = a1 + a3;
        cvec<T, N> diff13       = swap<2>(a1 - a3);
        if constexpr (inverse)
            diff13 = negodd(diff13);
        else
            diff13 = negeven(diff13);
        w0 = sum02 + sum13;
        w2 = sum02 - sum13;
        w1 = diff02 - diff13;
        w3 = diff02 + diff13;
    }
}

template <size_t N, bool inverse, bool split_format, typename T>
KFR_INTRINSIC void radix4_butterfly(cvec<T, N> a0, cvec<T, N> a1, cvec<T, N> a2, cvec<T, N> a3,
                                    cvec<T, N>& w0, cvec<T, N>& w1, cvec<T, N>& w2, cvec<T, N>& w3,
                                    cvec<T, N> tw1, cvec<T, N> tw2, cvec<T, N> tw3)
{
    if constexpr (split_format)
    {
        const cvec<T, N> sum02  = a0 + a2;
        const cvec<T, N> diff02 = a0 - a2;
        const cvec<T, N> sum13  = a1 + a3;
        cvec<T, N> diff13       = a1 - a3;
        vec<T, N> diff13re, diff13im, diff02re, diff02im;
        split(diff02, diff02re, diff02im);
        split(diff13, diff13re, diff13im);

        w0                  = sum02 + sum13;
        w2                  = sum02 - sum13;
        (inverse ? w1 : w3) = concat(diff02re - diff13im, diff02im + diff13re);
        (inverse ? w3 : w1) = concat(diff02re + diff13im, diff02im - diff13re);

        w2 = radix4_apply_twiddle(csize<N>, ctrue, cbool<inverse>, w2, tw2);
        w1 = radix4_apply_twiddle(csize<N>, ctrue, cbool<inverse>, w1, tw1);
        w3 = radix4_apply_twiddle(csize<N>, ctrue, cbool<inverse>, w3, tw3);
    }
    else
    {
        const cvec<T, N> sum02  = a0 + a2;
        const cvec<T, N> diff02 = a0 - a2;
        const cvec<T, N> sum13  = a1 + a3;
        cvec<T, N> diff13       = swap<2>(a1 - a3);
        if constexpr (inverse)
            diff13 = negodd(diff13);
        else
            diff13 = negeven(diff13);

        w0 = sum02 + sum13;
        w2 = sum02 - sum13;
        w1 = diff02 - diff13;
        w3 = diff02 + diff13;
        w2 = radix4_apply_twiddle(csize<N>, cfalse, cbool<inverse>, w2, tw2);
        w1 = radix4_apply_twiddle(csize<N>, cfalse, cbool<inverse>, w1, tw1);
        w3 = radix4_apply_twiddle(csize<N>, cfalse, cbool<inverse>, w3, tw3);
    }
}

template <size_t N, bool split_format, typename T>
KFR_INTRINSIC cvec<T, N> read_twiddle(const complex<T>* tw)
{
    if constexpr (split_format)
    {
        return concat(repeat<N>(read(cunaligned, csize<1>, ptr_cast<T>(tw))),
                      repeat<N>(read(cunaligned, csize<1>, ptr_cast<T>(tw) + 1)));
    }
    else
    {
        return repeat<N>(cread<1>(tw));
    }
}

template <size_t width_, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void radix4_autosort_pass_first(size_t N, csize_t<width_>, cbool_t<splitout>, cbool_t<splitin>,
                                              cbool_t<prefetch>, cbool_t<inverse>, complex<T>* out,
                                              const complex<T>* in, const complex<T>*& twiddle)
{
    static_assert(width_ > 0, "width cannot be zero");
    const size_t N4        = N / 4;
    const size_t Nstride   = N4;
    constexpr size_t width = width_;
    constexpr bool split   = splitin || splitout;
    static_assert(!split);
    constexpr static size_t prefetch_cycles = 8;

    // CMT_LOOP_NOUNROLL
    for (size_t b = 0; b < N4; b += width)
    {
        if constexpr (prefetch)
            prefetch_four<width>(Nstride, in + prefetch_cycles * width);
        cvec<T, width> tw1 = cread<width>(twiddle);
        cvec<T, width> tw2 = cread<width>(twiddle + width);
        cvec<T, width> tw3 = cread<width>(twiddle + 2 * width);

        const cvec<T, width> a0 = cread<width>(in + 0 * Nstride);
        const cvec<T, width> a1 = cread<width>(in + 1 * Nstride);
        const cvec<T, width> a2 = cread<width>(in + 2 * Nstride);
        const cvec<T, width> a3 = cread<width>(in + 3 * Nstride);
        cvec<T, width> w0, w1, w2, w3;
        radix4_butterfly<width, inverse, split>(a0, a1, a2, a3, w0, w1, w2, w3, tw1, tw2, tw3);
        cvec<T, 4 * width> w0123 = concat(w0, w1, w2, w3);
        w0123                    = ctranspose<width>(w0123);
        cwrite<4 * width>(out, w0123);
        twiddle += 3 * width;
        in += width;
        out += 4 * width;
    }
}

template <size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void radix4_autosort_pass_last(size_t stride, csize_t<width>, cbool_t<splitout>,
                                             cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>,
                                             complex<T>* out, const complex<T>* in,
                                             const complex<T>*& twiddle)
{
    static_assert(width > 0, "width cannot be zero");
    constexpr static size_t prefetch_cycles = 8;
    constexpr bool split                    = splitin || splitout;
    constexpr bool read_split               = !splitin && split;
    constexpr bool write_split              = !splitout && split;
    static_assert(!splitout);

    CMT_PRAGMA_CLANG(clang loop unroll_count(4))
    for (size_t n = 0; n < stride; n += width)
    {
        if constexpr (prefetch)
            prefetch_four<width>(stride, in + prefetch_cycles * width);
        const cvec<T, width> a0 = cread_split<width, false, read_split>(in + 0 * stride);
        const cvec<T, width> a1 = cread_split<width, false, read_split>(in + 1 * stride);
        const cvec<T, width> a2 = cread_split<width, false, read_split>(in + 2 * stride);
        const cvec<T, width> a3 = cread_split<width, false, read_split>(in + 3 * stride);
        cvec<T, width> w0, w1, w2, w3;
        radix4_butterfly<width, inverse, split>(a0, a1, a2, a3, w0, w1, w2, w3);
        cwrite_split<width, false, write_split>(out + 0 * stride, w0);
        cwrite_split<width, false, write_split>(out + 1 * stride, w1);
        cwrite_split<width, false, write_split>(out + 2 * stride, w2);
        cwrite_split<width, false, write_split>(out + 3 * stride, w3);
        in += width;
        out += width;
    }
}

template <size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void radix8_autosort_pass_last(size_t stride, csize_t<width>, cbool_t<splitout>,
                                             cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>,
                                             complex<T>* out, const complex<T>* in,
                                             const complex<T>*& twiddle)
{
    static_assert(width > 0, "width cannot be zero");
    constexpr static size_t prefetch_cycles = 4;
    constexpr bool split                    = splitin || splitout;
    constexpr bool read_split               = !splitin && split;
    constexpr bool write_split              = !splitout && split;
    static_assert(!splitout);

    CMT_PRAGMA_CLANG(clang loop unroll_count(4))
    for (size_t n = 0; n < stride; n += width)
    {
        if constexpr (prefetch)
            prefetch_eight<width>(stride, in + prefetch_cycles * width);
        const cvec<T, width> a0 = cread_split<width, false, read_split>(in + 0 * stride);
        const cvec<T, width> a1 = cread_split<width, false, read_split>(in + 1 * stride);
        const cvec<T, width> a2 = cread_split<width, false, read_split>(in + 2 * stride);
        const cvec<T, width> a3 = cread_split<width, false, read_split>(in + 3 * stride);
        const cvec<T, width> a4 = cread_split<width, false, read_split>(in + 4 * stride);
        const cvec<T, width> a5 = cread_split<width, false, read_split>(in + 5 * stride);
        const cvec<T, width> a6 = cread_split<width, false, read_split>(in + 6 * stride);
        const cvec<T, width> a7 = cread_split<width, false, read_split>(in + 7 * stride);
        cvec<T, width> w0, w1, w2, w3, w4, w5, w6, w7;
        butterfly8<width, inverse>(a0, a1, a2, a3, a4, a5, a6, a7, w0, w1, w2, w3, w4, w5, w6, w7);
        cwrite_split<width, false, write_split>(out + 0 * stride, w0);
        cwrite_split<width, false, write_split>(out + 1 * stride, w1);
        cwrite_split<width, false, write_split>(out + 2 * stride, w2);
        cwrite_split<width, false, write_split>(out + 3 * stride, w3);
        cwrite_split<width, false, write_split>(out + 4 * stride, w4);
        cwrite_split<width, false, write_split>(out + 5 * stride, w5);
        cwrite_split<width, false, write_split>(out + 6 * stride, w6);
        cwrite_split<width, false, write_split>(out + 7 * stride, w7);
        in += width;
        out += width;
    }
}

template <size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void radix4_autosort_pass(size_t N, size_t stride, csize_t<width>, cbool_t<splitout>,
                                        cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>,
                                        complex<T>* out, const complex<T>* in, const complex<T>*& twiddle)
{
    static_assert(width > 0, "width cannot be zero");
    constexpr static size_t prefetch_cycles = 8;
    const size_t N4                         = N / 4;
    const size_t Nstride                    = stride * N4;
    const size_t stride3                    = 3 * stride;
    constexpr bool split                    = splitin || splitout;
    constexpr bool read_split               = !splitin && split;
    constexpr bool write_split              = !splitout && split;

    {
        for (size_t n = 0; n < stride; n += width)
        {
            if constexpr (prefetch)
                prefetch_four<width>(Nstride, in + prefetch_cycles * width);
            const cvec<T, width> a0 = cread_split<width, false, read_split>(in + 0 * Nstride);
            const cvec<T, width> a1 = cread_split<width, false, read_split>(in + 1 * Nstride);
            const cvec<T, width> a2 = cread_split<width, false, read_split>(in + 2 * Nstride);
            const cvec<T, width> a3 = cread_split<width, false, read_split>(in + 3 * Nstride);
            cvec<T, width> w0, w1, w2, w3;
            radix4_butterfly<width, inverse, split>(a0, a1, a2, a3, w0, w1, w2, w3);
            cwrite_split<width, false, write_split>(out + 0 * stride, w0);
            cwrite_split<width, false, write_split>(out + 1 * stride, w1);
            cwrite_split<width, false, write_split>(out + 2 * stride, w2);
            cwrite_split<width, false, write_split>(out + 3 * stride, w3);
            in += width;
            out += width;
        }
        twiddle += 3;
        out += stride3;
    }

    // CMT_LOOP_NOUNROLL
    for (size_t b = 1; b < N4; b++)
    {
        cvec<T, width> tw1 = read_twiddle<width, split>(twiddle);
        cvec<T, width> tw2 = read_twiddle<width, split>(twiddle + 1);
        cvec<T, width> tw3 = read_twiddle<width, split>(twiddle + 2);
        for (size_t n = 0; n < stride; n += width)
        {
            if constexpr (prefetch)
                prefetch_four<width>(Nstride, in + prefetch_cycles * width);
            const cvec<T, width> a0 = cread_split<width, false, read_split>(in + 0 * Nstride);
            const cvec<T, width> a1 = cread_split<width, false, read_split>(in + 1 * Nstride);
            const cvec<T, width> a2 = cread_split<width, false, read_split>(in + 2 * Nstride);
            const cvec<T, width> a3 = cread_split<width, false, read_split>(in + 3 * Nstride);
            cvec<T, width> w0, w1, w2, w3;
            radix4_butterfly<width, inverse, split>(a0, a1, a2, a3, w0, w1, w2, w3, tw1, tw2, tw3);
            cwrite_split<width, false, write_split>(out + 0 * stride, w0);
            cwrite_split<width, false, write_split>(out + 1 * stride, w1);
            cwrite_split<width, false, write_split>(out + 2 * stride, w2);
            cwrite_split<width, false, write_split>(out + 3 * stride, w3);
            in += width;
            out += width;
        }
        twiddle += 3;
        out += stride3;
    }
}

template <typename T>
static void initialize_twiddle_autosort(size_t N, size_t w, complex<T>*& twiddle)
{
    for (size_t b = 0; b < N / 4; ++b)
    {
        cwrite<1>(twiddle + b / w * 3 * w + b % w + 0 * w, calculate_twiddle<T>(b, N));
        cwrite<1>(twiddle + b / w * 3 * w + b % w + 1 * w, calculate_twiddle<T>(2 * b, N));
        cwrite<1>(twiddle + b / w * 3 * w + b % w + 2 * w, calculate_twiddle<T>(3 * b, N));
    }
    twiddle += N / 4 * 3;
}

template <typename T>
KFR_INTRINSIC void interleavehalves32(cvec<T, 32>& v0)
{
    cvec<T, 8> t0, t1, t2, t3;
    split(v0, t0, t1, t2, t3);
    t0 = interleavehalves(t0);
    t1 = interleavehalves(t1);
    t2 = interleavehalves(t2);
    t3 = interleavehalves(t3);
    v0 = concat(t0, t1, t2, t3);
}

template <size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<8>, size_t blocks, csize_t<width>, cfalse_t, cfalse_t,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    CMT_ASSUME(blocks > 0);
    DFT_ASSERT(4 <= blocks);
    constexpr static size_t prefetch_cycles = 8;
    if constexpr (vector_capacity<T> >= 128)
    {
        CMT_PRAGMA_CLANG(clang loop unroll(disable))
        for (size_t b = 0; b < blocks; b += 4)
        {
            if constexpr (prefetch)
                prefetch_one<32>(out + prefetch_cycles * 32);

            cvec<T, 32> v32 = cread<32, aligned>(out);
            cvec<T, 4> v0, v1, v2, v3, v4, v5, v6, v7;
            v32 = ctranspose<8>(v32);
            split(v32, v0, v1, v2, v3, v4, v5, v6, v7);
            butterfly8<4, inverse>(v0, v1, v2, v3, v4, v5, v6, v7);
            v32 = concat(v0, v4, v2, v6, v1, v5, v3, v7);
            v32 = ctranspose<4>(v32);
            cwrite<32, aligned>(out, v32);

            out += 32;
        }
    }
    else
    {
        CMT_PRAGMA_CLANG(clang loop unroll(disable))
        for (size_t b = 0; b < blocks; b += 2)
        {
            if constexpr (prefetch)
                prefetch_one<16>(out + prefetch_cycles * 16);

            cvec<T, 16> v16 = cread<16, aligned>(out);
            cvec<T, 2> v0, v1, v2, v3, v4, v5, v6, v7;
            v16 = ctranspose<8>(v16);
            split(v16, v0, v1, v2, v3, v4, v5, v6, v7);
            butterfly8<2, inverse>(v0, v1, v2, v3, v4, v5, v6, v7);
            v16 = concat(v0, v4, v2, v6, v1, v5, v3, v7);
            v16 = ctranspose<2>(v16);
            cwrite<16, aligned>(out, v16);

            out += 16;
        }
    }
    return {};
}

template <size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<16>, size_t blocks, csize_t<width>, cfalse_t, cfalse_t,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    CMT_ASSUME(blocks > 0);
    constexpr static size_t prefetch_cycles = 4;
    DFT_ASSERT(4 <= blocks);
    if constexpr (vector_capacity<T> >= 128)
    {
        CMT_PRAGMA_CLANG(clang loop unroll(disable))
        for (size_t b = 0; b < blocks; b += 4)
        {
            if constexpr (prefetch)
                prefetch_one<64>(out + prefetch_cycles * 64);

            cvec<T, 16> v0 = cread<16, aligned>(out);
            cvec<T, 16> v1 = cread<16, aligned>(out + 16);
            cvec<T, 16> v2 = cread<16, aligned>(out + 32);
            cvec<T, 16> v3 = cread<16, aligned>(out + 48);
            butterfly4_packed<4, inverse>(v0);
            butterfly4_packed<4, inverse>(v1);
            butterfly4_packed<4, inverse>(v2);
            butterfly4_packed<4, inverse>(v3);
            apply_twiddles4<0, 4, 4, inverse>(v0);
            apply_twiddles4<0, 4, 4, inverse>(v1);
            apply_twiddles4<0, 4, 4, inverse>(v2);
            apply_twiddles4<0, 4, 4, inverse>(v3);
            v0 = digitreverse4<2>(v0);
            v1 = digitreverse4<2>(v1);
            v2 = digitreverse4<2>(v2);
            v3 = digitreverse4<2>(v3);
            butterfly4_packed<4, inverse>(v0);
            butterfly4_packed<4, inverse>(v1);
            butterfly4_packed<4, inverse>(v2);
            butterfly4_packed<4, inverse>(v3);

            use_br2 ? cbitreverse_write(out, v0) : cdigitreverse4_write(out, v0);
            use_br2 ? cbitreverse_write(out + 16, v1) : cdigitreverse4_write(out + 16, v1);
            use_br2 ? cbitreverse_write(out + 32, v2) : cdigitreverse4_write(out + 32, v2);
            use_br2 ? cbitreverse_write(out + 48, v3) : cdigitreverse4_write(out + 48, v3);
            out += 64;
        }
    }
    else
    {
        CMT_PRAGMA_CLANG(clang loop unroll(disable))
        for (size_t b = 0; b < blocks; b += 2)
        {
            if constexpr (prefetch)
                prefetch_one<32>(out + prefetch_cycles * 32);

            cvec<T, 16> vlo = cread<16, aligned>(out);
            cvec<T, 16> vhi = cread<16, aligned>(out + 16);
            butterfly4_packed<4, inverse>(vlo);
            butterfly4_packed<4, inverse>(vhi);
            apply_twiddles4<0, 4, 4, inverse>(vlo);
            apply_twiddles4<0, 4, 4, inverse>(vhi);
            vlo = digitreverse4<2>(vlo);
            vhi = digitreverse4<2>(vhi);
            butterfly4_packed<4, inverse>(vlo);
            butterfly4_packed<4, inverse>(vhi);

            use_br2 ? cbitreverse_write(out, vlo) : cdigitreverse4_write(out, vlo);
            use_br2 ? cbitreverse_write(out + 16, vhi) : cdigitreverse4_write(out + 16, vhi);
            out += 32;
        }
    }
    return {};
}

template <size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<4>, size_t blocks, csize_t<width>, cfalse_t, cfalse_t,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    constexpr static size_t prefetch_cycles = 8;
    CMT_ASSUME(blocks > 8);
    DFT_ASSERT(8 <= blocks);
    for (size_t b = 0; b < blocks; b += 4)
    {
        if constexpr (prefetch)
            prefetch_one<16>(out + prefetch_cycles * 16);

        cvec<T, 16> v16 = cdigitreverse4_read<16, aligned>(out);
        butterfly4_packed<4, inverse>(v16);
        if constexpr (use_br2)
            v16 = permutegroups<(8), 0, 2, 1, 3>(v16);
        cdigitreverse4_write<aligned>(out, v16);

        out += 4 * 4;
    }
    return {};
}

template <typename T>
struct fft_config
{
    constexpr static inline const bool recursion = true;
    constexpr static inline const bool prefetch  = true;
    constexpr static inline const size_t process_width =
        const_max(static_cast<size_t>(1), vector_capacity<T> / 16);
};

template <typename T, bool splitin, bool is_even>
struct fft_stage_impl : dft_stage<T>
{
    fft_stage_impl(size_t stage_size)
    {
        this->name       = dft_name(this);
        this->radix      = 4;
        this->stage_size = stage_size;
        this->repeats    = 4;
        this->recursion  = fft_config<T>::recursion;
        this->data_size =
            align_up(sizeof(complex<T>) * stage_size / 4 * 3, platform<>::native_cache_alignment);
    }

    constexpr static bool prefetch = fft_config<T>::prefetch;
    constexpr static bool aligned  = false;
    constexpr static size_t width  = fft_config<T>::process_width;
    constexpr static bool use_br2  = !is_even || always_br2;

    virtual void do_initialize(size_t size) override final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddles<T, width>(twiddle, this->stage_size, size, true);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        if constexpr (splitin)
            in = out;
        const size_t stg_size = this->stage_size;
        CMT_ASSUME(stg_size >= 2048);
        CMT_ASSUME(stg_size % 2048 == 0);
        radix4_pass(stg_size, 1, csize_t<width>(), ctrue, cbool_t<splitin>(), cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, in, twiddle);
    }
};

template <typename T, bool splitin, size_t size>
struct fft_final_stage_impl : dft_stage<T>
{
    fft_final_stage_impl(size_t)
    {
        this->name       = dft_name(this);
        this->radix      = size;
        this->stage_size = size;
        this->out_offset = size;
        this->repeats    = 4;
        this->recursion  = fft_config<T>::recursion;
        this->data_size  = align_up(sizeof(complex<T>) * size * 3 / 2, platform<>::native_cache_alignment);
    }

    constexpr static size_t width  = fft_config<T>::process_width;
    constexpr static bool is_even  = cometa::is_even(ilog2(size));
    constexpr static bool use_br2  = !is_even || always_br2;
    constexpr static bool aligned  = false;
    constexpr static bool prefetch = fft_config<T>::prefetch && splitin;

    template <bool pass_splitin>
    KFR_MEM_INTRINSIC void init_twiddles(csize_t<8>, size_t, cbool_t<pass_splitin>, complex<T>*&)
    {
    }
    template <bool pass_splitin>
    KFR_MEM_INTRINSIC void init_twiddles(csize_t<4>, size_t, cbool_t<pass_splitin>, complex<T>*&)
    {
    }

    static constexpr bool get_pass_splitout(size_t N) { return N / 4 > 8 && N / 4 / 4 >= width; }

    template <size_t N, bool pass_splitin>
    KFR_MEM_INTRINSIC void init_twiddles(csize_t<N>, size_t total_size, cbool_t<pass_splitin>,
                                         complex<T>*& twiddle)
    {
        constexpr bool pass_splitout = get_pass_splitout(N);
        constexpr size_t pass_width  = const_min(width, N / 4);
        initialize_twiddles<T, pass_width>(twiddle, N, total_size, pass_splitout || pass_splitin);
        init_twiddles(csize<N / 4>, total_size, cbool<pass_splitout>, twiddle);
    }

    virtual void do_initialize(size_t total_size) override final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        init_twiddles(csize<size>, total_size, cbool<splitin>, twiddle);
    }

    DFT_STAGE_FN_NONFINAL
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        final_stage<inverse>(csize<size>, 1, cbool<splitin>, out, in, twiddle);
    }

    template <bool inverse, bool pass_splitin, typename U = T, KFR_ENABLE_IF(vector_capacity<U> >= 128)>
    KFR_MEM_INTRINSIC void final_stage(csize_t<16>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>*, const complex<T>*& twiddle)
    {
        radix4_pass(csize_t<16>(), invN, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
    }

    template <bool inverse, bool pass_splitin>
    KFR_MEM_INTRINSIC void final_stage(csize_t<8>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>*, const complex<T>*& twiddle)
    {
        radix4_pass(csize_t<8>(), invN, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
    }

    template <bool inverse, bool pass_splitin>
    KFR_MEM_INTRINSIC void final_stage(csize_t<4>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>*, const complex<T>*& twiddle)
    {
        radix4_pass(csize_t<4>(), invN, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
    }

    template <bool inverse, size_t N, bool pass_splitin>
    KFR_MEM_INTRINSIC void final_stage(csize_t<N>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>* in, const complex<T>*& twiddle)
    {
        static_assert(N > 8, "");
        constexpr bool pass_splitout = get_pass_splitout(N);
        constexpr size_t pass_width  = const_min(width, N / 4);
        static_assert(pass_width == width || !pass_splitin, "");
        static_assert(pass_width <= N / 4, "");
        radix4_pass(N, invN, csize_t<pass_width>(), cbool<pass_splitout>, cbool_t<pass_splitin>(),
                    cbool_t<use_br2>(), cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, in,
                    twiddle);
        final_stage<inverse>(csize<N / 4>, invN * 4, cbool<pass_splitout>, out, out, twiddle);
    }
};

template <typename T, bool is_even>
struct fft_reorder_stage_impl : dft_stage<T>
{
    fft_reorder_stage_impl(size_t stage_size)
    {
        this->name       = dft_name(this);
        this->stage_size = stage_size;
        this->user       = ilog2(stage_size);
        this->data_size  = 0;
    }

    virtual void do_initialize(size_t) override final {}

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>*, u8*)
    {
        fft_reorder(out, this->user, cbool_t<(!is_even || always_br2)>());
    }
};

template <typename T, bool is_first, bool is_last, bool radix8>
struct fft_autosort_stage_impl : dft_stage<T>
{
    fft_autosort_stage_impl(size_t stage_size, size_t stride)
    {
        this->name         = dft_name(this);
        this->radix        = radix8 ? 8 : 4;
        this->stage_size   = stage_size * stride * this->radix;
        this->blocks       = stage_size;
        this->recursion    = false;
        this->can_inplace  = is_last;
        this->need_reorder = false;
        this->user         = stride;
        if constexpr (!is_last)
        {
            this->data_size =
                align_up(sizeof(complex<T>) * stage_size / 4 * 3, platform<>::native_cache_alignment);
        }
    }

    constexpr static bool prefetch = fft_config<T>::prefetch;
    constexpr static bool aligned  = false;

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t total_size) final
    {
        if constexpr (!is_last)
        {
            complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
            if constexpr (is_first)
                initialize_twiddle_autosort(this->blocks, width, twiddle);
            else
                initialize_twiddle_autosort(this->blocks, 1, twiddle);
        }
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        const size_t stg_size     = this->blocks;
        const size_t stride       = this->user;
        if constexpr (is_first)
        {
            radix4_autosort_pass_first(stg_size, csize_t<width>(), cfalse, cfalse, cbool_t<prefetch>(),
                                       cbool_t<inverse>(), out, in, twiddle);
        }
        else if constexpr (is_last)
        {
            if constexpr (radix8)
                radix8_autosort_pass_last(stride, csize_t<width / 2>(), cfalse, cfalse, cbool_t<prefetch>(),
                                          cbool_t<inverse>(), out, in, twiddle);
            else
                radix4_autosort_pass_last(stride, csize_t<width>(), cfalse, cfalse, cbool_t<prefetch>(),
                                          cbool_t<inverse>(), out, in, twiddle);
        }
        else
        {
            if (stride == 4)
                radix4_autosort_pass(stg_size, stride, csize_t<4>(), cfalse, cfalse, cbool_t<prefetch>(),
                                     cbool_t<inverse>(), out, in, twiddle);
            else
                radix4_autosort_pass(stg_size, stride, csize_t<width>(), cfalse, cfalse, cbool_t<prefetch>(),
                                     cbool_t<inverse>(), out, in, twiddle);
        }
    }
};

template <typename T, size_t log2n>
struct fft_specialization;

template <typename T>
struct fft_specialization<T, 0> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 1;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN

    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        out[0] = in[0];
    }
};

template <typename T>
struct fft_specialization<T, 1> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 2;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN

    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 1> a0, a1;
        split<T, 4>(cread<2, aligned>(in), a0, a1);
        cwrite<2, aligned>(out, concat(a0 + a1, a0 - a1));
    }
};

template <typename T>
struct fft_specialization<T, 2> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 4;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 1> a0, a1, a2, a3;
        split<T, 8>(cread<4>(in), a0, a1, a2, a3);
        butterfly(cbool_t<inverse>(), a0, a1, a2, a3, a0, a1, a2, a3);
        cwrite<4>(out, concat(concat(a0, a1), concat(a2, a3)));
    }
};

template <typename T>
struct fft_specialization<T, 3> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 8;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 8> v8 = cread<8, aligned>(in);
        butterfly8_packed<inverse>(v8);
        cwrite<8, aligned>(out, v8);
    }
};

template <typename T>
struct fft_specialization<T, 4> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 16;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 16> v16 = cread<16, aligned>(in);
        butterfly16_packed<inverse>(v16);
        cwrite<16, aligned>(out, v16);
    }
};

template <typename T>
struct fft_specialization<T, 5> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 32;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 32> v32 = cread<32, aligned>(in);
        butterfly32_packed<inverse>(v32);
        cwrite<32, aligned>(out, v32);
    }
};

#ifdef KFR_AUTOSORT_FOR_64
template <typename T>
struct fft_specialization<T, 6> : dft_stage<T>
{
    fft_specialization(size_t stage_size)
    {
        this->stage_size = 64;
        this->name       = dft_name(this);
        this->temp_size  = 64 * sizeof(complex<T>);
        this->data_size  = 64 * sizeof(complex<T>);
    }

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort(64, width, twiddle);
        initialize_twiddle_autosort(16, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        radix4_autosort_pass_first(64, csize<width>, no, no, no, cbool<inverse>, scratch, in, tw);
        radix4_autosort_pass(16, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        radix4_autosort_pass_last(16, csize<width>, no, no, no, cbool<inverse>, out, out, tw);
    }
};
#else
template <typename T>
struct fft_specialization<T, 6> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 64;
        this->name       = dft_name(this);
    }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        butterfly64_memory(cbool_t<inverse>(), cbool_t<aligned>(), out, in);
    }
};
#endif

#ifdef KFR_AUTOSORT_FOR_128D
template <>
struct fft_specialization<double, 7> : dft_stage<double>
{
    using T = double;
    fft_specialization(size_t stage_size)
    {
        this->stage_size = 128;
        this->name       = dft_name(this);
        this->temp_size  = 128 * sizeof(complex<T>);
        this->data_size  = 128 * sizeof(complex<T>);
    }

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort(128, width, twiddle);
        initialize_twiddle_autosort(32, 1, twiddle);
        initialize_twiddle_autosort(8, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        radix4_autosort_pass_first(128, csize<width>, no, no, no, cbool<inverse>, scratch, in, tw);
        radix4_autosort_pass(32, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        radix8_autosort_pass_last(16, csize<width>, no, no, no, cbool<inverse>, out, out, tw);
    }
};
#else
template <>
struct fft_specialization<double, 7> : dft_stage<double>
{
    using T = double;
    fft_specialization(size_t)
    {
        this->name       = dft_name(this);
        this->stage_size = 128;
        this->data_size  = align_up(sizeof(complex<T>) * 128 * 3 / 2, platform<>::native_cache_alignment);
    }

    constexpr static bool aligned        = false;
    constexpr static size_t width        = const_min(fft_config<T>::process_width, size_t(8));
    constexpr static bool use_br2        = true;
    constexpr static bool prefetch       = false;
    constexpr static size_t split_format = true;

    virtual void do_initialize(size_t total_size) override final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddles<T, width>(twiddle, 128, total_size, split_format);
        initialize_twiddles<T, width>(twiddle, 32, total_size, split_format);
        initialize_twiddles<T, width>(twiddle, 8, total_size, split_format);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        radix4_pass(128, 1, csize_t<width>(), ctrue, cfalse, cbool_t<use_br2>(), cbool_t<prefetch>(),
                    cbool_t<inverse>(), cbool_t<aligned>(), out, in, twiddle);
        radix4_pass(32, 4, csize_t<width>(), cfalse, ctrue, cbool_t<use_br2>(), cbool_t<prefetch>(),
                    cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
        radix4_pass(csize_t<8>(), 16, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
        if (this->need_reorder)
            fft_reorder(out, csize_t<7>());
    }
};
#endif

template <>
struct fft_specialization<float, 7> : dft_stage<float>
{
    using T = float;
    fft_specialization(size_t)
    {
        this->name       = dft_name(this);
        this->stage_size = 128;
        this->data_size  = align_up(sizeof(complex<T>) * 128 * 3 / 2, platform<>::native_cache_alignment);
    }

    constexpr static bool aligned        = false;
    constexpr static size_t width1       = fft_config<T>::process_width;
    constexpr static size_t width2       = const_min(width1, size_t(8));
    constexpr static bool use_br2        = true;
    constexpr static bool prefetch       = false;
    constexpr static size_t final_size   = 32;
    constexpr static size_t split_format = false;

    virtual void do_initialize(size_t total_size) override final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddles<T, width1>(twiddle, 128, total_size, split_format);
        initialize_twiddles<T, width2>(twiddle, 32, total_size, split_format);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        const complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        radix4_pass(128, 1, csize_t<width1>(), cfalse, cfalse, cbool_t<use_br2>(), cbool_t<prefetch>(),
                    cbool_t<inverse>(), cbool_t<aligned>(), out, in, twiddle);
        radix4_pass(32, 4, csize_t<width2>(), cfalse, cfalse, cbool_t<use_br2>(), cbool_t<prefetch>(),
                    cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
        radix4_pass(csize_t<8>(), 16, csize_t<width2>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
        if (this->need_reorder)
            fft_reorder(out, csize_t<7>());
    }
};

template <>
struct fft_specialization<float, 8> : dft_stage<float>
{
    fft_specialization(size_t)
    {
        this->stage_size = 256;
        this->name       = dft_name(this);
        this->temp_size  = sizeof(complex<float>) * 256;
    }

    using T = float;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        complex<float>* scratch = ptr_cast<complex<float>>(temp);
        if (out == in)
        {
            butterfly16_multi_flip<0, inverse>(scratch, out);
            butterfly16_multi_flip<1, inverse>(scratch, out);
            butterfly16_multi_flip<2, inverse>(scratch, out);
            butterfly16_multi_flip<3, inverse>(scratch, out);

            butterfly16_multi_natural<0, inverse>(out, scratch);
            butterfly16_multi_natural<1, inverse>(out, scratch);
            butterfly16_multi_natural<2, inverse>(out, scratch);
            butterfly16_multi_natural<3, inverse>(out, scratch);
        }
        else
        {
            butterfly16_multi_flip<0, inverse>(out, in);
            butterfly16_multi_flip<1, inverse>(out, in);
            butterfly16_multi_flip<2, inverse>(out, in);
            butterfly16_multi_flip<3, inverse>(out, in);

            butterfly16_multi_natural<0, inverse>(out, out);
            butterfly16_multi_natural<1, inverse>(out, out);
            butterfly16_multi_natural<2, inverse>(out, out);
            butterfly16_multi_natural<3, inverse>(out, out);
        }
    }
};

#ifdef KFR_AUTOSORT_FOR_256D

template <>
struct fft_specialization<double, 8> : dft_stage<double>
{
    using T = double;
    fft_specialization(size_t stage_size)
    {
        this->stage_size = 256;
        this->name       = dft_name(this);
        this->temp_size  = 256 * sizeof(complex<T>);
        this->data_size  = 256 * sizeof(complex<T>);
    }

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort(256, width, twiddle);
        initialize_twiddle_autosort(64, 1, twiddle);
        initialize_twiddle_autosort(16, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        if (in != out)
        {
            radix4_autosort_pass_first(256, csize<width>, no, no, no, cbool<inverse>, out, in, tw);
            radix4_autosort_pass(64, 4, csize<4>, no, no, no, cbool<inverse>, scratch, out, tw);
            radix4_autosort_pass(16, 16, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
            radix4_autosort_pass_last(64, csize<width>, no, no, no, cbool<inverse>, out, out, tw);
        }
        else
        {
            radix4_autosort_pass_first(256, csize<width>, no, no, no, cbool<inverse>, scratch, in, tw);
            radix4_autosort_pass(64, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
            radix4_autosort_pass(16, 16, csize<width>, no, no, no, cbool<inverse>, scratch, out, tw);
            radix4_autosort_pass_last(64, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
        }
    }
};
#else
template <>
struct fft_specialization<double, 8> : fft_final_stage_impl<double, false, 256>
{
    using T = double;
    fft_specialization(size_t stage_size) : fft_final_stage_impl<double, false, 256>(stage_size)
    {
        this->stage_size = 256;
        this->name       = dft_name(this);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        fft_final_stage_impl<double, false, 256>::template do_execute<inverse>(out, in, nullptr);
        if (this->need_reorder)
            fft_reorder(out, csize_t<8>(), cbool<always_br2>);
    }
};
#endif

#ifdef KFR_AUTOSORT_FOR_512

template <typename T>
struct fft_specialization<T, 9> : dft_stage<T>
{
    fft_specialization(size_t stage_size)
    {
        this->stage_size = 512;
        this->name       = dft_name(this);
        this->temp_size  = 512 * sizeof(complex<T>);
        this->data_size  = 512 * sizeof(complex<T>);
    }

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort(512, width, twiddle);
        initialize_twiddle_autosort(128, 1, twiddle);
        initialize_twiddle_autosort(32, 1, twiddle);
        initialize_twiddle_autosort(8, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        radix4_autosort_pass_first(512, csize<width>, no, no, no, cbool<inverse>, scratch, in, tw);
        radix4_autosort_pass(128, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        radix4_autosort_pass(32, 16, csize<width>, no, no, no, cbool<inverse>, scratch, out, tw);
        radix8_autosort_pass_last(64, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
    }
};
#else
template <typename T>
struct fft_specialization<T, 9> : fft_final_stage_impl<T, false, 512>
{
    fft_specialization(size_t stage_size) : fft_final_stage_impl<T, false, 512>(stage_size)
    {
        this->stage_size = 512;
        this->name       = dft_name(this);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        fft_final_stage_impl<T, false, 512>::template do_execute<inverse>(out, in, nullptr);
        if (this->need_reorder)
            fft_reorder(out, csize_t<9>());
    }
};
#endif

#ifdef KFR_AUTOSORT_FOR_1024
template <typename T>
struct fft_specialization<T, 10> : dft_stage<T>
{
    fft_specialization(size_t stage_size)
    {
        this->stage_size = 1024;
        this->name       = dft_name(this);
        this->temp_size  = 1024 * sizeof(complex<T>);
        this->data_size  = 1024 * sizeof(complex<T>);
    }

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort(1024, width, twiddle);
        initialize_twiddle_autosort(256, 1, twiddle);
        initialize_twiddle_autosort(64, 1, twiddle);
        initialize_twiddle_autosort(16, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        auto split           = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        radix4_autosort_pass_first(1024, csize<width>, no, no, no, cbool<inverse>, scratch, in, tw);
        radix4_autosort_pass(256, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        radix4_autosort_pass(64, 16, csize<width>, split, no, no, cbool<inverse>, scratch, out, tw);
        radix4_autosort_pass(16, 64, csize<width>, split, split, no, cbool<inverse>, out, scratch, tw);
        radix4_autosort_pass_last(256, csize<width>, no, split, no, cbool<inverse>, out, out, tw);
    }
};
#else
template <typename T>
struct fft_specialization<T, 10> : fft_final_stage_impl<T, false, 1024>
{
    fft_specialization(size_t stage_size) : fft_final_stage_impl<T, false, 1024>(stage_size)
    {
        this->name = dft_name(this);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        fft_final_stage_impl<T, false, 1024>::template do_execute<inverse>(out, in, nullptr);
        if (this->need_reorder)
            fft_reorder(out, 10, cfalse);
    }
};
#endif

#ifdef KFR_AUTOSORT_FOR_2048
template <typename T>
struct fft_specialization<T, 11> : dft_stage<T>
{
    fft_specialization(size_t stage_size)
    {
        this->stage_size = 2048;
        this->name       = dft_name(this);
        this->temp_size  = 2048 * sizeof(complex<T>);
        this->data_size  = 2048 * sizeof(complex<T>);
    }

    constexpr static size_t width = const_min(16, const_max(4, fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort(2048, width, twiddle);
        initialize_twiddle_autosort(512, 1, twiddle);
        initialize_twiddle_autosort(128, 1, twiddle);
        initialize_twiddle_autosort(32, 1, twiddle);
        initialize_twiddle_autosort(8, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        radix4_autosort_pass_first(2048, csize<width>, no, no, no, cbool<inverse>, scratch, in, tw);
        radix4_autosort_pass(512, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        radix4_autosort_pass(128, 16, csize<4>, no, no, no, cbool<inverse>, scratch, out, tw);
        radix4_autosort_pass(32, 64, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
        radix8_autosort_pass_last(256, csize<width>, no, no, no, cbool<inverse>, out, out, tw);
    }
};

#else
#endif

template <bool is_even, bool first, typename T, bool autosort>
void make_fft_stages(dft_plan<T>* self, cbool_t<autosort>, size_t stage_size, cbool_t<is_even>,
                     cbool_t<first>)
{
    if constexpr (autosort)
    {
        if (stage_size >= 16)
        {
            add_stage<fft_autosort_stage_impl<T, first, false, false>>(self, stage_size,
                                                                       self->size / stage_size);
            make_fft_stages(self, ctrue, stage_size / 4, cbool_t<is_even>(), cfalse);
        }
        else
        {
            if (stage_size == 8)
                add_stage<fft_autosort_stage_impl<T, false, true, true>>(self, stage_size,
                                                                         self->size / stage_size);
            else
                add_stage<fft_autosort_stage_impl<T, false, true, false>>(self, stage_size,
                                                                          self->size / stage_size);
        }
    }
    else
    {
        constexpr size_t final_size = is_even ? 1024 : 512;

        if (stage_size >= 2048)
        {
            add_stage<fft_stage_impl<T, !first, is_even>>(self, stage_size);

            make_fft_stages(self, cfalse, stage_size / 4, cbool_t<is_even>(), cfalse);
        }
        else
        {
            add_stage<fft_final_stage_impl<T, !first, final_size>>(self, final_size);
            add_stage<fft_reorder_stage_impl<T, is_even>>(self, self->size);
        }
    }
}

} // namespace intrinsics

template <bool is_even, typename T>
void make_fft(dft_plan<T>* self, size_t stage_size, cbool_t<is_even>)
{
    if (use_autosort<T>(ilog2(stage_size)))
    {
        intrinsics::make_fft_stages(self, ctrue, stage_size, cbool<is_even>, ctrue);
    }
    else
    {
        intrinsics::make_fft_stages(self, cfalse, stage_size, cbool<is_even>, ctrue);
    }
}

template <typename T>
struct reverse_wrapper
{
    T& iterable;
};

template <typename T>
KFR_INTRINSIC auto begin(reverse_wrapper<T> w)
{
    return std::rbegin(w.iterable);
}

template <typename T>
KFR_INTRINSIC auto end(reverse_wrapper<T> w)
{
    return std::rend(w.iterable);
}

template <typename T>
KFR_INTRINSIC reverse_wrapper<T> reversed(T&& iterable)
{
    return { iterable };
}

template <typename T>
KFR_INTRINSIC void initialize_data_stage(dft_plan<T>* self, const dft_stage_ptr<T>& stage, size_t& offset)
{
    stage->data = self->data.data() + offset;
    stage->initialize(self->size);
    offset += stage->data_size;
}

template <typename T>
KFR_INTRINSIC size_t initialize_data(dft_plan<T>* self)
{
    self->data    = autofree<u8>(self->data_size);
    size_t offset = 0;
    for (dft_stage_ptr<T>& stage : self->all_stages)
    {
        initialize_data_stage(self, stage, offset);
    }
    return offset;
}

template <typename T>
KFR_INTRINSIC void initialize_order(dft_plan<T>* self)
{
    self->calc_disposition();
    typename dft_plan<T>::bitset ored = self->disposition_inplace[0] | self->disposition_inplace[1] |
                                        self->disposition_outofplace[0] | self->disposition_outofplace[1];
    if (ored.any()) // if scratch needed
        self->temp_size += align_up(sizeof(complex<T>) * self->size, platform<>::native_cache_alignment);
}

template <typename T>
KFR_INTRINSIC void init_fft(dft_plan<T>* self, size_t size, dft_order)
{
    const size_t log2n = ilog2(size);
    cswitch(
        csizes_t<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11>(), log2n,
        [&](auto log2n)
        {
            (void)log2n;
            constexpr size_t log2nv = val_of(decltype(log2n)());
            add_stage<intrinsics::fft_specialization<T, log2nv>>(self, size);
        },
        [&]()
        { cswitch(cfalse_true, is_even(log2n), [&](auto is_even) { make_fft(self, size, is_even); }); });
}

template <typename T>
KFR_INTRINSIC void generate_real_twiddles(dft_plan_real<T>* self, size_t size)
{
    using namespace intrinsics;
    constexpr size_t width = vector_width<T> * 2;
    block_process(size / 4, csizes_t<width, 1>(),
                  [=](size_t i, auto w)
                  {
                      constexpr size_t width = val_of(decltype(w)());
                      cwrite<width>(self->rtwiddle.data() + i,
                                    cossin(dup(-constants<T>::pi *
                                               ((enumerate<T, width>() + i + size / 4) / (size / 2)))));
                  });
}

template <typename T>
#if (defined CMT_ARCH_X32 && defined CMT_ARCH_X86 && defined __clang__) &&                                   \
    ((defined __APPLE__) || (__clang_major__ == 8))
// Fix for Clang 8.0 bug (x32 with FMA instructions)
// Xcode has different versions but x86 is very rare on macOS these days, 
// so disable inlining and FMA for x32 macOS and Clang 8.x
__attribute__((target("no-fma"), flatten, noinline))
#else
KFR_INTRINSIC
#endif
void
to_fmt(size_t real_size, const complex<T>* rtwiddle, complex<T>* out, const complex<T>* in,
       dft_pack_format fmt)
{
    using namespace intrinsics;
    size_t csize = real_size / 2; // const size_t causes internal compiler error: in tsubst_copy in GCC 5.2

    constexpr size_t width = vector_width<T> * 2;
    const cvec<T, 1> dc    = cread<1>(out);
    const size_t count     = (csize + 1) / 2;

    block_process(count - 1, csizes_t<width, 1>(),
                  [&](size_t i, auto w)
                  {
                      i++;
                      constexpr size_t width    = val_of(decltype(w)());
                      constexpr size_t widthm1  = width - 1;
                      const cvec<T, width> tw   = cread<width>(rtwiddle + i);
                      const cvec<T, width> fpk  = cread<width>(in + i);
                      const cvec<T, width> fpnk = reverse<2>(negodd(cread<width>(in + csize - i - widthm1)));

                      const cvec<T, width> f1k = fpk + fpnk;
                      const cvec<T, width> f2k = fpk - fpnk;
                      const cvec<T, width> t   = cmul(f2k, tw);
                      cwrite<width>(out + i, T(0.5) * (f1k + t));
                      cwrite<width>(out + csize - i - widthm1, reverse<2>(negodd(T(0.5) * (f1k - t))));
                  });

    if (is_even(csize))
    {
        size_t k              = csize / 2;
        const cvec<T, 1> fpk  = cread<1>(in + k);
        const cvec<T, 1> fpnk = negodd(fpk);
        cwrite<1>(out + k, fpnk);
    }
    if (fmt == dft_pack_format::CCs)
    {
        cwrite<1>(out, pack(dc[0] + dc[1], 0));
        cwrite<1>(out + csize, pack(dc[0] - dc[1], 0));
    }
    else
    {
        cwrite<1>(out, pack(dc[0] + dc[1], dc[0] - dc[1]));
    }
}

template <typename T>
#if (defined CMT_ARCH_X32 && defined CMT_ARCH_X86 && defined __clang__) &&                                   \
    ((defined __APPLE__) || (__clang_major__ == 8))
// Fix for Clang 8.0 bug (x32 with FMA instructions)
// Xcode has different versions but x86 is very rare on macOS these days, 
// so disable inlining and FMA for x32 macOS and Clang 8.x
__attribute__((target("no-fma"), flatten, noinline))
#else
KFR_INTRINSIC
#endif
void from_fmt(size_t real_size, complex<T>* rtwiddle, complex<T>* out, const complex<T>* in,
                            dft_pack_format fmt)
{
    using namespace intrinsics;

    const size_t csize = real_size / 2;

    cvec<T, 1> dc;

    if (fmt == dft_pack_format::CCs)
    {
        dc = pack(in[0].real() + in[csize].real(), in[0].real() - in[csize].real());
    }
    else
    {
        dc = pack(in[0].real() + in[0].imag(), in[0].real() - in[0].imag());
    }

    constexpr size_t width = vector_width<T> * 2;
    const size_t count     = (csize + 1) / 2;

    block_process(count - 1, csizes_t<width, 1>(),
                  [&](size_t i, auto w)
                  {
                      i++;
                      constexpr size_t width    = val_of(decltype(w)());
                      constexpr size_t widthm1  = width - 1;
                      const cvec<T, width> tw   = cread<width>(rtwiddle + i);
                      const cvec<T, width> fpk  = cread<width>(in + i);
                      const cvec<T, width> fpnk = reverse<2>(negodd(cread<width>(in + csize - i - widthm1)));

                      const cvec<T, width> f1k = fpk + fpnk;
                      const cvec<T, width> f2k = fpk - fpnk;
                      const cvec<T, width> t   = cmul_conj(f2k, tw);
                      cwrite<width>(out + i, f1k + t);
                      cwrite<width>(out + csize - i - widthm1, reverse<2>(negodd(f1k - t)));
                  });
    if (is_even(csize))
    {
        size_t k              = csize / 2;
        const cvec<T, 1> fpk  = cread<1>(in + k);
        const cvec<T, 1> fpnk = 2 * negodd(fpk);
        cwrite<1>(out + k, fpnk);
    }
    cwrite<1>(out, dc);
}

#ifndef KFR_DFT_NO_NPo2
template <typename T>
void init_dft(dft_plan<T>* self, size_t size, dft_order);
#endif

template <typename T>
KFR_INTRINSIC void initialize_stages(dft_plan<T>* self)
{
    if (is_poweroftwo(self->size))
    {
        init_fft(self, self->size, dft_order::normal);
    }
    else
    {
#ifndef KFR_DFT_NO_NPo2
        init_dft(self, self->size, dft_order::normal);
#else
        KFR_REPORT_LOGIC_ERROR("Non-power of 2 FFT is disabled but ", self->size, " size is requested");
#endif
    }
}

namespace impl
{
template <typename T>
void dft_initialize(dft_plan<T>& plan)
{
    if (plan.size == 0)
        return;
    initialize_stages(&plan);
    initialize_data(&plan);
    initialize_order(&plan);
}
} // namespace impl

template <typename T>
struct dft_stage_real_repack : dft_stage<T>
{
public:
    dft_stage_real_repack(size_t real_size, dft_pack_format fmt)
    {
        this->user         = static_cast<int>(fmt);
        this->stage_size   = real_size;
        this->can_inplace  = true;
        const size_t count = (real_size / 2 + 1) / 2;
        this->data_size    = align_up(sizeof(complex<T>) * count, platform<>::native_cache_alignment);
    }
    void do_initialize(size_t) override
    {
        using namespace intrinsics;
        constexpr size_t width = vector_width<T> * 2;
        size_t real_size       = this->stage_size;
        complex<T>* rtwiddle   = ptr_cast<complex<T>>(this->data);
        const size_t count     = (real_size / 2 + 1) / 2;
        block_process(count, csizes_t<width, 1>(),
                      [=](size_t i, auto w)
                      {
                          constexpr size_t width = val_of(decltype(w)());
                          cwrite<width>(
                              rtwiddle + i,
                              cossin(dup(-constants<T>::pi * ((enumerate<T, width>() + i + real_size / T(4)) /
                                                              (real_size / 2)))));
                      });
    }
    void do_execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp) override
    {
        to_fmt(this->stage_size, ptr_cast<complex<T>>(this->data), out, in,
               static_cast<dft_pack_format>(this->user));
    }
    void do_execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp) override
    {
        from_fmt(this->stage_size, ptr_cast<complex<T>>(this->data), out, in,
                 static_cast<dft_pack_format>(this->user));
    }
};

namespace impl
{
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan)
{
    if (plan.size == 0)
        return;
    initialize_stages(&plan);
    add_stage<dft_stage_real_repack<T>, false>(&plan, plan.size, plan.fmt);
    plan.stages[0].push_back(plan.all_stages.back().get());
    plan.stages[1].insert(plan.stages[1].begin(), plan.all_stages.back().get());
    initialize_data(&plan);
    initialize_order(&plan);
}
} // namespace impl

} // namespace CMT_ARCH_NAME

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)

CMT_PRAGMA_MSVC(warning(pop))
