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

#include "constants.hpp"
#include "vec.hpp"

namespace kfr
{

CMT_INLINE float bitwisenot(float x) { return fbitcast(~ubitcast(x)); }
CMT_INLINE float bitwiseor(float x, float y) { return fbitcast(ubitcast(x) | ubitcast(y)); }
CMT_INLINE float bitwiseand(float x, float y) { return fbitcast(ubitcast(x) & ubitcast(y)); }
CMT_INLINE float bitwiseandnot(float x, float y) { return fbitcast(ubitcast(x) & ~ubitcast(y)); }
CMT_INLINE float bitwisexor(float x, float y) { return fbitcast(ubitcast(x) ^ ubitcast(y)); }
CMT_INLINE double bitwisenot(double x) { return fbitcast(~ubitcast(x)); }
CMT_INLINE double bitwiseor(double x, double y) { return fbitcast(ubitcast(x) | ubitcast(y)); }
CMT_INLINE double bitwiseand(double x, double y) { return fbitcast(ubitcast(x) & ubitcast(y)); }
CMT_INLINE double bitwiseandnot(double x, double y) { return fbitcast(ubitcast(x) & ~ubitcast(y)); }
CMT_INLINE double bitwisexor(double x, double y) { return fbitcast(ubitcast(x) ^ ubitcast(y)); }

/// @brief Bitwise Not
template <typename T1>
CMT_INLINE T1 bitwisenot(const T1& x)
{
    return ~x;
}
KFR_FN(bitwisenot)

/// @brief Bitwise And
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> bitwiseand(const T1& x, const T2& y)
{
    return x & y;
}
template <typename T>
constexpr CMT_INLINE T bitwiseand(initialvalue<T>)
{
    return constants<T>::allones();
}
KFR_FN(bitwiseand)

/// @brief Bitwise And-Not
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> bitwiseandnot(const T1& x, const T2& y)
{
    return x & ~y;
}
template <typename T>
constexpr inline T bitwiseandnot(initialvalue<T>)
{
    return constants<T>::allones();
}
KFR_FN(bitwiseandnot)

/// @brief Bitwise Or
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> bitwiseor(const T1& x, const T2& y)
{
    return x | y;
}
template <typename T>
constexpr CMT_INLINE T bitwiseor(initialvalue<T>)
{
    return subtype<T>(0);
}
KFR_FN(bitwiseor)

/// @brief Bitwise Xor (Exclusive Or)
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> bitwisexor(const T1& x, const T2& y)
{
    return x ^ y;
}
template <typename T>
constexpr CMT_INLINE T bitwisexor(initialvalue<T>)
{
    return subtype<T>();
}
KFR_FN(bitwisexor)

/// @brief Bitwise Left shift
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> shl(const T1& left, const T2& right)
{
    return left << right;
}
KFR_FN(shl)

/// @brief Bitwise Right shift
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> shr(const T1& left, const T2& right)
{
    return left >> right;
}
KFR_FN(shr)

/// @brief Bitwise Left Rotate
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> rol(const T1& left, const T2& right)
{
    return shl(left, right) | shr(left, (static_cast<subtype<T1>>(typebits<T1>::bits) - right));
}
KFR_FN(rol)

/// @brief Bitwise Right Rotate
template <typename T1, typename T2>
CMT_INLINE common_type<T1, T2> ror(const T1& left, const T2& right)
{
    return shr(left, right) | shl(left, (static_cast<subtype<T1>>(typebits<T1>::bits) - right));
}
KFR_FN(ror)
}
