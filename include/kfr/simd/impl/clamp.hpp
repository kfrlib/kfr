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

#include "../min_max.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T>
KFR_INTRINSIC T clamp(const T& x, const T& lo, const T& hi)
{
    return max(min(x, hi), lo);
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> clamp(const vec<T, N>& x, const vec<T, N>& lo, const vec<T, N>& hi)
{
    return max(min(x, hi), lo);
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> clamp(const vec<T, N>& x, const vec<T, N>& hi)
{
    return max(min(x, hi), zerovector<T, N>());
}
} // namespace intrinsics
KFR_I_FN(clamp)
} // namespace CMT_ARCH_NAME
} // namespace kfr
