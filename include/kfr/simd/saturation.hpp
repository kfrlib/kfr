/** @addtogroup saturation
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Adds two numeric values using saturation arithmetic.
 *
 * Saturated addition clamps the result to the maximum or minimum value
 * representable by the result type if overflow or underflow occurs.
 *
 * @tparam T1 Type of the first operand.
 * @tparam T2 Type of the second operand.
 * @tparam Tout Common type deduced from T1 and T2 (result type).
 * @param x The first operand.
 * @param y The second operand.
 * @return The saturated sum of x and y.
 */
template <numeric T1, numeric T2, typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout satadd(const T1& x, const T2& y)
{
    return intr::satadd(x, y);
}

/**
 * @brief Subtracts two numeric values using saturation arithmetic.
 *
 * Saturated subtraction clamps the result to the maximum or minimum value
 * representable by the result type if overflow or underflow occurs.
 *
 * @tparam T1 Type of the first operand.
 * @tparam T2 Type of the second operand.
 * @tparam Tout Common type deduced from T1 and T2 (result type).
 * @param x The first operand.
 * @param y The second operand.
 * @return The saturated difference of x and y.
 */
template <numeric T1, numeric T2, typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout satsub(const T1& x, const T2& y)
{
    return intr::satsub(x, y);
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
