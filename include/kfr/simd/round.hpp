/** @addtogroup round
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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

#include "impl/round.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns the largest integer value not greater than @p x.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return The largest integer value not greater than @p x.
 */
template <numeric T1>
KFR_INTRINSIC T1 floor(const T1& x)
{
    return intr::floor(x);
}

/**
 * @brief Returns the smallest integer value not less than @p x.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return The smallest integer value not less than @p x.
 */
template <numeric T1>
KFR_INTRINSIC T1 ceil(const T1& x)
{
    return intr::ceil(x);
}

/**
 * @brief Returns the nearest integer value to @p x.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return The nearest integer value to @p x.
 */
template <numeric T1>
KFR_INTRINSIC T1 round(const T1& x)
{
    return intr::round(x);
}

/**
 * @brief Returns the integer part of @p x by removing its fractional part.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return The truncated integer part of @p x.
 */
template <numeric T1>
KFR_INTRINSIC T1 trunc(const T1& x)
{
    return intr::trunc(x);
}

/**
 * @brief Returns the fractional part of @p x.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return The fractional part of @p x.
 */
template <numeric T1>
KFR_INTRINSIC T1 fract(const T1& x)
{
    return intr::fract(x);
}

/**
 * @brief Returns the largest integer value not greater than @p x, as an integer type.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return Integer equivalent of floor(@p x).
 */
template <numeric T1>
KFR_INTRINSIC itype<T1> ifloor(const T1& x)
{
    return intr::ifloor(x);
}

/**
 * @brief Returns the smallest integer value not less than @p x, as an integer type.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return Integer equivalent of ceil(@p x).
 */
template <numeric T1>
KFR_INTRINSIC itype<T1> iceil(const T1& x)
{
    return intr::iceil(x);
}

/**
 * @brief Returns the nearest integer value to @p x, as an integer type.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return Integer equivalent of round(@p x).
 */
template <numeric T1>
KFR_INTRINSIC itype<T1> iround(const T1& x)
{
    return intr::iround(x);
}

/**
 * @brief Returns the truncated integer part of @p x, as an integer type.
 * @tparam T1 Numeric type.
 * @param x Input value.
 * @return Integer equivalent of trunc(@p x).
 */
template <numeric T1>
KFR_INTRINSIC itype<T1> itrunc(const T1& x)
{
    return intr::itrunc(x);
}

/**
 * @brief Returns the floating-point remainder of dividing @p x by @p y.
 * @tparam T Floating-point type.
 * @param x Dividend.
 * @param y Divisor.
 * @return The remainder of the division @p x / @p y.
 */
template <f_class T>
KFR_INTRINSIC T fmod(const T& x, const T& y)
{
    return x - trunc(x / y) * y;
}
KFR_FN(fmod)

/**
 * @brief Returns the element-wise remainder of integer vectors @p x and @p y.
 * @tparam T Integer type.
 * @tparam N Number of elements in the vector.
 * @param x Dividend vector.
 * @param y Divisor vector.
 * @return Element-wise remainder of @p x divided by @p y.
 */
template <not_f_class T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return x % y;
}

/**
 * @brief Returns the element-wise floating-point remainder of vectors @p x and @p y.
 * @tparam T Floating-point type.
 * @tparam N Number of elements in the vector.
 * @param x Dividend vector.
 * @param y Divisor vector.
 * @return Element-wise floating-point remainder of @p x divided by @p y.
 */
template <f_class T, size_t N>
KFR_INTRINSIC vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return fmod(x, y);
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
/** @} */
