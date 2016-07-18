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

namespace kfr
{
namespace internal
{

template <cpu_t c>
struct in_select_impl : in_select_impl<older(c)>
{
    struct fn_select : fn_disabled
    {
    };
};

template <>
struct in_select_impl<cpu_t::sse2>
{
    constexpr static cpu_t cur = cpu_t::sse2;

    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> select(vec<T, N> m, vec<T, N> x, vec<T, N> y)
    {
        return y ^ ((x ^ y) & m);
    }
    KFR_SPEC_FN(in_select_impl, select)
};

template <>
struct in_select_impl<cpu_t::sse41> : in_select_impl<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse41;

    KFR_CPU_INTRIN(sse41) u8sse select(u8sse m, u8sse x, u8sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) u16sse select(u16sse m, u16sse x, u16sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) u32sse select(u32sse m, u32sse x, u32sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) u64sse select(u64sse m, u64sse x, u64sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) i8sse select(i8sse m, i8sse x, i8sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) i16sse select(i16sse m, i16sse x, i16sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) i32sse select(i32sse m, i32sse x, i32sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) i64sse select(i64sse m, i64sse x, i64sse y) { return _mm_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) f32sse select(f32sse m, f32sse x, f32sse y) { return _mm_blendv_ps(*y, *x, *m); }
    KFR_CPU_INTRIN(sse41) f64sse select(f64sse m, f64sse x, f64sse y) { return _mm_blendv_pd(*y, *x, *m); }

    KFR_HANDLE_ALL(select)
    KFR_SPEC_FN(in_select_impl, select)
};

template <>
struct in_select_impl<cpu_t::avx1> : in_select_impl<cpu_t::sse41>
{
    constexpr static cpu_t cpu = cpu_t::avx1;
    using in_select_impl<cpu_t::sse41>::select;

    KFR_CPU_INTRIN(avx) f64avx select(f64avx m, f64avx x, f64avx y) { return _mm256_blendv_pd(*y, *x, *m); }
    KFR_CPU_INTRIN(avx) f32avx select(f32avx m, f32avx x, f32avx y) { return _mm256_blendv_ps(*y, *x, *m); }

    KFR_HANDLE_ALL(select)
    KFR_SPEC_FN(in_select_impl, select)
};

template <>
struct in_select_impl<cpu_t::avx2> : in_select_impl<cpu_t::avx1>
{
    constexpr static cpu_t cpu = cpu_t::avx2;
    using in_select_impl<cpu_t::avx1>::select;

    KFR_CPU_INTRIN(avx2) u8avx select(u8avx m, u8avx x, u8avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(avx2) u16avx select(u16avx m, u16avx x, u16avx y)
    {
        return _mm256_blendv_epi8(*y, *x, *m);
    }
    KFR_CPU_INTRIN(avx2) u32avx select(u32avx m, u32avx x, u32avx y)
    {
        return _mm256_blendv_epi8(*y, *x, *m);
    }
    KFR_CPU_INTRIN(avx2) u64avx select(u64avx m, u64avx x, u64avx y)
    {
        return _mm256_blendv_epi8(*y, *x, *m);
    }
    KFR_CPU_INTRIN(avx2) i8avx select(i8avx m, i8avx x, i8avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
    KFR_CPU_INTRIN(avx2) i16avx select(i16avx m, i16avx x, i16avx y)
    {
        return _mm256_blendv_epi8(*y, *x, *m);
    }
    KFR_CPU_INTRIN(avx2) i32avx select(i32avx m, i32avx x, i32avx y)
    {
        return _mm256_blendv_epi8(*y, *x, *m);
    }
    KFR_CPU_INTRIN(avx2) i64avx select(i64avx m, i64avx x, i64avx y)
    {
        return _mm256_blendv_epi8(*y, *x, *m);
    }

    KFR_HANDLE_ALL(select)
    KFR_SPEC_FN(in_select_impl, select)
};

template <cpu_t c = cpu_t::native>
struct in_select : in_select_impl<c>
{
    using in_select_impl<c>::select;

    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, vec<T, N> x, vec<T, N> y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(bitcast<T>(m), x, y);
    }
    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, mask<T, N> x, mask<T, N> y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(bitcast<T>(m), ref_cast<vec<T, N>>(x), ref_cast<vec<T, N>>(y));
    }

    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, T x, T y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(bitcast<T>(m), broadcast<N>(x), broadcast<N>(y));
    }

    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, vec<T, N> x, T y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(bitcast<T>(m), x, broadcast<N>(y));
    }

    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, T x, vec<T, N> y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(bitcast<T>(m), broadcast<N>(x), y);
    }
    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, mask<T, N> x, T y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(bitcast<T>(m), ref_cast<vec<T, N>>(x), broadcast<N>(y));
    }

    template <typename T, size_t N, typename M>
    KFR_SINTRIN vec<T, N> select(mask<M, N> m, T x, mask<T, N> y)
    {
        static_assert(sizeof(M) == sizeof(T), "select: Incompatible types");
        return in_select_impl<c>::select(m, broadcast<N>(x), ref_cast<vec<T, N>>(y));
    }
    KFR_SPEC_FN(in_select, select)

    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> sign(vec<T, N> x)
    {
        return select(x > T(), T(1), select(x < T(), T(-1), T(0)));
    }
};
}

namespace native
{
using fn_select = internal::in_select<>::fn_select;
template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value)>
KFR_INLINE ftype<common_type<T2, T3>> select(const T1& arg1, const T2& arg2, const T3& arg3)
{
    return internal::in_select<>::select(arg1, arg2, arg3);
}
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INLINE expr_func<fn_select, E1, E2, E3> select(E1&& arg1, E2&& arg2, E3&& arg3)
{
    return { fn_select(), std::forward<E1>(arg1), std::forward<E2>(arg2), std::forward<E3>(arg3) };
}
}
}
