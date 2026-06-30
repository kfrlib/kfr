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
#include "bitrev.hpp"

namespace kfr
{

inline namespace KFR_ARCH_NAME
{

namespace intr
{

template <typename T, size_t group_n>
KFR_INTRINSIC void br_prefetch_idx(size_t i, size_t j, size_t numgroups_minus1, complex<T>* data,
                                   size_t stride)
{
    const size_t a = i > j ? numgroups_minus1 ^ j : i;
    const size_t b = i < j ? j : numgroups_minus1 ^ i;

    br_prefetch<T, group_n>(data + a * group_n, data + b * group_n, stride);
}

template <typename T>
void br_impl(uint32_t log2n, std::span<complex<T>> data)
{
    constexpr uint32_t group_log2n     = br_group_log2n<T>;
    constexpr size_t group_n           = size_t(1) << group_log2n;
    constexpr uint32_t group_log2narea = 2 * group_log2n;
    if (log2n <= group_log2narea) [[unlikely]]
        return;

    const uint32_t m              = log2n - group_log2narea; // log2numgroups
    const size_t numgroups        = size_t(1) << m;
    const size_t numgroups_minus1 = numgroups - 1;
    const size_t stride           = size_t(1) << (log2n - group_log2n);
    complex<T>* const ptr         = data.data();

    // block side in tiles: 32 for f32, 16 for f64  (≈1 MiB working set, fits L2 + STLB)
    constexpr uint32_t beta = (sizeof(T) == 4) ? 5u : 4u;

    // not enough middle bits to block
    if (m < 2 * beta + 1) [[unlikely]]
    {
        const size_t log2numgroups  = m;
        const size_t half_numgroups = numgroups / 2;
        if (half_numgroups < 4) [[unlikely]]
            return;
        const size_t numgroups_eighth = half_numgroups / 4;
        br_process_idx<T, group_n>(0, 0, numgroups_minus1, data.data(), stride);
        br_process_idx<T, group_n>(numgroups_eighth * 1, 4, numgroups_minus1, data.data(), stride);
        br_process_idx<T, group_n>(numgroups_eighth * 2, 2, numgroups_minus1, data.data(), stride);
        br_process_idx<T, group_n>(numgroups_eighth * 3, 6, numgroups_minus1, data.data(), stride);

        uint32_t j          = 1u << (log2numgroups - 1); // bit-reversed index of i=1
        const uint32_t mask = (1u << log2numgroups) - 1u;

        for (uint32_t i = 1; i < numgroups_eighth; i++)
        {
            uint32_t bit    = 0x80000000u >> lzcnt_u32(mask ^ j);
            uint32_t next_j = (j & (bit - 1u)) | bit;

            br_process_idx<T, group_n>(i, j, numgroups_minus1, data.data(), stride);
            br_process_idx<T, group_n>(i + numgroups_eighth * 1, j + 4, numgroups_minus1, data.data(),
                                       stride);
            br_process_idx<T, group_n>(i + numgroups_eighth * 2, j + 2, numgroups_minus1, data.data(),
                                       stride);
            br_process_idx<T, group_n>(i + numgroups_eighth * 3, j + 6, numgroups_minus1, data.data(),
                                       stride);

            j = next_j;
        }
        return;
    }
    constexpr uint32_t B = 1u << beta;

    const uint32_t high_shift = m - 1 - beta; // hi -> top bits of i
    const size_t lo_mid_count = size_t(1) << high_shift; // == 2^(m-1-beta)

    uint32_t j_base     = 0;
    const uint32_t mask = (1u << m) - 1u;

    for (size_t lo_mid = 0; lo_mid < lo_mid_count; ++lo_mid)
    {
        KFR_FOR(hi, 0, B) // unrolled; partner page-set fixed
        {
            constexpr size_t hi_off = size_t(bitreverse<beta>(hi)) << 1;
            const size_t i          = lo_mid | (size_t(hi) << high_shift);
            br_process_idx<T, group_n>(i, size_t(j_base) | hi_off, numgroups_minus1, ptr, stride);
        };

        // advance reversed counter
        const uint32_t bit = 0x80000000u >> lzcnt_u32(j_base ^ mask);
        j_base             = (j_base & (bit - 1u)) | bit;
    }
}

} // namespace intr
} // namespace KFR_ARCH_NAME
} // namespace kfr
