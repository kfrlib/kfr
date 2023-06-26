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
#include "../select.hpp"
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#if defined CMT_ARCH_SSSE3 && defined KFR_NATIVE_INTRINSICS

// floating point
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> abs(const vec<T, N>& x) CMT_NOEXCEPT
{
    return x & special_constants<T>::invhighbitmask();
}

KFR_INTRINSIC i64sse abs(const i64sse& x) CMT_NOEXCEPT
{
    const __m128i sh  = _mm_srai_epi32(x.v, 31);
    const __m128i msk = _mm_shuffle_epi32(sh, _MM_SHUFFLE(3, 3, 1, 1));
    return _mm_sub_epi64(_mm_xor_si128(x.v, msk), msk);
}
KFR_INTRINSIC i32sse abs(const i32sse& x) CMT_NOEXCEPT { return _mm_abs_epi32(x.v); }
KFR_INTRINSIC i16sse abs(const i16sse& x) CMT_NOEXCEPT { return _mm_abs_epi16(x.v); }
KFR_INTRINSIC i8sse abs(const i8sse& x) CMT_NOEXCEPT { return _mm_abs_epi8(x.v); }
KFR_INTRINSIC u64sse abs(const u64sse& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u32sse abs(const u32sse& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u16sse abs(const u16sse& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u8sse abs(const u8sse& x) CMT_NOEXCEPT { return x; }

#if defined CMT_ARCH_AVX2
KFR_INTRINSIC i64avx abs(const i64avx& x) CMT_NOEXCEPT
{
    const __m256i sh  = _mm256_srai_epi32(x.v, 31);
    const __m256i msk = _mm256_shuffle_epi32(sh, _MM_SHUFFLE(3, 3, 1, 1));
    return _mm256_sub_epi64(_mm256_xor_si256(x.v, msk), msk);
}
KFR_INTRINSIC i32avx abs(const i32avx& x) CMT_NOEXCEPT { return _mm256_abs_epi32(x.v); }
KFR_INTRINSIC i16avx abs(const i16avx& x) CMT_NOEXCEPT { return _mm256_abs_epi16(x.v); }
KFR_INTRINSIC i8avx abs(const i8avx& x) CMT_NOEXCEPT { return _mm256_abs_epi8(x.v); }
KFR_INTRINSIC u64avx abs(const u64avx& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u32avx abs(const u32avx& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u16avx abs(const u16avx& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u8avx abs(const u8avx& x) CMT_NOEXCEPT { return x; }
#endif

#if defined CMT_ARCH_AVX512
KFR_INTRINSIC i64avx512 abs(const i64avx512& x) CMT_NOEXCEPT { return _mm512_abs_epi64(x.v); }
KFR_INTRINSIC i32avx512 abs(const i32avx512& x) CMT_NOEXCEPT { return _mm512_abs_epi32(x.v); }
KFR_INTRINSIC i16avx512 abs(const i16avx512& x) CMT_NOEXCEPT { return _mm512_abs_epi16(x.v); }
KFR_INTRINSIC i8avx512 abs(const i8avx512& x) CMT_NOEXCEPT { return _mm512_abs_epi8(x.v); }
KFR_INTRINSIC u64avx512 abs(const u64avx512& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u32avx512 abs(const u32avx512& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u16avx512 abs(const u16avx512& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u8avx512 abs(const u8avx512& x) CMT_NOEXCEPT { return x; }
#endif

KFR_HANDLE_ALL_SIZES_1_IF(abs, !is_f_class<T>)

#elif defined CMT_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC i8neon abs(const i8neon& x) CMT_NOEXCEPT { return vabsq_s8(x.v); }
KFR_INTRINSIC i16neon abs(const i16neon& x) CMT_NOEXCEPT { return vabsq_s16(x.v); }
KFR_INTRINSIC i32neon abs(const i32neon& x) CMT_NOEXCEPT { return vabsq_s32(x.v); }
#if defined CMT_ARCH_NEON64
KFR_INTRINSIC i64neon abs(const i64neon& x) CMT_NOEXCEPT { return vabsq_s64(x.v); }
#else
KFR_INTRINSIC i64neon abs(const i64neon& x) CMT_NOEXCEPT { return select(x >= 0, x, -x); }
#endif

KFR_INTRINSIC u8neon abs(const u8neon& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u16neon abs(const u16neon& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u32neon abs(const u32neon& x) CMT_NOEXCEPT { return x; }
KFR_INTRINSIC u64neon abs(const u64neon& x) CMT_NOEXCEPT { return x; }

KFR_INTRINSIC f32neon abs(const f32neon& x) CMT_NOEXCEPT { return vabsq_f32(x.v); }
#if defined CMT_ARCH_NEON64
KFR_INTRINSIC f64neon abs(const f64neon& x) CMT_NOEXCEPT { return vabsq_f64(x.v); }
#else
KFR_INTRINSIC f64neon abs(const f64neon& x) CMT_NOEXCEPT
{
    return x & special_constants<f64>::invhighbitmask();
}
#endif

KFR_HANDLE_ALL_SIZES_1(abs)

#else

// floating point
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> abs(const vec<T, N>& x) CMT_NOEXCEPT
{
    return x & special_constants<T>::invhighbitmask();
}

// fallback
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> abs(const vec<T, N>& x) CMT_NOEXCEPT
{
    return select(x >= T(0), x, -x);
}
#endif
KFR_HANDLE_SCALAR(abs)
} // namespace intrinsics

KFR_I_FN(abs)
} // namespace CMT_ARCH_NAME
} // namespace kfr
