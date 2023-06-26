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
#include "../../simd/operators.hpp"
#include "../../simd/select.hpp"
#include "../sin_cos.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T, size_t N, typename IT = itype<T>>
KFR_INTRINSIC vec<T, N> trig_fold_simple(const vec<T, N>& x_full, mask<T, N>& inverse)
{
    constexpr T pi_14 = c_pi<T, 1, 4>;

    vec<T, N> y      = abs(x_full);
    vec<T, N> scaled = y / pi_14;

    vec<T, N> k_real = floor(scaled);
    vec<IT, N> k     = broadcastto<IT>(k_real);

    vec<T, N> x = y - k_real * pi_14;

    mask<T, N> need_offset = (k & 1) != 0;
    x                      = select(need_offset, x - pi_14, x);

    vec<IT, N> k_mod4 = k & 3;
    inverse           = (k_mod4 == 1) || (k_mod4 == 2);
    return x;
}

template <size_t N>
KFR_INTRINSIC vec<f32, N> tan(const vec<f32, N>& x_full)
{
    mask<f32, N> inverse;
    vec<i32, N> quad;
    const vec<f32, N> x = trig_fold(x_full, quad); // trig_fold_simple(x_full, inverse);
    inverse             = quad == 2 || quad == 6;

    constexpr f32 tan_c2  = CMT_FP(0x5.555378p-4, 3.333315551280975342e-01);
    constexpr f32 tan_c4  = CMT_FP(0x2.225bb8p-4, 1.333882510662078857e-01);
    constexpr f32 tan_c6  = CMT_FP(0xd.ac3fep-8, 5.340956896543502808e-02);
    constexpr f32 tan_c8  = CMT_FP(0x6.41644p-8, 2.443529665470123291e-02);
    constexpr f32 tan_c10 = CMT_FP(0xc.bfe7ep-12, 3.112703096121549606e-03);
    constexpr f32 tan_c12 = CMT_FP(0x2.6754dp-8, 9.389210492372512817e-03);

    constexpr f32 cot_c2  = CMT_FP(-0x5.555558p-4, -3.333333432674407959e-01);
    constexpr f32 cot_c4  = CMT_FP(-0x5.b0581p-8, -2.222204580903053284e-02);
    constexpr f32 cot_c6  = CMT_FP(-0x8.ac5ccp-12, -2.117502503097057343e-03);
    constexpr f32 cot_c8  = CMT_FP(-0xd.aaa01p-16, -2.085343148792162538e-04);
    constexpr f32 cot_c10 = CMT_FP(-0x1.a9a9b4p-16, -2.537148611736483872e-05);
    constexpr f32 cot_c12 = CMT_FP(-0x6.f7d4dp-24, -4.153305894760705996e-07);

    const vec<f32, N> x2  = x * x;
    const vec<f32, N> val = trig_horner(x2, inverse, 1.0f, 1.0f, cot_c2, tan_c2, cot_c4, tan_c4, cot_c6,
                                        tan_c6, cot_c8, tan_c8, cot_c10, tan_c10, cot_c12, tan_c12);

    const vec<f32, N> z = select(inverse, val / -x, val * x);
    return mulsign(z, x_full);
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> tan(const vec<f64, N>& x_full)
{
    mask<f64, N> inverse;
    vec<i64, N> quad;
    const vec<f64, N> x = trig_fold(x_full, quad); // trig_fold_simple(x_full, inverse);
    inverse             = quad == 2 || quad == 6;

    constexpr f64 tan_c2  = 0x1.5555555555a3cp-2;
    constexpr f64 tan_c4  = 0x1.11111110c4068p-3;
    constexpr f64 tan_c6  = 0x1.ba1ba1ef36a4dp-5;
    constexpr f64 tan_c8  = 0x1.664f3f4af7ce2p-6;
    constexpr f64 tan_c10 = 0x1.226f2682a2616p-7;
    constexpr f64 tan_c12 = 0x1.d6b440e73f61dp-9;
    constexpr f64 tan_c14 = 0x1.7f06cdd30bd39p-10;
    constexpr f64 tan_c16 = 0x1.2a8fab895738ep-11;
    constexpr f64 tan_c18 = 0x1.34ff88cfdc292p-12;
    constexpr f64 tan_c20 = -0x1.b4165ea04339fp-18;
    constexpr f64 tan_c22 = 0x1.5f93701d86962p-13;
    constexpr f64 tan_c24 = -0x1.5a13a3cdfb8c1p-14;
    constexpr f64 tan_c26 = 0x1.77c69cef3306cp-15;

    constexpr f64 cot_c2  = -0x1.5555555555555p-2;
    constexpr f64 cot_c4  = -0x1.6c16c16c16dcdp-6;
    constexpr f64 cot_c6  = -0x1.1566abbff68a7p-9;
    constexpr f64 cot_c8  = -0x1.bbd7794ef9999p-13;
    constexpr f64 cot_c10 = -0x1.66a8ea1991906p-16;
    constexpr f64 cot_c12 = -0x1.228220068711cp-19;
    constexpr f64 cot_c14 = -0x1.d65ed2c45e21dp-23;
    constexpr f64 cot_c16 = -0x1.897ead4a2f71dp-26;
    constexpr f64 cot_c18 = -0x1.b592dc8656ec9p-31;
    constexpr f64 cot_c20 = -0x1.3dc07078c46d6p-29;
    constexpr f64 cot_c22 = 0x1.06c9e5c370edcp-29;
    constexpr f64 cot_c24 = -0x1.217f50c9dbca3p-30;
    constexpr f64 cot_c26 = 0x1.163ed8171a0c8p-32;

    const vec<f64, N> x2 = x * x;
    const vec<f64, N> val =
        trig_horner(x2, inverse, 1.0, 1.0, cot_c2, tan_c2, cot_c4, tan_c4, cot_c6, tan_c6, cot_c8, tan_c8,
                    cot_c10, tan_c10, cot_c12, tan_c12, cot_c14, tan_c14, cot_c16, tan_c16, cot_c18, tan_c18,
                    cot_c20, tan_c20, cot_c22, tan_c22, cot_c24, tan_c24, cot_c26, tan_c26);

    const vec<f64, N> z = select(inverse, val / -x, val * x);
    return mulsign(z, x_full);
}

KFR_HANDLE_SCALAR_1_T(tan, flt_type<T>)
KFR_HANDLE_NOT_F_1(tan)

template <typename T>
KFR_INTRINSIC flt_type<T> tandeg(const T& x)
{
    return tan(x * c_degtorad<flt_type<T>>);
}
} // namespace intrinsics
namespace fn
{
}
KFR_I_FN(tan)
KFR_I_FN(tandeg)
} // namespace CMT_ARCH_NAME
} // namespace kfr
