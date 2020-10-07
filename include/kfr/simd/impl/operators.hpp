/** @addtogroup basic_math
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

#include "function.hpp"

#ifdef CMT_CLANG_EXT
#include "basicoperators_clang.hpp"
#else
#include "basicoperators_generic.hpp"
#endif

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

KFR_COMPLEX_OP_CVT(mul)
KFR_COMPLEX_OP_CVT(div)
KFR_COMPLEX_OP_CVT(band)
KFR_COMPLEX_OP_CVT(bxor)
KFR_COMPLEX_OP_CVT(bor)

#define KFR_VECVEC_OP1(fn)                                                                                   \
    template <typename T1, size_t N1, size_t N2>                                                             \
    KFR_INTRINSIC vec<vec<T1, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x)                                     \
    {                                                                                                        \
        return fn(x.flatten()).v;                                                                            \
    }

#define KFR_VECVEC_OP2(fn)                                                                                   \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = common_type<T1, T2>,              \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x, const vec<vec<T2, N1>, N2>& y)       \
    {                                                                                                        \
        return fn(innercast<C>(x.flatten()), innercast<C>(y.flatten())).v;                                   \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = common_type<T1, T2>,              \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x, const T2& y)                         \
    {                                                                                                        \
        return fn(innercast<C>(x.flatten()), innercast<C>(y)).v;                                             \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = common_type<T1, T2>,              \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x, const vec<T2, N1>& y)                \
    {                                                                                                        \
        return fn(innercast<C>(x.flatten()), repeat<N2>(innercast<C>(y.flatten()))).v;                       \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = common_type<T1, T2>,              \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const T1& x, const vec<vec<T2, N1>, N2>& y)                         \
    {                                                                                                        \
        return fn(innercast<C>(x), innercast<C>(y.flatten())).v;                                             \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = common_type<T1, T2>,              \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<T1, N1>& x, const vec<vec<T2, N1>, N2>& y)                \
    {                                                                                                        \
        return fn(repeat<N2>(innercast<C>(x.flatten())), innercast<C>(y.flatten())).v;                       \
    }

#define KFR_VECVECVEC_OP1(fn)                                                                                \
    template <typename T1, size_t N1, size_t N2, size_t N3>                                                  \
    KFR_INTRINSIC vec<vec<vec<T1, N1>, N2>, N3> fn(const vec<vec<vec<T1, N1>, N2>, N3>& x)                   \
    {                                                                                                        \
        return fn(x.flatten()).v;                                                                            \
    }

#define KFR_VECVECVEC_OP2(fn)                                                                                \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const vec<vec<vec<T1, N1>, N2>, N3>& x,                    \
                                                  const vec<vec<vec<T2, N1>, N2>, N3>& y)                    \
    { /* VVV @ VVV */                                                                                        \
        return fn(innercast<C>(x.flatten()), innercast<C>(y.flatten())).v;                                   \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const vec<vec<vec<T1, N1>, N2>, N3>& x,                    \
                                                  const vec<vec<T2, N1>, N2>& y)                             \
    { /* VVV @ VV */                                                                                         \
        return fn(innercast<C>(x.flatten()), repeat<N3>(innercast<C>(y.flatten()))).v;                       \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const vec<vec<T1, N1>, N2>& x,                             \
                                                  const vec<vec<vec<T2, N1>, N2>, N3>& y)                    \
    { /* VV @ VVV */                                                                                         \
        return fn(repeat<N3>(innercast<C>(x.flatten())), innercast<C>(y.flatten())).v;                       \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const vec<vec<vec<T1, N1>, N2>, N3>& x, const T2& y)       \
    { /* VVV @ S */                                                                                          \
        return fn(innercast<C>(x.flatten()), innercast<C>(y)).v;                                             \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const vec<vec<vec<T1, N1>, N2>, N3>& x,                    \
                                                  const vec<T2, N1>& y)                                      \
    { /* VVV @ V */                                                                                          \
        return fn(innercast<C>(x.flatten()), repeat<N2>(innercast<C>(y.flatten()))).v;                       \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const T1& x, const vec<vec<vec<T2, N1>, N2>, N3>& y)       \
    { /* S @ VVV */                                                                                          \
        return fn(innercast<C>(x), innercast<C>(y.flatten())).v;                                             \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, size_t N3, typename C = common_type<T1, T2>,   \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<vec<C, N1>, N2>, N3> fn(const vec<T1, N1>& x,                                      \
                                                  const vec<vec<vec<T2, N1>, N2>, N3>& y)                    \
    { /* V @ VVV */                                                                                          \
        return fn(repeat<N2>(innercast<C>(x.flatten())), innercast<C>(y.flatten())).v;                       \
    }

KFR_VECVEC_OP1(neg)
KFR_VECVEC_OP1(bnot)
KFR_VECVEC_OP2(add)
KFR_VECVEC_OP2(sub)
KFR_VECVEC_OP2(mul)
KFR_VECVEC_OP2(div)
KFR_VECVEC_OP2(band)
KFR_VECVEC_OP2(bor)
KFR_VECVEC_OP2(bxor)

KFR_VECVECVEC_OP1(neg)
KFR_VECVECVEC_OP1(bnot)
KFR_VECVECVEC_OP2(add)
KFR_VECVECVEC_OP2(sub)
KFR_VECVECVEC_OP2(mul)
KFR_VECVECVEC_OP2(div)
KFR_VECVECVEC_OP2(band)
KFR_VECVECVEC_OP2(bor)
KFR_VECVECVEC_OP2(bxor)

} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
