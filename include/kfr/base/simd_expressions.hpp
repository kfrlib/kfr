/** @addtogroup expressions
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

inline namespace CMT_ARCH_NAME
{

/**
 * @brief Returns template expression that returns sum of all the arguments passed to a function.
 */
template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC expression_function<fn::add, E...> add(E&&... x)
{
    return { fn::add(), std::forward<E>(x)... };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::sub, E1, E2> sub(E1&& x, E2&& y)
{
    return { fn::sub(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns template expression that returns product of all the arguments passed to a function.
 */
template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC expression_function<fn::mul, E...> mul(E&&... x)
{
    return { fn::mul(), std::forward<E>(x)... };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::ipow, E1, E2> ipow(E1&& x, E2&& b)
{
    return { fn::ipow(), std::forward<E1>(x), std::forward<E2>(b) };
}

template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_INTRINSIC expression_function<fn::mix, E1, E2, E3> mix(E1&& c, E2&& x, E3&& y)
{
    return { fn::mix(), std::forward<E1>(c), std::forward<E2>(x), std::forward<E3>(y) };
}

template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_INTRINSIC expression_function<fn::mixs, E1, E2, E3> mixs(E1&& c, E2&& x, E3&& y)
{
    return { fn::mixs(), std::forward<E1>(c), std::forward<E2>(x), std::forward<E3>(y) };
}

template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC expression_function<fn::horner, E...> horner(E&&... x)
{
    return { fn::horner(), std::forward<E>(x)... };
}

template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC expression_function<fn::horner_even, E...> horner_even(E&&... x)
{
    return { fn::horner_even(), std::forward<E>(x)... };
}

template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC expression_function<fn::horner_odd, E...> horner_odd(E&&... x)
{
    return { fn::horner_odd(), std::forward<E>(x)... };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::add, E1, E2> operator+(E1&& e1, E2&& e2)
{
    return { fn::add(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::sub, E1, E2> operator-(E1&& e1, E2&& e2)
{
    return { fn::sub(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::mul, E1, E2> operator*(E1&& e1, E2&& e2)
{
    return { fn::mul(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::div, E1, E2> operator/(E1&& e1, E2&& e2)
{
    return { fn::div(), std::forward<E1>(e1), std::forward<E2>(e2) };
}
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::mod, E1, E2> operator%(E1&& e1, E2&& e2)
{
    return { fn::mod(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::bitwiseand, E1, E2> operator&(E1&& e1, E2&& e2)
{
    return { fn::bitwiseand(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::bitwiseor, E1, E2> operator|(E1&& e1, E2&& e2)
{
    return { fn::bitwiseor(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::bitwisexor, E1, E2> operator^(E1&& e1, E2&& e2)
{
    return { fn::bitwisexor(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::shl, E1, E2> operator<<(E1&& e1, E2&& e2)
{
    return { fn::shl(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::shr, E1, E2> operator>>(E1&& e1, E2&& e2)
{
    return { fn::shr(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Returns template expression that returns square of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::sqr, E1> sqr(E1&& x)
{
    return { fn::sqr(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns cube of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::cub, E1> cub(E1&& x)
{
    return { fn::cub(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::pow2, E1> pow2(E1&& x)
{
    return { fn::pow2(), std::forward<E1>(x) };
}
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::pow3, E1> pow3(E1&& x)
{
    return { fn::pow3(), std::forward<E1>(x) };
}
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::pow4, E1> pow4(E1&& x)
{
    return { fn::pow4(), std::forward<E1>(x) };
}
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::pow5, E1> pow5(E1&& x)
{
    return { fn::pow5(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::neg, E1> operator-(E1&& e1)
{
    return { fn::neg(), std::forward<E1>(e1) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::bitwisenot, E1> operator~(E1&& e1)
{
    return { fn::bitwisenot(), std::forward<E1>(e1) };
}

/// @brief Constructs complex value from real and imaginary parts
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::make_complex, E1, E2> make_complex(E1&& re, E2&& im)
{
    return { fn::make_complex{}, std::forward<E1>(re), std::forward<E2>(im) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::equal, E1, E2> operator==(E1&& e1, E2&& e2)
{
    return { fn::equal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::notequal, E1, E2> operator!=(E1&& e1, E2&& e2)
{
    return { fn::notequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::less, E1, E2> operator<(E1&& e1, E2&& e2)
{
    return { fn::less(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::greater, E1, E2> operator>(E1&& e1, E2&& e2)
{
    return { fn::greater(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::lessorequal, E1, E2> operator<=(E1&& e1, E2&& e2)
{
    return { fn::lessorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::greaterorequal, E1, E2> operator>=(E1&& e1, E2&& e2)
{
    return { fn::greaterorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/// @brief Returns the real part of the complex value
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::real, E1> real(E1&& x)
{
    return { fn::real{}, std::forward<E1>(x) };
}

/// @brief Returns the imaginary part of the complex value
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::imag, E1> imag(E1&& x)
{
    return { fn::imag{}, std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the complex conjugate of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::cconj, E1> cconj(E1&& x)
{
    return { fn::cconj(), std::forward<E1>(x) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
CMT_INTRINSIC expression_function<fn::interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return { fn::interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns template expression that returns x if m is true, otherwise return y. Order of the arguments
 * is same as in ternary operator.
 */
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION expression_function<fn::select, E1, E2, E3> select(E1&& m, E2&& x, E3&& y)
{
    return { fn::select(), std::forward<E1>(m), std::forward<E2>(x), std::forward<E3>(y) };
}

/**
 * @brief Returns template expression that returns the absolute value of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::abs, E1> abs(E1&& x)
{
    return { fn::abs(), std::forward<E1>(x) };
}

/**
 * @brief Returns the smaller of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_function<fn::min, E1, E2> min(E1&& x, E2&& y)
{
    return { fn::min(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the greater of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_function<fn::max, E1, E2> max(E1&& x, E2&& y)
{
    return { fn::max(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the smaller in magnitude of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_function<fn::absmin, E1, E2> absmin(E1&& x, E2&& y)
{
    return { fn::absmin(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the greater in magnitude of two values. Accepts and returns expressions.
 */
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_function<fn::absmax, E1, E2> absmax(E1&& x, E2&& y)
{
    return { fn::absmax(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the largest integer value not greater than x. Accepts and returns expressions.
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::floor, E1> floor(E1&& x)
{
    return { fn::floor(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::ceil, E1> ceil(E1&& x)
{
    return { fn::ceil(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::round, E1> round(E1&& x)
{
    return { fn::round(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::trunc, E1> trunc(E1&& x)
{
    return { fn::trunc(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::fract, E1> fract(E1&& x)
{
    return { fn::fract(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::ifloor, E1> ifloor(E1&& x)
{
    return { fn::ifloor(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::iceil, E1> iceil(E1&& x)
{
    return { fn::iceil(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::iround, E1> iround(E1&& x)
{
    return { fn::iround(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::itrunc, E1> itrunc(E1&& x)
{
    return { fn::itrunc(), std::forward<E1>(x) };
}

/// @brief Creates an expression that returns the first argument clamped to a range [lo, hi]
template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_FUNCTION expression_function<fn::clamp, E1, E2, E3> clamp(E1&& x, E2&& lo, E3&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(lo), std::forward<E3>(hi) };
}

/// @brief Creates an expression that returns the first argument clamped to a range [0, hi]
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION expression_function<fn::clamp, E1, E2> clamp(E1&& x, E2&& hi)
{
    return { fn::clamp(), std::forward<E1>(x), std::forward<E2>(hi) };
}

/// @brief Creates an expression that adds two arguments using saturation
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::satadd, E1, E2> satadd(E1&& x, E2&& y)
{
    return { fn::satadd(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Creates an expression that subtracts two arguments using saturation
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC expression_function<fn::satsub, E1, E2> satsub(E1&& x, E2&& y)
{
    return { fn::satsub(), std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator+=(E1&& e1, E2&& e2)
{
    process(e1, operator+(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator-=(E1&& e1, E2&& e2)
{
    process(e1, operator-(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator*=(E1&& e1, E2&& e2)
{
    process(e1, operator*(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator/=(E1&& e1, E2&& e2)
{
    process(e1, operator/(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator%=(E1&& e1, E2&& e2)
{
    process(e1, operator%(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator|=(E1&& e1, E2&& e2)
{
    process(e1, operator|(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator&=(E1&& e1, E2&& e2)
{
    process(e1, operator&(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator^=(E1&& e1, E2&& e2)
{
    process(e1, operator^(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator<<=(E1&& e1, E2&& e2)
{
    process(e1, operator<<(e1, e2));
    return e1;
}
template <typename E1, typename E2, KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC E1& operator>>=(E1&& e1, E2&& e2)
{
    process(e1, operator>>(e1, e2));
    return e1;
}

} // namespace CMT_ARCH_NAME

} // namespace kfr
