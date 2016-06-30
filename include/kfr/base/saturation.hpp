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

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

namespace internal
{

template <cpu_t c = cpu_t::native, cpu_t cc = c>
struct in_saturated : in_saturated<older(c), cc>
{
    struct fn_satadd : in_saturated<older(c), cc>::fn_satadd, fn_disabled
    {
    };
};

template <cpu_t cc>
struct in_saturated<cpu_t::sse2, cc> : in_select<cc>
{
    constexpr static cpu_t cpu = cpu_t::sse2;

private:
    using in_select<cc>::select;

public:
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

private:
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> saturated_signed_add(vec<T, N> a, vec<T, N> b)
    {
        constexpr size_t shift = typebits<i32>::bits - 1;
        const vec<T, N> sum = a + b;
        a = (a >> shift) + allonesvector(a);

        return select(((a ^ b) | ~(b ^ sum)) >= 0, a, sum);
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> saturated_signed_sub(vec<T, N> a, vec<T, N> b)
    {
        constexpr size_t shift = typebits<i32>::bits - 1;
        const vec<T, N> diff = a - b;
        a = (a >> shift) + allonesvector(a);

        return select(((a ^ b) & (a ^ diff)) < 0, a, diff);
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> saturated_unsigned_add(vec<T, N> a, vec<T, N> b)
    {
        constexpr vec<T, N> t = allonesvector(a);
        return select(a > t - b, t, a + b);
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> saturated_unsigned_sub(vec<T, N> a, vec<T, N> b)
    {
        return select(a < b, zerovector(a), a - b);
    }

public:
    KFR_HANDLE_ALL(satadd)
    KFR_HANDLE_ALL(satsub)
    KFR_SPEC_FN(in_saturated, satadd)
    KFR_SPEC_FN(in_saturated, satsub)
};

template <cpu_t cc>
struct in_saturated<cpu_t::avx2, cc> : in_saturated<cpu_t::sse2, cc>
{
    constexpr static cpu_t cpu = cpu_t::avx2;
    using in_saturated<cpu_t::sse2, cc>::satadd;
    using in_saturated<cpu_t::sse2, cc>::satsub;

    KFR_SINTRIN u8avx satadd(u8avx x, u8avx y) { return _mm256_adds_epu8(*x, *y); }
    KFR_SINTRIN i8avx satadd(i8avx x, i8avx y) { return _mm256_adds_epi8(*x, *y); }
    KFR_SINTRIN u16avx satadd(u16avx x, u16avx y) { return _mm256_adds_epu16(*x, *y); }
    KFR_SINTRIN i16avx satadd(i16avx x, i16avx y) { return _mm256_adds_epi16(*x, *y); }

    KFR_SINTRIN u8avx satsub(u8avx x, u8avx y) { return _mm256_subs_epu8(*x, *y); }
    KFR_SINTRIN i8avx satsub(i8avx x, i8avx y) { return _mm256_subs_epi8(*x, *y); }
    KFR_SINTRIN u16avx satsub(u16avx x, u16avx y) { return _mm256_subs_epu16(*x, *y); }
    KFR_SINTRIN i16avx satsub(i16avx x, i16avx y) { return _mm256_subs_epi16(*x, *y); }

    KFR_HANDLE_ALL(satadd)
    KFR_HANDLE_ALL(satsub)
    KFR_SPEC_FN(in_saturated, satadd)
    KFR_SPEC_FN(in_saturated, satsub)
};
}
namespace native
{
using fn_satadd = internal::in_saturated<>::fn_satadd;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>> satadd(const T1& x, const T2& y)
{
    return internal::in_saturated<>::satadd(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_satadd, E1, E2> satadd(E1&& x, E2&& y)
{
    return { fn_satadd(), std::forward<E1>(x), std::forward<E2>(y) };
}
using fn_satsub = internal::in_saturated<>::fn_satsub;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>> satsub(const T1& x, const T2& y)
{
    return internal::in_saturated<>::satsub(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_satsub, E1, E2> satsub(E1&& x, E2&& y)
{
    return { fn_satsub(), std::forward<E1>(x), std::forward<E2>(y) };
}
}
}

#pragma clang diagnostic pop
