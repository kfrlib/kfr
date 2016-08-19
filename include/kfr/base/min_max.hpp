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

#include "abs.hpp"
#include "function.hpp"
#include "operators.hpp"
#include "select.hpp"

namespace kfr
{

namespace intrinsics
{

#if defined CMT_ARCH_SSE2

KFR_SINTRIN f32sse min(const f32sse& x, const f32sse& y) { return _mm_min_ps(*x, *y); }
KFR_SINTRIN f64sse min(const f64sse& x, const f64sse& y) { return _mm_min_pd(*x, *y); }
KFR_SINTRIN u8sse min(const u8sse& x, const u8sse& y) { return _mm_min_epu8(*x, *y); }
KFR_SINTRIN i16sse min(const i16sse& x, const i16sse& y) { return _mm_min_epi16(*x, *y); }
KFR_SINTRIN i64sse min(const i64sse& x, const i64sse& y) { return select(x < y, x, y); }
KFR_SINTRIN u64sse min(const u64sse& x, const u64sse& y) { return select(x < y, x, y); }

KFR_SINTRIN f32sse max(const f32sse& x, const f32sse& y) { return _mm_max_ps(*x, *y); }
KFR_SINTRIN f64sse max(const f64sse& x, const f64sse& y) { return _mm_max_pd(*x, *y); }
KFR_SINTRIN u8sse max(const u8sse& x, const u8sse& y) { return _mm_max_epu8(*x, *y); }
KFR_SINTRIN i16sse max(const i16sse& x, const i16sse& y) { return _mm_max_epi16(*x, *y); }
KFR_SINTRIN i64sse max(const i64sse& x, const i64sse& y) { return select(x > y, x, y); }
KFR_SINTRIN u64sse max(const u64sse& x, const u64sse& y) { return select(x > y, x, y); }

#if defined CMT_ARCH_AVX2
KFR_SINTRIN u8avx min(const u8avx& x, const u8avx& y) { return _mm256_min_epu8(*x, *y); }
KFR_SINTRIN i16avx min(const i16avx& x, const i16avx& y) { return _mm256_min_epi16(*x, *y); }
KFR_SINTRIN i8avx min(const i8avx& x, const i8avx& y) { return _mm256_min_epi8(*x, *y); }
KFR_SINTRIN u16avx min(const u16avx& x, const u16avx& y) { return _mm256_min_epu16(*x, *y); }
KFR_SINTRIN i32avx min(const i32avx& x, const i32avx& y) { return _mm256_min_epi32(*x, *y); }
KFR_SINTRIN u32avx min(const u32avx& x, const u32avx& y) { return _mm256_min_epu32(*x, *y); }

KFR_SINTRIN u8avx max(const u8avx& x, const u8avx& y) { return _mm256_max_epu8(*x, *y); }
KFR_SINTRIN i16avx max(const i16avx& x, const i16avx& y) { return _mm256_max_epi16(*x, *y); }
KFR_SINTRIN i8avx max(const i8avx& x, const i8avx& y) { return _mm256_max_epi8(*x, *y); }
KFR_SINTRIN u16avx max(const u16avx& x, const u16avx& y) { return _mm256_max_epu16(*x, *y); }
KFR_SINTRIN i32avx max(const i32avx& x, const i32avx& y) { return _mm256_max_epi32(*x, *y); }
KFR_SINTRIN u32avx max(const u32avx& x, const u32avx& y) { return _mm256_max_epu32(*x, *y); }

KFR_SINTRIN i64avx min(const i64avx& x, const i64avx& y) { return select(x < y, x, y); }
KFR_SINTRIN u64avx min(const u64avx& x, const u64avx& y) { return select(x < y, x, y); }
KFR_SINTRIN i64avx max(const i64avx& x, const i64avx& y) { return select(x > y, x, y); }
KFR_SINTRIN u64avx max(const u64avx& x, const u64avx& y) { return select(x > y, x, y); }
#endif

#if defined CMT_ARCH_AVX
KFR_SINTRIN f32avx min(const f32avx& x, const f32avx& y) { return _mm256_min_ps(*x, *y); }
KFR_SINTRIN f64avx min(const f64avx& x, const f64avx& y) { return _mm256_min_pd(*x, *y); }
KFR_SINTRIN f32avx max(const f32avx& x, const f32avx& y) { return _mm256_max_ps(*x, *y); }
KFR_SINTRIN f64avx max(const f64avx& x, const f64avx& y) { return _mm256_max_pd(*x, *y); }
#endif

#if defined CMT_ARCH_SSE41
KFR_SINTRIN i8sse min(const i8sse& x, const i8sse& y) { return _mm_min_epi8(*x, *y); }
KFR_SINTRIN u16sse min(const u16sse& x, const u16sse& y) { return _mm_min_epu16(*x, *y); }
KFR_SINTRIN i32sse min(const i32sse& x, const i32sse& y) { return _mm_min_epi32(*x, *y); }
KFR_SINTRIN u32sse min(const u32sse& x, const u32sse& y) { return _mm_min_epu32(*x, *y); }

KFR_SINTRIN i8sse max(const i8sse& x, const i8sse& y) { return _mm_max_epi8(*x, *y); }
KFR_SINTRIN u16sse max(const u16sse& x, const u16sse& y) { return _mm_max_epu16(*x, *y); }
KFR_SINTRIN i32sse max(const i32sse& x, const i32sse& y) { return _mm_max_epi32(*x, *y); }
KFR_SINTRIN u32sse max(const u32sse& x, const u32sse& y) { return _mm_max_epu32(*x, *y); }
#else
KFR_SINTRIN i8sse min(const i8sse& x, const i8sse& y) { return select(x < y, x, y); }
KFR_SINTRIN u16sse min(const u16sse& x, const u16sse& y) { return select(x < y, x, y); }
KFR_SINTRIN i32sse min(const i32sse& x, const i32sse& y) { return select(x < y, x, y); }
KFR_SINTRIN u32sse min(const u32sse& x, const u32sse& y) { return select(x < y, x, y); }

KFR_SINTRIN i8sse max(const i8sse& x, const i8sse& y) { return select(x > y, x, y); }
KFR_SINTRIN u16sse max(const u16sse& x, const u16sse& y) { return select(x > y, x, y); }
KFR_SINTRIN i32sse max(const i32sse& x, const i32sse& y) { return select(x > y, x, y); }
KFR_SINTRIN u32sse max(const u32sse& x, const u32sse& y) { return select(x > y, x, y); }

#endif

KFR_HANDLE_ALL_SIZES_2(min)
KFR_HANDLE_ALL_SIZES_2(max)

#elif defined CMT_ARCH_NEON

KFR_SINTRIN i8neon min(const i8neon& x, const i8neon& y) { return vminq_s8(*x, *y); }
KFR_SINTRIN u8neon min(const u8neon& x, const u8neon& y) { return vminq_u8(*x, *y); }
KFR_SINTRIN i16neon min(const i16neon& x, const i16neon& y) { return vminq_s16(*x, *y); }
KFR_SINTRIN u16neon min(const u16neon& x, const u16neon& y) { return vminq_u16(*x, *y); }
KFR_SINTRIN i32neon min(const i32neon& x, const i32neon& y) { return vminq_s32(*x, *y); }
KFR_SINTRIN u32neon min(const u32neon& x, const u32neon& y) { return vminq_u32(*x, *y); }
KFR_SINTRIN i64neon min(const i64neon& x, const i64neon& y) { return select(x < y, x, y); }
KFR_SINTRIN u64neon min(const u64neon& x, const u64neon& y) { return select(x < y, x, y); }

KFR_SINTRIN i8neon max(const i8neon& x, const i8neon& y) { return vmaxq_s8(*x, *y); }
KFR_SINTRIN u8neon max(const u8neon& x, const u8neon& y) { return vmaxq_u8(*x, *y); }
KFR_SINTRIN i16neon max(const i16neon& x, const i16neon& y) { return vmaxq_s16(*x, *y); }
KFR_SINTRIN u16neon max(const u16neon& x, const u16neon& y) { return vmaxq_u16(*x, *y); }
KFR_SINTRIN i32neon max(const i32neon& x, const i32neon& y) { return vmaxq_s32(*x, *y); }
KFR_SINTRIN u32neon max(const u32neon& x, const u32neon& y) { return vmaxq_u32(*x, *y); }
KFR_SINTRIN i64neon max(const i64neon& x, const i64neon& y) { return select(x > y, x, y); }
KFR_SINTRIN u64neon max(const u64neon& x, const u64neon& y) { return select(x > y, x, y); }

KFR_SINTRIN f32neon min(const f32neon& x, const f32neon& y) { return vminq_f32(*x, *y); }
KFR_SINTRIN f32neon max(const f32neon& x, const f32neon& y) { return vmaxq_f32(*x, *y); }
#if defined CMT_ARCH_NEON64
KFR_SINTRIN f64neon min(const f64neon& x, const f64neon& y) { return vminq_f64(*x, *y); }
KFR_SINTRIN f64neon max(const f64neon& x, const f64neon& y) { return vmaxq_f64(*x, *y); }
#else
KFR_SINTRIN f64neon min(const f64neon& x, const f64neon& y) { return select(x < y, x, y); }
KFR_SINTRIN f64neon max(const f64neon& x, const f64neon& y) { return select(x > y, x, y); }
#endif

KFR_HANDLE_ALL_SIZES_2(min)
KFR_HANDLE_ALL_SIZES_2(max)

#else

// fallback
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> min(const vec<T, N>& x, const vec<T, N>& y)
{
    return select(x < y, x, y);
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> max(const vec<T, N>& x, const vec<T, N>& y)
{
    return select(x > y, x, y);
}
#endif

template <typename T>
KFR_SINTRIN T min(initialvalue<T>)
{
    return std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity()
                                                : std::numeric_limits<T>::max();
}
template <typename T>
KFR_SINTRIN T max(initialvalue<T>)
{
    return std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity()
                                                : std::numeric_limits<T>::min();
}
template <typename T>
KFR_SINTRIN T absmin(initialvalue<T>)
{
    return std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity()
                                                : std::numeric_limits<T>::max();
}
template <typename T>
KFR_SINTRIN T absmax(initialvalue<T>)
{
    return 0;
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> absmin(const vec<T, N>& x, const vec<T, N>& y)
{
    return min(abs(x), abs(y));
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> absmax(const vec<T, N>& x, const vec<T, N>& y)
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

/**
 * @brief Returns the smaller of two values.
 */
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout min(const T1& x, const T2& y)
{
    return intrinsics::min(x, y);
}

/**
 * @brief Returns template expression that returns the smaller of two values.
 */
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN internal::expression_function<fn::min, E1, E2> min(E1&& x, E2&& y)
{
    return { fn::min(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the greater of two values.
 */
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout max(const T1& x, const T2& y)
{
    return intrinsics::max(x, y);
}

/**
 * @brief Returns template expression that returns the greater of two values.
 */
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN internal::expression_function<fn::max, E1, E2> max(E1&& x, E2&& y)
{
    return { fn::max(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the smaller in magnitude of two values.
 */
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout absmin(const T1& x, const T2& y)
{
    return intrinsics::absmin(x, y);
}

/**
 * @brief Returns template expression that returns the smaller in magnitude of two values.
 */
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN internal::expression_function<fn::absmin, E1, E2> absmin(E1&& x, E2&& y)
{
    return { fn::absmin(), std::forward<E1>(x), std::forward<E2>(y) };
}

/**
 * @brief Returns the greater in magnitude of two values.
 */
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value),
          typename Tout = common_type<T1, T2>>
KFR_INTRIN Tout absmax(const T1& x, const T2& y)
{
    return intrinsics::absmax(x, y);
}

/**
 * @brief Returns template expression that returns the greater in magnitude of two values.
 */
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN internal::expression_function<fn::absmax, E1, E2> absmax(E1&& x, E2&& y)
{
    return { fn::absmax(), std::forward<E1>(x), std::forward<E2>(y) };
}
}
