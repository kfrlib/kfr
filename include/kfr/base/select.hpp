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

namespace kfr
{
namespace intrinsics
{

#if defined CMT_ARCH_SSE41

KFR_SINTRIN u8sse select(const mu8sse& m, const u8sse& x, const u8sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN u16sse select(const mu16sse& m, const u16sse& x, const u16sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN u32sse select(const mu32sse& m, const u32sse& x, const u32sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN u64sse select(const mu64sse& m, const u64sse& x, const u64sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i8sse select(const mi8sse& m, const i8sse& x, const i8sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i16sse select(const mi16sse& m, const i16sse& x, const i16sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i32sse select(const mi32sse& m, const i32sse& x, const i32sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i64sse select(const mi64sse& m, const i64sse& x, const i64sse& y)
{
    return _mm_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN f32sse select(const mf32sse& m, const f32sse& x, const f32sse& y)
{
    return _mm_blendv_ps(*y, *x, *m);
}
KFR_SINTRIN f64sse select(const mf64sse& m, const f64sse& x, const f64sse& y)
{
    return _mm_blendv_pd(*y, *x, *m);
}

#if defined CMT_ARCH_AVX
KFR_SINTRIN f64avx select(const mf64avx& m, const f64avx& x, const f64avx& y)
{
    return _mm256_blendv_pd(*y, *x, *m);
}
KFR_SINTRIN f32avx select(const mf32avx& m, const f32avx& x, const f32avx& y)
{
    return _mm256_blendv_ps(*y, *x, *m);
}
#endif

#if defined CMT_ARCH_AVX2
KFR_SINTRIN u8avx select(const mu8avx& m, const u8avx& x, const u8avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN u16avx select(const mu16avx& m, const u16avx& x, const u16avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN u32avx select(const mu32avx& m, const u32avx& x, const u32avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN u64avx select(const mu64avx& m, const u64avx& x, const u64avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i8avx select(const mi8avx& m, const i8avx& x, const i8avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i16avx select(const mi16avx& m, const i16avx& x, const i16avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i32avx select(const mi32avx& m, const i32avx& x, const i32avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
KFR_SINTRIN i64avx select(const mi64avx& m, const i64avx& x, const i64avx& y)
{
    return _mm256_blendv_epi8(*y, *x, *m);
}
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>
KFR_SINTRIN vec<T, N> select(const mask<T, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    return slice<0, N>(select(expand_simd(a).asmask(), expand_simd(b), expand_simd(c)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>
KFR_SINTRIN vec<T, N> select(const mask<T, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    return concat(select(low(a).asmask(), low(b), low(c)), select(high(a).asmask(), high(b), high(c)));
}

#elif defined CMT_ARCH_NEON

KFR_SINTRIN f32neon select(const mf32neon& m, const f32neon& x, const f32neon& y)
{
    return vbslq_f32(*m, *x, *y);
}

KFR_SINTRIN i8neon select(const mi8neon& m, const i8neon& x, const i8neon& y) { return vbslq_s8(*m, *x, *y); }
KFR_SINTRIN u8neon select(const mu8neon& m, const u8neon& x, const u8neon& y) { return vbslq_u8(*m, *x, *y); }
KFR_SINTRIN i16neon select(const mi16neon& m, const i16neon& x, const i16neon& y)
{
    return vbslq_s16(*m, *x, *y);
}
KFR_SINTRIN u16neon select(const mu16neon& m, const u16neon& x, const u16neon& y)
{
    return vbslq_u16(*m, *x, *y);
}
KFR_SINTRIN i32neon select(const mi32neon& m, const i32neon& x, const i32neon& y)
{
    return vbslq_s32(*m, *x, *y);
}
KFR_SINTRIN u32neon select(const mu32neon& m, const u32neon& x, const u32neon& y)
{
    return vbslq_u32(*m, *x, *y);
}
KFR_SINTRIN i64neon select(const mi64neon& m, const i64neon& x, const i64neon& y)
{
    return vbslq_s64(*m, *x, *y);
}
KFR_SINTRIN u64neon select(const mu64neon& m, const u64neon& x, const u64neon& y)
{
    return vbslq_u64(*m, *x, *y);
}

#ifdef CMT_ARCH_NEON64
KFR_SINTRIN f64neon select(const mf64neon& m, const f64neon& x, const f64neon& y)
{
    return vbslq_f64(*m, *x, *y);
}
#else
KFR_SINTRIN f64neon select(const mf64neon& m, const f64neon& x, const f64neon& y)
{
    return y ^ ((x ^ y) & f64neon(*m));
}
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>
KFR_SINTRIN vec<T, N> select(const mask<T, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    return slice<0, N>(select(expand_simd(a).asmask(), expand_simd(b), expand_simd(c)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>
KFR_SINTRIN vec<T, N> select(const mask<T, N>& a, const vec<T, N>& b, const vec<T, N>& c)
{
    return concat(select(low(a).asmask(), low(b), low(c)), select(high(a).asmask(), high(b), high(c)));
}

#else

// fallback
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> select(const mask<T, N>& m, const vec<T, N>& x, const vec<T, N>& y)
{
    return y ^ ((x ^ y) & vec<T, N>(*m));
}
#endif

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> select(const vec<T, N>& m, const vec<T, N>& x, const vec<T, N>& y)
{
    return select(mask<T, N>(*m), x, y);
}
}
KFR_I_FN(select)

/**
 * @brief Returns x if m is true, otherwise return y. Order of the arguments is same as in ternary operator.
 * @code
 * return m ? x : y
 * @endcode
 */
template <typename T1, size_t N, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value),
          typename Tout = subtype<common_type<T2, T3>>>
KFR_INTRIN vec<Tout, N> select(const mask<T1, N>& m, const T2& x, const T3& y)
{
    static_assert(sizeof(T1) == sizeof(Tout), "select: incompatible types");
    return intrinsics::select(bitcast<Tout>(m), static_cast<vec<Tout, N>>(x), static_cast<vec<Tout, N>>(y));
}

/**
 * @brief Returns template expression that returns x if m is true, otherwise return y. Order of the arguments
 * is same as in ternary operator.
 */
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INTRIN internal::expression_function<fn::select, E1, E2, E3> select(E1&& m, E2&& x, E3&& y)
{
    return { fn::select(), std::forward<E1>(m), std::forward<E2>(x), std::forward<E3>(y) };
}
}
