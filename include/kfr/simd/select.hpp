/** @addtogroup basic_math
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
          typename Tout = subtype<std::common_type_t<T2, T3>>>
KFR_INTRINSIC vec<Tout, N> select(const mask<T1, N>& m, const T2& x, const T3& y)
{
    return intrinsics::select(bitcast<Tout>(cast<itype<Tout>>(bitcast<itype<T1>>(m.asvec()))).asmask(),
                              broadcastto<Tout>(x), broadcastto<Tout>(y));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
