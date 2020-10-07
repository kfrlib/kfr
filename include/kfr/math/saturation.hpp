/** @addtogroup saturation
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

#include "impl/saturation.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Adds two arguments using saturation
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>),
          typename Tout = common_type<T1, T2>>
KFR_INTRINSIC Tout satadd(const T1& x, const T2& y)
{
    return intrinsics::satadd(x, y);
}

/// @brief Creates an expression that adds two arguments using saturation
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::satadd, E1, E2> satadd(E1&& x, E2&& y)
{
    return { fn::satadd(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Subtracts two arguments using saturation
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>),
          typename Tout = common_type<T1, T2>>
KFR_INTRINSIC Tout satsub(const T1& x, const T2& y)
{
    return intrinsics::satsub(x, y);
}

/// @brief Creates an expression that subtracts two arguments using saturation
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::satsub, E1, E2> satsub(E1&& x, E2&& y)
{
    return { fn::satsub(), std::forward<E1>(x), std::forward<E2>(y) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
