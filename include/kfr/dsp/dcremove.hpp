/** @addtogroup biquad
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

#include "biquad.hpp"
#include "biquad_design.hpp"

namespace kfr
{

template <typename E1, typename T = flt_type<expression_value_type<E1>>>
KFR_INTRINSIC expression_biquads<1, T, E1> dcremove(E1&& e1, double cutoff = 0.00025)
{
    const biquad_params<T> bqs[1] = { biquad_highpass(cutoff, 0.5) };
    return expression_biquads<1, T, E1>(bqs, std::forward<E1>(e1));
}

} // namespace kfr
