/** @addtogroup other_math
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

#include "impl/gamma.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Returns the approximate gamma function of an argument
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNCTION flt_type<T1> gamma(const T1& x)
{
    return intrinsics::gamma(x);
}

/// @brief Creates expression that returns the approximate gamma function of an argument
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNCTION internal::expression_function<fn::gamma, E1> gamma(E1&& x)
{
    return { fn::gamma(), std::forward<E1>(x) };
}

/// @brief Returns the approximate factorial of an argument
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNCTION flt_type<T1> factorial_approx(const T1& x)
{
    return intrinsics::factorial_approx(x);
}

/// @brief Creates expression that returns the approximate factorial of an argument
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNCTION internal::expression_function<fn::factorial_approx, E1> factorial_approx(E1&& x)
{
    return { fn::factorial_approx(), std::forward<E1>(x) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
