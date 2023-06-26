/** @addtogroup math
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
#include "../simd/constants.hpp"
#include "../simd/operators.hpp"
#include "../simd/types.hpp"

namespace kfr
{

namespace compiletime
{

template <typename T>
constexpr inline T select(bool c, T x, T y)
{
    return c ? x : y;
}
template <typename T>
constexpr inline T trunc(T x)
{
    return static_cast<T>(static_cast<long long>(x));
}
template <typename T>
constexpr inline T abs(T x)
{
    return x < T() ? -x : x;
}
template <typename T>
constexpr inline T mulsign(T x, T y)
{
    return y < T() ? -x : x;
}
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
template <typename T>
constexpr inline T cos(T x)
{
    return sin(x + c_pi<T, 1, 2>);
}
} // namespace compiletime
} // namespace kfr
