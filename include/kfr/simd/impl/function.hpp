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

#include "../shuffle.hpp"
#include "../types.hpp"
#include "../vec.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

#define KFR_HANDLE_NOT_F_1(fn)                                                                               \
    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>                                           \
    KFR_INTRINSIC vec<flt_type<T>, N> fn(const vec<T, N>& a) CMT_NOEXCEPT                                    \
    {                                                                                                        \
        return intrinsics::fn(promoteto<flt_type<T>>(a));                                                    \
    }

#define KFR_HANDLE_SCALAR(fn)                                                                                \
    template <typename T1, typename... Args, typename Tout = std::common_type_t<T1, Args...>,                \
              KFR_ENABLE_IF(!(is_vec<T1> || (is_vec<Args> || ...)))>                                         \
    KFR_INTRINSIC Tout fn(const T1& a, const Args&... b) CMT_NOEXCEPT                                        \
    {                                                                                                        \
        using vecout = vec1<Tout>;                                                                           \
        return to_scalar(::kfr::intrinsics::fn(vecout(a), vecout(b)...));                                    \
    }

#define KFR_HANDLE_SCALAR_1_T(fn, Tout)                                                                      \
    template <typename T1, typename... Args, typename T = std::common_type_t<T1, Args...>,                   \
              KFR_ENABLE_IF(!(is_vec<T1> || (is_vec<Args> || ...)))>                                         \
    KFR_INTRINSIC Tout fn(const T1& a, const Args&... b) CMT_NOEXCEPT                                        \
    {                                                                                                        \
        using vecout = vec1<Tout>;                                                                           \
        return to_scalar(::kfr::intrinsics::fn(vecout(a), vecout(b)...));                                    \
    }

#define KFR_HANDLE_ARGS_T(fn, Tout)                                                                          \
    template <typename T1, typename... Args, typename T = std::common_type_t<T1, Args...>,                   \
              KFR_ENABLE_IF((is_vec<T1> || (is_vec<Args> || ...)))>                                          \
    KFR_INTRINSIC Tout fn(const T1& a, const Args&... b) CMT_NOEXCEPT                                        \
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

using f32avx512 = vec<f32, 16>;
using f64avx512 = vec<f64, 8>;
using i8avx512  = vec<i8, 64>;
using i16avx512 = vec<i16, 32>;
using i32avx512 = vec<i32, 16>;
using i64avx512 = vec<i64, 8>;
using u8avx512  = vec<u8, 64>;
using u16avx512 = vec<u16, 32>;
using u32avx512 = vec<u32, 16>;
using u64avx512 = vec<u64, 8>;

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

using mf32avx512 = mask<f32, 16>;
using mf64avx512 = mask<f64, 8>;
using mi8avx512  = mask<i8, 64>;
using mi16avx512 = mask<i16, 32>;
using mi32avx512 = mask<i32, 16>;
using mi64avx512 = mask<i64, 8>;
using mu8avx512  = mask<u8, 64>;
using mu16avx512 = mask<u16, 32>;
using mu32avx512 = mask<u32, 16>;
using mu64avx512 = mask<u64, 8>;

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

template <typename T>
constexpr inline size_t next_simd_width(size_t n) CMT_NOEXCEPT
{
    return n < minimum_vector_width<T> ? minimum_vector_width<T> : next_poweroftwo(n);
}

template <typename T, size_t N, size_t Nout = next_simd_width<T>(N)>
KFR_INTRINSIC vec<T, Nout> expand_simd(const vec<T, 1>& x) CMT_NOEXCEPT
{
    return broadcast<Nout>(x);
}

template <typename T, size_t N, size_t Nout = next_simd_width<T>(N)>
KFR_INTRINSIC vec<T, Nout> expand_simd(const vec<T, N>& x) CMT_NOEXCEPT
{
    return extend<Nout>(x);
}

template <typename T, size_t N, size_t Nout = next_simd_width<T>(N)>
KFR_INTRINSIC vec<T, Nout> expand_simd(const vec<T, N>& x, identity<T> value) CMT_NOEXCEPT
{
    return widen<Nout>(x, value);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N <= Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c,
                          Fn&& fn)
{
    result = fn(a, b, c);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N > Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c,
                          Fn&& fn)
{
    intrin(result.h.low, a.h.low, b.h.low, c.h.low, fn);
    intrin(result.h.high, a.h.high, b.h.high, c.h.high, fn);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N <= Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, Fn&& fn)
{
    result = fn(a);
}

template <typename T, size_t Nvec = vector_width<T>, size_t N, typename Fn, KFR_ENABLE_IF(N > Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, Fn&& fn)
{
    intrin(result.h.low, a.h.low, fn);
    intrin(result.h.high, a.h.high, fn);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N <= Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, const vec<T, N>& b, Fn&& fn)
{
    result = fn(a, b);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N > Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, const vec<T, N>& b, Fn&& fn)
{
    intrin(result.h.low, a.h.low, b.h.low, fn);
    intrin(result.h.high, a.h.high, b.h.high, fn);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N <= Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, const T& b, Fn&& fn)
{
    result = fn(a, b);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N > Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const vec<T, N>& a, const T& b, Fn&& fn)
{
    intrin(result.h.low, a.h.low, b, fn);
    intrin(result.h.high, a.h.high, b, fn);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N <= Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const T& a, const vec<T, N>& b, Fn&& fn)
{
    result = fn(a, b);
}

template <typename T, size_t N, size_t Nvec = vector_width<T>, typename Fn, KFR_ENABLE_IF(N > Nvec)>
KFR_INTRINSIC void intrin(vec<T, N>& result, const T& a, const vec<T, N>& b, Fn&& fn)
{
    intrin(result.h.low, a, b.h.low, fn);
    intrin(result.h.high, a, b.h.high, fn);
}

#define KFR_HANDLE_ALL_SIZES_1_IF(fn, cond)                                                                  \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N) && is_simd_type<T> && cond)>          \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a) CMT_NOEXCEPT                                              \
    {                                                                                                        \
        constexpr size_t Nout = intrinsics::next_simd_width<T>(N);                                           \
        return intrinsics::fn(a.shuffle(csizeseq<Nout>)).shuffle(csizeseq<N>);                               \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T> && is_simd_type<T> && cond),           \
              typename = void>                                                                               \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a) CMT_NOEXCEPT                                              \
    {                                                                                                        \
        vec<T, N> r;                                                                                         \
        intrin(r, a, [](const auto& x) { return intrinsics::fn(x); });                                       \
        return r;                                                                                            \
    }

#define KFR_HANDLE_ALL_SIZES_1(fn) KFR_HANDLE_ALL_SIZES_1_IF(fn, true)

#define KFR_HANDLE_ALL_SIZES_2(fn)                                                                           \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N) && is_simd_type<T>)>                  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b) CMT_NOEXCEPT                          \
    {                                                                                                        \
        constexpr size_t Nout = intrinsics::next_simd_width<T>(N);                                           \
        return intrinsics::fn(a.shuffle(csizeseq_t<Nout>()), b.shuffle(csizeseq_t<Nout>()))                  \
            .shuffle(csizeseq<N>);                                                                           \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T> && is_simd_type<T>), typename = void>  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const vec<T, N>& b) CMT_NOEXCEPT                          \
    {                                                                                                        \
        vec<T, N> r;                                                                                         \
        intrin(r, a, b, [](const auto& aa, const auto& bb) { return intrinsics::fn(aa, bb); });              \
        return r;                                                                                            \
    }                                                                                                        \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N) && is_simd_type<T>)>                  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const T& b) CMT_NOEXCEPT                                  \
    {                                                                                                        \
        constexpr size_t Nout = intrinsics::next_simd_width<T>(N);                                           \
        return intrinsics::fn(a.shuffle(csizeseq_t<Nout>()), vec<T, Nout>(b)).shuffle(csizeseq<N>);          \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T> && is_simd_type<T>), typename = void>  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const T& b) CMT_NOEXCEPT                                  \
    {                                                                                                        \
        vec<T, N> r;                                                                                         \
        intrin(r, a, b, [](const auto& aa, const auto& bb) { return intrinsics::fn(aa, bb); });              \
        return r;                                                                                            \
    }                                                                                                        \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N) && is_simd_type<T>)>                  \
    KFR_INTRINSIC vec<T, N> fn(const T& a, const vec<T, N>& b) CMT_NOEXCEPT                                  \
    {                                                                                                        \
        constexpr size_t Nout = intrinsics::next_simd_width<T>(N);                                           \
        return intrinsics::fn(vec<T, Nout>(a), b.shuffle(csizeseq_t<Nout>())).shuffle(csizeseq<N>);          \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T> && is_simd_type<T>), typename = void>  \
    KFR_INTRINSIC vec<T, N> fn(const T& a, const vec<T, N>& b) CMT_NOEXCEPT                                  \
    {                                                                                                        \
        vec<T, N> r;                                                                                         \
        intrin(r, a, b, [](const auto& aa, const auto& bb) { return intrinsics::fn(aa, bb); });              \
        return r;                                                                                            \
    }

template <typename T>
using vec1 = std::conditional_t<is_vec<T>, T, vec<T, 1>>;

template <typename T>
inline const T& to_scalar(const T& value) CMT_NOEXCEPT
{
    return value;
}
template <typename T>
inline T to_scalar(const vec<T, 1>& value) CMT_NOEXCEPT
{
    return value[0];
}
} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
CMT_PRAGMA_GNU(GCC diagnostic pop)
