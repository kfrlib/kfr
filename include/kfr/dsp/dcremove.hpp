/** @addtogroup biquad
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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
[[deprecated("Use dcremove(e, cutoff, fs) overload")]] KFR_INTRINSIC expression_iir<1, T, E1> dcremove(
    E1&& e1, double cutoff = 0.00025)
{
    const biquad_section<T> bqs[1] = { biquad_highpass(cutoff, 0.5) };
    return expression_iir<1, T, E1>(std::forward<E1>(e1), iir_state<T, 1>{ bqs });
}

/**
 * @brief Applies a DC removal filter to the given input expression.
 *
 * This function designs a 2nd order Butterworth high-pass filter to remove
 * the DC component from the input signal. The filter is implemented as a
 * single biquad section with a cutoff frequency specified by `cutoff_hz`.
 *
 * @tparam T The data type used for the filter coefficients.
 * @tparam E1 The type of the input expression.
 * @param e1 The input expression to be filtered.
 * @param cutoff_hz The cutoff frequency of the high-pass filter in Hz.
 * @param fs_hz The sampling frequency of the input signal in Hz.
 * @return An expression representing the filtered signal.
 */
template <typename E1, typename T = flt_type<expression_value_type<E1>>>
KFR_INTRINSIC expression_iir<1, T, E1> dcremove(E1&& e1, double cutoff_hz, double fs_hz)
{
    const biquad_section<T> bqs[1] = { biquad_highpass(cutoff_hz / fs_hz, 0.707) };
    return expression_iir<1, T, E1>(std::forward<E1>(e1), iir_state<T, 1>{ bqs });
}

} // namespace kfr
