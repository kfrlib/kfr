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

#include "impl/hyperbolic.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns the hyperbolic sine of x.
 *
 * Computes (exp(x) - exp(-x)) / 2 element-wise.
 *
 * @param x Input value or SIMD vector of values.
 * @return Hyperbolic sine of x, with the floating-point type derived from the input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sinh(const T1& x)
{
    return intr::sinh(x);
}

/**
 * @brief Returns the hyperbolic cosine of x.
 *
 * Computes (exp(x) + exp(-x)) / 2 element-wise.
 *
 * @param x Input value or SIMD vector of values.
 * @return Hyperbolic cosine of x, with the floating-point type derived from the input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> cosh(const T1& x)
{
    return intr::cosh(x);
}

/**
 * @brief Returns the hyperbolic tangent of x.
 *
 * Computes (exp(2x) - 1) / (exp(2x) + 1) element-wise.
 *
 * @param x Input value or SIMD vector of values.
 * @return Hyperbolic tangent of x, with the floating-point type derived from the input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> tanh(const T1& x)
{
    return intr::tanh(x);
}

/**
 * @brief Returns the hyperbolic cotangent of x.
 *
 * Computes (exp(2x) + 1) / (exp(2x) - 1) element-wise. The result is undefined where x is 0.
 *
 * @param x Input value or SIMD vector of values (must be nonzero).
 * @return Hyperbolic cotangent of x, with the floating-point type derived from the input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> coth(const T1& x)
{
    return intr::coth(x);
}

/**
 * @brief Computes sinh and cosh of x in a single pass and interleaves the results.
 *
 * The result is a SIMD vector whose even lanes hold sinh of the corresponding input lane
 * and whose odd lanes hold cosh of the corresponding input lane. Useful for SIMD-friendly
 * evaluation of sinh/cosh pairs.
 *
 * @param x Input value or SIMD vector of values.
 * @return Vector with sinh at even lanes and cosh at odd lanes, of the floating-point type derived from the
 * input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> sinhcosh(const T1& x)
{
    return intr::sinhcosh(x);
}

/**
 * @brief Computes cosh and sinh of x in a single pass and interleaves the results.
 *
 * The result is a SIMD vector whose even lanes hold cosh of the corresponding input lane
 * and whose odd lanes hold sinh of the corresponding input lane. This is the lane-swapped
 * counterpart of sinhcosh.
 *
 * @param x Input value or SIMD vector of values.
 * @return Vector with cosh at even lanes and sinh at odd lanes, of the floating-point type derived from the
 * input.
 */
template <numeric T1>
KFR_FUNCTION flt_type<T1> coshsinh(const T1& x)
{
    return intr::coshsinh(x);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
