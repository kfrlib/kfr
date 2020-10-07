/** @addtogroup exponential
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

#include "impl/log_exp.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Returns e raised to the given power x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> exp(const T1& x)
{
    return intrinsics::exp(x);
}

/// @brief Returns e raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::exp, E1> exp(E1&& x)
{
    return { fn::exp(), std::forward<E1>(x) };
}

/// @brief Returns 2 raised to the given power x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> exp2(const T1& x)
{
    return intrinsics::exp2(x);
}

/// @brief Returns 2 raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::exp2, E1> exp2(E1&& x)
{
    return { fn::exp2(), std::forward<E1>(x) };
}

/// @brief Returns 10 raised to the given power x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> exp10(const T1& x)
{
    return intrinsics::exp10(x);
}

/// @brief Returns 10 raised to the given power x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::exp10, E1> exp10(E1&& x)
{
    return { fn::exp10(), std::forward<E1>(x) };
}

/// @brief Returns the natural logarithm of the x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> log(const T1& x)
{
    return intrinsics::log(x);
}

/// @brief Returns the natural logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::log, E1> log(E1&& x)
{
    return { fn::log(), std::forward<E1>(x) };
}

/// @brief Returns the binary (base-2) logarithm of the x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> log2(const T1& x)
{
    return intrinsics::log2(x);
}

/// @brief Returns the binary (base-2) logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::log2, E1> log2(E1&& x)
{
    return { fn::log2(), std::forward<E1>(x) };
}

/// @brief Returns the common (base-10) logarithm of the x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> log10(const T1& x)
{
    return intrinsics::log10(x);
}

/// @brief Returns the common (base-10) logarithm of the x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::log10, E1> log10(E1&& x)
{
    return { fn::log10(), std::forward<E1>(x) };
}

/// @brief Returns the rounded binary (base-2) logarithm of the x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> logb(const T1& x)
{
    return intrinsics::logb(x);
}

/// @brief Returns the rounded binary (base-2) logarithm of the x. Version that accepts and returns
/// expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::logb, E1> logb(E1&& x)
{
    return { fn::logb(), std::forward<E1>(x) };
}

/// @brief Returns the logarithm of the x with base y.
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
KFR_FUNCTION flt_type<common_type<T1, T2>> logn(const T1& x, const T2& y)
{
    return intrinsics::logn(x, y);
}

/// @brief Returns the logarithm of the x with base y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_FUNCTION internal::expression_function<fn::logn, E1, E2> logn(E1&& x, E2&& y)
{
    return { fn::logn(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns log(x) * y.
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
KFR_FUNCTION flt_type<common_type<T1, T2>> logm(const T1& x, const T2& y)
{
    return intrinsics::logm(x, y);
}

/// @brief Returns log(x) * y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_FUNCTION internal::expression_function<fn::logm, E1, E2> logm(E1&& x, E2&& y)
{
    return { fn::logm(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns exp(x * m + a).
template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>)>
KFR_FUNCTION flt_type<common_type<T1, T2, T3>> exp_fmadd(const T1& x, const T2& y, const T3& z)
{
    return intrinsics::exp_fmadd(x, y, z);
}

/// @brief Returns exp(x * m + a). Accepts and returns expressions.
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>)>
KFR_FUNCTION internal::expression_function<fn::exp_fmadd, E1, E2, E3> exp_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { fn::exp_fmadd(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

/// @brief Returns log(x) * m + a.
template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>)>
KFR_FUNCTION flt_type<common_type<T1, T2, T3>> log_fmadd(const T1& x, const T2& y, const T3& z)
{
    return intrinsics::log_fmadd(x, y, z);
}

/// @brief Returns log(x) * m + a. Accepts and returns expressions.
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>)>
KFR_FUNCTION internal::expression_function<fn::log_fmadd, E1, E2, E3> log_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { fn::log_fmadd(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

/// @brief Returns the x raised to the given power y.
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
KFR_FUNCTION flt_type<common_type<T1, T2>> pow(const T1& x, const T2& y)
{
    return intrinsics::pow(x, y);
}

/// @brief Returns the x raised to the given power y. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_FUNCTION internal::expression_function<fn::pow, E1, E2> pow(E1&& x, E2&& y)
{
    return { fn::pow(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the real nth root of the x.
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
KFR_FUNCTION flt_type<common_type<T1, T2>> root(const T1& x, const T2& y)
{
    return intrinsics::root(x, y);
}

/// @brief Returns the real nth root of the x. Accepts and returns expressions.
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_FUNCTION internal::expression_function<fn::root, E1, E2> root(E1&& x, E2&& y)
{
    return { fn::root(), std::forward<E1>(x), std::forward<E2>(y) };
}

/// @brief Returns the cube root of the x.
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> cbrt(const T1& x)
{
    return intrinsics::cbrt(x);
}

/// @brief Returns the cube root of the x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION internal::expression_function<fn::cbrt, E1> cbrt(E1&& x)
{
    return { fn::cbrt(), std::forward<E1>(x) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
