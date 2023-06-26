/** @addtogroup basic_math
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

#include "function.hpp"

#ifdef CMT_CLANG_EXT
#include "basicoperators_clang.hpp"
#else
#include "basicoperators_generic.hpp"
#endif

#include "basicoperators_complex.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
namespace intrinsics
{

#define KFR_VECVEC_OP1(fn)                                                                                   \
    template <typename T1, size_t N1, size_t N2>                                                             \
    KFR_INTRINSIC vec<vec<T1, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x)                                     \
    {                                                                                                        \
        return fn(x.flatten()).v;                                                                            \
    }

#define KFR_VECVEC_OP2(fn)                                                                                   \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = std::common_type_t<T1, T2>,       \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x, const vec<vec<T2, N1>, N2>& y)       \
    {                                                                                                        \
        return fn(broadcastto<C>(x.flatten()), broadcastto<C>(y.flatten())).v;                               \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = std::common_type_t<T1, T2>,       \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x, const T2& y)                         \
    {                                                                                                        \
        return fn(broadcastto<C>(x.flatten()), broadcastto<C>(y)).v;                                         \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = std::common_type_t<T1, T2>,       \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<vec<T1, N1>, N2>& x, const vec<T2, N1>& y)                \
    {                                                                                                        \
        return fn(broadcastto<C>(x.flatten()), repeat<N2>(broadcastto<C>(y.flatten()))).v;                   \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = std::common_type_t<T1, T2>,       \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const T1& x, const vec<vec<T2, N1>, N2>& y)                         \
    {                                                                                                        \
        return fn(broadcastto<C>(x), broadcastto<C>(y.flatten())).v;                                         \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N1, size_t N2, typename C = std::common_type_t<T1, T2>,       \
              KFR_ENABLE_IF(is_simd_type<C>)>                                                                \
    KFR_INTRINSIC vec<vec<C, N1>, N2> fn(const vec<T1, N1>& x, const vec<vec<T2, N1>, N2>& y)                \
    {                                                                                                        \
        return fn(repeat<N2>(broadcastto<C>(x.flatten())), broadcastto<C>(y.flatten())).v;                   \
    }

KFR_VECVEC_OP1(neg)
KFR_VECVEC_OP1(bnot)
KFR_VECVEC_OP2(add)
KFR_VECVEC_OP2(sub)
KFR_VECVEC_OP2(mul)
KFR_VECVEC_OP2(div)
KFR_VECVEC_OP2(mod)
KFR_VECVEC_OP2(band)
KFR_VECVEC_OP2(bor)
KFR_VECVEC_OP2(bxor)

} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
