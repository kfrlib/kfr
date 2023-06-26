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
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#if defined CMT_ARCH_SSE41 && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC u8sse select(const mu8sse& m, const u8sse& x, const u8sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC u16sse select(const mu16sse& m, const u16sse& x, const u16sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC u32sse select(const mu32sse& m, const u32sse& x, const u32sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC u64sse select(const mu64sse& m, const u64sse& x, const u64sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i8sse select(const mi8sse& m, const i8sse& x, const i8sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i16sse select(const mi16sse& m, const i16sse& x, const i16sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i32sse select(const mi32sse& m, const i32sse& x, const i32sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i64sse select(const mi64sse& m, const i64sse& x, const i64sse& y)
{
    return _mm_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC f32sse select(const mf32sse& m, const f32sse& x, const f32sse& y)
{
    return _mm_blendv_ps(y.v, x.v, m.v);
}
KFR_INTRINSIC f64sse select(const mf64sse& m, const f64sse& x, const f64sse& y)
{
    return _mm_blendv_pd(y.v, x.v, m.v);
}

#if defined CMT_ARCH_AVX
KFR_INTRINSIC f64avx select(const mf64avx& m, const f64avx& x, const f64avx& y)
{
    return _mm256_blendv_pd(y.v, x.v, m.v);
}
KFR_INTRINSIC f32avx select(const mf32avx& m, const f32avx& x, const f32avx& y)
{
    return _mm256_blendv_ps(y.v, x.v, m.v);
}
#endif

#if defined CMT_ARCH_AVX2
KFR_INTRINSIC u8avx select(const mu8avx& m, const u8avx& x, const u8avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC u16avx select(const mu16avx& m, const u16avx& x, const u16avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC u32avx select(const mu32avx& m, const u32avx& x, const u32avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC u64avx select(const mu64avx& m, const u64avx& x, const u64avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i8avx select(const mi8avx& m, const i8avx& x, const i8avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i16avx select(const mi16avx& m, const i16avx& x, const i16avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i32avx select(const mi32avx& m, const i32avx& x, const i32avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
KFR_INTRINSIC i64avx select(const mi64avx& m, const i64avx& x, const i64avx& y)
{
    return _mm256_blendv_epi8(y.v, x.v, m.v);
}
#endif

#if defined CMT_ARCH_AVX512
KFR_INTRINSIC f64avx512 select(const mf64avx512& m, const f64avx512& x, const f64avx512& y)
{
    return _mm512_mask_blend_pd(_mm512_movepi64_mask(_mm512_castpd_si512(m.v)), y.v, x.v);
}
KFR_INTRINSIC f32avx512 select(const mf32avx512& m, const f32avx512& x, const f32avx512& y)
{
    return _mm512_mask_blend_ps(_mm512_movepi32_mask(_mm512_castps_si512(m.v)), y.v, x.v);
}
KFR_INTRINSIC u8avx512 select(const mu8avx512& m, const u8avx512& x, const u8avx512& y)
{
    return _mm512_mask_blend_epi8(_mm512_movepi8_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC u16avx512 select(const mu16avx512& m, const u16avx512& x, const u16avx512& y)
{
    return _mm512_mask_blend_epi16(_mm512_movepi16_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC u32avx512 select(const mu32avx512& m, const u32avx512& x, const u32avx512& y)
{
    return _mm512_mask_blend_epi32(_mm512_movepi32_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC u64avx512 select(const mu64avx512& m, const u64avx512& x, const u64avx512& y)
{
    return _mm512_mask_blend_epi64(_mm512_movepi64_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC i8avx512 select(const mi8avx512& m, const i8avx512& x, const i8avx512& y)
{
    return _mm512_mask_blend_epi8(_mm512_movepi8_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC i16avx512 select(const mi16avx512& m, const i16avx512& x, const i16avx512& y)
{
    return _mm512_mask_blend_epi16(_mm512_movepi16_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC i32avx512 select(const mi32avx512& m, const i32avx512& x, const i32avx512& y)
{
    return _mm512_mask_blend_epi32(_mm512_movepi32_mask(m.v), y.v, x.v);
}
KFR_INTRINSIC i64avx512 select(const mi64avx512& m, const i64avx512& x, const i64avx512& y)
{
    return _mm512_mask_blend_epi64(_mm512_movepi64_mask(m.v), y.v, x.v);
}
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    constexpr size_t Nout = next_simd_width<T>(N);
    return select(a.shuffle(csizeseq<Nout>), b.shuffle(csizeseq<Nout>), c.shuffle(csizeseq<Nout>))
        .shuffle(csizeseq<N>);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T>), typename = void>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    return concat(select(low(a), low(b), low(c)), select(high(a), high(b), high(c)));
    //    return concat2(select(a.h.low, b.h.low, c.h.low), select(a.h.high, b.h.high, c.h.high));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const T& b, const T& c)
{
    constexpr size_t Nout = next_simd_width<T>(N);
    return select(a.shuffle(csizeseq<Nout>), vec<T, Nout>(b), vec<T, Nout>(c)).shuffle(csizeseq<N>);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T>), typename = void>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const T& b, const T& c)
{
    return concat2(select(a.h.low, b, c), select(a.h.high, b, c));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const vec<T, N>& b, const T& c)
{
    constexpr size_t Nout = next_simd_width<T>(N);
    return select(a.shuffle(csizeseq<Nout>), b.shuffle(csizeseq<Nout>), vec<T, Nout>(c)).shuffle(csizeseq<N>);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T>), typename = void>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const vec<T, N>& b, const T& c)
{
    return concat2(select(a.h.low, b.h.low, c), select(a.h.high, b.h.high, c));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const T& b, const vec<T, N>& c)
{
    constexpr size_t Nout = next_simd_width<T>(N);
    return select(shufflevector(a, csizeseq<Nout>), vec<T, Nout>(b), c.shuffle(csizeseq<Nout>))
        .shuffle(csizeseq<N>);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T>), typename = void>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const T& b, const vec<T, N>& c)
{
    return concat2(select(a.h.low, b, c.h.low), select(a.h.high, b, c.h.high));
}

#elif defined CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC f32neon select(const mf32neon& m, const f32neon& x, const f32neon& y)
{
    return vbslq_f32(m.v, x.v, y.v);
}
KFR_INTRINSIC i8neon select(const mi8neon& m, const i8neon& x, const i8neon& y)
{
    return vbslq_s8(m.v, x.v, y.v);
}
KFR_INTRINSIC u8neon select(const mu8neon& m, const u8neon& x, const u8neon& y)
{
    return vbslq_u8(m.v, x.v, y.v);
}
KFR_INTRINSIC i16neon select(const mi16neon& m, const i16neon& x, const i16neon& y)
{
    return vbslq_s16(m.v, x.v, y.v);
}
KFR_INTRINSIC u16neon select(const mu16neon& m, const u16neon& x, const u16neon& y)
{
    return vbslq_u16(m.v, x.v, y.v);
}
KFR_INTRINSIC i32neon select(const mi32neon& m, const i32neon& x, const i32neon& y)
{
    return vbslq_s32(m.v, x.v, y.v);
}
KFR_INTRINSIC u32neon select(const mu32neon& m, const u32neon& x, const u32neon& y)
{
    return vbslq_u32(m.v, x.v, y.v);
}
KFR_INTRINSIC i64neon select(const mi64neon& m, const i64neon& x, const i64neon& y)
{
    return vbslq_s64(m.v, x.v, y.v);
}
KFR_INTRINSIC u64neon select(const mu64neon& m, const u64neon& x, const u64neon& y)
{
    return vbslq_u64(m.v, x.v, y.v);
}

#ifdef CMT_ARCH_NEON64
KFR_INTRINSIC f64neon select(const mf64neon& m, const f64neon& x, const f64neon& y)
{
    return vbslq_f64(m.v, x.v, y.v);
}
#else
KFR_INTRINSIC f64neon select(const mf64neon& m, const f64neon& x, const f64neon& y)
{
    return y ^ ((x ^ y) & m.asvec());
}
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T> && !is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    constexpr size_t Nout = next_simd_width<T>(N);
    return select(a.shuffle(csizeseq<Nout>), b.shuffle(csizeseq<Nout>), c.shuffle(csizeseq<Nout>))
        .shuffle(csizeseq<N>);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > vector_width<T>), typename = void>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    return concat2(select(a.h.low, b.h.low, c.h.low), select(a.h.high, b.h.high, c.h.high));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const T& x, const T& y)
{
    return select(m, vec<T, N>(x), vec<T, N>(y));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const vec<T, N>& x, const T& y)
{
    return select(m, x, vec<T, N>(y));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const T& x, const vec<T, N>& y)
{
    return select(m, vec<T, N>(x), y);
}

#else

// fallback
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const vec<T, N>& x, const vec<T, N>& y)
{
    return y ^ ((x ^ y) & m.asvec());
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const T& x, const T& y)
{
    return select(m, vec<T, N>(x), vec<T, N>(y));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const vec<T, N>& x, const T& y)
{
    return select(m, x, vec<T, N>(y));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> select(const vec<bit<T>, N>& m, const T& x, const vec<T, N>& y)
{
    return select(m, vec<T, N>(x), y);
}
#endif
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> select(bool m, const T1& x, const T2& y)
{
    return m ? x : y;
}

} // namespace intrinsics
KFR_I_FN(select)
} // namespace CMT_ARCH_NAME
} // namespace kfr
