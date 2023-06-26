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
#include "../../math/log_exp.hpp"
#include "../../simd/impl/function.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wc99-extensions")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wc99-extensions")
#endif

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{
template <typename T>
constexpr T gamma_precalc[] = {
    0x2.81b263fec4e08p+0,  0x3.07b4100e04448p+16, -0xa.a0da01d4d4e2p+16, 0xf.05ccb27bb9dbp+16,
    -0xa.fa79616b7c6ep+16, 0x4.6dd6c10d4df5p+16,  -0xf.a2304199eb4ap+12, 0x1.c21dd4aade3dp+12,
    -0x1.62f981f01cf84p+8, 0x5.a937aa5c48d98p+0,  -0x3.c640bf82e2104p-8, 0xc.914c540f959cp-24,
};

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> gamma(const vec<T, N>& z)
{
    constexpr size_t Count = arraysize(gamma_precalc<T>);
    vec<T, N> accm         = gamma_precalc<T>[0];
    CMT_LOOP_UNROLL
    for (size_t k = 1; k < Count; k++)
        accm += gamma_precalc<T>[k] / (z + broadcastto<utype<T>>(k));
    accm *= exp(-(z + Count)) * pow(z + Count, z + 0.5);
    return accm / z;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> factorial_approx(const vec<T, N>& x)
{
    return gamma(x + T(1));
}
KFR_HANDLE_SCALAR(gamma)
KFR_HANDLE_SCALAR(factorial_approx)
} // namespace intrinsics
KFR_I_FN(gamma)
KFR_I_FN(factorial_approx)
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
