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

#include "../../simd/abs.hpp"
#include "../../simd/constants.hpp"
#include "../../simd/impl/function.hpp"
#include "../../simd/min_max.hpp"
#include "../../simd/operators.hpp"
#include "../../simd/select.hpp"
#include "../log_exp.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

namespace intr
{

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> sinh_small(const vec<T, N>& x)
{
    const auto x2 = sqr(x);
    return x * (T(1) + x2 * (T(1.0 / 6) + x2 * (T(1.0 / 120) + x2 * T(1.0 / 5040))));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> cosh_small(const vec<T, N>& x)
{
    const auto x2 = sqr(x);
    return T(1) + x2 * (T(0.5) + x2 * (T(1.0 / 24) + x2 * (T(1.0 / 720) + x2 * T(1.0 / 40320))));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> sinh(const vec<T, N>& x)
{
    const vec<Tout, N> xx = static_cast<vec<Tout, N>>(x);
    const auto small      = sinh_small(xx);
    return select(abs(xx) < Tout(0.05), small, (exp(xx) - exp(-xx)) * Tout(0.5));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> cosh(const vec<T, N>& x)
{
    const vec<Tout, N> xx = static_cast<vec<Tout, N>>(x);
    return select(abs(xx) < Tout(0.05), cosh_small(xx), (exp(xx) + exp(-xx)) * Tout(0.5));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> tanh(const vec<T, N>& x)
{
    const vec<Tout, N> xx = static_cast<vec<Tout, N>>(x);
    // Both ratios round to ±1 beyond this bound, before exp can overflow.
    const auto bounded = select(abs(xx) > Tout(20), copysign(vec<Tout, N>(Tout(20)), xx), xx);
    const auto a       = exp(Tout(2) * bounded);
    const auto small   = abs(xx) < Tout(0.05);
    const auto t       = sinh_small(xx) / cosh_small(xx);
    return select(small, t, (a - Tout(1)) / (a + Tout(1)));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> coth(const vec<T, N>& x)
{
    const vec<Tout, N> xx = static_cast<vec<Tout, N>>(x);
    const auto bounded    = select(abs(xx) > Tout(20), copysign(vec<Tout, N>(Tout(20)), xx), xx);
    const auto a          = exp(Tout(2) * bounded);
    return select(abs(xx) < Tout(0.05), cosh_small(xx) / sinh_small(xx), (a + Tout(1)) / (a - Tout(1)));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> sinhcosh(const vec<T, N>& x)
{
    const vec<Tout, N> xx = static_cast<vec<Tout, N>>(x);
    const auto small      = select(internal::evenmask<Tout, N>(), sinh_small(xx), cosh_small(xx));
    return select(abs(xx) < Tout(0.05), small, subadd(exp(xx), exp(-xx)) * Tout(0.5));
}

template <typename T, size_t N, typename Tout = flt_type<T>>
KFR_INTRINSIC vec<Tout, N> coshsinh(const vec<T, N>& x)
{
    const vec<Tout, N> xx = static_cast<vec<Tout, N>>(x);
    const auto small      = select(internal::oddmask<Tout, N>(), sinh_small(xx), cosh_small(xx));
    return select(abs(xx) < Tout(0.05), small, addsub(exp(xx), exp(-xx)) * Tout(0.5));
}

KFR_HANDLE_SCALAR_1_T(sinh, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(cosh, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(tanh, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(coth, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(sinhcosh, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(coshsinh, flt_type<T>)
} // namespace intr
KFR_I_FN(sinh)
KFR_I_FN(cosh)
KFR_I_FN(tanh)
KFR_I_FN(coth)
KFR_I_FN(sinhcosh)
KFR_I_FN(coshsinh)
} // namespace KFR_ARCH_NAME
} // namespace kfr
