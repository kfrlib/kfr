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

#include "../simd/abs.hpp"
#include "../simd/clamp.hpp"
#include "../simd/comparison.hpp"
#include "../simd/complex.hpp"
#include "../simd/min_max.hpp"
#include "../simd/operators.hpp"
#include "../simd/round.hpp"
#include "../simd/saturation.hpp"
#include "../simd/select.hpp"
#include "../simd/vec.hpp"
#include "expression.hpp"
#include "univector.hpp"
#include <algorithm>

namespace kfr
{

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Creates an expression that returns the sum of all the arguments passed to a function.
 */
template <typename... E>
    requires expression_arguments<E...>
KFR_INTRINSIC expression_make_function<fn::add, E...> add(E&&... x)
{
    return { fn::add(), std::forward<E>(x)... };
}

/**
 * @brief Creates an expression that returns the difference between @p x and @p y.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::sub, E1, E2> sub(E1&& x, E2&& y)
{
    return { fn::sub(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that returns the product of all the arguments passed to a function.
 */
template <typename... E>
    requires expression_arguments<E...>
KFR_INTRINSIC expression_make_function<fn::mul, E...> mul(E&&... x)
{
    return { fn::mul(), std::forward<E>(x)... };
}

/**
 * @brief Creates an expression that returns @p x raised to the integer power @p b.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::ipow, E1, E2> ipow(E1&& x, E2&& b)
{
    return { fn::ipow(), std::forward<E1>(x), std::forward<E2>(b) };
}

/**
 * @brief Creates an expression that returns the linear blend of @p x and @p y.
 *
 * Returns `x + (y - x) * c`. The blend factor @p c must be in the range \f$ [0, 1] \f$,
 * where `0` yields @p x and `1` yields @p y.
 */
template <typename E1, typename E2, typename E3>
    requires expression_arguments<E1, E2, E3>
KFR_INTRINSIC expression_make_function<fn::mix, E1, E2, E3> mix(E1&& c, E2&& x, E3&& y)
{
    return { fn::mix(), std::forward<E1>(c), std::forward<E2>(x), std::forward<E3>(y) };
}

/**
 * @brief Creates an expression that returns the signed linear blend of @p x and @p y.
 *
 * As `mix` but accepts a blend factor @p c in the range \f$ [-1, 1] \f$, mapping it to
 * \f$ [0, 1] \f$ internally.
 */
template <typename E1, typename E2, typename E3>
    requires expression_arguments<E1, E2, E3>
KFR_INTRINSIC expression_make_function<fn::mixs, E1, E2, E3> mixs(E1&& c, E2&& x, E3&& y)
{
    return { fn::mixs(), std::forward<E1>(c), std::forward<E2>(x), std::forward<E3>(y) };
}

/**
 * @brief Creates an expression that evaluates a polynomial using Horner's method.
 *
 * `horner(x, 1, 2, 3)` is equivalent to \f$ 3x^2 + 2x + 1 \f$.
 */
template <typename... E>
    requires expression_arguments<E...>
KFR_INTRINSIC expression_make_function<fn::horner, E...> horner(E&&... x)
{
    return { fn::horner(), std::forward<E>(x)... };
}

/**
 * @brief Creates an expression that evaluates a polynomial with even powers using Horner's method.
 *
 * `horner_even(x, 1, 2, 3)` is equivalent to \f$ 3x^4 + 2x^2 + 1 \f$.
 */
template <typename... E>
    requires expression_arguments<E...>
KFR_INTRINSIC expression_make_function<fn::horner_even, E...> horner_even(E&&... x)
{
    return { fn::horner_even(), std::forward<E>(x)... };
}

/**
 * @brief Creates an expression that evaluates a polynomial with odd powers using Horner's method.
 *
 * `horner_odd(x, 1, 2, 3)` is equivalent to \f$ 3x^5 + 2x^3 + 1x \f$.
 */
template <typename... E>
    requires expression_arguments<E...>
KFR_INTRINSIC expression_make_function<fn::horner_odd, E...> horner_odd(E&&... x)
{
    return { fn::horner_odd(), std::forward<E>(x)... };
}

/**
 * @brief Creates an expression that returns the sum of @p e1 and @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::add, E1, E2> operator+(E1&& e1, E2&& e2)
{
    return { fn::add(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the difference between @p e1 and @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::sub, E1, E2> operator-(E1&& e1, E2&& e2)
{
    return { fn::sub(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the product of @p e1 and @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::mul, E1, E2> operator*(E1&& e1, E2&& e2)
{
    return { fn::mul(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the quotient of @p e1 divided by @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::div, E1, E2> operator/(E1&& e1, E2&& e2)
{
    return { fn::div(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the remainder of @p e1 divided by @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::mod, E1, E2> operator%(E1&& e1, E2&& e2)
{
    return { fn::mod(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the bitwise AND of @p e1 and @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::bitwiseand, E1, E2> operator&(E1&& e1, E2&& e2)
{
    return { fn::bitwiseand(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the bitwise OR of @p e1 and @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::bitwiseor, E1, E2> operator|(E1&& e1, E2&& e2)
{
    return { fn::bitwiseor(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the bitwise XOR of @p e1 and @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::bitwisexor, E1, E2> operator^(E1&& e1, E2&& e2)
{
    return { fn::bitwisexor(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns @p e1 shifted left by @p e2 bits.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::shl, E1, E2> operator<<(E1&& e1, E2&& e2)
{
    return { fn::shl(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns @p e1 shifted right by @p e2 bits.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::shr, E1, E2> operator>>(E1&& e1, E2&& e2)
{
    return { fn::shr(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the square of @p x.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::sqr, E1> sqr(E1&& x)
{
    return { fn::sqr(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the cube of @p x.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::cub, E1> cub(E1&& x)
{
    return { fn::cub(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns @p x raised to the power of 2.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::pow2, E1> pow2(E1&& x)
{
    return { fn::pow2(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns @p x raised to the power of 3.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::pow3, E1> pow3(E1&& x)
{
    return { fn::pow3(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns @p x raised to the power of 4.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::pow4, E1> pow4(E1&& x)
{
    return { fn::pow4(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns @p x raised to the power of 5.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::pow5, E1> pow5(E1&& x)
{
    return { fn::pow5(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the arithmetic negation of @p e1.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::neg, E1> operator-(E1&& e1)
{
    return { fn::neg(), std::forward<E1>(e1) };
}

/**
 * @brief Creates an expression that returns the bitwise NOT of @p e1.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::bitwisenot, E1> operator~(E1&& e1)
{
    return { fn::bitwisenot(), std::forward<E1>(e1) };
}

/**
 * @brief Creates an expression that constructs a complex value from the real part @p re and the
 * imaginary part @p im.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::make_complex, E1, E2> make_complex(E1&& re, E2&& im)
{
    return { fn::make_complex{}, std::forward<E1>(re), std::forward<E2>(im) };
}

#ifdef KFR_ENABLE_EXPR_CMP

/**
 * @brief Creates an expression that returns the result of comparing @p e1 and @p e2 for equality.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::equal, E1, E2> operator==(E1&& e1, E2&& e2)
{
    return { fn::equal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the result of comparing @p e1 and @p e2 for inequality.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::notequal, E1, E2> operator!=(E1&& e1, E2&& e2)
{
    return { fn::notequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is less than @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::less, E1, E2> operator<(E1&& e1, E2&& e2)
{
    return { fn::less(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is greater than @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::greater, E1, E2> operator>(E1&& e1, E2&& e2)
{
    return { fn::greater(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is less than or equal to @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::lessorequal, E1, E2> operator<=(E1&& e1, E2&& e2)
{
    return { fn::lessorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is greater than or equal to @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::greaterorequal, E1, E2> operator>=(E1&& e1, E2&& e2)
{
    return { fn::greaterorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

#endif

/**
 * @brief Creates an expression that returns whether @p e1 is equal to @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::equal, E1, E2> eq(E1&& e1, E2&& e2)
{
    return { fn::equal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is not equal to @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::notequal, E1, E2> ne(E1&& e1, E2&& e2)
{
    return { fn::notequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is less than @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::less, E1, E2> lt(E1&& e1, E2&& e2)
{
    return { fn::less(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is greater than @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::greater, E1, E2> gt(E1&& e1, E2&& e2)
{
    return { fn::greater(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is less than or equal to @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::lessorequal, E1, E2> le(E1&& e1, E2&& e2)
{
    return { fn::lessorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns whether @p e1 is greater than or equal to @p e2.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::greaterorequal, E1, E2> ge(E1&& e1, E2&& e2)
{
    return { fn::greaterorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Creates an expression that returns the real part of the complex value @p x.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::real, E1> real(E1&& x)
{
    return { fn::real{}, std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the imaginary part of the complex value @p x.
 */
template <expression_argument E1>
KFR_INTRINSIC expression_make_function<fn::imag, E1> imag(E1&& x)
{
    return { fn::imag{}, std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the complex conjugate of the complex number @p x.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::cconj, E1> cconj(E1&& x)
{
    return { fn::cconj(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that interleaves the lanes of @p x and @p y.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return { fn::interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that returns @p x if @p m is true, otherwise returns @p y.
 *
 * The order of the arguments is the same as in the ternary operator.
 */
template <typename E1, typename E2, typename E3>
    requires expression_arguments<E1, E2, E3>
KFR_FUNCTION expression_make_function<fn::select, E1, E2, E3> select(E1&& m, E2&& x, E3&& y)
{
    return { fn::select(), std::forward<E1>(m), std::forward<E2>(x), std::forward<E3>(y) };
}

/**
 * @brief Creates an expression that returns the absolute value of @p x.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::abs, E1> abs(E1&& x)
{
    return { fn::abs(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the smaller of @p x and @p y.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_FUNCTION expression_make_function<fn::min, E1, E2> min(E1&& x, E2&& y)
{
    return { fn::min(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that returns the greater of @p x and @p y.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_FUNCTION expression_make_function<fn::max, E1, E2> max(E1&& x, E2&& y)
{
    return { fn::max(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that returns the smaller in magnitude of @p x and @p y.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_FUNCTION expression_make_function<fn::absmin, E1, E2> absmin(E1&& x, E2&& y)
{
    return { fn::absmin(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that returns the greater in magnitude of @p x and @p y.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_FUNCTION expression_make_function<fn::absmax, E1, E2> absmax(E1&& x, E2&& y)
{
    return { fn::absmax(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that returns the largest integer value not greater than @p x.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::floor, E1> floor(E1&& x)
{
    return { fn::floor(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the smallest integer value not less than @p x.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::ceil, E1> ceil(E1&& x)
{
    return { fn::ceil(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns @p x rounded to the nearest integer.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::round, E1> round(E1&& x)
{
    return { fn::round(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the integer part of @p x by removing its fractional part.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::trunc, E1> trunc(E1&& x)
{
    return { fn::trunc(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the fractional part of @p x.
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::fract, E1> fract(E1&& x)
{
    return { fn::fract(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the integer equivalent of floor(@p x).
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::ifloor, E1> ifloor(E1&& x)
{
    return { fn::ifloor(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the integer equivalent of ceil(@p x).
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::iceil, E1> iceil(E1&& x)
{
    return { fn::iceil(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the integer equivalent of round(@p x).
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::iround, E1> iround(E1&& x)
{
    return { fn::iround(), std::forward<E1>(x) };
}

/**
 * @brief Creates an expression that returns the integer equivalent of trunc(@p x).
 */
template <expression_argument E1>
KFR_FUNCTION expression_make_function<fn::itrunc, E1> itrunc(E1&& x)
{
    return { fn::itrunc(), std::forward<E1>(x) };
}

/// @brief Creates an expression that returns the first argument clamped to a range [lo, hi]
template <typename E1, typename E2, typename E3>
    requires expression_arguments<E1, E2, E3>
KFR_FUNCTION expression_make_function<fn::clamp, E1, E2, E3> clamp(E1&& x, E2&& lo, E3&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(lo), std::forward<E3>(hi) };
}

/// @brief Creates an expression that returns the first argument clamped to a range [0, hi]
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_FUNCTION expression_make_function<fn::clamp, E1, E2> clamp(E1&& x, E2&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(hi) };
}

/**
 * @brief Creates an expression that adds @p x and @p y using saturation.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::satadd, E1, E2> satadd(E1&& x, E2&& y)
{
    return { fn::satadd(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Creates an expression that subtracts @p y from @p x using saturation.
 */
template <typename E1, typename E2>
    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_make_function<fn::satsub, E1, E2> satsub(E1&& x, E2&& y)
{
    return { fn::satsub(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Adds @p e2 to @p e1 in place and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator+=(E1&& e1, E2&& e2)
{
    process(e1, operator+(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Subtracts @p e2 from @p e1 in place and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator-=(E1&& e1, E2&& e2)
{
    process(e1, operator-(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Multiplies @p e1 by @p e2 in place and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator*=(E1&& e1, E2&& e2)
{
    process(e1, operator*(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Divides @p e1 by @p e2 in place and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator/=(E1&& e1, E2&& e2)
{
    process(e1, operator/(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Assigns the remainder of @p e1 divided by @p e2 to @p e1 and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator%=(E1&& e1, E2&& e2)
{
    process(e1, operator%(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Assigns the bitwise OR of @p e1 and @p e2 to @p e1 and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator|=(E1&& e1, E2&& e2)
{
    process(e1, operator|(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Assigns the bitwise AND of @p e1 and @p e2 to @p e1 and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator&=(E1&& e1, E2&& e2)
{
    process(e1, operator&(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Assigns the bitwise XOR of @p e1 and @p e2 to @p e1 and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator^=(E1&& e1, E2&& e2)
{
    process(e1, operator^(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Shifts @p e1 left by @p e2 bits in place and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator<<=(E1&& e1, E2&& e2)
{
    process(e1, operator<<(e1, e2));
    return std::forward<E1>(e1);
}

/**
 * @brief Shifts @p e1 right by @p e2 bits in place and returns a reference to @p e1.
 */
template <input_output_expression E1, input_expression E2>
KFR_INTRINSIC std::remove_reference_t<E1> operator>>=(E1&& e1, E2&& e2)
{
    process(e1, operator>>(e1, e2));
    return std::forward<E1>(e1);
}

} // namespace KFR_ARCH_NAME

} // namespace kfr
