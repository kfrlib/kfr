/** @addtogroup interpolation
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

#include "../simd/select.hpp"
#include "sin_cos.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, typename M>
KFR_FUNCTION T nearest(M mu, T x1, T x2)
{
    return select(mu < M(0.5), x1, x2);
}

template <typename T, typename M>
KFR_FUNCTION T linear(M mu, T x1, T x2)
{
    return mix(mu, x1, x2);
}

template <typename T, typename M>
KFR_FUNCTION T cosine(M mu, T x1, T x2)
{
    return mix((M(1) - fastcos(mu * c_pi<T>)) * M(0.5), x1, x2);
}

template <typename T, typename M>
KFR_FUNCTION T cubic(M mu, T x0, T x1, T x2, T x3)
{
    const T a0 = x3 - x2 - x0 + x1;
    const T a1 = x0 - x1 - a0;
    const T a2 = x2 - x0;
    const T a3 = x1;
    return horner(mu, a0, a1, a2, a3);
}

template <typename T, typename M>
KFR_FUNCTION T catmullrom(M mu, T x0, T x1, T x2, T x3)
{
    const T a0 = T(0.5) * (x3 - x0) - T(1.5) * (x2 - x1);
    const T a1 = x0 - T(2.5) * x1 + T(2) * x2 - T(0.5) * x3;
    const T a2 = T(0.5) * (x2 - x0);
    const T a3 = x1;
    return horner(mu, a0, a1, a2, a3);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
