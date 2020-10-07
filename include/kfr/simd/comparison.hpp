/** @addtogroup logical
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

#include "constants.hpp"
#include "impl/function.hpp"
#include "vec.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T1, typename T2>
inline maskfor<common_type<T1, T2>> equal(const T1& x, const T2& y)
{
    return x == y;
}
template <typename T1, typename T2>
inline maskfor<common_type<T1, T2>> notequal(const T1& x, const T2& y)
{
    return x != y;
}
template <typename T1, typename T2>
inline maskfor<common_type<T1, T2>> less(const T1& x, const T2& y)
{
    return x < y;
}
template <typename T1, typename T2>
inline maskfor<common_type<T1, T2>> greater(const T1& x, const T2& y)
{
    return x > y;
}
template <typename T1, typename T2>
inline maskfor<common_type<T1, T2>> lessorequal(const T1& x, const T2& y)
{
    return x <= y;
}
template <typename T1, typename T2>
inline maskfor<common_type<T1, T2>> greaterorequal(const T1& x, const T2& y)
{
    return x >= y;
}
KFR_FN(equal)
KFR_FN(notequal)
KFR_FN(less)
KFR_FN(greater)
KFR_FN(lessorequal)
KFR_FN(greaterorequal)

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::equal, E1, E2> operator==(E1&& e1, E2&& e2)
{
    return { fn::equal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::notequal, E1, E2> operator!=(E1&& e1, E2&& e2)
{
    return { fn::notequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::less, E1, E2> operator<(E1&& e1, E2&& e2)
{
    return { fn::less(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::greater, E1, E2> operator>(E1&& e1, E2&& e2)
{
    return { fn::greater(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::lessorequal, E1, E2> operator<=(E1&& e1, E2&& e2)
{
    return { fn::lessorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
KFR_INTRINSIC internal::expression_function<fn::greaterorequal, E1, E2> operator>=(E1&& e1, E2&& e2)
{
    return { fn::greaterorequal(), std::forward<E1>(e1), std::forward<E2>(e2) };
}

template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isnan(const vec<T, N>& x)
{
    return x != x;
}

template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isinf(const vec<T, N>& x)
{
    return x == constants<T>::infinity || x == -constants<T>::infinity;
}

template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isfinite(const vec<T, N>& x)
{
    return !isnan(x) && !isinf(x);
}

template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isnegative(const vec<T, N>& x)
{
    return (x & constants<T>::highbitmask()) != 0;
}

template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> ispositive(const vec<T, N>& x)
{
    return !isnegative(x);
}

template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> iszero(const vec<T, N>& x)
{
    return x == T();
}

template <typename T1, typename T2, typename T3>
KFR_INTRINSIC maskfor<common_type<T1, T2, T3>> inrange(const T1& x, const T2& min, const T3& max)
{
    return x >= min && x <= max;
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
