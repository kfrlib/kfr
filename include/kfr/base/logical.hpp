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

namespace internal
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

struct logical_and
{
    template <typename T1, typename T2>
    auto operator()(T1 x, T2 y) -> decltype(x && y)
    {
        return x && y;
    }

    template <typename T>
    T operator()(initialvalue<T>)
    {
        return T();
    }
};

#if defined CID_ARCH_SSE41

KFR_SINTRIN bool bittestnone(f32sse x, f32sse y) { return _mm_testz_ps(*x, *y); }
KFR_SINTRIN bool bittestnone(f64sse x, f64sse y) { return _mm_testz_pd(*x, *y); }
KFR_SINTRIN bool bittestnone(u8sse x, u8sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(u16sse x, u16sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(u32sse x, u32sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(u64sse x, u64sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(i8sse x, i8sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(i16sse x, i16sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(i32sse x, i32sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(i64sse x, i64sse y) { return _mm_testz_si128(*x, *y); }
KFR_SINTRIN bool bittestnone(f32sse x) { return _mm_testz_ps(*x, *x); }
KFR_SINTRIN bool bittestnone(f64sse x) { return _mm_testz_pd(*x, *x); }
KFR_SINTRIN bool bittestnone(u8sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(u16sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(u32sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(u64sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(i8sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(i16sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(i32sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestnone(i64sse x) { return _mm_testz_si128(*x, *x); }
KFR_SINTRIN bool bittestall(f32sse x, f32sse y) { return _mm_testc_ps(*x, *y); }
KFR_SINTRIN bool bittestall(f64sse x, f64sse y) { return _mm_testc_pd(*x, *y); }
KFR_SINTRIN bool bittestall(u8sse x, u8sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(u16sse x, u16sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(u32sse x, u32sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(u64sse x, u64sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(i8sse x, i8sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(i16sse x, i16sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(i32sse x, i32sse y) { return _mm_testc_si128(*x, *y); }
KFR_SINTRIN bool bittestall(i64sse x, i64sse y) { return _mm_testc_si128(*x, *y); }
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

#if defined CID_ARCH_AVX
KFR_SINTRIN bool bittestnone(f32avx x, f32avx y) { return _mm256_testz_ps(*x, *y); }
KFR_SINTRIN bool bittestnone(f64avx x, f64avx y) { return _mm256_testz_pd(*x, *y); }
KFR_SINTRIN bool bittestnone(f32avx x) { return _mm256_testz_ps(*x, *x); }
KFR_SINTRIN bool bittestnone(f64avx x) { return _mm256_testz_pd(*x, *x); }
KFR_SINTRIN bool bittestnall(f32avx x, f32avx y) { return _mm256_testc_ps(*x, *y); }
KFR_SINTRIN bool bittestnall(f64avx x, f64avx y) { return _mm256_testc_pd(*x, *y); }
KFR_SINTRIN bool bittestnall(f32avx x) { return _mm256_testc_ps(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestnall(f64avx x) { return _mm256_testc_pd(*x, *allonesvector(x)); }
#endif

#if defined CID_ARCH_AVX2
KFR_SINTRIN bool bittestnone(u8avx x, u8avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(u16avx x, u16avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(u32avx x, u32avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(u64avx x, u64avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(i8avx x, i8avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(i16avx x, i16avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(i32avx x, i32avx y) { return _mm256_testz_si256(*x, *y); }
KFR_SINTRIN bool bittestnone(i64avx x, i64avx y) { return _mm256_testz_si256(*x, *y); }

KFR_SINTRIN bool bittestnone(u8avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(u16avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(u32avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(u64avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(i8avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(i16avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(i32avx x) { return _mm256_testz_si256(*x, *x); }
KFR_SINTRIN bool bittestnone(i64avx x) { return _mm256_testz_si256(*x, *x); }

KFR_SINTRIN bool bittestall(u8avx x, u8avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(u16avx x, u16avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(u32avx x, u32avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(u64avx x, u64avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(i8avx x, i8avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(i16avx x, i16avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(i32avx x, i32avx y) { return _mm256_testc_si256(*x, *y); }
KFR_SINTRIN bool bittestall(i64avx x, i64avx y) { return _mm256_testc_si256(*x, *y); }

KFR_SINTRIN bool bittestall(u8avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u16avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u32avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(u64avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i8avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i16avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i32avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
KFR_SINTRIN bool bittestall(i64avx x) { return _mm256_testc_si256(*x, *allonesvector(x)); }
#endif

#elif defined CID_ARCH_SSE2

KFR_SINTRIN bool bittestnone(f32sse x) { return !_mm_movemask_ps(*x); }
KFR_SINTRIN bool bittestnone(f64sse x) { return !_mm_movemask_pd(*x); }
KFR_SINTRIN bool bittestnone(u8sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(u16sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(u32sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(u64sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(i8sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(i16sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(i32sse x) { return !_mm_movemask_epi8(*x); }
KFR_SINTRIN bool bittestnone(i64sse x) { return !_mm_movemask_epi8(*x); }

KFR_SINTRIN bool bittestnone(f32sse x, f32sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(f64sse x, f64sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(u8sse x, u8sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(u16sse x, u16sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(u32sse x, u32sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(u64sse x, u64sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(i8sse x, i8sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(i16sse x, i16sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(i32sse x, i32sse y) { return bittestnone(x & y); }
KFR_SINTRIN bool bittestnone(i64sse x, i64sse y) { return bittestnone(x & y); }

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

KFR_SINTRIN bool bittestall(f32sse x, f32sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(f64sse x, f64sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(u8sse x, u8sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(u16sse x, u16sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(u32sse x, u32sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(u64sse x, u64sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(i8sse x, i8sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(i16sse x, i16sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(i32sse x, i32sse y) { return bittestnone(~x & y); }
KFR_SINTRIN bool bittestall(i64sse x, i64sse y) { return bittestnone(~x & y); }

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
KFR_SINTRIN bool bittestnone(vec<T, N> x)
{
    return !getmask(x).value;
}
template <typename T, size_t N>
KFR_SINTRIN bool bittestnone(vec<T, N> x, vec<T, N> y)
{
    return bittestnone(x & y);
}

template <typename T, size_t N>
KFR_SINTRIN bool bittestall(vec<T, N> x)
{
    return !getmask(~x).value;
}
template <typename T, size_t N>
KFR_SINTRIN bool bittestall(vec<T, N> x, vec<T, N> y)
{
    return bittestnone(~x & y);
}
#endif
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 bittestnone(const T1& x)
{
    return internal::bittestnone(x);
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 bittestall(const T1& x)
{
    return internal::bittestall(x);
}
}
