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

#include "ngfft.hpp"

namespace kfr
{

template <>
struct dft_config<dft_family::mixedradix>
{
    uint8_t l2remainingradix;
    uint8_t l2chunksize;
    size_t prefetch_offset;
    bool use_split;
    uint8_t small_stride_passes;
    bool all_small_stride_passes;
};

inline namespace KFR_ARCH_NAME
{

namespace intr
{

template <size_t Radix, typename T, size_t N, size_t split_width, bool split_format, bitrev_permute bitrev,
          uintptr_t prefetch = 0, size_t fixed_stride = 0>
struct bfly_read2
{
    static_assert(split_width >= 1);
    static_assert(fixed_stride <= N);
    const std::complex<T>* in;
    size_t offset;

    KFR_INLINE_MEMBER bfly_read2(const std::complex<T>* in, uint8_t /* l2butterflies */, size_t offset = 0)
        : in(in), offset(offset)
    {
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        if constexpr (prefetch > 0)
            prefetch_one<Radix * N>(in + offset * Radix + N * Radix * prefetch);

        ww = cread_group2<Radix, T, N, split_width, split_format, bitrev>(in + offset * Radix,
                                                                          csize<fixed_stride>);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

template <size_t Radix, typename T, size_t N, size_t split_width, bool split_format, bitrev_permute bitrev,
          uintptr_t prefetch>
struct bfly_read2<Radix, T, N, split_width, split_format, bitrev, prefetch, 0>
{
    static_assert(split_width >= 1);
    const std::complex<T>* in;
    size_t stride;

    size_t mask;
    size_t offset;
    constexpr static size_t gap = ilog2(Radix);

    KFR_INLINE_MEMBER bfly_read2(const std::complex<T>* in, uint8_t l2butterflies, size_t offset = 0)
        : in(in), offset(offset)
    {
        stride = 1u << l2butterflies;
        mask   = stride - 1;
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        size_t memory_offset = ((offset & ~mask) << gap) | (offset & mask);

        const std::complex<T>* in = this->in + memory_offset;
        if constexpr (prefetch > 0)
            cprefetch<Radix, N>(in + N * prefetch, stride);

        ww = cread_group2<Radix, T, N, split_width, split_format, bitrev>(in, stride);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

template <size_t Radix, typename T, size_t N, size_t split_width, bool split_format, bitrev_permute bitrev,
          uintptr_t prefetch>
struct bfly_read2<Radix, T, N, split_width, split_format, bitrev, prefetch, SIZE_MAX>
{
    static_assert(split_width >= 1);
    // Single-block specialization
    const std::complex<T>* in;
    size_t stride;
    size_t offset;

    KFR_INLINE_MEMBER bfly_read2(const std::complex<T>* in, uint8_t l2butterflies, size_t offset = 0)
        : in(in), offset(offset)
    {
        stride = 1u << l2butterflies;
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        size_t memory_offset      = offset;
        const std::complex<T>* in = this->in + memory_offset;
        if constexpr (prefetch > 0)
            cprefetch<Radix, N>(in + N * prefetch, stride);

        ww = cread_group2<Radix, T, N, split_width, split_format, bitrev>(in, stride);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

/**
 * @brief Butterfly write step with bit-reversal and transposition.
 */
template <size_t Radix, typename T, size_t N, size_t split_width, bool split_format, bitrev_permute bitrev,
          size_t fixed_stride = 0>
struct bfly_write2
{
    static_assert(split_width >= 1);
    static_assert(fixed_stride <= N);
    std::complex<T>* out;
    size_t offset;
    KFR_INLINE_MEMBER bfly_write2(std::complex<T>* out, uint8_t /* l2butterflies */, size_t offset = 0)
        : out(out), offset(offset)
    {
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        cwrite_group2<Radix, T, N, split_width, split_format, bitrev>(out + offset * Radix, ww,
                                                                      csize<fixed_stride>);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

/**
 * @brief Butterfly write step
 */
template <size_t Radix, typename T, size_t N, size_t split_width, bool split_format, bitrev_permute bitrev>
struct bfly_write2<Radix, T, N, split_width, split_format, bitrev, 0>
{
    static_assert(split_width >= 1);
    std::complex<T>* out;
    size_t stride;
    size_t mask;
    constexpr static size_t gap = ilog2(Radix);
    size_t offset;

    KFR_INLINE_MEMBER bfly_write2(std::complex<T>* out, size_t l2butterflies, size_t offset = 0)
        : out(out), offset(offset)
    {
        stride = 1u << l2butterflies;
        mask   = stride - 1;
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        KFR_BFLY_TRACE("bfly_write: out=", fmt<'x'>(uintptr_t(out)), " stride=", stride);
        complex<T>* out = this->out + (((offset & ~mask) << gap) | (offset & mask));

        cwrite_group2<Radix, T, N, split_width, split_format, bitrev>(out, ww, stride);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

/**
 * @brief Butterfly write step
 */
template <size_t Radix, typename T, size_t N, size_t split_width, bool split_format, bitrev_permute bitrev>
struct bfly_write2<Radix, T, N, split_width, split_format, bitrev, SIZE_MAX>
{
    std::complex<T>* out;
    size_t stride;
    size_t offset;

    KFR_INLINE_MEMBER bfly_write2(std::complex<T>* out, size_t l2butterflies, size_t offset = 0)
        : out(out), offset(offset)
    {
        stride = 1u << l2butterflies;
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        KFR_BFLY_TRACE("bfly_write: out=", fmt<'x'>(uintptr_t(out)), " stride=", stride);
        complex<T>* out = this->out + offset;

        cwrite_group2<Radix, T, N, split_width, split_format, bitrev>(out, ww, stride);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

template <size_t Radix, typename T, size_t N, bool inverse, size_t split_width, bool split_format,
          size_t fixed_stride = 0>
struct bfly_twiddle2
{
    const std::complex<T>* twiddles;

    KFR_INLINE_MEMBER bfly_twiddle2(const std::complex<T>* twiddles, size_t /* l2butterflies */,
                                    size_t /* offset */ = 0) noexcept
        : twiddles(twiddles)
    {
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA
        {
            cvec<T, N> w[Radix];
            split(ww, w[I]...);
            ((I == 0 ? void()
                     : (w[I] = cmuli<inverse>(cbool<split_format>, w[I],
                                              cread<N>(twiddles + N * (std::max(I, size_t(1)) - 1))),
                        void())),
             ...);
            ww = concat(w[I]...);
        }(csizeseq<Radix>);
    }
    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept {}
};

template <size_t Radix, typename T, size_t N, bool inverse, size_t split_width, bool split_format>
struct bfly_twiddle2<Radix, T, N, inverse, split_width, split_format, 0>
{
    const std::complex<T>* twiddles;
    size_t mask;
    size_t offset;

    bfly_twiddle2(const std::complex<T>* twiddles, size_t l2butterflies, size_t offset = 0) noexcept
        : twiddles(twiddles), offset(offset)
    {
        mask = (1u << l2butterflies) - 1;
    }

    cvec<T, N> read_tw(const std::complex<T>* tw) const noexcept
    {
        cvec<T, N> val = cread<N>(tw);
        return val;
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        const std::complex<T>* tw = twiddles + (offset & mask) * (Radix - 1);

        [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA
        {
            cvec<T, N> w[Radix];
            split(ww, w[I]...);
            ((I == 0 ? void()
                     : (w[I] = cmuli<inverse>(cbool<split_format>, w[I], read_tw(tw)), tw += N, void())),
             ...);
            ww = concat(w[I]...);
        }(csizeseq<Radix>);
    }
    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { offset += N; }
};

/**
 * @brief Butterfly compute step
 */
template <size_t Radix, typename T, size_t N, bool inverse, dft_decomp decomp, bool split_format = false,
          size_t prefetch = 0>
struct bfly_combined_bfly
{
    complex<T>* inout;
    size_t stride;
    const complex<T>*& tw;

    constexpr static size_t br(size_t n) noexcept { return bitreverse<ilog2(Radix)>(n); }

    template <size_t, size_t... I>
    KFR_INLINE_MEMBER void read_all(cvec<T, N> w[Radix])
    {
        complex<T>* in = inout;
        w[0]           = cread_prefetch<N, prefetch>(in), in += stride;
        if constexpr (decomp == dft_decomp::dit)
        {
            ((w[br(I)] = cmuli<inverse>(cbool<split_format>, cread_prefetch<N, prefetch>(in),
                                        cread<N>(tw + (br(I) - 1) * N)),
              in += stride),
             ...);
            tw += (Radix - 1) * N;
        }
        else
        {
            ((w[I] = cread_prefetch<N, prefetch>(in), in += stride), ...);
        }
    }

    template <size_t, size_t... I>
    KFR_INLINE_MEMBER void write_all(cvec<T, N> w[Radix])
    {
        complex<T>* out = inout;
        cwrite<N, false>(out, w[0]);
        out += stride;
        if constexpr (decomp == dft_decomp::dif)
        {
            ((cwrite<N, false>(out,
                               cmuli<inverse>(cbool<split_format>, w[br(I)], cread<N>(tw + (br(I) - 1) * N))),
              out += stride),
             ...);
            tw += (Radix - 1) * N;
        }
        else
        {
            ((cwrite<N, false>(out, w[I]), out += stride), ...);
        }
    }

    KFR_INLINE_MEMBER void operator()(cvec<T, N * Radix>& ww) noexcept
    {
        [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA
        {
            cvec<T, N> w[Radix];

            read_all<I...>(w);

            KFR_BFLY_TRACE("bfly_bfly: Radix=", Radix);
            bfly<inverse, N>(cbool<split_format>, w[I]...);

            write_all<I...>(w);
        }(csizeseq<Radix>);
    }

    KFR_INLINE_MEMBER void begin() noexcept {}
    KFR_INLINE_MEMBER void end() noexcept {}
    KFR_INLINE_MEMBER void advance() noexcept { inout += N; }
};
} // namespace intr

template <dft_traits traits, typename Fn>
KFR_INTRINSIC constexpr void mixedradix_iterate_callback(uint8_t l2fftsize,
                                                         const dft_config<dft_family::mixedradix>& cfg,
                                                         Fn&& fn) noexcept
{
    constexpr dft_decomp dir           = algo_dir<traits>();
    constexpr uint8_t l2baseradix      = traits::l2baseradix;
    constexpr uint8_t l2maxradix       = traits::l2maxradix;
    constexpr uint8_t l2basewidth      = traits::l2basewidth;
    const uint8_t l2small_stride_total = cfg.l2remainingradix + cfg.small_stride_passes * l2baseradix;

    const uint8_t regular_passes = (l2fftsize - l2maxradix + l2baseradix - 1) / l2baseradix;

    if (l2fftsize <= l2maxradix)
    {
        cswitch(cvalseq<uint8_t, l2maxradix, 1>, cfg.l2remainingradix,
                [&]<uint8_t rem>(cval_t<uint8_t, rem>) KFR_INLINE_LAMBDA { //
                    fn(bfly_pass(cl2radix<rem>{}, cl2butterflies<0>{}, cl2blocks<0>{}));
                });
        return;
    }

    if constexpr (dir == dft_decomp::dit)
    {
        cswitch(cvalseq<uint8_t, l2maxradix + 1 - l2baseradix, l2baseradix>, cfg.l2remainingradix,
                [&]<uint8_t rem>(cval_t<uint8_t, rem>) KFR_INLINE_LAMBDA { //
                    fn(bfly_pass(cl2radix<rem>{}, cl2butterflies<0>{},
                                 uint8_t(l2fftsize - cfg.l2remainingradix)));
                });

        uint8_t l2stride = cfg.l2remainingradix;

        while (l2stride < l2fftsize - l2baseradix)
        {
            fn(bfly_pass(cl2radix<l2baseradix>{}, l2stride, uint8_t(l2fftsize - l2stride - l2baseradix)));
            l2stride += l2baseradix;
        }

        fn(bfly_pass(cl2radix<l2baseradix>{}, l2stride, cl2blocks<0>{}));
    }
    else
    {
        fn(bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2fftsize - l2baseradix), cl2blocks<0>{}));
        uint8_t l2blocks = l2baseradix;

        while (l2blocks < l2fftsize - cfg.l2remainingradix)
        {
            fn(bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2fftsize - l2blocks - l2baseradix), l2blocks));
            l2blocks += l2baseradix;
        }

        cswitch(cvalseq<uint8_t, l2maxradix + 1 - l2baseradix, l2baseradix>, cfg.l2remainingradix,
                [&]<uint8_t rem>(cval_t<uint8_t, rem>) KFR_INLINE_LAMBDA { //
                    fn(bfly_pass(cl2radix<rem>{}, cl2butterflies<0>{},
                                 uint8_t(l2fftsize - cfg.l2remainingradix)));
                });
    }
}

template <size_t fixed_stride>
void expose_fixed_stride()
{
}

template <dft_traits traits, bool inverse, bool use_split, size_t prefetch_offset, uint8_t l2radix,
          uint8_t l2bf, uint8_t l2bl>
KFR_INTRINSIC void mixedradix_body(std::complex<typename traits::type>* inout,
                                   const bfly_pass<l2radix, l2bf, l2bl>& pass, size_t num_blocks,
                                   const std::complex<typename traits::type>* twiddles)
{
    using T                              = typename traits::type;
    constexpr uint8_t l2baseradix        = traits::l2baseradix;
    constexpr uint8_t l2width            = l2pass_width<traits, l2radix>();
    constexpr size_t width               = 1u << l2width;
    constexpr size_t split_width         = use_split ? 1u << traits::l2basesplitwidth : 1;
    constexpr bool split_format          = use_split && split_width > 1 && l2width >= traits::l2minsplitwidth;
    constexpr size_t twiddle_split_width = std::min(split_width, width);
    constexpr dft_decomp dir             = algo_dir<traits>();
    using namespace intr;
    using enum bitrev_permute;

    constexpr size_t fixed_stride_orig = l2bf == 0xff ? 0 : size_t(1) << (l2bf & 31u);
    constexpr size_t fixed_stride = fixed_stride_orig <= 1 << traits::l2basewidth ? fixed_stride_orig : 0;

    using pass_t = std::decay_t<decltype(pass)>;

    constexpr dft_pass_type pass_type = pass_t::pass_type(dir);

    constexpr size_t R = 1u << l2radix;
    // larger radix → smaller width
    constexpr size_t in_split_width  = pass_type == dft_pass_type::first ? 1 : split_width;
    constexpr size_t out_split_width = pass_type == dft_pass_type::last ? 1 : split_width;
    constexpr uint8_t l2fixed_stride = fixed_stride == SIZE_MAX ? 0 : ilog2(fixed_stride);

    KFR_EXPOSE_VALUE(&expose_fixed_stride<fixed_stride>);

    const size_t butterflies_v    = fixed_stride == 0 ? pass.butterflies() : fixed_stride;
    const uint8_t l2butterflies_v = fixed_stride == 0 ? pass.l2butterflies() : l2fixed_stride;

    const size_t block_size = butterflies_v * R;

    if constexpr (pass_type == dft_pass_type::middle && (fixed_stride == 0 || fixed_stride >= width) &&
                  (!split_format || split_width == width))
    {
        KFR_ASSUME(num_blocks > 0);

        for (size_t b = 0; b < num_blocks; ++b)
        {
            const std::complex<T>* tw = twiddles;
            bfly_loop<R, T, width>( //
                butterflies_v, //
                bfly_combined_bfly<R, T, width, inverse, dir, split_format, prefetch_offset>{
                    inout,
                    butterflies_v,
                    tw,
                });
            inout += block_size;
        }
    }
    else
    {
        constexpr bool packed           = fixed_stride == 1 && width < complex_vector_width<T>;
        constexpr size_t stride_v       = packed ? width // Disable transpose if packed
                                          : fixed_stride == 0 && pass_t::has_one_block() ? SIZE_MAX
                                          : fixed_stride > width                         ? 0
                                                                                         : fixed_stride;
        constexpr bool use_split_format = split_format && !packed;

        constexpr bitrev_permute bitrev    = packed ? bitrev_permute::packed : bitrev_permute::parallel;
        constexpr bitrev_permute rd_bitrev = dir == dft_decomp::dit ? bitrev : bitrev_permute::none;
        constexpr bitrev_permute wr_bitrev = dir == dft_decomp::dif ? bitrev : bitrev_permute::none;

        bfly_read2<R, T, width, in_split_width, use_split_format, rd_bitrev, prefetch_offset, stride_v> rd{
            inout,
            l2butterflies_v,
            0,
        };
        std::conditional_t<packed, bfly_bfly_packed<R, T, width, inverse>,
                           bfly_bfly<R, T, width, inverse, split_format>>
            bf{};
        bfly_write2<R, T, width, out_split_width, use_split_format, wr_bitrev, stride_v> wr{
            inout,
            l2butterflies_v,
            0,
        };

        const size_t count = num_blocks << pass.l2butterflies();

        if constexpr (!pass_t::has_one_butterfly())
        {
            bfly_twiddle2<R, T, width, inverse, twiddle_split_width, split_format, fixed_stride> tw{
                twiddles,
                l2butterflies_v,
                0,
            };
            if constexpr (dir == dft_decomp::dif)
            {
                bfly_loop<R, T, width>(count, rd, bf, tw, wr);
            }
            else
            {
                bfly_loop<R, T, width>(count, rd, tw, bf, wr);
            }
        }
        else
        {
            bfly_loop<R, T, width>(count, rd, bf, wr);
        }
    }
}

template <dft_traits traits, bool inverse, bool use_split = true, size_t prefetch_offset = 0, uint8_t l2radix,
          uint8_t l2bf, uint8_t l2bl>
KFR_INTRINSIC void mixedradix_pass(std::complex<typename traits::type>* inout,
                                   const bfly_pass<l2radix, l2bf, l2bl>& pass, size_t num_blocks,
                                   const std::complex<typename traits::type>*& twiddles)
{
    ffttimes.record("pass_start");

    using T                   = typename traits::type;
    constexpr uint8_t l2width = l2pass_width<traits, l2radix>();

    mixedradix_body<traits, inverse, use_split, prefetch_offset>(inout, pass, num_blocks, twiddles);

    const size_t num_twiddles =
        pass.need_twiddles() ? std::max(size_t(1u << l2width), pass.butterflies()) * (pass.radix() - 1) : 0;
    twiddles += num_twiddles;

    ffttimes.record("pass_end");
}

template <dft_traits traits, uint8_t l2splitwidth, uint8_t l2radix, uint8_t l2bf, uint8_t l2bl>
void mixedradix_prepare_pass(ngfft_plan<typename traits::type>& plan,
                             const bfly_pass<l2radix, l2bf, l2bl>& pass, size_t twiddle_offset)
{
    using T                   = typename traits::type;
    constexpr uint8_t l2width = l2pass_width<traits, l2radix>();
    static_assert(l2width >= l2splitwidth);
    constexpr size_t width       = 1u << l2width;
    constexpr size_t split_width = 1u << l2splitwidth;

    for (size_t e = 0; e < pass.butterflies(); e += width)
    {
        for (size_t n = 1; n < pass.radix(); ++n)
        {
            cvec<T, width> twid;
            for (size_t i = 0; i < width; ++i)
            {
                cvec<T, 1> tw   = intr::calculate_twiddle<T>(pass.twiddle_k(e + i, n), pass.block_size());
                twid[i * 2]     = tw[0];
                twid[i * 2 + 1] = tw[1];
            }
            cwrite_group2<1, T, width, (l2width >= traits::l2minsplitwidth ? split_width : 1), false,
                          bitrev_permute::none>(plan.twiddles + twiddle_offset, twid, 0);
            twiddle_offset += width;
        }
    }
}

//--------------------------------------------------------------------------------------------

template <dft_traits traits>
constexpr size_t ng_twiddle_count(uint8_t l2fftsize, const dft_config<dft_family::mixedradix>& cfg) noexcept
{
    constexpr dft_decomp dir = algo_dir<traits>();
    size_t twiddle_size      = 0;
    mixedradix_iterate_callback<traits>( //
        l2fftsize, cfg,
        [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(
            const bfly_pass<l2passradix, l2bf, l2bl>& pass) constexpr
        {
            if (!pass.has_one_butterfly())
            {
                const size_t w = 1u << l2pass_width<traits, l2passradix>();
                twiddle_size += std::max(pass.butterflies(), w) * (pass.radix() - 1);
            }
        });
    return twiddle_size;
}

template <dft_traits traits>
void ng_do_init_dft(ngfft_plan<typename traits::type>& plan,
                    const dft_config<dft_family::mixedradix>& cfg) noexcept
{
    size_t twiddle_offset = 0;

    mixedradix_iterate_callback<traits>( //
        plan.l2fftsize, cfg,
        [&]<uint8_t l2passradix, uint8_t l2bf, uint8_t l2bl>(
            const bfly_pass<l2passradix, l2bf, l2bl>& pass) { //
            if constexpr (!std::decay_t<decltype(pass)>::has_one_butterfly())
            {
                if (cfg.use_split)
                    mixedradix_prepare_pass<traits, traits::l2basesplitwidth>(plan, pass, twiddle_offset);
                else
                    mixedradix_prepare_pass<traits, 0>(plan, pass, twiddle_offset);
                constexpr size_t w = 1u << l2pass_width<traits, l2passradix>();
                twiddle_offset += std::max(pass.butterflies(), w) * (pass.radix() - 1);
            }
        });
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, uint8_t l2fftsize, bool inverse>
KFR_INTRINSIC constexpr void mixedradix_iterate_static(
    cval_t<uint8_t, l2fftsize>, std::complex<typename traits::type>* inout,
    const std::complex<typename traits::type>*& twiddles) noexcept
{
    constexpr dft_decomp dir      = algo_dir<traits>();
    constexpr uint8_t l2baseradix = traits::l2baseradix;
    constexpr uint8_t l2maxradix  = traits::l2maxradix;
    constexpr uint8_t l2basewidth = traits::l2basewidth;

    if constexpr (l2fftsize <= l2maxradix)
    {
        const auto pass = bfly_pass{ cl2radix<l2fftsize>{}, cl2butterflies<0>{}, cl2blocks<0>{} };
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, pass, pass.blocks(),
                                                                             twiddles);
    }
    else
    {
        constexpr uint8_t regular_passes = (l2fftsize - l2maxradix + l2baseradix - 1) / l2baseradix;
        static_assert(regular_passes > 0);
        if constexpr (dir == dft_decomp::dit)
        {
            {
                const auto pass = bfly_pass(cl2radix<cfg.l2remainingradix>{}, cl2butterflies<0>{},
                                            cl2butterflies<l2baseradix * regular_passes>{});
                mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, pass,
                                                                                     pass.blocks(), twiddles);
            }
            KFR_FOR(p, 0, regular_passes)
            {
                constexpr uint8_t l2stride = cfg.l2remainingradix + p * l2baseradix;

                {
                    const auto pass = bfly_pass(cl2radix<l2baseradix>{}, cl2butterflies<l2stride>{},
                                                cl2blocks<((regular_passes - 1 - p) * l2baseradix)>{});
                    mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
                        inout, pass, pass.blocks(), twiddles);
                }
            };
        }
        else
        {
            // regular_passes of l2radix, then remaining
            KFR_FOR(p, 0, regular_passes)
            {
                constexpr uint8_t l2stride =
                    uint8_t(l2baseradix * (regular_passes - 1 - p) + cfg.l2remainingradix);

                {
                    const auto pass = bfly_pass(cl2radix<l2baseradix>{}, cl2butterflies<l2stride>{},
                                                cl2blocks<l2baseradix * p>{});
                    mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
                        inout, pass, pass.blocks(), twiddles);
                }
            };

            {
                const auto pass = bfly_pass(cl2radix<cfg.l2remainingradix>{}, cl2butterflies<0>{},
                                            cl2blocks<l2baseradix * regular_passes>{});
                mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, pass,
                                                                                     pass.blocks(), twiddles);
            }
        }
    }
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, uint8_t l2fftsize, bool inverse>
void ng_do_fixed_dft(const ngfft_plan<typename traits::type>& plan,
                     std::complex<typename traits::type>* inout) noexcept
{
    using T                       = typename traits::type;
    constexpr dft_decomp dir      = algo_dir<traits>();
    constexpr uint8_t l2baseradix = traits::l2baseradix;
    constexpr uint8_t l2maxradix  = traits::l2maxradix;
    constexpr uint8_t l2basewidth = traits::l2basewidth;
    static_assert(l2maxradix >= l2baseradix, "max radix must be at least the base radix");
    const std::complex<T>* twiddles = plan.twiddles;

    const std::span<std::complex<T>, 1 << l2fftsize> span{ inout, size_t(1) << l2fftsize };
    if constexpr (dir == dft_decomp::dit)
    {
        intr::br(span);
    }
    mixedradix_iterate_static<traits, cfg, l2fftsize, inverse>(cval<uint8_t, l2fftsize>, inout, twiddles);
    if constexpr (dir == dft_decomp::dif)
    {
        intr::br(span);
    }
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, bool inverse>
KFR_INTRINSIC void mixedradix_iterate(uint8_t l2fftsize, std::complex<typename traits::type>* inout,
                                      const std::complex<typename traits::type>*& twiddles) noexcept
{
    constexpr dft_decomp dir               = algo_dir<traits>();
    constexpr uint8_t l2baseradix          = traits::l2baseradix;
    constexpr uint8_t l2maxradix           = traits::l2maxradix;
    constexpr uint8_t l2basewidth          = traits::l2basewidth;
    constexpr uint8_t l2basesplitwidth     = traits::l2basesplitwidth;
    constexpr uint8_t l2small_stride_total = cfg.l2remainingradix + cfg.small_stride_passes * l2baseradix;

    static_assert(cfg.all_small_stride_passes);

    const uint8_t regular_passes = (l2fftsize - l2maxradix + l2baseradix - 1) / l2baseradix;

    if constexpr (dir == dft_decomp::dit)
    {
        {
            const auto p = bfly_pass(cl2radix<cfg.l2remainingradix>{}, cl2butterflies<0>{},
                                     l2fftsize - cfg.l2remainingradix);
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
        }

        KFR_FOR(pass_index, 0, cfg.small_stride_passes)
        {
            constexpr uint8_t l2stride = cfg.l2remainingradix + pass_index * l2baseradix;
            const auto p =
                bfly_pass(cl2radix<l2baseradix>{}, cl2radix<l2stride>{}, l2fftsize - l2stride - l2baseradix);

            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
        };

        uint8_t following = l2fftsize - l2baseradix - l2small_stride_total;
        while (following > 0)
        {
            const auto p = bfly_pass(cl2radix<l2baseradix>{}, l2fftsize - following - l2baseradix, following);
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
            following -= l2baseradix;
        }

        {
            const auto p = bfly_pass(cl2radix<l2baseradix>{}, l2fftsize - l2baseradix, cl2blocks<0>{});
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
        }
    }
    else
    {
        {
            const auto p = bfly_pass(cl2radix<l2baseradix>{}, l2fftsize - l2baseradix, cl2blocks<0>{});
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
        }

        uint8_t following = l2fftsize - l2baseradix - l2baseradix;
        while (following + l2baseradix > l2small_stride_total)
        {
            const auto p = bfly_pass(cl2radix<l2baseradix>{}, following, l2fftsize - following - l2baseradix);
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
            following -= l2baseradix;
        }

        KFR_FOR(pass_index, 0, cfg.small_stride_passes)
        {
            constexpr uint8_t l2stride =
                cfg.l2remainingradix + (cfg.small_stride_passes - 1 - pass_index) * l2baseradix;
            const auto p = bfly_pass(cl2radix<l2baseradix>{}, cl2butterflies<l2stride>{},
                                     l2fftsize - (l2stride + l2baseradix));

            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
        };

        {
            const auto p = bfly_pass(cl2radix<cfg.l2remainingradix>{}, cl2butterflies<0>{},
                                     l2fftsize - cfg.l2remainingradix);
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, p, p.blocks(),
                                                                                 twiddles);
        }
    }
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, bool inverse, uint8_t l2radix,
          uint8_t l2bf, uint8_t l2bl>
KFR_INTRINSIC void mixedradix_iterate_dif_recursive_leaf(
    const bfly_pass<l2radix, l2bf, l2bl>& pass, std::complex<typename traits::type>* inout,
    const std::complex<typename traits::type>* twiddles) noexcept
{
    constexpr uint8_t l2baseradix          = traits::l2baseradix;
    constexpr uint8_t l2small_stride_total = cfg.l2remainingradix + cfg.small_stride_passes * l2baseradix;

    uint8_t l2fftsize = pass.l2fftsize();

    uint8_t following = pass.l2stride() - l2baseradix;
    while (following + l2baseradix > l2small_stride_total)
    {
        const auto p = bfly_pass(cl2radix<l2baseradix>{}, following, l2fftsize - following - l2baseradix);
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, p, p.blocks() >> pass.l2blocks(), twiddles);
        following -= l2baseradix;
    }

    KFR_FOR(pass_index, 0, cfg.small_stride_passes)
    {
        constexpr uint8_t l2stride =
            cfg.l2remainingradix + (cfg.small_stride_passes - 1 - pass_index) * l2baseradix;
        const auto p = bfly_pass(cl2radix<l2baseradix>{}, cl2butterflies<l2stride>{},
                                 l2fftsize - (l2stride + l2baseradix));

        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, p, p.blocks() >> pass.l2blocks(), twiddles);
    };

    {
        const auto p = bfly_pass(cl2radix<cfg.l2remainingradix>{}, cl2butterflies<0>{},
                                 l2fftsize - cfg.l2remainingradix);
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, p, p.blocks() >> pass.l2blocks(), twiddles);
    }
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, bool inverse, uint8_t l2radix,
          uint8_t l2bf, uint8_t l2bl>
KFR_INTRINSIC void mixedradix_iterate_dit_recursive_leaf(
    const bfly_pass<l2radix, l2bf, l2bl>& pass, std::complex<typename traits::type>* inout,
    const std::complex<typename traits::type>*& twiddles) noexcept
{
    constexpr uint8_t l2baseradix          = traits::l2baseradix;
    constexpr uint8_t l2small_stride_total = cfg.l2remainingradix + cfg.small_stride_passes * l2baseradix;

    if (pass.has_one_butterfly())
    {
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, pass, 1, twiddles);
        return;
    }

    const uint8_t top_l2butterflies = pass.l2butterflies();
    const uint8_t top_l2blocks      = pass.l2blocks();

    {
        const auto p =
            bfly_pass(cl2radix<cfg.l2remainingradix>{}, cl2butterflies<0>{},
                      uint8_t(top_l2blocks + top_l2butterflies + l2baseradix - cfg.l2remainingradix));
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, p, p.blocks() >> top_l2blocks, twiddles);
    }

    uint8_t preceding = cfg.l2remainingradix;
    while (preceding < std::min(l2small_stride_total, top_l2butterflies))
    {
        const auto p = bfly_pass(cl2radix<l2baseradix>{}, preceding,
                                 uint8_t(top_l2blocks + top_l2butterflies - preceding));
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, p, p.blocks() >> top_l2blocks, twiddles);
        preceding += l2baseradix;
    }

    preceding = std::max(preceding, l2small_stride_total);
    while (preceding < top_l2butterflies)
    {
        const auto p = bfly_pass(cl2radix<l2baseradix>{}, preceding,
                                 uint8_t(top_l2blocks + top_l2butterflies - preceding));
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, p, p.blocks() >> top_l2blocks, twiddles);
        preceding += l2baseradix;
    }

    mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(inout, pass, 1, twiddles);
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, bool inverse>
KFR_INTRINSIC void mixedradix_iterate_dif_recursive(
    uint8_t l2fftsize, std::complex<typename traits::type>* inout,
    const std::complex<typename traits::type>* twiddles) noexcept
{
    using T                       = typename traits::type;
    constexpr uint8_t l2baseradix = traits::l2baseradix;
    constexpr size_t radix        = size_t(1) << l2baseradix;

    // Root pass — l2bl == 0 is compile-time, so pass_type is first for DIF.
    const auto root = bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2fftsize - l2baseradix), cl2blocks<0>{});
    mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
        inout, root, 1, twiddles); // advances twiddles past this pass

    if (root.l2block_size() <= cfg.l2chunksize)
    {
        mixedradix_iterate_dif_recursive_leaf<traits, cfg, inverse>(root, inout, twiddles);
        return;
    }

    using dyn_pass_t = bfly_pass<l2baseradix, 0xff, 0xff>;

    struct frame
    {
        dyn_pass_t pass;
        uint32_t remaining; // siblings left to process at this level (including current)
        const std::complex<T>* twiddles;
    };

    // One frame per tree level, depth <= 36 / l2baseradix.
    constexpr size_t stack_capacity = 36u / l2baseradix + 1;
    frame stack[stack_capacity];
    int top = -1;

    stack[++top] = { root.template advance<l2baseradix, false, false>(), radix, twiddles };

    while (top >= 0)
    {
        auto& frame = stack[top];
        if (frame.remaining > 1)
        {
            // Reserve this slot for the remaining siblings; process current one below.
            --frame.remaining;
        }
        else
        {
            --top;
        }

        auto tw = frame.twiddles;
        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
            inout, frame.pass, 1,
            tw); // advances tw past this pass

        if (frame.pass.l2block_size() <= cfg.l2chunksize)
        {
            mixedradix_iterate_dif_recursive_leaf<traits, cfg, inverse>(frame.pass, inout, tw);
            inout += frame.pass.block_size();
        }
        else
        {
            stack[++top] = { frame.pass.template advance<l2baseradix, false, false>(), radix, tw };
        }
    }
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, bool inverse>
KFR_INTRINSIC void mixedradix_iterate_dit_recursive(
    uint8_t l2fftsize, std::complex<typename traits::type>* inout,
    const std::complex<typename traits::type>* twiddles) noexcept
{
    using T                       = typename traits::type;
    constexpr uint8_t l2baseradix = traits::l2baseradix;
    constexpr size_t radix        = size_t(1) << l2baseradix;

    const auto root = bfly_pass(cl2radix<l2baseradix>{}, uint8_t(l2fftsize - l2baseradix), cl2blocks<0>{});

    using dyn_pass_t = bfly_pass<l2baseradix, 0xff, 0xff>;

    struct frame
    {
        dyn_pass_t pass;
        std::complex<T>* inout;
        uint32_t next_child;
        const std::complex<T>* child_twiddles_start;
        const std::complex<T>* child_twiddles_end;
    };

    constexpr size_t stack_capacity = 36u / l2baseradix + 1;
    frame stack[stack_capacity];
    int top = 0;

    stack[top] = {
        root.template advance<0, false, false>(), inout, 0, twiddles, nullptr,
    };

    while (top >= 0)
    {
        auto& current         = stack[top];
        const auto child_pass = current.pass.template advance<l2baseradix, false, false>();

        if (current.next_child < radix)
        {
            const size_t child_block_size = child_pass.block_size();
            std::complex<T>* child_inout  = current.inout + current.next_child * child_block_size;
            auto child_tw                 = current.child_twiddles_start;

            if (child_pass.l2block_size() <= cfg.l2chunksize)
            {
                mixedradix_iterate_dit_recursive_leaf<traits, cfg, inverse>(child_pass, child_inout,
                                                                            child_tw);
                if (current.next_child == 0)
                    current.child_twiddles_end = child_tw;
                ++current.next_child;
            }
            else
            {
                stack[++top] = {
                    child_pass, child_inout, 0, child_tw, nullptr,
                };
            }
            continue;
        }

        auto tw = current.child_twiddles_end;
        if (top == 0)
        {
            mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(
                current.inout, bfly_pass(cl2radix<l2baseradix>{}, current.pass.l2stride(), cl2blocks<0>{}), 1,
                tw);
            break;
        }

        mixedradix_pass<traits, inverse, cfg.use_split, cfg.prefetch_offset>(current.inout, current.pass, 1,
                                                                             tw);

        --top;
        auto& parent = stack[top];
        if (parent.next_child == 0)
            parent.child_twiddles_end = tw;
        ++parent.next_child;
    }
}

template <dft_traits traits, dft_config<dft_family::mixedradix> cfg, bool inverse>
void ng_do_dft(const ngfft_plan<typename traits::type>& plan,
               std::complex<typename traits::type>* inout) noexcept
{
    using T                       = typename traits::type;
    constexpr dft_decomp dir      = algo_dir<traits>();
    constexpr uint8_t l2baseradix = traits::l2baseradix;
    constexpr uint8_t l2maxradix  = traits::l2maxradix;
    constexpr uint8_t l2basewidth = traits::l2basewidth;
    static_assert(l2maxradix >= l2baseradix, "max radix must be at least the base radix");
    const std::complex<T>* twiddles = plan.twiddles;

    const std::span<std::complex<T>, std::dynamic_extent> span{ inout, size_t(1) << plan.l2fftsize };

    // DIT (Cooley-Tukey) expects bit-reversed input; apply the permutation before the passes.
    if constexpr (dir == dft_decomp::dit)
    {
        intr::br(span);
    }

    if constexpr (cfg.l2chunksize > 0)
    {
        if constexpr (dir == dft_decomp::dif)
            mixedradix_iterate_dif_recursive<traits, cfg, inverse>(plan.l2fftsize, inout, twiddles);
        else
            mixedradix_iterate_dit_recursive<traits, cfg, inverse>(plan.l2fftsize, inout, twiddles);
    }
    else
    {
        mixedradix_iterate<traits, cfg, inverse>(plan.l2fftsize, inout, twiddles);
    }

    // DIF (Gentleman-Sande) produces bit-reversed output; apply the permutation after the passes.
    if constexpr (dir == dft_decomp::dif)
    {
        intr::br(span);
    }
}

} // namespace KFR_ARCH_NAME

template <dft_traits traits>
constexpr dft_config<dft_family::mixedradix> ng_config(cval_t<dft_family, dft_family::mixedradix>,
                                                       uint8_t l2fftsize) noexcept
{
    const uint8_t regular_passes =
        (l2fftsize - traits::l2maxradix + traits::l2baseradix - 1) / traits::l2baseradix;
    const uint8_t l2remaining = l2fftsize - traits::l2baseradix * regular_passes;
    uint8_t small_stride_passes =
        traits::l2basewidth < l2remaining
            ? 0
            : uint8_t((traits::l2basewidth - l2remaining) / traits::l2baseradix + 1);

    constexpr uint8_t l2elsize = l2elementsize<typename traits::type>;

    // Convert from byte size to element size
    uint8_t l2chunksize                 = kfr::l2chunksize - l2elsize;
    constexpr uint8_t l2prefetchminsize = kfr::l2prefetchminsize - l2elsize;
    constexpr uint8_t l2splitminsize    = kfr::l2splitminsize - l2elsize;

    // align l2chunksize
    l2chunksize =
        (l2chunksize - l2remaining + traits::l2baseradix - 1) / traits::l2baseradix * traits::l2baseradix +
        l2remaining;

    small_stride_passes = std::max(uint8_t(8 / traits::l2baseradix), small_stride_passes);
    // adjust small_stride_passes so cfg.l2remainingradix + cfg.small_stride_passes * l2baseradix stays
    // strictly below l2chunksize
    const uint8_t max_small_stride_passes = uint8_t((l2chunksize - 1 - l2remaining) / traits::l2baseradix);
    small_stride_passes                   = std::min(small_stride_passes, max_small_stride_passes);

    // Small-stride scheduling only applies to middle passes. Leave the dedicated last pass intact,
    // otherwise DIF repeats the first pass and DIT underflows its remaining-following counter.
    small_stride_passes = std::min(small_stride_passes, uint8_t(regular_passes - 1));

    size_t prefetch_offset = 0;
    if (l2fftsize >= l2prefetchminsize)
    {
        if (l2fftsize <= 18 - l2elementsize<typename traits::type>)
            prefetch_offset = 6;
        else
            prefetch_offset = 8;
    }

    return dft_config<dft_family::mixedradix>{
        .l2remainingradix        = l2remaining,
        .l2chunksize             = l2fftsize >= l2chunksize + traits::l2baseradix ? l2chunksize : uint8_t(0),
        .prefetch_offset         = prefetch_offset,
        .use_split               = l2fftsize >= l2splitminsize,
        .small_stride_passes     = small_stride_passes,
        .all_small_stride_passes = regular_passes >= small_stride_passes,
    };
}

} // namespace kfr
