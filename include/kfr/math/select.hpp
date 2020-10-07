/** @addtogroup basic_math
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

#include "impl/select.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/**
 * @brief Returns x if m is true, otherwise return y. Order of the arguments is same as in ternary operator.
 * @code
 * return m ? x : y
 * @endcode
 */
template <typename T1, size_t N, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>),
          typename Tout = subtype<common_type<T2, T3>>>
KFR_INTRINSIC vec<Tout, N> select(const mask<T1, N>& m, const T2& x, const T3& y)
{
    static_assert(sizeof(T1) == sizeof(Tout), "select: incompatible types");
    return intrinsics::select(bitcast<Tout>(m.asvec()).asmask(), innercast<Tout>(x), innercast<Tout>(y));
}

/**
 * @brief Returns template expression that returns x if m is true, otherwise return y. Order of the arguments
 * is same as in ternary operator.
 */
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>)>
KFR_INTRINSIC internal::expression_function<fn::select, E1, E2, E3> select(E1&& m, E2&& x, E3&& y)
{
    return { fn::select(), std::forward<E1>(m), std::forward<E2>(x), std::forward<E3>(y) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
