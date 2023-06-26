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

#include "impl/sin_cos.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/**
 * @brief Returns the trigonometric sine of x.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sin(const T1& x)
{
    return intrinsics::sin(x);
}

/**
 * @brief Returns the trigonometric cosine of x.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> cos(const T1& x)
{
    return intrinsics::cos(x);
}

/**
 * @brief Returns an approximation of the trigonometric sine of x.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> fastsin(const T1& x)
{
    return intrinsics::fastsin(x);
}

/**
 * @brief Returns an approximation of the trigonometric cosine of x.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> fastcos(const T1& x)
{
    return intrinsics::fastcos(x);
}

/**
 * @brief Returns the trigonometric sine of the even elements of the x and cosine of the odd elements. x must
 * be a vector.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sincos(const T1& x)
{
    return intrinsics::sincos(x);
}

/**
 * @brief Returns the trigonometric cosine of the even elements of the x and sine of the odd elements. x must
 * be a vector.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> cossin(const T1& x)
{
    return intrinsics::cossin(x);
}

/**
 * @brief Returns the trigonometric sine of the x (expressed in degrees).
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sindeg(const T1& x)
{
    return intrinsics::sindeg(x);
}

/**
 * @brief Returns the trigonometric cosine of the x (expressed in degrees).
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> cosdeg(const T1& x)
{
    return intrinsics::cosdeg(x);
}

/**
 * @brief Returns an approximation of the trigonometric sine of the x (expressed in degrees).
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> fastsindeg(const T1& x)
{
    return intrinsics::fastsindeg(x);
}

/**
 * @brief Returns an approximation of the trigonometric cosine of the x (expressed in degrees).
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> fastcosdeg(const T1& x)
{
    return intrinsics::fastcosdeg(x);
}

/**
 * @brief Returns the trigonometric sine of the even elements of the x and cosine of the odd elements. x must
 * be a vector and expressed in degrees.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sincosdeg(const T1& x)
{
    return intrinsics::sincosdeg(x);
}

/**
 * @brief Returns the trigonometric cosine of the even elements of the x and sine of the odd elements. x must
 * be a vector and expressed in degrees.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> cossindeg(const T1& x)
{
    return intrinsics::cossindeg(x);
}

/**
 * @brief Returns the sinc function of x.
 * \f[
 * sinc(x) = \frac{sin(x)}{x}
 * \f]
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sinc(const T1& x)
{
    return intrinsics::sinc(x);
}

/**
 * @brief Returns the trigonometric sine of the angle 2x using sin(x) and cos(x).
 */
template <typename T>
KFR_INTRINSIC T sin2x(const T& sinx, const T& cosx)
{
    return 2 * sinx * cosx;
}

/**
 * @brief Returns the trigonometric sine of the angle 3x using already computed sin(x) and cos(x).
 */
template <typename T>
KFR_INTRINSIC T sin3x(const T& sinx, const T& cosx)
{
    return sinx * (-1 + 4 * sqr(cosx));
}

/**
 * @brief Returns the trigonometric cosine of the angle 2x using already computed sin(x) and cos(x).
 */
template <typename T>
KFR_INTRINSIC T cos2x(const T& sinx, const T& cosx)
{
    return sqr(cosx) - sqr(sinx);
}

/**
 * @brief Returns the trigonometric cosine of the angle 3x using already computed sin(x) and cos(x).
 */
template <typename T>
KFR_INTRINSIC T cos3x(const T& sinx, const T& cosx)
{
    return cosx * (1 - 4 * sqr(sinx));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
