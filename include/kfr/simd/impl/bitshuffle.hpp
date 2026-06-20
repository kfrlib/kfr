/** @addtogroup basic_math
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

#include "../platform.hpp"

namespace kfr
{

#if defined(__GNUC__) || defined(__clang__)
// #define KFR_ENABLE_GAS 1
#endif

inline namespace KFR_ARCH_NAME
{

namespace intr
{

template <typename T>
constexpr inline size_t in_reg_bits = ilog2(vector_width<T>);

template <typename T>
struct bitperm_op
{
    uint8_t p[in_reg_bits<T> + 1];
};

template <typename T>
constexpr inline std::nullptr_t bitperm_ops{};

#ifdef KFR_ARCH_AVX

template <>
constexpr inline bitperm_op<double> bitperm_ops<double>[] = {
    { 2, 1, 0 }, // Op 0
    { 0, 2, 1 }, // Op 1
    { 2, 0, 1 }, // Op 2
#ifdef KFR_ARCH_AVX2
    { 1, 0, 2 }, // Op 3
#endif
};

template <int op_idx>
KFR_INTRINSIC void bitpermute_pair(__m256d& r0, __m256d& r1, __m256d v0, __m256d v1) noexcept;

template <>
KFR_INTRINSIC void bitpermute_pair<0>(__m256d& r0, __m256d& r1, __m256d x0, __m256d x1) noexcept
{
    // bits: {2, 1, 0}
    r0 = _mm256_unpacklo_pd(x0, x1);
    r1 = _mm256_unpackhi_pd(x0, x1);
}
template <>
KFR_INTRINSIC void bitpermute_pair<1>(__m256d& r0, __m256d& r1, __m256d x0, __m256d x1) noexcept
{
    // bits: {0, 2, 1}
    // elements: {0, 1, 4, 5, 2, 3, 6, 7}
    r0 = _mm256_insertf128_pd(x0, _mm256_castpd256_pd128(x1), 1);
    r1 = _mm256_permute2f128_pd(x0, x1, 0x31);
}

template <>
KFR_INTRINSIC void bitpermute_pair<2>(__m256d& r0, __m256d& r1, __m256d x0, __m256d x1) noexcept
{
    // bits: {2, 0, 1}
    // elements: {0, 4, 1, 5, 2, 6, 3, 7}
    const __m256d t0 = _mm256_shuffle_pd(x0, x1, 0x0); // [0, 4, 2, 6]
    const __m256d t1 = _mm256_shuffle_pd(x0, x1, 0xF); // [1, 5, 3, 7]
    r0               = _mm256_insertf128_pd(t0, _mm256_castpd256_pd128(t1), 1); // [0, 4, 1, 5]
    r1               = _mm256_permute2f128_pd(t0, t1, 0x31); // [2, 6, 3, 7]
}
#ifdef KFR_ARCH_AVX2
template <>
KFR_INTRINSIC void bitpermute_pair<3>(__m256d& r0, __m256d& r1, __m256d x0, __m256d x1) noexcept
{
    // bits: {1, 0, 2}
    // 1 instruction per register: permute [0,1,2,3] → [0,2,1,3]
    r0 = _mm256_permute4x64_pd(x0, 0xD8); // 0xD8 = 0b11_01_10_00
    r1 = _mm256_permute4x64_pd(x1, 0xD8);
}
#endif

template <>
constexpr inline bitperm_op<float> bitperm_ops<float>[] = {
    { 1, 3, 2, 0 }, // Op 0
    { 0, 3, 2, 1 }, // Op 1
    { 0, 1, 3, 2 }, // Op 2
    { 3, 0, 2, 1 }, // Op 3
    { 1, 0, 2, 3 }, // Op 4
    { 3, 1, 2, 0 }, // Op 5
#ifdef KFR_ARCH_AVX2
    { 0, 2, 1, 3 }, // Op 6
    { 2, 1, 0, 3 }, // Op 7
#endif
};

template <int op_idx>
KFR_INTRINSIC void bitpermute_pair(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept;

template <>
KFR_INTRINSIC void bitpermute_pair<0>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // elements: {0, 2, 8, 10, 4, 6, 12, 14, 1, 3, 9, 11, 5, 7, 13, 15}
    // bits: {1, 3, 2, 0}
    r0 = _mm256_shuffle_ps(v0, v1, 0x88);
    r1 = _mm256_shuffle_ps(v0, v1, 0xDD);
}
template <>
KFR_INTRINSIC void bitpermute_pair<1>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // elements: {0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 10, 11, 6, 7, 14, 15}
    // bits: {0, 3, 2, 1}
    r0 = _mm256_shuffle_ps(v0, v1, 0x44);
    r1 = _mm256_shuffle_ps(v0, v1, 0xEE);
}
template <>
KFR_INTRINSIC void bitpermute_pair<2>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // elements: {0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7, 12, 13, 14, 15}
    // bits:{0, 1, 3, 2}
#ifdef KFR_ENABLE_GAS
    __asm__ __volatile__("vperm2f128 $0x20, %[v1], %[v0], %[r0]\n\t"
                         "vperm2f128 $0x31, %[v1], %[v0], %[r1]"
                         : [r0] "=&v"(r0), [r1] "=&v"(r1)
                         : [v0] "v"(v0), [v1] "v"(v1));
#else
    r0 = _mm256_permute2f128_ps(v0, v1, 0x20);
    r1 = _mm256_permute2f128_ps(v0, v1, 0x31);
#endif
}
template <>
KFR_INTRINSIC void bitpermute_pair<3>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // elements: {0, 8, 1, 9, 4, 12, 5, 13, 2, 10, 3, 11, 6, 14, 7, 15}
    // bits: {3, 0, 2, 1}
    r0 = _mm256_unpacklo_ps(v0, v1);
    r1 = _mm256_unpackhi_ps(v0, v1);
}
template <>
KFR_INTRINSIC void bitpermute_pair<4>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // bits: {1, 0, 2, 3}
    r0 = _mm256_permute_ps(v0, 0xD8);
    r1 = _mm256_permute_ps(v1, 0xD8);
}
template <>
KFR_INTRINSIC void bitpermute_pair<5>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // bits: {3, 1, 2, 0}
#ifdef KFR_ENABLE_GAS
    __asm__ __volatile__("vpermilps $0xB1, %[x1], %[r0]\n\t"
                         "vpermilps $0xB1, %[x0], %[r1]\n\t"
                         "vblendps $0xAA, %[r0], %[x0], %[r0]\n\t"
                         "vblendps $0xAA, %[x1], %[r1], %[r1]"
                         : [r0] "=&v"(r0), [r1] "=&v"(r1)
                         : [x0] "v"(v0), [x1] "v"(v1));
#else
    const __m256 v0_swapped = _mm256_permute_ps(v0, 0xB1);
    const __m256 v1_swapped = _mm256_permute_ps(v1, 0xB1);
    r0                      = _mm256_blend_ps(v0, v1_swapped, 0xAA);
    r1                      = _mm256_blend_ps(v0_swapped, v1, 0xAA);
#endif
}

#ifdef KFR_ARCH_AVX2
template <>
KFR_INTRINSIC void bitpermute_pair<6>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // bits: {0, 2, 1, 3}
    r0 = _mm256_castpd_ps(_mm256_permute4x64_pd(_mm256_castps_pd(v0), 0xD8));
    r1 = _mm256_castpd_ps(_mm256_permute4x64_pd(_mm256_castps_pd(v1), 0xD8));
}
template <>
KFR_INTRINSIC void bitpermute_pair<7>(__m256& r0, __m256& r1, __m256 v0, __m256 v1) noexcept
{
    // bits: {2, 1, 0, 3}
    const __m256i mask = _mm256_setr_epi32(0, 4, 2, 6, 1, 5, 3, 7);
    r0                 = _mm256_permutevar8x32_ps(v0, mask);
    r1                 = _mm256_permutevar8x32_ps(v1, mask);
}
#endif

#elif defined KFR_ARCH_SSE2

template <>
constexpr inline bitperm_op<double> bitperm_ops<double>[] = {
    { 1, 0 }, // Op 0
};

template <int op_idx>
KFR_INTRINSIC void bitpermute_pair(__m128d& r0, __m128d& r1, __m128d v0, __m128d v1) noexcept;

template <>
KFR_INTRINSIC void bitpermute_pair<0>(__m128d& r0, __m128d& r1, __m128d x0, __m128d x1) noexcept
{
    // elements: {0, 2, 1, 3}
    // bits: {1, 0}
    r0 = _mm_unpacklo_pd(x0, x1);
    r1 = _mm_unpackhi_pd(x0, x1);
}

template <>
constexpr inline bitperm_op<float> bitperm_ops<float>[] = {
    { 2, 0, 1 }, // Op 0
    { 1, 2, 0 }, // Op 1
    { 0, 2, 1 }, // Op 2
    { 1, 0, 2 }, // Op 3
};
template <int op_idx>
KFR_INTRINSIC void bitpermute_pair(__m128& r0, __m128& r1, __m128 v0, __m128 v1) noexcept;

template <>
KFR_INTRINSIC void bitpermute_pair<0>(__m128& r0, __m128& r1, __m128 x0, __m128 x1) noexcept
{
    // bits: {2, 0, 1}
    // elements: {0, 4, 1, 5, 2, 6, 3, 7}
    r0 = _mm_unpacklo_ps(x0, x1);
    r1 = _mm_unpackhi_ps(x0, x1);
}
template <>
KFR_INTRINSIC void bitpermute_pair<1>(__m128& r0, __m128& r1, __m128 x0, __m128 x1) noexcept
{
    // bits: {1, 2, 0}
    // elements: {0, 2, 4, 6, 1, 3, 5, 7}
    r0 = _mm_shuffle_ps(x0, x1, 0x88);
    r1 = _mm_shuffle_ps(x0, x1, 0xDD);
}
template <>
KFR_INTRINSIC void bitpermute_pair<2>(__m128& r0, __m128& r1, __m128 x0, __m128 x1) noexcept
{
    // bits: {0, 2, 1}
    // elements: {0, 1, 4, 5, 2, 3, 6, 7}
    r0 = _mm_shuffle_ps(x0, x1, 0x44);
    r1 = _mm_shuffle_ps(x0, x1, 0xEE);
}

template <>
KFR_INTRINSIC void bitpermute_pair<3>(__m128& r0, __m128& r1, __m128 x0, __m128 x1) noexcept
{
    // bits: {1, 0, 2}
    // elements: {0, 2, 1, 3, 4, 6, 5, 7}
    r0 = _mm_shuffle_ps(x0, x0, 0xD8);
    r1 = _mm_shuffle_ps(x1, x1, 0xD8);
}

#elif defined __ARM_NEON || defined __ARM_NEON__

// float32x4_t: 4 elements per register, pair holds 8 elements → 3-bit index.
// Available on both AArch32 (neon) and AArch64 (neon64).

template <>
constexpr inline bitperm_op<float> bitperm_ops<float>[] = {
    { 2, 0, 1 }, // Op 0 - vzipq  (interleave)
    { 1, 2, 0 }, // Op 1 - vuzpq  (deinterleave)
    { 0, 2, 1 }, // Op 2 - vcombine low/high halves
    { 1, 0, 2 }, // Op 3 - within-register bit-0/bit-1 swap
};

template <int op_idx>
KFR_INTRINSIC void bitpermute_pair(float32x4_t& r0, float32x4_t& r1, float32x4_t v0, float32x4_t v1) noexcept;

template <>
KFR_INTRINSIC void bitpermute_pair<0>(float32x4_t& r0, float32x4_t& r1, float32x4_t v0,
                                      float32x4_t v1) noexcept
{
    // bits: {2, 0, 1}
    // elements: {0, 4, 1, 5, 2, 6, 3, 7}
    const float32x4x2_t t = vzipq_f32(v0, v1);
    r0                    = t.val[0];
    r1                    = t.val[1];
}

template <>
KFR_INTRINSIC void bitpermute_pair<1>(float32x4_t& r0, float32x4_t& r1, float32x4_t v0,
                                      float32x4_t v1) noexcept
{
    // bits: {1, 2, 0}
    // elements: {0, 2, 4, 6, 1, 3, 5, 7}
    const float32x4x2_t t = vuzpq_f32(v0, v1);
    r0                    = t.val[0];
    r1                    = t.val[1];
}

template <>
KFR_INTRINSIC void bitpermute_pair<2>(float32x4_t& r0, float32x4_t& r1, float32x4_t v0,
                                      float32x4_t v1) noexcept
{
    // bits: {0, 2, 1}
    // elements: {0, 1, 4, 5, 2, 3, 6, 7}
    r0 = vcombine_f32(vget_low_f32(v0), vget_low_f32(v1));
    r1 = vcombine_f32(vget_high_f32(v0), vget_high_f32(v1));
}

template <>
KFR_INTRINSIC void bitpermute_pair<3>(float32x4_t& r0, float32x4_t& r1, float32x4_t v0,
                                      float32x4_t v1) noexcept
{
    // bits: {1, 0, 2}
    // elements: {0, 2, 1, 3, 4, 6, 5, 7}
    // Within each register independently: permute [0,1,2,3] → [0,2,1,3]
    // vtrn_f32(low, high): val[0]=[low[0],high[0]], val[1]=[low[1],high[1]]
    const float32x2x2_t t0 = vtrn_f32(vget_low_f32(v0), vget_high_f32(v0));
    const float32x2x2_t t1 = vtrn_f32(vget_low_f32(v1), vget_high_f32(v1));
    r0                     = vcombine_f32(t0.val[0], t0.val[1]);
    r1                     = vcombine_f32(t1.val[0], t1.val[1]);
}

// float64x2_t: 2 elements per register, pair holds 4 elements → 2-bit index.
// Only available on AArch64 (neon64).
#ifdef __aarch64__

template <>
constexpr inline bitperm_op<double> bitperm_ops<double>[] = {
    { 1, 0 }, // Op 0 - zip low halves (interleave)
};

template <int op_idx>
KFR_INTRINSIC void bitpermute_pair(float64x2_t& r0, float64x2_t& r1, float64x2_t v0, float64x2_t v1) noexcept;

template <>
KFR_INTRINSIC void bitpermute_pair<0>(float64x2_t& r0, float64x2_t& r1, float64x2_t v0,
                                      float64x2_t v1) noexcept
{
    // bits: {1, 0}
    // elements: {0, 2, 1, 3}
    r0 = vcombine_f64(vget_low_f64(v0), vget_low_f64(v1));
    r1 = vcombine_f64(vget_high_f64(v0), vget_high_f64(v1));
}

#endif // __aarch64__

#endif
} // namespace intr
} // namespace KFR_ARCH_NAME
} // namespace kfr
