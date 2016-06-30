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
#include "select.hpp"

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

namespace internal
{

template <cpu_t cpu = cpu_t::native>
struct in_min_max : in_min_max<older(cpu)>
{
    struct fn_min : in_min_max<older(cpu)>::fn_min, fn_disabled
    {
    };
    struct fn_max : in_min_max<older(cpu)>::fn_max, fn_disabled
    {
    };
};

template <>
struct in_min_max<cpu_t::sse2> : in_select<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse2;

private:
    using in_select<cpu>::select;

public:
    template <typename T>
    KFR_SINTRIN T min(initialvalue<T>)
    {
        return std::numeric_limits<T>::max();
    }
    template <typename T>
    KFR_SINTRIN T max(initialvalue<T>)
    {
        return std::numeric_limits<T>::min();
    }

    KFR_CPU_INTRIN(sse2) f32sse min(f32sse x, f32sse y) { return _mm_min_ps(*x, *y); }
    KFR_CPU_INTRIN(sse2) f64sse min(f64sse x, f64sse y) { return _mm_min_pd(*x, *y); }
    KFR_CPU_INTRIN(sse2) i8sse min(i8sse x, i8sse y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(sse2) u16sse min(u16sse x, u16sse y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(sse2) i32sse min(i32sse x, i32sse y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(sse2) u32sse min(u32sse x, u32sse y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(sse2) u8sse min(u8sse x, u8sse y) { return _mm_min_epu8(*x, *y); }
    KFR_CPU_INTRIN(sse2) i16sse min(i16sse x, i16sse y) { return _mm_min_epi16(*x, *y); }
    KFR_CPU_INTRIN(sse2) i64sse min(i64sse x, i64sse y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(sse2) u64sse min(u64sse x, u64sse y) { return select(x < y, x, y); }

    KFR_CPU_INTRIN(sse2) f32sse max(f32sse x, f32sse y) { return _mm_max_ps(*x, *y); }
    KFR_CPU_INTRIN(sse2) f64sse max(f64sse x, f64sse y) { return _mm_max_pd(*x, *y); }
    KFR_CPU_INTRIN(sse2) i8sse max(i8sse x, i8sse y) { return select(x > y, x, y); }
    KFR_CPU_INTRIN(sse2) u16sse max(u16sse x, u16sse y) { return select(x > y, x, y); }
    KFR_CPU_INTRIN(sse2) i32sse max(i32sse x, i32sse y) { return select(x > y, x, y); }
    KFR_CPU_INTRIN(sse2) u32sse max(u32sse x, u32sse y) { return select(x > y, x, y); }
    KFR_CPU_INTRIN(sse2) u8sse max(u8sse x, u8sse y) { return _mm_max_epu8(*x, *y); }
    KFR_CPU_INTRIN(sse2) i16sse max(i16sse x, i16sse y) { return _mm_max_epi16(*x, *y); }
    KFR_CPU_INTRIN(sse2) i64sse max(i64sse x, i64sse y) { return select(x > y, x, y); }
    KFR_CPU_INTRIN(sse2) u64sse max(u64sse x, u64sse y) { return select(x > y, x, y); }

    KFR_HANDLE_ALL(min)
    KFR_HANDLE_ALL(max)

    KFR_SPEC_FN(in_min_max, min)
    KFR_SPEC_FN(in_min_max, max)
};

template <>
struct in_min_max<cpu_t::sse41> : in_min_max<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse41;
    using in_min_max<cpu_t::sse2>::min;
    using in_min_max<cpu_t::sse2>::max;

    KFR_CPU_INTRIN(sse41) i8sse min(i8sse x, i8sse y) { return _mm_min_epi8(*x, *y); }
    KFR_CPU_INTRIN(sse41) u16sse min(u16sse x, u16sse y) { return _mm_min_epu16(*x, *y); }
    KFR_CPU_INTRIN(sse41) i32sse min(i32sse x, i32sse y) { return _mm_min_epi32(*x, *y); }
    KFR_CPU_INTRIN(sse41) u32sse min(u32sse x, u32sse y) { return _mm_min_epu32(*x, *y); }

    KFR_CPU_INTRIN(sse41) i8sse max(i8sse x, i8sse y) { return _mm_max_epi8(*x, *y); }
    KFR_CPU_INTRIN(sse41) u16sse max(u16sse x, u16sse y) { return _mm_max_epu16(*x, *y); }
    KFR_CPU_INTRIN(sse41) i32sse max(i32sse x, i32sse y) { return _mm_max_epi32(*x, *y); }
    KFR_CPU_INTRIN(sse41) u32sse max(u32sse x, u32sse y) { return _mm_max_epu32(*x, *y); }

    KFR_HANDLE_ALL(min)
    KFR_HANDLE_ALL(max)
    KFR_SPEC_FN(in_min_max, min)
    KFR_SPEC_FN(in_min_max, max)
};

template <>
struct in_min_max<cpu_t::avx1> : in_min_max<cpu_t::sse41>
{
    constexpr static cpu_t cpu = cpu_t::avx1;
    using in_min_max<cpu_t::sse41>::min;
    using in_min_max<cpu_t::sse41>::max;

    KFR_CPU_INTRIN(avx) f32avx min(f32avx x, f32avx y) { return _mm256_min_ps(*x, *y); }
    KFR_CPU_INTRIN(avx) f64avx min(f64avx x, f64avx y) { return _mm256_min_pd(*x, *y); }
    KFR_CPU_INTRIN(avx) f32avx max(f32avx x, f32avx y) { return _mm256_max_ps(*x, *y); }
    KFR_CPU_INTRIN(avx) f64avx max(f64avx x, f64avx y) { return _mm256_max_pd(*x, *y); }

    KFR_HANDLE_ALL(min)
    KFR_HANDLE_ALL(max)
    KFR_SPEC_FN(in_min_max, min)
    KFR_SPEC_FN(in_min_max, max)
};

template <>
struct in_min_max<cpu_t::avx2> : in_min_max<cpu_t::avx1>, in_select<cpu_t::avx2>
{
    constexpr static cpu_t cpu = cpu_t::avx2;

private:
    using in_select<cpu>::select;

public:
    using in_min_max<cpu_t::avx1>::min;
    using in_min_max<cpu_t::avx1>::max;

    KFR_CPU_INTRIN(avx2) u8avx min(u8avx x, u8avx y) { return _mm256_min_epu8(*x, *y); }
    KFR_CPU_INTRIN(avx2) i16avx min(i16avx x, i16avx y) { return _mm256_min_epi16(*x, *y); }
    KFR_CPU_INTRIN(avx2) i8avx min(i8avx x, i8avx y) { return _mm256_min_epi8(*x, *y); }
    KFR_CPU_INTRIN(avx2) u16avx min(u16avx x, u16avx y) { return _mm256_min_epu16(*x, *y); }
    KFR_CPU_INTRIN(avx2) i32avx min(i32avx x, i32avx y) { return _mm256_min_epi32(*x, *y); }
    KFR_CPU_INTRIN(avx2) u32avx min(u32avx x, u32avx y) { return _mm256_min_epu32(*x, *y); }

    KFR_CPU_INTRIN(avx2) u8avx max(u8avx x, u8avx y) { return _mm256_max_epu8(*x, *y); }
    KFR_CPU_INTRIN(avx2) i16avx max(i16avx x, i16avx y) { return _mm256_max_epi16(*x, *y); }
    KFR_CPU_INTRIN(avx2) i8avx max(i8avx x, i8avx y) { return _mm256_max_epi8(*x, *y); }
    KFR_CPU_INTRIN(avx2) u16avx max(u16avx x, u16avx y) { return _mm256_max_epu16(*x, *y); }
    KFR_CPU_INTRIN(avx2) i32avx max(i32avx x, i32avx y) { return _mm256_max_epi32(*x, *y); }
    KFR_CPU_INTRIN(avx2) u32avx max(u32avx x, u32avx y) { return _mm256_max_epu32(*x, *y); }

    KFR_CPU_INTRIN(avx2) i64avx min(i64avx x, i64avx y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(avx2) u64avx min(u64avx x, u64avx y) { return select(x < y, x, y); }
    KFR_CPU_INTRIN(avx2) i64avx max(i64avx x, i64avx y) { return select(x > y, x, y); }
    KFR_CPU_INTRIN(avx2) u64avx max(u64avx x, u64avx y) { return select(x > y, x, y); }

    KFR_HANDLE_ALL(min)
    KFR_HANDLE_ALL(max)
    KFR_SPEC_FN(in_min_max, min)
    KFR_SPEC_FN(in_min_max, max)
};

template <cpu_t cpu = cpu_t::native>
struct in_minabs_maxabs
{
public:
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> minabs(vec<T, N> x, vec<T, N> y)
    {
        return in_min_max<cpu>::min(in_abs<cpu>::abs(x), in_abs<cpu>::abs(y));
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> maxabs(vec<T, N> x, vec<T, N> y)
    {
        return in_min_max<cpu>::max(in_abs<cpu>::abs(x), in_abs<cpu>::abs(y));
    }

    KFR_HANDLE_ALL(minabs)
    KFR_HANDLE_ALL(maxabs)
    KFR_SPEC_FN(in_minabs_maxabs, minabs)
    KFR_SPEC_FN(in_minabs_maxabs, maxabs)
};

template <cpu_t cpu = cpu_t::native>
struct in_clamp : in_min_max<cpu>
{
    using in_min_max<cpu>::min;
    using in_min_max<cpu>::max;

    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clamp(vec<T, N> x, T minimum, T maximum)
    {
        return clamp(x, broadcast<N>(minimum), broadcast<N>(maximum));
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clamp(vec<T, N> x, T minimum, vec<T, N> maximum)
    {
        return clamp(x, broadcast<N>(minimum), maximum);
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clamp(vec<T, N> x, vec<T, N> minimum, T maximum)
    {
        return clamp(x, minimum, broadcast<N>(maximum));
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clamp(vec<T, N> x, T maximum)
    {
        return clamp(x, broadcast<N>(maximum));
    }

    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clamp(vec<T, N> x, vec<T, N> minimum, vec<T, N> maximum)
    {
        return max(minimum, min(x, maximum));
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clamp(vec<T, N> x, vec<T, N> maximum)
    {
        return max(zerovector<T, N>(), min(x, maximum));
    }

    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clampm1(vec<T, N> x, vec<T, N> minimum, vec<T, N> maximum)
    {
        return max(minimum, min(x, maximum - T(1)));
    }
    template <typename T, size_t N>
    KFR_SINTRIN vec<T, N> clampm1(vec<T, N> x, vec<T, N> maximum)
    {
        return max(zerovector<T, N>(), min(x, maximum - T(1)));
    }
    KFR_HANDLE_ALL(clamp)
    KFR_HANDLE_ALL(clampm1)
    KFR_SPEC_FN(in_clamp, clamp)
    KFR_SPEC_FN(in_clamp, clampm1)
};
}

namespace native
{
using fn_min = internal::in_min_max<>::fn_min;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>>

min(const T1& x, const T2& y)
{
    return internal::in_min_max<>::min(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_min, E1, E2> min(E1&& x, E2&& y)
{
    return { fn_min(), std::forward<E1>(x), std::forward<E2>(y) };
}
using fn_max = internal::in_min_max<>::fn_max;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>>

max(const T1& x, const T2& y)
{
    return internal::in_min_max<>::max(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_max, E1, E2> max(E1&& x, E2&& y)
{
    return { fn_max(), std::forward<E1>(x), std::forward<E2>(y)

    };
}
using fn_minabs = internal::in_minabs_maxabs<>::fn_minabs;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>>

minabs(const T1& x, const T2& y)
{
    return internal::in_minabs_maxabs<>::minabs(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_minabs, E1, E2> minabs(E1&& x, E2&& y)
{
    return { fn_minabs(), std::forward<E1>(x), std::forward<E2>(y)

    };
}
using fn_maxabs = internal::in_minabs_maxabs<>::fn_maxabs;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>>

maxabs(const T1& x, const T2& y)
{
    return internal::in_minabs_maxabs<>::maxabs(x, y);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_maxabs, E1, E2> maxabs(E1&& x, E2&& y)
{
    return { fn_maxabs(), std::forward<E1>(x), std::forward<E2>(y)

    };
}
using fn_clamp = internal::in_clamp<>::fn_clamp;
template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value)>
KFR_INLINE ftype<common_type<T1, T2, T3>>

clamp(const T1& x, const T2& l, const T3& h)
{
    return internal::in_clamp<>::clamp(x, l, h);
}

template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INLINE expr_func<fn_clamp, E1, E2, E3> clamp(E1&& x, E2&& l, E3&& h)
{
    return { fn_clamp(), std::forward<E1>(x), std::forward<E2>(l), std::forward<E3>(h)

    };
}
using fn_clampm1 = internal::in_clamp<>::fn_clampm1;
template <typename T1, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value)>
KFR_INLINE ftype<common_type<T1, T2, T3>>

clampm1(const T1& x, const T2& l, const T3& h)
{
    return internal::in_clamp<>::clampm1(x, l, h);
}

template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INLINE expr_func<fn_clampm1, E1, E2, E3> clampm1(E1&& x, E2&& l, E3&& h)
{
    return { fn_clampm1(), std::forward<E1>(x), std::forward<E2>(l), std::forward<E3>(h)

    };
}

using fn_clamp = internal::in_clamp<>::fn_clamp;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>>

clamp(const T1& x, const T2& h)
{
    return internal::in_clamp<>::clamp(x, h);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_clamp, E1, E2> clamp(E1&& x, E2&& h)
{
    return { fn_clamp(), std::forward<E1>(x), std::forward<E2>(h)

    };
}
using fn_clampm1 = internal::in_clamp<>::fn_clampm1;
template <typename T1, typename T2, KFR_ENABLE_IF(is_numeric_args<T1, T2>::value)>
KFR_INLINE ftype<common_type<T1, T2>>

clampm1(const T1& x, const T2& h)
{
    return internal::in_clamp<>::clampm1(x, h);
}

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_clampm1, E1, E2> clampm1(E1&& x, E2&& h)
{
    return { fn_clampm1(), std::forward<E1>(x), std::forward<E2>(h)

    };
}
}
}

#pragma clang diagnostic pop
