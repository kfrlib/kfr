/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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
#ifdef KFR_DFT_MEASURE_STAGE_TIME
#include <kfr/runtime/time.hpp>
#endif

#define KFR_NEW_SMALL_FFT

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
#endif
#if KFR_HAS_WARNING("-Wunused-lambda-capture")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wunused-lambda-capture")
#endif
#if KFR_HAS_WARNING("-Wpass-failed")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpass-failed")
#endif

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4100))

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

template <typename T>
inline std::bitset<DFT_MAX_STAGES> fft_algorithm_selection;

template <>
inline std::bitset<DFT_MAX_STAGES> fft_algorithm_selection<float>{
#ifdef KFR_ARCH_NEON
    0
#else
    (1ull << 15) - 1
#endif
};

template <>
inline std::bitset<DFT_MAX_STAGES> fft_algorithm_selection<double>{ 0 };

template <typename T>
inline bool use_autosort(size_t log2n)
{
    return fft_algorithm_selection<T>[log2n];
}

#ifndef KFR_ARCH_NEON
#define KFR_AUTOSORT_FOR_2048
#define KFR_AUTOSORT_FOR_128D
#define KFR_AUTOSORT_FOR_256D
#define KFR_AUTOSORT_FOR_512
#define KFR_AUTOSORT_FOR_1024
#endif

#ifdef KFR_ARCH_AVX
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

namespace intr
{

template <typename T, size_t width>
KFR_INTRINSIC void initialize_twiddles_impl(complex<T>*& twiddle, size_t nn, size_t nnstep, size_t size,
                                            bool split_format)
{
    static_assert(width > 0, "width cannot be zero");
    vec<T, 2 * width> result = T();
    KFR_LOOP_UNROLL
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
KFR_NOINLINE void initialize_twiddles(complex<T>*& twiddle, size_t stage_size, size_t size, bool split_format)
{
    static_assert(width > 0, "width cannot be zero");
    const size_t count = stage_size / 4;
    size_t nnstep      = size / stage_size;
    DFT_ASSERT(width <= count);
    KFR_LOOP_NOUNROLL
    for (size_t n = 0; n < count; n += width)
    {
        initialize_twiddles_impl<T, width>(twiddle, n * nnstep * 1, nnstep * 1, size, split_format);
        initialize_twiddles_impl<T, width>(twiddle, n * nnstep * 2, nnstep * 2, size, split_format);
        initialize_twiddles_impl<T, width>(twiddle, n * nnstep * 3, nnstep * 3, size, split_format);
    }
}

constexpr static size_t fft_prefetch_iterations = 8;

template <size_t radix, size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC cfalse_t radix_pass(size_t N, size_t blocks, csize_t<width>, cbool_t<splitout>,
                                  cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>, complex<T>* out,
                                  const complex<T>* in, const complex<T>*& twiddle)
{
    KFR_ASSUME(blocks > 0);
    KFR_ASSUME(N > 0);

    for (size_t b = 0; b < blocks; ++b)
    {
        auto* tw = twiddle;
        bfly_loop<radix, T, width>( //
            N / radix, //
            bfly_read<radix, T, width, (splitout && !splitin), (prefetch ? fft_prefetch_iterations : 0)>{
                in, N / radix }, //
            bfly_bfly<radix, T, width, inverse, (splitin || splitout)>{}, //
            bfly_twiddle<radix, T, width, inverse, (splitin || splitout)>{ tw }, //
            bfly_permute<radix, T, width>{}, //
            bfly_write<radix, T, width, (splitin && !splitout)>{ out, N / radix });
        in += N;
        out += N;
    }
    twiddle += N / radix * (radix - 1);
    return {};
}

template <size_t Radix, size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void autosort_pass_first(csize_t<Radix>, size_t N, csize_t<width>, cbool_t<splitout>,
                                       cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>, complex<T>* out,
                                       const complex<T>* in, const complex<T>*& twiddle)
{
    const size_t Nblock             = N / Radix;
    const size_t Nstride            = Nblock;
    constexpr bool split_process    = splitin || splitout;
    constexpr bool split_read       = !splitin && split_process;
    constexpr bool interleave_write = !splitout && split_process;

    bfly_loop<Radix, T, width, 2>( //
        Nblock, //
        bfly_read<Radix, T, width, split_read, fft_prefetch_iterations>{ in, Nstride }, //
        bfly_bfly<Radix, T, width, inverse, split_process>{}, //
        bfly_twiddle<Radix, T, width, inverse, split_process>{ twiddle },
        bfly_write<Radix, T, width, interleave_write, 1>{ out });
}

template <size_t Radix, typename T, size_t N, bool inverse, bool split>
struct bfly_static_twiddle
{
    cvec<T, Radix - 1> tw_pkd;

    template <size_t i>
    KFR_INLINE_MEMBER cvec<T, N> unpack() noexcept
    {
        if constexpr (split)
        {
            return concat(repeat<N>(slice<i * 2, 1>(tw_pkd)), // re
                          repeat<N>(slice<i * 2 + 1, 1>(tw_pkd)) // im
            );
        }
        else
        {
            return repeat<N>(slice<i * 2, 2>(tw_pkd));
        }
    }
    KFR_INLINE_MEMBER void operator()(auto&& w0, auto&&... w) noexcept
    {
        static_assert(1 + sizeof...(w) == Radix);

        [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA { //
            ((w = cmuli<inverse>(cbool<split>, w, unpack<I>())), ...);
        }(csizeseq<Radix - 1>);
    }
    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept {}
};

template <size_t Radix, size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void autosort_pass_last(csize_t<Radix>, size_t stride, csize_t<width>, cbool_t<splitout>,
                                      cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>, complex<T>* out,
                                      const complex<T>* in, const complex<T>*&)
{
    static_assert(width > 0, "width cannot be zero");

    constexpr bool split_process    = splitin || splitout;
    constexpr bool split_read       = !splitin && split_process;
    constexpr bool interleave_write = !splitout && split_process;

    bfly_loop<Radix, T, width>( //
        stride, //
        bfly_read<Radix, T, width, split_read, fft_prefetch_iterations>{ in, stride }, //
        bfly_bfly<Radix, T, width, inverse, split_process>{}, //
        bfly_write<Radix, T, width, interleave_write>{ out, stride });
}

template <size_t Radix, size_t width, bool splitout, bool splitin, bool prefetch, bool inverse, typename T>
KFR_INTRINSIC void autosort_pass(csize_t<Radix>, size_t N, size_t stride, csize_t<width>, cbool_t<splitout>,
                                 cbool_t<splitin>, cbool_t<prefetch>, cbool_t<inverse>, complex<T>* out,
                                 const complex<T>* in, const complex<T>*& twiddle)
{
    static_assert(width > 0, "width cannot be zero");
    const size_t Nblock             = N / Radix;
    const size_t Nstride            = stride * Nblock;
    const size_t stridem1           = (Radix - 1) * stride;
    constexpr bool split_process    = splitin || splitout;
    constexpr bool split_read       = !splitin && split_process;
    constexpr bool interleave_write = !splitout && split_process;

    bfly_loop<Radix, T, width>( //
        stride, //
        bfly_read<Radix, T, width, split_read, fft_prefetch_iterations>{ in, Nstride }, //
        bfly_bfly<Radix, T, width, inverse, split_process>{}, //
        bfly_write<Radix, T, width, interleave_write>{ out, stride });
    in += stride;
    out += stride;
    twiddle += Radix - 1;
    out += stridem1;

    KFR_LOOP_NOUNROLL
    for (size_t b = 1; b < Nblock; b++)
    {
        bfly_loop<Radix, T, width>( //
            stride, //
            bfly_read<Radix, T, width, split_read, fft_prefetch_iterations>{ in, Nstride }, //
            bfly_bfly<Radix, T, width, inverse, split_process>{}, //
            bfly_static_twiddle<Radix, T, width, inverse, split_process>{ cread<Radix - 1>(twiddle) }, //
            bfly_write<Radix, T, width, interleave_write>{ out, stride });
        in += stride;
        out += stride;
        twiddle += Radix - 1;
        out += stridem1;
    }
}

template <size_t Radix = 4, typename T>
static void initialize_twiddle_autosort(size_t N, size_t w, complex<T>*& twiddle, bool split_format = false)
{
    for (size_t b = 0; b < N / Radix; ++b)
    {
        for (size_t i = 0; i < Radix - 1; ++i)
        {
            cwrite<1>(twiddle + b / w * (Radix - 1) * w + b % w + i * w,
                      calculate_twiddle<T>((i + 1) * b, N));
        }
    }
    twiddle += N / Radix * (Radix - 1);
}

template <typename T, size_t cols, size_t rows, size_t col_w, bool split>
struct fourstep_twiddles
{
    static_assert(col_w > 0, "col_w cannot be zero");
    static_assert(std::has_single_bit(cols), "cols must be a power of 2");
    static_assert(std::has_single_bit(rows), "rows must be a power of 2");

    constexpr fourstep_twiddles() noexcept
    {
        constexpr size_t block_size = col_w * (rows - 1);
        for (size_t r = 1; r < rows; ++r)
        {
            for (size_t c = 0; c < cols; ++c)
            {
                size_t k = r * c;

                std::complex<T> v;
                if constexpr (cols * rows <= 256)
                {
                    // constexpr-friendly twiddle factor generation using lookup tables
                    v = {
                        cos_using_table<T>(cols * rows, k),
                        -sin_using_table<T>(cols * rows, k),
                    };
                }
                else
                {
                    T a = c_pi<T, 2> * (T(k) / T(cols * rows));
                    v   = { std::cos(a), -std::sin(a) };
                }

                size_t b = c / col_w; // block
                size_t o = c % col_w; // offset

                size_t block_offs = b * block_size * 2 + (r - 1) * col_w * 2;
                if constexpr (split)
                {
                    twiddles[block_offs + o]         = v.real();
                    twiddles[block_offs + o + col_w] = v.imag();
                }
                else
                {
                    twiddles[block_offs + o * 2 + 0] = v.real();
                    twiddles[block_offs + o * 2 + 1] = v.imag();
                }
            }
        }
    }

    std::array<T, 2 * cols*(rows - 1)> twiddles;

    KFR_INTRINSIC const std::complex<T>* data() const noexcept
    {
        return reinterpret_cast<const std::complex<T>*>(twiddles.data());
    }
};

template <bool inverse, size_t r1, size_t r2, bool split = false, typename T>
KFR_INLINE void fourstep(std::complex<T>* out, const std::complex<T>* in)
{
    alignas(64) T scratch_buf[r1 * r2 * 2];
    std::complex<T>* KFR_RESTRICT scratch = reinterpret_cast<std::complex<T>*>(scratch_buf);

    constexpr bool fit_registers = r1 * r2 * 2 <= vector_capacity<T>;
    constexpr size_t width_scale = fit_registers ? 1 : 2;

    constexpr size_t n1 = std::clamp(bflyw<T>(r1) * width_scale, size_t(1), r2);
    constexpr size_t n2 = std::clamp(bflyw<T>(r2) * width_scale, size_t(1), r1);
    static constexpr fourstep_twiddles<T, r2, r1, n1, split> twiddles{};
    const std::complex<T>* tw = twiddles.data();

    bfly_loop<r1, T, n1, 2>( //
        r2, //
        bfly_read<r1, T, n1, split>{ in, r2 }, //
        bfly_bfly<r1, T, n1, inverse, split>{}, //
        bfly_twiddle<r1, T, n1, inverse, split>{ tw }, //
        bfly_write<r1, T, n1, split, 1>{ scratch } //
    );

    bfly_loop<r2, T, n2, 2>( //
        r1, //
        bfly_read<r2, T, n2, split>{ scratch, r1 }, //
        bfly_bfly<r2, T, n2, inverse, split>{}, //
        bfly_write<r2, T, n2, split>{ out, r1 });
}

template <typename T>
struct fft_config
{
#ifdef KFR_ARCH_NEON
    constexpr static inline const bool prefetch = false;
#else
    constexpr static inline const bool prefetch = true;
#endif
    constexpr static inline const size_t process_width =
        std::max(static_cast<size_t>(1), vector_capacity<T> / 16);
};

constexpr inline bool fft_recursion = true;

template <typename T, bool splitin>
struct fft_stage_impl : dft_stage<T>
{
    fft_stage_impl(size_t stage_size)
    {
        this->name       = dft_name(this);
        this->radix      = 4;
        this->stage_size = stage_size;
        this->repeats    = 4;
        this->recursion  = fft_recursion;
        this->data_size =
            align_up(sizeof(complex<T>) * stage_size / 4 * 3, platform<>::native_cache_alignment);
    }

    constexpr static bool prefetch = fft_config<T>::prefetch;
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
        KFR_ASSUME(stg_size >= 2048);
        KFR_ASSUME(stg_size % 2048 == 0);

        constexpr size_t radix = 4;

        bfly_loop<radix, T, width>( //
            stg_size / radix, //
            bfly_read<radix, T, width, !splitin, (prefetch ? 8 : 0)>{ in, stg_size / radix }, //
            bfly_bfly<radix, T, width, inverse, true>{}, //
            bfly_twiddle<radix, T, width, inverse, true>{ twiddle }, //
            bfly_permute<radix, T, width>{}, //
            bfly_write<radix, T, width, false>{ out, stg_size / radix });
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
        this->recursion  = fft_recursion;
        this->data_size  = align_up(sizeof(complex<T>) * size * 3 / 2, platform<>::native_cache_alignment);
    }

    constexpr static size_t width  = fft_config<T>::process_width;
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
        constexpr size_t pass_width  = std::min(width, N / 4);
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

    template <bool inverse, bool pass_splitin, typename U = T>
        requires(vector_capacity<U> >= 128)
    KFR_MEM_INTRINSIC void final_stage(csize_t<16>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>*, const complex<T>*& twiddle)
    {
        constexpr size_t radix = 16;
        constexpr size_t w     = std::max(vector_capacity<T> / 2 / radix, size_t(1));

        bfly_loop<radix, T, w>( //
            invN, //
            bfly_read<radix, T, w, false, (prefetch ? 8 : 0), 1>{ out }, //
            bfly_bfly<radix, T, w, inverse, false>{}, //
            bfly_permute<radix, T, w>{}, //
            bfly_write<radix, T, w, false, 1>{ out });
    }

    template <bool inverse, bool pass_splitin>
    KFR_MEM_INTRINSIC void final_stage(csize_t<8>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>*, const complex<T>*& twiddle)
    {
        constexpr size_t radix = 8;
        constexpr size_t w     = std::max(vector_capacity<T> / 2 / radix, size_t(1));

        bfly_loop<radix, T, w>( //
            invN, //
            bfly_read<radix, T, w, false, (prefetch ? 8 : 0), 1>{ out }, //
            bfly_bfly<radix, T, w, inverse, false>{}, //
            bfly_permute<radix, T, w>{}, //
            bfly_write<radix, T, w, false, 1>{ out });
    }

    template <bool inverse, bool pass_splitin>
    KFR_MEM_INTRINSIC void final_stage(csize_t<4>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>*, const complex<T>*& twiddle)
    {
        constexpr size_t radix = 4;
        constexpr size_t w     = std::max(vector_capacity<T> / 2 / radix, size_t(1));

        bfly_loop<radix, T, w>( //
            invN, //
            bfly_read<radix, T, w, false, (prefetch ? 8 : 0), 1>{ out }, //
            bfly_bfly<radix, T, w, inverse, false>{}, //
            bfly_permute<radix, T, w>{}, //
            bfly_write<radix, T, w, false, 1>{ out });
    }

    template <bool inverse, size_t N, bool pass_splitin>
    KFR_MEM_INTRINSIC void final_stage(csize_t<N>, size_t invN, cbool_t<pass_splitin>, complex<T>* out,
                                       const complex<T>* in, const complex<T>*& twiddle)
    {
        static_assert(N > 8, "");
        constexpr bool pass_splitout = get_pass_splitout(N);
        constexpr size_t pass_width  = std::min(width, N / 4);
        static_assert(pass_width == width || !pass_splitin, "");
        static_assert(pass_width <= N / 4, "");
        radix_pass<4>(N, invN, csize_t<pass_width>(), cbool<pass_splitout>, cbool_t<pass_splitin>(),
                      cbool_t<prefetch>(), cbool_t<inverse>(), out, in, twiddle);

        final_stage<inverse>(csize<N / 4>, invN * 4, cbool<pass_splitout>, out, out, twiddle);
    }
};

template <typename T>
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
        intr::br<T>(std::span(out, this->stage_size));
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

    constexpr static size_t width = std::min(size_t(16), std::max(size_t(4), fft_config<T>::process_width));

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
            autosort_pass_first(csize<4>, stg_size, csize_t<width>(), cfalse, cfalse, cbool_t<prefetch>(),
                                cbool_t<inverse>(), out, in, twiddle);
        }
        else if constexpr (is_last)
        {
            if constexpr (radix8)
                autosort_pass_last(csize<8>, stride, csize_t<width / 2>(), cfalse, cfalse,
                                   cbool_t<prefetch>(), cbool_t<inverse>(), out, in, twiddle);
            else
                autosort_pass_last(csize<4>, stride, csize_t<width>(), cfalse, cfalse, cbool_t<prefetch>(),
                                   cbool_t<inverse>(), out, in, twiddle);
        }
        else
        {
            if (stride == 4)
                autosort_pass(csize<4>, stg_size, stride, csize_t<4>(), cfalse, cfalse, cbool_t<prefetch>(),
                              cbool_t<inverse>(), out, in, twiddle);
            else
                autosort_pass(csize<4>, stg_size, stride, csize_t<width>(), cfalse, cfalse,
                              cbool_t<prefetch>(), cbool_t<inverse>(), out, in, twiddle);
        }
    }
};

template <typename T, size_t log2n>
struct fft_specialization : dft_stage<T>
{
    static_assert(log2n > 0 && log2n <= 8);
    fft_specialization(size_t size)
    {
        this->stage_size = size;
        this->name       = dft_name(this);
    }

    DFT_STAGE_FN

    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        constexpr size_t Radix = 1u << log2n;
        if constexpr (Radix <= bfly_max_packed_radix<T>)
        {
            // In-register FFT for small sizes
            bfly_packed<inverse>(csize_t<Radix>{}, out, in);
        }
        else
        {
            // For larger sizes, use the four-step algorithm to stay within register limits
            constexpr size_t r1 = 1 << ((log2n + 1) / 2);
            constexpr size_t r2 = 1 << (log2n / 2);
            fourstep<inverse, r1, r2>(out, in);
        }
    }
};

template <typename T>
struct fft_specialization<T, 0> : dft_stage<T>
{
    fft_specialization(size_t)
    {
        this->stage_size = 1;
        this->name       = dft_name(this);
    }

    DFT_STAGE_FN

    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8*)
    {
        out[0] = in[0];
    }
};

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

    constexpr static size_t width = std::min(size_t(16), std::max(size_t(4), fft_config<T>::process_width));

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
        autosort_pass_first(csize<4>, 512, csize<width>, no, no, ctrue, cbool<inverse>, scratch, in, tw);
        autosort_pass(csize<4>, 128, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        autosort_pass(csize<4>, 32, 16, csize<width>, no, no, no, cbool<inverse>, scratch, out, tw);
        autosort_pass_last(csize<8>, 64, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
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
            intr::br(std::span<std::complex<T>, 512>{ out, 512 });
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

    constexpr static size_t width = std::min(size_t(16), std::max(size_t(4), fft_config<T>::process_width));

    void do_initialize(size_t) final
    {
        complex<T>* twiddle = ptr_cast<complex<T>>(this->data);
        initialize_twiddle_autosort<4>(1024, width, twiddle);
        initialize_twiddle_autosort<4>(256, 1, twiddle);
        initialize_twiddle_autosort<4>(64, 1, twiddle);
        initialize_twiddle_autosort<4>(16, 1, twiddle);
    }

    DFT_STAGE_FN
    template <bool inverse>
    KFR_MEM_INTRINSIC void do_execute(complex<T>* out, const complex<T>* in, u8* temp)
    {
        auto no              = cfalse;
        const complex<T>* tw = ptr_cast<complex<T>>(this->data);
        complex<T>* scratch  = ptr_cast<complex<T>>(temp);
        autosort_pass_first(csize<4>, 1024, csize<width>, no, no, ctrue, cbool<inverse>, scratch, in, tw);
        autosort_pass(csize<4>, 256, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        autosort_pass(csize<4>, 64, 16, csize<width>, no, no, no, cbool<inverse>, scratch, out, tw);
        autosort_pass(csize<4>, 16, 64, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
        autosort_pass_last(csize<4>, 256, csize<width>, no, no, no, cbool<inverse>, out, out, tw);
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
            intr::br(std::span<std::complex<T>, 1024>{ out, 1024 });
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

    constexpr static size_t width = std::min(size_t(16), std::max(size_t(4), fft_config<T>::process_width));

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
        autosort_pass_first(csize<4>, 2048, csize<width>, no, no, ctrue, cbool<inverse>, scratch, in, tw);
        autosort_pass(csize<4>, 512, 4, csize<4>, no, no, no, cbool<inverse>, out, scratch, tw);
        autosort_pass(csize<4>, 128, 16, csize<4>, no, no, no, cbool<inverse>, scratch, out, tw);
        autosort_pass(csize<4>, 32, 64, csize<width>, no, no, no, cbool<inverse>, out, scratch, tw);
        autosort_pass_last(csize<8>, 256, csize<width>, no, no, no, cbool<inverse>, out, out, tw);
    }
};
#endif

template <bool first, typename T, bool autosort>
void make_fft_stages(dft_plan<T>* self, cbool_t<autosort>, size_t stage_size, cbool_t<first>)
{
    if constexpr (autosort)
    {
        if (stage_size >= 16)
        {
            add_stage<fft_autosort_stage_impl<T, first, false, false>>(self, stage_size,
                                                                       self->size / stage_size);
            make_fft_stages(self, ctrue, stage_size / 4, cfalse);
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
        if (stage_size >= 2048)
        {
            add_stage<fft_stage_impl<T, !first>>(self, stage_size);

            make_fft_stages(self, cfalse, stage_size / 4, cfalse);
        }
        else
        {
            if (std::countr_zero(self->size) % 2 == 0) // is even
            {
                add_stage<fft_final_stage_impl<T, !first, 1024>>(self, 1024);
            }
            else
            {
                add_stage<fft_final_stage_impl<T, !first, 512>>(self, 512);
            }
            add_stage<fft_reorder_stage_impl<T>>(self, self->size);
        }
    }
}

} // namespace intr

template <typename T>
void make_fft(dft_plan<T>* self, size_t stage_size, bool autosort)
{
    if (autosort)
    {
        intr::make_fft_stages(self, ctrue, stage_size, ctrue);
    }
    else
    {
        intr::make_fft_stages(self, cfalse, stage_size, ctrue);
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
        self->temp_size +=
            align_up(sizeof(complex<T>) * (self->size + 1), platform<>::native_cache_alignment);
}

template <typename T>
KFR_INTRINSIC void init_fft(dft_plan<T>* self, size_t size, dft_order)
{
    const size_t log2n  = ilog2(size);
    const bool autosort = use_autosort<T>(ilog2(size)) || self->progressive_optimized;
    cswitch(
        csizes_t<0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
#ifdef KFR_AUTOSORT_FOR_2048
                 ,
                 11
#endif
                 >(),
        log2n,
        [&](auto log2n)
        {
            (void)log2n;
            constexpr size_t log2nv = val_of(decltype(log2n)());
            add_stage<intr::fft_specialization<T, log2nv>>(self, size);
        },
        [&]() { make_fft(self, size, autosort); });
}

template <typename T>
KFR_INTRINSIC void generate_real_twiddles(dft_plan_real<T>* self, size_t size)
{
    using namespace intr;
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
#if (defined KFR_ARCH_X32 && defined KFR_ARCH_X86 && defined __clang__) &&                                   \
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
    using namespace intr;
    size_t csize = real_size / 2; // const size_t causes internal compiler error: in tsubst_copy in GCC 5.2

    constexpr size_t width = vector_width<T> * 2;
    const cvec<T, 1> dc    = cread<1>(in);
    cvec<T, 1> inmid       = cread<1>(in + csize / 2);
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
        cwrite<1>(out + csize / 2, negodd(inmid));
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
#if (defined KFR_ARCH_X32 && defined KFR_ARCH_X86 && defined __clang__) &&                                   \
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
    using namespace intr;

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
    cvec<T, 1> inmid = cread<1>(in + csize / 2);

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
        cwrite<1>(out + csize / 2, 2 * negodd(inmid));
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

template <typename T>
const complex<T>* select_in(const dft_plan<T>& plan, typename dft_plan<T>::bitset disposition, size_t stage,
                            const complex<T>* out, const complex<T>* in, const complex<T>* scratch)
{
    return disposition.test(stage) ? scratch : stage == 0 ? in : out;
}
template <typename T>
complex<T>* select_out(const dft_plan<T>& plan, typename dft_plan<T>::bitset disposition, size_t stage,
                       size_t total_stages, complex<T>* out, complex<T>* scratch)
{
    return stage == total_stages - 1 ? out : disposition.test(stage + 1) ? scratch : out;
}

template <typename T, bool inverse>
void dft_execute(const dft_plan<T>& plan, cbool_t<inverse>, complex<T>* out, const complex<T>* in, u8* temp)
{
    if (temp == nullptr && plan.temp_size > 0) [[unlikely]]
    {
        return call_with_temp(plan.temp_size, std::bind(&impl::dft_execute<T, inverse>, std::cref(plan),
                                                        cbool_t<inverse>{}, out, in, std::placeholders::_1));
    }
    auto&& stages = plan.stages[inverse];
    if (stages.size() == 1 && (stages[0]->can_inplace || in != out))
    {
#ifdef KFR_DFT_MEASURE_STAGE_TIME
        uint64_t time_before = clock_now();
#endif
        stages[0]->execute(cbool<inverse>, out, in, temp);
#ifdef KFR_DFT_MEASURE_STAGE_TIME
        stages[0]->time += clock_elapsed(time_before);
#endif
        return;
    }
    size_t stack[DFT_MAX_STAGES] = { 0 };

    typename dft_plan<T>::bitset disposition =
        in == out ? plan.disposition_inplace[inverse] : plan.disposition_outofplace[inverse];

    complex<T>* scratch = ptr_cast<complex<T>>(
        temp + plan.temp_size -
        align_up(sizeof(complex<T>) * (plan.size + 1), platform<>::native_cache_alignment));

    bool in_scratch = disposition.test(0);
    if (in_scratch)
    {
        stages[0]->copy_input(inverse, scratch, in, plan.size);
    }

    const size_t count = stages.size();

    for (size_t depth = 0; depth < count;)
    {
        if (stages[depth]->recursion)
        {
            size_t offset   = 0;
            size_t rdepth   = depth;
            size_t maxdepth = depth;
            do
            {
                if (stack[rdepth] == stages[rdepth]->repeats)
                {
                    stack[rdepth] = 0;
                    rdepth--;
                }
                else
                {
                    complex<T>* rout = select_out(plan, disposition, rdepth, stages.size(), out, scratch);
                    const complex<T>* rin = select_in(plan, disposition, rdepth, out, in, scratch);
#ifdef KFR_DFT_MEASURE_STAGE_TIME
                    uint64_t time_before = clock_now();
#endif
                    stages[rdepth]->execute(cbool<inverse>, rout + offset, rin + offset, temp);
#ifdef KFR_DFT_MEASURE_STAGE_TIME
                    stages[rdepth]->time += clock_elapsed(time_before);
#endif
                    offset += stages[rdepth]->out_offset;
                    stack[rdepth]++;
                    if (rdepth < count - 1 && stages[rdepth + 1]->recursion)
                        rdepth++;
                    else
                        maxdepth = rdepth;
                }
            } while (rdepth != depth);
            depth = maxdepth + 1;
        }
        else
        {
            size_t offset            = 0;
            complex<T>* cur_out      = select_out(plan, disposition, depth, stages.size(), out, scratch);
            const complex<T>* cur_in = select_in(plan, disposition, depth, out, in, scratch);
            dft_stage<T>* stage      = stages[depth];
            while (offset < plan.size)
            {
#ifdef KFR_DFT_MEASURE_STAGE_TIME
                uint64_t time_before = clock_now();
#endif
                stage->execute(cbool<inverse>, cur_out + offset, cur_in + offset, temp);
#ifdef KFR_DFT_MEASURE_STAGE_TIME
                stage->time += clock_elapsed(time_before);
#endif
                offset += stage->stage_size;
            }
            depth++;
        }
    }
}
template <typename T>
void dft_initialize_transpose(internal_generic::fn_transpose<T>& transpose)
{
    transpose = &kfr::KFR_ARCH_NAME::matrix_transpose;
}

template <typename T>
void dft_progressive_start(const dft_plan<T>& plan, typename dft_plan<T>::progressive& prog, bool inverse,
                           complex<T>* out, const complex<T>* in, u8* temp)
{
    prog.inverse = inverse;
    prog.out     = out;
    prog.in      = in;
    prog.temp    = temp;
    prog.scratch = ptr_cast<complex<T>>(
        temp + plan.temp_size -
        align_up(sizeof(complex<T>) * (plan.size + 1), platform<>::native_cache_alignment));

    prog.disposition = in == out ? plan.disposition_inplace[inverse] : plan.disposition_outofplace[inverse];

    bool in_scratch = prog.disposition.test(0);
    if (in_scratch)
    {
        plan.stages[inverse][0]->copy_input(inverse, prog.scratch, in, plan.size);
    }
    prog.step = 0;
}

template <typename T>
void dft_progressive_step(const dft_plan<T>& plan, typename dft_plan<T>::progressive& progressive)
{
    auto&& stages  = plan.stages[progressive.inverse];
    uint32_t depth = progressive.step;
    complex<T>* cur_out =
        select_out(plan, progressive.disposition, depth, stages.size(), progressive.out, progressive.scratch);
    const complex<T>* cur_in =
        select_in(plan, progressive.disposition, depth, progressive.out, progressive.in, progressive.scratch);

    size_t offset       = 0;
    dft_stage<T>* stage = stages[depth];
    while (offset < plan.size)
    {
        stage->execute(progressive.inverse, cur_out + offset, cur_in + offset, progressive.temp);
        offset += stage->stage_size;
    }
}
} // namespace impl

namespace intr
{

template <typename T>
struct dft_stage_real_repack : dft_stage<T>
{
public:
    dft_stage_real_repack(size_t real_size, dft_pack_format fmt)
    {
        this->user         = static_cast<int>(fmt);
        this->stage_size   = real_size;
        this->can_inplace  = true;
        this->name         = dft_name(this);
        const size_t count = (real_size / 2 + 1) / 2;
        this->data_size    = align_up(sizeof(complex<T>) * count, platform<>::native_cache_alignment);
    }
    void do_initialize(size_t) final
    {
        using namespace intr;
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
    void do_execute(cdirect_t, complex<T>* out, const complex<T>* in, u8* temp) final
    {
        to_fmt(this->stage_size, ptr_cast<complex<T>>(this->data), out, in,
               static_cast<dft_pack_format>(this->user));
    }
    void do_execute(cinvert_t, complex<T>* out, const complex<T>* in, u8* temp) final
    {
        from_fmt(this->stage_size, ptr_cast<complex<T>>(this->data), out, in,
                 static_cast<dft_pack_format>(this->user));
    }
    void copy_input(bool invert, complex<T>* out, const complex<T>* in, size_t size) final
    {
        size_t extra = invert && static_cast<dft_pack_format>(this->user) == dft_pack_format::CCs ? 1 : 0;
        builtin_memcpy(out, in, sizeof(complex<T>) * (size + extra));
    }
};
} // namespace intr

namespace impl
{
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan)
{
    if (plan.size == 0)
        return;
    initialize_stages(&plan);
    add_stage<intr::dft_stage_real_repack<T>, false>(&plan, plan.size, plan.fmt);
    plan.stages[0].push_back(plan.all_stages.back().get());
    plan.stages[1].insert(plan.stages[1].begin(), plan.all_stages.back().get());
    initialize_data(&plan);
    initialize_order(&plan);
}
} // namespace impl

} // namespace KFR_ARCH_NAME

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)

KFR_PRAGMA_MSVC(warning(pop))
