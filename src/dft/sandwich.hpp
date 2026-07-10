/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

#include "ngfft.hpp"

namespace kfr
{

struct dft_sandwich_half
{
    uint8_t l2size; // log2 of the FFT size for this half or UINT8_MAX if not known at compile time
    uint8_t l2remaining;
    bool single_pass;
};

template <>
struct dft_config<dft_family::fourstep>
{
    dft_sandwich_half dif;
    dft_sandwich_half dit;
    size_t prefetch_offset;

    constexpr bool fixed_twiddles() const noexcept
    {
        return dif.l2size != UINT8_MAX && dit.l2size != UINT8_MAX &&
               dif.l2size + dit.l2size <= data::sin_table_log2;
    }

    constexpr uint8_t l2fftsize() const noexcept
    {
        if (dif.l2size != UINT8_MAX && dit.l2size != UINT8_MAX)
            return dif.l2size + dit.l2size;
        else
            return UINT8_MAX;
    }
};

inline namespace KFR_ARCH_NAME
{

template <dft_traits traits>
KFR_INTRINSIC constexpr std::pair<uint8_t, uint8_t> sandwich_split_size(uint8_t l2fftsize) noexcept
{
    using T = typename traits::type;

    if constexpr (requires { traits::override_sandwich_split; })
    {
        return { traits::override_sandwich_split, l2fftsize - traits::override_sandwich_split };
    }

    constexpr uint8_t l2elemsize = static_cast<uint8_t>(ilog2(2 * sizeof(T)));

#ifdef KFR_ARCH_AVX
    if (l2fftsize + l2elemsize > 15) // memory-bound, 32KiB+
    {
        uint8_t n2 = 8 - l2elemsize;
        return { uint8_t(l2fftsize - n2), n2 };
    }
    else // compute-bound, <32KiB
    {
        if constexpr (std::is_same_v<T, float>)
        {
            constexpr uint8_t n1[]{
                1, 1, 2, 2, 3, 4, 4, 3, 4, 4, 5, 6,
            };
            return { n1[l2fftsize - 1], l2fftsize - n1[l2fftsize - 1] };
        }
        else
        {
            constexpr uint8_t n1[]{
                1, 1, 2, 2, 3, 4, 3, 2, 3, 4, 5,
            };
            return { n1[l2fftsize - 1], l2fftsize - n1[l2fftsize - 1] };
        }
    }
#elif defined KFR_ARCH_NEON && defined KFR_OS_APPLE
    if (l2fftsize + l2elemsize > 17) // memory-bound, 128KiB+
    {
        if constexpr (std::is_same_v<T, float>)
        {
            const uint8_t n2 = (l2fftsize & 1) ? 5 : 8;
            return { uint8_t(l2fftsize - n2), n2 };
        }
        else
        {
            const uint8_t n2 = (l2fftsize & 1) ? 2 : 5;
            return { uint8_t(l2fftsize - n2), n2 };
        }
    }
    else
    {
        if constexpr (std::is_same_v<T, float>)
        {
            constexpr uint8_t n1[]{
                1, 1, 2, 2, 3, 3, 2, 3, 3, 3, 3, 6, 3, 6,
            };
            return { n1[l2fftsize - 1], l2fftsize - n1[l2fftsize - 1] };
        }
        else
        {
            constexpr uint8_t n1[]{
                1, 1, 2, 2, 3, 2, 3, 3, 3, 7, 8, 9, 10,
            };
            return { n1[l2fftsize - 1], l2fftsize - n1[l2fftsize - 1] };
        }
    }
#else
    if (l2fftsize >= 10)
    {
        return { uint8_t(l2fftsize - 6), 6 };
    }
    else
    {
        return { l2fftsize / 2, l2fftsize - l2fftsize / 2 };
    }
#endif
}

template <dft_traits traits>
constexpr dft_sandwich_half get_sandwich_half(uint8_t l2size) noexcept
{
    constexpr uint8_t l2maxsize = 6;

    if (l2size <= traits::l2maxsingleradix)
    {
        return {
            .l2size      = l2size,
            .l2remaining = l2size,
            .single_pass = true,
        };
    }

    const uint8_t regular_passes =
        (l2size - traits::l2maxradix + traits::l2baseradix - 1) / traits::l2baseradix;
    const uint8_t l2remaining = l2size - traits::l2baseradix * regular_passes;

    return {
        .l2size      = uint8_t(l2size > l2maxsize ? UINT8_MAX : l2size),
        .l2remaining = l2remaining,
        .single_pass = (l2remaining == l2size),
    };
}

template <dft_traits traits, dft_config<dft_family::fourstep> cfg>
KFR_INTRINSIC constexpr std::pair<uint8_t, uint8_t> sandwich_split_size_rt(uint8_t l2fftsize) noexcept
{
    if constexpr (cfg.dif.l2size != UINT8_MAX && cfg.dit.l2size != UINT8_MAX)
    {
        return { cfg.dif.l2size, cfg.dit.l2size };
    }
    else if constexpr (cfg.dif.l2size != UINT8_MAX)
    {
        return { cfg.dif.l2size, uint8_t(l2fftsize - cfg.dif.l2size) };
    }
    else if constexpr (cfg.dit.l2size != UINT8_MAX)
    {
        return { uint8_t(l2fftsize - cfg.dit.l2size), cfg.dit.l2size };
    }
    else
    {
        return sandwich_split_size<traits>(l2fftsize);
    }
}

template <dft_traits traits, dft_decomp dir, dft_sandwich_half half, typename Fn>
KFR_INTRINSIC void sandwich_iterate(uint8_t l2fftsize, Fn&& fn)
{
    constexpr uint8_t l2baseradix = traits::l2baseradix;

    if constexpr (half.single_pass)
    {
        fn(bfly_pass(cl2radix<half.l2remaining>{}, cl2butterflies<0>{}, cl2blocks<0>{}));
        return;
    }

    const uint8_t l2fmb = l2fftsize - l2baseradix;

    if constexpr (dir == dft_decomp::dit)
    {
        // butterflies=1 blocks=max
        fn(bfly_pass(cl2radix<half.l2remaining>{}, cl2butterflies<0>{},
                     uint8_t(l2fftsize - half.l2remaining)));

        uint8_t l2stride = half.l2remaining;

        while (l2stride < l2fmb)
        {
            fn(bfly_pass(cl2radix<l2baseradix>{}, l2stride, uint8_t(l2fmb - l2stride)));
            l2stride += l2baseradix;
        }

        // butterflies=max blocks=1
        fn(bfly_pass(cl2radix<l2baseradix>{}, l2fmb, cl2blocks<0>{}));
    }
    else
    {
        // butterflies=max blocks=1
        fn(bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2fmb), cl2blocks<0>{}));
        int l2stride = l2fmb - l2baseradix;

        while (l2stride >= static_cast<int>(half.l2remaining))
        {
            fn(bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2stride), uint8_t(l2fmb - l2stride)));
            l2stride -= l2baseradix;
        }

        // butterflies=1 blocks=max
        fn(bfly_pass(cl2radix<half.l2remaining>{}, cl2butterflies<0>{},
                     uint8_t(l2fftsize - half.l2remaining)));
    }
}

template <dft_traits traits, dft_decomp dir, typename Fn>
constexpr KFR_INTRINSIC void sandwich_iterate(uint8_t l2fftsize, const dft_sandwich_half& half, Fn&& fn)
{
    constexpr uint8_t l2baseradix = traits::l2baseradix;

    if (half.single_pass)
    {
        cswitch(cvalseq<uint8_t, traits::l2maxradix, 1>, half.l2remaining,
                [&]<uint8_t rem>(cval_t<uint8_t, rem>) KFR_INLINE_LAMBDA
                { fn(bfly_pass(cl2radix<rem>{}, cl2butterflies<0>{}, cl2blocks<0>{})); });
        return;
    }

    const uint8_t l2fmb = l2fftsize - l2baseradix;

    if constexpr (dir == dft_decomp::dit)
    {
        // butterflies=1 blocks=max
        cswitch(
            cvalseq<uint8_t, traits::l2maxradix, 1>, half.l2remaining,
            [&]<uint8_t rem>(cval_t<uint8_t, rem>) KFR_INLINE_LAMBDA
            { fn(bfly_pass(cl2radix<rem>{}, cl2butterflies<0>{}, uint8_t(l2fftsize - half.l2remaining))); });

        uint8_t l2stride = half.l2remaining;

        while (l2stride < l2fmb)
        {
            fn(bfly_pass(cl2radix<l2baseradix>{}, l2stride, uint8_t(l2fmb - l2stride)));
            l2stride += l2baseradix;
        }

        // butterflies=max blocks=1
        fn(bfly_pass(cl2radix<l2baseradix>{}, l2fmb, cl2blocks<0>{}));
    }
    else
    {
        // butterflies=max blocks=1
        fn(bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2fmb), cl2blocks<0>{}));

        uint8_t l2stride = l2fmb;
        while (l2stride >= half.l2remaining + l2baseradix)
        {
            l2stride -= l2baseradix;
            fn(bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2stride), uint8_t(l2fmb - l2stride)));
        }

        // butterflies=1 blocks=max
        cswitch(
            cvalseq<uint8_t, traits::l2maxradix, 1>, half.l2remaining,
            [&]<uint8_t rem>(cval_t<uint8_t, rem>) KFR_INLINE_LAMBDA
            { fn(bfly_pass(cl2radix<rem>{}, cl2butterflies<0>{}, uint8_t(l2fftsize - half.l2remaining))); });
    }
}

template <dft_traits traits>
constexpr size_t sandwich_twiddle_size(uint8_t l2fftsize, const dft_config<dft_family::fourstep>& cfg)
{
    size_t twiddle_count    = 0;
    const auto [l2r1, l2r2] = sandwich_split_size<traits>(l2fftsize);
    const size_t r1         = 1ull << l2r1;
    const size_t r2         = 1ull << l2r2;

    sandwich_iterate<traits, dft_decomp::dif>(
        l2r1, cfg.dif,
        [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(const bfly_pass<l2passradix, l2bf, l2bl>& pass)
        { twiddle_count += (pass.radix() - 1) * (pass.butterflies() - 1); });

    if (!(cfg.dit.single_pass && cfg.fixed_twiddles()))
    {
        constexpr size_t complex_per_cacheline = KFR_CACHE_LINE_SIZE / sizeof(complex<typename traits::type>);
        twiddle_count                          = align_up(twiddle_count, complex_per_cacheline);
        twiddle_count += r1 * r2;
    }

    sandwich_iterate<traits, dft_decomp::dit>(
        l2r2, cfg.dit,
        [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(const bfly_pass<l2passradix, l2bf, l2bl>& pass)
        { twiddle_count += (pass.radix() - 1) * (pass.butterflies() - 1); });

    return twiddle_count;
}

template <dft_traits traits>
void sandwich_prepare(complex<typename traits::type>* twiddles, uint8_t l2fftsize,
                      const dft_config<dft_family::fourstep>& cfg)
{
    using namespace intr;

    using T = typename traits::type;
    if (l2fftsize < 4) [[unlikely]]
        return;
    auto [l2r1, l2r2] = sandwich_split_size<traits>(l2fftsize);
    const size_t r1   = 1ull << l2r1;
    const size_t r2   = 1ull << l2r2;

    sandwich_iterate<traits, dft_decomp::dif>(
        l2r1, cfg.dif,
        [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(const bfly_pass<l2passradix, l2bf, l2bl>& pass)
        {
            for (size_t e = 1; e < pass.butterflies(); ++e)
            {
                constexpr size_t R = 1 << l2passradix;
                for (size_t n = 1; n < R; ++n)
                {
                    size_t k    = pass.twiddle_k(e, n);
                    *twiddles++ = complex_twiddle<T>(k, pass.block_size());
                }
            }
        });

    const bool twiddle_pass = cfg.dit.single_pass && cfg.fixed_twiddles();

    if (!twiddle_pass)
    {
        twiddles = align_up(twiddles, KFR_CACHE_LINE_SIZE);

        const size_t R = 1ull << cfg.dit.l2remaining;
        const size_t N = 1ull << std::min(traits::l2basewidth, l2r1);

        const uint8_t l2R = cfg.dit.l2remaining;
        for (size_t bl = 0; bl < r2 / R; ++bl)
        {
            for (size_t bf = 0; bf < r1; bf += N)
            {
                for (size_t a = 0; a < R; ++a)
                {
                    T* scalars = reinterpret_cast<T*>(twiddles);
                    for (size_t i = 0; i < N; ++i)
                    {
                        size_t j = i;
                        if (N >= 32 / sizeof(T))
                            j = shuffle_optimizer<T>(j);
                        size_t k       = bitreverse(bl * R + bitreverse(a, l2R), l2r2) * (bf + j);
                        complex<T> c   = complex_twiddle<T>(k, 1ull << l2fftsize);
                        scalars[i]     = c.real();
                        scalars[N + i] = c.imag();
                    }
                    twiddles += N;
                }
            }
        }
    }

    sandwich_iterate<traits, dft_decomp::dit>(
        l2r2, cfg.dit,
        [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(const bfly_pass<l2passradix, l2bf, l2bl>& pass)
        {
            for (size_t e = 1; e < pass.butterflies(); ++e)
            {
                constexpr size_t R = 1 << l2passradix;
                for (size_t n = 1; n < R; ++n)
                {
                    size_t k    = pass.twiddle_k(e, n);
                    *twiddles++ = complex_twiddle<T>(k, pass.block_size());
                }
            }
        });
}

template <dft_traits traits>
constexpr size_t max_slice_width =
    size_t(1) << (14 - l2elementsize<typename traits::type> - traits::l2baseradix); // 16KiB

template <dft_traits traits, dft_decomp dir, dft_sandwich_half half, typename Fn>
KFR_INTRINSIC const complex<typename traits::type>* sandwich_iterate_recursive(
    uint8_t l2fftsize, size_t total_width, const complex<typename traits::type>* twiddle, Fn&& fn)
{
    using T                       = typename traits::type;
    constexpr uint8_t l2baseradix = traits::l2baseradix;

    if constexpr (half.single_pass || (half.l2size <= half.l2remaining + 2 * l2baseradix))
    {
        const complex<T>* saved_twiddle = twiddle;

        for (size_t slice_offset = 0;;)
        {
            twiddle                    = saved_twiddle;
            const size_t process_width = std::min(max_slice_width<traits>, total_width - slice_offset);
            // Avoiding overhead of recursion
            sandwich_iterate<traits, dir, half>(
                l2fftsize, [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(
                               const bfly_pass<l2passradix, l2bf, l2bl>& pass) KFR_INLINE_LAMBDA
                { twiddle = fn(pass, process_width, total_width, pass.blocks(), slice_offset, twiddle); });
            slice_offset += max_slice_width<traits>;
            if (slice_offset >= total_width)
                break;
        }
    }
    else
    {
        constexpr size_t R         = size_t(1) << l2baseradix;
        const uint8_t l2fmb        = l2fftsize - l2baseradix;
        const uint8_t l2items      = uint8_t(l2fftsize - half.l2remaining);
        const size_t process_width = total_width;

        if constexpr (dir == dft_decomp::dit)
        {
            const complex<T>* saved_twiddle = twiddle;

            size_t offset = total_width << l2fftsize; // end of the buffer

            // traverses backwards
            traverse_childrenfirst<R>(
                l2items,
                [&](uint8_t /* levels_down */) KFR_INLINE_LAMBDA { // Merged leaf: all R siblings at once
                    const bfly_pass pass(cl2radix<half.l2remaining>{}, cl2butterflies<0>{},
                                         uint8_t(l2fftsize - half.l2remaining));
                    offset -= total_width << (l2baseradix + pass.l2block_size());
                    // DIT leaves are first-touch passes: they read fresh input data.
                    twiddle = fn(pass, process_width, total_width, R, offset, saved_twiddle);
                },
                [&](uint8_t depth) KFR_INLINE_LAMBDA { // Non-leaf: one sub-problem at current depth
                    const uint8_t l2bl = uint8_t(depth * l2baseradix);
                    const bfly_pass pass(cl2radix<l2baseradix>{}, uint8_t(l2fmb - l2bl), l2bl);
                    twiddle = fn(pass, process_width, total_width, 1, offset, twiddle);
                },
                [&]() KFR_INLINE_LAMBDA { // Root: one block, all butterflies
                    const bfly_pass pass(cl2radix<l2baseradix>{}, l2fmb, cl2blocks<0>{});
                    twiddle = fn(pass, process_width, total_width, 1, offset, twiddle);
                });
        }
        else // DIF
        {
            const uint8_t max_depth = l2items / l2baseradix;

            size_t offset = 0;

            traverse_parentfirst<R>(
                l2items,
                [&]() KFR_INLINE_LAMBDA { // Root: one block, all butterflies
                    const bfly_pass pass(cl2radix<l2baseradix>{}, l2fmb, cl2blocks<0>{});
                    // DIF root is the first-touch pass: it reads fresh input data spanning all blocks.
                    twiddle = fn(pass, process_width, total_width, 1, offset, twiddle);
                },
                [&](uint8_t depth) KFR_INLINE_LAMBDA { // Non-leaf: one sub-problem at current depth
                    const uint8_t l2bl = uint8_t(depth * l2baseradix);
                    const bfly_pass pass(cl2radix<l2baseradix>{}, uint8_t(l2fmb - l2bl), l2bl);
                    twiddle = fn(pass, process_width, total_width, 1, offset, twiddle);
                },
                [&](uint8_t levels_up) KFR_INLINE_LAMBDA { // Merged leaf: all R siblings at once
                    const bfly_pass pass(cl2radix<half.l2remaining>{}, cl2butterflies<0>{},
                                         uint8_t(l2fftsize - half.l2remaining));
                    fn(pass, process_width, total_width, R, offset, twiddle);
                    offset += total_width << (l2baseradix + pass.l2block_size());
                    if (levels_up < max_depth)
                    {
                        twiddle -= (size_t(1) << (half.l2remaining + l2baseradix * levels_up)) -
                                   (size_t(1) << half.l2remaining) - (R - 1) * levels_up;
                    }
                });
        }
    }

    return twiddle;
}

template <dft_traits traits, bool inverse = false, dft_decomp dir, dft_sandwich_half half,
          bool matrix_twiddles, uint8_t l2fixedstride, bool inplace>
KFR_INTRINSIC const complex<typename traits::type>* sandwich_half(
    uint8_t l2size, size_t stride, complex<typename traits::type>* out_,
    const complex<typename traits::type>* in_, const complex<typename traits::type>* twiddle)
{
    constexpr size_t prefetch = 0; // cfg.prefetch_offset;
    using namespace intr;

    using T = typename traits::type;

    return sandwich_iterate_recursive<traits, dir, half>(
        l2size, stride, twiddle,
        [l2size, in_, out_]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(
            const bfly_pass<l2passradix, l2bf, l2bl>& pass, size_t lane_width, size_t stride, size_t blocks,
            size_t offset, const complex<T>* twiddle) KFR_INLINE_LAMBDA
        {
            using pass_t = std::decay_t<decltype(pass)>;
            // Only the first pass to touch a given region reads the (possibly separate) input buffer.
            // Subsequent passes operate in place on the output buffer where the data now lives.
            const complex<T>* in =
                inplace
                    ? out_ + offset
                    : ((pass_t::pass_type(dir) & dft_pass_type::first) == dft_pass_type::first ? in_ : out_) +
                          offset;
            complex<T>* out = out_ + offset;
            KFR_ASSUME(blocks > 0);
            constexpr size_t R           = pass_t::radix();
            constexpr uint8_t l2maxwidth = l2fixedstride ? l2fixedstride : traits::l2basewidth;
            constexpr size_t w           = 1ull << std::min(traits::l2basewidth, l2maxwidth);

            if constexpr (l2fixedstride != 0)
            {
                KFR_ASSUME(stride == (size_t(1) << l2fixedstride));
                KFR_ASSUME(lane_width == (size_t(1) << l2fixedstride));
            }

            constexpr size_t u0 = size_t(1) << std::max(int(l2fixedstride) - int(traits::l2basewidth), 0);
            constexpr size_t u  = u0 <= 4 ? u0 : 1;

            constexpr bool split_format = true;

            if constexpr (half.single_pass && matrix_twiddles)
            {
                static_assert(dir == dft_decomp::dit, "Matrix twiddles are only applied on the DIT half");

                bfly_loop<R, T, w, u>( //
                    lane_width, //
                    bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::matrix, dir, false, false,
                                       prefetch, inplace, true>{ out, in, stride, twiddle + offset * R });
                twiddle += lane_width * R;
            }
            else if constexpr (half.single_pass)
            {
                static_assert(traits::l2basewidth + traits::l2baseradix >= l2passradix,
                              "Single pass should be large enough to benefit from parallelism");
                constexpr size_t w =
                    1ull << std::min(uint8_t(traits::l2basewidth + 1 + traits::l2baseradix - l2passradix),
                                     l2maxwidth);
                constexpr size_t u0 = std::max(size_t(1), (size_t(1) << l2fixedstride) / w);
                constexpr size_t u  = u0 <= 4 ? u0 : 1;
                static_assert(u > 0, "Invalid fixed stride");
                bfly_loop<R, T, w, u>( //
                    lane_width, //
                    bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::none, dir, false, false,
                                       prefetch, inplace>{ out, in, stride });
            }
            else if constexpr (pass_t::has_one_block())
            {
                // DIF: first pass, DIT: last pass
                const size_t pass_stride = stride << pass.l2stride();
                size_t offs              = 0;

                size_t butterflies = pass.butterflies();
                KFR_ASSUME(butterflies > 0);

                {
                    bfly_loop<R, T, w, u>( //
                        lane_width, //
                        bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::none, dir,
                                           dir == dft_decomp::dit, dir == dft_decomp::dif, prefetch, inplace>{
                            out + offs, in + offs, pass_stride });
                    offs += stride;
                }

                for (size_t e = 1; e < butterflies; ++e)
                {
                    bfly_loop<R, T, w, u>( //
                        lane_width, //
                        bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::scalar, dir,
                                           dir == dft_decomp::dit, dir == dft_decomp::dif, prefetch, inplace>{
                            out + offs, in + offs, pass_stride, twiddle });
                    twiddle += R - 1;
                    offs += stride;
                }
            }
            else if constexpr (pass_t::has_one_butterfly())
            {
                // DIF: last pass, DIT: first pass
                // 1 butterfly per block, no twiddles
                size_t offs = 0;
                KFR_ASSUME(blocks > 0);

                if constexpr (matrix_twiddles)
                {
                    uint8_t l2stride = countr_zero(stride);

                    const size_t b_offset    = offset >> (l2stride + pass.l2block_size());
                    const size_t lane_offset = offset & (stride - 1);

                    for (size_t b = 0; b < blocks; ++b)
                    {
                        bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::matrix, dir, false, true,
                                           prefetch, inplace>
                            bf{ out + offs, in + offs, stride,
                                twiddle + ((b + b_offset) * stride + lane_offset) * R };

                        bfly_loop<R, T, w, u>( //
                            lane_width, //
                            bf);
                        offs += stride << pass.l2block_size();
                    }
                    twiddle += pass.blocks() * stride * R;
                }
                else
                {
                    for (size_t b = 0; b < blocks; ++b)
                    {
                        bfly_loop<R, T, w, u>( //
                            lane_width, //
                            bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::none, dir,
                                               dir == dft_decomp::dif, dir == dft_decomp::dit, prefetch,
                                               inplace>{ out + offs, in + offs, stride });

                        offs += stride << pass.l2block_size();
                    }
                }
            }
            else
            {
                // Middle pass
                const size_t pass_stride = stride << pass.l2stride();
                size_t offs              = 0;

                size_t butterflies = pass.butterflies();
                KFR_ASSUME(butterflies > 0);
                size_t io_step = stride << pass.l2block_size();

                {
                    bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::none, dir, split_format,
                                       split_format, prefetch, inplace>
                        bfly{ out + offs, in + offs, pass_stride };
                    for (size_t b = 0; b < blocks; ++b)
                    {
                        bfly.out = out + offs + b * io_step;
                        if (!inplace)
                            bfly.in = in + offs + b * io_step;
                        bfly_loop<R, T, w, u>( //
                            lane_width, //
                            bfly);
                    }
                    offs += stride;
                }

                for (size_t e = 1; e < butterflies; ++e)
                {
                    bfly_parallel_bfly<R, T, w, inverse, bfly_twiddles_type::scalar, dir, split_format,
                                       split_format, prefetch, inplace>
                        bfly{ out + offs, in + offs, pass_stride, twiddle };
                    for (size_t b = 0; b < blocks; ++b)
                    {
                        bfly.out = out + offs + b * io_step;
                        if (!inplace)
                            bfly.in = in + offs + b * io_step;
                        bfly_loop<R, T, w, u>( //
                            lane_width, //
                            bfly);
                    }
                    twiddle += R - 1;
                    offs += stride;
                }
            }
            return twiddle;
        });
}

template <dft_traits traits, uint8_t l2passradix, uint8_t l2fixedstride>
constexpr size_t sandwich_dif_bitrev_width() noexcept
{
    constexpr uint8_t l2maxwidth = l2fixedstride ? l2fixedstride : traits::l2basewidth;
    return size_t(1) << std::min(uint8_t(traits::l2basewidth + traits::l2baseradix - l2passradix),
                                 l2maxwidth);
}

template <typename T, size_t count, size_t N>
struct stride_rw
{
    static_assert(std::has_single_bit(count), "count must be a power of 2");
    static_assert(std::has_single_bit(N), "N must be a power of 2");

    KFR_MEM_INTRINSIC stride_rw(size_t stride_) noexcept //
        : stride(stride_), stride2(stride_ * 2)
    {
    }

    KFR_MEM_INTRINSIC void read(const complex<T>* in, cvec<T, N> w[count]) noexcept
    {
        KFR_FOR(i, 0, count / 2)
        {
            w[i * 2]     = intr::cread<N>(in);
            w[i * 2 + 1] = intr::cread<N>(in + stride);
            in += stride2;
        };
    }
    KFR_MEM_INTRINSIC void write(complex<T>* out, const cvec<T, N> w[count]) noexcept
    {
        KFR_FOR(i, 0, count / 2)
        {
            intr::cwrite<N>(out, w[i * 2]);
            intr::cwrite<N>(out + stride, w[i * 2 + 1]);
            out += stride2;
        };
    }

    KFR_MEM_INTRINSIC cvec<T, N * count> read(const complex<T>* in) noexcept
    {
        cvec<T, N * count> result;
        cvec<T, N> w[count];
        read(in, w);
        return concat_native(result, w);
    }
    KFR_MEM_INTRINSIC void write(complex<T>* out, const cvec<T, N * count>& w) noexcept
    {
        cvec<T, N> ws[count];
        split_native(w, ws);
        write(out, ws);
    }

private:
    size_t stride;
    size_t stride2;
};

template <typename T, size_t N>
struct stride_rw<T, 1, N>
{
    static_assert(std::has_single_bit(N), "N must be a power of 2");

    KFR_MEM_INTRINSIC stride_rw(size_t stride_) noexcept //
    {
    }
    KFR_MEM_INTRINSIC void read(const complex<T>* in, cvec<T, N> w[1]) noexcept { w[0] = intr::cread<N>(in); }
    KFR_MEM_INTRINSIC cvec<T, N * 1> read(const complex<T>* in) noexcept { return intr::cread<N>(in); }
    KFR_MEM_INTRINSIC void write(complex<T>* out, const cvec<T, N> w[1]) noexcept
    {
        intr::cwrite<N>(out, w[0]);
    }
    KFR_MEM_INTRINSIC void write(complex<T>* out, const cvec<T, N * 1>& w) noexcept
    {
        intr::cwrite<N>(out, w);
    }
};

template <typename T, bool inverse, size_t w, size_t R, uint8_t l2passradix, typename Reader, typename Writer>
KFR_INTRINSIC void sandwich_dif_merged_bitrev_lane(Reader& reader, Writer& writer, const complex<T>* in,
                                                   complex<T>* out)
{
    using namespace intr;
    [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA
    {
        cvec<T, w> ws[R];
        reader.read(in, ws);
        bfly<inverse, w>(cfalse, ws[I]...);
        cvec<T, w * R> ww = concat(ws[bitreverse<l2passradix>(I)]...);
        ww                = bitreverse<2>(ww);
        writer.write(out, ww);
    }(csizeseq<R>);
}

struct bitrev_generator
{
    uint32_t j;
    uint32_t M;

    bitrev_generator(uint8_t bits)
    {
        j = 0;
        M = 1u << bits;
    }

    uint32_t next(uint32_t i)
    {
        uint32_t k = tzcnt_u32(i);
        j ^= M - (M >> (k + 1));
        return j;
    }
};

template <dft_traits traits, bool inverse, dft_sandwich_half half, uint8_t l2fixedstride>
KFR_INTRINSIC void sandwich_dif_merged_bitrev(size_t r2, complex<typename traits::type>* out,
                                              const complex<typename traits::type>* in)
{
    using namespace intr;
    using T = typename traits::type;

    static_assert(half.single_pass, "sandwich_dif_merged_bitrev requires a single-pass DIF half");

    constexpr uint8_t l2passradix = half.l2remaining;
    constexpr size_t R            = size_t(1) << l2passradix;
    constexpr size_t w            = sandwich_dif_bitrev_width<traits, l2passradix, l2fixedstride>();
    constexpr uint8_t l2w         = static_cast<uint8_t>(ilog2(w));

    KFR_ASSUME(r2 >= w);

    const uint8_t l2r2      = l2fixedstride != 0 ? l2fixedstride : uint8_t(countr_zero(r2));
    const uint8_t l2c_bits  = uint8_t(l2r2 - l2w);
    const size_t row_stride = (r2 >> l2w) * R;
    stride_rw<T, R, w> reader(r2);
    stride_rw<T, w, R> writer(row_stride);

    if (l2c_bits == 0) [[unlikely]]
    {
        sandwich_dif_merged_bitrev_lane<T, inverse, w, R, l2passradix>(reader, writer, in, out);
    }
    else
    {
        bitrev_generator gen(l2c_bits - 1);
        uint32_t m = (1u << uint8_t(l2c_bits - 1));
        uint32_t j = 0;
        for (size_t i = 0; i < (r2 >> l2w);)
        {
            sandwich_dif_merged_bitrev_lane<T, inverse, w, R, l2passradix>(reader, writer, in + i * w,
                                                                           out + j * R);
            i++;
            sandwich_dif_merged_bitrev_lane<T, inverse, w, R, l2passradix>(reader, writer, in + i * w,
                                                                           out + (j + m) * R);
            i++;
            j = gen.next(uint32_t(i >> 1));
        }
    }
}

template <dft_traits traits, bool inverse = false, dft_config<dft_family::fourstep> cfg>
KFR_INLINE void sandwich(complex<typename traits::type>* out, const complex<typename traits::type>* in,
                         uint8_t l2fftsize, const complex<typename traits::type>* twiddle)
{
    using namespace intr;

    using T = typename traits::type;
    if (l2fftsize < 4) [[unlikely]]
        return;
    const auto [l2r1, l2r2] = sandwich_split_size_rt<traits, cfg>(l2fftsize);
    const size_t r1         = 1ull << l2r1;
    const size_t r2         = 1ull << l2r2;
    KFR_ASSUME(r1 > 0);
    KFR_ASSUME(r2 > 0);
    constexpr size_t prefetch = 0; // cfg.prefetch_offset;

    constexpr uint8_t l2fixedstride1 = cfg.dif.l2size != UINT8_MAX ? cfg.dif.l2size : 0;
    constexpr uint8_t l2fixedstride2 = cfg.dit.l2size != UINT8_MAX ? cfg.dit.l2size : 0;

    // DIF
    bool dif_bitrev_merged = false;
    if constexpr (cfg.dif.single_pass)
    {
        constexpr size_t dif_bitrev_w =
            sandwich_dif_bitrev_width<traits, cfg.dif.l2remaining, l2fixedstride2>();
        if (in != out && r2 >= dif_bitrev_w)
        {
            sandwich_dif_merged_bitrev<traits, inverse, cfg.dif, l2fixedstride2>(r2, out, in);
            dif_bitrev_merged = true;
        }
    }
    if (!dif_bitrev_merged)
    {
        twiddle = sandwich_half<traits, inverse, dft_decomp::dif, cfg.dif, false, l2fixedstride2, false>(
            l2r1, r2, out, in, twiddle);
    }

    const size_t fftsize = 1ull << l2fftsize;

    if (!dif_bitrev_merged)
    {
        if constexpr (cfg.l2fftsize() != UINT8_MAX)
            intr::br(std::span<complex<T>, (1ull << cfg.l2fftsize())>{ out, 1ull << cfg.l2fftsize() });
        else
            intr::br(std::span<complex<T>>{ out, fftsize });
    }

    constexpr bool twiddle_pass = cfg.dit.single_pass && cfg.fixed_twiddles();

    if constexpr (twiddle_pass)
    {
        constexpr size_t r1 = 1ull << cfg.dif.l2size;
        constexpr size_t r2 = 1ull << cfg.dit.l2size;

        constexpr size_t l2complexcapacity = ilog2(vector_capacity<typename traits::type> / 2);
        constexpr uint8_t l2mulwidth       = std::min(uint8_t(l2complexcapacity - 1), cfg.dif.l2size);

        constexpr size_t w = 1ull << l2mulwidth;

        // known at compile time
        KFR_FOR(i, 1, r2)
        {
            constexpr size_t ii = bitreverse<cfg.dit.l2size>(i);

            KFR_FOR(j, 0, r1 / w)
            {
                constexpr size_t jj = j * w;
                complex<T>* io      = out + i * r1 + jj;
                cvec<T, w> v        = cread<w>(io);
                v = cmuli<inverse>(cfalse, v, fixed_twiddle<T, w, r1 * r2, jj * ii, ii, false>());
                cwrite<w>(io, v);
            };
        };
    }

    twiddle = align_up(twiddle, KFR_CACHE_LINE_SIZE);

    // DIT
    sandwich_half<traits, inverse, dft_decomp::dit, cfg.dit, !twiddle_pass, l2fixedstride1, true>(
        l2r2, r1, out, out, twiddle);
}

template <dft_traits traits, dft_config<dft_family::fourstep> cfg, bool inverse>
void ng_do_dft(const ngfft_plan<typename traits::type>& plan, std::complex<typename traits::type>* out,
               const std::complex<typename traits::type>* in) noexcept
{
    sandwich<traits, inverse, cfg>(out, in, plan.l2fftsize, plan.twiddles);
}

template <dft_traits traits>
void ng_do_init_dft(const ngfft_plan<typename traits::type>& plan,
                    const dft_config<dft_family::fourstep>& cfg) noexcept
{
    sandwich_prepare<traits>(plan.twiddles, plan.l2fftsize, cfg);
}

template <dft_traits traits>
constexpr size_t ng_twiddle_count(uint8_t l2fftsize, const dft_config<dft_family::fourstep>& cfg) noexcept
{
    return sandwich_twiddle_size<traits>(l2fftsize, cfg);
}

template <dft_traits traits, dft_config<dft_family::fourstep> cfg, uint8_t l2fftsize, bool inverse>
void ng_do_fixed_dft(const ngfft_plan<typename traits::type>& plan, std::complex<typename traits::type>* out,
                     const std::complex<typename traits::type>* in) noexcept
{
    sandwich<traits, inverse, cfg>(out, in, l2fftsize, plan.twiddles);
}

} // namespace KFR_ARCH_NAME

template <dft_traits traits>
constexpr dft_config<dft_family::fourstep> ng_config(cval_t<dft_family, dft_family::fourstep>,
                                                     uint8_t l2fftsize) noexcept
{
    const auto [l2r1, l2r2] = sandwich_split_size<traits>(l2fftsize);

    return {
        .dif = get_sandwich_half<traits>(l2r1),
        .dit = get_sandwich_half<traits>(l2r2),

        .prefetch_offset = 8, //
    };
}

} // namespace kfr
