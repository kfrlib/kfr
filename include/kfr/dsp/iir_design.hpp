/** @addtogroup iir
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

/**
 * @brief Zero-pole-gain (ZPK) representation of a linear time-invariant (LTI) filter.
 *
 * The transfer function is given by:
 * \f[ H(s) = k \frac{(s - z_0)(s - z_1)\dots}{(s - p_0)(s - p_1)\dots} \f]
 *
 * Compatible with the ZPK representation used in scipy.signal.
 */
struct zpk
{
    univector<complex<double>> z; ///< Zeros of the transfer function
    univector<complex<double>> p; ///< Poles of the transfer function
    double k; ///< System gain
};

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Analog Chebyshev Type I lowpass filter prototype (normalized cutoff = 1 rad/s).
 *
 * Returns the zeros, poles, and gain of an Nth-order analog Chebyshev Type I
 * lowpass filter with @p rp decibels of peak-to-peak passband ripple.
 *
 * The filter has no zeros and equiripple passband magnitude response.
 * DC gain is unity for odd-order filters and \f$-r_p\f$ dB for even-order filters.
 *
 * Compatible with scipy.signal.cheby1 with output='zpk' and analog=True.
 *
 * @param N Filter order (must be >= 0). Returns a gain-only filter if N == 0.
 * @param rp Maximum passband ripple in decibels (positive value).
 * @return ZPK representation of the analog lowpass filter prototype.
 */
KFR_FUNCTION zpk chebyshev1(int N, double rp);

/**
 * @brief Analog Chebyshev Type II lowpass filter prototype (normalized cutoff = 1 rad/s).
 *
 * Returns the zeros, poles, and gain of an Nth-order analog Chebyshev Type II
 * lowpass filter with @p rs decibels of stopband attenuation.
 *
 * Type II filters are the dual of Type I: they have ripple only in the
 * stopband and a monotonic (ripple-free) passband, with the same asymptotic
 * rolloff rate as Type I for a given order.
 *
 * Compatible with scipy.signal.cheby2 with output='zpk' and analog=True.
 *
 * @param N Filter order (must be >= 0). Returns a gain-only filter if N == 0.
 * @param rs Minimum stopband attenuation in decibels (positive value).
 * @return ZPK representation of the analog lowpass filter prototype.
 */
KFR_FUNCTION zpk chebyshev2(int N, double rs);

#ifdef KFR_HAVE_ELLIPTIC
/**
 * @brief Analog elliptic (Cauer) lowpass filter prototype (normalized cutoff = 1 rad/s).
 *
 * Returns the zeros, poles, and gain of an Nth-order analog elliptic (Cauer)
 * lowpass filter with @p rp decibels of passband ripple and @p rs decibels of
 * stopband attenuation.
 *
 * Elliptic filters offer the steepest rolloff for a given order, at the expense
 * of ripple in both passband and stopband.
 *
 * Requires Boost.Math (elliptic integrals) and the KFR_HAVE_ELLIPTIC define.
 *
 * Compatible with scipy.signal.ellip with output='zpk' and analog=True.
 *
 * @param N Filter order (must be >= 0). Returns a gain-only filter if N == 0.
 * @param rp Maximum passband ripple in decibels (positive value).
 * @param rs Minimum stopband attenuation in decibels (positive value).
 * @return ZPK representation of the analog lowpass filter prototype.
 */
KFR_FUNCTION zpk elliptic(int N, double rp, double rs);
#endif

/**
 * @brief Analog Butterworth lowpass filter prototype (normalized cutoff = 1 rad/s).
 *
 * Returns the zeros, poles, and gain of an Nth-order analog Butterworth
 * lowpass filter. The filter has no zeros and a maximally flat passband
 * magnitude response.
 *
 * Uses a hardcoded table for orders 1-24; returns a gain-only filter for
 * unsupported orders.
 *
 * Compatible with scipy.signal.butter with output='zpk' and analog=True.
 *
 * @param N Filter order (must be 0-24). Returns a gain-only filter if N == 0 or N > 24.
 * @return ZPK representation of the analog lowpass filter prototype.
 */
KFR_FUNCTION zpk butterworth(int N);

/**
 * @brief Analog Bessel/Thomson lowpass filter prototype (normalized cutoff = 1 rad/s).
 *
 * Returns the zeros, poles, and gain of an Nth-order analog Bessel filter.
 * Bessel filters have maximally flat group delay and linear phase response
 * in the passband, with minimal step-response ringing.
 *
 * Uses a hardcoded table for orders 1-24; returns a gain-only filter for
 * unsupported orders.
 *
 * Compatible with scipy.signal.bessel with output='zpk', analog=True, norm='phase'.
 *
 * @param N Filter order (must be 0-24). Returns a gain-only filter if N == 0 or N > 24.
 * @return ZPK representation of the analog lowpass filter prototype.
 */
KFR_FUNCTION zpk bessel(int N);

namespace internal
{
/**
 * @brief Bilinear (Tustin) transform from analog s-plane to digital z-plane.
 *
 * Substitutes \f$ s = 2 f_s \frac{z - 1}{z + 1} \f$ to convert an analog
 * filter to a digital filter. Pads zeros with -1 (z-plane zeros at Nyquist)
 * to match the number of poles.
 *
 * Compatible with scipy.signal.bilinear_zpk.
 *
 * @param filter Analog filter in ZPK form.
 * @param fs Sample rate used in the bilinear transform.
 * @return Digital filter in ZPK form.
 */
KFR_FUNCTION zpk bilinear(const zpk& filter, double fs);

/**
 * @brief Lowpass-to-lowpass analog frequency transformation.
 *
 * Scales cutoff from 1 rad/s to @p wo rad/s via \f$ s \rightarrow s / \omega_0 \f$.
 *
 * Compatible with scipy.signal.lp2lp_zpk.
 *
 * @param filter Analog lowpass filter prototype (normalized to 1 rad/s).
 * @param wo Desired cutoff frequency (rad/s).
 * @return Transformed lowpass filter in ZPK form.
 */
KFR_FUNCTION zpk lp2lp_zpk(const zpk& filter, double wo);

/**
 * @brief Lowpass-to-highpass analog frequency transformation.
 *
 * Converts a lowpass prototype to a highpass filter via \f$ s \rightarrow \omega_0 / s \f$.
 *
 * Compatible with scipy.signal.lp2hp_zpk.
 *
 * @param filter Analog lowpass filter prototype (normalized to 1 rad/s).
 * @param wo Desired cutoff frequency (rad/s).
 * @return Transformed highpass filter in ZPK form.
 */
KFR_FUNCTION zpk lp2hp_zpk(const zpk& filter, double wo);

/**
 * @brief Lowpass-to-bandpass analog frequency transformation.
 *
 * Converts a lowpass prototype to a bandpass filter, placing zeros at the
 * origin for the order difference.
 *
 * Compatible with scipy.signal.lp2bp_zpk.
 *
 * @param filter Analog lowpass filter prototype (normalized to 1 rad/s).
 * @param wo Center frequency (rad/s), typically \f$ \sqrt{\omega_{low} \cdot \omega_{high}} \f$.
 * @param bw Bandwidth (rad/s), typically \f$ \omega_{high} - \omega_{low} \f$.
 * @return Transformed bandpass filter in ZPK form.
 */
KFR_FUNCTION zpk lp2bp_zpk(const zpk& filter, double wo, double bw);

/**
 * @brief Lowpass-to-bandstop analog frequency transformation.
 *
 * Converts a lowpass prototype to a bandstop (notch) filter, placing zeros at
 * \f$ \pm j \omega_0 \f$ for the order difference.
 *
 * Compatible with scipy.signal.lp2bs_zpk.
 *
 * @param filter Analog lowpass filter prototype (normalized to 1 rad/s).
 * @param wo Center frequency (rad/s), typically \f$ \sqrt{\omega_{low} \cdot \omega_{high}} \f$.
 * @param bw Bandwidth (rad/s), typically \f$ \omega_{high} - \omega_{low} \f$.
 * @return Transformed bandstop filter in ZPK form.
 */
KFR_FUNCTION zpk lp2bs_zpk(const zpk& filter, double wo, double bw);

/**
 * @brief Frequency pre-warping for the bilinear transform.
 *
 * Applies frequency pre-warping for the bilinear transform.
 *
 * The sampling frequency is normalized to 2 Hz internally (so that the
 * subsequent bilinear transform can use fs = 2), yielding:
 * \f[ \omega_{warped} = 4 \tan\left(\pi \frac{frequency}{f_s}\right) \f]
 *
 * This is equivalent in the overall design pipeline to the standard
 * pre-warping \f$ 2 f_s \tan(\pi f / f_s) \f$ followed by a bilinear
 * transform with the same \f$ f_s \f$, since both stages use the
 * normalized fs = 2 here.
 *
 * @param frequency Desired digital cutoff frequency in Hz.
 * @param fs Sampling frequency in Hz.
 * @return Pre-warped analog frequency in rad/s.
 */
KFR_FUNCTION double warp_freq(double frequency, double fs);

} // namespace internal

/**
 * @brief Designs a digital lowpass IIR filter and returns ZPK coefficients.
 *
 * Applies frequency pre-warping, lowpass-to-lowpass analog transformation,
 * and bilinear transform to convert an analog lowpass prototype to a
 * digital lowpass filter.
 *
 * Usage: iir_lowpass(butterworth(4), 1000, 48000) designs a 4th-order
 * Butterworth lowpass at 1 kHz with 48 kHz sample rate.
 *
 * @param filter Analog lowpass prototype (butterworth, chebyshev1, chebyshev2, bessel, or elliptic).
 * @param frequency Cutoff frequency in Hz. If fs is omitted (left at its default of 2.0),
 *                 frequency is interpreted as normalized to Nyquist, so the valid range is 0..1.
 * @param fs Sampling frequency in Hz (default 2.0, i.e. Nyquist = 1 Hz).
 * @return Digital lowpass filter in ZPK form.
 */
KFR_FUNCTION zpk iir_lowpass(const zpk& filter, double frequency, double fs = 2.0);

/**
 * @brief Designs a digital highpass IIR filter and returns ZPK coefficients.
 *
 * Applies frequency pre-warping, lowpass-to-highpass analog transformation,
 * and bilinear transform to convert an analog lowpass prototype to a
 * digital highpass filter.
 *
 * @param filter Analog lowpass prototype (butterworth, chebyshev1, chebyshev2, bessel, or elliptic).
 * @param frequency Cutoff frequency in Hz. If fs is omitted (left at its default of 2.0),
 *                 frequency is interpreted as normalized to Nyquist, so the valid range is 0..1.
 * @param fs Sampling frequency in Hz (default 2.0, i.e. Nyquist = 1 Hz).
 * @return Digital highpass filter in ZPK form.
 */
KFR_FUNCTION zpk iir_highpass(const zpk& filter, double frequency, double fs = 2.0);

/**
 * @brief Designs a digital bandpass IIR filter and returns ZPK coefficients.
 *
 * Applies frequency pre-warping, lowpass-to-bandpass analog transformation
 * (with center frequency \f$ \sqrt{\omega_{low} \cdot \omega_{high}} \f$ and
 * bandwidth \f$ \omega_{high} - \omega_{low} \f$), and bilinear transform.
 *
 * @param filter Analog lowpass prototype (butterworth, chebyshev1, chebyshev2, bessel, or elliptic).
 * @param lowfreq Lower cutoff frequency in Hz. If fs is omitted (left at its default of 2.0),
 *               frequencies are interpreted as normalized to Nyquist, so the valid range is 0..1.
 * @param highfreq Upper cutoff frequency in Hz. See note on lowfreq.
 * @param fs Sampling frequency in Hz (default 2.0, i.e. Nyquist = 1 Hz).
 * @return Digital bandpass filter in ZPK form.
 */
KFR_FUNCTION zpk iir_bandpass(const zpk& filter, double lowfreq, double highfreq, double fs = 2.0);

/**
 * @brief Designs a digital bandstop (notch) IIR filter and returns ZPK coefficients.
 *
 * Applies frequency pre-warping, lowpass-to-bandstop analog transformation,
 * and bilinear transform.
 *
 * @param filter Analog lowpass prototype (butterworth, chebyshev1, chebyshev2, bessel, or elliptic).
 * @param lowfreq Lower cutoff frequency in Hz. If fs is omitted (left at its default of 2.0),
 *               frequencies are interpreted as normalized to Nyquist, so the valid range is 0..1.
 * @param highfreq Upper cutoff frequency in Hz. See note on lowfreq.
 * @param fs Sampling frequency in Hz (default 2.0, i.e. Nyquist = 1 Hz).
 * @return Digital bandstop filter in ZPK form.
 */
KFR_FUNCTION zpk iir_bandstop(const zpk& filter, double lowfreq, double highfreq, double fs = 2.0);

/**
 * @brief Converts a zero-pole-gain filter to second-order sections (SOS) form.
 *
 * Pairs poles and zeros into real-valued biquad sections using a
 * pole-matching algorithm that pairs each pole with its nearest zero.
 * Real poles are matched with real zeros; complex poles are paired with
 * their conjugates and matched with complex zero pairs.
 *
 * Compatible with scipy.signal.zpk2sos.
 *
 * @tparam T Floating-point type for the coefficients (default double).
 * @param filter Filter in ZPK form.
 * @return IIR parameters as a sequence of biquad sections.
 */
template <typename T = double>
KFR_FUNCTION iir_params<T> to_sos(const zpk& filter);

/**
 * @brief Applies an IIR filter (in ZPK form) to an input signal expression.
 *
 * This overload internally converts the @p params from ZPK to SOS using
 * to_sos() on every call, then delegates to the SOS-based iir().
 * For repeated usage, pre-convert with to_sos() and use the SOS overload.
 *
 * @tparam T Element type of the filter coefficients (default fbase).
 * @tparam E1 Input expression type.
 * @param e1 Input signal expression.
 * @param params IIR filter coefficients in ZPK form.
 * @return Expression handle applying the biquad filter.
 */
template <typename T = fbase, typename E1>
KFR_FUNCTION expression_handle<T, 1> iir(E1&& e1, const zpk& params)
{
    return iir(std::forward<E1>(e1), to_sos<T>(params));
}

} // namespace KFR_ARCH_NAME

} // namespace kfr
