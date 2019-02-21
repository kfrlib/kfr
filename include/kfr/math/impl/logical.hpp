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

#include "../../math/abs.hpp"
#include "../../simd/impl/function.hpp"
#include "../../simd/operators.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_INTRINSICS

#if defined CMT_ARCH_SSE41

// horizontal OR
KFR_INTRINSIC bool bittestany(const u8sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const u16sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const u32sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const u64sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i8sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i16sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i32sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i64sse& x) { return !_mm_testz_si128(x.v, x.v); }

// horizontal AND
KFR_INTRINSIC bool bittestall(const u8sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const u16sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const u32sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const u64sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i8sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i16sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i32sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i64sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
#endif

#if defined CMT_ARCH_AVX
// horizontal OR
KFR_INTRINSIC bool bittestany(const f32sse& x) { return !_mm_testz_ps(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const f64sse& x) { return !_mm_testz_pd(x.v, x.v); }

KFR_INTRINSIC bool bittestany(const f32avx& x) { return !_mm256_testz_ps(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const f64avx& x) { return !_mm256_testz_pd(x.v, x.v); }

KFR_INTRINSIC bool bittestany(const u8avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const u16avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const u32avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const u64avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i8avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i16avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i32avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const i64avx& x) { return !_mm256_testz_si256(x.v, x.v); }

// horizontal AND
KFR_INTRINSIC bool bittestall(const f32sse& x) { return _mm_testc_ps(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const f64sse& x) { return _mm_testc_pd(x.v, allonesvector(x).v); }

KFR_INTRINSIC bool bittestall(const f32avx& x) { return _mm256_testc_ps(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const f64avx& x) { return _mm256_testc_pd(x.v, allonesvector(x).v); }

KFR_INTRINSIC bool bittestall(const u8avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const u16avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const u32avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const u64avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i8avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i16avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i32avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const i64avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }

#if defined CMT_ARCH_AVX512
// horizontal OR
KFR_INTRINSIC bool bittestany(const f32avx512& x) { return _mm512_movepi32_mask(_mm512_castps_si512(x.v)); }
KFR_INTRINSIC bool bittestany(const f64avx512& x) { return _mm512_movepi64_mask(_mm512_castpd_si512(x.v)); }
KFR_INTRINSIC bool bittestany(const u8avx512& x) { return _mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestany(const u16avx512& x) { return _mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestany(const u32avx512& x) { return _mm512_movepi32_mask(x.v); }
KFR_INTRINSIC bool bittestany(const u64avx512& x) { return _mm512_movepi64_mask(x.v); }
KFR_INTRINSIC bool bittestany(const i8avx512& x) { return _mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestany(const i16avx512& x) { return _mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestany(const i32avx512& x) { return _mm512_movepi32_mask(x.v); }
KFR_INTRINSIC bool bittestany(const i64avx512& x) { return _mm512_movepi64_mask(x.v); }

// horizontal AND
KFR_INTRINSIC bool bittestall(const f32avx512& x) { return !~_mm512_movepi32_mask(_mm512_castps_si512(x.v)); }
KFR_INTRINSIC bool bittestall(const f64avx512& x) { return !~_mm512_movepi64_mask(_mm512_castpd_si512(x.v)); }
KFR_INTRINSIC bool bittestall(const u8avx512& x) { return !~_mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestall(const u16avx512& x) { return !~_mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestall(const u32avx512& x) { return !uint16_t(~_mm512_movepi32_mask(x.v)); }
KFR_INTRINSIC bool bittestall(const u64avx512& x) { return !uint8_t(~_mm512_movepi64_mask(x.v)); }
KFR_INTRINSIC bool bittestall(const i8avx512& x) { return !~_mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestall(const i16avx512& x) { return !~_mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestall(const i32avx512& x) { return !uint16_t(~_mm512_movepi32_mask(x.v)); }
KFR_INTRINSIC bool bittestall(const i64avx512& x) { return !uint8_t(~_mm512_movepi64_mask(x.v)); }

#endif

#elif defined CMT_ARCH_SSE41
KFR_INTRINSIC bool bittestany(const f32sse& x)
{
    return !_mm_testz_si128(bitcast<u8>(x).v, bitcast<u8>(x).v);
}
KFR_INTRINSIC bool bittestany(const f64sse& x)
{
    return !_mm_testz_si128(bitcast<u8>(x).v, bitcast<u8>(x).v);
}
KFR_INTRINSIC bool bittestall(const f32sse& x)
{
    return _mm_testc_si128(bitcast<u8>(x).v, allonesvector(bitcast<u8>(x)).v);
}
KFR_INTRINSIC bool bittestall(const f64sse& x)
{
    return _mm_testc_si128(bitcast<u8>(x).v, allonesvector(bitcast<u8>(x)).v);
}
#endif

#if !defined CMT_ARCH_SSE41

KFR_INTRINSIC bool bittestany(const f32sse& x) { return _mm_movemask_ps(x.v); }
KFR_INTRINSIC bool bittestany(const f64sse& x) { return _mm_movemask_pd(x.v); }
KFR_INTRINSIC bool bittestany(const u8sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const u16sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const u32sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const u64sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const i8sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const i16sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const i32sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const i64sse& x) { return _mm_movemask_epi8(x.v); }

KFR_INTRINSIC bool bittestall(const f32sse& x) { return !_mm_movemask_ps((~x).v); }
KFR_INTRINSIC bool bittestall(const f64sse& x) { return !_mm_movemask_pd((~x).v); }
KFR_INTRINSIC bool bittestall(const u8sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const u16sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const u32sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const u64sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const i8sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const i16sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const i32sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const i64sse& x) { return !_mm_movemask_epi8((~x).v); }
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestall(const vec<T, N>& a)
{
    return bittestall(expand_simd(a, internal::maskbits<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestall(const vec<T, N>& a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestany(const vec<T, N>& a)
{
    return bittestany(expand_simd(a, internal::maskbits<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestany(const vec<T, N>& a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#elif CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC bool bittestall(const u32neon& a)
{
    const uint32x2_t tmp = vand_u32(vget_low_u32(a.v), vget_high_u32(a.v));
    return vget_lane_u32(vpmin_u32(tmp, tmp), 0) == 0xFFFFFFFFu;
}

KFR_INTRINSIC bool bittestany(const u32neon& a)
{
    const uint32x2_t tmp = vorr_u32(vget_low_u32(a.v), vget_high_u32(a.v));
    return vget_lane_u32(vpmax_u32(tmp, tmp), 0) != 0;
}
KFR_INTRINSIC bool bittestany(const u8neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const u16neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const u64neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const i8neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const i16neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const i64neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const f32neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestany(const f64neon& a) { return bittestany(bitcast<u32>(a)); }

KFR_INTRINSIC bool bittestall(const u8neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const u16neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const u64neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const i8neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const i16neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const i64neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const f32neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_INTRINSIC bool bittestall(const f64neon& a) { return bittestall(bitcast<u32>(a)); }

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestall(const vec<T, N>& a)
{
    return bittestall(expand_simd(a, internal::maskbits<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestall(const vec<T, N>& a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestany(const vec<T, N>& a)
{
    return bittestany(expand_simd(a, internal::maskbits<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestany(const vec<T, N>& a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#else

template <typename T, size_t N>
KFR_INTRINSIC bitmask<N> getmask(const vec<T, N>& x)
{
    typename bitmask<N>::type val = 0;
    for (size_t i = 0; i < N; i++)
    {
        val |= (ubitcast(x[i]) >> (typebits<T>::bits - 1)) << i;
    }
    return val;
}

template <typename T, size_t N>
KFR_INTRINSIC bool bittestany(const vec<T, N>& x)
{
    return getmask(x).value;
}
template <typename T, size_t N>
KFR_INTRINSIC bool bittestany(const vec<T, N>& x, const vec<T, N>& y)
{
    return bittestany(x & y);
}

template <typename T, size_t N>
KFR_INTRINSIC bool bittestall(const vec<T, N>& x)
{
    return !getmask(~x).value;
}
template <typename T, size_t N>
KFR_INTRINSIC bool bittestall(const vec<T, N>& x, const vec<T, N>& y)
{
    return !bittestany(~x & y);
}
#endif
} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
