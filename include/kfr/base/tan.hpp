/** @addtogroup math
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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
#include "abs.hpp"
#include "constants.hpp"
#include "function.hpp"
#include "operators.hpp"
#include "select.hpp"
#include "sin_cos.hpp"

namespace kfr
{

namespace intrinsics
{

template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<T, N> trig_fold_simple(const vec<T, N>& x_full, mask<T, N>& inverse)
{
    constexpr T pi_14 = c_pi<T, 1, 4>;

    vec<T, N> y      = abs(x_full);
    vec<T, N> scaled = y / pi_14;

    vec<T, N> k_real = floor(scaled);
    vec<IT, N> k     = cast<IT>(k_real);

    vec<T, N> x = y - k_real * pi_14;

    mask<T, N> need_offset = (k & 1) != 0;
    x = select(need_offset, x - pi_14, x);

    vec<IT, N> k_mod4 = k & 3;
    inverse = (k_mod4 == 1) || (k_mod4 == 2);
    return x;
}

template <size_t N>
KFR_SINTRIN vec<f32, N> tan(const vec<f32, N>& x_full)
{
    mask<f32, N> inverse;
    const vec<f32, N> x = trig_fold_simple(x_full, inverse);

    constexpr f32 tan_c2  = 0x5.555378p-4;
    constexpr f32 tan_c4  = 0x2.225bb8p-4;
    constexpr f32 tan_c6  = 0xd.ac3fep-8;
    constexpr f32 tan_c8  = 0x6.41644p-8;
    constexpr f32 tan_c10 = 0xc.bfe7ep-12;
    constexpr f32 tan_c12 = 0x2.6754dp-8;

    constexpr f32 cot_c2  = -0x5.555558p-4;
    constexpr f32 cot_c4  = -0x5.b0581p-8;
    constexpr f32 cot_c6  = -0x8.ac5ccp-12;
    constexpr f32 cot_c8  = -0xd.aaa01p-16;
    constexpr f32 cot_c10 = -0x1.a9a9b4p-16;
    constexpr f32 cot_c12 = -0x6.f7d4dp-24;

    const vec<f32, N> x2  = x * x;
    const vec<f32, N> val = trig_horner(x2, inverse, 1.0f, 1.0f, cot_c2, tan_c2, cot_c4, tan_c4, cot_c6,
                                        tan_c6, cot_c8, tan_c8, cot_c10, tan_c10, cot_c12, tan_c12);

    const vec<f32, N> z = select(inverse, val / -x, val * x);
    return mulsign(z, x_full);
}

template <size_t N>
KFR_SINTRIN vec<f64, N> tan(const vec<f64, N>& x_full)
{
    mask<f64, N> inverse;
    const vec<f64, N> x = trig_fold_simple(x_full, inverse);

    constexpr f64 tan_c2  = 0x5.5555554d8e5b8p-4;
    constexpr f64 tan_c4  = 0x2.222224820264p-4;
    constexpr f64 tan_c6  = 0xd.d0d90de32b3e8p-8;
    constexpr f64 tan_c8  = 0x5.99723bdcf5cacp-8;
    constexpr f64 tan_c10 = 0x2.434a142e413ap-8;
    constexpr f64 tan_c12 = 0xf.2b59061305efp-12;
    constexpr f64 tan_c14 = 0x4.a12565071a664p-12;
    constexpr f64 tan_c16 = 0x4.dada3797ac1bcp-12;
    constexpr f64 tan_c18 = -0x1.a74976b6ea3f3p-12;
    constexpr f64 tan_c20 = 0x1.d06a5ae5e4a74p-12;

    constexpr f64 cot_c2  = -0x5.5555555555554p-4;
    constexpr f64 cot_c4  = -0x5.b05b05b05b758p-8;
    constexpr f64 cot_c6  = -0x8.ab355dffc79a8p-12;
    constexpr f64 cot_c8  = -0xd.debbca405c9f8p-16;
    constexpr f64 cot_c10 = -0x1.66a8edb99b15p-16;
    constexpr f64 cot_c12 = -0x2.450239be0ee92p-20;
    constexpr f64 cot_c14 = -0x3.ad6ddb4719438p-24;
    constexpr f64 cot_c16 = -0x5.ff4c42741356p-28;
    constexpr f64 cot_c18 = -0x9.06881bcdf3108p-32;
    constexpr f64 cot_c20 = -0x1.644abedc113cap-32;

    const vec<f64, N> x2  = x * x;
    const vec<f64, N> val = trig_horner(x2, inverse, 1.0, 1.0, cot_c2, tan_c2, cot_c4, tan_c4, cot_c6, tan_c6,
                                        cot_c8, tan_c8, cot_c10, tan_c10, cot_c12, tan_c12, cot_c14, tan_c14,
                                        cot_c16, tan_c16, cot_c18, tan_c18, cot_c20, tan_c20);

    const vec<f64, N> z = select(inverse, val / -x, val * x);
    return mulsign(z, x_full);
}

KFR_I_FLT_CONVERTER(tan)
template <typename T>
KFR_SINTRIN flt_type<T> tandeg(const T& x)
{
    return tan(x * c_degtorad<flt_type<T>>);
}
}
KFR_I_FN(tan)
KFR_I_FN(tandeg)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN flt_type<T1> tan(const T1& x)
{
    return intrinsics::tan(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::tan, E1> tan(E1&& x)
{
    return { fn::tan(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN flt_type<T1> tandeg(const T1& x)
{
    return intrinsics::tandeg(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::tandeg, E1> tandeg(E1&& x)
{
    return { fn::tandeg(), std::forward<E1>(x) };
}
}
