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
    template <typename T, typename... Ts, KFR_ENABLE_IF(!is_vec<T>::value)>                                  \
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
}
#pragma clang diagnostic pop
