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

namespace kfr
{

namespace intrinsics
{

template <size_t bits>
struct bitmask
{
    using type = findinttype<0, (1ull << bits) - 1>;

    bitmask(type val) : value(val) {}

    template <typename Itype>
    bitmask(Itype val) : value(static_cast<type>(val))
    {
    }

    type value;
};

#if defined CID_ARCH_SSE2

#if defined CID_ARCH_SSE41

KFR_SINTRIN bool bittestany(f32sse x) { return !_mm_testz_ps(*x, *x); }
KFR_SINTRIN bool bittestany(f64sse x) { return !_mm_testz_pd(*x, *x); }
KFR_SINTRIN bool bittestany(u8sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(u16sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(u32sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(u64sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(i8sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(i16sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(i32sse x) { return !_mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestany(i64sse x) { return !_mm_testz_si128(*x, *x); }

KFR_SINTRIN bool bittestall(f32sse x) { return _mm_testc_ps(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(f64sse x) { return _mm_testc_pd(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u8sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u16sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u32sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u64sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i8sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i16sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i32sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i64sse x) { return _mm_testc_si128(*x, *allonesvector(x)); }
#endif

#if defined CID_ARCH_AVX
KFR_SINTRIN bool bittestany(f32avx x) { return !_mm256_testz_ps(*x, *x); }
KFR_SINTRIN bool bittestany(f64avx x) { return !_mm256_testz_pd(*x, *x); }

KFR_SINTRIN bool bittestnall(f32avx x) { return _mm256_testc_ps(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestnall(f64avx x) { return _mm256_testc_pd(*x, *allonesvector(x)); }

KFR_SINTRIN bool bittestany(u8avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(u16avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(u32avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(u64avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(i8avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(i16avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(i32avx x) { return !_mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestany(i64avx x) { return !_mm256_testz_si256(*x, *x); }

KFR_SINTRIN bool bittestall(u8avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u16avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u32avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u64avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i8avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i16avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i32avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i64avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }

#endif

#if !defined CID_ARCH_SSE41

KFR_SINTRIN bool bittestany(f32sse x) { return _mm_movemask_ps(*x); }
KFR_SINTRIN bool bittestany(f64sse x) { return _mm_movemask_pd(*x); }
KFR_SINTRIN bool bittestany(u8sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(u16sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(u32sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(u64sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(i8sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(i16sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(i32sse x) { return _mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestany(i64sse x) { return _mm_movemask_epi8(*x); }

KFR_SINTRIN bool bittestall(f32sse x) { return !_mm_movemask_ps(*~x); }
KFR_SINTRIN bool bittestall(f64sse x) { return !_mm_movemask_pd(*~x); }
KFR_SINTRIN bool bittestall(u8sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(u16sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(u32sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(u64sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(i8sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(i16sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(i32sse x) { return !_mm_movemask_epi8(*~x); }
KFR_SINTRIN bool bittestall(i64sse x) { return !_mm_movemask_epi8(*~x); }
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>
KFR_SINTRIN bool bittestall(vec<T, N> a)
{
    return bittestall(expand_simd(a, internal::maskbits<T>(true)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>
KFR_SINTRIN bool bittestall(vec<T, N> a)
{
    return bittestall(low(a)) && bittestall(high(a));
}

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>
KFR_SINTRIN bool bittestany(vec<T, N> a)
{
    return bittestany(expand_simd(a, internal::maskbits<T>(false)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>
KFR_SINTRIN bool bittestany(vec<T, N> a)
{
    return bittestany(low(a)) || bittestany(high(a));
}

#else

template <typename T, size_t N>
KFR_SINTRIN bitmask<N> getmask(vec<T, N> x)
{
    typename bitmask<N>::type val = 0;
    for (size_t i = 0; i < N; i++)
    {
        val |= (ubitcast(x[i]) >> (typebits<T>::bits - 1)) << i;
    }
    return val;
}

template <typename T, size_t N>
KFR_SINTRIN bool bittestany(vec<T, N> x)
{
    return getmask(x).value;
}
template <typename T, size_t N>
KFR_SINTRIN bool bittestany(vec<T, N> x, vec<T, N> y)
{
    return bittestany(x & y);
}

template <typename T, size_t N>
KFR_SINTRIN bool bittestall(vec<T, N> x)
{
    return !getmask(~x).value;
}
template <typename T, size_t N>
KFR_SINTRIN bool bittestall(vec<T, N> x, vec<T, N> y)
{
    return !bittestany(~x & y);
}
#endif
}

/// Returns x[0] && x[1] && ... && x[N-1]
template <typename T, size_t N>
KFR_SINTRIN bool all(const mask<T, N>& x)
{
    return intrinsics::bittestall(x.asvec());
}

/// Returns x[0] || x[1] || ... || x[N-1]
template <typename T, size_t N>
KFR_SINTRIN bool any(const mask<T, N>& x)
{
    return intrinsics::bittestany(x.asvec());
}
}
