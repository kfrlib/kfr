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

#include "../mask.hpp"
#include "function.hpp"
#include <algorithm>
#include <utility>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> neg(const vec<T, N>& x)
{
    return -x.v;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> bnot(const vec<T, N>& x)
{
    return simd_bitcast(simd_cvt_t<T, utype<T>, N>{}, ~simd_bitcast(simd_cvt_t<utype<T>, T, N>{}, x.v));
}

#define KFR_OP_SCALAR2(fn, op, resultprefix, operprefix, soperprefix)                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>                                          \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& x, const T& y)                                               \
    {                                                                                                        \
        return resultprefix(operprefix(x.v) op soperprefix(y));                                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>                                          \
    KFR_INTRINSIC vec<T, N> fn(const T& x, const vec<T, N>& y)                                               \
    {                                                                                                        \
        return resultprefix(soperprefix(x) op operprefix(y.v));                                              \
    }

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> add(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.v + y.v;
}
KFR_OP_SCALAR2(add, +, , , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> sub(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.v - y.v;
}
KFR_OP_SCALAR2(sub, -, , , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> mul(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.v * y.v;
}
KFR_OP_SCALAR2(mul, *, , , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> div(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.v / y.v;
}
KFR_OP_SCALAR2(div, /, , , )
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> mod(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.v % y.v;
}
KFR_OP_SCALAR2(mod, %, , , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> band(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)((simd<utype<T>, N>)(x.v) & (simd<utype<T>, N>)(y.v));
}
KFR_OP_SCALAR2(band, &, (simd<T, N>), (simd<utype<T>, N>), ubitcast)

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> bor(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)((simd<utype<T>, N>)(x.v) | (simd<utype<T>, N>)(y.v));
}
KFR_OP_SCALAR2(bor, |, (simd<T, N>), (simd<utype<T>, N>), ubitcast)

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> bxor(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)((simd<utype<T>, N>)(x.v) ^ (simd<utype<T>, N>)(y.v));
}
KFR_OP_SCALAR2(bxor, ^, (simd<T, N>), (simd<utype<T>, N>), ubitcast)

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shl(const vec<T, N>& x, const vec<utype<T>, N>& y)
{
    return (simd<T, N>)((simd<uitype<deep_subtype<T>>, N * sizeof(deep_subtype<T>) / sizeof(T)>)(x.v) << y.v);
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shr(const vec<T, N>& x, const vec<utype<T>, N>& y)
{
    return (simd<T, N>)((simd<uitype<deep_subtype<T>>, N * sizeof(deep_subtype<T>) / sizeof(T)>)(x.v) >> y.v);
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shl(const vec<T, N>& x, unsigned y)
{
    return (simd<T, N>)((simd<uitype<deep_subtype<T>>, N * sizeof(deep_subtype<T>) / sizeof(T)>)(x.v) << y);
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shr(const vec<T, N>& x, unsigned y)
{
    return (simd<T, N>)((simd<uitype<deep_subtype<T>>, N * sizeof(deep_subtype<T>) / sizeof(T)>)(x.v) >> y);
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> eq(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)(x.v == y.v);
}
KFR_OP_SCALAR2(eq, ==, (simd<T, N>), , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> ne(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)(x.v != y.v);
}
KFR_OP_SCALAR2(ne, !=, (simd<T, N>), , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> le(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)(x.v <= y.v);
}
KFR_OP_SCALAR2(le, <=, (simd<T, N>), , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> ge(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)(x.v >= y.v);
}
KFR_OP_SCALAR2(ge, >=, (simd<T, N>), , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> lt(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)(x.v < y.v);
}
KFR_OP_SCALAR2(lt, <, (simd<T, N>), , )

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> gt(const vec<T, N>& x, const vec<T, N>& y)
{
    return (simd<T, N>)(x.v > y.v);
}
KFR_OP_SCALAR2(gt, >, (simd<T, N>), , )
} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
