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

#include "function.hpp"
#include "select.hpp"

namespace kfr
{

namespace internal
{
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> saturated_signed_add(vec<T, N> a, vec<T, N> b)
{
    using UT               = utype<T>;
    constexpr size_t shift = typebits<UT>::bits - 1;
    vec<UT, N> aa        = bitcast<UT>(a);
    vec<UT, N> bb        = bitcast<UT>(b);
    const vec<UT, N> sum = aa + bb;
    aa = (aa >> shift) + static_cast<UT>(std::numeric_limits<T>::max());

    return select(bitcast<T>((aa ^ bb) | ~(bb ^ sum)) >= 0, a, bitcast<T>(sum));
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> saturated_signed_sub(vec<T, N> a, vec<T, N> b)
{
    using UT               = utype<T>;
    constexpr size_t shift = typebits<UT>::bits - 1;
    vec<UT, N> aa         = bitcast<UT>(a);
    vec<UT, N> bb         = bitcast<UT>(b);
    const vec<UT, N> diff = aa - bb;
    aa = (aa >> shift) + static_cast<UT>(std::numeric_limits<T>::max());

    return select(bitcast<T>((aa ^ bb) & (aa ^ diff)) < 0, a, bitcast<T>(diff));
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> saturated_unsigned_add(vec<T, N> a, vec<T, N> b)
{
    const vec<T, N> t = allonesvector(a);
    return select(a > t - b, t, a + b);
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> saturated_unsigned_sub(vec<T, N> a, vec<T, N> b)
{
    return select(a < b, zerovector(a), a - b);
}

#if defined CID_ARCH_SSE2

KFR_SINTRIN u8sse satadd(u8sse x, u8sse y) { return _mm_adds_epu8(*x, *y); }
KFR_SINTRIN i8sse satadd(i8sse x, i8sse y) { return _mm_adds_epi8(*x, *y); }
KFR_SINTRIN u16sse satadd(u16sse x, u16sse y) { return _mm_adds_epu16(*x, *y); }
KFR_SINTRIN i16sse satadd(i16sse x, i16sse y) { return _mm_adds_epi16(*x, *y); }

KFR_SINTRIN u8sse satsub(u8sse x, u8sse y) { return _mm_subs_epu8(*x, *y); }
KFR_SINTRIN i8sse satsub(i8sse x, i8sse y) { return _mm_subs_epi8(*x, *y); }
KFR_SINTRIN u16sse satsub(u16sse x, u16sse y) { return _mm_subs_epu16(*x, *y); }
KFR_SINTRIN i16sse satsub(i16sse x, i16sse y) { return _mm_subs_epi16(*x, *y); }

KFR_SINTRIN i32sse satadd(i32sse a, i32sse b) { return saturated_signed_add(a, b); }
KFR_SINTRIN i64sse satadd(i64sse a, i64sse b) { return saturated_signed_add(a, b); }
KFR_SINTRIN u32sse satadd(u32sse a, u32sse b) { return saturated_unsigned_add(a, b); }
KFR_SINTRIN u64sse satadd(u64sse a, u64sse b) { return saturated_unsigned_add(a, b); }

KFR_SINTRIN i32sse satsub(i32sse a, i32sse b) { return saturated_signed_sub(a, b); }
KFR_SINTRIN i64sse satsub(i64sse a, i64sse b) { return saturated_signed_sub(a, b); }
KFR_SINTRIN u32sse satsub(u32sse a, u32sse b) { return saturated_unsigned_sub(a, b); }
KFR_SINTRIN u64sse satsub(u64sse a, u64sse b) { return saturated_unsigned_sub(a, b); }

#if defined CID_ARCH_AVX2
KFR_SINTRIN u8avx satadd(u8avx x, u8avx y) { return _mm256_adds_epu8(*x, *y); }
KFR_SINTRIN i8avx satadd(i8avx x, i8avx y) { return _mm256_adds_epi8(*x, *y); }
KFR_SINTRIN u16avx satadd(u16avx x, u16avx y) { return _mm256_adds_epu16(*x, *y); }
KFR_SINTRIN i16avx satadd(i16avx x, i16avx y) { return _mm256_adds_epi16(*x, *y); }

KFR_SINTRIN u8avx satsub(u8avx x, u8avx y) { return _mm256_subs_epu8(*x, *y); }
KFR_SINTRIN i8avx satsub(i8avx x, i8avx y) { return _mm256_subs_epi8(*x, *y); }
KFR_SINTRIN u16avx satsub(u16avx x, u16avx y) { return _mm256_subs_epu16(*x, *y); }
KFR_SINTRIN i16avx satsub(i16avx x, i16avx y) { return _mm256_subs_epi16(*x, *y); }
#endif

KFR_HANDLE_ALL_SIZES_2(satadd)
KFR_HANDLE_ALL_SIZES_2(satsub)

#else
// fallback
template <typename T, size_t N, KFR_ENABLE_IF(std::is_signed<T>::value)>
KFR_SINTRIN vec<T, N> satadd(vec<T, N> a, vec<T, N> b)
{
    return saturated_signed_add(a, b);
}
template <typename T, size_t N, KFR_ENABLE_IF(std::is_unsigned<T>::value)>
KFR_SINTRIN vec<T, N> satadd(vec<T, N> a, vec<T, N> b)
{
    return saturated_unsigned_add(a, b);
}
template <typename T, size_t N, KFR_ENABLE_IF(std::is_signed<T>::value)>
KFR_SINTRIN vec<T, N> satsub(vec<T, N> a, vec<T, N> b)
{
    return saturated_signed_sub(a, b);
}
template <typename T, size_t N, KFR_ENABLE_IF(std::is_unsigned<T>::value)>
KFR_SINTRIN vec<T, N> satsub(vec<T, N> a, vec<T, N> b)
{
    return saturated_unsigned_sub(a, b);
}
#endif
KFR_HANDLE_SCALAR_2(satadd)
KFR_I_FN(satadd)
KFR_HANDLE_SCALAR_2(satsub)
KFR_I_FN(satsub)
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INTRIN common_type<T1, T2> satadd(const T1& x, const T2& y)
{
    return internal::satadd(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<internal::fn_satadd, E1, E2> satadd(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}

template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INTRIN common_type<T1, T2> satsub(const T1& x, const T2& y)
{
    return internal::satsub(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INTRIN expr_func<internal::fn_satsub, E1, E2> satsub(E1&& x, E2&& y)
{
    return { {}, std::forward<E1>(x), std::forward<E2>(y) };
}
}
