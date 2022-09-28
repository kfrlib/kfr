/** @addtogroup expressions
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

#include "../math.hpp"
#include "new_expressions.hpp"
#include "expression.hpp"

namespace kfr
{

/**
 * @brief Returns template expression that returns x if m is true, otherwise return y. Order of the arguments
 * is same as in ternary operator.
 */
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION xfunction<fn::select, E1, E2, E3> select(E1&& m, E2&& x, E3&& y)
{
    return { fn::select(), std::forward<E1>(m), std::forward<E2>(x), std::forward<E3>(y) };
}

/**
 * @brief Returns template expression that returns the absolute value of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::abs, E1> abs(E1&& x)
{
    return { fn::abs(), std::forward<E1>(x) };
}

/**
 * @brief Returns the smaller of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::min, E1, E2> min(E1&& x, E2&& y)
{
    return { fn::min(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the greater of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::max, E1, E2> max(E1&& x, E2&& y)
{
    return { fn::max(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the smaller in magnitude of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::absmin, E1, E2> absmin(E1&& x, E2&& y)
{
    return { fn::absmin(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the greater in magnitude of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::absmax, E1, E2> absmax(E1&& x, E2&& y)
{
    return { fn::absmax(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the largest integer value not greater than x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::floor, E1> floor(E1&& x)
{
    return { fn::floor(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::ceil, E1> ceil(E1&& x)
{
    return { fn::ceil(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::round, E1> round(E1&& x)
{
    return { fn::round(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::trunc, E1> trunc(E1&& x)
{
    return { fn::trunc(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::fract, E1> fract(E1&& x)
{
    return { fn::fract(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::ifloor, E1> ifloor(E1&& x)
{
    return { fn::ifloor(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::iceil, E1> iceil(E1&& x)
{
    return { fn::iceil(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::iround, E1> iround(E1&& x)
{
    return { fn::iround(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::itrunc, E1> itrunc(E1&& x)
{
    return { fn::itrunc(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sin, E1> sin(E1&& x)
{
    return { fn::sin(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cos, E1> cos(E1&& x)
{
    return { fn::cos(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric sine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::fastsin, E1> fastsin(E1&& x)
{
    return { fn::fastsin(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric cosine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::fastcos, E1> fastcos(E1&& x)
{
    return { fn::fastcos(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of the even elements of the x and
 * cosine of the odd elements. x must be a vector. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sincos, E1> sincos(E1&& x)
{
    return { fn::sincos(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of the even elements of the x and
 * sine of the odd elements. x must be a vector. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cossin, E1> cossin(E1&& x)
{
    return { fn::cossin(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of the x (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sindeg, E1> sindeg(E1&& x)
{
    return { fn::sindeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of the x (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cosdeg, E1> cosdeg(E1&& x)
{
    return { fn::cosdeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric sine of the x
 * (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::fastsindeg, E1> fastsindeg(E1&& x)
{
    return { fn::fastsindeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric cosine of the x
 * (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::fastcosdeg, E1> fastcosdeg(E1&& x)
{
    return { fn::fastcosdeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of the even elements of the x and
 * cosine of the odd elements. x must be expressed in degrees. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sincosdeg, E1> sincosdeg(E1&& x)
{
    return { fn::sincosdeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of the even elements of the x and
 * sine of the odd elements. x must be expressed in degrees. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cossindeg, E1> cossindeg(E1&& x)
{
    return { fn::cossindeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the sinc function of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sinc, E1> sinc(E1&& x)
{
    return { fn::sinc(), std::forward<E1>(x) };
}

/// @brief Creates an expression that returns the first argument clamped to a range [lo, hi]
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION xfunction<fn::clamp, E1, E2, E3> clamp(E1&& x, E2&& lo, E3&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(lo), std::forward<E3>(hi) };
}

/// @brief Creates an expression that returns the first argument clamped to a range [0, hi]
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::clamp, E1, E2> clamp(E1&& x, E2&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(hi) };
}

/// @brief Creates expression that returns the approximate gamma function of an argument
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::gamma, E1> gamma(E1&& x)
{
    return { fn::gamma(), std::forward<E1>(x) };
}

/// @brief Creates expression that returns the approximate factorial of an argument
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::factorial_approx, E1> factorial_approx(E1&& x)
{
    return { fn::factorial_approx(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the positive square root of the x. \f$\sqrt{x}\f$
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sqrt, E1> sqrt(E1&& x)
{
    return { fn::sqrt(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::tan, E1> tan(E1&& x)
{
    return { fn::tan(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::tandeg, E1> tandeg(E1&& x)
{
    return { fn::tandeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc sine of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::asin, E1> asin(E1&& x)
{
    return { fn::asin(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc cosine of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::acos, E1> acos(E1&& x)
{
    return { fn::acos(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the sine of the the complex value x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::csin, E1> csin(E1&& x)
{
    return { fn::csin(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic sine of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::csinh, E1> csinh(E1&& x)
{
    return { fn::csinh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the cosine of the the complex value x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::ccos, E1> ccos(E1&& x)
{
    return { fn::ccos(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cosine of the the complex value x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::ccosh, E1> ccosh(E1&& x)
{
    return { fn::ccosh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the squared absolute value (magnitude squared) of the
/// complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cabssqr, E1> cabssqr(E1&& x)
{
    return { fn::cabssqr(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the absolute value (magnitude) of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cabs, E1> cabs(E1&& x)
{
    return { fn::cabs(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the phase angle (argument) of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::carg, E1> carg(E1&& x)
{
    return { fn::carg(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the natural logarithm of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::clog, E1> clog(E1&& x)
{
    return { fn::clog(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the binary (base-2) logarithm of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::clog2, E1> clog2(E1&& x)
{
    return { fn::clog2(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the common (base-10) logarithm of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::clog10, E1> clog10(E1&& x)
{
    return { fn::clog10(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns \f$e\f$ raised to the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cexp, E1> cexp(E1&& x)
{
    return { fn::cexp(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns 2 raised to the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cexp2, E1> cexp2(E1&& x)
{
    return { fn::cexp2(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns 10 raised to the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cexp10, E1> cexp10(E1&& x)
{
    return { fn::cexp10(), std::forward<E1>(x) };
}

/// @brief Returns template expression that converts complex number to polar
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::polar, E1> polar(E1&& x)
{
    return { fn::polar(), std::forward<E1>(x) };
}

/// @brief Returns template expression that converts complex number to cartesian
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cartesian, E1> cartesian(E1&& x)
{
    return { fn::cartesian(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns square root of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::csqrt, E1> csqrt(E1&& x)
{
    return { fn::csqrt(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns square of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::csqr, E1> csqr(E1&& x)
{
    return { fn::csqr(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc tangent of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::atan, E1> atan(E1&& x)
{
    return { fn::atan(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc tangent of the x, expressed in degrees.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::atandeg, E1> atandeg(E1&& x)
{
    return { fn::atandeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc tangent of y/x.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::atan2, E1, E2> atan2(E1&& x, E2&& y)
{
    return { fn::atan2(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns template expression that returns the arc tangent of y/x (expressed in degrees).
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::atan2deg, E1, E2> atan2deg(E1&& x, E2&& y)
{
    return { fn::atan2deg(), std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::modzerobessel, E1> modzerobessel(E1&& x)
{
    return { fn::modzerobessel(), std::forward<E1>(x) };
}

/// @brief Creates an expression that adds two arguments using saturation
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::satadd, E1, E2> satadd(E1&& x, E2&& y)
{
    return { fn::satadd(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Creates an expression that subtracts two arguments using saturation
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::satsub, E1, E2> satsub(E1&& x, E2&& y)
{
    return { fn::satsub(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns template expression that returns the hyperbolic sine of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sinh, E1> sinh(E1&& x)
{
    return { fn::sinh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cosine of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cosh, E1> cosh(E1&& x)
{
    return { fn::cosh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic tangent of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::tanh, E1> tanh(E1&& x)
{
    return { fn::tanh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cotangent of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::coth, E1> coth(E1&& x)
{
    return { fn::coth(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic sine of the even elements of the x and the
/// hyperbolic cosine of the odd elements of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::sinhcosh, E1> sinhcosh(E1&& x)
{
    return { fn::sinhcosh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cosine of the even elements of the x and
/// the hyperbolic sine of the odd elements of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::coshsinh, E1> coshsinh(E1&& x)
{
    return { fn::coshsinh(), std::forward<E1>(x) };
}

/// @brief Returns e raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::exp, E1> exp(E1&& x)
{
    return { fn::exp(), std::forward<E1>(x) };
}

/// @brief Returns 2 raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::exp2, E1> exp2(E1&& x)
{
    return { fn::exp2(), std::forward<E1>(x) };
}

/// @brief Returns 10 raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::exp10, E1> exp10(E1&& x)
{
    return { fn::exp10(), std::forward<E1>(x) };
}

/// @brief Returns the natural logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::log, E1> log(E1&& x)
{
    return { fn::log(), std::forward<E1>(x) };
}

/// @brief Returns the binary (base-2) logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::log2, E1> log2(E1&& x)
{
    return { fn::log2(), std::forward<E1>(x) };
}

/// @brief Returns the common (base-10) logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::log10, E1> log10(E1&& x)
{
    return { fn::log10(), std::forward<E1>(x) };
}

/// @brief Returns the rounded binary (base-2) logarithm of the x. Version that accepts and returns
/// expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::logb, E1> logb(E1&& x)
{
    return { fn::logb(), std::forward<E1>(x) };
}

/// @brief Returns the logarithm of the x with base y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::logn, E1, E2> logn(E1&& x, E2&& y)
{
    return { fn::logn(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns log(x) * y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::logm, E1, E2> logm(E1&& x, E2&& y)
{
    return { fn::logm(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns exp(x * m + a). Accepts and returns expressions.
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION xfunction<fn::exp_fmadd, E1, E2, E3> exp_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { fn::exp_fmadd(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

/// @brief Returns log(x) * m + a. Accepts and returns expressions.
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION xfunction<fn::log_fmadd, E1, E2, E3> log_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { fn::log_fmadd(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

/// @brief Returns the x raised to the given power y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::pow, E1, E2> pow(E1&& x, E2&& y)
{
    return { fn::pow(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the real nth root of the x. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::root, E1, E2> root(E1&& x, E2&& y)
{
    return { fn::root(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the cube root of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cbrt, E1> cbrt(E1&& x)
{
    return { fn::cbrt(), std::forward<E1>(x) };
}

} // namespace kfr
