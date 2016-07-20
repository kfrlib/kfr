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

namespace internal
{

template <cpu_t c = cpu_t::native>
struct in_bittest : in_bittest<older(c)>
{
    struct fn_bittestnone : fn_disabled
    {
    };
    struct fn_bittestall : fn_disabled
    {
    };
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

template <>
struct in_bittest<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse2;

    KFR_SINTRIN bitmask<4> getmask(f32sse x) { return bitmask<4>(_mm_movemask_pd(*x)); }
    KFR_SINTRIN bitmask<4> getmask(f64sse x) { return bitmask<4>(_mm_movemask_pd(*x)); }
    KFR_SINTRIN bitmask<16> getmask(u8sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(u16sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(u32sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(u64sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(i8sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(i16sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(i32sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<16> getmask(i64sse x) { return bitmask<16>(_mm_movemask_epi8(*x)); }

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

    KFR_HANDLE_ALL_REDUCE(logical_and, bittestnone)
    KFR_HANDLE_ALL_REDUCE(logical_and, bittestall)
    KFR_SPEC_FN(in_bittest, bittestnone)
    KFR_SPEC_FN(in_bittest, bittestall)
};

template <>
struct in_bittest<cpu_t::sse41> : in_bittest<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse41;

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

    KFR_HANDLE_ALL_REDUCE(logical_and, bittestnone)
    KFR_HANDLE_ALL_REDUCE(logical_and, bittestall)
    KFR_SPEC_FN(in_bittest, bittestnone)
    KFR_SPEC_FN(in_bittest, bittestall)
};

template <>
struct in_bittest<cpu_t::avx1> : in_bittest<cpu_t::sse41>
{
    constexpr static cpu_t cpu = cpu_t::avx1;
    using in_bittest<cpu_t::sse41>::bittestnone;
    using in_bittest<cpu_t::sse41>::bittestall;

    KFR_SINTRIN bitmask<8> getmask(f32avx x) { return bitmask<8>(_mm256_movemask_pd(*x)); }
    KFR_SINTRIN bitmask<8> getmask(f64avx x) { return bitmask<8>(_mm256_movemask_pd(*x)); }

    KFR_SINTRIN bool bittestnone(f32avx x, f32avx y) { return _mm256_testz_ps(*x, *y); }
    KFR_SINTRIN bool bittestnone(f64avx x, f64avx y) { return _mm256_testz_pd(*x, *y); }
    KFR_SINTRIN bool bittestnone(f32avx x) { return _mm256_testz_ps(*x, *x); }
    KFR_SINTRIN bool bittestnone(f64avx x) { return _mm256_testz_pd(*x, *x); }
    KFR_SINTRIN bool bittestnall(f32avx x, f32avx y) { return _mm256_testc_ps(*x, *y); }
    KFR_SINTRIN bool bittestnall(f64avx x, f64avx y) { return _mm256_testc_pd(*x, *y); }
    KFR_SINTRIN bool bittestnall(f32avx x) { return _mm256_testc_ps(*x, *allonesvector(x)); }
    KFR_SINTRIN bool bittestnall(f64avx x) { return _mm256_testc_pd(*x, *allonesvector(x)); }

    KFR_HANDLE_ALL_REDUCE(logical_and, bittestnone)
    KFR_HANDLE_ALL_REDUCE(logical_and, bittestall)
    KFR_SPEC_FN(in_bittest, bittestnone)
    KFR_SPEC_FN(in_bittest, bittestall)
};

template <>
struct in_bittest<cpu_t::avx2> : in_bittest<cpu_t::avx1>
{
    constexpr static cpu_t cpu = cpu_t::avx2;
    using in_bittest<cpu_t::avx1>::bittestnone;
    using in_bittest<cpu_t::avx1>::bittestall;

    KFR_SINTRIN bitmask<32> getmask(u8avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(u16avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(u32avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(u64avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(i8avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(i16avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(i32avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }
    KFR_SINTRIN bitmask<32> getmask(i64avx x) { return bitmask<32>(_mm256_movemask_epi8(*x)); }

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

    KFR_HANDLE_ALL_REDUCE(logical_and, bittestnone)
    KFR_HANDLE_ALL_REDUCE(logical_and, bittestall)
    KFR_SPEC_FN(in_bittest, bittestnone)
    KFR_SPEC_FN(in_bittest, bittestall)
};
}

namespace native
{
using fn_bittestnone = internal::in_bittest<>::fn_bittestnone;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> bittestnone(const T1& x)
{
    return internal::in_bittest<>::bittestnone(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_bittestnone, E1> bittestnone(E1&& x)
{
    return { fn_bittestnone(), std::forward<E1>(x) };
}

using fn_bittestall = internal::in_bittest<>::fn_bittestall;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> bittestall(const T1& x)
{
    return internal::in_bittest<>::bittestall(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_bittestall, E1> bittestall(E1&& x)
{
    return { fn_bittestall(), std::forward<E1>(x) };
}

using fn_bittestnone = internal::in_bittest<>::fn_bittestnone;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>> bittestnone(const T1& x, const T2& y)
{
    return internal::in_bittest<>::bittestnone(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_bittestnone, E1, E2> bittestnone(E1&& x, E2&& y)
{
    return { fn_bittestnone(), std::forward<E1>(x), std::forward<E2>(y) };
}
using fn_bittestall = internal::in_bittest<>::fn_bittestall;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>> bittestall(const T1& x, const T2& y)
{
    return internal::in_bittest<>::bittestall(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_bittestall, E1, E2> bittestall(E1&& x, E2&& y)
{
    return { fn_bittestall(), std::forward<E1>(x), std::forward<E2>(y) };
}
}
}
