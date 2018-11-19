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

#include "impl/hyperbolic.hpp"

namespace kfr
{

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC flt_type<T1> sinh(const T1& x)
{
    return intrinsics::sinh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::sinh, E1> sinh(E1&& x)
{
    return { fn::sinh(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC flt_type<T1> cosh(const T1& x)
{
    return intrinsics::cosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::cosh, E1> cosh(E1&& x)
{
    return { fn::cosh(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC flt_type<T1> tanh(const T1& x)
{
    return intrinsics::tanh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::tanh, E1> tanh(E1&& x)
{
    return { fn::tanh(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC flt_type<T1> coth(const T1& x)
{
    return intrinsics::coth(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::coth, E1> coth(E1&& x)
{
    return { fn::coth(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC flt_type<T1> sinhcosh(const T1& x)
{
    return intrinsics::sinhcosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::sinhcosh, E1> sinhcosh(E1&& x)
{
    return { fn::sinhcosh(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC flt_type<T1> coshsinh(const T1& x)
{
    return intrinsics::coshsinh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::coshsinh, E1> coshsinh(E1&& x)
{
    return { fn::coshsinh(), std::forward<E1>(x) };
}
}
