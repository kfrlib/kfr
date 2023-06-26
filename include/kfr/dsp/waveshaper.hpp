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
#include "../math/hyperbolic.hpp"
#include "../simd/clamp.hpp"
#include "../simd/operators.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename E1>
inline auto waveshaper_hardclip(E1&& input, double clip_level)
{
    return clamp(input, -clip_level, +clip_level);
}

template <typename E1>
inline auto waveshaper_tanh(E1&& input, double saturation)
{
    return tanh(saturation * input) * (coth(saturation));
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> saturate_I(const T1& x)
{
    const flt_type<T1> xx = -1 / (abs(static_cast<flt_type<T1>>(x)) + 1) + 1;
    return mulsign(xx, static_cast<flt_type<T1>>(x));
}
KFR_FN(saturate_I)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> saturate_II(const T1& x)
{
    const flt_type<T1> xx = sqr(abs(static_cast<flt_type<T1>>(x)) + 1);
    return mulsign((xx - 1) / (xx + 1), static_cast<flt_type<T1>>(x));
}
KFR_FN(saturate_II)

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::saturate_II, E1> saturate_I(E1&& x)
{
    return { fn::saturate_I(), std::forward<E1>(x) };
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::saturate_II, E1> saturate_II(E1&& x)
{
    return { fn::saturate_II(), std::forward<E1>(x) };
}

template <typename E1>
inline auto waveshaper_saturate_I(E1&& input, double saturation)
{
    return saturate_I(saturation * input) / (saturate_I(saturation));
}

template <typename E1>
inline auto waveshaper_saturate_II(E1&& input, double saturation)
{
    return saturate_II(saturation * input) / (saturate_II(saturation));
}

template <typename E1, typename... Cs>
inline auto waveshaper_poly(E1&& input, fbase c1, fbase c3, Cs... cs)
{
    return horner_odd(input, c1, c3, static_cast<fbase>(cs)...);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
