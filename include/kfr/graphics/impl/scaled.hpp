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

#include "../../cometa/string.hpp"
#include "../../math.hpp"

namespace kfr
{

template <typename Tout, int Mout, int Min, typename Tin, size_t N,
          KFR_ENABLE_IF(Mout != Min &&
                        (std::is_floating_point<Tin>::value || std::is_floating_point<Tout>::value))>
KFR_INTRINSIC vec<Tout, N> convert_scaled(const vec<Tin, N>& value)
{
    using Tcommon = std::common_type_t<Tin, Tout>;
    return static_cast<vec<Tout, N>>(static_cast<vec<Tcommon, N>>(value) * Mout / Min);
}

template <typename Tout, int Mout, int Min, typename Tin, size_t N,
          KFR_ENABLE_IF(Mout != Min &&
                        !(std::is_floating_point<Tin>::value || std::is_floating_point<Tout>::value))>
KFR_INTRINSIC vec<Tout, N> convert_scaled(const vec<Tin, N>& value)
{
    using Tcommon =
        findinttype<std::numeric_limits<Tin>::min() * Mout, std::numeric_limits<Tin>::max() * Mout>;
    return static_cast<vec<Tout, N>>(static_cast<vec<Tcommon, N>>(value) * Mout / Min);
}

template <typename Tout, int Mout, int Min, typename Tin, size_t N, KFR_ENABLE_IF(Mout == Min)>
KFR_INTRINSIC vec<Tout, N> convert_scaled(const vec<Tin, N>& value)
{
    return static_cast<vec<Tout, N>>(value);
}
} // namespace kfr
