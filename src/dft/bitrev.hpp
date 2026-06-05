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

#include <span>

#include <kfr/simd/complex.hpp>
#include <kfr/simd/constants.hpp>
#include <kfr/simd/read_write.hpp>
#include <kfr/simd/digitreverse.hpp>
#include <kfr/simd/vec.hpp>
#include <kfr/runtime/time.hpp>

#include "data/bitrev.hpp"

#include "ft.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

namespace intr
{

constexpr inline static bool fft_reorder_aligned = false;

constexpr inline static size_t bitrev_table_log2N = ilog2(std::size(data::bitrev_table));

template <size_t Bits>
inline u32 bitrev_using_table(u32 x)
{
#ifdef KFR_ARCH_NEON
    return __builtin_bitreverse32(x) >> (32 - Bits);
#else
    if constexpr (Bits > bitrev_table_log2N)
        return bitreverse<Bits>(x);

    return data::bitrev_table[x] >> (bitrev_table_log2N - Bits);
#endif
}

template <bool use_table>
inline u32 bitrev_using_table(u32 x, size_t bits, cbool_t<use_table>)
{
#ifdef KFR_ARCH_NEON
    return __builtin_bitreverse32(x) >> (32 - bits);
#else
    if constexpr (use_table)
    {
        return data::bitrev_table[x] >> (bitrev_table_log2N - bits);
    }
    else
    {
#ifdef __clang__
        return __builtin_bitreverse32(x) >> (32 - bits);
#else
        return bitreverse<32>(x) >> (32 - bits);
#endif
    }
#endif
}

inline u32 dig4rev_using_table(u32 x, size_t bits)
{
#ifdef KFR_ARCH_NEON
    x = __builtin_bitreverse32(x);
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = x >> (32 - bits);
    return x;
#else
    if (bits > bitrev_table_log2N)
    {
        if (bits <= 16)
            return digitreverse4<16>(x) >> (16 - bits);
        else
            return digitreverse4<32>(x) >> (32 - bits);
    }

    x = data::bitrev_table[x];
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = x >> (bitrev_table_log2N - bits);
    return x;
#endif
}

template <typename T, size_t N>
KFR_INTRINSIC void br_simd(std::complex<T>* data, csize_t<N>)
{
    vec<T, N * 2> v = kfr::read<N * 2>(reinterpret_cast<T*>(data));
    v               = bitreverse<2>(v);
    write(reinterpret_cast<T*>(data), v);
}

template <typename T>
constexpr inline size_t br_type_penalty = 1; // std::is_same_v<T, float> ? 2 : 1;

template <typename T, size_t N>
KFR_INTRINSIC void br_prefetch(std::complex<T>* data0, std::complex<T>* data1, csize_t<N>, size_t stride)
{
    cprefetch<N, N, T>(data0, stride);
    cprefetch<N, N, T>(data1, stride);
}

template <bool swap, typename T, size_t N>
KFR_INTRINSIC void br_simd_two(std::complex<T>* data0, std::complex<T>* data1, csize_t<N>, size_t stride)
    requires((N * N * 2 * br_type_penalty<T>) < vector_capacity<T>)
{
    cvec<T, N * N> v0 = read_group<N, N, 2>(reinterpret_cast<const T*>(data0), stride);
    cvec<T, N * N> v1 = read_group<N, N, 2>(reinterpret_cast<const T*>(data1), stride);
    v0                = bitreverse<2>(v0);
    v1                = bitreverse<2>(v1);
    if constexpr (swap)
    {
        std::swap(v0, v1);
    }
    write_group<N, N, 2>(reinterpret_cast<T*>(data0), stride, v0);
    write_group<N, N, 2>(reinterpret_cast<T*>(data1), stride, v1);
}

template <bool swap, typename T, size_t N>
KFR_INTRINSIC void br_simd_two(std::complex<T>* data0, std::complex<T>* data1, csize_t<N>, size_t stride)
    requires((N * N * 2 * br_type_penalty<T>) >= vector_capacity<T>)
{
    constexpr size_t N2  = N / 2;
    const size_t stride2 = 2 * stride;

    if constexpr (!swap)
    {
        br_simd_two<false>(data0, data0 + stride + N2, csize<N2>, stride2); //  0,  3
        br_simd_two<true>(data0 + N2, data0 + stride, csize<N2>,
                          stride2); //  1 <-> 2

        br_simd_two<false>(data1, data1 + stride + N2, csize<N2>, stride2); //  0,  3
        br_simd_two<true>(data1 + N2, data1 + stride, csize<N2>,
                          stride2); //  1 <-> 2
    }
    else
    {
        br_simd_two<true>(data0, data1, csize<N2>, stride2); // 0

        br_simd_two<true>(data0 + stride, data1 + N2, csize<N2>, stride2); // 1 <-> 2
        br_simd_two<true>(data0 + N2, data1 + stride, csize<N2>, stride2); // 1 <-> 2

        br_simd_two<true>(data0 + stride + N2, data1 + stride + N2, csize<N2>, stride2); // 3
    }
}

template <typename T, size_t N>
KFR_INTRINSIC void br_simd_one(std::complex<T>* data, csize_t<N>, size_t stride)
{
    constexpr size_t N2 = N / 2;
    br_simd_two<false>(data, data + stride + N2, csize<N2>, 2 * stride); //  0,  3
    br_simd_two<true>(data + N2, data + stride, csize<N2>, 2 * stride); //  1 <-> 2
}

template <typename T, size_t Extent>
KFR_INTRINSIC void br_small(uint32_t log2n, std::span<std::complex<T>, Extent> data)
{
    switch (log2n)
    {
    case 6:
        return br_simd_one(data.data(), csize_t<8>{}, 8);
    case 5:
        [[unlikely]] return br_simd(data.data(), csize_t<32>{});
    case 4:
        [[unlikely]] return br_simd(data.data(), csize_t<16>{});
    case 3:
        [[unlikely]] return br_simd(data.data(), csize_t<8>{});
    case 2:
        [[unlikely]] return br_simd(data.data(), csize_t<4>{});
    case 1:
    case 0:
        break;
    default:
        KFR_UNREACHABLE;
    }
}

#if defined(_MSC_VER) && !defined(__clang__)
KFR_INTRINSIC uint32_t lzcnt_u32(uint32_t x) noexcept
{
    unsigned long index;
    _BitScanReverse(&index, x);
    return 31 - (unsigned int)index;
}
KFR_INTRINSIC uint32_t tzcnt_u32(uint32_t x) noexcept
{
    unsigned long index;
    _BitScanForward(&index, x);
    return (uint32_t)index;
}
#else
#define lzcnt_u32(x) __builtin_clz(x)
#define tzcnt_u32(x) __builtin_ctz(x)
#endif

template <typename T, size_t Extent = std::dynamic_extent, bool large>
void br_impl(uint32_t log2n, std::span<std::complex<T>, Extent> data, cbool_t<large>)
{
    constexpr uint32_t group_log2n = std::is_same_v<T, double> ? 2 : 3;
    // log2n(N) in NxN group
    constexpr size_t group_n           = size_t(1) << group_log2n; // N in NxN group
    constexpr uint32_t group_log2narea = 2 * group_log2n; // log2n(N^2) in NxN group
    uint32_t log2numgroups             = log2n - group_log2narea; // log2(num_groups)
    size_t numgroups                   = size_t(1) << log2numgroups; // num_groups
    const size_t numgroups_minus1      = numgroups - 1;
    size_t stride                      = size_t(1) << (log2n - group_log2n); // size / group_n

    size_t half_numgroups = numgroups / 2;

    auto prefetch = [&](size_t i, size_t j) KFR_INLINE_LAMBDA
    {
        if (i == j) [[unlikely]]
        {
            size_t mi = numgroups_minus1 - i;
            br_prefetch(data.data() + i * group_n, data.data() + mi * group_n, csize_t<group_n>{}, stride);
        }
        else
        {
            size_t a = (i < j) ? i : (numgroups_minus1 - j);
            size_t b = (i < j) ? j : (numgroups_minus1 - i);
            br_prefetch(data.data() + a * group_n, data.data() + b * group_n, csize_t<group_n>{}, stride);
        }
    };

    auto process = [&](size_t i, size_t j) KFR_INLINE_LAMBDA
    {
        if (i == j) [[unlikely]]
        {
            size_t mi = numgroups_minus1 - i;
            br_simd_two<false>(data.data() + i * group_n, data.data() + mi * group_n, csize_t<group_n>{},
                               stride);
        }
        else
        {
            size_t a = (i < j) ? i : (numgroups_minus1 - j);
            size_t b = (i < j) ? j : (numgroups_minus1 - i);
            br_simd_two<true>(data.data() + a * group_n, data.data() + b * group_n, csize_t<group_n>{},
                              stride);
        }
    };

    if constexpr (!large)
    {
        if (half_numgroups == 1)
        {
            process(0, 0);
            return;
        }
        else if (half_numgroups == 2)
        {
            process(0, 0);
            process(1, half_numgroups);
            return;
        }
        else if (half_numgroups == 4)
        {
            process(0, 0);
            process(1, half_numgroups);
            process(2, half_numgroups / 2);
            process(3, half_numgroups * 3 / 2);
            return;
        }
        else if (half_numgroups == 8)
        {
            process(0, 0);
            process(1, half_numgroups);
            process(2, half_numgroups / 2);
            process(3, half_numgroups * 3 / 2);
            process(4, half_numgroups / 4);
            process(5, half_numgroups / 4 + half_numgroups);
            process(6, half_numgroups / 4 + half_numgroups / 2);
            process(7, half_numgroups / 4 + half_numgroups * 3 / 2);
            return;
        }
    }

    process(0, 0);

    uint32_t j          = 1u << (log2numgroups - 1); // bit-reversed index of i=1
    const uint32_t mask = (1u << log2numgroups) - 1u;

    for (uint32_t i = 1; i < half_numgroups; i++)
    {
        uint32_t bit    = 0x80000000u >> lzcnt_u32(mask ^ j);
        uint32_t next_j = (j & (bit - 1u)) | bit;

        if constexpr (large)
        {
            prefetch(i + 1, next_j);
        }

        process(i, j);

        j = next_j;
    }
}

template <typename T, size_t Extent = std::dynamic_extent>
KFR_INTRINSIC void br(std::span<std::complex<T>, Extent> data)
{
    if constexpr (Extent != std::dynamic_extent)
    {
        // Known at compile time
        constexpr uint32_t log2n = std::countr_zero(Extent);

        if constexpr (log2n <= 6)
        {
            br_small(log2n, data);
        }
        else
        {
            constexpr bool large = log2n > 14;
            if constexpr (large)
            {
                br_impl<T, Extent>(log2n, data, cbool_t<true>());
            }
            else
            {
                br_impl<T, Extent>(log2n, data, cbool_t<false>());
            }
        }
    }
    else
    {
        // Known only at runtime

        uint32_t log2n = tzcnt_u32(uint32_t(data.size()));

        if (log2n <= 6) [[unlikely]]
        {
            br_small(log2n, data);
        }
        else
        {
            bool large = log2n > 14;
            if (large) [[unlikely]]
            {
                br_impl<T, Extent>(log2n, data, cbool_t<true>());
            }
            else
            {
                br_impl<T, Extent>(log2n, data, cbool_t<false>());
            }
        }
    }
}

} // namespace intr
} // namespace KFR_ARCH_NAME
} // namespace kfr
