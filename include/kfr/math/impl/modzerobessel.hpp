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

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> modzerobessel(const vec<T, N>& x)
{
    constexpr static T bessel_coef[] = { T(0.25),
                                         T(0.027777777777777776236),
                                         T(0.0017361111111111110147),
                                         T(6.9444444444444444384e-005),
                                         T(1.9290123456790123911e-006),
                                         T(3.9367598891408417495e-008),
                                         T(6.1511873267825652335e-010),
                                         T(7.5940584281266239246e-012),
                                         T(7.5940584281266233693e-014),
                                         T(6.2760813455591932909e-016),
                                         T(4.3583898233049949985e-018),
                                         T(2.5789288895295827557e-020),
                                         T(1.3157800456783586208e-022),
                                         T(5.8479113141260384983e-025),
                                         T(2.2843403570804837884e-027),
                                         T(7.904291893012054025e-030),
                                         T(2.4395962632753252792e-032),
                                         T(6.75788438580422547e-035),
                                         T(1.689471096451056426e-037),
                                         T(3.8310002187098784929e-040),
                                         T(7.9152897080782616517e-043),
                                         T(1.4962740468957016443e-045),
                                         T(2.5976979980828152196e-048),
                                         T(4.1563167969325041577e-051),
                                         T(6.1483976285983795968e-054),
                                         T(8.434015951438105991e-057),
                                         T(1.0757673407446563809e-059),
                                         T(1.2791526049282476926e-062),
                                         T(1.4212806721424974034e-065),
                                         T(1.4789601166935457918e-068),
                                         T(1.4442969889585408123e-071),
                                         T(1.3262598613026086927e-074),
                                         T(1.1472836170437790782e-077),
                                         T(9.3655805472961564331e-081),
                                         T(7.2265282000741942594e-084),
                                         T(5.2786911614858977913e-087),
                                         T(3.6556032974279072401e-090),
                                         T(2.4034209713529963119e-093),
                                         T(1.5021381070956226783e-096) };

    const vec<T, N> x_2     = x * 0.5;
    const vec<T, N> x_2_sqr = x_2 * x_2;
    vec<T, N> num           = x_2_sqr;
    vec<T, N> result;
    result = 1 + x_2_sqr;

    CMT_LOOP_UNROLL
    for (size_t i = 0; i < (sizeof(T) == 4 ? 20 : 39); i++)
    {
        result = fmadd((num *= x_2_sqr), bessel_coef[i], result);
    }
    return result;
}

KFR_HANDLE_SCALAR(modzerobessel)
} // namespace intrinsics
KFR_I_FN(modzerobessel)
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
