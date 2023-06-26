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

#include "../mask.hpp"
#include "function.hpp"
#include <algorithm>
#include <utility>

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4700))
CMT_PRAGMA_MSVC(warning(disable : 4309))

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#define KFR_DIV_MOD_FN(ty)                                                                                   \
    KFR_INTRINSIC ty div(const ty& x, const ty& y)                                                           \
    {                                                                                                        \
        KFR_COMPONENTWISE_RET_I(ty, result[i] = y[i] ? x[i] / y[i] : 0);                                     \
    }                                                                                                        \
    KFR_INTRINSIC ty mod(const ty& x, const ty& y)                                                           \
    {                                                                                                        \
        KFR_COMPONENTWISE_RET_I(ty, result[i] = y[i] ? x[i] % y[i] : 0);                                     \
    }

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC __m128 _mm_allones_ps()
{
    return _mm_castsi128_ps(_mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128()));
}

KFR_INTRINSIC __m128d _mm_allones_pd()
{
    return _mm_castsi128_pd(_mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128()));
}

KFR_INTRINSIC __m128i _mm_allones_si128() { return _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128()); }

KFR_INTRINSIC __m128 _mm_not_ps(const __m128& x) { return _mm_xor_ps(x, _mm_allones_ps()); }

KFR_INTRINSIC __m128d _mm_not_pd(const __m128d& x) { return _mm_xor_pd(x, _mm_allones_pd()); }

KFR_INTRINSIC __m128i _mm_not_si128(const __m128i& x) { return _mm_xor_si128(x, _mm_allones_si128()); }

KFR_INTRINSIC __m128i _mm_highbit_epi8() { return _mm_set1_epi8(static_cast<char>(0x80)); }
KFR_INTRINSIC __m128i _mm_highbit_epi16() { return _mm_set1_epi16(static_cast<short>(0x8000)); }
KFR_INTRINSIC __m128i _mm_highbit_epi32() { return _mm_set1_epi32(static_cast<int>(0x80000000)); }
KFR_INTRINSIC __m128i _mm_highbit_epi64() { return _mm_set1_epi64x(0x8000000000000000ll); }

KFR_INTRINSIC f32sse add(const f32sse& x, const f32sse& y) { return f32sse(_mm_add_ps(x.v, y.v)); }
KFR_INTRINSIC f32sse sub(const f32sse& x, const f32sse& y) { return f32sse(_mm_sub_ps(x.v, y.v)); }
KFR_INTRINSIC f32sse mul(const f32sse& x, const f32sse& y) { return f32sse(_mm_mul_ps(x.v, y.v)); }
KFR_INTRINSIC f32sse div(const f32sse& x, const f32sse& y) { return f32sse(_mm_div_ps(x.v, y.v)); }

KFR_INTRINSIC f64sse add(const f64sse& x, const f64sse& y) { return f64sse(_mm_add_pd(x.v, y.v)); }
KFR_INTRINSIC f64sse sub(const f64sse& x, const f64sse& y) { return f64sse(_mm_sub_pd(x.v, y.v)); }
KFR_INTRINSIC f64sse mul(const f64sse& x, const f64sse& y) { return f64sse(_mm_mul_pd(x.v, y.v)); }
KFR_INTRINSIC f64sse div(const f64sse& x, const f64sse& y) { return f64sse(_mm_div_pd(x.v, y.v)); }

KFR_INTRINSIC u8sse add(const u8sse& x, const u8sse& y) { return _mm_add_epi8(x.v, y.v); }
KFR_INTRINSIC u8sse sub(const u8sse& x, const u8sse& y) { return _mm_sub_epi8(x.v, y.v); }
KFR_DIV_MOD_FN(u8sse)

KFR_INTRINSIC i8sse add(const i8sse& x, const i8sse& y) { return _mm_add_epi8(x.v, y.v); }
KFR_INTRINSIC i8sse sub(const i8sse& x, const i8sse& y) { return _mm_sub_epi8(x.v, y.v); }
KFR_DIV_MOD_FN(i8sse)

KFR_INTRINSIC __m128i mul_epi8(const __m128i& x, const __m128i& y)
{
    const __m128i even = _mm_mullo_epi16(x, y);
    const __m128i odd  = _mm_mullo_epi16(_mm_srli_epi16(x, 8), _mm_srli_epi16(y, 8));
    return _mm_or_si128(_mm_slli_epi16(odd, 8), _mm_srli_epi16(_mm_slli_epi16(even, 8), 8));
}

KFR_INTRINSIC u8sse mul(const u8sse& x, const u8sse& y) { return mul_epi8(x.v, y.v); }

KFR_INTRINSIC i8sse mul(const i8sse& x, const i8sse& y) { return mul_epi8(x.v, y.v); }

KFR_INTRINSIC u16sse add(const u16sse& x, const u16sse& y) { return _mm_add_epi16(x.v, y.v); }
KFR_INTRINSIC u16sse sub(const u16sse& x, const u16sse& y) { return _mm_sub_epi16(x.v, y.v); }
KFR_INTRINSIC u16sse mul(const u16sse& x, const u16sse& y) { return _mm_mullo_epi16(x.v, y.v); }
KFR_DIV_MOD_FN(u16sse)

KFR_INTRINSIC i16sse add(const i16sse& x, const i16sse& y) { return _mm_add_epi16(x.v, y.v); }
KFR_INTRINSIC i16sse sub(const i16sse& x, const i16sse& y) { return _mm_sub_epi16(x.v, y.v); }
KFR_INTRINSIC i16sse mul(const i16sse& x, const i16sse& y) { return _mm_mullo_epi16(x.v, y.v); }
KFR_DIV_MOD_FN(i16sse)

KFR_INTRINSIC u32sse add(const u32sse& x, const u32sse& y) { return _mm_add_epi32(x.v, y.v); }
KFR_INTRINSIC u32sse sub(const u32sse& x, const u32sse& y) { return _mm_sub_epi32(x.v, y.v); }

KFR_INTRINSIC i32sse add(const i32sse& x, const i32sse& y) { return _mm_add_epi32(x.v, y.v); }
KFR_INTRINSIC i32sse sub(const i32sse& x, const i32sse& y) { return _mm_sub_epi32(x.v, y.v); }

#if defined CMT_ARCH_SSE41
KFR_INTRINSIC u32sse mul(const u32sse& x, const u32sse& y) { return _mm_mullo_epi32(x.v, y.v); }
KFR_INTRINSIC i32sse mul(const i32sse& x, const i32sse& y) { return _mm_mullo_epi32(x.v, y.v); }
#else
KFR_INTRINSIC u32sse mul(const u32sse& x, const u32sse& y)
{
    __m128i tmp1 = _mm_mul_epu32(x.v, y.v);
    __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(x.v, 4), _mm_srli_si128(y.v, 4));
    return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)),
                              _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
}
KFR_INTRINSIC i32sse mul(const i32sse& x, const i32sse& y)
{
    __m128i tmp1 = _mm_mul_epu32(x.v, y.v);
    __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(x.v, 4), _mm_srli_si128(y.v, 4));
    return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)),
                              _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
}
#endif
KFR_DIV_MOD_FN(u32sse)
KFR_DIV_MOD_FN(i32sse)

KFR_INTRINSIC u64sse add(const u64sse& x, const u64sse& y) { return _mm_add_epi64(x.v, y.v); }
KFR_INTRINSIC u64sse sub(const u64sse& x, const u64sse& y) { return _mm_sub_epi64(x.v, y.v); }
KFR_INTRINSIC u64sse mul(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = x[i] * y[i]);
}

KFR_INTRINSIC i64sse add(const i64sse& x, const i64sse& y) { return _mm_add_epi64(x.v, y.v); }
KFR_INTRINSIC i64sse sub(const i64sse& x, const i64sse& y) { return _mm_sub_epi64(x.v, y.v); }
KFR_INTRINSIC i64sse mul(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = x[i] * y[i]);
}
KFR_DIV_MOD_FN(u64sse)
KFR_DIV_MOD_FN(i64sse)

KFR_INTRINSIC f32sse shl(const f32sse& x, unsigned y)
{
    return _mm_castsi128_ps(_mm_slli_epi32(_mm_castps_si128(x.v), y));
}
KFR_INTRINSIC f64sse shl(const f64sse& x, unsigned y)
{
    return _mm_castsi128_pd(_mm_slli_epi64(_mm_castpd_si128(x.v), y));
}
KFR_INTRINSIC f32sse shr(const f32sse& x, unsigned y)
{
    return _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(x.v), y));
}
KFR_INTRINSIC f64sse shr(const f64sse& x, unsigned y)
{
    return _mm_castsi128_pd(_mm_srli_epi64(_mm_castpd_si128(x.v), y));
}

KFR_INTRINSIC u16sse shl(const u16sse& x, unsigned y) { return _mm_slli_epi16(x.v, y); }
KFR_INTRINSIC u32sse shl(const u32sse& x, unsigned y) { return _mm_slli_epi32(x.v, y); }
KFR_INTRINSIC u64sse shl(const u64sse& x, unsigned y) { return _mm_slli_epi64(x.v, y); }
KFR_INTRINSIC i16sse shl(const i16sse& x, unsigned y) { return _mm_slli_epi16(x.v, y); }
KFR_INTRINSIC i32sse shl(const i32sse& x, unsigned y) { return _mm_slli_epi32(x.v, y); }
KFR_INTRINSIC i64sse shl(const i64sse& x, unsigned y) { return _mm_slli_epi64(x.v, y); }

KFR_INTRINSIC u16sse shr(const u16sse& x, unsigned y) { return _mm_srli_epi16(x.v, y); }
KFR_INTRINSIC u32sse shr(const u32sse& x, unsigned y) { return _mm_srli_epi32(x.v, y); }
KFR_INTRINSIC u64sse shr(const u64sse& x, unsigned y) { return _mm_srli_epi64(x.v, y); }
KFR_INTRINSIC i16sse shr(const i16sse& x, unsigned y) { return _mm_srai_epi16(x.v, y); }
KFR_INTRINSIC i32sse shr(const i32sse& x, unsigned y) { return _mm_srai_epi32(x.v, y); }

KFR_INTRINSIC u8sse shl(const u8sse& x, unsigned y)
{
    __m128i l = _mm_unpacklo_epi8(_mm_setzero_si128(), x.v);
    __m128i h = _mm_unpackhi_epi8(_mm_setzero_si128(), x.v);

    __m128i ll = _mm_slli_epi16(l, y);
    __m128i hh = _mm_slli_epi16(h, y);

    return _mm_packs_epi16(ll, hh);
}
KFR_INTRINSIC i8sse shl(const i8sse& x, unsigned y)
{
    __m128i l = _mm_unpacklo_epi8(_mm_setzero_si128(), x.v);
    __m128i h = _mm_unpackhi_epi8(_mm_setzero_si128(), x.v);

    __m128i ll = _mm_slli_epi16(l, y);
    __m128i hh = _mm_slli_epi16(h, y);

    return _mm_packs_epi16(ll, hh);
}
KFR_INTRINSIC u8sse shr(const u8sse& x, unsigned y)
{
    __m128i l = _mm_unpacklo_epi8(_mm_setzero_si128(), x.v);
    __m128i h = _mm_unpackhi_epi8(_mm_setzero_si128(), x.v);

    __m128i ll = _mm_srli_epi16(l, y);
    __m128i hh = _mm_srli_epi16(h, y);

    return _mm_packs_epi16(ll, hh);
}
KFR_INTRINSIC i8sse shr(const i8sse& x, unsigned y)
{
    __m128i l = _mm_unpacklo_epi8(_mm_setzero_si128(), x.v);
    __m128i h = _mm_unpackhi_epi8(_mm_setzero_si128(), x.v);

    __m128i ll = _mm_srai_epi16(l, y);
    __m128i hh = _mm_srai_epi16(h, y);

    return _mm_packs_epi16(ll, hh);
}

KFR_INTRINSIC i64sse shr(const i64sse& x, unsigned y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = x[i] >> y);
}

template <typename T, size_t N, typename = decltype(uibitcast(T())), KFR_ENABLE_IF(is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> shl(const vec<T, N>& x, const vec<utype<T>, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<uitype<T>>(uibitcast(x[i]) << y[i])));
}
template <typename T, size_t N, typename = decltype(uibitcast(T())), KFR_ENABLE_IF(is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> shr(const vec<T, N>& x, const vec<utype<T>, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<uitype<T>>(uibitcast(x[i]) >> y[i])));
}

KFR_INTRINSIC f32sse band(const f32sse& x, const f32sse& y) { return _mm_and_ps(x.v, y.v); }
KFR_INTRINSIC f64sse band(const f64sse& x, const f64sse& y) { return _mm_and_pd(x.v, y.v); }

KFR_INTRINSIC u8sse band(const u8sse& x, const u8sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC u16sse band(const u16sse& x, const u16sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC u32sse band(const u32sse& x, const u32sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC u64sse band(const u64sse& x, const u64sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC i8sse band(const i8sse& x, const i8sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC i16sse band(const i16sse& x, const i16sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC i32sse band(const i32sse& x, const i32sse& y) { return _mm_and_si128(x.v, y.v); }
KFR_INTRINSIC i64sse band(const i64sse& x, const i64sse& y) { return _mm_and_si128(x.v, y.v); }

KFR_INTRINSIC f32sse bor(const f32sse& x, const f32sse& y) { return _mm_or_ps(x.v, y.v); }
KFR_INTRINSIC f64sse bor(const f64sse& x, const f64sse& y) { return _mm_or_pd(x.v, y.v); }

KFR_INTRINSIC u8sse bor(const u8sse& x, const u8sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC u16sse bor(const u16sse& x, const u16sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC u32sse bor(const u32sse& x, const u32sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC u64sse bor(const u64sse& x, const u64sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC i8sse bor(const i8sse& x, const i8sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC i16sse bor(const i16sse& x, const i16sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC i32sse bor(const i32sse& x, const i32sse& y) { return _mm_or_si128(x.v, y.v); }
KFR_INTRINSIC i64sse bor(const i64sse& x, const i64sse& y) { return _mm_or_si128(x.v, y.v); }

KFR_INTRINSIC f32sse bxor(const f32sse& x, const f32sse& y) { return _mm_xor_ps(x.v, y.v); }
KFR_INTRINSIC f64sse bxor(const f64sse& x, const f64sse& y) { return _mm_xor_pd(x.v, y.v); }

KFR_INTRINSIC u8sse bxor(const u8sse& x, const u8sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC u16sse bxor(const u16sse& x, const u16sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC u32sse bxor(const u32sse& x, const u32sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC u64sse bxor(const u64sse& x, const u64sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC i8sse bxor(const i8sse& x, const i8sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC i16sse bxor(const i16sse& x, const i16sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC i32sse bxor(const i32sse& x, const i32sse& y) { return _mm_xor_si128(x.v, y.v); }
KFR_INTRINSIC i64sse bxor(const i64sse& x, const i64sse& y) { return _mm_xor_si128(x.v, y.v); }

KFR_INTRINSIC f32sse eq(const f32sse& x, const f32sse& y) { return _mm_cmpeq_ps(x.v, y.v); }
KFR_INTRINSIC f64sse eq(const f64sse& x, const f64sse& y) { return _mm_cmpeq_pd(x.v, y.v); }
KFR_INTRINSIC u8sse eq(const u8sse& x, const u8sse& y) { return _mm_cmpeq_epi8(x.v, y.v); }
KFR_INTRINSIC u16sse eq(const u16sse& x, const u16sse& y) { return _mm_cmpeq_epi16(x.v, y.v); }
KFR_INTRINSIC u32sse eq(const u32sse& x, const u32sse& y) { return _mm_cmpeq_epi32(x.v, y.v); }
KFR_INTRINSIC i8sse eq(const i8sse& x, const i8sse& y) { return _mm_cmpeq_epi8(x.v, y.v); }
KFR_INTRINSIC i16sse eq(const i16sse& x, const i16sse& y) { return _mm_cmpeq_epi16(x.v, y.v); }
KFR_INTRINSIC i32sse eq(const i32sse& x, const i32sse& y) { return _mm_cmpeq_epi32(x.v, y.v); }

KFR_INTRINSIC f32sse ne(const f32sse& x, const f32sse& y) { return _mm_not_ps(_mm_cmpeq_ps(x.v, y.v)); }
KFR_INTRINSIC f64sse ne(const f64sse& x, const f64sse& y) { return _mm_not_pd(_mm_cmpeq_pd(x.v, y.v)); }
KFR_INTRINSIC u8sse ne(const u8sse& x, const u8sse& y) { return _mm_not_si128(_mm_cmpeq_epi8(x.v, y.v)); }
KFR_INTRINSIC u16sse ne(const u16sse& x, const u16sse& y) { return _mm_not_si128(_mm_cmpeq_epi16(x.v, y.v)); }
KFR_INTRINSIC u32sse ne(const u32sse& x, const u32sse& y) { return _mm_not_si128(_mm_cmpeq_epi32(x.v, y.v)); }
KFR_INTRINSIC i8sse ne(const i8sse& x, const i8sse& y) { return _mm_not_si128(_mm_cmpeq_epi8(x.v, y.v)); }
KFR_INTRINSIC i16sse ne(const i16sse& x, const i16sse& y) { return _mm_not_si128(_mm_cmpeq_epi16(x.v, y.v)); }
KFR_INTRINSIC i32sse ne(const i32sse& x, const i32sse& y) { return _mm_not_si128(_mm_cmpeq_epi32(x.v, y.v)); }

KFR_INTRINSIC f32sse lt(const f32sse& x, const f32sse& y) { return _mm_cmplt_ps(x.v, y.v); }
KFR_INTRINSIC f64sse lt(const f64sse& x, const f64sse& y) { return _mm_cmplt_pd(x.v, y.v); }
KFR_INTRINSIC i8sse lt(const i8sse& x, const i8sse& y) { return _mm_cmplt_epi8(x.v, y.v); }
KFR_INTRINSIC i16sse lt(const i16sse& x, const i16sse& y) { return _mm_cmplt_epi16(x.v, y.v); }
KFR_INTRINSIC i32sse lt(const i32sse& x, const i32sse& y) { return _mm_cmplt_epi32(x.v, y.v); }

KFR_INTRINSIC u8sse lt(const u8sse& x, const u8sse& y)
{
    const __m128i hb = _mm_highbit_epi8();
    return _mm_cmplt_epi8(_mm_add_epi8(x.v, hb), _mm_add_epi8(y.v, hb));
}

KFR_INTRINSIC u16sse lt(const u16sse& x, const u16sse& y)
{
    const __m128i hb = _mm_highbit_epi16();
    return _mm_cmplt_epi16(_mm_add_epi16(x.v, hb), _mm_add_epi16(y.v, hb));
}
KFR_INTRINSIC u32sse lt(const u32sse& x, const u32sse& y)
{
    const __m128i hb = _mm_highbit_epi32();
    return _mm_cmplt_epi32(_mm_add_epi32(x.v, hb), _mm_add_epi32(y.v, hb));
}

KFR_INTRINSIC f32sse gt(const f32sse& x, const f32sse& y) { return _mm_cmpgt_ps(x.v, y.v); }
KFR_INTRINSIC f64sse gt(const f64sse& x, const f64sse& y) { return _mm_cmpgt_pd(x.v, y.v); }
KFR_INTRINSIC i8sse gt(const i8sse& x, const i8sse& y) { return _mm_cmpgt_epi8(x.v, y.v); }
KFR_INTRINSIC i16sse gt(const i16sse& x, const i16sse& y) { return _mm_cmpgt_epi16(x.v, y.v); }
KFR_INTRINSIC i32sse gt(const i32sse& x, const i32sse& y) { return _mm_cmpgt_epi32(x.v, y.v); }

KFR_INTRINSIC u8sse gt(const u8sse& x, const u8sse& y)
{
    const __m128i hb = _mm_highbit_epi8();
    return _mm_cmpgt_epi8(_mm_add_epi8(x.v, hb), _mm_add_epi8(y.v, hb));
}

KFR_INTRINSIC u16sse gt(const u16sse& x, const u16sse& y)
{
    const __m128i hb = _mm_highbit_epi16();
    return _mm_cmpgt_epi16(_mm_add_epi16(x.v, hb), _mm_add_epi16(y.v, hb));
}
KFR_INTRINSIC u32sse gt(const u32sse& x, const u32sse& y)
{
    const __m128i hb = _mm_highbit_epi32();
    return _mm_cmpgt_epi32(_mm_add_epi32(x.v, hb), _mm_add_epi32(y.v, hb));
}

KFR_INTRINSIC f32sse le(const f32sse& x, const f32sse& y) { return _mm_cmple_ps(x.v, y.v); }
KFR_INTRINSIC f64sse le(const f64sse& x, const f64sse& y) { return _mm_cmple_pd(x.v, y.v); }
KFR_INTRINSIC i8sse le(const i8sse& x, const i8sse& y) { return _mm_not_si128(_mm_cmpgt_epi8(x.v, y.v)); }
KFR_INTRINSIC i16sse le(const i16sse& x, const i16sse& y) { return _mm_not_si128(_mm_cmpgt_epi16(x.v, y.v)); }
KFR_INTRINSIC i32sse le(const i32sse& x, const i32sse& y) { return _mm_not_si128(_mm_cmpgt_epi32(x.v, y.v)); }

KFR_INTRINSIC u8sse le(const u8sse& x, const u8sse& y)
{
    const __m128i hb = _mm_highbit_epi8();
    return _mm_not_si128(_mm_cmpgt_epi8(_mm_add_epi8(x.v, hb), _mm_add_epi8(y.v, hb)));
}

KFR_INTRINSIC u16sse le(const u16sse& x, const u16sse& y)
{
    const __m128i hb = _mm_highbit_epi16();
    return _mm_not_si128(_mm_cmpgt_epi16(_mm_add_epi16(x.v, hb), _mm_add_epi16(y.v, hb)));
}
KFR_INTRINSIC u32sse le(const u32sse& x, const u32sse& y)
{
    const __m128i hb = _mm_highbit_epi32();
    return _mm_not_si128(_mm_cmpgt_epi32(_mm_add_epi32(x.v, hb), _mm_add_epi32(y.v, hb)));
}

KFR_INTRINSIC f32sse ge(const f32sse& x, const f32sse& y) { return _mm_cmpge_ps(x.v, y.v); }
KFR_INTRINSIC f64sse ge(const f64sse& x, const f64sse& y) { return _mm_cmpge_pd(x.v, y.v); }
KFR_INTRINSIC i8sse ge(const i8sse& x, const i8sse& y) { return _mm_not_si128(_mm_cmplt_epi8(x.v, y.v)); }
KFR_INTRINSIC i16sse ge(const i16sse& x, const i16sse& y) { return _mm_not_si128(_mm_cmplt_epi16(x.v, y.v)); }
KFR_INTRINSIC i32sse ge(const i32sse& x, const i32sse& y) { return _mm_not_si128(_mm_cmplt_epi32(x.v, y.v)); }

KFR_INTRINSIC u8sse ge(const u8sse& x, const u8sse& y)
{
    const __m128i hb = _mm_highbit_epi8();
    return _mm_not_si128(_mm_cmplt_epi8(_mm_add_epi8(x.v, hb), _mm_add_epi8(y.v, hb)));
}

KFR_INTRINSIC u16sse ge(const u16sse& x, const u16sse& y)
{
    const __m128i hb = _mm_highbit_epi16();
    return _mm_not_si128(_mm_cmplt_epi16(_mm_add_epi16(x.v, hb), _mm_add_epi16(y.v, hb)));
}
KFR_INTRINSIC u32sse ge(const u32sse& x, const u32sse& y)
{
    const __m128i hb = _mm_highbit_epi32();
    return _mm_not_si128(_mm_cmplt_epi32(_mm_add_epi32(x.v, hb), _mm_add_epi32(y.v, hb)));
}

#if defined CMT_ARCH_SSE41 && defined KFR_NATIVE_INTRINSICS
KFR_INTRINSIC u64sse eq(const u64sse& x, const u64sse& y) { return _mm_cmpeq_epi64(x.v, y.v); }
KFR_INTRINSIC i64sse eq(const i64sse& x, const i64sse& y) { return _mm_cmpeq_epi64(x.v, y.v); }
KFR_INTRINSIC u64sse ne(const u64sse& x, const u64sse& y) { return _mm_not_si128(_mm_cmpeq_epi64(x.v, y.v)); }
KFR_INTRINSIC i64sse ne(const i64sse& x, const i64sse& y) { return _mm_not_si128(_mm_cmpeq_epi64(x.v, y.v)); }
#else
KFR_INTRINSIC u64sse eq(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = maskbits<u64>(x[i] == y[i]));
}
KFR_INTRINSIC i64sse eq(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = maskbits<i64>(x[i] == y[i]));
}
KFR_INTRINSIC u64sse ne(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = maskbits<u64>(x[i] != y[i]));
}
KFR_INTRINSIC i64sse ne(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = maskbits<i64>(x[i] != y[i]));
}
#endif

#if defined CMT_ARCH_SSE42
KFR_INTRINSIC i64sse gt(const i64sse& x, const i64sse& y) { return _mm_cmpgt_epi64(x.v, y.v); }
KFR_INTRINSIC i64sse lt(const i64sse& x, const i64sse& y) { return _mm_cmpgt_epi64(y.v, x.v); }
KFR_INTRINSIC i64sse ge(const i64sse& x, const i64sse& y) { return _mm_not_si128(_mm_cmpgt_epi64(y.v, x.v)); }
KFR_INTRINSIC i64sse le(const i64sse& x, const i64sse& y) { return _mm_not_si128(_mm_cmpgt_epi64(x.v, y.v)); }

KFR_INTRINSIC u64sse gt(const u64sse& x, const u64sse& y)
{
    const __m128i hb = _mm_highbit_epi64();
    return _mm_cmpgt_epi64(_mm_add_epi64(x.v, hb), _mm_add_epi64(y.v, hb));
}
KFR_INTRINSIC u64sse lt(const u64sse& x, const u64sse& y)
{
    const __m128i hb = _mm_highbit_epi64();
    return _mm_cmpgt_epi64(_mm_add_epi64(y.v, hb), _mm_add_epi64(x.v, hb));
}
KFR_INTRINSIC u64sse ge(const u64sse& x, const u64sse& y)
{
    const __m128i hb = _mm_highbit_epi64();
    return _mm_not_si128(_mm_cmpgt_epi64(_mm_add_epi64(y.v, hb), _mm_add_epi64(x.v, hb)));
}
KFR_INTRINSIC u64sse le(const u64sse& x, const u64sse& y)
{
    const __m128i hb = _mm_highbit_epi64();
    return _mm_not_si128(_mm_cmpgt_epi64(_mm_add_epi64(x.v, hb), _mm_add_epi64(y.v, hb)));
}

#else
KFR_INTRINSIC u64sse gt(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = maskbits<u64>(x[i] > y[i]));
}
KFR_INTRINSIC i64sse gt(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = maskbits<i64>(x[i] > y[i]));
}
KFR_INTRINSIC u64sse lt(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = maskbits<u64>(x[i] < y[i]));
}
KFR_INTRINSIC i64sse lt(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = maskbits<i64>(x[i] < y[i]));
}
KFR_INTRINSIC u64sse ge(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = maskbits<u64>(x[i] >= y[i]));
}
KFR_INTRINSIC i64sse ge(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = maskbits<i64>(x[i] >= y[i]));
}
KFR_INTRINSIC u64sse le(const u64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(u64sse, result[i] = maskbits<u64>(x[i] <= y[i]));
}
KFR_INTRINSIC i64sse le(const i64sse& x, const i64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = maskbits<i64>(x[i] <= y[i]));
}
#endif

#if defined CMT_ARCH_AVX

KFR_INTRINSIC f32avx add(const f32avx& x, const f32avx& y) { return f32avx(_mm256_add_ps(x.v, y.v)); }
KFR_INTRINSIC f64avx add(const f64avx& x, const f64avx& y) { return f64avx(_mm256_add_pd(x.v, y.v)); }
KFR_INTRINSIC f32avx sub(const f32avx& x, const f32avx& y) { return f32avx(_mm256_sub_ps(x.v, y.v)); }
KFR_INTRINSIC f64avx sub(const f64avx& x, const f64avx& y) { return f64avx(_mm256_sub_pd(x.v, y.v)); }
KFR_INTRINSIC f32avx mul(const f32avx& x, const f32avx& y) { return f32avx(_mm256_mul_ps(x.v, y.v)); }
KFR_INTRINSIC f64avx mul(const f64avx& x, const f64avx& y) { return f64avx(_mm256_mul_pd(x.v, y.v)); }
KFR_INTRINSIC f32avx div(const f32avx& x, const f32avx& y) { return f32avx(_mm256_div_ps(x.v, y.v)); }
KFR_INTRINSIC f64avx div(const f64avx& x, const f64avx& y) { return f64avx(_mm256_div_pd(x.v, y.v)); }

KFR_INTRINSIC __m256 _mm256_allones_ps()
{
    return _mm256_cmp_ps(_mm256_setzero_ps(), _mm256_setzero_ps(), _CMP_EQ_UQ);
}

KFR_INTRINSIC __m256d _mm256_allones_pd()
{
    return _mm256_cmp_pd(_mm256_setzero_pd(), _mm256_setzero_pd(), _CMP_EQ_UQ);
}

#if defined CMT_ARCH_AVX2
KFR_INTRINSIC __m256i _mm256_allones_si256()
{
    return _mm256_cmpeq_epi8(_mm256_setzero_si256(), _mm256_setzero_si256());
}
KFR_INTRINSIC __m256i _mm256_not_si256(const __m256i& x)
{
    return _mm256_xor_si256(x, _mm256_allones_si256());
}
#else
KFR_INTRINSIC __m256i _mm256_allones_si256()
{
    return _mm256_castps_si256(_mm256_cmp_ps(_mm256_setzero_ps(), _mm256_setzero_ps(), _CMP_EQ_UQ));
}
KFR_INTRINSIC __m256i _mm256_not_si256(const __m256i& x)
{
    return _mm256_castps_si256(_mm256_xor_ps(_mm256_castsi256_ps(x), _mm256_allones_ps()));
}
#endif

KFR_INTRINSIC __m256 _mm256_not_ps(const __m256& x) { return _mm256_xor_ps(x, _mm256_allones_ps()); }
KFR_INTRINSIC __m256d _mm256_not_pd(const __m256d& x) { return _mm256_xor_pd(x, _mm256_allones_pd()); }

KFR_INTRINSIC __m256i _mm256_highbit_epi8() { return _mm256_set1_epi8(static_cast<char>(0x80)); }
KFR_INTRINSIC __m256i _mm256_highbit_epi16() { return _mm256_set1_epi16(static_cast<short>(0x8000)); }
KFR_INTRINSIC __m256i _mm256_highbit_epi32() { return _mm256_set1_epi32(static_cast<int>(0x80000000)); }
KFR_INTRINSIC __m256i _mm256_highbit_epi64() { return _mm256_set1_epi64x(0x8000000000000000ll); }

KFR_INTRINSIC f32avx eq(const f32avx& x, const f32avx& y) { return _mm256_cmp_ps(x.v, y.v, _CMP_EQ_OQ); }
KFR_INTRINSIC f64avx eq(const f64avx& x, const f64avx& y) { return _mm256_cmp_pd(x.v, y.v, _CMP_EQ_OQ); }
KFR_INTRINSIC f32avx ne(const f32avx& x, const f32avx& y) { return _mm256_cmp_ps(x.v, y.v, _CMP_NEQ_OQ); }
KFR_INTRINSIC f64avx ne(const f64avx& x, const f64avx& y) { return _mm256_cmp_pd(x.v, y.v, _CMP_NEQ_OQ); }
KFR_INTRINSIC f32avx lt(const f32avx& x, const f32avx& y) { return _mm256_cmp_ps(x.v, y.v, _CMP_LT_OQ); }
KFR_INTRINSIC f64avx lt(const f64avx& x, const f64avx& y) { return _mm256_cmp_pd(x.v, y.v, _CMP_LT_OQ); }
KFR_INTRINSIC f32avx gt(const f32avx& x, const f32avx& y) { return _mm256_cmp_ps(x.v, y.v, _CMP_GT_OQ); }
KFR_INTRINSIC f64avx gt(const f64avx& x, const f64avx& y) { return _mm256_cmp_pd(x.v, y.v, _CMP_GT_OQ); }
KFR_INTRINSIC f32avx le(const f32avx& x, const f32avx& y) { return _mm256_cmp_ps(x.v, y.v, _CMP_LE_OQ); }
KFR_INTRINSIC f64avx le(const f64avx& x, const f64avx& y) { return _mm256_cmp_pd(x.v, y.v, _CMP_LE_OQ); }
KFR_INTRINSIC f32avx ge(const f32avx& x, const f32avx& y) { return _mm256_cmp_ps(x.v, y.v, _CMP_GE_OQ); }
KFR_INTRINSIC f64avx ge(const f64avx& x, const f64avx& y) { return _mm256_cmp_pd(x.v, y.v, _CMP_GE_OQ); }

KFR_INTRINSIC f32avx band(const f32avx& x, const f32avx& y) { return _mm256_and_ps(x.v, y.v); }
KFR_INTRINSIC f64avx band(const f64avx& x, const f64avx& y) { return _mm256_and_pd(x.v, y.v); }
KFR_INTRINSIC f32avx bor(const f32avx& x, const f32avx& y) { return _mm256_or_ps(x.v, y.v); }
KFR_INTRINSIC f64avx bor(const f64avx& x, const f64avx& y) { return _mm256_or_pd(x.v, y.v); }
KFR_INTRINSIC f32avx bxor(const f32avx& x, const f32avx& y) { return _mm256_xor_ps(x.v, y.v); }
KFR_INTRINSIC f64avx bxor(const f64avx& x, const f64avx& y) { return _mm256_xor_pd(x.v, y.v); }

KFR_INTRINSIC f32avx shl(const f32avx& x, unsigned y)
{
#if defined CMT_ARCH_AVX2
    return _mm256_castsi256_ps(_mm256_slli_epi32(_mm256_castps_si256(x.v), y));
#else
    return KFR_mm256_setr_m128(
        _mm_castsi128_ps(_mm_slli_epi32(_mm_castps_si128(_mm256_castps256_ps128(x.v)), y)),
        _mm_castsi128_ps(_mm_slli_epi32(_mm_castps_si128(_mm256_extractf128_ps(x.v, 1)), y)));
#endif
}
KFR_INTRINSIC f64avx shl(const f64avx& x, unsigned y)
{
#if defined CMT_ARCH_AVX2
    return _mm256_castsi256_pd(_mm256_slli_epi64(_mm256_castpd_si256(x.v), y));
#else
    return KFR_mm256_setr_m128d(
        _mm_castsi128_pd(_mm_slli_epi64(_mm_castpd_si128(_mm256_castpd256_pd128(x.v)), y)),
        _mm_castsi128_pd(_mm_slli_epi64(_mm_castpd_si128(_mm256_extractf128_pd(x.v, 1)), y)));
#endif
}
KFR_INTRINSIC f32avx shr(const f32avx& x, unsigned y)
{
#if defined CMT_ARCH_AVX2
    return _mm256_castsi256_ps(_mm256_srli_epi32(_mm256_castps_si256(x.v), y));
#else
    return KFR_mm256_setr_m128(
        _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(_mm256_castps256_ps128(x.v)), y)),
        _mm_castsi128_ps(_mm_srli_epi32(_mm_castps_si128(_mm256_extractf128_ps(x.v, 1)), y)));
#endif
}
KFR_INTRINSIC f64avx shr(const f64avx& x, unsigned y)
{
#if defined CMT_ARCH_AVX2
    return _mm256_castsi256_pd(_mm256_srli_epi64(_mm256_castpd_si256(x.v), y));
#else
    return KFR_mm256_setr_m128d(
        _mm_castsi128_pd(_mm_srli_epi64(_mm_castpd_si128(_mm256_castpd256_pd128(x.v)), y)),
        _mm_castsi128_pd(_mm_srli_epi64(_mm_castpd_si128(_mm256_extractf128_pd(x.v, 1)), y)));
#endif
}

#if defined CMT_ARCH_AVX2

KFR_INTRINSIC u8avx add(const u8avx& x, const u8avx& y) { return _mm256_add_epi8(x.v, y.v); }
KFR_INTRINSIC u8avx sub(const u8avx& x, const u8avx& y) { return _mm256_sub_epi8(x.v, y.v); }
KFR_DIV_MOD_FN(u8avx)

KFR_INTRINSIC i8avx add(const i8avx& x, const i8avx& y) { return _mm256_add_epi8(x.v, y.v); }
KFR_INTRINSIC i8avx sub(const i8avx& x, const i8avx& y) { return _mm256_sub_epi8(x.v, y.v); }
KFR_DIV_MOD_FN(i8avx)

KFR_INTRINSIC u16avx add(const u16avx& x, const u16avx& y) { return _mm256_add_epi16(x.v, y.v); }
KFR_INTRINSIC u16avx sub(const u16avx& x, const u16avx& y) { return _mm256_sub_epi16(x.v, y.v); }
KFR_INTRINSIC u16avx mul(const u16avx& x, const u16avx& y) { return _mm256_mullo_epi16(x.v, y.v); }
KFR_DIV_MOD_FN(u16avx)

KFR_INTRINSIC i16avx add(const i16avx& x, const i16avx& y) { return _mm256_add_epi16(x.v, y.v); }
KFR_INTRINSIC i16avx sub(const i16avx& x, const i16avx& y) { return _mm256_sub_epi16(x.v, y.v); }
KFR_INTRINSIC i16avx mul(const i16avx& x, const i16avx& y) { return _mm256_mullo_epi16(x.v, y.v); }
KFR_DIV_MOD_FN(i16avx)

KFR_INTRINSIC u32avx add(const u32avx& x, const u32avx& y) { return _mm256_add_epi32(x.v, y.v); }
KFR_INTRINSIC u32avx sub(const u32avx& x, const u32avx& y) { return _mm256_sub_epi32(x.v, y.v); }

KFR_INTRINSIC i32avx add(const i32avx& x, const i32avx& y) { return _mm256_add_epi32(x.v, y.v); }
KFR_INTRINSIC i32avx sub(const i32avx& x, const i32avx& y) { return _mm256_sub_epi32(x.v, y.v); }

KFR_INTRINSIC u32avx mul(const u32avx& x, const u32avx& y) { return _mm256_mullo_epi32(x.v, y.v); }
KFR_INTRINSIC i32avx mul(const i32avx& x, const i32avx& y) { return _mm256_mullo_epi32(x.v, y.v); }
KFR_DIV_MOD_FN(u32avx)
KFR_DIV_MOD_FN(i32avx)

KFR_INTRINSIC u64avx add(const u64avx& x, const u64avx& y) { return _mm256_add_epi64(x.v, y.v); }
KFR_INTRINSIC u64avx sub(const u64avx& x, const u64avx& y) { return _mm256_sub_epi64(x.v, y.v); }
KFR_INTRINSIC u64avx mul(const u64avx& x, const u64avx& y)
{
    KFR_COMPONENTWISE_RET_I(u64avx, result[i] = x[i] * y[i]);
}

KFR_INTRINSIC i64avx add(const i64avx& x, const i64avx& y) { return _mm256_add_epi64(x.v, y.v); }
KFR_INTRINSIC i64avx sub(const i64avx& x, const i64avx& y) { return _mm256_sub_epi64(x.v, y.v); }
KFR_INTRINSIC i64avx mul(const i64avx& x, const i64avx& y)
{
    KFR_COMPONENTWISE_RET_I(i64avx, result[i] = x[i] * y[i]);
}
KFR_DIV_MOD_FN(u64avx)
KFR_DIV_MOD_FN(i64avx)

KFR_INTRINSIC __m256i mul_epi8(const __m256i& x, const __m256i& y)
{
    const __m256i even = _mm256_mullo_epi16(x, y);
    const __m256i odd  = _mm256_mullo_epi16(_mm256_srli_epi16(x, 8), _mm256_srli_epi16(y, 8));
    return _mm256_or_si256(_mm256_slli_epi16(odd, 8), _mm256_srli_epi16(_mm256_slli_epi16(even, 8), 8));
}

KFR_INTRINSIC u8avx mul(const u8avx& x, const u8avx& y) { return mul_epi8(x.v, y.v); }
KFR_INTRINSIC i8avx mul(const i8avx& x, const i8avx& y) { return mul_epi8(x.v, y.v); }

KFR_INTRINSIC u8avx band(const u8avx& x, const u8avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC u16avx band(const u16avx& x, const u16avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC u32avx band(const u32avx& x, const u32avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC u64avx band(const u64avx& x, const u64avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC i8avx band(const i8avx& x, const i8avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC i16avx band(const i16avx& x, const i16avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC i32avx band(const i32avx& x, const i32avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC i64avx band(const i64avx& x, const i64avx& y) { return _mm256_and_si256(x.v, y.v); }
KFR_INTRINSIC u8avx bor(const u8avx& x, const u8avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC u16avx bor(const u16avx& x, const u16avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC u32avx bor(const u32avx& x, const u32avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC u64avx bor(const u64avx& x, const u64avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC i8avx bor(const i8avx& x, const i8avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC i16avx bor(const i16avx& x, const i16avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC i32avx bor(const i32avx& x, const i32avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC i64avx bor(const i64avx& x, const i64avx& y) { return _mm256_or_si256(x.v, y.v); }
KFR_INTRINSIC u8avx bxor(const u8avx& x, const u8avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC u16avx bxor(const u16avx& x, const u16avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC u32avx bxor(const u32avx& x, const u32avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC u64avx bxor(const u64avx& x, const u64avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC i8avx bxor(const i8avx& x, const i8avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC i16avx bxor(const i16avx& x, const i16avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC i32avx bxor(const i32avx& x, const i32avx& y) { return _mm256_xor_si256(x.v, y.v); }
KFR_INTRINSIC i64avx bxor(const i64avx& x, const i64avx& y) { return _mm256_xor_si256(x.v, y.v); }

KFR_INTRINSIC u16avx shl(const u16avx& x, unsigned y) { return _mm256_slli_epi16(x.v, y); }
KFR_INTRINSIC u32avx shl(const u32avx& x, unsigned y) { return _mm256_slli_epi32(x.v, y); }
KFR_INTRINSIC i16avx shl(const i16avx& x, unsigned y) { return _mm256_slli_epi16(x.v, y); }
KFR_INTRINSIC i32avx shl(const i32avx& x, unsigned y) { return _mm256_slli_epi32(x.v, y); }
KFR_INTRINSIC u16avx shr(const u16avx& x, unsigned y) { return _mm256_srli_epi16(x.v, y); }
KFR_INTRINSIC u32avx shr(const u32avx& x, unsigned y) { return _mm256_srli_epi32(x.v, y); }
KFR_INTRINSIC i16avx shr(const i16avx& x, unsigned y) { return _mm256_srai_epi16(x.v, y); }
KFR_INTRINSIC i32avx shr(const i32avx& x, unsigned y) { return _mm256_srai_epi32(x.v, y); }

KFR_INTRINSIC u64avx shl(const u64avx& x, unsigned y) { return _mm256_slli_epi64(x.v, y); }
KFR_INTRINSIC u64avx shr(const u64avx& x, unsigned y) { return _mm256_srli_epi64(x.v, y); }
KFR_INTRINSIC i64avx shl(const i64avx& x, unsigned y) { return _mm256_slli_epi64(x.v, y); }
KFR_INTRINSIC i64avx shr(const i64avx& x, unsigned y)
{
    KFR_COMPONENTWISE_RET_I(u64avx, result[i] = x[i] >> y);
}

KFR_INTRINSIC u8avx shl(const u8avx& x, unsigned y)
{
    __m256i l  = _mm256_unpacklo_epi8(_mm256_setzero_si256(), x.v);
    __m256i h  = _mm256_unpackhi_epi8(_mm256_setzero_si256(), x.v);
    __m256i ll = _mm256_slli_epi16(l, y);
    __m256i hh = _mm256_slli_epi16(h, y);

    return _mm256_packs_epi16(ll, hh);
}
KFR_INTRINSIC i8avx shl(const i8avx& x, unsigned y)
{
    __m256i l  = _mm256_unpacklo_epi8(_mm256_setzero_si256(), x.v);
    __m256i h  = _mm256_unpackhi_epi8(_mm256_setzero_si256(), x.v);
    __m256i ll = _mm256_slli_epi16(l, y);
    __m256i hh = _mm256_slli_epi16(h, y);

    return _mm256_packs_epi16(ll, hh);
}
KFR_INTRINSIC u8avx shr(const u8avx& x, unsigned y)
{
    __m256i l  = _mm256_unpacklo_epi8(_mm256_setzero_si256(), x.v);
    __m256i h  = _mm256_unpackhi_epi8(_mm256_setzero_si256(), x.v);
    __m256i ll = _mm256_srli_epi16(l, y);
    __m256i hh = _mm256_srli_epi16(h, y);

    return _mm256_packs_epi16(ll, hh);
}
KFR_INTRINSIC i8avx shr(const i8avx& x, unsigned y)
{
    __m256i l  = _mm256_unpacklo_epi8(_mm256_setzero_si256(), x.v);
    __m256i h  = _mm256_unpackhi_epi8(_mm256_setzero_si256(), x.v);
    __m256i ll = _mm256_srai_epi16(l, y);
    __m256i hh = _mm256_srai_epi16(h, y);

    return _mm256_packs_epi16(ll, hh);
}

KFR_INTRINSIC u32sse shl(const u32sse& x, const u32sse& y) { return _mm_sllv_epi32(x.v, y.v); }
KFR_INTRINSIC i32sse shl(const i32sse& x, const u32sse& y) { return _mm_sllv_epi32(x.v, y.v); }
KFR_INTRINSIC u64sse shl(const u64sse& x, const u64sse& y) { return _mm_sllv_epi64(x.v, y.v); }
KFR_INTRINSIC i64sse shl(const i64sse& x, const u64sse& y) { return _mm_sllv_epi64(x.v, y.v); }

KFR_INTRINSIC u32avx shl(const u32avx& x, const u32avx& y) { return _mm256_sllv_epi32(x.v, y.v); }
KFR_INTRINSIC i32avx shl(const i32avx& x, const u32avx& y) { return _mm256_sllv_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx shl(const u64avx& x, const u64avx& y) { return _mm256_sllv_epi64(x.v, y.v); }
KFR_INTRINSIC i64avx shl(const i64avx& x, const u64avx& y) { return _mm256_sllv_epi64(x.v, y.v); }

KFR_INTRINSIC u32sse shr(const u32sse& x, const u32sse& y) { return _mm_srlv_epi32(x.v, y.v); }
KFR_INTRINSIC i32sse shr(const i32sse& x, const u32sse& y) { return _mm_srav_epi32(x.v, y.v); }
KFR_INTRINSIC u64sse shr(const u64sse& x, const u64sse& y) { return _mm_srlv_epi64(x.v, y.v); }
KFR_INTRINSIC i64sse shr(const i64sse& x, const u64sse& y)
{
    KFR_COMPONENTWISE_RET_I(i64sse, result[i] = x[i] >> y[i]);
}

KFR_INTRINSIC u32avx shr(const u32avx& x, const u32avx& y) { return _mm256_srlv_epi32(x.v, y.v); }
KFR_INTRINSIC i32avx shr(const i32avx& x, const u32avx& y) { return _mm256_srav_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx shr(const u64avx& x, const u64avx& y) { return _mm256_srlv_epi64(x.v, y.v); }
KFR_INTRINSIC i64avx shr(const i64avx& x, const u64avx& y)
{
    KFR_COMPONENTWISE_RET_I(i64avx, result[i] = x[i] >> y[i]);
}

KFR_INTRINSIC f32sse shl(const f32sse& x, const u32sse& y)
{
    return _mm_castsi128_ps(_mm_sllv_epi32(_mm_castps_si128(x.v), y.v));
}
KFR_INTRINSIC f64sse shl(const f64sse& x, const u64sse& y)
{
    return _mm_castsi128_pd(_mm_sllv_epi64(_mm_castpd_si128(x.v), y.v));
}
KFR_INTRINSIC f32sse shr(const f32sse& x, const u32sse& y)
{
    return _mm_castsi128_ps(_mm_srlv_epi32(_mm_castps_si128(x.v), y.v));
}
KFR_INTRINSIC f64sse shr(const f64sse& x, const u64sse& y)
{
    return _mm_castsi128_pd(_mm_srlv_epi64(_mm_castpd_si128(x.v), y.v));
}

KFR_INTRINSIC f32avx shl(const f32avx& x, const u32avx& y)
{
    return _mm256_castsi256_ps(_mm256_sllv_epi32(_mm256_castps_si256(x.v), y.v));
}
KFR_INTRINSIC f64avx shl(const f64avx& x, const u64avx& y)
{
    return _mm256_castsi256_pd(_mm256_sllv_epi64(_mm256_castpd_si256(x.v), y.v));
}
KFR_INTRINSIC f32avx shr(const f32avx& x, const u32avx& y)
{
    return _mm256_castsi256_ps(_mm256_srlv_epi32(_mm256_castps_si256(x.v), y.v));
}
KFR_INTRINSIC f64avx shr(const f64avx& x, const u64avx& y)
{
    return _mm256_castsi256_pd(_mm256_srlv_epi64(_mm256_castpd_si256(x.v), y.v));
}

KFR_INTRINSIC i8avx eq(const i8avx& x, const i8avx& y) { return _mm256_cmpeq_epi8(x.v, y.v); }
KFR_INTRINSIC i16avx eq(const i16avx& x, const i16avx& y) { return _mm256_cmpeq_epi16(x.v, y.v); }
KFR_INTRINSIC i32avx eq(const i32avx& x, const i32avx& y) { return _mm256_cmpeq_epi32(x.v, y.v); }
KFR_INTRINSIC i64avx eq(const i64avx& x, const i64avx& y) { return _mm256_cmpeq_epi64(x.v, y.v); }
KFR_INTRINSIC u8avx eq(const u8avx& x, const u8avx& y) { return _mm256_cmpeq_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx eq(const u16avx& x, const u16avx& y) { return _mm256_cmpeq_epi16(x.v, y.v); }
KFR_INTRINSIC u32avx eq(const u32avx& x, const u32avx& y) { return _mm256_cmpeq_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx eq(const u64avx& x, const u64avx& y) { return _mm256_cmpeq_epi64(x.v, y.v); }

KFR_INTRINSIC i8avx ne(const i8avx& x, const i8avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi8(x.v, y.v));
}
KFR_INTRINSIC i16avx ne(const i16avx& x, const i16avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi16(x.v, y.v));
}
KFR_INTRINSIC i32avx ne(const i32avx& x, const i32avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi32(x.v, y.v));
}
KFR_INTRINSIC i64avx ne(const i64avx& x, const i64avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi64(x.v, y.v));
}
KFR_INTRINSIC u8avx ne(const u8avx& x, const u8avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi8(x.v, y.v));
}
KFR_INTRINSIC u16avx ne(const u16avx& x, const u16avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi16(x.v, y.v));
}
KFR_INTRINSIC u32avx ne(const u32avx& x, const u32avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi32(x.v, y.v));
}
KFR_INTRINSIC u64avx ne(const u64avx& x, const u64avx& y)
{
    return _mm256_not_si256(_mm256_cmpeq_epi64(x.v, y.v));
}

KFR_INTRINSIC i8avx lt(const i8avx& x, const i8avx& y) { return _mm256_cmpgt_epi8(y.v, x.v); }
KFR_INTRINSIC i16avx lt(const i16avx& x, const i16avx& y) { return _mm256_cmpgt_epi16(y.v, x.v); }
KFR_INTRINSIC i32avx lt(const i32avx& x, const i32avx& y) { return _mm256_cmpgt_epi32(y.v, x.v); }
KFR_INTRINSIC i64avx lt(const i64avx& x, const i64avx& y) { return _mm256_cmpgt_epi64(y.v, x.v); }

KFR_INTRINSIC i8avx gt(const i8avx& x, const i8avx& y) { return _mm256_cmpgt_epi8(x.v, y.v); }
KFR_INTRINSIC i16avx gt(const i16avx& x, const i16avx& y) { return _mm256_cmpgt_epi16(x.v, y.v); }
KFR_INTRINSIC i32avx gt(const i32avx& x, const i32avx& y) { return _mm256_cmpgt_epi32(x.v, y.v); }
KFR_INTRINSIC i64avx gt(const i64avx& x, const i64avx& y) { return _mm256_cmpgt_epi64(x.v, y.v); }

KFR_INTRINSIC i8avx le(const i8avx& x, const i8avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi8(x.v, y.v));
}
KFR_INTRINSIC i16avx le(const i16avx& x, const i16avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi16(x.v, y.v));
}
KFR_INTRINSIC i32avx le(const i32avx& x, const i32avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi32(x.v, y.v));
}
KFR_INTRINSIC i64avx le(const i64avx& x, const i64avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi64(x.v, y.v));
}

KFR_INTRINSIC i8avx ge(const i8avx& x, const i8avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi8(y.v, x.v));
}
KFR_INTRINSIC i16avx ge(const i16avx& x, const i16avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi16(y.v, x.v));
}
KFR_INTRINSIC i32avx ge(const i32avx& x, const i32avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi32(y.v, x.v));
}
KFR_INTRINSIC i64avx ge(const i64avx& x, const i64avx& y)
{
    return _mm256_not_si256(_mm256_cmpgt_epi64(y.v, x.v));
}

KFR_INTRINSIC u8avx lt(const u8avx& x, const u8avx& y)
{
    const __m256i hb = _mm256_highbit_epi8();
    return _mm256_cmpgt_epi8(_mm256_add_epi8(y.v, hb), _mm256_add_epi8(x.v, hb));
}
KFR_INTRINSIC u16avx lt(const u16avx& x, const u16avx& y)
{
    const __m256i hb = _mm256_highbit_epi16();
    return _mm256_cmpgt_epi16(_mm256_add_epi16(y.v, hb), _mm256_add_epi16(x.v, hb));
}
KFR_INTRINSIC u32avx lt(const u32avx& x, const u32avx& y)
{
    const __m256i hb = _mm256_highbit_epi32();
    return _mm256_cmpgt_epi32(_mm256_add_epi32(y.v, hb), _mm256_add_epi32(x.v, hb));
}
KFR_INTRINSIC u64avx lt(const u64avx& x, const u64avx& y)
{
    const __m256i hb = _mm256_highbit_epi64();
    return _mm256_cmpgt_epi64(_mm256_add_epi64(y.v, hb), _mm256_add_epi64(x.v, hb));
}
KFR_INTRINSIC u8avx gt(const u8avx& x, const u8avx& y)
{
    const __m256i hb = _mm256_highbit_epi8();
    return _mm256_cmpgt_epi8(_mm256_add_epi8(x.v, hb), _mm256_add_epi8(y.v, hb));
}
KFR_INTRINSIC u16avx gt(const u16avx& x, const u16avx& y)
{
    const __m256i hb = _mm256_highbit_epi16();
    return _mm256_cmpgt_epi16(_mm256_add_epi16(x.v, hb), _mm256_add_epi16(y.v, hb));
}
KFR_INTRINSIC u32avx gt(const u32avx& x, const u32avx& y)
{
    const __m256i hb = _mm256_highbit_epi32();
    return _mm256_cmpgt_epi32(_mm256_add_epi32(x.v, hb), _mm256_add_epi32(y.v, hb));
}
KFR_INTRINSIC u64avx gt(const u64avx& x, const u64avx& y)
{
    const __m256i hb = _mm256_highbit_epi64();
    return _mm256_cmpgt_epi64(_mm256_add_epi64(x.v, hb), _mm256_add_epi64(y.v, hb));
}
KFR_INTRINSIC u8avx le(const u8avx& x, const u8avx& y)
{
    const __m256i hb = _mm256_highbit_epi8();
    return _mm256_not_si256(_mm256_cmpgt_epi8(_mm256_add_epi8(x.v, hb), _mm256_add_epi8(y.v, hb)));
}
KFR_INTRINSIC u16avx le(const u16avx& x, const u16avx& y)
{
    const __m256i hb = _mm256_highbit_epi16();
    return _mm256_not_si256(_mm256_cmpgt_epi16(_mm256_add_epi16(x.v, hb), _mm256_add_epi16(y.v, hb)));
}
KFR_INTRINSIC u32avx le(const u32avx& x, const u32avx& y)
{
    const __m256i hb = _mm256_highbit_epi32();
    return _mm256_not_si256(_mm256_cmpgt_epi32(_mm256_add_epi32(x.v, hb), _mm256_add_epi32(y.v, hb)));
}
KFR_INTRINSIC u64avx le(const u64avx& x, const u64avx& y)
{
    const __m256i hb = _mm256_highbit_epi64();
    return _mm256_not_si256(_mm256_cmpgt_epi64(_mm256_add_epi64(x.v, hb), _mm256_add_epi64(y.v, hb)));
}
KFR_INTRINSIC u8avx ge(const u8avx& x, const u8avx& y)
{
    const __m256i hb = _mm256_highbit_epi8();
    return _mm256_not_si256(_mm256_cmpgt_epi8(_mm256_add_epi8(y.v, hb), _mm256_add_epi8(x.v, hb)));
}
KFR_INTRINSIC u16avx ge(const u16avx& x, const u16avx& y)
{
    const __m256i hb = _mm256_highbit_epi16();
    return _mm256_not_si256(_mm256_cmpgt_epi16(_mm256_add_epi16(y.v, hb), _mm256_add_epi16(x.v, hb)));
}
KFR_INTRINSIC u32avx ge(const u32avx& x, const u32avx& y)
{
    const __m256i hb = _mm256_highbit_epi32();
    return _mm256_not_si256(_mm256_cmpgt_epi32(_mm256_add_epi32(y.v, hb), _mm256_add_epi32(x.v, hb)));
}
KFR_INTRINSIC u64avx ge(const u64avx& x, const u64avx& y)
{
    const __m256i hb = _mm256_highbit_epi64();
    return _mm256_not_si256(_mm256_cmpgt_epi64(_mm256_add_epi64(y.v, hb), _mm256_add_epi64(x.v, hb)));
}

#if defined CMT_ARCH_AVX512
KFR_INTRINSIC f32avx512 add(const f32avx512& x, const f32avx512& y) { return _mm512_add_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 add(const f64avx512& x, const f64avx512& y) { return _mm512_add_pd(x.v, y.v); }
KFR_INTRINSIC f32avx512 sub(const f32avx512& x, const f32avx512& y) { return _mm512_sub_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 sub(const f64avx512& x, const f64avx512& y) { return _mm512_sub_pd(x.v, y.v); }
KFR_INTRINSIC f32avx512 mul(const f32avx512& x, const f32avx512& y) { return _mm512_mul_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 mul(const f64avx512& x, const f64avx512& y) { return _mm512_mul_pd(x.v, y.v); }
KFR_INTRINSIC f32avx512 div(const f32avx512& x, const f32avx512& y) { return _mm512_div_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 div(const f64avx512& x, const f64avx512& y) { return _mm512_div_pd(x.v, y.v); }

KFR_INTRINSIC __m512 _mm512_allones_ps()
{
    return _mm512_castsi512_ps(_mm512_ternarylogic_epi32(_mm512_setzero_si512(), _mm512_setzero_si512(),
                                                         _mm512_setzero_si512(), 0xFF));
}

KFR_INTRINSIC __m512d _mm512_allones_pd()
{
    return _mm512_castsi512_pd(_mm512_ternarylogic_epi32(_mm512_setzero_si512(), _mm512_setzero_si512(),
                                                         _mm512_setzero_si512(), 0xFF));
}

KFR_INTRINSIC __m512i _mm512_allones_si512()
{
    return _mm512_ternarylogic_epi32(_mm512_setzero_si512(), _mm512_setzero_si512(), _mm512_setzero_si512(),
                                     0xFF);
}

KFR_INTRINSIC __m512 _mm512_not_ps(const __m512& x) { return _mm512_xor_ps(x, _mm512_allones_ps()); }
KFR_INTRINSIC __m512d _mm512_not_pd(const __m512d& x) { return _mm512_xor_pd(x, _mm512_allones_pd()); }
KFR_INTRINSIC __m512i _mm512_not_si512(const __m512i& x)
{
    return _mm512_xor_si512(x, _mm512_allones_si512());
}

KFR_INTRINSIC __m512i _mm512_highbit_epi8() { return _mm512_set1_epi8(static_cast<char>(0x80)); }
KFR_INTRINSIC __m512i _mm512_highbit_epi16() { return _mm512_set1_epi16(static_cast<short>(0x8000)); }
KFR_INTRINSIC __m512i _mm512_highbit_epi32() { return _mm512_set1_epi32(static_cast<int>(0x80000000)); }
KFR_INTRINSIC __m512i _mm512_highbit_epi64() { return _mm512_set1_epi64(0x8000000000000000ll); }

KFR_INTRINSIC f32avx512 eq(const f32avx512& x, const f32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_movm_epi32(_mm512_cmp_ps_mask(x.v, y.v, _CMP_EQ_OQ)));
}
KFR_INTRINSIC f64avx512 eq(const f64avx512& x, const f64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_movm_epi64(_mm512_cmp_pd_mask(x.v, y.v, _CMP_EQ_OQ)));
}
KFR_INTRINSIC f32avx512 ne(const f32avx512& x, const f32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_movm_epi32(_mm512_cmp_ps_mask(x.v, y.v, _CMP_NEQ_OQ)));
}
KFR_INTRINSIC f64avx512 ne(const f64avx512& x, const f64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_movm_epi64(_mm512_cmp_pd_mask(x.v, y.v, _CMP_NEQ_OQ)));
}
KFR_INTRINSIC f32avx512 lt(const f32avx512& x, const f32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_movm_epi32(_mm512_cmp_ps_mask(x.v, y.v, _CMP_LT_OQ)));
}
KFR_INTRINSIC f64avx512 lt(const f64avx512& x, const f64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_movm_epi64(_mm512_cmp_pd_mask(x.v, y.v, _CMP_LT_OQ)));
}
KFR_INTRINSIC f32avx512 gt(const f32avx512& x, const f32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_movm_epi32(_mm512_cmp_ps_mask(x.v, y.v, _CMP_GT_OQ)));
}
KFR_INTRINSIC f64avx512 gt(const f64avx512& x, const f64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_movm_epi64(_mm512_cmp_pd_mask(x.v, y.v, _CMP_GT_OQ)));
}
KFR_INTRINSIC f32avx512 le(const f32avx512& x, const f32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_movm_epi32(_mm512_cmp_ps_mask(x.v, y.v, _CMP_LE_OQ)));
}
KFR_INTRINSIC f64avx512 le(const f64avx512& x, const f64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_movm_epi64(_mm512_cmp_pd_mask(x.v, y.v, _CMP_LE_OQ)));
}
KFR_INTRINSIC f32avx512 ge(const f32avx512& x, const f32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_movm_epi32(_mm512_cmp_ps_mask(x.v, y.v, _CMP_GE_OQ)));
}
KFR_INTRINSIC f64avx512 ge(const f64avx512& x, const f64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_movm_epi64(_mm512_cmp_pd_mask(x.v, y.v, _CMP_GE_OQ)));
}

KFR_INTRINSIC f32avx512 band(const f32avx512& x, const f32avx512& y) { return _mm512_and_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 band(const f64avx512& x, const f64avx512& y) { return _mm512_and_pd(x.v, y.v); }
KFR_INTRINSIC f32avx512 bor(const f32avx512& x, const f32avx512& y) { return _mm512_or_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 bor(const f64avx512& x, const f64avx512& y) { return _mm512_or_pd(x.v, y.v); }
KFR_INTRINSIC f32avx512 bxor(const f32avx512& x, const f32avx512& y) { return _mm512_xor_ps(x.v, y.v); }
KFR_INTRINSIC f64avx512 bxor(const f64avx512& x, const f64avx512& y) { return _mm512_xor_pd(x.v, y.v); }

#if 1
#define KFR_knot_mask8(x) ((__mmask8)(~((u8)(x))))
#define KFR_knot_mask16(x) ((__mmask16)(~((u16)(x))))
#define KFR_knot_mask32(x) ((__mmask32)(~((u32)(x))))
#define KFR_knot_mask64(x) ((__mmask64)(~((u64)(x))))
#else
#define KFR_knot_mask8(x) _knot_mask8(x)
#define KFR_knot_mask16(x) _knot_mask16(x)
#define KFR_knot_mask32(x) _knot_mask32(x)
#define KFR_knot_mask64(x) _knot_mask64(x)
#endif

KFR_INTRINSIC i8avx512 eq(const i8avx512& x, const i8avx512& y)
{
    return _mm512_movm_epi8(_mm512_cmpeq_epi8_mask(x.v, y.v));
}
KFR_INTRINSIC i16avx512 eq(const i16avx512& x, const i16avx512& y)
{
    return _mm512_movm_epi16(_mm512_cmpeq_epi16_mask(x.v, y.v));
}
KFR_INTRINSIC i32avx512 eq(const i32avx512& x, const i32avx512& y)
{
    return _mm512_movm_epi32(_mm512_cmpeq_epi32_mask(x.v, y.v));
}
KFR_INTRINSIC i64avx512 eq(const i64avx512& x, const i64avx512& y)
{
    return _mm512_movm_epi64(_mm512_cmpeq_epi64_mask(x.v, y.v));
}
KFR_INTRINSIC i8avx512 ne(const i8avx512& x, const i8avx512& y)
{
    return _mm512_movm_epi8(KFR_knot_mask64(_mm512_cmpeq_epi8_mask(x.v, y.v)));
}
KFR_INTRINSIC i16avx512 ne(const i16avx512& x, const i16avx512& y)
{
    return _mm512_movm_epi16(KFR_knot_mask32(_mm512_cmpeq_epi16_mask(x.v, y.v)));
}
KFR_INTRINSIC i32avx512 ne(const i32avx512& x, const i32avx512& y)
{
    return _mm512_movm_epi32(KFR_knot_mask16(_mm512_cmpeq_epi32_mask(x.v, y.v)));
}
KFR_INTRINSIC i64avx512 ne(const i64avx512& x, const i64avx512& y)
{
    return _mm512_movm_epi64(KFR_knot_mask8(_mm512_cmpeq_epi64_mask(x.v, y.v)));
}
KFR_INTRINSIC i8avx512 ge(const i8avx512& x, const i8avx512& y)
{
    return _mm512_movm_epi8(KFR_knot_mask64(_mm512_cmplt_epi8_mask(x.v, y.v)));
}
KFR_INTRINSIC i16avx512 ge(const i16avx512& x, const i16avx512& y)
{
    return _mm512_movm_epi16(KFR_knot_mask32(_mm512_cmplt_epi16_mask(x.v, y.v)));
}
KFR_INTRINSIC i32avx512 ge(const i32avx512& x, const i32avx512& y)
{
    return _mm512_movm_epi32(KFR_knot_mask16(_mm512_cmplt_epi32_mask(x.v, y.v)));
}
KFR_INTRINSIC i64avx512 ge(const i64avx512& x, const i64avx512& y)
{
    return _mm512_movm_epi64(KFR_knot_mask8(_mm512_cmplt_epi64_mask(x.v, y.v)));
}
KFR_INTRINSIC i8avx512 lt(const i8avx512& x, const i8avx512& y)
{
    return _mm512_movm_epi8(_mm512_cmplt_epi8_mask(x.v, y.v));
}
KFR_INTRINSIC i16avx512 lt(const i16avx512& x, const i16avx512& y)
{
    return _mm512_movm_epi16(_mm512_cmplt_epi16_mask(x.v, y.v));
}
KFR_INTRINSIC i32avx512 lt(const i32avx512& x, const i32avx512& y)
{
    return _mm512_movm_epi32(_mm512_cmplt_epi32_mask(x.v, y.v));
}
KFR_INTRINSIC i64avx512 lt(const i64avx512& x, const i64avx512& y)
{
    return _mm512_movm_epi64(_mm512_cmplt_epi64_mask(x.v, y.v));
}
KFR_INTRINSIC i8avx512 le(const i8avx512& x, const i8avx512& y)
{
    return _mm512_movm_epi8(KFR_knot_mask64(_mm512_cmplt_epi8_mask(y.v, x.v)));
}
KFR_INTRINSIC i16avx512 le(const i16avx512& x, const i16avx512& y)
{
    return _mm512_movm_epi16(KFR_knot_mask32(_mm512_cmplt_epi16_mask(y.v, x.v)));
}
KFR_INTRINSIC i32avx512 le(const i32avx512& x, const i32avx512& y)
{
    return _mm512_movm_epi32(KFR_knot_mask16(_mm512_cmplt_epi32_mask(y.v, x.v)));
}
KFR_INTRINSIC i64avx512 le(const i64avx512& x, const i64avx512& y)
{
    return _mm512_movm_epi64(KFR_knot_mask8(_mm512_cmplt_epi64_mask(y.v, x.v)));
}
KFR_INTRINSIC i8avx512 gt(const i8avx512& x, const i8avx512& y)
{
    return _mm512_movm_epi8(_mm512_cmplt_epi8_mask(y.v, x.v));
}
KFR_INTRINSIC i16avx512 gt(const i16avx512& x, const i16avx512& y)
{
    return _mm512_movm_epi16(_mm512_cmplt_epi16_mask(y.v, x.v));
}
KFR_INTRINSIC i32avx512 gt(const i32avx512& x, const i32avx512& y)
{
    return _mm512_movm_epi32(_mm512_cmplt_epi32_mask(y.v, x.v));
}
KFR_INTRINSIC i64avx512 gt(const i64avx512& x, const i64avx512& y)
{
    return _mm512_movm_epi64(_mm512_cmplt_epi64_mask(y.v, x.v));
}

KFR_INTRINSIC u8avx512 eq(const u8avx512& x, const u8avx512& y)
{
    return _mm512_movm_epi8(_mm512_cmpeq_epu8_mask(x.v, y.v));
}
KFR_INTRINSIC u16avx512 eq(const u16avx512& x, const u16avx512& y)
{
    return _mm512_movm_epi16(_mm512_cmpeq_epu16_mask(x.v, y.v));
}
KFR_INTRINSIC u32avx512 eq(const u32avx512& x, const u32avx512& y)
{
    return _mm512_movm_epi32(_mm512_cmpeq_epu32_mask(x.v, y.v));
}
KFR_INTRINSIC u64avx512 eq(const u64avx512& x, const u64avx512& y)
{
    return _mm512_movm_epi64(_mm512_cmpeq_epu64_mask(x.v, y.v));
}
KFR_INTRINSIC u8avx512 ne(const u8avx512& x, const u8avx512& y)
{
    return _mm512_movm_epi8(KFR_knot_mask64(_mm512_cmpeq_epu8_mask(x.v, y.v)));
}
KFR_INTRINSIC u16avx512 ne(const u16avx512& x, const u16avx512& y)
{
    return _mm512_movm_epi16(KFR_knot_mask32(_mm512_cmpeq_epu16_mask(x.v, y.v)));
}
KFR_INTRINSIC u32avx512 ne(const u32avx512& x, const u32avx512& y)
{
    return _mm512_movm_epi32(KFR_knot_mask16(_mm512_cmpeq_epu32_mask(x.v, y.v)));
}
KFR_INTRINSIC u64avx512 ne(const u64avx512& x, const u64avx512& y)
{
    return _mm512_movm_epi64(KFR_knot_mask8(_mm512_cmpeq_epu64_mask(x.v, y.v)));
}
KFR_INTRINSIC u8avx512 ge(const u8avx512& x, const u8avx512& y)
{
    return _mm512_movm_epi8(KFR_knot_mask64(_mm512_cmplt_epu8_mask(x.v, y.v)));
}
KFR_INTRINSIC u16avx512 ge(const u16avx512& x, const u16avx512& y)
{
    return _mm512_movm_epi16(KFR_knot_mask32(_mm512_cmplt_epu16_mask(x.v, y.v)));
}
KFR_INTRINSIC u32avx512 ge(const u32avx512& x, const u32avx512& y)
{
    return _mm512_movm_epi32(KFR_knot_mask16(_mm512_cmplt_epu32_mask(x.v, y.v)));
}
KFR_INTRINSIC u64avx512 ge(const u64avx512& x, const u64avx512& y)
{
    return _mm512_movm_epi64(KFR_knot_mask8(_mm512_cmplt_epu64_mask(x.v, y.v)));
}
KFR_INTRINSIC u8avx512 lt(const u8avx512& x, const u8avx512& y)
{
    return _mm512_movm_epi8(_mm512_cmplt_epu8_mask(x.v, y.v));
}
KFR_INTRINSIC u16avx512 lt(const u16avx512& x, const u16avx512& y)
{
    return _mm512_movm_epi16(_mm512_cmplt_epu16_mask(x.v, y.v));
}
KFR_INTRINSIC u32avx512 lt(const u32avx512& x, const u32avx512& y)
{
    return _mm512_movm_epi32(_mm512_cmplt_epu32_mask(x.v, y.v));
}
KFR_INTRINSIC u64avx512 lt(const u64avx512& x, const u64avx512& y)
{
    return _mm512_movm_epi64(_mm512_cmplt_epu64_mask(x.v, y.v));
}
KFR_INTRINSIC u8avx512 le(const u8avx512& x, const u8avx512& y)
{
    return _mm512_movm_epi8(KFR_knot_mask64(_mm512_cmplt_epu8_mask(y.v, x.v)));
}
KFR_INTRINSIC u16avx512 le(const u16avx512& x, const u16avx512& y)
{
    return _mm512_movm_epi16(KFR_knot_mask32(_mm512_cmplt_epu16_mask(y.v, x.v)));
}
KFR_INTRINSIC u32avx512 le(const u32avx512& x, const u32avx512& y)
{
    return _mm512_movm_epi32(KFR_knot_mask16(_mm512_cmplt_epu32_mask(y.v, x.v)));
}
KFR_INTRINSIC u64avx512 le(const u64avx512& x, const u64avx512& y)
{
    return _mm512_movm_epi64(KFR_knot_mask8(_mm512_cmplt_epu64_mask(y.v, x.v)));
}
KFR_INTRINSIC u8avx512 gt(const u8avx512& x, const u8avx512& y)
{
    return _mm512_movm_epi8(_mm512_cmplt_epu8_mask(y.v, x.v));
}
KFR_INTRINSIC u16avx512 gt(const u16avx512& x, const u16avx512& y)
{
    return _mm512_movm_epi16(_mm512_cmplt_epu16_mask(y.v, x.v));
}
KFR_INTRINSIC u32avx512 gt(const u32avx512& x, const u32avx512& y)
{
    return _mm512_movm_epi32(_mm512_cmplt_epu32_mask(y.v, x.v));
}
KFR_INTRINSIC u64avx512 gt(const u64avx512& x, const u64avx512& y)
{
    return _mm512_movm_epi64(_mm512_cmplt_epu64_mask(y.v, x.v));
}

KFR_INTRINSIC i8avx512 add(const i8avx512& x, const i8avx512& y) { return _mm512_add_epi8(x.v, y.v); }
KFR_INTRINSIC i16avx512 add(const i16avx512& x, const i16avx512& y) { return _mm512_add_epi16(x.v, y.v); }
KFR_INTRINSIC i32avx512 add(const i32avx512& x, const i32avx512& y) { return _mm512_add_epi32(x.v, y.v); }
KFR_INTRINSIC i64avx512 add(const i64avx512& x, const i64avx512& y) { return _mm512_add_epi64(x.v, y.v); }
KFR_INTRINSIC u8avx512 add(const u8avx512& x, const u8avx512& y) { return _mm512_add_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 add(const u16avx512& x, const u16avx512& y) { return _mm512_add_epi16(x.v, y.v); }
KFR_INTRINSIC u32avx512 add(const u32avx512& x, const u32avx512& y) { return _mm512_add_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx512 add(const u64avx512& x, const u64avx512& y) { return _mm512_add_epi64(x.v, y.v); }

KFR_INTRINSIC i8avx512 sub(const i8avx512& x, const i8avx512& y) { return _mm512_sub_epi8(x.v, y.v); }
KFR_INTRINSIC i16avx512 sub(const i16avx512& x, const i16avx512& y) { return _mm512_sub_epi16(x.v, y.v); }
KFR_INTRINSIC i32avx512 sub(const i32avx512& x, const i32avx512& y) { return _mm512_sub_epi32(x.v, y.v); }
KFR_INTRINSIC i64avx512 sub(const i64avx512& x, const i64avx512& y) { return _mm512_sub_epi64(x.v, y.v); }
KFR_INTRINSIC u8avx512 sub(const u8avx512& x, const u8avx512& y) { return _mm512_sub_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 sub(const u16avx512& x, const u16avx512& y) { return _mm512_sub_epi16(x.v, y.v); }
KFR_INTRINSIC u32avx512 sub(const u32avx512& x, const u32avx512& y) { return _mm512_sub_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx512 sub(const u64avx512& x, const u64avx512& y) { return _mm512_sub_epi64(x.v, y.v); }

KFR_INTRINSIC __m512i mul_epi8(const __m512i& x, const __m512i& y)
{
    const __m512i even = _mm512_mullo_epi16(x, y);
    const __m512i odd  = _mm512_mullo_epi16(_mm512_srli_epi16(x, 8), _mm512_srli_epi16(y, 8));
    return _mm512_or_si512(_mm512_slli_epi16(odd, 8), _mm512_srli_epi16(_mm512_slli_epi16(even, 8), 8));
}

KFR_INTRINSIC i8avx512 mul(const i8avx512& x, const i8avx512& y) { return mul_epi8(x.v, y.v); }
KFR_INTRINSIC i16avx512 mul(const i16avx512& x, const i16avx512& y) { return _mm512_mullo_epi16(x.v, y.v); }
KFR_INTRINSIC i32avx512 mul(const i32avx512& x, const i32avx512& y) { return _mm512_mullo_epi32(x.v, y.v); }
KFR_INTRINSIC i64avx512 mul(const i64avx512& x, const i64avx512& y) { return _mm512_mullo_epi64(x.v, y.v); }
KFR_INTRINSIC u8avx512 mul(const u8avx512& x, const u8avx512& y) { return mul_epi8(x.v, y.v); }
KFR_INTRINSIC u16avx512 mul(const u16avx512& x, const u16avx512& y) { return _mm512_mullo_epi16(x.v, y.v); }
KFR_INTRINSIC u32avx512 mul(const u32avx512& x, const u32avx512& y) { return _mm512_mullo_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx512 mul(const u64avx512& x, const u64avx512& y) { return _mm512_mullo_epi64(x.v, y.v); }

KFR_DIV_MOD_FN(i8avx512)
KFR_DIV_MOD_FN(i16avx512)
KFR_DIV_MOD_FN(i32avx512)
KFR_DIV_MOD_FN(i64avx512)
KFR_DIV_MOD_FN(u8avx512)
KFR_DIV_MOD_FN(u16avx512)
KFR_DIV_MOD_FN(u32avx512)
KFR_DIV_MOD_FN(u64avx512)

KFR_INTRINSIC i8avx512 band(const i8avx512& x, const i8avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC i16avx512 band(const i16avx512& x, const i16avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC i32avx512 band(const i32avx512& x, const i32avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC i64avx512 band(const i64avx512& x, const i64avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC u8avx512 band(const u8avx512& x, const u8avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC u16avx512 band(const u16avx512& x, const u16avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC u32avx512 band(const u32avx512& x, const u32avx512& y) { return _mm512_and_si512(x.v, y.v); }
KFR_INTRINSIC u64avx512 band(const u64avx512& x, const u64avx512& y) { return _mm512_and_si512(x.v, y.v); }

KFR_INTRINSIC i8avx512 bor(const i8avx512& x, const i8avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC i16avx512 bor(const i16avx512& x, const i16avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC i32avx512 bor(const i32avx512& x, const i32avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC i64avx512 bor(const i64avx512& x, const i64avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC u8avx512 bor(const u8avx512& x, const u8avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC u16avx512 bor(const u16avx512& x, const u16avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC u32avx512 bor(const u32avx512& x, const u32avx512& y) { return _mm512_or_si512(x.v, y.v); }
KFR_INTRINSIC u64avx512 bor(const u64avx512& x, const u64avx512& y) { return _mm512_or_si512(x.v, y.v); }

KFR_INTRINSIC i8avx512 bxor(const i8avx512& x, const i8avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC i16avx512 bxor(const i16avx512& x, const i16avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC i32avx512 bxor(const i32avx512& x, const i32avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC i64avx512 bxor(const i64avx512& x, const i64avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC u8avx512 bxor(const u8avx512& x, const u8avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC u16avx512 bxor(const u16avx512& x, const u16avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC u32avx512 bxor(const u32avx512& x, const u32avx512& y) { return _mm512_xor_si512(x.v, y.v); }
KFR_INTRINSIC u64avx512 bxor(const u64avx512& x, const u64avx512& y) { return _mm512_xor_si512(x.v, y.v); }

KFR_INTRINSIC f32avx512 shl(const f32avx512& x, unsigned y)
{
    return _mm512_castsi512_ps(_mm512_slli_epi32(_mm512_castps_si512(x.v), y));
}
KFR_INTRINSIC f64avx512 shl(const f64avx512& x, unsigned y)
{
    return _mm512_castsi512_pd(_mm512_slli_epi64(_mm512_castpd_si512(x.v), y));
}
KFR_INTRINSIC f32avx512 shr(const f32avx512& x, unsigned y)
{
    return _mm512_castsi512_ps(_mm512_srli_epi32(_mm512_castps_si512(x.v), y));
}
KFR_INTRINSIC f64avx512 shr(const f64avx512& x, unsigned y)
{
    return _mm512_castsi512_pd(_mm512_srli_epi64(_mm512_castpd_si512(x.v), y));
}

KFR_INTRINSIC u16avx512 shl(const u16avx512& x, unsigned y) { return _mm512_slli_epi16(x.v, y); }
KFR_INTRINSIC u32avx512 shl(const u32avx512& x, unsigned y) { return _mm512_slli_epi32(x.v, y); }
KFR_INTRINSIC i16avx512 shl(const i16avx512& x, unsigned y) { return _mm512_slli_epi16(x.v, y); }
KFR_INTRINSIC i32avx512 shl(const i32avx512& x, unsigned y) { return _mm512_slli_epi32(x.v, y); }
KFR_INTRINSIC u16avx512 shr(const u16avx512& x, unsigned y) { return _mm512_srli_epi16(x.v, y); }
KFR_INTRINSIC u32avx512 shr(const u32avx512& x, unsigned y) { return _mm512_srli_epi32(x.v, y); }
KFR_INTRINSIC i16avx512 shr(const i16avx512& x, unsigned y) { return _mm512_srai_epi16(x.v, y); }
KFR_INTRINSIC i32avx512 shr(const i32avx512& x, unsigned y) { return _mm512_srai_epi32(x.v, y); }

KFR_INTRINSIC u64avx512 shl(const u64avx512& x, unsigned y) { return _mm512_slli_epi64(x.v, y); }
KFR_INTRINSIC u64avx512 shr(const u64avx512& x, unsigned y) { return _mm512_srli_epi64(x.v, y); }
KFR_INTRINSIC i64avx512 shl(const i64avx512& x, unsigned y) { return _mm512_slli_epi64(x.v, y); }
KFR_INTRINSIC i64avx512 shr(const i64avx512& x, unsigned y)
{
    KFR_COMPONENTWISE_RET_I(u64avx512, result[i] = x[i] >> y);
}

KFR_INTRINSIC u8avx512 shl(const u8avx512& x, unsigned y)
{
    __m512i l  = _mm512_unpacklo_epi8(_mm512_setzero_si512(), x.v);
    __m512i h  = _mm512_unpackhi_epi8(_mm512_setzero_si512(), x.v);
    __m512i ll = _mm512_slli_epi16(l, y);
    __m512i hh = _mm512_slli_epi16(h, y);

    return _mm512_packs_epi16(ll, hh);
}
KFR_INTRINSIC i8avx512 shl(const i8avx512& x, unsigned y)
{
    __m512i l  = _mm512_unpacklo_epi8(_mm512_setzero_si512(), x.v);
    __m512i h  = _mm512_unpackhi_epi8(_mm512_setzero_si512(), x.v);
    __m512i ll = _mm512_slli_epi16(l, y);
    __m512i hh = _mm512_slli_epi16(h, y);

    return _mm512_packs_epi16(ll, hh);
}
KFR_INTRINSIC u8avx512 shr(const u8avx512& x, unsigned y)
{
    __m512i l  = _mm512_unpacklo_epi8(_mm512_setzero_si512(), x.v);
    __m512i h  = _mm512_unpackhi_epi8(_mm512_setzero_si512(), x.v);
    __m512i ll = _mm512_srli_epi16(l, y);
    __m512i hh = _mm512_srli_epi16(h, y);

    return _mm512_packs_epi16(ll, hh);
}
KFR_INTRINSIC i8avx512 shr(const i8avx512& x, unsigned y)
{
    __m512i l  = _mm512_unpacklo_epi8(_mm512_setzero_si512(), x.v);
    __m512i h  = _mm512_unpackhi_epi8(_mm512_setzero_si512(), x.v);
    __m512i ll = _mm512_srai_epi16(l, y);
    __m512i hh = _mm512_srai_epi16(h, y);

    return _mm512_packs_epi16(ll, hh);
}

KFR_INTRINSIC u32avx512 shl(const u32avx512& x, const u32avx512& y) { return _mm512_sllv_epi32(x.v, y.v); }
KFR_INTRINSIC i32avx512 shl(const i32avx512& x, const u32avx512& y) { return _mm512_sllv_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx512 shl(const u64avx512& x, const u64avx512& y) { return _mm512_sllv_epi64(x.v, y.v); }
KFR_INTRINSIC i64avx512 shl(const i64avx512& x, const u64avx512& y) { return _mm512_sllv_epi64(x.v, y.v); }

KFR_INTRINSIC u32avx512 shr(const u32avx512& x, const u32avx512& y) { return _mm512_srlv_epi32(x.v, y.v); }
KFR_INTRINSIC i32avx512 shr(const i32avx512& x, const u32avx512& y) { return _mm512_srav_epi32(x.v, y.v); }
KFR_INTRINSIC u64avx512 shr(const u64avx512& x, const u64avx512& y) { return _mm512_srlv_epi64(x.v, y.v); }
KFR_INTRINSIC i64avx512 shr(const i64avx512& x, const u64avx512& y) { return _mm512_srav_epi64(x.v, y.v); }

KFR_INTRINSIC f32avx512 shl(const f32avx512& x, const u32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_sllv_epi32(_mm512_castps_si512(x.v), y.v));
}
KFR_INTRINSIC f64avx512 shl(const f64avx512& x, const u64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_sllv_epi64(_mm512_castpd_si512(x.v), y.v));
}
KFR_INTRINSIC f32avx512 shr(const f32avx512& x, const u32avx512& y)
{
    return _mm512_castsi512_ps(_mm512_srlv_epi32(_mm512_castps_si512(x.v), y.v));
}
KFR_INTRINSIC f64avx512 shr(const f64avx512& x, const u64avx512& y)
{
    return _mm512_castsi512_pd(_mm512_srlv_epi64(_mm512_castpd_si512(x.v), y.v));
}

#endif

#endif

#endif

#define KFR_HANDLE_ALL_SIZES_SHIFT_2(fn)                                                                     \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N) && is_simd_type<T>)>                  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const unsigned b)                                         \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), b));                                                           \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T> && is_simd_type<T>), typename = void>  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const unsigned b)                                         \
    {                                                                                                        \
        return concat(fn(low(a), b), fn(high(a), b));                                                        \
    }
#define KFR_HANDLE_ALL_SIZES_SHIFT_VAR_2(fn)                                                                 \
    template <typename T, size_t N,                                                                          \
              KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N) && is_simd_type<T>)>                  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const vec<utype<T>, N>& b)                                \
    {                                                                                                        \
        return slice<0, N>(fn(expand_simd(a), expand_simd(b)));                                              \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T> && is_simd_type<T>), typename = void>  \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& a, const vec<utype<T>, N>& b)                                \
    {                                                                                                        \
        return concat(fn(low(a), low(b)), fn(high(a), high(b)));                                             \
    }

KFR_HANDLE_ALL_SIZES_2(add)
KFR_HANDLE_ALL_SIZES_2(sub)
KFR_HANDLE_ALL_SIZES_2(mul)
KFR_HANDLE_ALL_SIZES_2(div)
KFR_HANDLE_ALL_SIZES_2(mod)

KFR_HANDLE_ALL_SIZES_2(eq)
KFR_HANDLE_ALL_SIZES_2(ne)
KFR_HANDLE_ALL_SIZES_2(lt)
KFR_HANDLE_ALL_SIZES_2(gt)
KFR_HANDLE_ALL_SIZES_2(le)
KFR_HANDLE_ALL_SIZES_2(ge)

KFR_HANDLE_ALL_SIZES_2(band)
KFR_HANDLE_ALL_SIZES_2(bor)
KFR_HANDLE_ALL_SIZES_2(bxor)

KFR_HANDLE_ALL_SIZES_SHIFT_2(shl)
KFR_HANDLE_ALL_SIZES_SHIFT_2(shr)
KFR_HANDLE_ALL_SIZES_SHIFT_VAR_2(shl)
KFR_HANDLE_ALL_SIZES_SHIFT_VAR_2(shr)

#else

template <typename T, size_t N, typename = decltype(uibitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shl(const vec<T, N>& x, const vec<utype<T>, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<uitype<T>>(uibitcast(x[i]) << y[i])));
}
template <typename T, size_t N, typename = decltype(uibitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shl(const vec<T, N>& x, unsigned y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<uitype<T>>(uibitcast(x[i]) << y)));
}
template <typename T, size_t N, typename = decltype(uibitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shr(const vec<T, N>& x, const vec<utype<T>, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<uitype<T>>(uibitcast(x[i]) >> y[i])));
}
template <typename T, size_t N, typename = decltype(uibitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> shr(const vec<T, N>& x, unsigned y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<uitype<T>>(uibitcast(x[i]) >> y)));
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> eq(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = maskbits<T>(x[i] == y[i]));
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> ne(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = maskbits<T>(x[i] != y[i]));
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> ge(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = maskbits<T>(x[i] >= y[i]));
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> le(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = maskbits<T>(x[i] <= y[i]));
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> gt(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = maskbits<T>(x[i] > y[i]));
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> lt(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = maskbits<T>(x[i] < y[i]));
}

template <typename T, size_t N, typename = decltype(ubitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> bor(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<utype<T>>((ubitcast(x[i]) | ubitcast(y[i])))));
}
template <typename T, size_t N, typename = decltype(ubitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> bxor(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<utype<T>>(ubitcast(x[i]) ^ ubitcast(y[i]))));
}
template <typename T, size_t N, typename = decltype(ubitcast(T())), KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> band(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = bitcast<T>(static_cast<utype<T>>(ubitcast(x[i]) & ubitcast(y[i]))));
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> add(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = x[i] + y[i]);
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> sub(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = x[i] - y[i]);
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> mul(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = x[i] * y[i]);
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> div(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = x[i] / y[i]);
}
template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>
KFR_INTRINSIC vec<T, N> mod(const vec<T, N>& x, const vec<T, N>& y)
{
    KFR_COMPONENTWISE_RET(result[i] = x[i] % y[i]);
}

#define KFR_HANDLE_VEC_SCA(fn)                                                                               \
    template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>                                          \
    KFR_INTRINSIC vec<T, N> fn(const vec<T, N>& x, const T& y)                                               \
    {                                                                                                        \
        return fn(x, vec<T, N>(y));                                                                          \
    }                                                                                                        \
    template <typename T, size_t N, KFR_ENABLE_IF(is_simd_type<T>)>                                          \
    KFR_INTRINSIC vec<T, N> fn(const T& x, const vec<T, N>& y)                                               \
    {                                                                                                        \
        return fn(vec<T, N>(x), y);                                                                          \
    }

KFR_HANDLE_VEC_SCA(add)
KFR_HANDLE_VEC_SCA(sub)
KFR_HANDLE_VEC_SCA(mul)
KFR_HANDLE_VEC_SCA(div)
KFR_HANDLE_VEC_SCA(mod)
KFR_HANDLE_VEC_SCA(band)
KFR_HANDLE_VEC_SCA(bor)
KFR_HANDLE_VEC_SCA(bxor)
KFR_HANDLE_VEC_SCA(eq)
KFR_HANDLE_VEC_SCA(ne)
KFR_HANDLE_VEC_SCA(lt)
KFR_HANDLE_VEC_SCA(gt)
KFR_HANDLE_VEC_SCA(le)
KFR_HANDLE_VEC_SCA(ge)

#endif

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> bnot(const vec<T, N>& x)
{
    return bxor(special_constants<T>::allones(), x);
}

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> neg(const vec<T, N>& x)
{
    return sub(T(0), x);
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> neg(const vec<T, N>& x)
{
    return bxor(special_constants<T>::highbitmask(), x);
}

template <typename T, size_t N>
KFR_INTRINSIC vec<bit<T>, N> bxor(const vec<bit<T>, N>& x, const vec<bit<T>, N>& y)
{
    return bxor(vec<T, N>(x.v), vec<T, N>(y.v)).v;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<bit<T>, N> bor(const vec<bit<T>, N>& x, const vec<bit<T>, N>& y)
{
    return bor(vec<T, N>(x.v), vec<T, N>(y.v)).v;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<bit<T>, N> band(const vec<bit<T>, N>& x, const vec<bit<T>, N>& y)
{
    return band(vec<T, N>(x.v), vec<T, N>(y.v)).v;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<bit<T>, N> bnot(const vec<bit<T>, N>& x)
{
    return bnot(vec<T, N>(x.v)).v;
}

} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_MSVC(warning(pop))
