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

#include "expression.hpp"
#include "shuffle.hpp"
#include "types.hpp"
#include "vec.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

namespace kfr
{

#define KFR_HANDLE_SCALAR(fn)                                                                                \
    template <typename T, typename... Ts, KFR_ENABLE_IF(is_numeric_args<T, Ts...>::value)>                   \
    KFR_SINTRIN auto fn(const T& x, const Ts&... rest)                                                       \
    {                                                                                                        \
        return fn(make_vector(x), make_vector(rest)...)[0];                                                  \
    }

namespace internal
{
#ifdef CID_ARCH_X86
using f32sse = vec<f32, 4>;
using f64sse = vec<f64, 2>;
using i8sse  = vec<i8, vector_width<i8, cpu_t::sse2>>;
using i16sse = vec<i16, vector_width<i16, cpu_t::sse2>>;
using i32sse = vec<i32, vector_width<i32, cpu_t::sse2>>;
using i64sse = vec<i64, vector_width<i64, cpu_t::sse2>>;
using u8sse  = vec<u8, vector_width<u8, cpu_t::sse2>>;
using u16sse = vec<u16, vector_width<u16, cpu_t::sse2>>;
using u32sse = vec<u32, vector_width<u32, cpu_t::sse2>>;
using u64sse = vec<u64, vector_width<u64, cpu_t::sse2>>;

using mf32sse = mask<f32, vector_width<f32, cpu_t::sse2>>;
using mf64sse = mask<f64, vector_width<f64, cpu_t::sse2>>;
using mi8sse  = mask<i8, vector_width<i8, cpu_t::sse2>>;
using mi16sse = mask<i16, vector_width<i16, cpu_t::sse2>>;
using mi32sse = mask<i32, vector_width<i32, cpu_t::sse2>>;
using mi64sse = mask<i64, vector_width<i64, cpu_t::sse2>>;
using mu8sse  = mask<u8, vector_width<u8, cpu_t::sse2>>;
using mu16sse = mask<u16, vector_width<u16, cpu_t::sse2>>;
using mu32sse = mask<u32, vector_width<u32, cpu_t::sse2>>;
using mu64sse = mask<u64, vector_width<u64, cpu_t::sse2>>;

using f32avx = vec<f32, 8>;
using f64avx = vec<f64, 4>;
using i8avx  = vec<i8, vector_width<i8, cpu_t::avx2>>;
using i16avx = vec<i16, vector_width<i16, cpu_t::avx2>>;
using i32avx = vec<i32, vector_width<i32, cpu_t::avx2>>;
using i64avx = vec<i64, vector_width<i64, cpu_t::avx2>>;
using u8avx  = vec<u8, vector_width<u8, cpu_t::avx2>>;
using u16avx = vec<u16, vector_width<u16, cpu_t::avx2>>;
using u32avx = vec<u32, vector_width<u32, cpu_t::avx2>>;
using u64avx = vec<u64, vector_width<u64, cpu_t::avx2>>;

using mf32avx = mask<f32, vector_width<f32, cpu_t::avx1>>;
using mf64avx = mask<f64, vector_width<f64, cpu_t::avx1>>;
using mi8avx  = mask<i8, vector_width<i8, cpu_t::avx2>>;
using mi16avx = mask<i16, vector_width<i16, cpu_t::avx2>>;
using mi32avx = mask<i32, vector_width<i32, cpu_t::avx2>>;
using mi64avx = mask<i64, vector_width<i64, cpu_t::avx2>>;
using mu8avx  = mask<u8, vector_width<u8, cpu_t::avx2>>;
using mu16avx = mask<u16, vector_width<u16, cpu_t::avx2>>;
using mu32avx = mask<u32, vector_width<u32, cpu_t::avx2>>;
using mu64avx = mask<u64, vector_width<u64, cpu_t::avx2>>;
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
#ifdef CID_ARCH_X86
    return n > vector_width<T, cpu_t::sse2> ? vector_width<T, c> : vector_width<T, cpu_t::sse2>;
#endif
#ifdef CID_ARCH_ARM
    return vector_width<T, cpu_t::neon>;
#endif
}

template <typename T, size_t N, size_t Nout = next_simd_width<cpu_t::native, T>(N)>
KFR_SINTRIN vec<T, Nout> expand_simd(vec<T, N> x)
{
    return extend<Nout>(x);
}

#define KFR_HANDLE_ALL_SIZES_1(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a)                                                                    \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a)));                                                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a)                                                                    \
    {                                                                                                        \
        return concat(fn(low(a)), fn(high(a)));                                                              \
    }
#define KFR_HANDLE_SCALAR_1(fn)                                                                              \
    template <typename T, KFR_ENABLE_IF(is_numeric<T>::value)>                                               \
    KFR_SINTRIN T fn(T a)                                                                                    \
    {                                                                                                        \
        return fn(make_vector(a))[0];                                                                        \
    }

#define KFR_HANDLE_ALL_SIZES_2(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a, vec<T, N> b)                                                       \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b)));                                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a, vec<T, N> b)                                                       \
    {                                                                                                        \
        return concat(fn(low(a), low(b)), fn(high(a), high(b)));                                             \
    }
#define KFR_HANDLE_SCALAR_2(fn)                                                                              \
    template <typename T, KFR_ENABLE_IF(is_numeric<T>::value)>                                               \
    KFR_SINTRIN T fn(T a, T b)                                                                               \
    {                                                                                                        \
        return fn(make_vector(a), make_vector(b))[0];                                                        \
    }

#define KFR_HANDLE_ALL_SIZES_3(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a, vec<T, N> b, vec<T, N> c)                                          \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b), expand_simd(c)));                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a, vec<T, N> b, vec<T, N> c)                                          \
    {                                                                                                        \
        return concat(fn(low(a), low(b), low(c)), fn(high(a), high(b), high(c)));                            \
    }
#define KFR_HANDLE_SCALAR_3(fn)                                                                              \
    template <typename T, KFR_ENABLE_IF(is_numeric<T>::value)>                                               \
    KFR_SINTRIN T fn(T a, T b, T c)                                                                          \
    {                                                                                                        \
        return fn(make_vector(a), make_vector(b), make_vector(c))[0];                                        \
    }

#define KFR_HANDLE_ALL_SIZES_4(fn)                                                                           \
    template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>                       \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a, vec<T, N> b, vec<T, N> c, vec<T, N> d)                             \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b), expand_simd(c), expand_simd(d)));              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>     \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> a, vec<T, N> b, vec<T, N> c, vec<T, N> d)                             \
    {                                                                                                        \
        return concat(fn(low(a), low(b), low(c), low(d)), fn(high(a), high(b), high(c), high(d)));           \
    }
#define KFR_HANDLE_SCALAR_4(fn)                                                                              \
    template <typename T, KFR_ENABLE_IF(is_numeric<T>::value)>                                               \
    KFR_SINTRIN T fn(T a, T b, T c, T d)                                                                     \
    {                                                                                                        \
        return fn(make_vector(a), make_vector(b), make_vector(c), make_vector(d))[0];                        \
    }
}
}
#pragma clang diagnostic pop
