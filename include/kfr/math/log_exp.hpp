/** @addtogroup exponential
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

#include "impl/log_exp.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/// @brief Returns e raised to the given power x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> exp(const T1& x)
{
    return intr::exp(x);
}

/// @brief Returns 2 raised to the given power x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> exp2(const T1& x)
{
    return intr::exp2(x);
}

/// @brief Returns 10 raised to the given power x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> exp10(const T1& x)
{
    return intr::exp10(x);
}

/// @brief Returns the natural logarithm of the x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> log(const T1& x)
{
    return intr::log(x);
}

/// @brief Returns the binary (base-2) logarithm of the x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> log2(const T1& x)
{
    return intr::log2(x);
}

/// @brief Returns the common (base-10) logarithm of the x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> log10(const T1& x)
{
    return intr::log10(x);
}

/// @brief Returns the rounded binary (base-2) logarithm of the x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> logb(const T1& x)
{
    return intr::logb(x);
}

/// @brief Returns the logarithm of the x with base y.
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> logn(const T1& x, const T2& y)
{
    return intr::logn(x, y);
}

/// @brief Returns log(x) * y.
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> logm(const T1& x, const T2& y)
{
    return intr::logm(x, y);
}

/// @brief Returns exp(x * m + a).
template <numeric T1, numeric T2, numeric T3>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2, T3>> exp_fmadd(const T1& x, const T2& y, const T3& z)
{
    return intr::exp_fmadd(x, y, z);
}

/// @brief Returns log(x) * m + a.
template <numeric T1, numeric T2, numeric T3>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2, T3>> log_fmadd(const T1& x, const T2& y, const T3& z)
{
    return intr::log_fmadd(x, y, z);
}

/// @brief Returns the x raised to the given power y.
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> pow(const T1& x, const T2& y)
{
    return intr::pow(x, y);
}

/// @brief Returns the real nth root of the x.
template <numeric T1, numeric T2>
KFR_FUNCTION flt_type<std::common_type_t<T1, T2>> root(const T1& x, const T2& y)
{
    return intr::root(x, y);
}

/// @brief Returns the cube root of the x.
template <numeric T1>
KFR_FUNCTION flt_type<T1> cbrt(const T1& x)
{
    return intr::cbrt(x);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
