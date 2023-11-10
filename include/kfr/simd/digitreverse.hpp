/** @addtogroup shuffle
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
#include "shuffle.hpp"
#include "types.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace internal
{

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshift-count-overflow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshift-count-negative")

constexpr KFR_INTRINSIC u32 bit_permute_step_impl(u32 x, cvals_t<u32>) { return x; }

template <u32 m, u32 shift, u32... values>
constexpr KFR_INTRINSIC u32 bit_permute_step_impl(u32 x, cvals_t<u32, m, shift, values...>)
{
    return bit_permute_step_impl(((x & m) << shift) | ((x >> shift) & m), cvals_t<u32, values...>());
}

template <size_t bits>
constexpr KFR_INTRINSIC u32 digitreverse_impl(u32 x, csize_t<2>)
{
    constexpr cvals_t<u32, 0x55555555, 1, 0x33333333, 2, 0x0f0f0f0f, 4, 0x00ff00ff, 8, 0x0000ffff, 16>
        steps{};
    if constexpr (bits > 16)
        return bit_permute_step_impl(x, steps) >> (32 - bits);
    else if constexpr (bits > 8)
        return bit_permute_step_impl(x, steps[csizeseq<8>]) >> (16 - bits);
    else
        return bit_permute_step_impl(x, steps[csizeseq<6>]) >> (8 - bits);
}

template <size_t bits>
constexpr KFR_INTRINSIC u32 digitreverse_impl(u32 x, csize_t<4>)
{
    constexpr cvals_t<u32, 0x33333333, 2, 0x0f0f0f0f, 4, 0x00ff00ff, 8, 0x0000ffff, 16> steps{};
    if constexpr (bits > 16)
        return bit_permute_step_impl(x, steps) >> (32 - bits);
    else if constexpr (bits > 8)
        return bit_permute_step_impl(x, steps[csizeseq<6>]) >> (16 - bits);
    else
        return bit_permute_step_impl(x, steps[csizeseq<4>]) >> (8 - bits);
}

CMT_PRAGMA_GNU(GCC diagnostic pop)

template <size_t radix, size_t bits>
struct shuffle_index_digitreverse
{
    constexpr KFR_INTRINSIC size_t operator()(size_t index) const CMT_NOEXCEPT
    {
        return digitreverse_impl<bits>(static_cast<u32>(index), csize_t<radix>());
    }
};
} // namespace internal

template <size_t radix, size_t group = 1, typename T, size_t N>
KFR_INTRINSIC vec<T, N> digitreverse(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(
        csizeseq<N / group>.map(internal::shuffle_index_digitreverse<radix, ilog2(N / group)>())));
}

template <size_t groupsize = 1, typename T, size_t N>
KFR_INTRINSIC vec<T, N> bitreverse(const vec<T, N>& x)
{
    return digitreverse<2, groupsize>(x);
}

template <size_t groupsize = 1, typename T, size_t N>
KFR_INTRINSIC vec<T, N> digitreverse4(const vec<T, N>& x)
{
    return digitreverse<4, groupsize>(x);
}

template <size_t bits>
constexpr KFR_INTRINSIC u32 bitreverse(u32 x)
{
    return internal::digitreverse_impl<bits>(x, csize_t<2>());
}

template <size_t bits>
constexpr KFR_INTRINSIC u32 digitreverse4(u32 x)
{
    return internal::digitreverse_impl<bits>(x, csize_t<4>());
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
