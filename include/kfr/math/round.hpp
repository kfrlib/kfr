/** @addtogroup round
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

#include "impl/round.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Returns the largest integer value not greater than x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 floor(const T1& x)
{
    return intrinsics::floor(x);
}

/// @brief Returns the largest integer value not greater than x. Accepts and returns expressions.
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::floor, E1> floor(E1&& x)
{
    return { fn::floor(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 ceil(const T1& x)
{
    return intrinsics::ceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::ceil, E1> ceil(E1&& x)
{
    return { fn::ceil(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 round(const T1& x)
{
    return intrinsics::round(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::round, E1> round(E1&& x)
{
    return { fn::round(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 trunc(const T1& x)
{
    return intrinsics::trunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::trunc, E1> trunc(E1&& x)
{
    return { fn::trunc(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 fract(const T1& x)
{
    return intrinsics::fract(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::fract, E1> fract(E1&& x)
{
    return { fn::fract(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> ifloor(const T1& x)
{
    return intrinsics::ifloor(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::ifloor, E1> ifloor(E1&& x)
{
    return { fn::ifloor(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> iceil(const T1& x)
{
    return intrinsics::iceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::iceil, E1> iceil(E1&& x)
{
    return { fn::iceil(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> iround(const T1& x)
{
    return intrinsics::iround(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::iround, E1> iround(E1&& x)
{
    return { fn::iround(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC itype<T1> itrunc(const T1& x)
{
    return intrinsics::itrunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_function<fn::itrunc, E1> itrunc(E1&& x)
{
    return { fn::itrunc(), std::forward<E1>(x) };
}

template <typename T, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC T fmod(const T& x, const T& y)
{
    return x - trunc(x / y) * y;
}
KFR_FN(fmod)

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
constexpr KFR_INTRINSIC vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return x % y;
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return fmod(x, y);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
