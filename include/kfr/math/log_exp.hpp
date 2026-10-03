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

#include "impl/log_exp.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns e raised to the given power x.
 *
 * Applied element-wise for vector inputs. Negative infinity input yields
 * zero; +infinity yields +infinity.
 *
 * @tparam T1 Input numeric type.
 * @param x Power to which e is raised.
 * @return e^x as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> exp(const T1& x)
{
    return intr::exp(x);
}

/**
 * @brief Returns 2 raised to the given power x.
 *
 * @tparam T1 Input numeric type.
 * @param x Power to which 2 is raised.
 * @return 2^x as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> exp2(const T1& x)
{
    return intr::exp2(x);
}

/**
 * @brief Returns 10 raised to the given power x.
 *
 * @tparam T1 Input numeric type.
 * @param x Power to which 10 is raised.
 * @return 10^x as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> exp10(const T1& x)
{
    return intr::exp10(x);
}

/**
 * @brief Returns the natural logarithm of x.
 *
 * @tparam T1 Input numeric type.
 * @param x Value whose natural logarithm is computed (must be non-negative).
 * @return ln(x) as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> log(const T1& x)
{
    return intr::log(x);
}

/**
 * @brief Returns the binary (base-2) logarithm of x.
 *
 * @tparam T1 Input numeric type.
 * @param x Value whose base-2 logarithm is computed (must be non-negative).
 * @return log2(x) as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> log2(const T1& x)
{
    return intr::log2(x);
}

/**
 * @brief Returns the common (base-10) logarithm of x.
 *
 * @tparam T1 Input numeric type.
 * @param x Value whose base-10 logarithm is computed (must be non-negative).
 * @return log10(x) as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> log10(const T1& x)
{
    return intr::log10(x);
}

/**
 * @brief Returns the unbiased exponent of x as a floating-point value
 * (equivalent to @c std::logb).
 *
 * For x equal to zero the result is negative infinity.
 *
 * @tparam T1 Input numeric type.
 * @param x Input value.
 * @return The exponent of x as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> logb(const T1& x)
{
    return intr::logb(x);
}

/**
 * @brief Returns the logarithm of x with base y.
 *
 * @tparam T1 Type of the argument x.
 * @tparam T2 Type of the base y.
 * @param x Value whose logarithm is computed.
 * @param y Base of the logarithm.
 * @return log_y(x) as @c flt_type<std::common_type_t<T1, T2>>.
 */
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> logn(const T1& x, const T2& y)
{
    return intr::logn(x, y);
}

/**
 * @brief Returns log(x) * m.
 *
 * @tparam T1 Type of the argument x.
 * @tparam T2 Type of the multiplier m.
 * @param x Value whose natural logarithm is computed.
 * @param y Multiplier (m) applied to the logarithm.
 * @return log(x) * m as @c flt_type<std::common_type_t<T1, T2>>.
 */
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> logm(const T1& x, const T2& y)
{
    return intr::logm(x, y);
}

/**
 * @brief Returns exp(x * m + a).
 *
 * Computes the exponential of a fused multiply-add expression in a single
 * call, which may be more efficient than evaluating the two operations
 * separately.
 *
 * @tparam T1 Type of the argument x.
 * @tparam T2 Type of the multiplier m.
 * @tparam T3 Type of the addend a.
 * @param x Input value.
 * @param y Multiplier (m).
 * @param z Addend (a).
 * @return exp(x * m + a) as @c flt_type<std::common_type_t<T1, T2, T3>>.
 */
template <numeric T1, numeric T2, numeric T3>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2, T3>> exp_fmadd(const T1& x, const T2& y, const T3& z)
{
    return intr::exp_fmadd(x, y, z);
}

/**
 * @brief Returns log(x) * m + a.
 *
 * Computes a fused multiply-add on the logarithm of x.
 *
 * @tparam T1 Type of the argument x.
 * @tparam T2 Type of the multiplier m.
 * @tparam T3 Type of the addend a.
 * @param x Input value (must be non-negative).
 * @param y Multiplier (m).
 * @param z Addend (a).
 * @return log(x) * m + a as @c flt_type<std::common_type_t<T1, T2, T3>>.
 */
template <numeric T1, numeric T2, numeric T3>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2, T3>> log_fmadd(const T1& x, const T2& y, const T3& z)
{
    return intr::log_fmadd(x, y, z);
}

/**
 * @brief Returns x raised to the given power y.
 *
 * For non-integer y the result is defined only for x >= 0. When x is
 * negative and y is a non-integer the result is NaN.
 *
 * @tparam T1 Type of the base x.
 * @tparam T2 Type of the exponent y.
 * @param x Base value.
 * @param y Exponent.
 * @return x^y as @c flt_type<std::common_type_t<T1, T2>>.
 */
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> pow(const T1& x, const T2& y)
{
    return intr::pow(x, y);
}

/**
 * @brief Returns the real y-th root of x.
 *
 * For odd integer y, root(-x, y) is the negative of root(x, y). For
 * non-integer y the result is defined only for x >= 0.
 *
 * @tparam T1 Type of the value x.
 * @tparam T2 Type of the root degree y.
 * @param x Value whose y-th root is computed.
 * @param y Root degree.
 * @return y-th root of x as @c flt_type<std::common_type_t<T1, T2>>.
 */
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> root(const T1& x, const T2& y)
{
    return intr::root(x, y);
}

/**
 * @brief Returns the cube root of x.
 *
 * Defined for negative x (returns a negative result).
 *
 * @tparam T1 Input numeric type.
 * @param x Value whose cube root is computed.
 * @return cbrt(x) as @c flt_type<T1>.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> cbrt(const T1& x)
{
    return intr::cbrt(x);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
