/** @addtogroup round
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

#include "impl/round.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Returns the largest integer value not greater than x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 floor(const T1& x)
{
    return intrinsics::floor(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 ceil(const T1& x)
{
    return intrinsics::ceil(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 round(const T1& x)
{
    return intrinsics::round(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 trunc(const T1& x)
{
    return intrinsics::trunc(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 fract(const T1& x)
{
    return intrinsics::fract(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> ifloor(const T1& x)
{
    return intrinsics::ifloor(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> iceil(const T1& x)
{
    return intrinsics::iceil(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> iround(const T1& x)
{
    return intrinsics::iround(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> itrunc(const T1& x)
{
    return intrinsics::itrunc(x);
}

template <typename T, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC T fmod(const T& x, const T& y)
{
    return x - trunc(x / y) * y;
}
KFR_FN(fmod)

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
constexpr KFR_INTRINSIC vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return x % y;
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return fmod(x, y);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
