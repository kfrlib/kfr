/** @addtogroup other_math
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

#include "impl/gamma.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/// @brief Returns the approximate gamma function of an argument
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> gamma(const T1& x)
{
    return intrinsics::gamma(x);
}

/// @brief Returns the approximate factorial of an argument
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> factorial_approx(const T1& x)
{
    return intrinsics::factorial_approx(x);
}

constexpr inline uint64_t factorial_table[21] = {
    0,
    1,
    2,
    6,
    24,
    120,
    720,
    5040,
    40320,
    362880,
    3628800,
    39916800,
    479001600,
    6227020800,
    87178291200,
    1307674368000,
    20922789888000,
    355687428096000,
    6402373705728000,
    121645100408832000,
    2432902008176640000,
};

/// @brief Returns the factorial of an argument. Returns max(uint64_t) if does not fit to uint64_t
constexpr uint64_t factorial(int n)
{
    if (CMT_LIKELY(n < 0 || n > 20))
        return std::numeric_limits<uint64_t>::max();
    return factorial_table[n];
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
