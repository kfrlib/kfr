/** @addtogroup math
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

#include "../atan.hpp"
#include "../function.hpp"
#include "../select.hpp"
#include "../sqrt.hpp"

namespace kfr
{

namespace intrinsics
{

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_SINTRIN vec<Tout, N> asin(const vec<T, N>& x)
{
    const vec<Tout, N> xx = x;
    return atan2(xx, sqrt(Tout(1) - xx * xx));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_SINTRIN vec<Tout, N> acos(const vec<T, N>& x)
{
    const vec<Tout, N> xx = x;
    return -atan2(xx, sqrt(Tout(1) - xx * xx)) + constants<Tout>::pi * 0.5;
}
KFR_I_FLT_CONVERTER(asin)
KFR_I_FLT_CONVERTER(acos)
} // namespace intrinsics
KFR_I_FN(asin)
KFR_I_FN(acos)

} // namespace kfr
