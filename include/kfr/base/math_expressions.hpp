/** @addtogroup base
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

#include "../math.hpp"
#include "basic_expressions.hpp"
#include "expression.hpp"

namespace kfr
{

/**
 * @brief Returns the trigonometric sine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sin, E1> sin(E1&& x)
{
    return { fn::sin(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cos, E1> cos(E1&& x)
{
    return { fn::cos(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric sine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::fastsin, E1> fastsin(E1&& x)
{
    return { fn::fastsin(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric cosine of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::fastcos, E1> fastcos(E1&& x)
{
    return { fn::fastcos(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of the even elements of the x and
 * cosine of the odd elements. x must be a vector. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sincos, E1> sincos(E1&& x)
{
    return { fn::sincos(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of the even elements of the x and
 * sine of the odd elements. x must be a vector. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cossin, E1> cossin(E1&& x)
{
    return { fn::cossin(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of the x (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sindeg, E1> sindeg(E1&& x)
{
    return { fn::sindeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of the x (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cosdeg, E1> cosdeg(E1&& x)
{
    return { fn::cosdeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric sine of the x
 * (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::fastsindeg, E1> fastsindeg(E1&& x)
{
    return { fn::fastsindeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns an approximation of the trigonometric cosine of the x
 * (expressed in degrees). Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::fastcosdeg, E1> fastcosdeg(E1&& x)
{
    return { fn::fastcosdeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric sine of the even elements of the x and
 * cosine of the odd elements. x must be expressed in degrees. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sincosdeg, E1> sincosdeg(E1&& x)
{
    return { fn::sincosdeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the trigonometric cosine of the even elements of the x and
 * sine of the odd elements. x must be expressed in degrees. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cossindeg, E1> cossindeg(E1&& x)
{
    return { fn::cossindeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns the sinc function of x. Accepts and returns expressions.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sinc, E1> sinc(E1&& x)
{
    return { fn::sinc(), std::forward<E1>(x) };
}

/// @brief Creates expression that returns the approximate gamma function of an argument
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::gamma, E1> gamma(E1&& x)
{
    return { fn::gamma(), std::forward<E1>(x) };
}

/// @brief Creates expression that returns the approximate factorial of an argument
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::factorial_approx, E1> factorial_approx(E1&& x)
{
    return { fn::factorial_approx(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the positive square root of the x. \f$\sqrt{x}\f$
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sqrt, E1> sqrt(E1&& x)
{
    return { fn::sqrt(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::tan, E1> tan(E1&& x)
{
    return { fn::tan(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::tandeg, E1> tandeg(E1&& x)
{
    return { fn::tandeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc sine of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_make_function<fn::asin, E1> asin(E1&& x)
{
    return { fn::asin(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc cosine of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_make_function<fn::acos, E1> acos(E1&& x)
{
    return { fn::acos(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the sine of the the complex value x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::csin, E1> csin(E1&& x)
{
    return { fn::csin(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic sine of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::csinh, E1> csinh(E1&& x)
{
    return { fn::csinh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the cosine of the the complex value x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::ccos, E1> ccos(E1&& x)
{
    return { fn::ccos(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cosine of the the complex value x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::ccosh, E1> ccosh(E1&& x)
{
    return { fn::ccosh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the squared absolute value (magnitude squared) of the
/// complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cabssqr, E1> cabssqr(E1&& x)
{
    return { fn::cabssqr(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the absolute value (magnitude) of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cabs, E1> cabs(E1&& x)
{
    return { fn::cabs(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the phase angle (argument) of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::carg, E1> carg(E1&& x)
{
    return { fn::carg(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the natural logarithm of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::clog, E1> clog(E1&& x)
{
    return { fn::clog(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the binary (base-2) logarithm of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::clog2, E1> clog2(E1&& x)
{
    return { fn::clog2(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the common (base-10) logarithm of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::clog10, E1> clog10(E1&& x)
{
    return { fn::clog10(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns \f$e\f$ raised to the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cexp, E1> cexp(E1&& x)
{
    return { fn::cexp(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns 2 raised to the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cexp2, E1> cexp2(E1&& x)
{
    return { fn::cexp2(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns 10 raised to the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cexp10, E1> cexp10(E1&& x)
{
    return { fn::cexp10(), std::forward<E1>(x) };
}

/// @brief Returns template expression that converts complex number to polar
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::polar, E1> polar(E1&& x)
{
    return { fn::polar(), std::forward<E1>(x) };
}

/// @brief Returns template expression that converts complex number to cartesian
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cartesian, E1> cartesian(E1&& x)
{
    return { fn::cartesian(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns square root of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::csqrt, E1> csqrt(E1&& x)
{
    return { fn::csqrt(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns square of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::csqr, E1> csqr(E1&& x)
{
    return { fn::csqr(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc tangent of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::atan, E1> atan(E1&& x)
{
    return { fn::atan(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc tangent of the x, expressed in degrees.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::atandeg, E1> atandeg(E1&& x)
{
    return { fn::atandeg(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns the arc tangent of y/x.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_make_function<fn::atan2, E1, E2> atan2(E1&& x, E2&& y)
{
    return { fn::atan2(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns template expression that returns the arc tangent of y/x (expressed in degrees).
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_make_function<fn::atan2deg, E1, E2> atan2deg(E1&& x, E2&& y)
{
    return { fn::atan2deg(), std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::modzerobessel, E1> modzerobessel(E1&& x)
{
    return { fn::modzerobessel(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic sine of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sinh, E1> sinh(E1&& x)
{
    return { fn::sinh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cosine of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cosh, E1> cosh(E1&& x)
{
    return { fn::cosh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic tangent of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::tanh, E1> tanh(E1&& x)
{
    return { fn::tanh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cotangent of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::coth, E1> coth(E1&& x)
{
    return { fn::coth(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic sine of the even elements of the x and the
/// hyperbolic cosine of the odd elements of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::sinhcosh, E1> sinhcosh(E1&& x)
{
    return { fn::sinhcosh(), std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the hyperbolic cosine of the even elements of the x and
/// the hyperbolic sine of the odd elements of the x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::coshsinh, E1> coshsinh(E1&& x)
{
    return { fn::coshsinh(), std::forward<E1>(x) };
}

/// @brief Returns e raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::exp, E1> exp(E1&& x)
{
    return { fn::exp(), std::forward<E1>(x) };
}

/// @brief Returns 2 raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::exp2, E1> exp2(E1&& x)
{
    return { fn::exp2(), std::forward<E1>(x) };
}

/// @brief Returns 10 raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::exp10, E1> exp10(E1&& x)
{
    return { fn::exp10(), std::forward<E1>(x) };
}

/// @brief Returns the natural logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::log, E1> log(E1&& x)
{
    return { fn::log(), std::forward<E1>(x) };
}

/// @brief Returns the binary (base-2) logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::log2, E1> log2(E1&& x)
{
    return { fn::log2(), std::forward<E1>(x) };
}

/// @brief Returns the common (base-10) logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::log10, E1> log10(E1&& x)
{
    return { fn::log10(), std::forward<E1>(x) };
}

/// @brief Returns the rounded binary (base-2) logarithm of the x. Version that accepts and returns
/// expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::logb, E1> logb(E1&& x)
{
    return { fn::logb(), std::forward<E1>(x) };
}

/// @brief Returns the logarithm of the x with base y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_make_function<fn::logn, E1, E2> logn(E1&& x, E2&& y)
{
    return { fn::logn(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns log(x) * y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_make_function<fn::logm, E1, E2> logm(E1&& x, E2&& y)
{
    return { fn::logm(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns exp(x * m + a). Accepts and returns expressions.
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION expression_make_function<fn::exp_fmadd, E1, E2, E3> exp_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { fn::exp_fmadd(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

/// @brief Returns log(x) * m + a. Accepts and returns expressions.
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION expression_make_function<fn::log_fmadd, E1, E2, E3> log_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { fn::log_fmadd(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

/// @brief Returns the x raised to the given power y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_make_function<fn::pow, E1, E2> pow(E1&& x, E2&& y)
{
    return { fn::pow(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the real nth root of the x. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_make_function<fn::root, E1, E2> root(E1&& x, E2&& y)
{
    return { fn::root(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the cube root of the x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_make_function<fn::cbrt, E1> cbrt(E1&& x)
{
    return { fn::cbrt(), std::forward<E1>(x) };
}

} // namespace kfr
