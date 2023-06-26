/** @addtogroup fir
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

#include "../math/sin_cos.hpp"
#include "fir.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace internal
{
template <typename T>
void fir_lowpass(univector_ref<T> taps, T cutoff, const expression_handle<T>& window, bool normalize = true)
{
    const T scale = 2.0 * cutoff;
    taps = bind_expression(fn::sinc(), symmlinspace<T>((taps.size() - 1) * cutoff * c_pi<T>, taps.size())) *
           scale * window;

    if (is_odd(taps.size()))
        taps[taps.size() / 2] = scale;

    if (normalize)
    {
        const T invsum = reciprocal(sum(taps));
        taps           = taps * invsum;
    }
}
template <typename T>
void fir_highpass(univector_ref<T> taps, T cutoff, const expression_handle<T>& window, bool normalize = true)
{
    const T scale = 2.0 * -cutoff;
    taps = bind_expression(fn::sinc(), symmlinspace<T>((taps.size() - 1) * cutoff * c_pi<T>, taps.size())) *
           scale * window;

    if (is_odd(taps.size()))
        taps[taps.size() / 2] = 1 - 2.0 * cutoff;

    if (normalize)
    {
        const T invsum = reciprocal(sum(taps) + 1);
        taps           = taps * invsum;
    }
}

template <typename T>
void fir_bandpass(univector_ref<T> taps, T frequency1, T frequency2, const expression_handle<T>& window,
                  bool normalize = true)
{
    const T scale1 = 2.0 * frequency1;
    const T scale2 = 2.0 * frequency2;
    const T sc     = c_pi<T> * T(taps.size() - 1);
    const T start1 = sc * frequency1;
    const T start2 = sc * frequency2;

    taps = (bind_expression(fn::sinc(), symmlinspace<T>(start2, taps.size())) * scale2 -
            bind_expression(fn::sinc(), symmlinspace<T>(start1, taps.size())) * scale1) *
           window;

    if (is_odd(taps.size()))
        taps[taps.size() / 2] = 2 * (frequency2 - frequency1);

    if (normalize)
    {
        const T invsum = reciprocal(sum(taps) + 1);
        taps           = taps * invsum;
    }
}

template <typename T>
void fir_bandstop(univector_ref<T> taps, T frequency1, T frequency2, const expression_handle<T>& window,
                  bool normalize = true)
{
    const T scale1 = 2.0 * frequency1;
    const T scale2 = 2.0 * frequency2;
    const T sc     = c_pi<T> * T(taps.size() - 1);
    const T start1 = sc * frequency1;
    const T start2 = sc * frequency2;

    taps = (bind_expression(fn::sinc(), symmlinspace<T>(start1, taps.size())) * scale1 -
            bind_expression(fn::sinc(), symmlinspace<T>(start2, taps.size())) * scale2) *
           window;

    if (is_odd(taps.size()))
        taps[taps.size() / 2] = 1 - 2 * (frequency2 - frequency1);

    if (normalize)
    {
        const T invsum = reciprocal(sum(taps));
        taps           = taps * invsum;
    }
}
} // namespace internal
KFR_I_FN_FULL(fir_lowpass, internal::fir_lowpass)
KFR_I_FN_FULL(fir_highpass, internal::fir_highpass)
KFR_I_FN_FULL(fir_bandpass, internal::fir_bandpass)
KFR_I_FN_FULL(fir_bandstop, internal::fir_bandstop)

/**
 * @brief Calculates coefficients for the low-pass FIR filter
 * @param taps array where computed coefficients are stored
 * @param cutoff Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param window pointer to a window function
 * @param normalize true for normalized coefficients
 */
template <typename T, univector_tag Tag>
KFR_INTRINSIC void fir_lowpass(univector<T, Tag>& taps, identity<T> cutoff,
                               const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_lowpass(taps.slice(), cutoff, window, normalize);
}

/**
 * @brief Calculates coefficients for the high-pass FIR filter
 * @param taps array where computed coefficients are stored
 * @param cutoff Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param window pointer to a window function
 * @param normalize true for normalized coefficients
 */
template <typename T, univector_tag Tag>
KFR_INTRINSIC void fir_highpass(univector<T, Tag>& taps, identity<T> cutoff,
                                const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_highpass(taps.slice(), cutoff, window, normalize);
}

/**
 * @brief Calculates coefficients for the band-pass FIR filter
 * @param taps array where computed coefficients are stored
 * @param frequency1 Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param frequency2 Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param window pointer to a window function
 * @param normalize true for normalized coefficients
 */
template <typename T, univector_tag Tag>
KFR_INTRINSIC void fir_bandpass(univector<T, Tag>& taps, identity<T> frequency1, identity<T> frequency2,
                                const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_bandpass(taps.slice(), frequency1, frequency2, window, normalize);
}

/**
 * @brief Calculates coefficients for the band-stop FIR filter
 * @param taps array where computed coefficients are stored
 * @param frequency1 Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param frequency2 Normalized frequency (frequency_Hz / samplerate_Hz)
 * @param window pointer to a window function
 * @param normalize true for normalized coefficients
 */
template <typename T, univector_tag Tag>
KFR_INTRINSIC void fir_bandstop(univector<T, Tag>& taps, identity<T> frequency1, identity<T> frequency2,
                                const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_bandstop(taps.slice(), frequency1, frequency2, window, normalize);
}

/**
 * @copydoc kfr::fir_lowpass
 */
template <typename T>
KFR_INTRINSIC void fir_lowpass(const univector_ref<T>& taps, identity<T> cutoff,
                               const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_lowpass(taps, cutoff, window, normalize);
}

/**
 * @copydoc kfr::fir_highpass
 */
template <typename T>
KFR_INTRINSIC void fir_highpass(const univector_ref<T>& taps, identity<T> cutoff,
                                const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_highpass(taps, cutoff, window, normalize);
}

/**
 * @copydoc kfr::fir_bandpass
 */
template <typename T>
KFR_INTRINSIC void fir_bandpass(const univector_ref<T>& taps, identity<T> frequency1, identity<T> frequency2,
                                const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_bandpass(taps, frequency1, frequency2, window, normalize);
}

/**
 * @copydoc kfr::fir_bandstop
 */
template <typename T>
KFR_INTRINSIC void fir_bandstop(const univector_ref<T>& taps, identity<T> frequency1, identity<T> frequency2,
                                const expression_handle<T>& window, bool normalize = true)
{
    return internal::fir_bandstop(taps, frequency1, frequency2, window, normalize);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
