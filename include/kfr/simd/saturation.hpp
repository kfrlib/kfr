/** @addtogroup saturation
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

#include "impl/saturation.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Adds two arguments using saturation
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>),
          typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout satadd(const T1& x, const T2& y)
{
    return intrinsics::satadd(x, y);
}

/// @brief Subtracts two arguments using saturation
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>),
          typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout satsub(const T1& x, const T2& y)
{
    return intrinsics::satsub(x, y);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
