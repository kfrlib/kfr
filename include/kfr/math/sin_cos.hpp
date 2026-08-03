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

#include "impl/sin_cos.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns the trigonometric sine of x, evaluated in radians.
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in radians.
 * @return Sine of @p x, in the same floating-point type as @p x (or its element type for vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sin(const T1& x)
{
    return intr::sin(x);
}

/**
 * @brief Returns the trigonometric cosine of x, evaluated in radians.
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in radians.
 * @return Cosine of @p x, in the same floating-point type as @p x (or its element type for vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> cos(const T1& x)
{
    return intr::cos(x);
}

/**
 * @brief Returns a fast (lower-precision) approximation of the trigonometric sine of x.
 *
 * Lower accuracy than @ref sin, suitable for use cases where speed matters more than full precision.
 *
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in radians.
 * @return Approximate sine of @p x, in the same floating-point type as @p x (or its element type for
 * vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> fastsin(const T1& x)
{
    return intr::fastsin(x);
}

/**
 * @brief Returns a fast (lower-precision) approximation of the trigonometric cosine of x.
 *
 * Lower accuracy than @ref cos, suitable for use cases where speed matters more than full precision.
 *
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in radians.
 * @return Approximate cosine of @p x, in the same floating-point type as @p x (or its element type for
 * vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> fastcos(const T1& x)
{
    return intr::fastcos(x);
}

/**
 * @brief Returns the trigonometric sine of the even elements of @p x and cosine of the odd elements.
 *
 * @p x must be a vector with at least two elements; the result is a vector of the same size in which
 * element @c 2*i holds @c sin(x[2*i]) and element @c 2*i+1 holds @c cos(x[2*i+1]).
 *
 * @tparam T1 Vector type with at least two elements.
 * @param x Vector of angles in radians.
 * @return Vector with interleaved sine/cosine values.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sincos(const T1& x)
{
    return intr::sincos(x);
}

/**
 * @brief Returns the trigonometric cosine of the even elements of @p x and sine of the odd elements.
 *
 * @p x must be a vector with at least two elements; the result is a vector of the same size in which
 * element @c 2*i holds @c cos(x[2*i]) and element @c 2*i+1 holds @c sin(x[2*i+1]).
 *
 * @tparam T1 Vector type with at least two elements.
 * @param x Vector of angles in radians.
 * @return Vector with interleaved cosine/sine values.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> cossin(const T1& x)
{
    return intr::cossin(x);
}

/**
 * @brief Returns the trigonometric sine of the angle expressed in degrees.
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in degrees.
 * @return Sine of @p x, in the same floating-point type as @p x (or its element type for vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sindeg(const T1& x)
{
    return intr::sindeg(x);
}

/**
 * @brief Returns the trigonometric cosine of the angle expressed in degrees.
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in degrees.
 * @return Cosine of @p x, in the same floating-point type as @p x (or its element type for vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> cosdeg(const T1& x)
{
    return intr::cosdeg(x);
}

/**
 * @brief Returns a fast (lower-precision) approximation of the trigonometric sine of the angle expressed in
 * degrees.
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in degrees.
 * @return Approximate sine of @p x, in the same floating-point type as @p x (or its element type for
 * vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> fastsindeg(const T1& x)
{
    return intr::fastsindeg(x);
}

/**
 * @brief Returns a fast (lower-precision) approximation of the trigonometric cosine of the angle expressed in
 * degrees.
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Angle in degrees.
 * @return Approximate cosine of @p x, in the same floating-point type as @p x (or its element type for
 * vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> fastcosdeg(const T1& x)
{
    return intr::fastcosdeg(x);
}

/**
 * @brief Returns the trigonometric sine of the even elements and cosine of the odd elements of @p x,
 * where @p x is a vector of angles expressed in degrees.
 *
 * @tparam T1 Vector type with at least two elements.
 * @param x Vector of angles in degrees.
 * @return Vector with interleaved sine/cosine values.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sincosdeg(const T1& x)
{
    return intr::sincosdeg(x);
}

/**
 * @brief Returns the trigonometric cosine of the even elements and sine of the odd elements of @p x,
 * where @p x is a vector of angles expressed in degrees.
 *
 * @tparam T1 Vector type with at least two elements.
 * @param x Vector of angles in degrees.
 * @return Vector with interleaved cosine/sine values.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> cossindeg(const T1& x)
{
    return intr::cossindeg(x);
}

/**
 * @brief Returns the (normalized) sinc function of @p x.
 *
 * \f[
 * sinc(x) = \begin{cases} \frac{\sin(x)}{x} & |x| > \varepsilon \\ 1 & |x| \le \varepsilon \end{cases}
 * \f]
 *
 * The value at @c x = 0 is defined as the limit @c 1 to avoid division by zero; the same
 * convention is used element-wise for vector arguments.
 *
 * @tparam T1 Scalar or vector type. Integer arguments are promoted to a floating-point type.
 * @param x Argument in radians.
 * @return Sinc of @p x, in the same floating-point type as @p x (or its element type for vectors).
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sinc(const T1& x)
{
    return intr::sinc(x);
}

/**
 * @brief Returns @c sin(2x) from already computed @c sin(x) and @c cos(x).
 *
 * Uses the identity @c sin(2x) = 2*sin(x)*cos(x); computing @c sin(x) and @c cos(x) once
 * and deriving the rest with these helpers is cheaper than evaluating each angle separately.
 *
 * @tparam T Scalar or vector type.
 * @param sinx Value of @c sin(x).
 * @param cosx Value of @c cos(x).
 * @return @c sin(2x).
 */
template <typename T>
KFR_INTRINSIC T sin2x(const T& sinx, const T& cosx)
{
    return 2 * sinx * cosx;
}

/**
 * @brief Returns @c sin(3x) from already computed @c sin(x) and @c cos(x).
 *
 * Uses the identity @c sin(3x) = sin(x) * (4*cos^2(x) - 1).
 *
 * @tparam T Scalar or vector type.
 * @param sinx Value of @c sin(x).
 * @param cosx Value of @c cos(x).
 * @return @c sin(3x).
 */
template <typename T>
KFR_INTRINSIC T sin3x(const T& sinx, const T& cosx)
{
    return sinx * (-1 + 4 * sqr(cosx));
}

/**
 * @brief Returns @c cos(2x) from already computed @c sin(x) and @c cos(x).
 *
 * Uses the identity @c cos(2x) = cos^2(x) - sin^2(x).
 *
 * @tparam T Scalar or vector type.
 * @param sinx Value of @c sin(x).
 * @param cosx Value of @c cos(x).
 * @return @c cos(2x).
 */
template <typename T>
KFR_INTRINSIC T cos2x(const T& sinx, const T& cosx)
{
    return sqr(cosx) - sqr(sinx);
}

/**
 * @brief Returns @c cos(3x) from already computed @c sin(x) and @c cos(x).
 *
 * Uses the identity @c cos(3x) = cos(x) * (1 - 4*sin^2(x)).
 *
 * @tparam T Scalar or vector type.
 * @param sinx Value of @c sin(x).
 * @param cosx Value of @c cos(x).
 * @return @c cos(3x).
 */
template <typename T>
KFR_INTRINSIC T cos3x(const T& sinx, const T& cosx)
{
    return cosx * (1 - 4 * sqr(sinx));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
