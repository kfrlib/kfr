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

#include "../../simd/abs.hpp"
#include "../../simd/constants.hpp"
#include "../../simd/impl/function.hpp"
#include "../../simd/min_max.hpp"
#include "../../simd/operators.hpp"
#include "../../simd/round.hpp"
#include "../../simd/select.hpp"
#include "../../simd/shuffle.hpp"

#if CMT_HAS_WARNING("-Wc99-extensions")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wc99-extensions")
#endif

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> trig_horner(const vec<T, N>&, const mask<T, N>& msk, const T& a0, const T& b0)
{
    return select(msk, a0, b0);
}

template <typename T, size_t N, typename... Ts>
KFR_INTRINSIC vec<T, N> trig_horner(const vec<T, N>& x, const mask<T, N>& msk, const T& a0, const T& b0,
                                    const T& a1, const T& b1, const Ts&... values)
{
    return fmadd(trig_horner(x, msk, a1, b1, values...), x, select(msk, a0, b0));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> trig_fold(const vec<T, N>& x, vec<itype<T>, N>& quadrant)
{
    const vec<T, N> xabs = abs(x);
    constexpr T div      = constants<T>::fold_constant_div;
    vec<T, N> y          = floor(xabs / div);
    quadrant             = broadcastto<itype<T>>(broadcastto<int>(y - floor(y * T(1.0 / 16.0)) * T(16.0)));

    const vec<itype<T>, N> odd = (quadrant & 1);
    quadrant                   = quadrant + odd;
    y                          = y + cast<T>(odd);
    quadrant                   = quadrant & 7;

    constexpr T hi   = constants<T>::fold_constant_hi;
    constexpr T rem1 = constants<T>::fold_constant_rem1;
    constexpr T rem2 = constants<T>::fold_constant_rem2;
    return (xabs - y * hi) - y * rem1 - y * rem2;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> fold_range(const vec<T, N>& x)
{
    vec<itype<T>, N> q;
    return trig_fold(x, q);
}

template <size_t N>
KFR_INTRINSIC vec<f32, N> trig_sincos(const vec<f32, N>& folded, const mask<f32, N>& cosmask)
{
    constexpr f32 sin_c2  = CMT_FP(-0x2.aaaaacp-4f, -1.6666667163e-01f);
    constexpr f32 sin_c4  = CMT_FP(0x2.222334p-8f, 8.3333970979e-03f);
    constexpr f32 sin_c6  = CMT_FP(-0xd.0566ep-16f, -1.9868623349e-04f);
    constexpr f32 sin_c8  = CMT_FP(0x3.64cc1cp-20f, 3.2365221614e-06f);
    constexpr f32 sin_c10 = CMT_FP(-0x5.6c4a4p-24f, -3.2323646337e-07f);
    constexpr f32 cos_c2  = CMT_FP(-0x8.p-4f, -5.0000000000e-01f);
    constexpr f32 cos_c4  = CMT_FP(0xa.aaaabp-8f, 4.1666667908e-02f);
    constexpr f32 cos_c6  = CMT_FP(-0x5.b05d48p-12f, -1.3888973044e-03f);
    constexpr f32 cos_c8  = CMT_FP(0x1.a065f8p-16f, 2.4819273676e-05f);
    constexpr f32 cos_c10 = CMT_FP(-0x4.cd156p-24f, -2.8616830150e-07f);

    const vec<f32, N> x2 = folded * folded;

    vec<f32, N> formula = trig_horner(x2, cosmask, 1.0f, 1.0f, cos_c2, sin_c2, cos_c4, sin_c4, cos_c6, sin_c6,
                                      cos_c8, sin_c8, cos_c10, sin_c10);

    formula = select(cosmask, formula, formula * folded);
    return formula;
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> trig_sincos(const vec<f64, N>& folded, const mask<f64, N>& cosmask)
{
    constexpr f64 sin_c2  = CMT_FP(-0x2.aaaaaaaaaaaaap-4, -1.666666666666666574e-01);
    constexpr f64 sin_c4  = CMT_FP(0x2.22222222220cep-8, 8.333333333333038315e-03);
    constexpr f64 sin_c6  = CMT_FP(-0xd.00d00cffd6618p-16, -1.984126984092335463e-04);
    constexpr f64 sin_c8  = CMT_FP(0x2.e3bc744fb879ep-20, 2.755731902164406591e-06);
    constexpr f64 sin_c10 = CMT_FP(-0x6.b99034c1467a4p-28, -2.505204327429436704e-08);
    constexpr f64 sin_c12 = CMT_FP(0xb.0711ea8fe8ee8p-36, 1.604729496525771112e-10);
    constexpr f64 sin_c14 = CMT_FP(-0xb.7e010897e55dp-44, -6.532561241665605726e-13);
    constexpr f64 sin_c16 = CMT_FP(-0xb.64eac07f1d6bp-48, -4.048035517573349688e-14);
    constexpr f64 cos_c2  = CMT_FP(-0x8.p-4, -5.000000000000000000e-01);
    constexpr f64 cos_c4  = CMT_FP(0xa.aaaaaaaaaaaa8p-8, 4.166666666666666435e-02);
    constexpr f64 cos_c6  = CMT_FP(-0x5.b05b05b05ad28p-12, -1.388888888888844490e-03);
    constexpr f64 cos_c8  = CMT_FP(0x1.a01a01a0022e6p-16, 2.480158730125666056e-05);
    constexpr f64 cos_c10 = CMT_FP(-0x4.9f93ed845de2cp-24, -2.755731909937878141e-07);
    constexpr f64 cos_c12 = CMT_FP(0x8.f76bc015abe48p-32, 2.087673146642573010e-09);
    constexpr f64 cos_c14 = CMT_FP(-0xc.9bf2dbe00379p-40, -1.146797738558921387e-11);
    constexpr f64 cos_c16 = CMT_FP(0xd.1232ac32f7258p-48, 4.643782497495272199e-14);

    vec<f64, N> x2 = folded * folded;
    vec<f64, N> formula =
        trig_horner(x2, cosmask, 1.0, 1.0, cos_c2, sin_c2, cos_c4, sin_c4, cos_c6, sin_c6, cos_c8, sin_c8,
                    cos_c10, sin_c10, cos_c12, sin_c12, cos_c14, sin_c14, cos_c16, sin_c16);

    formula = select(cosmask, formula, formula * folded);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1)>
KFR_INTRINSIC vec<T, N> sincos_mask(const vec<T, N>& x_full, const mask<T, N>& cosmask)
{
    vec<itype<T>, N> quadrant;
    vec<T, N> folded = trig_fold(x_full, quadrant);

    mask<T, N> flip_sign =
        kfr::select(cosmask, ((quadrant == 2) || (quadrant == 4)).asvec(), (quadrant >= 4).asvec()).asmask();

    mask<T, N> usecos = (quadrant == 2) || (quadrant == 6);
    usecos            = usecos ^ cosmask;

    vec<T, N> formula = trig_sincos(folded, usecos);

    mask<T, N> negmask = x_full < T(0);

    flip_sign = flip_sign ^ (negmask & ~cosmask);

    formula = select(flip_sign, -formula, formula);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> sin(const vec<T, N>& x)
{
    vec<itype<T>, N> quadrant;
    mask<T, N> xmask = mask<T, N>(x);
    vec<T, N> folded = trig_fold(x, quadrant);

    mask<T, N> flip_sign = (quadrant >= 4) ^ xmask;
    mask<T, N> usecos    = (quadrant == 2) || (quadrant == 6);

    vec<T, N> formula = trig_sincos(folded, usecos);

    formula = select(flip_sign, -formula, formula);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> cos(const vec<T, N>& x)
{
    vec<itype<T>, N> quadrant;
    vec<T, N> folded = trig_fold(x, quadrant);

    mask<T, N> eq4       = (quadrant == 4);
    mask<T, N> flip_sign = (quadrant == 2) || eq4;
    mask<T, N> usecos    = (quadrant == 0) || eq4;

    vec<T, N> formula = trig_sincos(folded, usecos);

    formula = select(flip_sign, -formula, formula);
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> fastsin(const vec<T, N>& x)
{
    const vec<T, N> msk = broadcast<N>(special_constants<T>::highbitmask());

    constexpr static T c2 = -0.16665853559970855712890625;
    constexpr static T c4 = +8.31427983939647674560546875e-3;
    constexpr static T c6 = -1.85423981747590005397796630859375e-4;

    const vec<T, N> pi = c_pi<T>;

    vec<T, N> xx = x - pi;
    vec<T, N> y  = abs(xx);
    y            = select(y > c_pi<T, 1, 2>, pi - y, y);
    y            = y ^ (msk & ~xx);

    vec<T, N> y2      = y * y;
    vec<T, N> formula = c6;
    vec<T, N> y3      = y2 * y;
    formula           = fmadd(formula, y2, c4);
    formula           = fmadd(formula, y2, c2);
    formula           = formula * y3 + y;
    return formula;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> fastcos(const vec<T, N>& x)
{
    x += c_pi<T, 1, 2>;
    x = select(x >= c_pi<T, 2>, x - c_pi<T, 2>, x);
    return fastsin(x);
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1 && is_f_class<T>)>
KFR_INTRINSIC vec<T, N> sincos(const vec<T, N>& x)
{
    return sincos_mask(x, internal::oddmask<T, N>());
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1 && is_f_class<T>)>
KFR_INTRINSIC vec<T, N> cossin(const vec<T, N>& x)
{
    return sincos_mask(x, internal::evenmask<T, N>());
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> sinc(const vec<T, N>& x)
{
    return select(abs(x) <= constants<T>::epsilon, T(1), sin(x) / x);
}

KFR_HANDLE_SCALAR_1_T(sin, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(cos, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(fastsin, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(fastcos, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(sincos, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(cossin, flt_type<T>)
KFR_HANDLE_SCALAR_1_T(sinc, flt_type<T>)

KFR_HANDLE_NOT_F_1(sin)
KFR_HANDLE_NOT_F_1(cos)
KFR_HANDLE_NOT_F_1(fastsin)
KFR_HANDLE_NOT_F_1(fastcos)
KFR_HANDLE_NOT_F_1(sincos)
KFR_HANDLE_NOT_F_1(cossin)
KFR_HANDLE_NOT_F_1(sinc)

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout sindeg(const T& x)
{
    return sin(x * constants<Tout>::degtorad);
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout cosdeg(const T& x)
{
    return cos(x * constants<Tout>::degtorad);
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout fastsindeg(const T& x)
{
    return fastsin(x * constants<Tout>::degtorad);
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout fastcosdeg(const T& x)
{
    return fastcos(x * constants<Tout>::degtorad);
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout sincosdeg(const T& x)
{
    return sincos(x * constants<Tout>::degtorad);
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout cossindeg(const T& x)
{
    return cossin(x * constants<Tout>::degtorad);
}
} // namespace intrinsics

KFR_I_FN(sin)
KFR_I_FN(cos)
KFR_I_FN(fastsin)
KFR_I_FN(fastcos)
KFR_I_FN(sincos)
KFR_I_FN(cossin)

KFR_I_FN(sindeg)
KFR_I_FN(cosdeg)
KFR_I_FN(fastsindeg)
KFR_I_FN(fastcosdeg)
KFR_I_FN(sincosdeg)
KFR_I_FN(cossindeg)

KFR_I_FN(sinc)
} // namespace CMT_ARCH_NAME
} // namespace kfr
