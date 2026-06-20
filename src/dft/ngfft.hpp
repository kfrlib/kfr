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

#include "dft-fft.hpp"

namespace kfr
{

using internal_generic::to_family;

enum class dft_pass_type : uint8_t
{
    first  = 1,
    last   = 2,
    only   = 3, // first | last
    middle = 0,
};

template <uint8_t l2radix>
using cl2radix = cval_t<uint8_t, l2radix>;

template <uint8_t value>
using cl2butterflies = cval_t<uint8_t, value>;

template <uint8_t value>
using cl2blocks = cval_t<uint8_t, value>;

/** Runtime state for a butterfly pass within an FFT of known size.
 * @tparam l2radix               log2 of the radix (number of arms per butterfly).
 * @tparam static_l2butterflies  compile-time log2 butterfly count; 0xff = runtime.
 * @tparam static_l2blocks       compile-time log2 block count; 0xff = runtime.
 */
template <uint8_t l2r, uint8_t l2bf = 0xff, uint8_t l2bl = 0xff>
struct bfly_pass
{
    template <uint8_t, uint8_t, uint8_t>
    friend struct bfly_pass;

private:
    constexpr bfly_pass() noexcept
        requires(l2bf != 0xff && l2bl != 0xff)
    {
    }

    constexpr bfly_pass(uint8_t l2butterflies, uint8_t l2blocks) noexcept
        requires(l2bf == 0xff && l2bl == 0xff)
    {
        l2butterflies_ = l2butterflies;
        l2blocks_      = l2blocks;
    }

    constexpr bfly_pass(uint8_t l2blocks) noexcept
        requires(l2bf != 0xff && l2bl == 0xff)
    {
        l2blocks_ = l2blocks;
    }

    constexpr bfly_pass(uint8_t l2butterflies) noexcept
        requires(l2bf == 0xff && l2bl != 0xff)
    {
        l2butterflies_ = l2butterflies;
    }

public:
    constexpr bfly_pass() = default;
    constexpr bfly_pass(cl2radix<l2r>, cl2butterflies<l2bf>, cl2blocks<l2bl>) noexcept
        requires(l2bf != 0xff && l2bl != 0xff)
    {
    }

    constexpr bfly_pass(cl2radix<l2r>, uint8_t l2butterflies, uint8_t l2blocks) noexcept
        requires(l2bf == 0xff && l2bl == 0xff)
    {
        l2butterflies_ = l2butterflies;
        l2blocks_      = l2blocks;
    }

    constexpr bfly_pass(cl2radix<l2r>, cl2butterflies<l2bf>, uint8_t l2blocks) noexcept
        requires(l2bf != 0xff && l2bl == 0xff)
    {
        l2blocks_ = l2blocks;
    }

    constexpr bfly_pass(cl2radix<l2r>, uint8_t l2butterflies, cl2blocks<l2bl>) noexcept
        requires(l2bf == 0xff && l2bl != 0xff)
    {
        l2butterflies_ = l2butterflies;
    }

    KFR_NO_UNIQUE_ADDRESS std::conditional_t<l2bf == 0xff, uint8_t, empty_struct<0>>
        l2butterflies_; ///< log2 butterfly count; present only when @p static_l2butterflies == 0xff.
    KFR_NO_UNIQUE_ADDRESS std::conditional_t<l2bl == 0xff, uint8_t, empty_struct<1>>
        l2blocks_; ///< log2 block count; present only when @p static_l2blocks == 0xff.

    constexpr static bool has_one_butterfly() noexcept { return l2bf == 0; }
    constexpr static bool has_one_block() noexcept { return l2bl == 0; }

    constexpr static bool known_butterflies() noexcept { return l2bf != 0xff; }
    constexpr static bool known_blocks() noexcept { return l2bl != 0xff; }

    template <int offset, bool static_l2bf = true, bool static_l2bl = true>
    constexpr auto advance() const noexcept
    {
        if constexpr (known_butterflies() && known_blocks() && static_l2bf && static_l2bl)
        {
            static_assert(l2bf >= offset);
            static_assert(l2bl >= -offset);
            return bfly_pass<l2r, l2bf - offset, l2bl + offset>{};
        }
        else if constexpr (known_butterflies() && static_l2bl)
        {
            static_assert(l2bf >= offset);
            return bfly_pass<l2r, l2bf - offset, 0xff>{ uint8_t(l2blocks() + offset) };
        }
        else if constexpr (known_blocks() && static_l2bl)
        {
            static_assert(l2bl >= -offset);
            return bfly_pass<l2r, 0xff, l2bl + offset>{ uint8_t(l2butterflies() - offset) };
        }
        else
        {
            return bfly_pass<l2r, 0xff, 0xff>{ uint8_t(l2butterflies() - offset),
                                               uint8_t(l2blocks() + offset) };
        }
    }

    static constexpr dft_pass_type pass_type(dft_decomp dir) noexcept
    {
        if constexpr (l2bf == 0 && l2bl == 0)
            return dft_pass_type::only;
        else if constexpr (l2bf == 0)
            return dir == dft_decomp::dit ? dft_pass_type::first : dft_pass_type::last;
        else if constexpr (l2bl == 0)
            return dir == dft_decomp::dif ? dft_pass_type::first : dft_pass_type::last;
        else
            return dft_pass_type::middle;
    }

    /// Returns log2(butterflies()): compile-time value if known, otherwise the runtime field.
    constexpr uint8_t l2butterflies() const noexcept
    {
        if constexpr (l2bf != 0xff)
            return l2bf;
        else
            return l2butterflies_;
    }
    /// Returns log2(blocks()): compile-time value if known, otherwise the runtime field.
    constexpr uint8_t l2blocks() const noexcept
    {
        if constexpr (l2bl != 0xff)
            return l2bl;
        else
            return l2blocks_;
    }

    /// log2 of the total FFT size: l2butterflies + l2radix + l2blocks.
    constexpr uint8_t l2fftsize() const noexcept { return l2butterflies() + l2r + l2blocks(); }
    /// Total FFT size N.
    constexpr size_t fftsize() const noexcept { return size_t(1) << l2fftsize(); }

    /// log2 of the radix (number of arms per butterfly).
    static constexpr size_t l2radix() noexcept { return l2r; }

    /// Radix R = number of arms per butterfly.
    static constexpr size_t radix() noexcept { return size_t(1) << l2r; }

    /// Number of butterfly operations per block; equals stride().
    /// Total per pass = blocks() * butterflies() = N / radix().
    constexpr size_t butterflies() const noexcept { return size_t(1) << l2butterflies(); }

    /// Number of independent butterfly groups in this pass.
    /// DIF: 1 at pass 0, grows by R; DIT: N/R at pass 0, shrinks by R.
    constexpr size_t blocks() const noexcept { return size_t(1) << l2blocks(); }

    /// Spacing between butterfly arms in the data array.
    /// DIF: N/R (largest) down to 1; DIT: 1 up to N/R.
    constexpr size_t stride() const noexcept { return butterflies(); }

    /// log2 of stride(); equals l2butterflies().
    constexpr uint8_t l2stride() const noexcept { return l2butterflies(); }

    /// N / radix() — constant across all passes.
    constexpr size_t total_butterflies() const noexcept { return 1u << l2total_butterflies(); }

    /// log2 of total_butterflies(); equals l2butterflies() + l2blocks().
    constexpr uint8_t l2total_butterflies() const noexcept { return l2butterflies() + l2blocks(); }

    /// @return @c false when every twiddle in this pass is trivially W^0 = 1
    ///         (DIF last pass or DIT first pass, where stride = 1).
    KFR_INTRINSIC constexpr bool need_twiddles() const noexcept { return l2butterflies() > 0; }

    /// Twiddle exponent k for W_M^k = exp(j·2π·k/M), where M = block_size().
    /// Twiddle values depend only on butterflies() (stride) and radix().
    /// @param e  element index within block [0, butterflies())
    /// @param n  arm index [0, radix()); n=0 is always W^0 = 1
    KFR_INTRINSIC constexpr size_t twiddle_k(size_t e, size_t n) const noexcept { return n * e; }

    /// Per-element twiddle index step: 1 per element, n per arm (in block_size() basis).
    constexpr size_t twiddle_step() const noexcept { return 1; }

    /// Linear data index of arm @p n in butterfly (b, e).
    /// Arms are evenly spaced by stride(): b·R·stride + e + n·stride.
    /// @param b  block index
    /// @param e  element index within block
    /// @param n  arm index [0, radix())
    KFR_INTRINSIC constexpr size_t data_index(size_t b, size_t e, size_t n) const noexcept
    {
        return (b << l2block_size()) + e + (n << l2stride());
    }
    /// Linear data index of arm 0 in butterfly (b, e): data_index(b, e, 0).
    KFR_INTRINSIC constexpr size_t data_index(size_t b, size_t e) const noexcept
    {
        return (b << l2block_size()) + e;
    }
    /// Linear data index of the first element in block b.
    KFR_INTRINSIC constexpr size_t data_index(size_t b) const noexcept { return b * block_size(); }

    /// Elements per block: radix() * stride().
    KFR_INTRINSIC constexpr size_t block_size() const noexcept { return size_t(1) << l2block_size(); }
    /// log2 of block_size(): l2radix + l2stride().
    KFR_INTRINSIC constexpr size_t l2block_size() const noexcept { return l2r + l2stride(); }
};

// Deduction guides for bfly_pass
// Both butterflies and blocks are compile-time.
template <uint8_t R, uint8_t B, uint8_t Bl>
bfly_pass(cl2radix<R>, cl2butterflies<B>, cl2blocks<Bl>) -> bfly_pass<R, B, Bl>;

// Butterflies compile-time, blocks runtime.
template <uint8_t R, uint8_t B>
bfly_pass(cl2radix<R>, cl2butterflies<B>, uint8_t) -> bfly_pass<R, B, 0xff>;

// Butterflies runtime, blocks compile-time.
template <uint8_t R, uint8_t Bl>
bfly_pass(cl2radix<R>, uint8_t, cl2blocks<Bl>) -> bfly_pass<R, 0xff, Bl>;

// Both runtime.
template <uint8_t R>
bfly_pass(cl2radix<R>, uint8_t, uint8_t) -> bfly_pass<R, 0xff, 0xff>;

static_assert(sizeof(bfly_pass<2, 0, 0>) == 1,
              "bfly_pass with compile-time butterfly and block counts should be 1 byte");

static_assert(std::is_empty_v<bfly_pass<2, 0, 0>>,
              "bfly_pass with compile-time butterfly and block counts should be empty");

constexpr dft_pass_type operator&(dft_pass_type a, dft_pass_type b) noexcept
{
    return static_cast<dft_pass_type>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
constexpr dft_pass_type operator|(dft_pass_type a, dft_pass_type b) noexcept
{
    return static_cast<dft_pass_type>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

enum class bitrev_permute : uint8_t
{
    none,
    packed,
    parallel,
};

template <size_t R, size_t N, bitrev_permute bitrev>
constexpr size_t br_permute(size_t i) noexcept
{
    constexpr size_t l2r = ilog2(R);
    if constexpr (bitrev == bitrev_permute::parallel)
    {
        size_t j = i / (2 * N);
        j        = bitreverse<l2r>(j);
        return j * (2 * N) + (i % (2 * N));
    }
    else if constexpr (bitrev == bitrev_permute::packed)
    {
        // Assume interleaved format
        size_t j = i / 2;
        j        = bitreverse<ilog2(R)>(j % R) + j / R * R;
        return 2 * j + (i % 2);
    }
    else
    {
        return i;
    }
}

struct reading
{
};
struct writing
{
};

struct reading_single
{
};
struct reading_group
{
};
struct reading_group_var
{
};

inline namespace KFR_ARCH_NAME
{

template <size_t R, typename T, size_t N, size_t in_split_width, bool split_format,
          bitrev_permute bitrev = bitrev_permute::none>
KFR_INTRINSIC cvec<T, R * N> cread_group2(const std::complex<T>* ptr, size_t stride)
{
    static_assert(std::has_single_bit(in_split_width));
    static_assert(in_split_width <= N);
    KFR_EXPOSE_VALUE(&expose_types<reading_group_var, csize_t<in_split_width>>);
    cvec<T, R * N> w = read_group<R, N, 2, false, T>(ptr_cast<T>(ptr), stride);
    using U          = T; // void

    using Indices = map_indices_t<2 * R * N, interleave_permute<in_split_width, U>,
                                  split_permute<split_format ? N : 1, U>, br_permute<R, N, bitrev>>;

    KFR_EXPOSE_VALUE(&expose_types<reading, Indices>);

    w = w.shuffle(Indices{});
    return w;
}

template <size_t R, typename T, size_t N, size_t in_split_width, bool split_format,
          bitrev_permute bitrev = bitrev_permute::none, size_t fixed_stride>
KFR_INTRINSIC cvec<T, R * N> cread_group2(const std::complex<T>* ptr, csize_t<fixed_stride>)
{
    static_assert(fixed_stride <= N);
    using U = T; // void
    cvec<T, R * N> w;
    if constexpr (N < intr::complex_vector_width<T>)
    {
        KFR_EXPOSE_VALUE(&expose_types<reading_single>);
        w = read<R * N * 2, false, T>(ptr_cast<T>(ptr));
    }
    else
    {
        KFR_EXPOSE_VALUE(&expose_types<reading_group>);
        w = read_group<R, N, 2, false, T>(ptr_cast<T>(ptr), N);
    }
    using Indices = map_indices_t<2 * R * N, interleave_permute<in_split_width, U>,
                                  ctranspose_permute<R, fixed_stride, N / fixed_stride>,
                                  split_permute<split_format ? N : 1, U>, br_permute<R, N, bitrev>>;

    KFR_EXPOSE_VALUE(&expose_types<reading, Indices>);
    w = w.shuffle(Indices{});
    return w;
}

template <size_t R, typename T, size_t N, size_t out_split_width, bool split_format,
          bitrev_permute bitrev = bitrev_permute::none>
KFR_INTRINSIC void cwrite_group2(std::complex<T>* ptr, cvec<T, R * N>& w, size_t stride)
{
    static_assert(std::has_single_bit(out_split_width));
    static_assert(out_split_width <= N);
    using U = T; // void
    using Indices =
        map_indices_t<2 * R * N, br_permute<R, N, bitrev>, interleave_permute<split_format ? N : 1, U>,
                      split_permute<out_split_width, U>>;

    KFR_EXPOSE_VALUE(&expose_types<writing, Indices>);
    w = w.shuffle(Indices{});
    write_group<R, N, 2, false, T>(ptr_cast<T>(ptr), stride, w);
}

template <size_t R, typename T, size_t N, size_t out_split_width, bool split_format,
          bitrev_permute bitrev = bitrev_permute::none, size_t fixed_stride>
KFR_INTRINSIC void cwrite_group2(std::complex<T>* ptr, cvec<T, R * N>& w, csize_t<fixed_stride>)
{
    using U = T; // void
    static_assert(fixed_stride <= N);
    using Indices =
        map_indices_t<2 * R * N, br_permute<R, N, bitrev>, interleave_permute<split_format ? N : 1, U>,
                      ctranspose_permute<N / fixed_stride, fixed_stride, R>,
                      split_permute<out_split_width, U>>;

    KFR_EXPOSE_VALUE(&expose_types<writing, Indices>);
    w = w.shuffle(Indices{});
    if constexpr (N < intr::complex_vector_width<T>)
        write<false>(ptr_cast<T>(ptr), w);
    else
        write_group<R, N, 2, false, T>(ptr_cast<T>(ptr), N, w);
}
} // namespace KFR_ARCH_NAME

template <typename T>
concept dft_traits = requires {
    { T::algo } -> std::convertible_to<dft_algorithm>;
    { T::l2baseradix } -> std::convertible_to<uint8_t>;
    { T::l2maxradix } -> std::convertible_to<uint8_t>;
    { T::l2basewidth } -> std::convertible_to<uint8_t>;
    { T::l2maxsingleradix } -> std::convertible_to<uint8_t>;

    typename T::type;
    requires std::floating_point<typename T::type>;

    requires(T::l2basewidth <= 5); // Assuming a maximum width of 32 for SIMD processing

    requires(T::l2maxsingleradix >= T::l2maxradix);
    requires(T::l2maxradix >= T::l2baseradix);
};

template <dft_family family>
struct dft_config;

template <dft_traits traits>
using dft_config_t = dft_config<to_family(traits::algo)>;

template <uint8_t l2basewidth, uint8_t l2baseradix, uint8_t l2passradix>
constexpr uint8_t l2pass_width() noexcept
{
    if constexpr (l2basewidth + l2baseradix >= l2passradix)
        return l2basewidth + l2baseradix - l2passradix;
    else
        return 0;
}

template <dft_traits traits, uint8_t l2passradix>
constexpr uint8_t l2pass_width() noexcept
{
    return l2pass_width<traits::l2basewidth, traits::l2baseradix, l2passradix>();
}

constexpr inline uint8_t numfixedfftsizes  = 8;
constexpr inline uint8_t l2chunksize       = 12; // 8KiB
constexpr inline uint8_t l2prefetchminsize = 15; // 32KiB
constexpr inline uint8_t l2splitminsize    = 9; // 512bytes

template <typename T>
constexpr inline uint8_t l2elementsize = ilog2(sizeof(std::complex<T>));

#ifdef KFR_ARCH_X64
template <typename T>
constexpr inline uint8_t maxfftsize = 36 - l2elementsize<T>;
#else
template <typename T>
constexpr inline uint8_t maxfftsize = 29 - l2elementsize<T>;
#endif

} // namespace kfr
