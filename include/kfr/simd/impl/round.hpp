/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
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

#include "../operators.hpp"
#include "abs.hpp"
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#define KFR_mm_trunc_ps(V) _mm_round_ps((V), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define KFR_mm_roundnearest_ps(V) _mm_round_ps((V), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)
#define KFR_mm_trunc_pd(V) _mm_round_pd((V), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define KFR_mm_roundnearest_pd(V) _mm_round_pd((V), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)

#define KFR_mm_trunc_ss(V) _mm_round_ss(_mm_setzero_ps(), (V), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define KFR_mm_roundnearest_ss(V)                                                                            \
    _mm_round_ss(_mm_setzero_ps(), (V), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)
#define KFR_mm_trunc_sd(V) _mm_round_sd(_mm_setzero_pd(), (V), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define KFR_mm_roundnearest_sd(V)                                                                            \
    _mm_round_sd(_mm_setzero_pd(), (V), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)

#define KFR_mm_floor_ss(V) _mm_floor_ss(_mm_setzero_ps(), (V))
#define KFR_mm_floor_sd(V) _mm_floor_sd(_mm_setzero_pd(), (V))
#define KFR_mm_ceil_ss(V) _mm_ceil_ss(_mm_setzero_ps(), (V))
#define KFR_mm_ceil_sd(V) _mm_ceil_sd(_mm_setzero_pd(), (V))

#define KFR_mm256_trunc_ps(V) _mm256_round_ps((V), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define KFR_mm256_roundnearest_ps(V) _mm256_round_ps((V), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)
#define KFR_mm256_trunc_pd(V) _mm256_round_pd((V), _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC)
#define KFR_mm256_roundnearest_pd(V) _mm256_round_pd((V), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC)

#if defined CMT_ARCH_SSE41 && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC f32sse floor(const f32sse& value) { return _mm_floor_ps(value.v); }
KFR_INTRINSIC f32sse ceil(const f32sse& value) { return _mm_ceil_ps(value.v); }
KFR_INTRINSIC f32sse trunc(const f32sse& value) { return KFR_mm_trunc_ps(value.v); }
KFR_INTRINSIC f32sse round(const f32sse& value) { return KFR_mm_roundnearest_ps(value.v); }
KFR_INTRINSIC f64sse floor(const f64sse& value) { return _mm_floor_pd(value.v); }
KFR_INTRINSIC f64sse ceil(const f64sse& value) { return _mm_ceil_pd(value.v); }
KFR_INTRINSIC f64sse trunc(const f64sse& value) { return KFR_mm_trunc_pd(value.v); }
KFR_INTRINSIC f64sse round(const f64sse& value) { return KFR_mm_roundnearest_pd(value.v); }
KFR_INTRINSIC f32sse fract(const f32sse& x) { return x - floor(x); }
KFR_INTRINSIC f64sse fract(const f64sse& x) { return x - floor(x); }

#if defined CMT_ARCH_AVX

KFR_INTRINSIC f32avx floor(const f32avx& value) { return _mm256_floor_ps(value.v); }
KFR_INTRINSIC f32avx ceil(const f32avx& value) { return _mm256_ceil_ps(value.v); }
KFR_INTRINSIC f32avx trunc(const f32avx& value) { return KFR_mm256_trunc_ps(value.v); }
KFR_INTRINSIC f32avx round(const f32avx& value) { return KFR_mm256_roundnearest_ps(value.v); }
KFR_INTRINSIC f64avx floor(const f64avx& value) { return _mm256_floor_pd(value.v); }
KFR_INTRINSIC f64avx ceil(const f64avx& value) { return _mm256_ceil_pd(value.v); }
KFR_INTRINSIC f64avx trunc(const f64avx& value) { return KFR_mm256_trunc_pd(value.v); }
KFR_INTRINSIC f64avx round(const f64avx& value) { return KFR_mm256_roundnearest_pd(value.v); }
KFR_INTRINSIC f32avx fract(const f32avx& x) { return x - floor(x); }
KFR_INTRINSIC f64avx fract(const f64avx& x) { return x - floor(x); }

#endif

#if defined CMT_ARCH_AVX512

KFR_INTRINSIC f32avx512 floor(const f32avx512& value)
{
    return _mm512_roundscale_ps(value.v, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f32avx512 ceil(const f32avx512& value)
{
    return _mm512_roundscale_ps(value.v, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f32avx512 trunc(const f32avx512& value)
{
    return _mm512_roundscale_ps(value.v, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f32avx512 round(const f32avx512& value)
{
    return _mm512_roundscale_ps(value.v, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f64avx512 floor(const f64avx512& value)
{
    return _mm512_roundscale_pd(value.v, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f64avx512 ceil(const f64avx512& value)
{
    return _mm512_roundscale_pd(value.v, _MM_FROUND_TO_POS_INF | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f64avx512 trunc(const f64avx512& value)
{
    return _mm512_roundscale_pd(value.v, _MM_FROUND_TO_ZERO | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f64avx512 round(const f64avx512& value)
{
    return _mm512_roundscale_pd(value.v, _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
}
KFR_INTRINSIC f32avx512 fract(const f32avx512& x) { return x - floor(x); }
KFR_INTRINSIC f64avx512 fract(const f64avx512& x) { return x - floor(x); }
#endif

KFR_HANDLE_ALL_SIZES_1_IF(floor, is_f_class<T>)
KFR_HANDLE_ALL_SIZES_1_IF(ceil, is_f_class<T>)
KFR_HANDLE_ALL_SIZES_1_IF(round, is_f_class<T>)
KFR_HANDLE_ALL_SIZES_1_IF(trunc, is_f_class<T>)
KFR_HANDLE_ALL_SIZES_1_IF(fract, is_f_class<T>)

#else

// fallback

template <typename T>
constexpr inline T fp_precision_limit = 4503599627370496.0;
template <>
constexpr inline f32 fp_precision_limit<f32> = 16777216.0f;

template <size_t N>
KFR_INTRINSIC vec<f32, N> floor(const vec<f32, N>& x)
{
    vec<f32, N> t = broadcastto<f32>(broadcastto<i32>(x));
    return select(abs(x) >= fp_precision_limit<f32>, x, t - select(x < t, 1.f, 0.f));
}
template <size_t N>
KFR_INTRINSIC vec<f64, N> floor(const vec<f64, N>& x)
{
    vec<f64, N> t = broadcastto<f64>(broadcastto<i64>(x));
    return select(abs(x) >= fp_precision_limit<f64>, x, t - select(x < t, 1., 0.));
}
template <size_t N>
KFR_INTRINSIC vec<f32, N> ceil(const vec<f32, N>& x)
{
    vec<f32, N> t = broadcastto<f32>(broadcastto<i32>(x));
    return select(abs(x) >= fp_precision_limit<f32>, x, t + select(x > t, 1.f, 0.f));
}
template <size_t N>
KFR_INTRINSIC vec<f64, N> ceil(const vec<f64, N>& x)
{
    vec<f64, N> t = broadcastto<f64>(broadcastto<i64>(x));
    return select(abs(x) >= fp_precision_limit<f64>, x, t + select(x > t, 1., 0.));
}
template <size_t N>
KFR_INTRINSIC vec<f32, N> round(const vec<f32, N>& x)
{
    return select(abs(x) >= fp_precision_limit<f32>, x,
                  broadcastto<f32>(broadcastto<i32>(x + mulsign(broadcast<N>(0.5f), x))));
}
template <size_t N>
KFR_INTRINSIC vec<f64, N> round(const vec<f64, N>& x)
{
    return select(abs(x) >= fp_precision_limit<f64>, x,
                  broadcastto<f64>(broadcastto<i64>(x + mulsign(broadcast<N>(0.5), x))));
}
template <size_t N>
KFR_INTRINSIC vec<f32, N> trunc(const vec<f32, N>& x)
{
    return select(abs(x) >= fp_precision_limit<f32>, x, broadcastto<f32>(broadcastto<i32>(x)));
}
template <size_t N>
KFR_INTRINSIC vec<f64, N> trunc(const vec<f64, N>& x)
{
    return select(abs(x) >= fp_precision_limit<f64>, x, broadcastto<f64>(broadcastto<i64>(x)));
}
template <size_t N>
KFR_INTRINSIC vec<f32, N> fract(const vec<f32, N>& x)
{
    return x - floor(x);
}
template <size_t N>
KFR_INTRINSIC vec<f64, N> fract(const vec<f64, N>& x)
{
    return x - floor(x);
}
#endif

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> floor(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> ceil(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> trunc(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> round(const vec<T, N>& value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> fract(const vec<T, N>&)
{
    return T(0);
}

template <typename T, size_t N, typename IT = itype<T>>
KFR_INTRINSIC vec<IT, N> ifloor(const vec<T, N>& value)
{
    return broadcastto<IT>(floor(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_INTRINSIC vec<IT, N> iceil(const vec<T, N>& value)
{
    return broadcastto<IT>(ceil(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_INTRINSIC vec<IT, N> itrunc(const vec<T, N>& value)
{
    return broadcastto<IT>(trunc(value));
}
template <typename T, size_t N, typename IT = itype<T>>
KFR_INTRINSIC vec<IT, N> iround(const vec<T, N>& value)
{
    return broadcastto<IT>(round(value));
}

KFR_HANDLE_SCALAR(floor)
KFR_HANDLE_SCALAR(ceil)
KFR_HANDLE_SCALAR(round)
KFR_HANDLE_SCALAR(trunc)
KFR_HANDLE_SCALAR(fract)
KFR_HANDLE_SCALAR(ifloor)
KFR_HANDLE_SCALAR(iceil)
KFR_HANDLE_SCALAR(iround)
KFR_HANDLE_SCALAR(itrunc)
} // namespace intrinsics
KFR_I_FN(floor)
KFR_I_FN(ceil)
KFR_I_FN(round)
KFR_I_FN(trunc)
KFR_I_FN(fract)
KFR_I_FN(ifloor)
KFR_I_FN(iceil)
KFR_I_FN(iround)
KFR_I_FN(itrunc)
} // namespace CMT_ARCH_NAME
} // namespace kfr

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
