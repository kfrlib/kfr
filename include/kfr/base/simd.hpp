/** @addtogroup types
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

#include "kfr.h"
#include "types.hpp"

namespace kfr
{

#ifdef CMT_COMPILER_CLANG

using simdindex = int;

template <typename T, simdindex N>
using simd = T __attribute__((ext_vector_type(N)));

#define KFT_CONVERT_VECTOR(X, T, N) __builtin_convertvector(X, ::kfr::simd<T, N>)

#define KFR_SIMD_PARAM_ARE_DEDUCIBLE 1
#define KFR_SIMD_FROM_SCALAR(X, T, N) (X)
#define KFR_BUILTIN_SHUFFLEVECTOR(T, N, X, Y, I) __builtin_shufflevector(X, Y, I)

#elif defined CMT_COMPILER_GNU

using simdindex = int;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

template <typename T, simdindex N>
struct simd_gcc
{
    constexpr static size_t NN = next_poweroftwo(N);
    typedef __attribute__((vector_size(NN * sizeof(T)))) T simd_type;
    typedef simd_type type __attribute__((__packed__, __aligned__(sizeof(T))));
};
#pragma GCC diagnostic pop

template <typename T, simdindex N>
using simd = typename simd_gcc<T, N>::type;

#define KFT_CONVERT_VECTOR(X, T, N) static_cast<::kfr::simd<T, N>>(X)
#define KFR_SIMD_FROM_SCALAR(X, T, N)                                                                        \
    (__builtin_shuffle(::kfr::simd<T, N>{ X }, ::kfr::simd<int_type<sizeof(T) * 8>, N>{ 0 }))
#define KFR_BUILTIN_SHUFFLEVECTOR(T, N, X, Y, I) ::kfr::internal::builtin_shufflevector<T, N>(X, Y, I)

namespace internal
{
template <typename T, size_t N, typename... Int>
KFR_INTRIN simd<T, sizeof...(Int)> builtin_shufflevector(const simd<T, N>& x, const simd<T, N>& y,
                                                         const Int&... indices)
{
    return simd<T, sizeof...(Int)>{ (indices < N ? x[indices] : y[indices])... };
}
}

#endif

template <typename T, size_t N>
struct vec_op
{
    using type                = subtype<T>;
    using utype               = kfr::utype<type>;
    constexpr static size_t w = compound_type_traits<T>::width * N;

    CMT_INLINE constexpr static simd<type, w> add(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x + y;
    }
    CMT_INLINE constexpr static simd<type, w> sub(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x - y;
    }
    CMT_INLINE constexpr static simd<type, w> mul(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x * y;
    }
    CMT_INLINE constexpr static simd<type, w> div(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x / y;
    }
    CMT_INLINE constexpr static simd<type, w> rem(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x % y;
    }
    CMT_INLINE constexpr static simd<type, w> shl(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x << y;
    }
    CMT_INLINE constexpr static simd<type, w> shr(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return x >> y;
    }
    CMT_INLINE constexpr static simd<type, w> neg(const simd<type, w>& x) noexcept { return -x; }
    CMT_INLINE constexpr static simd<type, w> band(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(reinterpret_cast<simd<utype, w>>(x) &
                                               reinterpret_cast<simd<utype, w>>(y));
    }
    CMT_INLINE constexpr static simd<type, w> bor(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(reinterpret_cast<simd<utype, w>>(x) |
                                               reinterpret_cast<simd<utype, w>>(y));
    }
    CMT_INLINE constexpr static simd<type, w> bxor(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(reinterpret_cast<simd<utype, w>>(x) ^
                                               reinterpret_cast<simd<utype, w>>(y));
    }
    CMT_INLINE constexpr static simd<type, w> bnot(const simd<type, w>& x) noexcept
    {
        return reinterpret_cast<simd<type, w>>(~reinterpret_cast<simd<utype, w>>(x));
    }
    CMT_INLINE constexpr static simd<type, w> eq(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(x == y);
    }
    CMT_INLINE constexpr static simd<type, w> ne(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(x != y);
    }
    CMT_INLINE constexpr static simd<type, w> lt(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(x < y);
    }
    CMT_INLINE constexpr static simd<type, w> gt(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(x > y);
    }
    CMT_INLINE constexpr static simd<type, w> le(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(x <= y);
    }
    CMT_INLINE constexpr static simd<type, w> ge(const simd<type, w>& x, const simd<type, w>& y) noexcept
    {
        return reinterpret_cast<simd<type, w>>(x >= y);
    }
};
}
