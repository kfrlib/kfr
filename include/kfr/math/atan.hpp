/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

#include "impl/atan.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns the arc tangent of x. The returned angle is in the range \f$-\pi/2\f$ through
 * \f$\pi/2\f$.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> atan(const T1& x)
{
    return intr::atan(x);
}

/**
 * @brief Returns the arc tangent of x, expressed in degrees. The returned angle is in the range -90
 * through 90.
 *
 * @param x Input value.
 * @return Arc tangent of @p x in degrees, in the range [-90, 90].
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> atandeg(const T1& x)
{
    return intr::atandeg(x);
}

/**
 * @brief Returns the arc tangent of y/x using the signs of both arguments to determine the correct
 * quadrant. The result is in the range \f$[-\pi, \pi]\f$.
 *
 * The first argument corresponds to the numerator (typically denoted y) and the second to the
 * denominator (typically denoted x) of the underlying y/x ratio, matching the C/C++ @c atan2
 * convention.
 *
 * @param x Numerator of the ratio (y component).
 * @param y Denominator of the ratio (x component).
 * @return Arc tangent of y/x, in radians, in the range \f$[-\pi, \pi]\f$.
 */
template <numeric T1, numeric T2>
KFR_FUNCTION std::common_type_t<T1, T2> atan2(const T1& x, const T2& y)
{
    return intr::atan2(x, y);
}

/**
 * @brief Returns the arc tangent of y/x (expressed in degrees) using the signs of both arguments
 * to determine the correct quadrant. The result is in the range [-180, 180].
 *
 * The first argument corresponds to the numerator (typically denoted y) and the second to the
 * denominator (typically denoted x) of the underlying y/x ratio, matching the C/C++ @c atan2
 * convention.
 *
 * @param x Numerator of the ratio (y component).
 * @param y Denominator of the ratio (x component).
 * @return Arc tangent of y/x, in degrees, in the range [-180, 180].
 */
template <numeric T1, numeric T2>
KFR_FUNCTION std::common_type_t<T1, T2> atan2deg(const T1& x, const T2& y)
{
    return intr::atan2deg(x, y);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
