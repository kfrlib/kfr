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
namespace intrinsics
{

#if defined CID_ARCH_SSE41

KFR_SINTRIN u8sse select(mu8sse m, u8sse x, u8sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN u16sse select(mu16sse m, u16sse x, u16sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN u32sse select(mu32sse m, u32sse x, u32sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN u64sse select(mu64sse m, u64sse x, u64sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i8sse select(mi8sse m, i8sse x, i8sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i16sse select(mi16sse m, i16sse x, i16sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i32sse select(mi32sse m, i32sse x, i32sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i64sse select(mi64sse m, i64sse x, i64sse y) { return _mm_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN f32sse select(mf32sse m, f32sse x, f32sse y) { return _mm_blendv_ps(*y, *x, *m); }
KFR_SINTRIN f64sse select(mf64sse m, f64sse x, f64sse y) { return _mm_blendv_pd(*y, *x, *m); }

#if defined CID_ARCH_AVX
KFR_SINTRIN f64avx select(mf64avx m, f64avx x, f64avx y) { return _mm256_blendv_pd(*y, *x, *m); }
KFR_SINTRIN f32avx select(mf32avx m, f32avx x, f32avx y) { return _mm256_blendv_ps(*y, *x, *m); }
#endif

#if defined CID_ARCH_AVX2
KFR_SINTRIN u8avx select(mu8avx m, u8avx x, u8avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN u16avx select(mu16avx m, u16avx x, u16avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN u32avx select(mu32avx m, u32avx x, u32avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN u64avx select(mu64avx m, u64avx x, u64avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i8avx select(mi8avx m, i8avx x, i8avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i16avx select(mi16avx m, i16avx x, i16avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i32avx select(mi32avx m, i32avx x, i32avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
KFR_SINTRIN i64avx select(mi64avx m, i64avx x, i64avx y) { return _mm256_blendv_epi8(*y, *x, *m); }
#endif

template <typename T, size_t N, KFR_ENABLE_IF(N < vector_width<T, cpu_t::native>)>
KFR_SINTRIN vec<T, N> select(mask<T, N> a, vec<T, N> b, vec<T, N> c)
{
    return slice<0, N>(select(expand_simd(a).asmask(), expand_simd(b), expand_simd(c)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= vector_width<T, cpu_t::native>), typename = void>
KFR_SINTRIN vec<T, N> select(mask<T, N> a, vec<T, N> b, vec<T, N> c)
{
    return concat(select(low(a).asmask(), low(b), low(c)), select(high(a).asmask(), high(b), high(c)));
}

#else

// fallback
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> select(mask<T, N> m, vec<T, N> x, vec<T, N> y)
{
    return y ^ ((x ^ y) & m);
}
#endif
}
KFR_I_FN(select)

template <typename T1, size_t N, typename T2, typename T3, KFR_ENABLE_IF(is_numeric_args<T1, T2, T3>::value),
          typename Tout = subtype<common_type<T2, T3>>>
KFR_INTRIN vec<Tout, N> select(const mask<T1, N>& m, const T2& x, const T3& y)
{
    static_assert(sizeof(T1) == sizeof(Tout), "select: incompatible types");
    return intrinsics::select(bitcast<Tout>(m).asmask(), static_cast<vec<Tout, N>>(x),
                              static_cast<vec<Tout, N>>(y));
}

template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2, E3>::value)>
KFR_INTRIN expr_func<fn::select, E1, E2, E3> select(E1&& m, E2&& x, E3&& y)
{
    return { {}, std::forward<E1>(m), std::forward<E2>(x), std::forward<E3>(y) };
}
}
