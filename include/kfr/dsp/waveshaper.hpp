/** @addtogroup dsp_extra
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Hard-clipping waveshaper.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * @param input Input signal expression.
 * @param clip_level Symmetric clipping threshold; samples outside [-clip_level, +clip_level] are clamped.
 * @return Clamped signal expression.
 */
template <typename E1>
inline auto waveshaper_hardclip(E1&& input, double clip_level)
{
    return clamp(input, -clip_level, +clip_level);
}

/**
 * @brief Hyperbolic-tangent waveshaper.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * The result is normalized by `tanh(saturation)` so that an input of 1 maps to 1.
 *
 * @param input Input signal expression.
 * @param saturation Drive amount; higher values increase the non-linearity.
 * @return Shaped signal expression.
 */
template <typename E1>
inline auto waveshaper_tanh(E1&& input, double saturation)
{
    return tanh(saturation * input) * (coth(saturation));
}

/**
 * @brief Type-I saturation curve (odd-symmetric, bounded to (-1, 1)).
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * @param x Input value.
 * @return Saturated value preserving the sign of @p x.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> saturate_I(const T1& x)
{
    const flt_type<T1> xx = -1 / (abs(static_cast<flt_type<T1>>(x)) + 1) + 1;
    return mulsign(xx, static_cast<flt_type<T1>>(x));
}
KFR_FN(saturate_I)

/**
 * @brief Type-II saturation curve (odd-symmetric, bounded to (-1, 1)).
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * @param x Input value.
 * @return Saturated value preserving the sign of @p x.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> saturate_II(const T1& x)
{
    const flt_type<T1> xx = sqr(abs(static_cast<flt_type<T1>>(x)) + 1);
    return mulsign((xx - 1) / (xx + 1), static_cast<flt_type<T1>>(x));
}
KFR_FN(saturate_II)

/**
 * @brief Expression form of saturate_I.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * @param x Input expression.
 * @return Expression yielding saturated values.
 */
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::saturate_II, E1> saturate_I(E1&& x)
{
    return { fn::saturate_I(), std::forward<E1>(x) };
}

/**
 * @brief Expression form of saturate_II.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * @param x Input expression.
 * @return Expression yielding saturated values.
 */
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::saturate_II, E1> saturate_II(E1&& x)
{
    return { fn::saturate_II(), std::forward<E1>(x) };
}

/**
 * @brief Waveshaper based on the type-I saturation curve.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * The output is normalized by `saturate_I(saturation)` so that an input of 1 maps to 1.
 *
 * @param input Input signal expression.
 * @param saturation Drive amount.
 * @return Shaped signal expression.
 */
template <typename E1>
inline auto waveshaper_saturate_I(E1&& input, double saturation)
{
    return saturate_I(saturation * input) / (saturate_I(saturation));
}

/**
 * @brief Waveshaper based on the type-II saturation curve.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * The output is normalized by `saturate_II(saturation)` so that an input of 1 maps to 1.
 *
 * @param input Input signal expression.
 * @param saturation Drive amount.
 * @return Shaped signal expression.
 */
template <typename E1>
inline auto waveshaper_saturate_II(E1&& input, double saturation)
{
    return saturate_II(saturation * input) / (saturate_II(saturation));
}

/**
 * @brief Polynomial (odd-only) waveshaper using Horner evaluation.
 * @note For demonstration only; musically pleasing saturation requires careful modelling.
 *
 * Evaluates `c1*x + c3*x^3 + c5*x^5 + ...` via horner_odd.
 *
 * @param input Input signal expression.
 * @param c1 Coefficient of the linear term.
 * @param c3 Coefficient of the cubic term.
 * @param cs Coefficients of the higher odd-order terms (x^5, x^7, ...).
 * @return Shaped signal expression.
 */
template <typename E1, typename... Cs>
inline auto waveshaper_poly(E1&& input, fbase c1, fbase c3, Cs... cs)
{
    return horner_odd(input, c1, c3, static_cast<fbase>(cs)...);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
