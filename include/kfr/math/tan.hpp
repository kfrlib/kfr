/** @addtogroup trigonometric
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

#include "impl/tan.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Computes the tangent of the input (in radians).
 *
 * @tparam T1 The type of the input.
 * @param x Input value in radians.
 * @return The tangent of the input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> tan(const T1& x)
{
    return intr::tan(x);
}

/**
 * @brief Computes the tangent of the input (in degrees).
 *
 * @tparam T1 The type of the input.
 * @param x Input value in degrees.
 * @return The tangent of the input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> tandeg(const T1& x)
{
    return intr::tandeg(x);
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
