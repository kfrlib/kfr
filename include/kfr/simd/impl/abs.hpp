/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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
inline namespace KFR_ARCH_NAME
{

namespace intr
{

#if defined KFR_ARCH_SSSE3 && defined KFR_NATIVE_INTRINSICS

// floating point
template <f_class T, size_t N>
KFR_INTRINSIC vec<T, N> abs(const vec<T, N>& x) noexcept
{
    return x & special_constants<T>::invhighbitmask();
}

KFR_INTRINSIC i64sse abs(const i64sse& x) noexcept
{
    const __m128i sh  = _mm_srai_epi32(x.v, 31);
    const __m128i msk = _mm_shuffle_epi32(sh, _MM_SHUFFLE(3, 3, 1, 1));
    return _mm_sub_epi64(_mm_xor_si128(x.v, msk), msk);
}
KFR_INTRINSIC i32sse abs(const i32sse& x) noexcept { return _mm_abs_epi32(x.v); }
KFR_INTRINSIC i16sse abs(const i16sse& x) noexcept { return _mm_abs_epi16(x.v); }
KFR_INTRINSIC i8sse abs(const i8sse& x) noexcept { return _mm_abs_epi8(x.v); }
KFR_INTRINSIC u64sse abs(const u64sse& x) noexcept { return x; }
KFR_INTRINSIC u32sse abs(const u32sse& x) noexcept { return x; }
KFR_INTRINSIC u16sse abs(const u16sse& x) noexcept { return x; }
KFR_INTRINSIC u8sse abs(const u8sse& x) noexcept { return x; }

#if defined KFR_ARCH_AVX2
KFR_INTRINSIC i64avx abs(const i64avx& x) noexcept
{
    const __m256i sh  = _mm256_srai_epi32(x.v, 31);
    const __m256i msk = _mm256_shuffle_epi32(sh, _MM_SHUFFLE(3, 3, 1, 1));
    return _mm256_sub_epi64(_mm256_xor_si256(x.v, msk), msk);
}
KFR_INTRINSIC i32avx abs(const i32avx& x) noexcept { return _mm256_abs_epi32(x.v); }
KFR_INTRINSIC i16avx abs(const i16avx& x) noexcept { return _mm256_abs_epi16(x.v); }
KFR_INTRINSIC i8avx abs(const i8avx& x) noexcept { return _mm256_abs_epi8(x.v); }
KFR_INTRINSIC u64avx abs(const u64avx& x) noexcept { return x; }
KFR_INTRINSIC u32avx abs(const u32avx& x) noexcept { return x; }
KFR_INTRINSIC u16avx abs(const u16avx& x) noexcept { return x; }
KFR_INTRINSIC u8avx abs(const u8avx& x) noexcept { return x; }
#endif

#if defined KFR_ARCH_AVX512
KFR_INTRINSIC i64avx512 abs(const i64avx512& x) noexcept { return _mm512_abs_epi64(x.v); }
KFR_INTRINSIC i32avx512 abs(const i32avx512& x) noexcept { return _mm512_abs_epi32(x.v); }
KFR_INTRINSIC i16avx512 abs(const i16avx512& x) noexcept { return _mm512_abs_epi16(x.v); }
KFR_INTRINSIC i8avx512 abs(const i8avx512& x) noexcept { return _mm512_abs_epi8(x.v); }
KFR_INTRINSIC u64avx512 abs(const u64avx512& x) noexcept { return x; }
KFR_INTRINSIC u32avx512 abs(const u32avx512& x) noexcept { return x; }
KFR_INTRINSIC u16avx512 abs(const u16avx512& x) noexcept { return x; }
KFR_INTRINSIC u8avx512 abs(const u8avx512& x) noexcept { return x; }
#endif

KFR_HANDLE_ALL_SIZES_1_IF(abs, !is_f_class<T>)

#elif defined KFR_ARCH_NEON && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC i8neon abs(const i8neon& x) noexcept { return vabsq_s8(x.v); }
KFR_INTRINSIC i16neon abs(const i16neon& x) noexcept { return vabsq_s16(x.v); }
KFR_INTRINSIC i32neon abs(const i32neon& x) noexcept { return vabsq_s32(x.v); }
#if defined KFR_ARCH_NEON64
KFR_INTRINSIC i64neon abs(const i64neon& x) noexcept { return vabsq_s64(x.v); }
#else
KFR_INTRINSIC i64neon abs(const i64neon& x) noexcept { return select(x >= 0, x, -x); }
#endif

KFR_INTRINSIC u8neon abs(const u8neon& x) noexcept { return x; }
KFR_INTRINSIC u16neon abs(const u16neon& x) noexcept { return x; }
KFR_INTRINSIC u32neon abs(const u32neon& x) noexcept { return x; }
KFR_INTRINSIC u64neon abs(const u64neon& x) noexcept { return x; }

KFR_INTRINSIC f32neon abs(const f32neon& x) noexcept { return vabsq_f32(x.v); }
#if defined KFR_ARCH_NEON64
KFR_INTRINSIC f64neon abs(const f64neon& x) noexcept { return vabsq_f64(x.v); }
#else
KFR_INTRINSIC f64neon abs(const f64neon& x) noexcept { return x & special_constants<f64>::invhighbitmask(); }
#endif

KFR_HANDLE_ALL_SIZES_1(abs)

#elif defined KFR_ARCH_RVV && defined KFR_NATIVE_INTRINSICS

KFR_INTRINSIC i8rvv abs(const i8rvv& x) noexcept
{
    vbool8_t mask = __riscv_vmslt_vx_i8m1_b8(x.v, 0, i8rvv::SN);
    return __riscv_vmerge_vvm_i8m1(x.v, __riscv_vneg_v_i8m1(x.v, i8rvv::SN), mask, i8rvv::SN);
}
KFR_INTRINSIC i16rvv abs(const i16rvv& x) noexcept
{
    vbool16_t mask = __riscv_vmslt_vx_i16m1_b16(x.v, 0, i16rvv::SN);
    return __riscv_vmerge_vvm_i16m1(x.v, __riscv_vneg_v_i16m1(x.v, i16rvv::SN), mask, i16rvv::SN);
}
KFR_INTRINSIC i32rvv abs(const i32rvv& x) noexcept
{
    vbool32_t mask = __riscv_vmslt_vx_i32m1_b32(x.v, 0, i32rvv::SN);
    return __riscv_vmerge_vvm_i32m1(x.v, __riscv_vneg_v_i32m1(x.v, i32rvv::SN), mask, i32rvv::SN);
}
KFR_INTRINSIC i64rvv abs(const i64rvv& x) noexcept
{
    vbool64_t mask = __riscv_vmslt_vx_i64m1_b64(x.v, 0, i64rvv::SN);
    return __riscv_vmerge_vvm_i64m1(x.v, __riscv_vneg_v_i64m1(x.v, i64rvv::SN), mask, i64rvv::SN);
}

KFR_INTRINSIC u8rvv abs(const u8rvv& x) noexcept { return x; }
KFR_INTRINSIC u16rvv abs(const u16rvv& x) noexcept { return x; }
KFR_INTRINSIC u32rvv abs(const u32rvv& x) noexcept { return x; }
KFR_INTRINSIC u64rvv abs(const u64rvv& x) noexcept { return x; }

KFR_INTRINSIC f32rvv abs(const f32rvv& x) noexcept { return __riscv_vfabs_v_f32m1(x.v, f32rvv::SN); }
KFR_INTRINSIC f64rvv abs(const f64rvv& x) noexcept { return __riscv_vfabs_v_f64m1(x.v, f64rvv::SN); }

KFR_HANDLE_ALL_SIZES_1(abs)

#else

// floating point
template <f_class T, size_t N>
KFR_INTRINSIC vec<T, N> abs(const vec<T, N>& x) noexcept
{
    return x & special_constants<T>::invhighbitmask();
}

// fallback
template <not_f_class T, size_t N>
KFR_INTRINSIC vec<T, N> abs(const vec<T, N>& x) noexcept
{
    return select(x >= T(0), x, -x);
}
#endif
KFR_HANDLE_SCALAR(abs)
} // namespace intr

KFR_I_FN(abs)
} // namespace KFR_ARCH_NAME
} // namespace kfr
