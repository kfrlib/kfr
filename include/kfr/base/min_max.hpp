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

#include "abs.hpp"
#include "function.hpp"
#include "operators.hpp"
#include "select.hpp"

namespace kfr
{

namespace intrinsics
{

#if defined CID_ARCH_SSE2

KFR_SINTRIN f32sse min(f32sse x, f32sse y) { return _mm_min_ps(*x, *y); }
KFR_SINTRIN f64sse min(f64sse x, f64sse y) { return _mm_min_pd(*x, *y); }
KFR_SINTRIN u8sse min(u8sse x, u8sse y) { return _mm_min_epu8(*x, *y); }
KFR_SINTRIN i16sse min(i16sse x, i16sse y) { return _mm_min_epi16(*x, *y); }
KFR_SINTRIN i64sse min(i64sse x, i64sse y) { return select(x < y, x, y); }
KFR_SINTRIN u64sse min(u64sse x, u64sse y) { return select(x < y, x, y); }

KFR_SINTRIN f32sse max(f32sse x, f32sse y) { return _mm_max_ps(*x, *y); }
KFR_SINTRIN f64sse max(f64sse x, f64sse y) { return _mm_max_pd(*x, *y); }
KFR_SINTRIN u8sse max(u8sse x, u8sse y) { return _mm_max_epu8(*x, *y); }
KFR_SINTRIN i16sse max(i16sse x, i16sse y) { return _mm_max_epi16(*x, *y); }
KFR_SINTRIN i64sse max(i64sse x, i64sse y) { return select(x > y, x, y); }
KFR_SINTRIN u64sse max(u64sse x, u64sse y) { return select(x > y, x, y); }

#if defined CID_ARCH_AVX2
KFR_SINTRIN u8avx min(u8avx x, u8avx y) { return _mm256_min_epu8(*x, *y); }
KFR_SINTRIN i16avx min(i16avx x, i16avx y) { return _mm256_min_epi16(*x, *y); }
KFR_SINTRIN i8avx min(i8avx x, i8avx y) { return _mm256_min_epi8(*x, *y); }
KFR_SINTRIN u16avx min(u16avx x, u16avx y) { return _mm256_min_epu16(*x, *y); }
KFR_SINTRIN i32avx min(i32avx x, i32avx y) { return _mm256_min_epi32(*x, *y); }
KFR_SINTRIN u32avx min(u32avx x, u32avx y) { return _mm256_min_epu32(*x, *y); }

KFR_SINTRIN u8avx max(u8avx x, u8avx y) { return _mm256_max_epu8(*x, *y); }
KFR_SINTRIN i16avx max(i16avx x, i16avx y) { return _mm256_max_epi16(*x, *y); }
KFR_SINTRIN i8avx max(i8avx x, i8avx y) { return _mm256_max_epi8(*x, *y); }
KFR_SINTRIN u16avx max(u16avx x, u16avx y) { return _mm256_max_epu16(*x, *y); }
KFR_SINTRIN i32avx max(i32avx x, i32avx y) { return _mm256_max_epi32(*x, *y); }
KFR_SINTRIN u32avx max(u32avx x, u32avx y) { return _mm256_max_epu32(*x, *y); }

KFR_SINTRIN i64avx min(i64avx x, i64avx y) { return select(x < y, x, y); }
KFR_SINTRIN u64avx min(u64avx x, u64avx y) { return select(x < y, x, y); }
KFR_SINTRIN i64avx max(i64avx x, i64avx y) { return select(x > y, x, y); }
KFR_SINTRIN u64avx max(u64avx x, u64avx y) { return select(x > y, x, y); }
#endif

#if defined CID_ARCH_AVX
KFR_SINTRIN f32avx min(f32avx x, f32avx y) { return _mm256_min_ps(*x, *y); }
KFR_SINTRIN f64avx min(f64avx x, f64avx y) { return _mm256_min_pd(*x, *y); }
KFR_SINTRIN f32avx max(f32avx x, f32avx y) { return _mm256_max_ps(*x, *y); }
KFR_SINTRIN f64avx max(f64avx x, f64avx y) { return _mm256_max_pd(*x, *y); }
#endif

#if defined CID_ARCH_SSE41
KFR_SINTRIN i8sse min(i8sse x, i8sse y) { return _mm_min_epi8(*x, *y); }
KFR_SINTRIN u16sse min(u16sse x, u16sse y) { return _mm_min_epu16(*x, *y); }
KFR_SINTRIN i32sse min(i32sse x, i32sse y) { return _mm_min_epi32(*x, *y); }
KFR_SINTRIN u32sse min(u32sse x, u32sse y) { return _mm_min_epu32(*x, *y); }

KFR_SINTRIN i8sse max(i8sse x, i8sse y) { return _mm_max_epi8(*x, *y); }
KFR_SINTRIN u16sse max(u16sse x, u16sse y) { return _mm_max_epu16(*x, *y); }
KFR_SINTRIN i32sse max(i32sse x, i32sse y) { return _mm_max_epi32(*x, *y); }
KFR_SINTRIN u32sse max(u32sse x, u32sse y) { return _mm_max_epu32(*x, *y); }
#else
KFR_SINTRIN i8sse min(i8sse x, i8sse y) { return select(x < y, x, y); }
KFR_SINTRIN u16sse min(u16sse x, u16sse y) { return select(x < y, x, y); }
KFR_SINTRIN i32sse min(i32sse x, i32sse y) { return select(x < y, x, y); }
KFR_SINTRIN u32sse min(u32sse x, u32sse y) { return select(x < y, x, y); }

KFR_SINTRIN i8sse max(i8sse x, i8sse y) { return select(x > y, x, y); }
KFR_SINTRIN u16sse max(u16sse x, u16sse y) { return select(x > y, x, y); }
KFR_SINTRIN i32sse max(i32sse x, i32sse y) { return select(x > y, x, y); }
KFR_SINTRIN u32sse max(u32sse x, u32sse y) { return select(x > y, x, y); }

#endif

KFR_HANDLE_ALL_SIZES_2(min)
KFR_HANDLE_ALL_SIZES_2(max)

#else

// fallback
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> min(vec<T, N> x, vec<T, N> y)
{
    return select(x < y, x, y);
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> max(vec<T, N> x, vec<T, N> y)
{
    return select(x > y, x, y);
}
#endif

template <typename T>
KFR_SINTRIN T min(initialvalue<T>)
{
    return std::numeric_limits<T>::max();
}
template <typename T>
KFR_SINTRIN T max(initialvalue<T>)
{
    return std::numeric_limits<T>::min();
}
template <typename T>
KFR_SINTRIN T absmin(initialvalue<T>)
{
    return std::numeric_limits<T>::max();
}
template <typename T>
KFR_SINTRIN T absmax(initialvalue<T>)
{
    return 0;
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> absmin(vec<T, N> x, vec<T, N> y)
{
    return min(abs(x), abs(y));
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> absmax(vec<T, N> x, vec<T, N> y)
{
    return max(abs(x), abs(y));
}

KFR_I_CONVERTER(min)
KFR_I_CONVERTER(max)
KFR_I_CONVERTER(absmin)
KFR_I_CONVERTER(absmax)
}
KFR_I_FN(min)
KFR_I_FN(max)
KFR_I_FN(absmin)
KFR_I_FN(absmax)

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout min(const T1& x, const T2& y)
{
    return intrinsics::min(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<fn::min, E1, E2> min(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout max(const T1& x, const T2& y)
{
    return intrinsics::max(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<fn::max, E1, E2> max(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout absmin(const T1& x, const T2& y)
{
    return intrinsics::absmin(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<fn::absmin, E1, E2> absmin(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout absmax(const T1& x, const T2& y)
{
    return intrinsics::absmax(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<fn::absmax, E1, E2> absmax(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}
}
