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

#include "../simd/select.hpp"
#include "sin_cos.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Nearest-neighbor interpolation.
 *
 * Returns @p x1 when @p mu is below 0.5, otherwise returns @p x2. This is the
 * simplest and cheapest interpolation scheme: it produces a step (hold) output
 * with no smoothing between samples.
 *
 * @tparam T sample (value) type, also the return type.
 * @tparam M interpolation-parameter type used for the comparison threshold.
 * @param mu interpolation parameter in the range [0, 1].
 * @param x1 value returned for @p mu < 0.5.
 * @param x2 value returned for @p mu >= 0.5.
 * @return @p x1 or @p x2 depending on the @p mu threshold.
 */
template <typename T, typename M>
KFR_FUNCTION T nearest(M mu, T x1, T x2)
{
    return select(mu < M(0.5), x1, x2);
}

/**
 * @brief Linear interpolation.
 *
 * Linearly blends between @p x1 and @p x2 by @p mu: the result is
 * (1 - mu) * x1 + mu * x2, i.e. @p x1 at @p mu == 0 and @p x2 at @p mu == 1.
 *
 * @tparam T sample (value) type, also the return type.
 * @tparam M interpolation-parameter type.
 * @param mu interpolation parameter; values outside [0, 1] extrapolate.
 * @param x1 value at @p mu == 0.
 * @param x2 value at @p mu == 1.
 * @return the linearly interpolated value between @p x1 and @p x2.
 */
template <typename T, typename M>
KFR_FUNCTION T linear(M mu, T x1, T x2)
{
    return mix(mu, x1, x2);
}

/**
 * @brief Cosine interpolation.
 *
 * Smoothly interpolates between @p x1 and @p x2 using a raised-cosine shape:
 * the effective blend factor is (1 - cos(mu * pi)) / 2, which is 0 at
 * @p mu == 0, 1 at @p mu == 1, and has zero first derivative at both ends.
 * Useful as a low-pass / smooth-step alternative to linear interpolation.
 *
 * @tparam T sample (value) type, also the return type.
 * @tparam M interpolation-parameter type.
 * @param mu interpolation parameter in the range [0, 1].
 * @param x1 value at @p mu == 0.
 * @param x2 value at @p mu == 1.
 * @return the cosine-interpolated value between @p x1 and @p x2.
 */
template <typename T, typename M>
KFR_FUNCTION T cosine(M mu, T x1, T x2)
{
    return mix((M(1) - fastcos(mu * c_pi<T>)) * M(0.5), x1, x2);
}

/**
 * @brief Cubic interpolation.
 *
 * Fits a cubic polynomial through the four control points @p x0, @p x1,
 * @p x2, @p x3 and evaluates it at @p mu. The curve passes through @p x1
 * at @p mu == 0 and through @p x2 at @p mu == 1, with the outer points
 * @p x0 and @p x3 controlling the tangent at the endpoints.
 *
 * @tparam T sample (value) type, also the return type.
 * @tparam M interpolation-parameter type.
 * @param mu interpolation parameter in the range [0, 1].
 * @param x0 control point preceding @p x1 (used to shape the start tangent).
 * @param x1 value at @p mu == 0.
 * @param x2 value at @p mu == 1.
 * @param x3 control point following @p x2 (used to shape the end tangent).
 * @return the cubic-interpolated value at @p mu.
 */
template <typename T, typename M>
KFR_FUNCTION T cubic(M mu, T x0, T x1, T x2, T x3)
{
    const T a0 = x3 - x2 - x0 + x1;
    const T a1 = x0 - x1 - a0;
    const T a2 = x2 - x0;
    const T a3 = x1;
    return horner(mu, a0, a1, a2, a3);
}

/**
 * @brief Catmull-Rom spline interpolation.
 *
 * Interpolating Catmull-Rom spline through the four control points @p x0,
 * @p x1, @p x2, @p x3 evaluated at @p mu. The curve passes through @p x1
 * at @p mu == 0 and through @p x2 at @p mu == 1, and the tangents at those
 * points are estimated as (x2 - x0) / 2 and (x3 - x1) / 2 respectively,
 * giving C^1 continuity when concatenated across a sample stream.
 *
 * @tparam T sample (value) type, also the return type.
 * @tparam M interpolation-parameter type.
 * @param mu interpolation parameter in the range [0, 1].
 * @param x0 control point preceding @p x1.
 * @param x1 value at @p mu == 0.
 * @param x2 value at @p mu == 1.
 * @param x3 control point following @p x2.
 * @return the Catmull-Rom-interpolated value at @p mu.
 */
template <typename T, typename M>
KFR_FUNCTION T catmullrom(M mu, T x0, T x1, T x2, T x3)
{
    const T a0 = T(0.5) * (x3 - x0) - T(1.5) * (x2 - x1);
    const T a1 = x0 - T(2.5) * x1 + T(2) * x2 - T(0.5) * x3;
    const T a2 = T(0.5) * (x2 - x0);
    const T a3 = x1;
    return horner(mu, a0, a1, a2, a3);
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
