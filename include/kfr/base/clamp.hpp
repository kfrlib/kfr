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

#include "min_max.hpp"

namespace kfr
{

namespace intrinsics
{

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> clamp(const vec<T, N>& x, const vec<T, N>& lo, const vec<T, N>& hi)
{
    return max(min(x, hi), lo);
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> clamp(const vec<T, N>& x, const vec<T, N>& hi)
{
    return max(min(x, hi), zerovector<T, N>());
}
}
KFR_I_FN(clamp)

template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value),
          typename Tout = common_type<T1, T2, T3>>
KFR_INTRIN Tout clamp(const T1& x, const T2& lo, const T3& hi)
{
    return intrinsics::clamp(static_cast<Tout>(x), static_cast<Tout>(lo), static_cast<Tout>(hi));
}

template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INTRIN internal::expression_function<fn::clamp, E1, E2, E3> clamp(E1&& x, E2&& lo, E3&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(lo), std::forward<E3>(hi) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout clamp(const T1& x, const T2& hi)
{
    return intrinsics::clamp(static_cast<Tout>(x), static_cast<Tout>(hi));
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN internal::expression_function<fn::clamp, E1, E2> clamp(E1&& x, E2&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(hi) };
}
}
