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

#include "../../simd/impl/function.hpp"
#include "../../simd/select.hpp"
#include "../atan.hpp"
#include "../sqrt.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> asin(const vec<T, N>& x)
{
    const vec<Tout, N> xx = x;
    return atan2(xx, sqrt(Tout(1) - xx * xx));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> acos(const vec<T, N>& x)
{
    const vec<Tout, N> xx = x;
    return atan2(sqrt(Tout(1) - xx * xx), xx);
}
KFR_HANDLE_SCALAR(asin)
KFR_HANDLE_SCALAR(acos)
} // namespace intrinsics
KFR_I_FN(asin)
KFR_I_FN(acos)
} // namespace CMT_ARCH_NAME

} // namespace kfr
