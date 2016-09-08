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

#include "function.hpp"
#include "operators.hpp"

namespace kfr
{

namespace intrinsics
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

#if defined CMT_ARCH_SSE41

KFR_SINTRIN f32sse floor(const f32sse& value) { return _mm_floor_ps(*value); }
KFR_SINTRIN f32sse ceil(const f32sse& value) { return _mm_ceil_ps(*value); }
KFR_SINTRIN f32sse trunc(const f32sse& value) { return KFR_mm_trunc_ps(*value); }
KFR_SINTRIN f32sse round(const f32sse& value) { return KFR_mm_roundnearest_ps(*value); }
KFR_SINTRIN f64sse floor(const f64sse& value) { return _mm_floor_pd(*value); }
KFR_SINTRIN f64sse ceil(const f64sse& value) { return _mm_ceil_pd(*value); }
KFR_SINTRIN f64sse trunc(const f64sse& value) { return KFR_mm_trunc_pd(*value); }
KFR_SINTRIN f64sse round(const f64sse& value) { return KFR_mm_roundnearest_pd(*value); }
KFR_SINTRIN f32sse fract(const f32sse& x) { return x - floor(x); }
KFR_SINTRIN f64sse fract(const f64sse& x) { return x - floor(x); }

#if defined CMT_ARCH_AVX

KFR_SINTRIN f32avx floor(const f32avx& value) { return _mm256_floor_ps(*value); }
KFR_SINTRIN f32avx ceil(const f32avx& value) { return _mm256_ceil_ps(*value); }
KFR_SINTRIN f32avx trunc(const f32avx& value) { return KFR_mm256_trunc_ps(*value); }
KFR_SINTRIN f32avx round(const f32avx& value) { return KFR_mm256_roundnearest_ps(*value); }
KFR_SINTRIN f64avx floor(const f64avx& value) { return _mm256_floor_pd(*value); }
KFR_SINTRIN f64avx ceil(const f64avx& value) { return _mm256_ceil_pd(*value); }
KFR_SINTRIN f64avx trunc(const f64avx& value) { return KFR_mm256_trunc_pd(*value); }
KFR_SINTRIN f64avx round(const f64avx& value) { return KFR_mm256_roundnearest_pd(*value); }
KFR_SINTRIN f32avx fract(const f32avx& x) { return x - floor(x); }
KFR_SINTRIN f64avx fract(const f64avx& x) { return x - floor(x); }
#endif

KFR_HANDLE_ALL_SIZES_F_1(floor)
KFR_HANDLE_ALL_SIZES_F_1(ceil)
KFR_HANDLE_ALL_SIZES_F_1(round)
KFR_HANDLE_ALL_SIZES_F_1(trunc)
KFR_HANDLE_ALL_SIZES_F_1(fract)

#else

// fallback

template <size_t N>
KFR_SINTRIN vec<f32, N> floor(const vec<f32, N>& x)
{
    vec<f32, N> t = cast<f32>(cast<i32>(x));
    return t - (tovec(x < t) & 1.f);
}
template <size_t N>
KFR_SINTRIN vec<f64, N> floor(const vec<f64, N>& x)
{
    vec<f64, N> t = cast<f64>(cast<i64>(x));
    return t - (tovec(x < t) & 1.0);
}
template <size_t N>
KFR_SINTRIN vec<f32, N> ceil(const vec<f32, N>& x)
{
    vec<f32, N> t = cast<f32>(cast<i32>(x));
    return t + (tovec(x > t) & 1.f);
}
template <size_t N>
KFR_SINTRIN vec<f64, N> ceil(const vec<f64, N>& x)
{
    vec<f64, N> t = cast<f64>(cast<i64>(x));
    return t + (tovec(x > t) & 1.0);
}
template <size_t N>
KFR_SINTRIN vec<f32, N> round(const vec<f32, N>& x)
{
    return cast<f32>(cast<i32>(x + mulsign(broadcast<N>(0.5f), x)));
}
template <size_t N>
KFR_SINTRIN vec<f64, N> round(const vec<f64, N>& x)
{
    return cast<f64>(cast<i64>(x + mulsign(broadcast<N>(0.5), x)));
}
template <size_t N>
KFR_SINTRIN vec<f32, N> trunc(const vec<f32, N>& x)
{
    return cast<f32>(cast<i32>(x));
}
template <size_t N>
KFR_SINTRIN vec<f64, N> trunc(const vec<f64, N>& x)
{
    return cast<f64>(cast<i64>(x));
}
template <size_t N>
KFR_SINTRIN vec<f32, N> fract(const vec<f32, N>& x)
{
    return x - floor(x);
}
template <size_t N>
KFR_SINTRIN vec<f64, N> fract(const vec<f64, N>& x)
{
    return x - floor(x);
}
#endif

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> floor(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> ceil(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> trunc(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> round(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> fract(const vec<T, N>&)
{
    return T(0);
}

template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> ifloor(const vec<T, N>& value)
{
    return cast<IT>(floor(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> iceil(const vec<T, N>& value)
{
    return cast<IT>(ceil(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> itrunc(const vec<T, N>& value)
{
    return cast<IT>(trunc(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_SINTRIN vec<IT, N> iround(const vec<T, N>& value)
{
    return cast<IT>(round(value));
}

KFR_I_CONVERTER(floor)
KFR_I_CONVERTER(ceil)
KFR_I_CONVERTER(round)
KFR_I_CONVERTER(trunc)
KFR_I_CONVERTER(fract)
KFR_I_CONVERTER(ifloor)
KFR_I_CONVERTER(iceil)
KFR_I_CONVERTER(iround)
KFR_I_CONVERTER(itrunc)
}
KFR_I_FN(floor)
KFR_I_FN(ceil)
KFR_I_FN(round)
KFR_I_FN(trunc)
KFR_I_FN(fract)
KFR_I_FN(ifloor)
KFR_I_FN(iceil)
KFR_I_FN(iround)
KFR_I_FN(itrunc)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 floor(const T1& x)
{
    return intrinsics::floor(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::floor, E1> floor(E1&& x)
{
    return { fn::floor(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 ceil(const T1& x)
{
    return intrinsics::ceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::ceil, E1> ceil(E1&& x)
{
    return { fn::ceil(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 round(const T1& x)
{
    return intrinsics::round(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::round, E1> round(E1&& x)
{
    return { fn::round(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 trunc(const T1& x)
{
    return intrinsics::trunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::trunc, E1> trunc(E1&& x)
{
    return { fn::trunc(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 fract(const T1& x)
{
    return intrinsics::fract(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::fract, E1> fract(E1&& x)
{
    return { fn::fract(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> ifloor(const T1& x)
{
    return intrinsics::ifloor(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::ifloor, E1> ifloor(E1&& x)
{
    return { fn::ifloor(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> iceil(const T1& x)
{
    return intrinsics::iceil(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::iceil, E1> iceil(E1&& x)
{
    return { fn::iceil(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> iround(const T1& x)
{
    return intrinsics::iround(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::iround, E1> iround(E1&& x)
{
    return { fn::iround(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN itype<T1> itrunc(const T1& x)
{
    return intrinsics::itrunc(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::itrunc, E1> itrunc(E1&& x)
{
    return { fn::itrunc(), std::forward<E1>(x) };
}

template <typename T, KFR_ENABLE_IF(is_f_class<T>::value)>
CMT_INLINE T fmod(const T& x, const T& y)
{
    return x - trunc(x / y) * y;
}
KFR_FN(fmod)

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
constexpr CMT_INLINE vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return x % y;
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
CMT_INLINE vec<T, N> rem(const vec<T, N>& x, const vec<T, N>& y)
{
    return fmod(x, y);
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
