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
#include "operators.hpp"

namespace kfr
{

#define KFR_mm_trunc_ps(V) _mm_round_ps((V), _MM_FROUND_TRUNC)
#define KFR_mm_roundnearest_ps(V) _mm_round_ps((V), _MM_FROUND_NINT)
#define KFR_mm_trunc_pd(V) _mm_round_pd((V), _MM_FROUND_TRUNC)
#define KFR_mm_roundnearest_pd(V) _mm_round_pd((V), _MM_FROUND_NINT)

#define KFR_mm_trunc_ss(V) _mm_round_ss(_mm_setzero_ps(), (V), _MM_FROUND_TRUNC)
#define KFR_mm_roundnearest_ss(V) _mm_round_ss(_mm_setzero_ps(), (V), _MM_FROUND_NINT)
#define KFR_mm_trunc_sd(V) _mm_round_sd(_mm_setzero_pd(), (V), _MM_FROUND_TRUNC)
#define KFR_mm_roundnearest_sd(V) _mm_round_sd(_mm_setzero_pd(), (V), _MM_FROUND_NINT)

#define KFR_mm_floor_ss(V) _mm_floor_ss(_mm_setzero_ps(), (V))
#define KFR_mm_floor_sd(V) _mm_floor_sd(_mm_setzero_pd(), (V))
#define KFR_mm_ceil_ss(V) _mm_ceil_ss(_mm_setzero_ps(), (V))
#define KFR_mm_ceil_sd(V) _mm_ceil_sd(_mm_setzero_pd(), (V))

#define KFR_mm256_trunc_ps(V) _mm256_round_ps((V), _MM_FROUND_TRUNC)
#define KFR_mm256_roundnearest_ps(V) _mm256_round_ps((V), _MM_FROUND_NINT)
#define KFR_mm256_trunc_pd(V) _mm256_round_pd((V), _MM_FROUND_TRUNC)
#define KFR_mm256_roundnearest_pd(V) _mm256_round_pd((V), _MM_FROUND_NINT)

namespace internal
{

template <cpu_t c = cpu_t::native>
struct in_round : in_round<older(c)>
{
    struct fn_floor : in_round<older(c)>::fn_floor, fn_disabled
    {
    };
    struct fn_ceil : in_round<older(c)>::fn_ceil, fn_disabled
    {
    };
    struct fn_round : in_round<older(c)>::fn_round, fn_disabled
    {
    };
    struct fn_trunc : in_round<older(c)>::fn_trunc, fn_disabled
    {
    };
    struct fn_fract : in_round<older(c)>::fn_fract, fn_disabled
    {
    };
};

template <>
struct in_round<cpu_t::common>
{
    constexpr static cpu_t cpu = cpu_t::common;

    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> floor(vec<T, N> value)
    {
        return value;
    }
    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> ceil(vec<T, N> value)
    {
        return value;
    }
    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> trunc(vec<T, N> value)
    {
        return value;
    }
    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> round(vec<T, N> value)
    {
        return value;
    }
    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> fract(vec<T, N>)
    {
        return T();
    }

    template <size_t N>
    KFR_SINTRIN vec<f32, N> floor(vec<f32, N> x)
    {
        vec<f32, N> t = cast<f32>(cast<i32>(x));
        return t - (bitcast<f32>(x < t) & 1.f);
    }
    template <size_t N>
    KFR_SINTRIN vec<f64, N> floor(vec<f64, N> x)
    {
        vec<f64, N> t = cast<f64>(cast<i64>(x));
        return t - (bitcast<f64>(x < t) & 1.0);
    }
    template <size_t N>
    KFR_SINTRIN vec<f32, N> ceil(vec<f32, N> x)
    {
        vec<f32, N> t = cast<f32>(cast<i32>(x));
        return t + (bitcast<f32>(x > t) & 1.f);
    }
    template <size_t N>
    KFR_SINTRIN vec<f64, N> ceil(vec<f64, N> x)
    {
        vec<f64, N> t = cast<f64>(cast<i64>(x));
        return t + (bitcast<f64>(x > t) & 1.0);
    }
    template <size_t N>
    KFR_SINTRIN vec<f32, N> round(vec<f32, N> x)
    {
        return cast<f32>(cast<i32>(x + mulsign(broadcast<N>(0.5f), x)));
    }
    template <size_t N>
    KFR_SINTRIN vec<f64, N> round(vec<f64, N> x)
    {
        return cast<f64>(cast<i64>(x + mulsign(broadcast<N>(0.5), x)));
    }
    template <size_t N>
    KFR_SINTRIN vec<f32, N> trunc(vec<f32, N> x)
    {
        return cast<f32>(cast<i32>(x));
    }
    template <size_t N>
    KFR_SINTRIN vec<f64, N> trunc(vec<f64, N> x)
    {
        return cast<f64>(cast<i64>(x));
    }
    template <size_t N>
    KFR_SINTRIN vec<f32, N> fract(vec<f32, N> x)
    {
        return x - floor(x);
    }
    template <size_t N>
    KFR_SINTRIN vec<f64, N> fract(vec<f64, N> x)
    {
        return x - floor(x);
    }

    KFR_HANDLE_SCALAR(floor)
    KFR_HANDLE_SCALAR(ceil)
    KFR_HANDLE_SCALAR(round)
    KFR_HANDLE_SCALAR(trunc)
    KFR_HANDLE_SCALAR(fract)
    KFR_SPEC_FN(in_round, floor)
    KFR_SPEC_FN(in_round, ceil)
    KFR_SPEC_FN(in_round, round)
    KFR_SPEC_FN(in_round, trunc)
    KFR_SPEC_FN(in_round, fract)
};

template <>
struct in_round<cpu_t::sse41> : in_round<cpu_t::common>
{
    constexpr static cpu_t cpu = cpu_t::sse41;

    KFR_SINTRIN f32sse floor(f32sse value) { return _mm_floor_ps(*value); }
    KFR_SINTRIN f32sse ceil(f32sse value) { return _mm_ceil_ps(*value); }
    KFR_SINTRIN f32sse trunc(f32sse value) { return KFR_mm_trunc_ps(*value); }
    KFR_SINTRIN f32sse round(f32sse value) { return KFR_mm_roundnearest_ps(*value); }
    KFR_SINTRIN f64sse floor(f64sse value) { return _mm_floor_pd(*value); }
    KFR_SINTRIN f64sse ceil(f64sse value) { return _mm_ceil_pd(*value); }
    KFR_SINTRIN f64sse trunc(f64sse value) { return KFR_mm_trunc_pd(*value); }
    KFR_SINTRIN f64sse round(f64sse value) { return KFR_mm_roundnearest_pd(*value); }
    KFR_SINTRIN f32sse fract(f32sse x) { return x - floor(x); }
    KFR_SINTRIN f64sse fract(f64sse x) { return x - floor(x); }

    KFR_HANDLE_ALL(floor)
    KFR_HANDLE_ALL(ceil)
    KFR_HANDLE_ALL(round)
    KFR_HANDLE_ALL(trunc)
    KFR_HANDLE_ALL(fract)
    KFR_HANDLE_SCALAR(floor)
    KFR_HANDLE_SCALAR(ceil)
    KFR_HANDLE_SCALAR(round)
    KFR_HANDLE_SCALAR(trunc)
    KFR_HANDLE_SCALAR(fract)
    KFR_SPEC_FN(in_round, floor)
    KFR_SPEC_FN(in_round, ceil)
    KFR_SPEC_FN(in_round, round)
    KFR_SPEC_FN(in_round, trunc)
    KFR_SPEC_FN(in_round, fract)
};

template <>
struct in_round<cpu_t::avx1> : in_round<cpu_t::sse41>
{
    constexpr static cpu_t cpu = cpu_t::avx1;
    using in_round<cpu_t::sse41>::floor;
    using in_round<cpu_t::sse41>::ceil;
    using in_round<cpu_t::sse41>::trunc;
    using in_round<cpu_t::sse41>::round;
    using in_round<cpu_t::sse41>::fract;

    KFR_SINTRIN f32avx floor(f32avx value) { return _mm256_floor_ps(*value); }
    KFR_SINTRIN f32avx ceil(f32avx value) { return _mm256_ceil_ps(*value); }
    KFR_SINTRIN f32avx trunc(f32avx value) { return KFR_mm256_trunc_ps(*value); }
    KFR_SINTRIN f32avx round(f32avx value) { return KFR_mm256_roundnearest_ps(*value); }
    KFR_SINTRIN f64avx floor(f64avx value) { return _mm256_floor_pd(*value); }
    KFR_SINTRIN f64avx ceil(f64avx value) { return _mm256_ceil_pd(*value); }
    KFR_SINTRIN f64avx trunc(f64avx value) { return KFR_mm256_trunc_pd(*value); }
    KFR_SINTRIN f64avx round(f64avx value) { return KFR_mm256_roundnearest_pd(*value); }
    KFR_SINTRIN f32avx fract(f32avx x) { return x - floor(x); }
    KFR_SINTRIN f64avx fract(f64avx x) { return x - floor(x); }

    KFR_HANDLE_ALL(floor)
    KFR_HANDLE_ALL(ceil)
    KFR_HANDLE_ALL(round)
    KFR_HANDLE_ALL(trunc)
    KFR_HANDLE_ALL(fract)
    KFR_HANDLE_SCALAR(floor)
    KFR_HANDLE_SCALAR(ceil)
    KFR_HANDLE_SCALAR(round)
    KFR_HANDLE_SCALAR(trunc)
    KFR_HANDLE_SCALAR(fract)
    KFR_SPEC_FN(in_round, floor)
    KFR_SPEC_FN(in_round, ceil)
    KFR_SPEC_FN(in_round, round)
    KFR_SPEC_FN(in_round, trunc)
    KFR_SPEC_FN(in_round, fract)
};

#undef KFR_mm_trunc_ps
#undef KFR_mm_roundnearest_ps
#undef KFR_mm_trunc_pd
#undef KFR_mm_roundnearest_pd
#undef KFR_mm_trunc_ss
#undef KFR_mm_roundnearest_ss
#undef KFR_mm_trunc_sd
#undef KFR_mm_roundnearest_sd
#undef KFR_mm_floor_ss
#undef KFR_mm_floor_sd
#undef KFR_mm_ceil_ss
#undef KFR_mm_ceil_sd
#undef KFR_mm256_trunc_ps
#undef KFR_mm256_roundnearest_ps
#undef KFR_mm256_trunc_pd
#undef KFR_mm256_roundnearest_pd
}

namespace native
{
using fn_floor = internal::in_round<>::fn_floor;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> floor(const T1& x)
{
    return internal::in_round<>::floor(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_floor, E1> floor(E1&& x)
{
    return { fn_floor(), std::forward<E1>(x) };
}

using fn_ceil = internal::in_round<>::fn_ceil;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> ceil(const T1& x)
{
    return internal::in_round<>::ceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_ceil, E1> ceil(E1&& x)
{
    return { fn_ceil(), std::forward<E1>(x) };
}

using fn_round = internal::in_round<>::fn_round;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> round(const T1& x)
{
    return internal::in_round<>::round(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_round, E1> round(E1&& x)
{
    return { fn_round(), std::forward<E1>(x) };
}

using fn_trunc = internal::in_round<>::fn_trunc;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> trunc(const T1& x)
{
    return internal::in_round<>::trunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_trunc, E1> trunc(E1&& x)
{
    return { fn_trunc(), std::forward<E1>(x) };
}

using fn_fract = internal::in_round<>::fn_fract;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> fract(const T1& x)
{
    return internal::in_round<>::fract(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_fract, E1> fract(E1&& x)
{
    return { fn_fract(), std::forward<E1>(x) };
}
}
}
