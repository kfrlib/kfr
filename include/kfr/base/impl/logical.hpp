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

#include "../abs.hpp"
#include "../function.hpp"
#include "../operators.hpp"

namespace kfr
{

namespace intrinsics
{

template <size_t bits>
struct bitmask
{
    using type = conditional<(bits > 32), uint64_t,
                             conditional<(bits > 16), uint32_t, conditional<(bits > 8), uint16_t, uint8_t>>>;

    bitmask(type val) : value(val) {}

    template <typename Itype>
    bitmask(Itype val) : value(static_cast<type>(val))
    {
    }

    type value;
};

#if defined CMT_ARCH_SSE2 && defined KFR_NATIVE_INTRINSICS

#if defined CMT_ARCH_SSE41

// horizontal OR
KFR_SINTRIN bool bittestany(const u8sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const u16sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const u32sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const u64sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const i8sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const i16sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const i32sse& x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(const i64sse& x) { return !_mm_testz_si128(*x, *x); }

// horizontal AND
KFR_SINTRIN bool bittestall(const u8sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const u16sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const u32sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const u64sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i8sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i16sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i32sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i64sse& x) { return _mm_testc_si128(*x, *allonesvector(x)); }
#endif

#if defined CMT_ARCH_AVX
// horizontal OR
KFR_SINTRIN bool bittestany(const f32sse& x) { return !_mm_testz_ps(*x, *x); }
KFR_SINTRIN bool bittestany(const f64sse& x) { return !_mm_testz_pd(*x, *x); }

KFR_SINTRIN bool bittestany(const f32avx& x) { return !_mm256_testz_ps(*x, *x); }
KFR_SINTRIN bool bittestany(const f64avx& x) { return !_mm256_testz_pd(*x, *x); }

KFR_SINTRIN bool bittestany(const u8avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const u16avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const u32avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const u64avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const i8avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const i16avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const i32avx& x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(const i64avx& x) { return !_mm256_testz_si256(*x, *x); }

// horizontal AND
KFR_SINTRIN bool bittestall(const f32sse& x) { return _mm_testc_ps(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const f64sse& x) { return _mm_testc_pd(*x, *allonesvector(x)); }

KFR_SINTRIN bool bittestall(const f32avx& x) { return _mm256_testc_ps(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const f64avx& x) { return _mm256_testc_pd(*x, *allonesvector(x)); }

KFR_SINTRIN bool bittestall(const u8avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const u16avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const u32avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const u64avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i8avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i16avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i32avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(const i64avx& x) { return _mm256_testc_si256(*x, *allonesvector(x)); }

#if defined CMT_ARCH_AVX512
// horizontal OR
KFR_SINTRIN bool bittestany(const f32avx512& x) { return _mm512_test_epi32_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const f64avx512& x) { return _mm512_test_epi64_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const u8avx512& x) { return _mm512_test_epi8_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const u16avx512& x) { return _mm512_test_epi16_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const u32avx512& x) { return _mm512_test_epi32_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const u64avx512& x) { return _mm512_test_epi64_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const i8avx512& x) { return _mm512_test_epi8_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const i16avx512& x) { return _mm512_test_epi16_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const i32avx512& x) { return _mm512_test_epi32_mask(*x, *x); }
KFR_SINTRIN bool bittestany(const i64avx512& x) { return _mm512_test_epi64_mask(*x, *x); }

// horizontal AND
KFR_SINTRIN bool bittestall(const f32avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const f64avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const u8avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const u16avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const u32avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const u64avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const i8avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const i16avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const i32avx512& x) { return !bittestany(~x); }
KFR_SINTRIN bool bittestall(const i64avx512& x) { return !bittestany(~x); }

#endif

#elif defined CMT_ARCH_SSE41
KFR_SINTRIN bool bittestany(const f32sse& x) { return !_mm_testz_si128(*bitcast<u8>(x), *bitcast<u8>(x)); }
KFR_SINTRIN bool bittestany(const f64sse& x) { return !_mm_testz_si128(*bitcast<u8>(x), *bitcast<u8>(x)); }
KFR_SINTRIN bool bittestall(const f32sse& x)
{
    return _mm_testc_si128(*bitcast<u8>(x), *allonesvector(bitcast<u8>(x)));
}
KFR_SINTRIN bool bittestall(const f64sse& x)
{
    return _mm_testc_si128(*bitcast<u8>(x), *allonesvector(bitcast<u8>(x)));
}
#endif

#if !defined CMT_ARCH_SSE41

KFR_SINTRIN bool bittestany(const f32sse& x) { return _mm_movemask_ps(*x); }
KFR_SINTRIN bool bittestany(const f64sse& x) { return _mm_movemask_pd(*x); }
KFR_SINTRIN bool bittestany(const u8sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const u16sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const u32sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const u64sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const i8sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const i16sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const i32sse& x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(const i64sse& x) { return _mm_movemask_epi8(*x); }

KFR_SINTRIN bool bittestall(const f32sse& x) { return !_mm_movemask_ps(*~x); }
KFR_SINTRIN bool bittestall(const f64sse& x) { return !_mm_movemask_pd(*~x); }
KFR_SINTRIN bool bittestall(const u8sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const u16sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const u32sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const u64sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const i8sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const i16sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const i32sse& x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(const i64sse& x) { return !_mm_movemask_epi8(*~x); }
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < platform<T>::vector_width)>
KFR_SINTRIN bool bittestall(const vec<T, N>& a)
{
    return bittestall(expand_simd(a, internal::maskbits<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= platform<T>::vector_width), typename = void>
KFR_SINTRIN bool bittestall(const vec<T, N>& a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < platform<T>::vector_width)>
KFR_SINTRIN bool bittestany(const vec<T, N>& a)
{
    return bittestany(expand_simd(a, internal::maskbits<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= platform<T>::vector_width), typename = void>
KFR_SINTRIN bool bittestany(const vec<T, N>& a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#elif CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_SINTRIN bool bittestall(const u32neon& a)
{
    const uint32x2_t tmp = vand_u32(vget_low_u32(*a), vget_high_u32(*a));
    return vget_lane_u32(vpmin_u32(tmp, tmp), 0) == 0xFFFFFFFFu;
}

KFR_SINTRIN bool bittestany(const u32neon& a)
{
    const uint32x2_t tmp = vorr_u32(vget_low_u32(*a), vget_high_u32(*a));
    return vget_lane_u32(vpmax_u32(tmp, tmp), 0) != 0;
}
KFR_SINTRIN bool bittestany(const u8neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const u16neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const u64neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const i8neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const i16neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const i64neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const f32neon& a) { return bittestany(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestany(const f64neon& a) { return bittestany(bitcast<u32>(a)); }

KFR_SINTRIN bool bittestall(const u8neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const u16neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const u64neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const i8neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const i16neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const i64neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const f32neon& a) { return bittestall(bitcast<u32>(a)); }
KFR_SINTRIN bool bittestall(const f64neon& a) { return bittestall(bitcast<u32>(a)); }

template <typename T, size_t N, KFR_ENABLE_IF(N < platform<T>::vector_width)>
KFR_SINTRIN bool bittestall(const vec<T, N>& a)
{
    return bittestall(expand_simd(a, internal::maskbits<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= platform<T>::vector_width), typename = void>
KFR_SINTRIN bool bittestall(const vec<T, N>& a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < platform<T>::vector_width)>
KFR_SINTRIN bool bittestany(const vec<T, N>& a)
{
    return bittestany(expand_simd(a, internal::maskbits<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= platform<T>::vector_width), typename = void>
KFR_SINTRIN bool bittestany(const vec<T, N>& a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#else

template <typename T, size_t N>
KFR_SINTRIN bitmask<N> getmask(const vec<T, N>& x)
{
    typename bitmask<N>::type val = 0;
    for (size_t i = 0; i < N; i++)
    {
        val |= (ubitcast(x[i]) >> (typebits<T>::bits - 1)) << i;
    }
    return val;
}

template <typename T, size_t N>
KFR_SINTRIN bool bittestany(const vec<T, N>& x)
{
    return getmask(x).value;
}
template <typename T, size_t N>
KFR_SINTRIN bool bittestany(const vec<T, N>& x, const vec<T, N>& y)
{
    return bittestany(x & y);
}

template <typename T, size_t N>
KFR_SINTRIN bool bittestall(const vec<T, N>& x)
{
    return !getmask(~x).value;
}
template <typename T, size_t N>
KFR_SINTRIN bool bittestall(const vec<T, N>& x, const vec<T, N>& y)
{
    return !bittestany(~x & y);
}
#endif
} // namespace intrinsics

} // namespace kfr
