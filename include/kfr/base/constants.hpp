/** @addtogroup math
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "types.hpp"
#include <limits>

namespace kfr
{

// π (pi)
// c_pi<f64, 4>      = 4pi
// c_pi<f64, 3, 4>   = 3/4pi
template <typename T, int m = 1, int d = 1, typename Tsub = subtype<T>>
constexpr Tsub c_pi = Tsub(3.1415926535897932384626433832795 * m / d);

// π² (pi²)
// c_sqr_pi<f64, 4>      = 4pi²
// c_sqr_pi<f64, 3, 4>   = 3/4pi²
template <typename T, int m = 1, int d = 1, typename Tsub = subtype<T>>
constexpr Tsub c_sqr_pi = Tsub(9.8696044010893586188344909998762 * m / d);

// 1/π (1/pi)
// c_recip_pi<f64>       1/pi
// c_recip_pi<f64, 4>    4/pi
template <typename T, int m = 1, int d = 1, typename Tsub = subtype<T>>
constexpr Tsub c_recip_pi = Tsub(0.31830988618379067153776752674503 * m / d);

// degree to radian conversion factor
template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_degtorad = c_pi<T, 1, 180>;

// radian to degree conversion factor
template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_radtodeg = c_recip_pi<T, 180>;

// e, Euler's number
template <typename T, int m = 1, int d = 1, typename Tsub = subtype<T>>
constexpr Tsub c_e = Tsub(2.718281828459045235360287471352662 * m / d);

template <typename T, typename Tsub = subtype<T>>
constexpr unsigned c_mantissa_bits = sizeof(Tsub) == 32 ? 23 : 52;

template <typename T, typename Tsub = usubtype<T>>
constexpr Tsub c_mantissa_mask = (Tsub(1) << c_mantissa_bits<T>)-1;

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_epsilon = (std::numeric_limits<Tsub>::epsilon());

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_infinity = std::numeric_limits<Tsub>::infinity();

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_neginfinity = -std::numeric_limits<Tsub>::infinity();

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_qnan = std::numeric_limits<Tsub>::quiet_NaN();

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_recip_log_2 = Tsub(1.442695040888963407359924681001892137426645954);

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_recip_log_10 = Tsub(0.43429448190325182765112891891661);

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_log_2 = Tsub(0.69314718055994530941723212145818);

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub c_log_10 = Tsub(2.3025850929940456840179914546844);

template <typename T, int m = 1, int d = 1, typename Tsub = subtype<T>>
constexpr Tsub c_sqrt_2 = Tsub(1.4142135623730950488016887242097 * m / d);
}
