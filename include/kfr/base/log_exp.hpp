/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "abs.hpp"
#include "clamp.hpp"
#include "constants.hpp"
#include "function.hpp"
#include "min_max.hpp"
#include "operators.hpp"
#include "round.hpp"
#include "select.hpp"
#include "shuffle.hpp"

namespace kfr
{

namespace internal
{

template <size_t N>
KFR_SINTRIN vec<i32, N> vilogbp1(vec<f32, N> d)
{
    mask<i32, N> m = d < 5.421010862427522E-20f;
    d = select(m, 1.8446744073709552E19f * d, d);
    vec<i32, N> q = (ibitcast(d) >> 23) & 0xff;
    q = select(m, q - (64 + 0x7e), q - 0x7e);
    return q;
}

template <size_t N>
KFR_SINTRIN vec<i64, N> vilogbp1(vec<f64, N> d)
{
    mask<i64, N> m = d < 4.9090934652977266E-91;
    d = select(m, 2.037035976334486E90 * d, d);
    vec<i64, N> q = (ibitcast(d) >> 52) & 0x7ff;
    q = select(m, q - (300 + 0x03fe), q - 0x03fe);
    return q;
}

template <size_t N>
KFR_SINTRIN vec<f32, N> vldexpk(vec<f32, N> x, vec<i32, N> q)
{
    vec<i32, N> m = q >> 31;
    m = (((m + q) >> 6) - m) << 4;
    q = q - (m << 2);
    m = clamp(m + 0x7f, vec<i32, N>(0xff));
    vec<f32, N> u = pow4(bitcast<f32>(cast<i32>(m) << 23));
    return x * u * bitcast<f32>((cast<i32>(q + 0x7f)) << 23);
}

template <size_t N>
KFR_SINTRIN vec<f64, N> vldexpk(vec<f64, N> x, vec<i64, N> q)
{
    vec<i64, N> m = q >> 31;
    m = (((m + q) >> 9) - m) << 7;
    q = q - (m << 2);
    m = clamp(m + 0x3ff, i64(0x7ff));
    vec<f64, N> u = pow4(bitcast<f64>(cast<i64>(m) << 52));
    return x * u * bitcast<f64>((cast<i64>(q + 0x3ff)) << 52);
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> logb(vec<T, N> x)
{
    return select(x == T(), -c_infinity<T>, cast<T>(vilogbp1(x) - 1));
}

template <size_t N>
KFR_SINTRIN vec<f32, N> log(vec<f32, N> d)
{
    vec<i32, N> e = vilogbp1(d * 0.7071); // 0678118654752440084436210485f );
    vec<f32, N> m = vldexpk(d, -e);

    vec<f32, N> x  = (m - 1.0f) / (m + 1.0f);
    vec<f32, N> x2 = x * x;

    vec<f32, N> sp = select(d < 0, c_qnan<f32>, c_neginfinity<f32>);

    vec<f32, N> t = 0.2371599674224853515625f;
    t = fmadd(t, x2, 0.285279005765914916992188f);
    t = fmadd(t, x2, 0.400005519390106201171875f);
    t = fmadd(t, x2, 0.666666567325592041015625f);
    t = fmadd(t, x2, 2.0f);

    x = x * t + c_log_2<f32> * cast<f32>(e);
    x = select(d > 0, x, sp);

    return x;
}

template <size_t N>
KFR_SINTRIN vec<f64, N> log(vec<f64, N> d)
{
    vec<i64, N> e = vilogbp1(d * 0.7071); // 0678118654752440084436210485 );
    vec<f64, N> m = vldexpk(d, -e);

    vec<f64, N> x  = (m - 1.0) / (m + 1.0);
    vec<f64, N> x2 = x * x;

    vec<f64, N> sp = select(d < 0, c_qnan<f64>, c_neginfinity<f64>);

    vec<f64, N> t = 0.148197055177935105296783;
    t = fmadd(t, x2, 0.153108178020442575739679);
    t = fmadd(t, x2, 0.181837339521549679055568);
    t = fmadd(t, x2, 0.22222194152736701733275);
    t = fmadd(t, x2, 0.285714288030134544449368);
    t = fmadd(t, x2, 0.399999999989941956712869);
    t = fmadd(t, x2, 0.666666666666685503450651);
    t = fmadd(t, x2, 2);

    x = x * t + c_log_2<f64> * cast<f64>(e);
    x = select(d > 0, x, sp);

    return x;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> log2(vec<T, N> x)
{
    return log(x) * c_recip_log_2<T>;
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> log10(vec<T, N> x)
{
    return log(x) * c_recip_log_10<T>;
}

template <size_t N>
KFR_SINTRIN vec<f32, N> exp(vec<f32, N> d)
{
    const f32 ln2_part1 = 0.6931457519f;
    const f32 ln2_part2 = 1.4286067653e-6f;

    vec<i32, N> q = cast<i32>(floor(d * c_recip_log_2<f32>));
    vec<f32, N> s, u;

    s = fmadd(cast<f32>(q), -ln2_part1, d);
    s = fmadd(cast<f32>(q), -ln2_part2, s);

    const f32 c2 = 0.4999999105930328369140625f;
    const f32 c3 = 0.166668415069580078125f;
    const f32 c4 = 4.16539050638675689697265625e-2f;
    const f32 c5 = 8.378830738365650177001953125e-3f;
    const f32 c6 = 1.304379315115511417388916015625e-3f;
    const f32 c7 = 2.7555381529964506626129150390625e-4f;

    u = c7;
    u = fmadd(u, s, c6);
    u = fmadd(u, s, c5);
    u = fmadd(u, s, c4);
    u = fmadd(u, s, c3);
    u = fmadd(u, s, c2);

    u = s * s * u + s + 1.0f;
    u = vldexpk(u, q);

    u = select(d == c_neginfinity<f32>, 0.f, u);

    return u;
}

template <size_t N>
KFR_SINTRIN vec<f64, N> exp(vec<f64, N> d)
{
    const f64 ln2_part1 = 0.69314717501401901245;
    const f64 ln2_part2 = 5.545926273775592108e-009;

    vec<i64, N> q = cast<i64>(floor(d * c_recip_log_2<f64>));
    vec<f64, N> s, u;

    s = fmadd(cast<f64>(q), -ln2_part1, d);
    s = fmadd(cast<f64>(q), -ln2_part2, s);

    const f64 c2  = 0.499999999999994948485237955537741072475910186767578;
    const f64 c3  = 0.166666666667024204739888659787538927048444747924805;
    const f64 c4  = 4.16666666578945840693215529881854308769106864929199e-2;
    const f64 c5  = 8.3333334397461874404333670440792047884315252304077e-3;
    const f64 c6  = 1.3888881489747750223179290074426717183087021112442e-3;
    const f64 c7  = 1.9841587032493949419205414574918222569976933300495e-4;
    const f64 c8  = 2.47929324077393282239802768662784160369483288377523e-5;
    const f64 c9  = 2.77076037925831049422552981864598109496000688523054e-6;
    const f64 c10 = 2.59589616274586264243611237120812340606335055781528e-7;
    const f64 c11 = 3.43801438838789632454461529017381016259946591162588e-8;

    u = c11;
    u = fmadd(u, s, c10);
    u = fmadd(u, s, c9);
    u = fmadd(u, s, c8);
    u = fmadd(u, s, c7);
    u = fmadd(u, s, c6);
    u = fmadd(u, s, c5);
    u = fmadd(u, s, c4);
    u = fmadd(u, s, c3);
    u = fmadd(u, s, c2);

    u = s * s * u + s + 1.0;
    u = vldexpk(u, q);

    u = select(d == c_neginfinity<f64>, 0.0, u);

    return u;
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> exp2(vec<T, N> x)
{
    return exp(x * c_log_2<T>);
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> exp10(vec<T, N> x)
{
    return exp(x * c_log_10<T>);
}

template <typename T1, typename T2>
KFR_SINTRIN common_type<T1, T2> logn(const T1& a, const T2& b)
{
    return log(a) / log(b);
}

template <typename T1, typename T2>
KFR_SINTRIN common_type<T1, T2> logm(const T1& a, const T2& b)
{
    return log(a) * b;
}

template <typename T1, typename T2, typename T3>
KFR_SINTRIN common_type<T1, T2, T3> exp_fmadd(const T1& x, const T2& m, const T3& a)
{
    return exp(fmadd(x, m, a));
}

template <typename T1, typename T2, typename T3>
KFR_SINTRIN common_type<T1, T2, T3> log_fmadd(const T1& x, const T2& m, const T3& a)
{
    return fmadd(log(x), m, a);
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> pow(vec<T, N> a, vec<T, N> b)
{
    const vec<T, N> t       = exp(b * log(abs(a)));
    const mask<T, N> isint  = floor(b) == b;
    const mask<T, N> iseven = (cast<itype<T>>(b) & 1) == 0;
    return select(a > T(), t,
                  select(a == T(), T(1), select(isint, select(iseven, t, -t), broadcast<N>(c_qnan<T>))));
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> root(vec<T, N> x, vec<T, N> b)
{
    return exp(reciprocal(b) * log(x));
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> cbrt(vec<T, N> x)
{
    return pow<T, N>(x, T(0.333333333333333333333333333333333));
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> exp(vec<T, N> x)
{
    return exp(cast<Tout>(x));
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> exp2(vec<T, N> x)
{
    return exp2(cast<Tout>(x));
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> exp10(vec<T, N> x)
{
    return exp10(cast<Tout>(x));
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> log(vec<T, N> x)
{
    return log(cast<Tout>(x));
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> log2(vec<T, N> x)
{
    return log2(cast<Tout>(x));
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> log10(vec<T, N> x)
{
    return log10(cast<Tout>(x));
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value), typename Tout = ftype<T>>
KFR_SINTRIN vec<Tout, N> cbrt(vec<T, N> x)
{
    return cbrt(cast<Tout>(x));
}

KFR_I_CONVERTER(exp)
KFR_I_CONVERTER(exp2)
KFR_I_CONVERTER(exp10)
KFR_I_CONVERTER(log)
KFR_I_CONVERTER(log2)
KFR_I_CONVERTER(log10)
KFR_I_CONVERTER(logb)
KFR_I_CONVERTER(pow)
KFR_I_CONVERTER(root)
KFR_I_CONVERTER(cbrt)

KFR_I_FN(exp)
KFR_I_FN(exp2)
KFR_I_FN(exp10)
KFR_I_FN(log)
KFR_I_FN(log2)
KFR_I_FN(log10)
KFR_I_FN(logb)
KFR_I_FN(logn)
KFR_I_FN(logm)
KFR_I_FN(exp_fmadd)
KFR_I_FN(log_fmadd)
KFR_I_FN(pow)
KFR_I_FN(root)
KFR_I_FN(cbrt)
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 exp(const T1& x)
{
    return internal::exp(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_exp, E1> exp(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 exp2(const T1& x)
{
    return internal::exp2(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_exp2, E1> exp2(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 exp10(const T1& x)
{
    return internal::exp10(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_exp10, E1> exp10(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 log(const T1& x)
{
    return internal::log(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_log, E1> log(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 log2(const T1& x)
{
    return internal::log2(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_log2, E1> log2(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 log10(const T1& x)
{
    return internal::log10(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_log10, E1> log10(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 logb(const T1& x)
{
    return internal::logb(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_logb, E1> logb(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INTRIN common_type<T1, T2> logn(const T1& x, const T2& y)
{
    return internal::logn(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<internal::fn_logn, E1, E2> logn(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INTRIN common_type<T1, T2> logm(const T1& x, const T2& y)
{
    return internal::logm(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<internal::fn_logm, E1, E2> logm(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value)>
KFR_INTRIN common_type<T1, T2, T3> exp_fmadd(const T1& x, const T2& y, const T3& z)
{
    return internal::exp_fmadd(x, y, z);
}

template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INTRIN expr_func<internal::fn_exp_fmadd, E1, E2, E3> exp_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value)>
KFR_INTRIN common_type<T1, T2, T3> log_fmadd(const T1& x, const T2& y, const T3& z)
{
    return internal::log_fmadd(x, y, z);
}

template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INTRIN expr_func<internal::fn_log_fmadd, E1, E2, E3> log_fmadd(E1&& x, E2&& y, E3&& z)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INTRIN common_type<T1, T2> pow(const T1& x, const T2& y)
{
    return internal::pow(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<internal::fn_pow, E1, E2> pow(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INTRIN common_type<T1, T2> root(const T1& x, const T2& y)
{
    return internal::root(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<internal::fn_root, E1, E2> root(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cbrt(const T1& x)
{
    return internal::cbrt(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_cbrt, E1> cbrt(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}
