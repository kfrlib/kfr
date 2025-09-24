/** @addtogroup iir
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

#include "../base/filter.hpp"
#include "../base/handle.hpp"
#include "../base/reduce.hpp"
#include "../math/complex_math.hpp"
#include "../math/hyperbolic.hpp"
#include "../simd/complex.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/operators.hpp"
#include "../simd/vec.hpp"
#include "../test/assert.hpp"
#include "biquad_design.hpp"

namespace kfr
{

struct zpk
{
    univector<complex<double>> z;
    univector<complex<double>> p;
    double k;
};

inline namespace KFR_ARCH_NAME
{

KFR_FUNCTION zpk chebyshev1(int N, double rp);

KFR_FUNCTION zpk chebyshev2(int N, double rs);

#ifdef KFR_HAVE_ELLIPTIC
KFR_FUNCTION zpk elliptic(int N, double rp, double rs);
#endif

KFR_FUNCTION zpk butterworth(int N);

KFR_FUNCTION zpk bessel(int N);

namespace internal
{
KFR_FUNCTION zpk bilinear(const zpk& filter, double fs);

KFR_FUNCTION zpk lp2lp_zpk(const zpk& filter, double wo);

KFR_FUNCTION zpk lp2hp_zpk(const zpk& filter, double wo);

KFR_FUNCTION zpk lp2bp_zpk(const zpk& filter, double wo, double bw);

KFR_FUNCTION zpk lp2bs_zpk(const zpk& filter, double wo, double bw);

KFR_FUNCTION double warp_freq(double frequency, double fs);

} // namespace internal

/**
 * @brief Calculates zero-pole-gain coefficients for the low-pass IIR filter
 * @param filter Filter type: chebyshev1, chebyshev2, bessel, butterworth
 * @param frequency Cutoff frequency (Hz)
 * @param fs Sampling frequency (Hz)
 * @return The resulting zpk filter
 */
KFR_FUNCTION zpk iir_lowpass(const zpk& filter, double frequency, double fs = 2.0);

/**
 * @brief Calculates zero-pole-gain coefficients for the high-pass IIR filter
 * @param filter Filter type: chebyshev1, chebyshev2, bessel, butterworth
 * @param frequency Cutoff frequency (Hz)
 * @param fs Sampling frequency (Hz)
 * @return The resulting zpk filter
 */
KFR_FUNCTION zpk iir_highpass(const zpk& filter, double frequency, double fs = 2.0);

/**
 * @brief Calculates zero-pole-gain coefficients for the band-pass IIR filter
 * @param filter Filter type: chebyshev1, chebyshev2, bessel, butterworth
 * @param lowfreq Low cutoff frequency (Hz)
 * @param lowfreq High cutoff frequency (Hz)
 * @param fs Sampling frequency (Hz)
 * @return The resulting zpk filter
 */
KFR_FUNCTION zpk iir_bandpass(const zpk& filter, double lowfreq, double highfreq, double fs = 2.0);

/**
 * @brief Calculates zero-pole-gain coefficients for the band-stop IIR filter
 * @param filter Filter type: chebyshev1, chebyshev2, bessel, butterworth
 * @param lowfreq Low cutoff frequency (Hz)
 * @param lowfreq High cutoff frequency (Hz)
 * @param fs Sampling frequency (Hz)
 * @return The resulting zpk filter
 */
KFR_FUNCTION zpk iir_bandstop(const zpk& filter, double lowfreq, double highfreq, double fs = 2.0);

template <typename T = double>
KFR_FUNCTION iir_params<T> to_sos(const zpk& filter);

/**
 * @brief Returns template expressions that applies biquad filter to the input.
 * @param e1 Input expression
 * @param params IIR filter in ZPK form
 * @remark This overload converts ZPK to biquad coefficients using to_sos function at every call
 */
template <typename T = fbase, typename E1>
KFR_FUNCTION expression_handle<T, 1> iir(E1&& e1, const zpk& params)
{
    return iir(std::forward<E1>(e1), to_sos<T>(params));
}

} // namespace KFR_ARCH_NAME

} // namespace kfr
