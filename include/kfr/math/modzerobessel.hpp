/** @addtogroup other_math
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

#include "impl/modzerobessel.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
/**
 * @brief Computes the modified zeroth-order Bessel function of the first kind.
 * This function calculates I₀(x), the modified Bessel function of the first kind
 * with order zero, for the given input value.
 * @tparam T1 The numeric type of the input parameter.
 * @param x The input value for which to compute the modified Bessel function
 * @return The computed value of I₀(x)
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 modzerobessel(const T1& x)
{
    return intrinsics::modzerobessel(x);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
