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
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_INTRINSICS

#if defined CMT_ARCH_SSE41

// horizontal OR
KFR_INTRINSIC bool bittestany(const mu8sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mu16sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mu32sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mu64sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi8sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi16sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi32sse& x) { return !_mm_testz_si128(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi64sse& x) { return !_mm_testz_si128(x.v, x.v); }

// horizontal AND
KFR_INTRINSIC bool bittestall(const mu8sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mu16sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mu32sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mu64sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi8sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi16sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi32sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi64sse& x) { return _mm_testc_si128(x.v, allonesvector(x).v); }
#endif

#if defined CMT_ARCH_AVX
// horizontal OR
KFR_INTRINSIC bool bittestany(const mf32sse& x) { return !_mm_testz_ps(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mf64sse& x) { return !_mm_testz_pd(x.v, x.v); }

KFR_INTRINSIC bool bittestany(const mf32avx& x) { return !_mm256_testz_ps(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mf64avx& x) { return !_mm256_testz_pd(x.v, x.v); }

KFR_INTRINSIC bool bittestany(const mu8avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mu16avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mu32avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mu64avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi8avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi16avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi32avx& x) { return !_mm256_testz_si256(x.v, x.v); }
KFR_INTRINSIC bool bittestany(const mi64avx& x) { return !_mm256_testz_si256(x.v, x.v); }

// horizontal AND
KFR_INTRINSIC bool bittestall(const mf32sse& x) { return _mm_testc_ps(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mf64sse& x) { return _mm_testc_pd(x.v, allonesvector(x).v); }

KFR_INTRINSIC bool bittestall(const mf32avx& x) { return _mm256_testc_ps(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mf64avx& x) { return _mm256_testc_pd(x.v, allonesvector(x).v); }

KFR_INTRINSIC bool bittestall(const mu8avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mu16avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mu32avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mu64avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi8avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi16avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi32avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }
KFR_INTRINSIC bool bittestall(const mi64avx& x) { return _mm256_testc_si256(x.v, allonesvector(x).v); }

#if defined CMT_ARCH_AVX512
// horizontal OR
KFR_INTRINSIC bool bittestany(const mf32avx512& x) { return _mm512_movepi32_mask(_mm512_castps_si512(x.v)); }
KFR_INTRINSIC bool bittestany(const mf64avx512& x) { return _mm512_movepi64_mask(_mm512_castpd_si512(x.v)); }
KFR_INTRINSIC bool bittestany(const mu8avx512& x) { return _mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mu16avx512& x) { return _mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mu32avx512& x) { return _mm512_movepi32_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mu64avx512& x) { return _mm512_movepi64_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mi8avx512& x) { return _mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mi16avx512& x) { return _mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mi32avx512& x) { return _mm512_movepi32_mask(x.v); }
KFR_INTRINSIC bool bittestany(const mi64avx512& x) { return _mm512_movepi64_mask(x.v); }

// horizontal AND
KFR_INTRINSIC bool bittestall(const mf32avx512& x)
{
    return !~_mm512_movepi32_mask(_mm512_castps_si512(x.v));
}
KFR_INTRINSIC bool bittestall(const mf64avx512& x)
{
    return !~_mm512_movepi64_mask(_mm512_castpd_si512(x.v));
}
KFR_INTRINSIC bool bittestall(const mu8avx512& x) { return !~_mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestall(const mu16avx512& x) { return !~_mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestall(const mu32avx512& x) { return !uint16_t(~_mm512_movepi32_mask(x.v)); }
KFR_INTRINSIC bool bittestall(const mu64avx512& x) { return !uint8_t(~_mm512_movepi64_mask(x.v)); }
KFR_INTRINSIC bool bittestall(const mi8avx512& x) { return !~_mm512_movepi8_mask(x.v); }
KFR_INTRINSIC bool bittestall(const mi16avx512& x) { return !~_mm512_movepi16_mask(x.v); }
KFR_INTRINSIC bool bittestall(const mi32avx512& x) { return !uint16_t(~_mm512_movepi32_mask(x.v)); }
KFR_INTRINSIC bool bittestall(const mi64avx512& x) { return !uint8_t(~_mm512_movepi64_mask(x.v)); }

#endif

#elif defined CMT_ARCH_SSE41
KFR_INTRINSIC bool bittestany(const mf32sse& x)
{
    return !_mm_testz_si128(bitcast<bit<u8>>(x).v, bitcast<bit<u8>>(x).v);
}
KFR_INTRINSIC bool bittestany(const mf64sse& x)
{
    return !_mm_testz_si128(bitcast<bit<u8>>(x).v, bitcast<bit<u8>>(x).v);
}
KFR_INTRINSIC bool bittestall(const mf32sse& x)
{
    return _mm_testc_si128(bitcast<bit<u8>>(x).v, allonesvector(bitcast<bit<u8>>(x)).v);
}
KFR_INTRINSIC bool bittestall(const mf64sse& x)
{
    return _mm_testc_si128(bitcast<bit<u8>>(x).v, allonesvector(bitcast<bit<u8>>(x)).v);
}
#endif

#if !defined CMT_ARCH_SSE41

KFR_INTRINSIC bool bittestany(const mf32sse& x) { return _mm_movemask_ps(x.v); }
KFR_INTRINSIC bool bittestany(const mf64sse& x) { return _mm_movemask_pd(x.v); }
KFR_INTRINSIC bool bittestany(const mu8sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mu16sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mu32sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mu64sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mi8sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mi16sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mi32sse& x) { return _mm_movemask_epi8(x.v); }
KFR_INTRINSIC bool bittestany(const mi64sse& x) { return _mm_movemask_epi8(x.v); }

KFR_INTRINSIC bool bittestall(const mf32sse& x) { return !_mm_movemask_ps((~x).v); }
KFR_INTRINSIC bool bittestall(const mf64sse& x) { return !_mm_movemask_pd((~x).v); }
KFR_INTRINSIC bool bittestall(const mu8sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mu16sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mu32sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mu64sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mi8sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mi16sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mi32sse& x) { return !_mm_movemask_epi8((~x).v); }
KFR_INTRINSIC bool bittestall(const mi64sse& x) { return !_mm_movemask_epi8((~x).v); }
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestall(const mask<T, N>& a)
{
    return bittestall(expand_simd(a, bit<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestall(const mask<T, N>& a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestany(const mask<T, N>& a)
{
    return bittestany(expand_simd(a, bit<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestany(const mask<T, N>& a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#elif CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC bool bittestall(const mu32neon& a)
{
    const uint32x2_t tmp = vand_u32(vget_low_u32(a.v), vget_high_u32(a.v));
    return vget_lane_u32(vpmin_u32(tmp, tmp), 0) == 0xFFFFFFFFu;
}

KFR_INTRINSIC bool bittestany(const mu32neon& a)
{
    const uint32x2_t tmp = vorr_u32(vget_low_u32(a.v), vget_high_u32(a.v));
    return vget_lane_u32(vpmax_u32(tmp, tmp), 0) != 0;
}
KFR_INTRINSIC bool bittestany(const mu8neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mu16neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mu64neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mi8neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mi16neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mi32neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mi64neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mf32neon& a) { return bittestany(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestany(const mf64neon& a) { return bittestany(bitcast<bit<u32>>(a)); }

KFR_INTRINSIC bool bittestall(const mu8neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mu16neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mu64neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mi8neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mi16neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mi32neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mi64neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mf32neon& a) { return bittestall(bitcast<bit<u32>>(a)); }
KFR_INTRINSIC bool bittestall(const mf64neon& a) { return bittestall(bitcast<bit<u32>>(a)); }

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestall(const mask<T, N>& a)
{
    return bittestall(expand_simd(a, bit<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestall(const mask<T, N>& a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T>)>
KFR_INTRINSIC bool bittestany(const mask<T, N>& a)
{
    return bittestany(expand_simd(a, bit<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T>), typename = void>
KFR_INTRINSIC bool bittestany(const mask<T, N>& a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#else

template <typename T, size_t N>
KFR_INTRINSIC bitmask<N> getmask(const mask<T, N>& x)
{
    typename bitmask<N>::type val = 0;
    for (size_t i = 0; i < N; i++)
    {
        val |= static_cast<int>(x[i]) << i;
    }
    return val;
}

template <typename T, size_t N>
KFR_INTRINSIC bool bittestany(const mask<T, N>& x)
{
    return getmask(x).value;
}
template <typename T, size_t N>
KFR_INTRINSIC bool bittestany(const mask<T, N>& x, const mask<T, N>& y)
{
    return bittestany(x & y);
}

template <typename T, size_t N>
KFR_INTRINSIC bool bittestall(const mask<T, N>& x)
{
    return !getmask(~x).value;
}
template <typename T, size_t N>
KFR_INTRINSIC bool bittestall(const mask<T, N>& x, const mask<T, N>& y)
{
    return !bittestany(~x & y);
}
#endif
} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
