/** @addtogroup hyperbolic
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

#include "impl/hyperbolic.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Returns the hyperbolic sine of the x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sinh(const T1& x)
{
    return intrinsics::sinh(x);
}

/// @brief Returns the hyperbolic cosine of the x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> cosh(const T1& x)
{
    return intrinsics::cosh(x);
}

/// @brief Returns the hyperbolic tangent of the x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> tanh(const T1& x)
{
    return intrinsics::tanh(x);
}

/// @brief Returns the hyperbolic cotangent of the x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> coth(const T1& x)
{
    return intrinsics::coth(x);
}

/// @brief Returns the hyperbolic sine of the even elements of the x and the hyperbolic cosine of the odd
/// elements of the x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> sinhcosh(const T1& x)
{
    return intrinsics::sinhcosh(x);
}

/// @brief Returns the hyperbolic cosine of the even elements of the x and the hyperbolic sine of the odd
/// elements of the x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> coshsinh(const T1& x)
{
    return intrinsics::coshsinh(x);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
