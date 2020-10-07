/** @addtogroup trigonometric
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

#include "impl/atan.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/**
 * @brief Returns the arc tangent of x. The returned angle is in the range \f$-\pi/2\f$ through
 * \f$\pi/2\f$.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> atan(const T1& x)
{
    return intrinsics::atan(x);
}

/**
 * @brief Returns template expression that returns the arc tangent of x.
 */
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::atan, E1> atan(E1&& x)
{
    return { fn::atan(), std::forward<E1>(x) };
}

/**
 * @brief Returns the arc tangent of the x, expressed in degrees. The returned angle is in the range -90
 * through 90.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> atandeg(const T1& x)
{
    return intrinsics::atandeg(x);
}

/**
 * @brief Returns template expression that returns the arc tangent of the x, expressed in degrees.
 */
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::atandeg, E1> atandeg(E1&& x)
{
    return { fn::atandeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the arc tangent of y/x using the signs of arguments to determine the correct quadrant.
 */
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
KFR_FUNCTION common_type<T1, T2> atan2(const T1& x, const T2& y)
{
    return intrinsics::atan2(x, y);
}

/**
 * @brief Returns template expression that returns the arc tangent of y/x.
 */
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_FUNCTION internal::expression_function<fn::atan2, E1, E2> atan2(E1&& x, E2&& y)
{
    return { fn::atan2(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the arc tangent of y/x (expressed in degrees) using the signs of arguments to determine the
 * correct quadrant.
 */
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
KFR_FUNCTION common_type<T1, T2> atan2deg(const T1& x, const T2& y)
{
    return intrinsics::atan2deg(x, y);
}

/**
 * @brief Returns template expression that returns the arc tangent of y/x (expressed in degrees).
 */
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_FUNCTION internal::expression_function<fn::atan2deg, E1, E2> atan2deg(E1&& x, E2&& y)
{
    return { fn::atan2deg(), std::forward<E1>(x), std::forward<E2>(y) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
