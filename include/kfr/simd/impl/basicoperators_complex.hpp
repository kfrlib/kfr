/** @addtogroup complex
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

#include "../complex_type.hpp"
#include "../operators.hpp"
#include "../vec.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
namespace intrinsics
{

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> neg(const vec<complex<T>, N>& x)
{
    return neg(x.flatten()).v;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> add(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    return add(x.flatten(), y.flatten()).v;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> sub(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    return sub(x.flatten(), y.flatten()).v;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> mul(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    const vec<T, (N * 2)> xx = x.v;
    const vec<T, (N * 2)> yy = y.v;
    return subadd(mul(xx, dupeven(yy)), mul(swap<2>(xx), dupodd(yy))).v;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> div(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    const vec<T, (N * 2)> xx = x.v;
    const vec<T, (N * 2)> yy = y.v;
    const vec<T, (N * 2)> m  = (add(sqr(dupeven(yy)), sqr(dupodd(yy))));
    return swap<2>(subadd(mul(swap<2>(xx), dupeven(yy)), mul(xx, dupodd(yy))) / m).v;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> bor(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    return bor(x.flatten(), y.flatten()).v;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> bxor(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    return bxor(x.flatten(), y.flatten()).v;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> band(const vec<complex<T>, N>& x, const vec<complex<T>, N>& y)
{
    return band(x.flatten(), y.flatten()).v;
}

#define KFR_COMPLEX_OP_CVT(fn)                                                                               \
    template <typename T, size_t N>                                                                          \
    KFR_INTRINSIC vec<complex<T>, N> fn(const vec<complex<T>, N>& x, const complex<T>& y)                    \
    {                                                                                                        \
        return fn(x, vec<complex<T>, N>(y));                                                                 \
    }                                                                                                        \
    template <typename T, size_t N>                                                                          \
    KFR_INTRINSIC vec<complex<T>, N> fn(const complex<T>& x, const vec<complex<T>, N>& y)                    \
    {                                                                                                        \
        return fn(vec<complex<T>, N>(x), y);                                                                 \
    }

KFR_COMPLEX_OP_CVT(add)
KFR_COMPLEX_OP_CVT(sub)
KFR_COMPLEX_OP_CVT(mul)
KFR_COMPLEX_OP_CVT(div)
KFR_COMPLEX_OP_CVT(band)
KFR_COMPLEX_OP_CVT(bxor)
KFR_COMPLEX_OP_CVT(bor)

} // namespace intrinsics
} // namespace CMT_ARCH_NAME

} // namespace kfr
