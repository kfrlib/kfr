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
        tw_ = -(tw_);
    ww = subadd(b1, ww * dupodd(tw_));
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
    constexpr bool read_split  = !splitin && splitout;
    constexpr bool write_split = splitin && !splitout;

    vec<T, width> re0, im0, re1, im1, re2, im2, re3, im3;

    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 0), re0, im0);
    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 1), re1, im1);
    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 2), re2, im2);
    split<T, 2 * width>(cread_split<width, aligned, read_split>(in + N4 * 3), re3, im3);

    const vec<T, width> sum02re = re0 + re2;
    const vec<T, width> sum02im = im0 + im2;
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

template <typename T>
KFR_INTRINSIC void prefetch_one(const complex<T>* in)
{
    KFR_PREFETCH(in);
}

template <typename T>
KFR_INTRINSIC void prefetch_four(size_t stride, const complex<T>* in)
{
    KFR_PREFETCH(in);
    KFR_PREFETCH(in + stride);
    KFR_PREFETCH(in + stride * 2);
    KFR_PREFETCH(in + stride * 3);
}

template <typename Ntype, size_t width, bool splitout, bool splitin, bool prefetch, bool use_br2,
          bool inverse, bool aligned, typename T>
KFR_INTRINSIC cfalse_t radix4_pass(Ntype N, size_t blocks, csize_t<width>, cbool_t<splitout>,
                                   cbool_t<splitin>, cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>,
                                   cbool_t<aligned>, complex<T>* out, const complex<T>* in,
                                   const complex<T>*& twiddle)
{
    static_assert(width > 0, "width cannot be zero");
    constexpr static size_t prefetch_offset = width * 8;
    const auto N4                           = N / csize_t<4>();
    const auto N43                          = N4 * csize_t<3>();
    CMT_ASSUME(blocks > 0);
    CMT_ASSUME(N > 0);
    CMT_ASSUME(N4 > 0);
    DFT_ASSERT(width <= N4);
    CMT_LOOP_NOUNROLL for (size_t b = 0; b < blocks; b++)
    {
        CMT_PRAGMA_CLANG(clang loop unroll_count(2))
        for (size_t n2 = 0; n2 < N4; n2 += width)
        {
            if constexpr (prefetch)
                prefetch_four(N4, in + prefetch_offset);
            radix4_body(N, csize_t<width>(), cbool_t<(splitout || splitin)>(), cbool_t<splitout>(),
                        cbool_t<splitin>(), cbool_t<use_br2>(), cbool_t<inverse>(), cbool_t<aligned>(), out,
                        in, twiddle + n2 * 3);
            in += width;
            out += width;
        }
        in += N43;
        out += N43;
    }
    twiddle += N43;
    return {};
}

template <bool splitin, size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<32>, size_t blocks, csize_t<width>, cfalse_t, cbool_t<splitin>,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    CMT_ASSUME(blocks > 0);
    constexpr static size_t prefetch_offset = 32 * 4;
    for (size_t b = 0; b < blocks; b++)
    {
        if constexpr (prefetch)
            prefetch_four(csize_t<64>(), out + prefetch_offset);
        cvec<T, 4> w0, w1, w2, w3, w4, w5, w6, w7;
        split(cread_split<8, aligned, splitin>(out + 0), w0, w1);
        split(cread_split<8, aligned, splitin>(out + 8), w2, w3);
        split(cread_split<8, aligned, splitin>(out + 16), w4, w5);
        split(cread_split<8, aligned, splitin>(out + 24), w6, w7);

        butterfly8<4, inverse>(w0, w1, w2, w3, w4, w5, w6, w7);

        w1 = cmul(w1, fixed_twiddle<T, 4, 32, 0, 1, inverse>());
        w2 = cmul(w2, fixed_twiddle<T, 4, 32, 0, 2, inverse>());
        w3 = cmul(w3, fixed_twiddle<T, 4, 32, 0, 3, inverse>());
        w4 = cmul(w4, fixed_twiddle<T, 4, 32, 0, 4, inverse>());
        w5 = cmul(w5, fixed_twiddle<T, 4, 32, 0, 5, inverse>());
        w6 = cmul(w6, fixed_twiddle<T, 4, 32, 0, 6, inverse>());
        w7 = cmul(w7, fixed_twiddle<T, 4, 32, 0, 7, inverse>());

        cvec<T, 8> z0, z1, z2, z3;
        transpose4x8(w0, w1, w2, w3, w4, w5, w6, w7, z0, z1, z2, z3);

        butterfly4<8, inverse>(cfalse, z0, z1, z2, z3, z0, z1, z2, z3);
        cwrite<32, aligned>(out, bitreverse<2>(concat(concat(z0, z1), concat(z2, z3))));
        out += 32;
    }
    return {};
}

template <size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<8>, size_t blocks, csize_t<width>, cfalse_t, cfalse_t,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    CMT_ASSUME(blocks > 0);
    DFT_ASSERT(2 <= blocks);
    constexpr static size_t prefetch_offset = width * 16;
    for (size_t b = 0; b < blocks; b += 2)
    {
        if constexpr (prefetch)
            prefetch_one(out + prefetch_offset);

        cvec<T, 8> vlo = cread<8, aligned>(out + 0);
        cvec<T, 8> vhi = cread<8, aligned>(out + 8);
        butterfly8<inverse>(vlo);
        butterfly8<inverse>(vhi);
        vlo = permutegroups<(2), 0, 4, 2, 6, 1, 5, 3, 7>(vlo);
        vhi = permutegroups<(2), 0, 4, 2, 6, 1, 5, 3, 7>(vhi);
        cwrite<8, aligned>(out, vlo);
        cwrite<8, aligned>(out + 8, vhi);
        out += 16;
    }
    return {};
}

template <size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<16>, size_t blocks, csize_t<width>, cfalse_t, cfalse_t,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    CMT_ASSUME(blocks > 0);
    constexpr static size_t prefetch_offset = width * 4;
    DFT_ASSERT(2 <= blocks);
    CMT_PRAGMA_CLANG(clang loop unroll_count(2))
    for (size_t b = 0; b < blocks; b += 2)
    {
        if constexpr (prefetch)
            prefetch_one(out + prefetch_offset);

        cvec<T, 16> vlo = cread<16, aligned>(out);
        cvec<T, 16> vhi = cread<16, aligned>(out + 16);
        butterfly4<4, inverse>(vlo);
        butterfly4<4, inverse>(vhi);
        apply_twiddles4<0, 4, 4, inverse>(vlo);
        apply_twiddles4<0, 4, 4, inverse>(vhi);
        vlo = digitreverse4<2>(vlo);
        vhi = digitreverse4<2>(vhi);
        butterfly4<4, inverse>(vlo);
        butterfly4<4, inverse>(vhi);

        use_br2 ? cbitreverse_write(out, vlo) : cdigitreverse4_write(out, vlo);
        use_br2 ? cbitreverse_write(out + 16, vhi) : cdigitreverse4_write(out + 16, vhi);
        out += 32;
    }
    return {};
}

template <size_t width, bool prefetch, bool use_br2, bool inverse, bool aligned, typename T>
KFR_INTRINSIC ctrue_t radix4_pass(csize_t<4>, size_t blocks, csize_t<width>, cfalse_t, cfalse_t,
                                  cbool_t<use_br2>, cbool_t<prefetch>, cbool_t<inverse>, cbool_t<aligned>,
                                  complex<T>* out, const complex<T>*, const complex<T>*& /*twiddle*/)
{
    constexpr static size_t prefetch_offset = width * 4;
    CMT_ASSUME(blocks > 8);
    DFT_ASSERT(8 <= blocks);
    for (size_t b = 0; b < blocks; b += 4)
    {
        if constexpr (prefetch)
            prefetch_one(out + prefetch_offset);

        cvec<T, 16> v16 = cdigitreverse4_read<16, aligned>(out);
        butterfly4<4, inverse>(v16);
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
        radix4_pass(stg_size, 1, csize_t<width>(), ctrue, cbool_t<splitin>(), cbool_t<!is_even>(),
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
    constexpr static bool use_br2  = !is_even;
    constexpr static bool aligned  = false;
    constexpr static bool prefetch = fft_config<T>::prefetch && splitin;

    KFR_MEM_INTRINSIC void init_twiddles(csize_t<8>, size_t, cfalse_t, complex<T>*&) {}
    KFR_MEM_INTRINSIC void init_twiddles(csize_t<4>, size_t, cfalse_t, complex<T>*&) {}

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

    template <bool inverse, typename U = T, KFR_ENABLE_IF(std::is_same_v<U, float>)>
    KFR_MEM_INTRINSIC void final_stage(csize_t<32>, size_t invN, cfalse_t, complex<T>* out, const complex<T>*,
                                       const complex<T>*& twiddle)
    {
        radix4_pass(csize_t<32>(), invN, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
    }

    template <bool inverse, typename U = T, KFR_ENABLE_IF(std::is_same_v<U, float>)>
    KFR_MEM_INTRINSIC void final_stage(csize_t<16>, size_t invN, cfalse_t, complex<T>* out, const complex<T>*,
                                       const complex<T>*& twiddle)
    {
        radix4_pass(csize_t<16>(), invN, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
    }

    template <bool inverse>
    KFR_MEM_INTRINSIC void final_stage(csize_t<8>, size_t invN, cfalse_t, complex<T>* out, const complex<T>*,
                                       const complex<T>*& twiddle)
    {
        radix4_pass(csize_t<8>(), invN, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
                    cbool_t<prefetch>(), cbool_t<inverse>(), cbool_t<aligned>(), out, out, twiddle);
    }

    template <bool inverse>
    KFR_MEM_INTRINSIC void final_stage(csize_t<4>, size_t invN, cfalse_t, complex<T>* out, const complex<T>*,
                                       const complex<T>*& twiddle)
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
        fft_reorder(out, this->user, cbool_t<!is_even>());
    }
};

template <typename T, size_t log2n>
struct fft_specialization;

template <typename T>
struct fft_specialization<T, 1> : dft_stage<T>
{
    fft_specialization(size_t) { this->name = dft_name(this); }

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
    fft_specialization(size_t) { this->name = dft_name(this); }

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
    fft_specialization(size_t) { this->name = dft_name(this); }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 8> v8 = cread<8, aligned>(in);
        butterfly8<inverse>(v8);
        cwrite<8, aligned>(out, v8);
    }
};

template <typename T>
struct fft_specialization<T, 4> : dft_stage<T>
{
    fft_specialization(size_t) { this->name = dft_name(this); }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 16> v16 = cread<16, aligned>(in);
        butterfly16<inverse>(v16);
        cwrite<16, aligned>(out, v16);
    }
};

template <typename T>
struct fft_specialization<T, 5> : dft_stage<T>
{
    fft_specialization(size_t) { this->name = dft_name(this); }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        cvec<T, 32> v32 = cread<32, aligned>(in);
        butterfly32<inverse>(v32);
        cwrite<32, aligned>(out, v32);
    }
};

template <typename T>
struct fft_specialization<T, 6> : dft_stage<T>
{
    fft_specialization(size_t) { this->name = dft_name(this); }

    constexpr static bool aligned = false;
    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        butterfly64(cbool_t<inverse>(), cbool_t<aligned>(), out, in);
    }
};

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
    constexpr static size_t width        = const_min(fft_config<T>::process_width, size_t(16));
    constexpr static bool use_br2        = true;
    constexpr static bool prefetch       = false;
    constexpr static size_t final_size   = 32;
    constexpr static size_t split_format = false;

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
        radix4_pass(128, 1, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(), cbool_t<prefetch>(),
                    cbool_t<inverse>(), cbool_t<aligned>(), out, in, twiddle);
        radix4_pass(csize_t<32>(), 4, csize_t<width>(), cfalse, cfalse, cbool_t<use_br2>(),
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
        this->name      = dft_name(this);
        this->temp_size = sizeof(complex<float>) * 256;
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

template <>
struct fft_specialization<double, 8> : fft_final_stage_impl<double, false, 256>
{
    using T = double;
    fft_specialization(size_t stage_size) : fft_final_stage_impl<double, false, 256>(stage_size)
    {
        this->name = dft_name(this);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        fft_final_stage_impl<double, false, 256>::template do_execute<inverse>(out, in, nullptr);
        if (this->need_reorder)
            fft_reorder(out, csize_t<8>());
    }
};

template <typename T>
struct fft_specialization<T, 9> : fft_final_stage_impl<T, false, 512>
{
    fft_specialization(size_t stage_size) : fft_final_stage_impl<T, false, 512>(stage_size)
    {
        this->name = dft_name(this);
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

} // namespace intrinsics

template <bool is_even, bool first, typename T>
void make_fft(dft_plan<T>* self, size_t stage_size, cbool_t<is_even>, cbool_t<first>)
{
    constexpr size_t final_size = is_even ? 1024 : 512;

    if (stage_size >= 2048)
    {
        add_stage<intrinsics::fft_stage_impl<T, !first, is_even>>(self, stage_size);

        make_fft(self, stage_size / 4, cbool_t<is_even>(), cfalse);
    }
    else
    {
        add_stage<intrinsics::fft_final_stage_impl<T, !first, final_size>>(self, final_size);
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
    for (dft_stage_ptr<T>& stage : self->stages)
    {
        initialize_data_stage(self, stage, offset);
    }
    return offset;
}

template <typename T>
KFR_INTRINSIC void initialize_order(dft_plan<T>* self)
{
    bool to_scratch     = false;
    bool scratch_needed = false;
    for (dft_stage_ptr<T>& stage : reversed(self->stages))
    {
        if (to_scratch)
        {
            scratch_needed = true;
        }
        stage->to_scratch = to_scratch;
        if (!stage->can_inplace)
        {
            to_scratch = !to_scratch;
        }
    }
    if (scratch_needed || !self->stages[0]->can_inplace)
        self->temp_size += align_up(sizeof(complex<T>) * self->size, platform<>::native_cache_alignment);
}

template <typename T>
KFR_INTRINSIC void init_fft(dft_plan<T>* self, size_t size, dft_order)
{
    const size_t log2n = ilog2(size);
    cswitch(
        csizes_t<1, 2, 3, 4, 5, 6, 7, 8, 9, 10>(), log2n,
        [&](auto log2n)
        {
            (void)log2n;
            constexpr size_t log2nv = val_of(decltype(log2n)());
            add_stage<intrinsics::fft_specialization<T, log2nv>>(self, size);
        },
        [&]()
        {
            cswitch(cfalse_true, is_even(log2n),
                    [&](auto is_even)
                    {
                        make_fft(self, size, is_even, ctrue);
                        constexpr size_t is_evenv = val_of(decltype(is_even)());
                        add_stage<intrinsics::fft_reorder_stage_impl<T, is_evenv>>(self, size);
                    });
        });
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

template <typename T>
void init_dft(dft_plan<T>* self, size_t size, dft_order);

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

template <typename T>
void dft_initialize(dft_plan<T>& plan)
{
    initialize_stages(&plan);
    initialize_data(&plan);
    initialize_order(&plan);
}

template <typename T>
struct dft_stage_real_repack : dft_stage<T>
{
public:
    dft_stage_real_repack(size_t real_size, dft_pack_format fmt)
    {
        this->user       = static_cast<int>(fmt);
        this->stage_size = real_size;
        const size_t count = (real_size / 2 + 1) / 2;
        this->data_size  = align_up(sizeof(complex<T>) * count, platform<>::native_cache_alignment);
    }
    void do_initialize(size_t) override
    {
        using namespace intrinsics;
        constexpr size_t width = vector_width<T> * 2;
        size_t real_size       = this->stage_size;
        complex<T>* rtwiddle   = ptr_cast<complex<T>>(this->data);
        const size_t count = (real_size / 2 + 1) / 2;
        block_process(count, csizes_t<width, 1>(),
                      [=](size_t i, auto w)
                      {
                          constexpr size_t width = val_of(decltype(w)());
                          cwrite<width>(
                              rtwiddle + i,
                              cossin(dup(-constants<T>::pi *
                                         ((enumerate<T, width>() + i + real_size / T(4)) / (real_size / 2)))));
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

template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan)
{
    initialize_stages(&plan);
    plan.fmt_stage.reset(new dft_stage_real_repack<T>(plan.size, plan.fmt));
    plan.data_size += plan.fmt_stage->data_size;
    size_t offset = initialize_data(&plan);
    initialize_data_stage(&plan, plan.fmt_stage, offset);
    initialize_order(&plan);
}

} // namespace CMT_ARCH_NAME

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)

CMT_PRAGMA_MSVC(warning(pop))
