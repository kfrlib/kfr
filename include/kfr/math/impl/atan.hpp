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
template <size_t N>
KFR_INTRINSIC vec<f32, N> atan2k(const vec<f32, N>& yy, const vec<f32, N>& xx)
{
    vec<f32, N> x = xx, y = yy;
    vec<f32, N> s, t, u;
    vec<i32, N> q;
    q = select(x < 0, -2, 0);
    x = select(x < 0, -x, x);
    mask<i32, N> m;
    m = y > x;
    t = x;
    x = select(m, y, x);
    y = select(m, -t, y);
    q = select(m, q + 1, q);
    s = y / x;
    t = s * s;
    u = fmadd(0.00282363896258175373077393f, t, -0.0159569028764963150024414f);
    u = fmadd(u, t, 0.0425049886107444763183594f);
    u = fmadd(u, t, -0.0748900920152664184570312f);
    u = fmadd(u, t, 0.106347933411598205566406f);
    u = fmadd(u, t, -0.142027363181114196777344f);
    u = fmadd(u, t, 0.199926957488059997558594f);
    u = fmadd(u, t, -0.333331018686294555664062f);
    t = u * t * s + s;
    t = broadcastto<f32>(q) * 1.5707963267948966192313216916398f + t;
    return t;
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> atan2k(const vec<f64, N>& yy, const vec<f64, N>& xx)
{
    vec<f64, N> x = xx, y = yy;
    vec<f64, N> s, t, u;
    vec<i64, N> q;
    q = select(x < 0, i64(-2), i64(0));
    x = select(x < 0, -x, x);
    mask<i64, N> m;
    m = y > x;
    t = x;
    x = select(m, y, x);
    y = select(m, -t, y);
    q = select(m, q + i64(1), q);
    s = y / x;
    t = s * s;
    u = fmadd(-1.88796008463073496563746e-05, t, 0.000209850076645816976906797);
    u = fmadd(u, t, -0.00110611831486672482563471);
    u = fmadd(u, t, 0.00370026744188713119232403);
    u = fmadd(u, t, -0.00889896195887655491740809);
    u = fmadd(u, t, 0.016599329773529201970117);
    u = fmadd(u, t, -0.0254517624932312641616861);
    u = fmadd(u, t, 0.0337852580001353069993897);
    u = fmadd(u, t, -0.0407629191276836500001934);
    u = fmadd(u, t, 0.0466667150077840625632675);
    u = fmadd(u, t, -0.0523674852303482457616113);
    u = fmadd(u, t, 0.0587666392926673580854313);
    u = fmadd(u, t, -0.0666573579361080525984562);
    u = fmadd(u, t, 0.0769219538311769618355029);
    u = fmadd(u, t, -0.090908995008245008229153);
    u = fmadd(u, t, 0.111111105648261418443745);
    u = fmadd(u, t, -0.14285714266771329383765);
    u = fmadd(u, t, 0.199999999996591265594148);
    u = fmadd(u, t, -0.333333333333311110369124);
    t = u * t * s + s;
    t = broadcastto<f64>(q) * 1.5707963267948966192313216916398 + t;
    return t;
}

template <size_t N>
KFR_INTRINSIC vec<f32, N> atan2(const vec<f32, N>& y, const vec<f32, N>& x)
{
    vec<f32, N> r           = atan2k(abs(y), x);
    constexpr f32 pi        = 3.1415926535897932384626433832795f;
    constexpr f32 pi_over_2 = 1.5707963267948966192313216916398f;
    constexpr f32 pi_over_4 = 0.78539816339744830961566084581988f;
    r                       = mulsign(r, x);
    r = select(isinf(x) || x == 0.0f, pi_over_2 - select(x.asmask(), mulsign(pi_over_2, x), 0.0f), r);
    r = select(isinf(y), pi_over_2 - select(x.asmask(), mulsign(pi_over_4, x), 0.0f), r);
    r = select(y == 0.0f, select(x < 0.f, pi, 0.f), r);
    r = (isnan(x) || isnan(y)).asvec() | mulsign(r, y);
    return r;
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> atan2(const vec<f64, N>& y, const vec<f64, N>& x)
{
    vec<f64, N> r           = atan2k(abs(y), x);
    constexpr f64 pi        = 3.1415926535897932384626433832795;
    constexpr f64 pi_over_2 = 1.5707963267948966192313216916398;
    constexpr f64 pi_over_4 = 0.78539816339744830961566084581988;
    r                       = mulsign(r, x);
    r = select(isinf(x) || x == 0.0, pi_over_2 - select(x.asmask(), mulsign(pi_over_2, x), 0.0), r);
    r = select(isinf(y), pi_over_2 - select(x.asmask(), mulsign(pi_over_4, x), 0.0), r);
    r = select(y == 0.0, select(x < 0., pi, 0.), r);
    r = (isnan(x) || isnan(y)).asvec() | mulsign(r, y);
    return r;
}

template <size_t N>
KFR_INTRINSIC vec<f32, N> atan(const vec<f32, N>& x)
{
    vec<f32, N> t, u;
    vec<i32, N> q;
    q             = select(x < 0.f, 2, 0);
    vec<f32, N> s = select(x < 0.f, -x, x);
    q             = select(s > 1.f, q | 1, q);
    s             = select(s > 1.f, 1.0f / s, s);
    t             = s * s;
    u             = fmadd(0.00282363896258175373077393f, t, -0.0159569028764963150024414f);
    u             = fmadd(u, t, 0.0425049886107444763183594f);
    u             = fmadd(u, t, -0.0748900920152664184570312f);
    u             = fmadd(u, t, 0.106347933411598205566406f);
    u             = fmadd(u, t, -0.142027363181114196777344f);
    u             = fmadd(u, t, 0.199926957488059997558594f);
    u             = fmadd(u, t, -0.333331018686294555664062f);
    t             = s + s * (t * u);
    t             = select((q & 1) != 0, 1.570796326794896557998982f - t, t);
    t             = select((q & 2) != 0, -t, t);
    return t;
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> atan(const vec<f64, N>& x)
{
    vec<f64, N> t, u;
    vec<i64, N> q;
    q             = select(x < 0.0, i64(2), i64(0));
    vec<f64, N> s = select(x < 0.0, -x, x);
    q             = select(s > 1.0, q | 1, q);
    s             = select(s > 1.0, 1.0 / s, s);
    t             = s * s;
    u             = fmadd(-1.88796008463073496563746e-05, t, 0.000209850076645816976906797);
    u             = fmadd(u, t, -0.00110611831486672482563471);
    u             = fmadd(u, t, 0.00370026744188713119232403);
    u             = fmadd(u, t, -0.00889896195887655491740809);
    u             = fmadd(u, t, 0.016599329773529201970117);
    u             = fmadd(u, t, -0.0254517624932312641616861);
    u             = fmadd(u, t, 0.0337852580001353069993897);
    u             = fmadd(u, t, -0.0407629191276836500001934);
    u             = fmadd(u, t, 0.0466667150077840625632675);
    u             = fmadd(u, t, -0.0523674852303482457616113);
    u             = fmadd(u, t, 0.0587666392926673580854313);
    u             = fmadd(u, t, -0.0666573579361080525984562);
    u             = fmadd(u, t, 0.0769219538311769618355029);
    u             = fmadd(u, t, -0.090908995008245008229153);
    u             = fmadd(u, t, 0.111111105648261418443745);
    u             = fmadd(u, t, -0.14285714266771329383765);
    u             = fmadd(u, t, 0.199999999996591265594148);
    u             = fmadd(u, t, -0.333333333333311110369124);
    t             = s + s * (t * u);
    t             = select((q & 1) != 0, 1.570796326794896557998982 - t, t);
    t             = select((q & 2) != 0, -t, t);
    return t;
}

template <size_t N>
KFR_INTRINSIC vec<f32, N> atandeg(const vec<f32, N>& x)
{
    return atan(x) * c_radtodeg<f32>;
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> atandeg(const vec<f64, N>& x)
{
    return atan(x) * c_radtodeg<f64>;
}

template <size_t N>
KFR_INTRINSIC vec<f32, N> atan2deg(const vec<f32, N>& y, const vec<f32, N>& x)
{
    return atan2(y, x) * c_radtodeg<f32>;
}

template <size_t N>
KFR_INTRINSIC vec<f64, N> atan2deg(const vec<f64, N>& y, const vec<f64, N>& x)
{
    return atan2(y, x) * c_radtodeg<f64>;
}

KFR_HANDLE_SCALAR(atan)
KFR_HANDLE_SCALAR(atan2)
KFR_HANDLE_SCALAR(atandeg)
KFR_HANDLE_SCALAR(atan2deg)
} // namespace intrinsics
KFR_I_FN(atan)
KFR_I_FN(atandeg)
KFR_I_FN(atan2)
KFR_I_FN(atan2deg)
} // namespace CMT_ARCH_NAME
} // namespace kfr
