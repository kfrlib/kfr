/**
 * Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 */
#pragma once

#include "../vec.hpp"
#ifndef KFR_SHUFFLE_SPECIALIZATIONS
#include "../shuffle.hpp"
#endif

#ifdef CMT_COMPILER_GNU

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <>
template <>
inline vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 1, 8, 9, 16, 17, 24, 25, 2, 3, 10, 11, 18, 19, 26, 27, 4, 5, 12, 13, 20, 21, 28, 29, 6, 7, 14,
             15, 22, 23, 30, 31>) const CMT_NOEXCEPT
{
    f32x32 w = *this;

    w = concat(permute<0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 10, 11, 6, 7, 14, 15>(low(w)),
               permute<0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 10, 11, 6, 7, 14, 15>(high(w)));

    w = permutegroups<(4), 0, 4, 2, 6, 1, 5, 3, 7>(w); // avx: vperm2f128 & vinsertf128, sse: no-op
    return w;
}

template <>
template <>
inline vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 1, 16, 17, 8, 9, 24, 25, 4, 5, 20, 21, 12, 13, 28, 29, 2, 3, 18, 19, 10, 11, 26, 27, 6, 7, 22,
             23, 14, 15, 30, 31>) const CMT_NOEXCEPT
{
    f32x32 w = *this;

    w = concat(permute<0, 1, 8, 9, 4, 5, 12, 13, /**/ 2, 3, 10, 11, 6, 7, 14, 15>(even<8>(w)),
               permute<0, 1, 8, 9, 4, 5, 12, 13, /**/ 2, 3, 10, 11, 6, 7, 14, 15>(odd<8>(w)));

    w = permutegroups<(4), 0, 4, 1, 5, 2, 6, 3, 7>(w); // avx: vperm2f128 & vinsertf128, sse: no-op
    return w;
}

inline vec<f32, 32> bitreverse_2(const vec<f32, 32>& x)
{
    return x.shuffle(csizes<0, 1, 16, 17, 8, 9, 24, 25, 4, 5, 20, 21, 12, 13, 28, 29, 2, 3, 18, 19, 10, 11,
                            26, 27, 6, 7, 22, 23, 14, 15, 30, 31>);
}

template <>
template <>
inline vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 32, 33, 16, 17, 48, 49, 8, 9, 40, 41, 24, 25, 56, 57, 4, 5, 36, 37, 20, 21, 52, 53, 12, 13,
             44, 45, 28, 29, 60, 61, 2, 3, 34, 35, 18, 19, 50, 51, 10, 11, 42, 43, 26, 27, 58, 59, 6, 7, 38,
             39, 22, 23, 54, 55, 14, 15, 46, 47, 30, 31, 62, 63>) const CMT_NOEXCEPT
{
    return permutegroups<(8), 0, 4, 1, 5, 2, 6, 3, 7>(
        concat(bitreverse_2(even<8>(*this)), bitreverse_2(odd<8>(*this))));
}

template <>
template <>
inline vec<f32, 16> vec<f32, 16>::shuffle(
    csizes_t<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15>) const CMT_NOEXCEPT
{
    const vec<f32, 16> xx = permutegroups<(4), 0, 2, 1, 3>(*this);

    return concat(low(xx).shuffle(high(xx), csizes<0, 2, 8 + 0, 8 + 2, 4, 6, 8 + 4, 8 + 6>),
                  low(xx).shuffle(high(xx), csizes<1, 3, 8 + 1, 8 + 3, 5, 7, 8 + 5, 8 + 7>));
}

template <>
template <>
inline vec<f32, 16> vec<f32, 16>::shuffle(
    csizes_t<0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15>) const CMT_NOEXCEPT
{
    const vec<f32, 16> xx =
        concat(low(*this).shuffle(high(*this), csizes<0, 8 + 0, 1, 8 + 1, 4, 8 + 4, 5, 8 + 5>),
               low(*this).shuffle(high(*this), csizes<2, 8 + 2, 3, 8 + 3, 6, 8 + 6, 7, 8 + 7>));

    return permutegroups<(4), 0, 2, 1, 3>(xx);
}

template <>
template <>
inline vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13,
             29, 14, 30, 15, 31>) const CMT_NOEXCEPT
{
    const vec<f32, 32> xx = permutegroups<(8), 0, 2, 1, 3>(*this);

    return concat(interleavehalves(low(xx)), interleavehalves(high(xx)));
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
#endif
