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

#include "../simd/comparison.hpp"
#include "../simd/complex.hpp"
#include "../simd/operators.hpp"
#include "../simd/vec.hpp"
#include "expression.hpp"
#include "univector.hpp"
#include <algorithm>

namespace kfr
{

/**
 * @brief Returns template expression that returns sum of all the arguments passed to a function.
 */
template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC xfunction<fn::add, E...> add(E&&... x)
{
    return { fn::add(), std::forward<E>(x)... };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::sub, E1, E2> sub(E1&& x, E2&& y)
{
    return { fn::sub(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns template expression that returns product of all the arguments passed to a function.
 */
template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC xfunction<fn::mul, E...> mul(E&&... x)
{
    return { fn::mul(), std::forward<E>(x)... };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::ipow, E1, E2> ipow(E1&& x, E2&& b)
{
    return { fn::ipow(), std::forward<E1>(x), std::forward<E2>(b) };
}

template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_INTRINSIC xfunction<fn::mix, E1, E2, E3> mix(E1&& c, E2&& x, E3&& y)
{
    return { fn::mix(), std::forward<E1>(c), std::forward<E2>(x), std::forward<E3>(y) };
}

template <typename E1, typename E2, typename E3, KFR_ACCEPT_EXPRESSIONS(E1, E2, E3)>
KFR_INTRINSIC xfunction<fn::mixs, E1, E2, E3> mixs(E1&& c, E2&& x, E3&& y)
{
    return { fn::mixs(), std::forward<E1>(c), std::forward<E2>(x), std::forward<E3>(y) };
}

template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC xfunction<fn::horner, E...> horner(E&&... x)
{
    return { fn::horner(), std::forward<E>(x)... };
}

template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC xfunction<fn::horner_even, E...> horner_even(E&&... x)
{
    return { fn::horner_even(), std::forward<E>(x)... };
}

template <typename... E, KFR_ACCEPT_EXPRESSIONS(E...)>
KFR_INTRINSIC xfunction<fn::horner_odd, E...> horner_odd(E&&... x)
{
    return { fn::horner_odd(), std::forward<E>(x)... };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::add, E1, E2> operator+(E1&& e1, E2&& e2)
{
    return { fn::add(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::sub, E1, E2> operator-(E1&& e1, E2&& e2)
{
    return { fn::sub(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::mul, E1, E2> operator*(E1&& e1, E2&& e2)
{
    return { fn::mul(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::div, E1, E2> operator/(E1&& e1, E2&& e2)
{
    return { fn::div(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::bitwiseand, E1, E2> operator&(E1&& e1, E2&& e2)
{
    return { fn::bitwiseand(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::bitwiseor, E1, E2> operator|(E1&& e1, E2&& e2)
{
    return { fn::bitwiseor(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::bitwisexor, E1, E2> operator^(E1&& e1, E2&& e2)
{
    return { fn::bitwisexor(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::shl, E1, E2> operator<<(E1&& e1, E2&& e2)
{
    return { fn::shl(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::shr, E1, E2> operator>>(E1&& e1, E2&& e2)
{
    return { fn::shr(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/**
 * @brief Returns template expression that returns square of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::sqr, E1> sqr(E1&& x)
{
    return { fn::sqr(), std::forward<E1>(x) };
}

/**
 * @brief Returns template expression that returns cube of x.
 */
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::cub, E1> cub(E1&& x)
{
    return { fn::cub(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::pow2, E1> pow2(E1&& x)
{
    return { fn::pow2(), std::forward<E1>(x) };
}
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::pow3, E1> pow3(E1&& x)
{
    return { fn::pow3(), std::forward<E1>(x) };
}
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::pow4, E1> pow4(E1&& x)
{
    return { fn::pow4(), std::forward<E1>(x) };
}
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::pow5, E1> pow5(E1&& x)
{
    return { fn::pow5(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::neg, E1> operator-(E1&& e1)
{
    return { fn::neg(), std::forward<E1>(e1) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::bitwisenot, E1> operator~(E1&& e1)
{
    return { fn::bitwisenot(), std::forward<E1>(e1) };
}

/// @brief Constructs complex value from real and imaginary parts
template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::make_complex, E1, E2> make_complex(E1&& re, E2&& im)
{
    return { fn::make_complex{}, std::forward<E1>(re), std::forward<E2>(im) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::equal, E1, E2> operator==(E1&& e1, E2&& e2)
{
    return { fn::equal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::notequal, E1, E2> operator!=(E1&& e1, E2&& e2)
{
    return { fn::notequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::less, E1, E2> operator<(E1&& e1, E2&& e2)
{
    return { fn::less(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::greater, E1, E2> operator>(E1&& e1, E2&& e2)
{
    return { fn::greater(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::lessorequal, E1, E2> operator<=(E1&& e1, E2&& e2)
{
    return { fn::lessorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_INTRINSIC xfunction<fn::greaterorequal, E1, E2> operator>=(E1&& e1, E2&& e2)
{
    return { fn::greaterorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

/// @brief Returns the real part of the complex value
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::real, E1> real(E1&& x)
{
    return { fn::real{}, std::forward<E1>(x) };
}

/// @brief Returns the imaginary part of the complex value
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC xfunction<fn::imag, E1> imag(E1&& x)
{
    return { fn::imag{}, std::forward<E1>(x) };
}

/// @brief Returns template expression that returns the complex conjugate of the complex number x
template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION xfunction<fn::cconj, E1> cconj(E1&& x)
{
    return { fn::cconj(), std::forward<E1>(x) };
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
CMT_INTRINSIC xfunction<fn::interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return { fn::interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}

} // namespace kfr
