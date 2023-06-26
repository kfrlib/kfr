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

#include "../operators.hpp"
#include "../select.hpp"
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

// Generic functions
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> saturated_signed_add(const vec<T, N>& a, const vec<T, N>& b)
{
    using UT               = utype<T>;
    constexpr size_t shift = typebits<UT>::bits - 1;
    vec<UT, N> aa          = bitcast<UT>(a);
    vec<UT, N> bb          = bitcast<UT>(b);
    const vec<UT, N> sum   = aa + bb;
    aa                     = (aa >> shift) + static_cast<UT>(std::numeric_limits<T>::max());

    return select(bitcast<T>((aa ^ bb) | ~(bb ^ sum)) >= T(), a, bitcast<T>(sum));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> saturated_signed_sub(const vec<T, N>& a, const vec<T, N>& b)
{
    using UT               = utype<T>;
    constexpr size_t shift = typebits<UT>::bits - 1;
    vec<UT, N> aa          = bitcast<UT>(a);
    vec<UT, N> bb          = bitcast<UT>(b);
    const vec<UT, N> diff  = aa - bb;
    aa                     = (aa >> shift) + static_cast<UT>(std::numeric_limits<T>::max());

    return select(bitcast<T>((aa ^ bb) & (aa ^ diff)) < T(), a, bitcast<T>(diff));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> saturated_unsigned_add(const vec<T, N>& a, const vec<T, N>& b)
{
    const vec<T, N> t = allonesvector(a);
    return select(a > t - b, t, a + b);
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> saturated_unsigned_sub(const vec<T, N>& a, const vec<T, N>& b)
{
    return select(a < b, zerovector(a), a - b);
}

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC u8sse satadd(const u8sse& x, const u8sse& y) { return _mm_adds_epu8(x.v, y.v); }
KFR_INTRINSIC i8sse satadd(const i8sse& x, const i8sse& y) { return _mm_adds_epi8(x.v, y.v); }
KFR_INTRINSIC u16sse satadd(const u16sse& x, const u16sse& y) { return _mm_adds_epu16(x.v, y.v); }
KFR_INTRINSIC i16sse satadd(const i16sse& x, const i16sse& y) { return _mm_adds_epi16(x.v, y.v); }

KFR_INTRINSIC u8sse satsub(const u8sse& x, const u8sse& y) { return _mm_subs_epu8(x.v, y.v); }
KFR_INTRINSIC i8sse satsub(const i8sse& x, const i8sse& y) { return _mm_subs_epi8(x.v, y.v); }
KFR_INTRINSIC u16sse satsub(const u16sse& x, const u16sse& y) { return _mm_subs_epu16(x.v, y.v); }
KFR_INTRINSIC i16sse satsub(const i16sse& x, const i16sse& y) { return _mm_subs_epi16(x.v, y.v); }

KFR_INTRINSIC i32sse satadd(const i32sse& a, const i32sse& b) { return saturated_signed_add(a, b); }
KFR_INTRINSIC i64sse satadd(const i64sse& a, const i64sse& b) { return saturated_signed_add(a, b); }
KFR_INTRINSIC u32sse satadd(const u32sse& a, const u32sse& b) { return saturated_unsigned_add(a, b); }
KFR_INTRINSIC u64sse satadd(const u64sse& a, const u64sse& b) { return saturated_unsigned_add(a, b); }

KFR_INTRINSIC i32sse satsub(const i32sse& a, const i32sse& b) { return saturated_signed_sub(a, b); }
KFR_INTRINSIC i64sse satsub(const i64sse& a, const i64sse& b) { return saturated_signed_sub(a, b); }
KFR_INTRINSIC u32sse satsub(const u32sse& a, const u32sse& b) { return saturated_unsigned_sub(a, b); }
KFR_INTRINSIC u64sse satsub(const u64sse& a, const u64sse& b) { return saturated_unsigned_sub(a, b); }

#if defined CMT_ARCH_AVX2
KFR_INTRINSIC u8avx satadd(const u8avx& x, const u8avx& y) { return _mm256_adds_epu8(x.v, y.v); }
KFR_INTRINSIC i8avx satadd(const i8avx& x, const i8avx& y) { return _mm256_adds_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx satadd(const u16avx& x, const u16avx& y) { return _mm256_adds_epu16(x.v, y.v); }
KFR_INTRINSIC i16avx satadd(const i16avx& x, const i16avx& y) { return _mm256_adds_epi16(x.v, y.v); }

KFR_INTRINSIC u8avx satsub(const u8avx& x, const u8avx& y) { return _mm256_subs_epu8(x.v, y.v); }
KFR_INTRINSIC i8avx satsub(const i8avx& x, const i8avx& y) { return _mm256_subs_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx satsub(const u16avx& x, const u16avx& y) { return _mm256_subs_epu16(x.v, y.v); }
KFR_INTRINSIC i16avx satsub(const i16avx& x, const i16avx& y) { return _mm256_subs_epi16(x.v, y.v); }

KFR_INTRINSIC i32avx satadd(const i32avx& a, const i32avx& b) { return saturated_signed_add(a, b); }
KFR_INTRINSIC i64avx satadd(const i64avx& a, const i64avx& b) { return saturated_signed_add(a, b); }
KFR_INTRINSIC u32avx satadd(const u32avx& a, const u32avx& b) { return saturated_unsigned_add(a, b); }
KFR_INTRINSIC u64avx satadd(const u64avx& a, const u64avx& b) { return saturated_unsigned_add(a, b); }

KFR_INTRINSIC i32avx satsub(const i32avx& a, const i32avx& b) { return saturated_signed_sub(a, b); }
KFR_INTRINSIC i64avx satsub(const i64avx& a, const i64avx& b) { return saturated_signed_sub(a, b); }
KFR_INTRINSIC u32avx satsub(const u32avx& a, const u32avx& b) { return saturated_unsigned_sub(a, b); }
KFR_INTRINSIC u64avx satsub(const u64avx& a, const u64avx& b) { return saturated_unsigned_sub(a, b); }
#endif

#if defined CMT_ARCH_AVX512
KFR_INTRINSIC u8avx512 satadd(const u8avx512& x, const u8avx512& y) { return _mm512_adds_epu8(x.v, y.v); }
KFR_INTRINSIC i8avx512 satadd(const i8avx512& x, const i8avx512& y) { return _mm512_adds_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 satadd(const u16avx512& x, const u16avx512& y) { return _mm512_adds_epu16(x.v, y.v); }
KFR_INTRINSIC i16avx512 satadd(const i16avx512& x, const i16avx512& y) { return _mm512_adds_epi16(x.v, y.v); }
KFR_INTRINSIC u8avx512 satsub(const u8avx512& x, const u8avx512& y) { return _mm512_subs_epu8(x.v, y.v); }
KFR_INTRINSIC i8avx512 satsub(const i8avx512& x, const i8avx512& y) { return _mm512_subs_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 satsub(const u16avx512& x, const u16avx512& y) { return _mm512_subs_epu16(x.v, y.v); }
KFR_INTRINSIC i16avx512 satsub(const i16avx512& x, const i16avx512& y) { return _mm512_subs_epi16(x.v, y.v); }

KFR_INTRINSIC i32avx512 satadd(const i32avx512& a, const i32avx512& b) { return saturated_signed_add(a, b); }
KFR_INTRINSIC i64avx512 satadd(const i64avx512& a, const i64avx512& b) { return saturated_signed_add(a, b); }
KFR_INTRINSIC u32avx512 satadd(const u32avx512& a, const u32avx512& b)
{
    return saturated_unsigned_add(a, b);
}
KFR_INTRINSIC u64avx512 satadd(const u64avx512& a, const u64avx512& b)
{
    return saturated_unsigned_add(a, b);
}
KFR_INTRINSIC i32avx512 satsub(const i32avx512& a, const i32avx512& b) { return saturated_signed_sub(a, b); }
KFR_INTRINSIC i64avx512 satsub(const i64avx512& a, const i64avx512& b) { return saturated_signed_sub(a, b); }
KFR_INTRINSIC u32avx512 satsub(const u32avx512& a, const u32avx512& b)
{
    return saturated_unsigned_sub(a, b);
}
KFR_INTRINSIC u64avx512 satsub(const u64avx512& a, const u64avx512& b)
{
    return saturated_unsigned_sub(a, b);
}
#endif

KFR_HANDLE_ALL_SIZES_2(satadd)
KFR_HANDLE_ALL_SIZES_2(satsub)

#elif defined CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC u8neon satadd(const u8neon& x, const u8neon& y) { return vqaddq_u8(x.v, y.v); }
KFR_INTRINSIC i8neon satadd(const i8neon& x, const i8neon& y) { return vqaddq_s8(x.v, y.v); }
KFR_INTRINSIC u16neon satadd(const u16neon& x, const u16neon& y) { return vqaddq_u16(x.v, y.v); }
KFR_INTRINSIC i16neon satadd(const i16neon& x, const i16neon& y) { return vqaddq_s16(x.v, y.v); }
KFR_INTRINSIC u32neon satadd(const u32neon& a, const u32neon& b) { return vqaddq_u32(a.v, b.v); }
KFR_INTRINSIC i32neon satadd(const i32neon& a, const i32neon& b) { return vqaddq_s32(a.v, b.v); }
KFR_INTRINSIC u64neon satadd(const u64neon& a, const u64neon& b) { return vqaddq_u64(a.v, b.v); }
KFR_INTRINSIC i64neon satadd(const i64neon& a, const i64neon& b) { return vqaddq_s64(a.v, b.v); }

KFR_INTRINSIC u8neon satsub(const u8neon& x, const u8neon& y) { return vqsubq_u8(x.v, y.v); }
KFR_INTRINSIC i8neon satsub(const i8neon& x, const i8neon& y) { return vqsubq_s8(x.v, y.v); }
KFR_INTRINSIC u16neon satsub(const u16neon& x, const u16neon& y) { return vqsubq_u16(x.v, y.v); }
KFR_INTRINSIC i16neon satsub(const i16neon& x, const i16neon& y) { return vqsubq_s16(x.v, y.v); }
KFR_INTRINSIC u32neon satsub(const u32neon& a, const u32neon& b) { return vqsubq_u32(a.v, b.v); }
KFR_INTRINSIC i32neon satsub(const i32neon& a, const i32neon& b) { return vqsubq_s32(a.v, b.v); }
KFR_INTRINSIC u64neon satsub(const u64neon& a, const u64neon& b) { return vqsubq_u64(a.v, b.v); }
KFR_INTRINSIC i64neon satsub(const i64neon& a, const i64neon& b) { return vqsubq_s64(a.v, b.v); }

KFR_HANDLE_ALL_SIZES_2(satadd)
KFR_HANDLE_ALL_SIZES_2(satsub)

#else
// fallback
template <typename T, size_t N, KFR_ENABLE_IF(std::is_signed_v<T>)>
KFR_INTRINSIC vec<T, N> satadd(const vec<T, N>& a, const vec<T, N>& b)
{
    return saturated_signed_add(a, b);
}
template <typename T, size_t N, KFR_ENABLE_IF(std::is_unsigned_v<T>)>
KFR_INTRINSIC vec<T, N> satadd(const vec<T, N>& a, const vec<T, N>& b)
{
    return saturated_unsigned_add(a, b);
}
template <typename T, size_t N, KFR_ENABLE_IF(std::is_signed_v<T>)>
KFR_INTRINSIC vec<T, N> satsub(const vec<T, N>& a, const vec<T, N>& b)
{
    return saturated_signed_sub(a, b);
}
template <typename T, size_t N, KFR_ENABLE_IF(std::is_unsigned_v<T>)>
KFR_INTRINSIC vec<T, N> satsub(const vec<T, N>& a, const vec<T, N>& b)
{
    return saturated_unsigned_sub(a, b);
}
#endif
KFR_HANDLE_SCALAR(satadd)
KFR_HANDLE_SCALAR(satsub)
} // namespace intrinsics
KFR_I_FN(satadd)
KFR_I_FN(satsub)
} // namespace CMT_ARCH_NAME
} // namespace kfr
