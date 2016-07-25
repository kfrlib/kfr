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

#include "dispatch.hpp"
#include "expression.hpp"
#include "shuffle.hpp"
#include "types.hpp"
#include "vec.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

namespace kfr
{

#define KFR_HANDLE_ALL(fn)                                                                                   \
    template <typename T, size_t N, typename... Args>                                                        \
    KFR_SINTRIN vec<T, N> fn(vec<T, N> x, Args&&... args)                                                    \
    {                                                                                                        \
        return handle_all<cpu, fn_##fn>(x, std::forward<Args>(args)...);                                     \
    }
#define KFR_HANDLE_ALL_REDUCE(redfn, fn)                                                                     \
    template <typename T, size_t N, typename... Args>                                                        \
    KFR_SINTRIN auto fn(vec<T, N> x, Args&&... args)                                                         \
    {                                                                                                        \
        return handle_all_reduce<cpu, redfn, fn_##fn>(x, std::forward<Args>(args)...);                       \
    }

#define KFR_HANDLE_SCALAR(fn)                                                                                \
    template <typename T, typename... Ts, KFR_ENABLE_IF(is_numeric_args<T, Ts...>::value)>                   \
    KFR_SINTRIN auto fn(const T& x, const Ts&... rest)                                                       \
    {                                                                                                        \
        return fn(make_vector(x), make_vector(rest)...)[0];                                                  \
    }

namespace internal
{

struct fn_disabled
{
    constexpr static bool disabled = true;
};

template <cpu_t c, typename T>
constexpr inline size_t next_fast_width(size_t n)
{
    return n > vector_width<T, cpu_t::sse2> ? vector_width<T, c> : vector_width<T, cpu_t::sse2>;
}

template <cpu_t c, typename T, size_t N, size_t Nout = next_fast_width<c, T>(N)>
KFR_INLINE vec<T, Nout> extend_reg(vec<T, N> x)
{
    return extend<Nout>(x);
}
template <cpu_t c, typename T, size_t N, size_t Nout = next_fast_width<c, T>(N)>
KFR_INLINE vec<T, Nout> extend_reg(vec<T, N> x, T value)
{
    return widen<Nout>(x, value);
}

template <cpu_t cur, typename Fn, typename T, size_t N, typename... Args,
          KFR_ENABLE_IF(N < vector_width<T, cur>)>
KFR_INLINE auto handle_all_f(Fn&& fn, vec<T, N> x, Args&&... args)
{
    return narrow<N>(fn(extend_reg<cur>(x), extend_reg<cur>(args)...));
}
template <cpu_t cur, typename Fn, typename T, size_t N, typename... Args,
          KFR_ENABLE_IF(N > vector_width<T, cur>)>
KFR_INLINE auto handle_all_f(Fn&& fn, vec<T, N> x, Args&&... args)
{
    return concat(fn(low(x), low(args)...), fn(high(x), high(args)...));
}

template <cpu_t cur, typename Fn, typename T, size_t N, typename... Args>
KFR_INLINE auto handle_all(vec<T, N> x, Args&&... args)
{
    Fn fn{};
    return handle_all_f<cur>(fn, x, std::forward<Args>(args)...);
}

template <cpu_t cur, typename RedFn, typename Fn, typename T, size_t N, typename... Args,
          typename = u8[N < vector_width<T, cur>]>
KFR_INLINE auto handle_all_reduce_f(RedFn&& redfn, Fn&& fn, vec<T, N> x, Args&&... args)
{
    return fn(extend_reg<cur>(x, redfn(initialvalue<T>())),
              extend_reg<cur>(args, redfn(initialvalue<T>()))...);
}
template <cpu_t cur, typename RedFn, typename Fn, typename T, size_t N, typename... Args,
          typename = u8[N > vector_width<T, cur>], typename = void>
KFR_INLINE auto handle_all_reduce_f(RedFn&& redfn, Fn&& fn, vec<T, N> x, Args&&... args)
{
    return redfn(fn(low(x), low(args)...), fn(high(x), high(args)...));
}
template <cpu_t cur, typename RedFn, typename Fn, typename T, size_t N, typename... Args>
KFR_INLINE auto handle_all_reduce(vec<T, N> x, Args&&... args)
{
    RedFn redfn{};
    Fn fn{};
    return handle_all_reduce_f<cur>(redfn, fn, x, std::forward<Args>(args)...);
}
}

namespace internal
{

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

template <cpu_t c, typename T>
constexpr inline size_t next_simd_width(size_t n)
{
    return n > vector_width<T, cpu_t::sse2> ? vector_width<T, c> : vector_width<T, cpu_t::sse2>;
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
