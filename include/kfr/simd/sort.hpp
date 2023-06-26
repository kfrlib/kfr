/** @addtogroup sort
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

#include "min_max.hpp"
#include "shuffle.hpp"
#include "vec.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/**
 * @brief Sort the elements in the vector in ascending order
 * @param x input vector
 * @return sorted vector
 * @code
 * CHECK(sort(make_vector(1000, 1, 2, -10)) == make_vector(-10, 1, 2, 1000));
 * @endcode
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> sort(const vec<T, N>& x)
{
    constexpr size_t Nhalf = N / 2;
    vec<T, Nhalf> e        = low(x);
    vec<T, Nhalf> o        = high(x);
    constexpr auto blend0  = cconcat(csizes<1>, csizeseq<Nhalf - 1, 0, 0>);
    for (size_t i = 0; i < Nhalf; i++)
    {
        vec<T, Nhalf> t;
        t = min(e, o);
        o = max(e, o);
        o = rotateright<1>(o);
        e = t;
        t = max(e, o);
        o = min(e, o);
        e = t;
        t = blend(e, o, blend0);
        o = blend(o, e, blend0);
        o = rotateleft<1>(o);
        e = t;
    }
    return interleavehalves(concat(e, o));
}

/**
 * @brief Sort the elements in the vector in descending order
 * @param x input vector
 * @return sorted vector
 * @code
 * CHECK(sort(make_vector(1000, 1, 2, -10)) == make_vector(1000, 2, 1, -10));
 * @endcode
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> sortdesc(const vec<T, N>& x)
{
    constexpr size_t Nhalf = N / 2;
    vec<T, Nhalf> e        = low(x);
    vec<T, Nhalf> o        = high(x);
    constexpr auto blend0  = cconcat(csizes<1>, csizeseq<Nhalf - 1, 0, 0>);
    for (size_t i = 0; i < Nhalf; i++)
    {
        vec<T, Nhalf> t;
        t = max(e, o);
        o = min(e, o);
        o = rotateright<1>(o);
        e = t;
        t = min(e, o);
        o = max(e, o);
        e = t;
        t = blend(e, o, blend0);
        o = blend(o, e, blend0);
        o = rotateleft<1>(o);
        e = t;
    }
    return interleavehalves(concat(e, o));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
