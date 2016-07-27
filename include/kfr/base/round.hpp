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

namespace internal
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

#if defined CID_ARCH_SSE41

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

#if defined CID_ARCH_AVX

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
#endif

KFR_HANDLE_ALL_SIZES_F_1(floor)
KFR_HANDLE_ALL_SIZES_F_1(ceil)
KFR_HANDLE_ALL_SIZES_F_1(round)
KFR_HANDLE_ALL_SIZES_F_1(trunc)
KFR_HANDLE_ALL_SIZES_F_1(fract)

#else

// fallback

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
#endif

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
    return T(0);
}

template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> ifloor(vec<T, N> value)
{
    return cast<IT>(floor(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> iceil(vec<T, N> value)
{
    return cast<IT>(ceil(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> itrunc(vec<T, N> value)
{
    return cast<IT>(trunc(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> iround(vec<T, N> value)
{
    return cast<IT>(round(value));
}

KFR_HANDLE_SCALAR_1(floor)
KFR_HANDLE_SCALAR_1(ceil)
KFR_HANDLE_SCALAR_1(round)
KFR_HANDLE_SCALAR_1(trunc)
KFR_HANDLE_SCALAR_1(fract)
KFR_HANDLE_SCALAR_1(ifloor)
KFR_HANDLE_SCALAR_1(iceil)
KFR_HANDLE_SCALAR_1(iround)
KFR_HANDLE_SCALAR_1(itrunc)
KFR_I_FN(floor)
KFR_I_FN(ceil)
KFR_I_FN(round)
KFR_I_FN(trunc)
KFR_I_FN(fract)
KFR_I_FN(ifloor)
KFR_I_FN(iceil)
KFR_I_FN(iround)
KFR_I_FN(itrunc)
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 floor(const T1& x)
{
    return internal::floor(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_floor, E1> floor(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 ceil(const T1& x)
{
    return internal::ceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_ceil, E1> ceil(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 round(const T1& x)
{
    return internal::round(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_round, E1> round(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 trunc(const T1& x)
{
    return internal::trunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_trunc, E1> trunc(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 fract(const T1& x)
{
    return internal::fract(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_fract, E1> fract(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> ifloor(const T1& x)
{
    return internal::ifloor(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_ifloor, E1> ifloor(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> iceil(const T1& x)
{
    return internal::iceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_iceil, E1> iceil(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> iround(const T1& x)
{
    return internal::iround(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_iround, E1> iround(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> itrunc(const T1& x)
{
    return internal::itrunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_itrunc, E1> itrunc(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}

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
