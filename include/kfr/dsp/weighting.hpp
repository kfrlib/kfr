/** @addtogroup dsp_extra
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

#include "../base/expression.hpp"
#include "../math/sqrt.hpp"
#include "../simd/operators.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T>
KFR_INTRINSIC T weight_a_unnorm(T f)
{
    const T f2  = pow2(f);
    const T nom = pow2(12200) * pow4(f);
    const T den = (f2 + pow2(20.6)) * (sqrt((f2 + pow2(107.7)) * (f2 + pow2(737.9)))) * (f2 + pow2(12200));
    return nom / den;
}

template <typename T>
constexpr inline T weight_a_gain = T(1.25889662908332766733);

template <typename T>
KFR_INTRINSIC T aweighting(T f)
{
    return weight_a_unnorm(f) * weight_a_gain<subtype<T>>;
}

template <typename T>
KFR_INTRINSIC T weight_b_unnorm(T f)
{
    const T f2  = pow2(f);
    const T nom = pow2(12200) * pow3(f);
    const T den = (f2 + pow2(20.6)) * (sqrt((f2 + pow2(158.5)))) * (f2 + pow2(12200));

    return nom / den;
}

template <typename T>
constexpr inline T weight_b_gain = T(1.01971824783723263863);

template <typename T>
KFR_INTRINSIC T bweighting(T f)
{
    return weight_b_unnorm(f) * weight_b_gain<subtype<T>>;
}

template <typename T>
KFR_INTRINSIC T weight_c_unnorm(T f)
{
    const T f2  = pow2(f);
    const T nom = pow2(12200) * f2;
    const T den = (f2 + pow2(20.6)) * (f2 + pow2(12200));

    return nom / den;
}

template <typename T>
constexpr inline T weight_c_gain = T(1.00714583514109112805);

template <typename T>
KFR_INTRINSIC T cweighting(T f)
{
    return weight_c_unnorm(f) * weight_c_gain<subtype<T>>;
}
} // namespace intrinsics
KFR_I_FN(aweighting)
KFR_I_FN(bweighting)
KFR_I_FN(cweighting)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 aweighting(const T1& x)
{
    return intrinsics::aweighting(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::aweighting, E1> aweighting(E1&& x)
{
    return { fn::aweighting(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 bweighting(const T1& x)
{
    return intrinsics::bweighting(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::bweighting, E1> bweighting(E1&& x)
{
    return { fn::bweighting(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 cweighting(const T1& x)
{
    return intrinsics::cweighting(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_INTRINSIC expression_function<fn::cweighting, E1> cweighting(E1&& x)
{
    return { fn::cweighting(), std::forward<E1>(x) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
