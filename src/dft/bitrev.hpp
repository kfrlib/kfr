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
KFR_INTRINSIC void br_simd(complex<T>* data)
{
    vec<T, N * 2> v = kfr::read<N * 2>(reinterpret_cast<T*>(data));
    v               = bitreverse<2>(v);
    write(reinterpret_cast<T*>(data), v);
}

template <typename T, size_t N>
KFR_INTRINSIC void br_prefetch(complex<T>* data0, complex<T>* data1, size_t stride)
{
    cprefetch<N, N, T>(data0, stride);
    cprefetch<N, N, T>(data1, stride);
}

template <typename T, size_t N>
KFR_INTRINSIC void br_simd_two(bool swap, complex<T>* data0, complex<T>* data1, size_t stride)
{
    if constexpr ((N * N * 2) <= complex_vector_capacity<T>)
    {
        cvec<T, N * N> v0 = read_group<N, N, 2>(reinterpret_cast<const T*>(data0), stride);
        v0                = bitreverse<2>(v0);
        cvec<T, N * N> v1 = read_group<N, N, 2>(reinterpret_cast<const T*>(data1), stride);
        v1                = bitreverse<2>(v1);
        if (swap)
        {
            std::swap(data0, data1);
        }
        write_group<N, N, 2>(reinterpret_cast<T*>(data0), stride, v0);
        write_group<N, N, 2>(reinterpret_cast<T*>(data1), stride, v1);
    }
    else
    {
        constexpr size_t N2  = N / 2;
        const size_t stride2 = 2 * stride;

        br_simd_two<T, N2>(swap, data0, swap ? data1 : data0 + stride + N2, stride2);
        br_simd_two<T, N2>(true, data0 + N2, (swap ? data1 : data0) + stride, stride2);
        br_simd_two<T, N2>(true, data1 + N2, (swap ? data0 : data1) + stride, stride2);
        br_simd_two<T, N2>(swap, data1 + stride + N2, swap ? data0 + stride + N2 : data1, stride2);
    }
}

template <typename T, size_t N>
KFR_INTRINSIC void br_simd_one(complex<T>* data, size_t stride)
{
    constexpr size_t N2 = N / 2;
    br_simd_two<T, N2>(false, data, data + stride + N2, 2 * stride); //  0,  3
    br_simd_two<T, N2>(true, data + N2, data + stride, 2 * stride); //  1 <-> 2
}

constexpr inline size_t br_group_log2n_adjust = 0;

template <typename T>
constexpr inline size_t br_group_log2n = 3; //

template <typename T, size_t Extent>
KFR_INTRINSIC void br_small(uint32_t log2n, std::span<complex<T>, Extent> data)
{
    switch (log2n)
    {
    case 6:
        return br_simd_one<T, 8>(data.data(), 8);
    case 5:
        [[unlikely]] return br_simd<T, 32>(data.data());
    case 4:
        [[unlikely]] return br_simd<T, 16>(data.data());
    case 3:
        [[unlikely]] return br_simd<T, 8>(data.data());
    case 2:
        [[unlikely]] return br_simd<T, 4>(data.data());
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

template <typename T, size_t group_n>
KFR_INTRINSIC void br_process_idx(size_t i, size_t j, size_t numgroups_minus1, complex<T>* data,
                                  size_t stride)
{
    const bool distinct = i != j;
    const size_t a      = i > j ? numgroups_minus1 ^ j : i;
    const size_t b      = i < j ? j : numgroups_minus1 ^ i;

    br_simd_two<T, group_n>(distinct, data + a * group_n, data + b * group_n, stride);
}

template <typename T>
void br_impl(uint32_t log2n, std::span<complex<T>> data);

template <typename T, size_t Extent = std::dynamic_extent>
KFR_INTRINSIC void br(std::span<complex<T>, Extent> data)
{
    if constexpr (Extent != std::dynamic_extent && Extent <= 1024)
    {
        // Known at compile time
        constexpr uint32_t log2n = std::countr_zero(Extent);

        if constexpr (log2n <= 6)
        {
            br_small(log2n, data);
        }
        else
        {
            constexpr uint32_t group_log2n     = std::min(br_group_log2n<T>, size_t(log2n / 2));
            constexpr size_t group_n           = size_t(1) << group_log2n;
            constexpr uint32_t group_log2narea = 2 * group_log2n;
            constexpr uint32_t log2numgroups   = log2n - group_log2narea;
            constexpr size_t numgroups         = size_t(1) << log2numgroups;
            constexpr size_t half_numgroups    = numgroups / 2;
            const size_t numgroups_minus1      = numgroups - 1;
            const size_t stride                = size_t(1) << (log2n - group_log2n);

            KFR_FOR(i, 0, half_numgroups)
            {
                constexpr size_t j = bitreverse<log2numgroups>(i);
                br_process_idx<T, group_n>(i, j, numgroups_minus1, data.data(), stride);
            };
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
            br_impl<T>(log2n, data);
        }
    }
}

} // namespace intr
} // namespace KFR_ARCH_NAME
} // namespace kfr
