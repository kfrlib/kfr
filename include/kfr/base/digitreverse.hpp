/** @addtogroup shuffle
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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
#include "shuffle.hpp"
#include "types.hpp"

namespace kfr
{

namespace internal
{

template <size_t radix, size_t bits>
constexpr enable_if<radix == 2, u32> digitreverse(u32 x)
{
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
    return ((x >> 16) | (x << 16)) >> (32 - bits);
}

constexpr inline u32 bit_permute_step_simple(u32 x, u32 m, u32 shift)
{
    return ((x & m) << shift) | ((x >> shift) & m);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#pragma GCC diagnostic ignored "-Wshift-count-negative"

template <size_t radix, size_t bits>
constexpr enable_if<radix == 4, u32> digitreverse(u32 x)
{
    if (bits <= 2)
        return x;
    if (bits <= 4)
    {
        x = bit_permute_step_simple(x, 0x33333333, 2); // Bit index complement 1      regroups 4 bits
        return x >> (4 - bits);
    }
    if (bits <= 8)
    {
        x = bit_permute_step_simple(x, 0x33333333, 2); // Bit index complement 1      regroups 4 bits
        x = bit_permute_step_simple(x, 0x0f0f0f0f, 4); // Bit index complement 2      regroups 8 bits
        return x >> (8 - bits);
    }
    if (bits <= 16)
    {
        x = bit_permute_step_simple(x, 0x33333333, 2); // Bit index complement 1      regroups 4 bits
        x = bit_permute_step_simple(x, 0x0f0f0f0f, 4); // Bit index complement 2      regroups 8 bits
        x = bit_permute_step_simple(x, 0x00ff00ff, 8); // Bit index complement 3      regroups 16 bits
        return x >> (16 - bits);
    }
    if (bits <= 32)
    {
        x = bit_permute_step_simple(x, 0x33333333, 2); // Bit index complement 1      regroups 4 bits
        x = bit_permute_step_simple(x, 0x0f0f0f0f, 4); // Bit index complement 2      regroups 8 bits
        x = bit_permute_step_simple(x, 0x00ff00ff, 8); // Bit index complement 3      regroups 16 bits
        x = bit_permute_step_simple(x, 0x0000ffff, 16); // Bit index complement 4     regroups 32 bits
        return x >> (32 - bits);
    }
    return x;
}

#pragma GCC diagnostic pop

template <size_t radix, size_t bits>
struct shuffle_index_digitreverse
{
    constexpr inline size_t operator()(size_t index) const
    {
        return digitreverse<radix, bits>(static_cast<u32>(index));
    }
};
}

template <size_t radix, size_t groupsize = 1, typename T, size_t N>
CMT_INLINE vec<T, N> digitreverse(const vec<T, N>& x)
{
    return shufflevector<N, internal::shuffle_index_digitreverse<radix, ilog2(N / groupsize)>, groupsize>(x);
}

template <size_t groupsize = 1, typename T, size_t N>
CMT_INLINE vec<T, N> bitreverse(const vec<T, N>& x)
{
    return digitreverse<2, groupsize>(x);
}

template <size_t groupsize = 1, typename T, size_t N>
CMT_INLINE vec<T, N> digitreverse4(const vec<T, N>& x)
{
    return digitreverse<4, groupsize>(x);
}

template <size_t bits>
constexpr inline u32 bitreverse(u32 x)
{
    return internal::digitreverse<2, bits>(x);
}

template <size_t bits>
constexpr inline u32 digitreverse4(u32 x)
{
    return internal::digitreverse<4, bits>(x);
}
}
