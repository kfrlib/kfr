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
#include "../simd/constants.hpp"
#include "../simd/operators.hpp"
#include "../simd/types.hpp"

namespace kfr
{

namespace compiletime
{

/**
 * @brief Compile-time conditional select between two values.
 *
 * Returns @p x when @p c is true, otherwise @p y. Evaluated in a
 * constexpr context so it can be used to build compile-time math
 * formulas.
 *
 * @tparam T  Value type.
 * @param  c  Condition.
 * @param  x  Value returned when @p c is true.
 * @param  y  Value returned when @p c is false.
 * @return    @p x or @p y depending on @p c.
 */
template <typename T>
constexpr inline T select(bool c, T x, T y)
{
    return c ? x : y;
}

/**
 * @brief Compile-time truncation toward zero.
 *
 * Converts @p x to `long long` (rounding toward zero) and casts the
 * result back to @p T.
 *
 * @tparam T  Value type (must be representable as a long long).
 * @param  x  Value to truncate.
 * @return    @p x truncated to an integer of type @p T.
 */
template <typename T>
constexpr inline T trunc(T x)
{
    return static_cast<T>(static_cast<long long>(x));
}

/**
 * @brief Compile-time absolute value.
 *
 * @tparam T Value type.
 * @param  x Input value.
 * @return   Magnitude of @p x.
 */
template <typename T>
constexpr inline T abs(T x)
{
    return x < T() ? -x : x;
}

/**
 * @brief Compile-time multiplication by sign of a second value.
 *
 * Returns @p x with the sign of @p y applied to it.
 *
 * @tparam T Value type.
 * @param  x Magnitude source.
 * @param  y Sign source.
 * @return   `x * sign(y)`.
 */
template <typename T>
constexpr inline T mulsign(T x, T y)
{
    return y < T() ? -x : x;
}

/**
 * @brief Compile-time sine, accurate to better than 1e-4.
 *
 * Reduces the argument to the `[-pi/2, pi/2]` range and evaluates
 * the sine using a 6th-degree polynomial in `y * y` with a
 * fused-multiply-add accumulation step. Suitable for building
 * other compile-time math expressions.
 *
 * @tparam T Floating-point value type.
 * @param  x Angle in radians.
 * @return   `sin(x)`, evaluated at compile time.
 */
template <typename T>
constexpr inline T sin(T x)
{
    x              = x - trunc(x / c_pi<T, 2>) * c_pi<T, 2>;
    constexpr T c2 = -0.16665853559970855712890625;
    constexpr T c4 = +8.31427983939647674560546875e-3;
    constexpr T c6 = -1.85423981747590005397796630859375e-4;

    x -= c_pi<T>;
    T y = abs(x);
    y   = select(y > c_pi<T, 1, 2>, c_pi<T> - y, y);
    y   = mulsign(y, -x);

    const T y2 = y * y;
    T formula  = c6;
    const T y3 = y2 * y;
    formula    = fmadd(formula, y2, c4);
    formula    = fmadd(formula, y2, c2);
    formula    = formula * y3 + y;
    return formula;
}

/**
 * @brief Compile-time cosine, derived from @ref sin.
 *
 * Implemented as `sin(x + pi/2)`; shares the same accuracy as
 * @ref sin (better than 1e-4).
 *
 * @tparam T Floating-point value type.
 * @param  x Angle in radians.
 * @return   `cos(x)`, evaluated at compile time.
 */
template <typename T>
constexpr inline T cos(T x)
{
    return sin(x + c_pi<T, 1, 2>);
}
} // namespace compiletime
} // namespace kfr
