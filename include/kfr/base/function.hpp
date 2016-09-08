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

#include "expression.hpp"
#include "shuffle.hpp"
#include "types.hpp"
#include "vec.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

namespace kfr
{

#define KFR_I_CONVERTER(fn)                                                                                  \
    template <typename T1, typename... Args, typename Tout = ::cometa::common_type<T1, Args...>>             \
    KFR_SINTRIN Tout fn(const T1& a, const Args&... b)                                                       \
    {                                                                                                        \
        using vecout = vec1<Tout>;                                                                           \
        return to_scalar(::kfr::intrinsics::fn(vecout(a), vecout(b)...));                                    \
    }

#define KFR_I_FLT_CONVERTER(fn)                                                                              \
    template <typename T1, typename... Args,                                                                 \
              typename Tout = ::kfr::flt_type<::cometa::common_type<T1, Args...>>>                           \
    KFR_SINTRIN Tout fn(const T1& a, const Args&... b)                                                       \
    {                                                                                                        \
        using vecout = vec1<Tout>;                                                                           \
        return to_scalar(::kfr::intrinsics::fn(vecout(a), vecout(b)...));                                    \
    }

namespace intrinsics
{
#ifdef CMT_ARCH_X86
using f32sse = vec<f32, 4>;
using f64sse = vec<f64, 2>;
using i8sse  = vec<i8, 16>;
using i16sse = vec<i16, 8>;
using i32sse = vec<i32, 4>;
using i64sse = vec<i64, 2>;
using u8sse  = vec<u8, 16>;
using u16sse = vec<u16, 8>;
using u32sse = vec<u32, 4>;
using u64sse = vec<u64, 2>;

using mf32sse = mask<f32, 4>;
using mf64sse = mask<f64, 2>;
using mi8sse  = mask<i8, 16>;
using mi16sse = mask<i16, 8>;
using mi32sse = mask<i32, 4>;
using mi64sse = mask<i64, 2>;
using mu8sse  = mask<u8, 16>;
using mu16sse = mask<u16, 8>;
using mu32sse = mask<u32, 4>;
using mu64sse = mask<u64, 2>;

using f32avx = vec<f32, 8>;
using f64avx = vec<f64, 4>;
using i8avx  = vec<i8, 32>;
using i16avx = vec<i16, 16>;
using i32avx = vec<i32, 8>;
using i64avx = vec<i64, 4>;
using u8avx  = vec<u8, 32>;
using u16avx = vec<u16, 16>;
using u32avx = vec<u32, 8>;
using u64avx = vec<u64, 4>;

using mf32avx = mask<f32, 8>;
using mf64avx = mask<f64, 4>;
using mi8avx  = mask<i8, 32>;
using mi16avx = mask<i16, 16>;
using mi32avx = mask<i32, 8>;
using mi64avx = mask<i64, 4>;
using mu8avx  = mask<u8, 32>;
using mu16avx = mask<u16, 16>;
using mu32avx = mask<u32, 8>;
using mu64avx = mask<u64, 4>;
#else
using f32neon = vec<f32, 4>;
using f64neon = vec<f64, 2>;
using i8neon  = vec<i8, 16>;
using i16neon = vec<i16, 8>;
using i32neon = vec<i32, 4>;
using i64neon = vec<i64, 2>;
using u8neon  = vec<u8, 16>;
using u16neon = vec<u16, 8>;
using u32neon = vec<u32, 4>;
using u64neon = vec<u64, 2>;

using mf32neon = mask<f32, 4>;
using mf64neon = mask<f64, 2>;
using mi8neon  = mask<i8, 16>;
using mi16neon = mask<i16, 8>;
using mi32neon = mask<i32, 4>;
using mi64neon = mask<i64, 2>;
using mu8neon  = mask<u8, 16>;
using mu16neon = mask<u16, 8>;
using mu32neon = mask<u32, 4>;
using mu64neon = mask<u64, 2>;
#endif

template <cpu_t c, typename T>
constexpr inline size_t next_simd_width(size_t n)
{
#ifdef CMT_ARCH_X86
    return n > vector_width<T, cpu_t::sse2> ? vector_width<T, c> : vector_width<T, cpu_t::sse2>;
#endif
#ifdef CMT_ARCH_ARM
    return vector_width<T, cpu_t::neon>;
#endif
}

template <typename T, size_t N, size_t Nout = next_simd_width<cpu_t::native, T>(N)>
KFR_SINTRIN vec<T, Nout> expand_simd(const vec<T, N>& x)
{
    return extend<Nout>(x);
}

template <typename T, size_t N, size_t Nout = next_simd_width<cpu_t::native, T>(N)>
KFR_SINTRIN vec<T, Nout> expand_simd(const vec<T, N>& x, identity<T> value)
{
    return widen<Nout>(x, value);
}

#define KFR_HANDLE_ALL_SIZES_1(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a)));                                                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return concat(fn(low(a)), fn(high(a)));                                                              \
    }

#define KFR_HANDLE_ALL_SIZES_FLT_1(fn)                                                                       \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<flt_type<T>, N> fn(const vec<T, N>& a)                                                   \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(cast<flt_type<T>>(a))));                                           \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<flt_type<T>, N> fn(const vec<T, N>& a)                                                   \
    {                                                                                                        \
        return concat(fn(low(cast<flt_type<T>>(a))), fn(high(cast<flt_type<T>>(a))));                        \
    }

#define KFR_HANDLE_ALL_SIZES_F_1(fn)                                                                         \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T, cpu_t::native> && is_f_class<T>::value)>                     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a)));                                                              \
    }                                                                                                        \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native> && is_f_class<T>::value), typename = void>   \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return concat(fn(low(a)), fn(high(a)));                                                              \
    }

#define KFR_HANDLE_ALL_SIZES_I_1(fn)                                                                         \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T, cpu_t::native> && is_i_class<T>::value)>                     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a)));                                                              \
    }                                                                                                        \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native> && is_i_class<T>::value), typename = void>   \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return concat(fn(low(a)), fn(high(a)));                                                              \
    }

#define KFR_HANDLE_ALL_SIZES_U_1(fn)                                                                         \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T, cpu_t::native> && is_u_class<T>::value)>                     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a)));                                                              \
    }                                                                                                        \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native> && is_u_class<T>::value), typename = void>   \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return concat(fn(low(a)), fn(high(a)));                                                              \
    }

#define KFR_HANDLE_ALL_SIZES_NOT_F_1(fn)                                                                     \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T, cpu_t::native> && !is_f_class<T>::value)>                    \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a)));                                                              \
    }                                                                                                        \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native> && !is_f_class<T>::value), typename = void>  \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a)                                                             \
    {                                                                                                        \
        return concat(fn(low(a)), fn(high(a)));                                                              \
    }

#define KFR_HANDLE_ALL_SIZES_2(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b)                                         \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b)));                                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b)                                         \
    {                                                                                                        \
        return concat(fn(low(a), low(b)), fn(high(a), high(b)));                                             \
    }

#define KFR_HANDLE_ALL_SIZES_3(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c)                     \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b), expand_simd(c)));                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c)                     \
    {                                                                                                        \
        return concat(fn(low(a), low(b), low(c)), fn(high(a), high(b), high(c)));                            \
    }

#define KFR_HANDLE_ALL_SIZES_4(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c, const vec<T, N>& d) \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b), expand_simd(c), expand_simd(d)));              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c, const vec<T, N>& d) \
    {                                                                                                        \
        return concat(fn(low(a), low(b), low(c), low(d)), fn(high(a), high(b), high(c), high(d)));           \
    }

template <typename T>
using vec1 = conditional<is_vec<T>::value, T, vec<T, 1>>;

template <typename T>
inline T to_scalar(const T& value)
{
    return value;
}
template <typename T>
inline T to_scalar(const vec<T, 1>& value)
{
    return value[0];
}
}
}
#pragma clang diagnostic pop
