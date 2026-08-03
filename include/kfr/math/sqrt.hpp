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

#include "impl/sqrt.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns the positive square root of x. \f$\sqrt{x}\f$
 *
 * For negative inputs the result is NaN. Applied element-wise for vector
 * inputs.
 *
 * @tparam T1 Input numeric type.
 * @param x Value whose square root is computed.
 * @return \f$\sqrt{x}\f$ as @c flt_type<T1>.
 */
template <numeric T1>
KFR_INTRINSIC flt_type<T1> sqrt(const T1& x)
{
    return intr::sqrt(x);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
