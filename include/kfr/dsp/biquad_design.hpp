/** @addtogroup biquad
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

#include "biquad.hpp"
#include <cmath>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/**
 * @brief Calculates coefficients for the all-pass biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param Q Q factor
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_allpass(identity<T> frequency, identity<T> Q)
{
    const T alpha = std::sin(frequency) / 2.0 * Q;
    const T cs    = std::cos(frequency);

    const T b0 = 1.0 / (1.0 + alpha);
    const T b1 = -2.0 * cs * b0;
    const T b2 = (1.0 - alpha) * b0;
    const T a0 = (1.0 - alpha) * b0;
    const T a1 = -2.0 * cs * b0;
    const T a2 = (1.0 + alpha) * b0;
    return { b0, b1, b2, a0, a1, a2 };
}

/**
 * @brief Calculates coefficients for the low-pass biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param Q Q factor
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_lowpass(identity<T> frequency, identity<T> Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = K2 * norm;
    const T a1   = 2 * a0;
    const T a2   = a0;
    const T b1   = 2 * (K2 - 1) * norm;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

/**
 * @brief Calculates coefficients for the high-pass biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param Q Q factor
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_highpass(identity<T> frequency, identity<T> Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = 1 * norm;
    const T a1   = -2 * a0;
    const T a2   = a0;
    const T b1   = 2 * (K2 - 1) * norm;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

/**
 * @brief Calculates coefficients for the band-pass biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param Q Q factor
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_bandpass(identity<T> frequency, identity<T> Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = K / Q * norm;
    const T a1   = 0;
    const T a2   = -a0;
    const T b1   = 2 * (K2 - 1) * norm;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

/**
 * @brief Calculates coefficients for the notch biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param Q Q factor
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_notch(identity<T> frequency, identity<T> Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = (1 + K2) * norm;
    const T a1   = 2 * (K2 - 1) * norm;
    const T a2   = a0;
    const T b1   = a1;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

/**
 * @brief Calculates coefficients for the peak biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param Q Q factor
 * @param gain Gain in dB
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_peak(identity<T> frequency, identity<T> Q, identity<T> gain)
{
    biquad_params<T> result;
    const T K  = std::tan(c_pi<T, 1> * frequency);
    const T K2 = K * K;
    const T V  = std::exp(std::abs(gain) * (1.0 / 20.0) * c_log_10<T>);

    if (gain >= 0)
    { // boost
        const T norm = 1 / (1 + 1 / Q * K + K2);
        const T a0   = (1 + V / Q * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - V / Q * K + K2) * norm;
        const T b1   = a1;
        const T b2   = (1 - 1 / Q * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    else
    { // cut
        const T norm = 1 / (1 + V / Q * K + K2);
        const T a0   = (1 + 1 / Q * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - 1 / Q * K + K2) * norm;
        const T b1   = a1;
        const T b2   = (1 - V / Q * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    return result;
}

/**
 * @brief Calculates coefficients for the low-shelf biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param gain Gain in dB
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_lowshelf(identity<T> frequency, identity<T> gain)
{
    biquad_params<T> result;
    const T K  = std::tan(c_pi<T, 1> * frequency);
    const T K2 = K * K;
    const T V  = std::exp(std::fabs(gain) * (1.0 / 20.0) * c_log_10<T>);

    if (gain >= 0)
    { // boost
        const T norm = 1 / (1 + c_sqrt_2<T> * K + K2);
        const T a0   = (1 + std::sqrt(2 * V) * K + V * K2) * norm;
        const T a1   = 2 * (V * K2 - 1) * norm;
        const T a2   = (1 - std::sqrt(2 * V) * K + V * K2) * norm;
        const T b1   = 2 * (K2 - 1) * norm;
        const T b2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    else
    { // cut
        const T norm = 1 / (1 + std::sqrt(2 * V) * K + V * K2);
        const T a0   = (1 + c_sqrt_2<T> * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        const T b1   = 2 * (V * K2 - 1) * norm;
        const T b2   = (1 - std::sqrt(2 * V) * K + V * K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    return result;
}

/**
 * @brief Calculates coefficients for the high-shelf biquad filter
 * @param frequency Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param gain Gain in dB
 * @return Biquad filter coefficients
 */
template <typename T = fbase>
KFR_FUNCTION biquad_params<T> biquad_highshelf(identity<T> frequency, identity<T> gain)
{
    biquad_params<T> result;
    const T K  = std::tan(c_pi<T, 1> * frequency);
    const T K2 = K * K;
    const T V  = std::exp(std::fabs(gain) * (1.0 / 20.0) * c_log_10<T>);

    if (gain >= 0)
    { // boost
        const T norm = 1 / (1 + c_sqrt_2<T> * K + K2);
        const T a0   = (V + std::sqrt(2 * V) * K + K2) * norm;
        const T a1   = 2 * (K2 - V) * norm;
        const T a2   = (V - std::sqrt(2 * V) * K + K2) * norm;
        const T b1   = 2 * (K2 - 1) * norm;
        const T b2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    else
    { // cut
        const T norm = 1 / (V + std::sqrt(2 * V) * K + K2);
        const T a0   = (1 + c_sqrt_2<T> * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        const T b1   = 2 * (K2 - V) * norm;
        const T b2   = (V - std::sqrt(2 * V) * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    return result;
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
