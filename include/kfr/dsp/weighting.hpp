/** @addtogroup dsp
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../base/operators.hpp"
#include "../base/sqrt.hpp"

namespace kfr
{
namespace intrinsics
{

template <typename T>
KFR_SINTRIN T weight_a_unnorm(T f)
{
    const T f2  = pow2(f);
    const T nom = pow2(12200) * pow4(f);
    const T den = (f2 + pow2(20.6)) * (sqrt((f2 + pow2(107.7)) * (f2 + pow2(737.9)))) * (f2 + pow2(12200));
    return nom / den;
}

template <typename T>
constexpr static T weight_a_gain = reciprocal(weight_a_unnorm(T(1000.0)));

template <typename T>
KFR_SINTRIN T aweighting(T f)
{
    return weight_a_unnorm(f) * weight_a_gain<subtype<T>>;
}

template <typename T>
KFR_SINTRIN T weight_b_unnorm(T f)
{
    const T f2  = pow2(f);
    const T nom = pow2(12200) * pow3(f);
    const T den = (f2 + pow2(20.6)) * (sqrt((f2 + pow2(158.5)))) * (f2 + pow2(12200));

    return nom / den;
}

template <typename T>
constexpr static T weight_b_gain = reciprocal(weight_b_unnorm(T(1000.0)));

template <typename T>
KFR_SINTRIN T bweighting(T f)
{
    return weight_b_unnorm(f) * weight_b_gain<subtype<T>>;
}

template <typename T>
KFR_SINTRIN T weight_c_unnorm(T f)
{
    const T f2  = pow2(f);
    const T nom = pow2(12200) * f2;
    const T den = (f2 + pow2(20.6)) * (f2 + pow2(12200));

    return nom / den;
}

template <typename T>
constexpr static T weight_c_gain = reciprocal(weight_c_unnorm(T(1000.0)));

template <typename T>
KFR_SINTRIN T cweighting(T f)
{
    return weight_c_unnorm(f) * weight_c_gain<subtype<T>>;
}
}
KFR_I_FN(aweighting)
KFR_I_FN(bweighting)
KFR_I_FN(cweighting)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 aweighting(const T1& x)
{
    return intrinsics::aweighting(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::aweighting, E1> aweighting(E1&& x)
{
    return { fn::aweighting(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 bweighting(const T1& x)
{
    return intrinsics::bweighting(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::bweighting, E1> bweighting(E1&& x)
{
    return { fn::bweighting(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cweighting(const T1& x)
{
    return intrinsics::cweighting(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::cweighting, E1> cweighting(E1&& x)
{
    return { fn::cweighting(), std::forward<E1>(x) };
}
}
