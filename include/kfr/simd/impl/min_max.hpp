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

#include "../abs.hpp"
#include "../operators.hpp"
#include "../select.hpp"
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC f32sse min(const f32sse& x, const f32sse& y) { return _mm_min_ps(x.v, y.v); }
KFR_INTRINSIC f64sse min(const f64sse& x, const f64sse& y) { return _mm_min_pd(x.v, y.v); }
KFR_INTRINSIC u8sse min(const u8sse& x, const u8sse& y) { return _mm_min_epu8(x.v, y.v); }
KFR_INTRINSIC i16sse min(const i16sse& x, const i16sse& y) { return _mm_min_epi16(x.v, y.v); }

KFR_INTRINSIC f32sse max(const f32sse& x, const f32sse& y) { return _mm_max_ps(x.v, y.v); }
KFR_INTRINSIC f64sse max(const f64sse& x, const f64sse& y) { return _mm_max_pd(x.v, y.v); }
KFR_INTRINSIC u8sse max(const u8sse& x, const u8sse& y) { return _mm_max_epu8(x.v, y.v); }
KFR_INTRINSIC i16sse max(const i16sse& x, const i16sse& y) { return _mm_max_epi16(x.v, y.v); }

#if defined CMT_ARCH_AVX2
KFR_INTRINSIC u8avx min(const u8avx& x, const u8avx& y) { return _mm256_min_epu8(x.v, y.v); }
KFR_INTRINSIC i16avx min(const i16avx& x, const i16avx& y) { return _mm256_min_epi16(x.v, y.v); }
KFR_INTRINSIC i8avx min(const i8avx& x, const i8avx& y) { return _mm256_min_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx min(const u16avx& x, const u16avx& y) { return _mm256_min_epu16(x.v, y.v); }
KFR_INTRINSIC i32avx min(const i32avx& x, const i32avx& y) { return _mm256_min_epi32(x.v, y.v); }
KFR_INTRINSIC u32avx min(const u32avx& x, const u32avx& y) { return _mm256_min_epu32(x.v, y.v); }

KFR_INTRINSIC u8avx max(const u8avx& x, const u8avx& y) { return _mm256_max_epu8(x.v, y.v); }
KFR_INTRINSIC i16avx max(const i16avx& x, const i16avx& y) { return _mm256_max_epi16(x.v, y.v); }
KFR_INTRINSIC i8avx max(const i8avx& x, const i8avx& y) { return _mm256_max_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx max(const u16avx& x, const u16avx& y) { return _mm256_max_epu16(x.v, y.v); }
KFR_INTRINSIC i32avx max(const i32avx& x, const i32avx& y) { return _mm256_max_epi32(x.v, y.v); }
KFR_INTRINSIC u32avx max(const u32avx& x, const u32avx& y) { return _mm256_max_epu32(x.v, y.v); }

#endif

#if defined CMT_ARCH_AVX512
KFR_INTRINSIC f32avx512 min(const f32avx512& x, const f32avx512& y) { return _mm512_min_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 min(const f64avx512& x, const f64avx512& y) { return _mm512_min_pd(x.v, y.v); }
KFR_INTRINSIC f32avx512 max(const f32avx512& x, const f32avx512& y) { return _mm512_max_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 max(const f64avx512& x, const f64avx512& y) { return _mm512_max_pd(x.v, y.v); }

KFR_INTRINSIC u8avx512 min(const u8avx512& x, const u8avx512& y) { return _mm512_min_epu8(x.v, y.v); }
KFR_INTRINSIC i16avx512 min(const i16avx512& x, const i16avx512& y) { return _mm512_min_epi16(x.v, y.v); }
KFR_INTRINSIC i8avx512 min(const i8avx512& x, const i8avx512& y) { return _mm512_min_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 min(const u16avx512& x, const u16avx512& y) { return _mm512_min_epu16(x.v, y.v); }
KFR_INTRINSIC i32avx512 min(const i32avx512& x, const i32avx512& y) { return _mm512_min_epi32(x.v, y.v); }
KFR_INTRINSIC u32avx512 min(const u32avx512& x, const u32avx512& y) { return _mm512_min_epu32(x.v, y.v); }
KFR_INTRINSIC u8avx512 max(const u8avx512& x, const u8avx512& y) { return _mm512_max_epu8(x.v, y.v); }
KFR_INTRINSIC i16avx512 max(const i16avx512& x, const i16avx512& y) { return _mm512_max_epi16(x.v, y.v); }
KFR_INTRINSIC i8avx512 max(const i8avx512& x, const i8avx512& y) { return _mm512_max_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 max(const u16avx512& x, const u16avx512& y) { return _mm512_max_epu16(x.v, y.v); }
KFR_INTRINSIC i32avx512 max(const i32avx512& x, const i32avx512& y) { return _mm512_max_epi32(x.v, y.v); }
KFR_INTRINSIC u32avx512 max(const u32avx512& x, const u32avx512& y) { return _mm512_max_epu32(x.v, y.v); }
KFR_INTRINSIC i64avx512 min(const i64avx512& x, const i64avx512& y) { return _mm512_min_epi64(x.v, y.v); }
KFR_INTRINSIC u64avx512 min(const u64avx512& x, const u64avx512& y) { return _mm512_min_epu64(x.v, y.v); }
KFR_INTRINSIC i64avx512 max(const i64avx512& x, const i64avx512& y) { return _mm512_max_epi64(x.v, y.v); }
KFR_INTRINSIC u64avx512 max(const u64avx512& x, const u64avx512& y) { return _mm512_max_epu64(x.v, y.v); }

KFR_INTRINSIC i64avx min(const i64avx& x, const i64avx& y) { return _mm256_min_epi64(x.v, y.v); }
KFR_INTRINSIC u64avx min(const u64avx& x, const u64avx& y) { return _mm256_min_epu64(x.v, y.v); }
KFR_INTRINSIC i64avx max(const i64avx& x, const i64avx& y) { return _mm256_max_epi64(x.v, y.v); }
KFR_INTRINSIC u64avx max(const u64avx& x, const u64avx& y) { return _mm256_max_epu64(x.v, y.v); }

KFR_INTRINSIC i64sse min(const i64sse& x, const i64sse& y) { return _mm_min_epi64(x.v, y.v); }
KFR_INTRINSIC u64sse min(const u64sse& x, const u64sse& y) { return _mm_min_epu64(x.v, y.v); }
KFR_INTRINSIC i64sse max(const i64sse& x, const i64sse& y) { return _mm_max_epi64(x.v, y.v); }
KFR_INTRINSIC u64sse max(const u64sse& x, const u64sse& y) { return _mm_max_epu64(x.v, y.v); }
#else
KFR_INTRINSIC i64sse min(const i64sse& x, const i64sse& y) { return select(x < y, x, y); }
KFR_INTRINSIC u64sse min(const u64sse& x, const u64sse& y) { return select(x < y, x, y); }
KFR_INTRINSIC i64sse max(const i64sse& x, const i64sse& y) { return select(x > y, x, y); }
KFR_INTRINSIC u64sse max(const u64sse& x, const u64sse& y) { return select(x > y, x, y); }
KFR_INTRINSIC i64avx min(const i64avx& x, const i64avx& y) { return select(x < y, x, y); }
KFR_INTRINSIC u64avx min(const u64avx& x, const u64avx& y) { return select(x < y, x, y); }
KFR_INTRINSIC i64avx max(const i64avx& x, const i64avx& y) { return select(x > y, x, y); }
KFR_INTRINSIC u64avx max(const u64avx& x, const u64avx& y) { return select(x > y, x, y); }
#endif

#if defined CMT_ARCH_AVX
KFR_INTRINSIC f32avx min(const f32avx& x, const f32avx& y) { return _mm256_min_ps(x.v, y.v); }
KFR_INTRINSIC f64avx min(const f64avx& x, const f64avx& y) { return _mm256_min_pd(x.v, y.v); }
KFR_INTRINSIC f32avx max(const f32avx& x, const f32avx& y) { return _mm256_max_ps(x.v, y.v); }
KFR_INTRINSIC f64avx max(const f64avx& x, const f64avx& y) { return _mm256_max_pd(x.v, y.v); }
#endif

#if defined CMT_ARCH_SSE41
KFR_INTRINSIC i8sse min(const i8sse& x, const i8sse& y) { return _mm_min_epi8(x.v, y.v); }
KFR_INTRINSIC u16sse min(const u16sse& x, const u16sse& y) { return _mm_min_epu16(x.v, y.v); }
KFR_INTRINSIC i32sse min(const i32sse& x, const i32sse& y) { return _mm_min_epi32(x.v, y.v); }
KFR_INTRINSIC u32sse min(const u32sse& x, const u32sse& y) { return _mm_min_epu32(x.v, y.v); }

KFR_INTRINSIC i8sse max(const i8sse& x, const i8sse& y) { return _mm_max_epi8(x.v, y.v); }
KFR_INTRINSIC u16sse max(const u16sse& x, const u16sse& y) { return _mm_max_epu16(x.v, y.v); }
KFR_INTRINSIC i32sse max(const i32sse& x, const i32sse& y) { return _mm_max_epi32(x.v, y.v); }
KFR_INTRINSIC u32sse max(const u32sse& x, const u32sse& y) { return _mm_max_epu32(x.v, y.v); }
#else
KFR_INTRINSIC i8sse min(const i8sse& x, const i8sse& y) { return select(x < y, x, y); }
KFR_INTRINSIC u16sse min(const u16sse& x, const u16sse& y) { return select(x < y, x, y); }
KFR_INTRINSIC i32sse min(const i32sse& x, const i32sse& y) { return select(x < y, x, y); }
KFR_INTRINSIC u32sse min(const u32sse& x, const u32sse& y) { return select(x < y, x, y); }

KFR_INTRINSIC i8sse max(const i8sse& x, const i8sse& y) { return select(x > y, x, y); }
KFR_INTRINSIC u16sse max(const u16sse& x, const u16sse& y) { return select(x > y, x, y); }
KFR_INTRINSIC i32sse max(const i32sse& x, const i32sse& y) { return select(x > y, x, y); }
KFR_INTRINSIC u32sse max(const u32sse& x, const u32sse& y) { return select(x > y, x, y); }

#endif

KFR_HANDLE_ALL_SIZES_2(min)
KFR_HANDLE_ALL_SIZES_2(max)

#elif defined CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC i8neon min(const i8neon& x, const i8neon& y) { return vminq_s8(x.v, y.v); }
KFR_INTRINSIC u8neon min(const u8neon& x, const u8neon& y) { return vminq_u8(x.v, y.v); }
KFR_INTRINSIC i16neon min(const i16neon& x, const i16neon& y) { return vminq_s16(x.v, y.v); }
KFR_INTRINSIC u16neon min(const u16neon& x, const u16neon& y) { return vminq_u16(x.v, y.v); }
KFR_INTRINSIC i32neon min(const i32neon& x, const i32neon& y) { return vminq_s32(x.v, y.v); }
KFR_INTRINSIC u32neon min(const u32neon& x, const u32neon& y) { return vminq_u32(x.v, y.v); }
KFR_INTRINSIC i64neon min(const i64neon& x, const i64neon& y) { return select(x < y, x, y); }
KFR_INTRINSIC u64neon min(const u64neon& x, const u64neon& y) { return select(x < y, x, y); }

KFR_INTRINSIC i8neon max(const i8neon& x, const i8neon& y) { return vmaxq_s8(x.v, y.v); }
KFR_INTRINSIC u8neon max(const u8neon& x, const u8neon& y) { return vmaxq_u8(x.v, y.v); }
KFR_INTRINSIC i16neon max(const i16neon& x, const i16neon& y) { return vmaxq_s16(x.v, y.v); }
KFR_INTRINSIC u16neon max(const u16neon& x, const u16neon& y) { return vmaxq_u16(x.v, y.v); }
KFR_INTRINSIC i32neon max(const i32neon& x, const i32neon& y) { return vmaxq_s32(x.v, y.v); }
KFR_INTRINSIC u32neon max(const u32neon& x, const u32neon& y) { return vmaxq_u32(x.v, y.v); }
KFR_INTRINSIC i64neon max(const i64neon& x, const i64neon& y) { return select(x > y, x, y); }
KFR_INTRINSIC u64neon max(const u64neon& x, const u64neon& y) { return select(x > y, x, y); }

KFR_INTRINSIC f32neon min(const f32neon& x, const f32neon& y) { return vminq_f32(x.v, y.v); }
KFR_INTRINSIC f32neon max(const f32neon& x, const f32neon& y) { return vmaxq_f32(x.v, y.v); }
#if defined CMT_ARCH_NEON64
KFR_INTRINSIC f64neon min(const f64neon& x, const f64neon& y) { return vminq_f64(x.v, y.v); }
KFR_INTRINSIC f64neon max(const f64neon& x, const f64neon& y) { return vmaxq_f64(x.v, y.v); }
#else
KFR_INTRINSIC f64neon min(const f64neon& x, const f64neon& y) { return select(x < y, x, y); }
KFR_INTRINSIC f64neon max(const f64neon& x, const f64neon& y) { return select(x > y, x, y); }
#endif

KFR_HANDLE_ALL_SIZES_2(min)
KFR_HANDLE_ALL_SIZES_2(max)

#else

// fallback
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> min(const vec<T, N>& x, const vec<T, N>& y)
{
    return select(x < y, x, y);
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> max(const vec<T, N>& x, const vec<T, N>& y)
{
    return select(x > y, x, y);
}
#endif

template <typename T>
KFR_INTRINSIC T min(initialvalue<T>)
{
    return std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity()
                                                : std::numeric_limits<T>::max();
}
template <typename T>
KFR_INTRINSIC T max(initialvalue<T>)
{
    return std::numeric_limits<T>::has_infinity ? -std::numeric_limits<T>::infinity()
                                                : std::numeric_limits<T>::min();
}
template <typename T>
KFR_INTRINSIC T absmin(initialvalue<T>)
{
    return std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity()
                                                : std::numeric_limits<T>::max();
}
template <typename T>
KFR_INTRINSIC T absmax(initialvalue<T>)
{
    return 0;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> absmin(const vec<T, N>& x, const vec<T, N>& y)
{
    return min(abs(x), abs(y));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> absmax(const vec<T, N>& x, const vec<T, N>& y)
{
    return max(abs(x), abs(y));
}

KFR_HANDLE_SCALAR(min)
KFR_HANDLE_SCALAR(max)
KFR_HANDLE_SCALAR(absmin)
KFR_HANDLE_SCALAR(absmax)
} // namespace intrinsics
KFR_I_FN(min)
KFR_I_FN(max)
KFR_I_FN(absmin)
KFR_I_FN(absmax)
} // namespace CMT_ARCH_NAME
} // namespace kfr
