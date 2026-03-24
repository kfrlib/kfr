/**
 * Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 */
#pragma once

#include "../vec.hpp"
#ifndef KFR_SHUFFLE_SPECIALIZATIONS
#include "../shuffle.hpp"
#endif

#ifdef KFR_COMPILER_GNU

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

template <>
template <>
inline vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 1, 8, 9, 16, 17, 24, 25, 2, 3, 10, 11, 18, 19, 26, 27, 4, 5, 12, 13, 20, 21, 28, 29, 6, 7, 14,
             15, 22, 23, 30, 31>) const noexcept
{
    f32x32 w = *this;

    w = concat(permute<0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 10, 11, 6, 7, 14, 15>(low(w)),
               permute<0, 1, 8, 9, 4, 5, 12, 13, 2, 3, 10, 11, 6, 7, 14, 15>(high(w)));

    w = permutegroups<(4), 0, 4, 2, 6, 1, 5, 3, 7>(w); // avx: vperm2f128 & vinsertf128, sse: no-op
    return w;
}

#if defined __clang__ && defined __AVX2__
KFR_INLINE static void bitrev_permute_16x2(__m256 a[4])
{
    typedef float v8f __attribute__((ext_vector_type(8)));
    v8f d0 = (v8f)a[0]; // [c0,  c1,  c2,  c3 ]  (each cN = two consecutive floats)
    v8f d1 = (v8f)a[1]; // [c4,  c5,  c6,  c7 ]
    v8f d2 = (v8f)a[2]; // [c8,  c9,  c10, c11]
    v8f d3 = (v8f)a[3]; // [c12, c13, c14, c15]

    // ── Stage 1: unpacklo/hi_pd ───────────────────────────────────────────────
    // unpacklo_pd(d0,d2): pick lo-64 from each 128-bit lane of both sources
    //   lo-lane: c0(d0[0,1]),  c8 (d2[0,1])  │ indices: 0,1,  8, 9
    //   hi-lane: c2(d0[4,5]),  c10(d2[4,5])  │ indices: 4,5, 12,13
    v8f t0 = __builtin_shufflevector(d0, d2, 0, 1, 8, 9, 4, 5, 12, 13); // [c0,  c8,  c2,  c10]

    // unpackhi_pd(d0,d2): pick hi-64 from each 128-bit lane
    //   lo-lane: c1(d0[2,3]),  c9 (d2[2,3])  │ indices: 2,3, 10,11
    //   hi-lane: c3(d0[6,7]),  c11(d2[6,7])  │ indices: 6,7, 14,15
    v8f t1 = __builtin_shufflevector(d0, d2, 2, 3, 10, 11, 6, 7, 14, 15); // [c1,  c9,  c3,  c11]

    v8f t2 = __builtin_shufflevector(d1, d3, 0, 1, 8, 9, 4, 5, 12, 13); // [c4,  c12, c6,  c14]
    v8f t3 = __builtin_shufflevector(d1, d3, 2, 3, 10, 11, 6, 7, 14, 15); // [c5,  c13, c7,  c15]

    // ── Stage 2: permute2f128 ─────────────────────────────────────────────────
    // 0x20 → [lo128(src1) | lo128(src2)]: floats [0,1,2,3] + [8,9,10,11]
    // 0x31 → [hi128(src1) | hi128(src2)]: floats [4,5,6,7] + [12,13,14,15]
    a[0] = (v8f)__builtin_shufflevector(t0, t2, 0, 1, 2, 3, 8, 9, 10, 11); // [c0,  c8,  c4,  c12]
    a[1] = (v8f)__builtin_shufflevector(t0, t2, 4, 5, 6, 7, 12, 13, 14, 15); // [c2,  c10, c6,  c14]
    a[2] = (v8f)__builtin_shufflevector(t1, t3, 0, 1, 2, 3, 8, 9, 10, 11); // [c1,  c9,  c5,  c13]
    a[3] = (v8f)__builtin_shufflevector(t1, t3, 4, 5, 6, 7, 12, 13, 14, 15); // [c3,  c11, c7,  c15]
}
#endif

template <>
template <>
inline vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 1, 16, 17, 8, 9, 24, 25, 4, 5, 20, 21, 12, 13, 28, 29, 2, 3, 18, 19, 10, 11, 26, 27, 6, 7, 22,
             23, 14, 15, 30, 31>) const noexcept
{
    vec<f32, 32> w = *this;
#if defined __clang__ && defined __AVX2__
    bitrev_permute_16x2(reinterpret_cast<__m256*>(&w));
    return w;
#else
    w = concat(permute<0, 1, 8, 9, 4, 5, 12, 13, /**/ 2, 3, 10, 11, 6, 7, 14, 15>(even<8>(w)),
               permute<0, 1, 8, 9, 4, 5, 12, 13, /**/ 2, 3, 10, 11, 6, 7, 14, 15>(odd<8>(w)));

    w = permutegroups<(4), 0, 4, 1, 5, 2, 6, 3, 7>(w); // avx: vperm2f128 & vinsertf128, sse: no-op
    return w;
#endif
}

inline vec<f32, 32> bitreverse_2(const vec<f32, 32>& x)
{
    return x.shuffle(csizes<0, 1, 16, 17, 8, 9, 24, 25, 4, 5, 20, 21, 12, 13, 28, 29, 2, 3, 18, 19, 10, 11,
                            26, 27, 6, 7, 22, 23, 14, 15, 30, 31>);
}

template <>
template <>
inline vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 32, 33, 16, 17, 48, 49, 8, 9, 40, 41, 24, 25, 56, 57, 4, 5, 36, 37, 20, 21, 52, 53, 12, 13,
             44, 45, 28, 29, 60, 61, 2, 3, 34, 35, 18, 19, 50, 51, 10, 11, 42, 43, 26, 27, 58, 59, 6, 7, 38,
             39, 22, 23, 54, 55, 14, 15, 46, 47, 30, 31, 62, 63>) const noexcept
{
    return permutegroups<(8), 0, 4, 1, 5, 2, 6, 3, 7>(
        concat(bitreverse_2(even<8>(*this)), bitreverse_2(odd<8>(*this))));
}

template <>
template <>
inline vec<f32, 16> vec<f32, 16>::shuffle(
    csizes_t<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15>) const noexcept
{
    const vec<f32, 16> xx = permutegroups<(4), 0, 2, 1, 3>(*this);

    return concat(low(xx).shuffle(high(xx), csizes<0, 2, 8 + 0, 8 + 2, 4, 6, 8 + 4, 8 + 6>),
                  low(xx).shuffle(high(xx), csizes<1, 3, 8 + 1, 8 + 3, 5, 7, 8 + 5, 8 + 7>));
}

template <>
template <>
inline vec<f32, 16> vec<f32, 16>::shuffle(
    csizes_t<0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13, 6, 14, 7, 15>) const noexcept
{
    const vec<f32, 16> xx =
        concat(low(*this).shuffle(high(*this), csizes<0, 8 + 0, 1, 8 + 1, 4, 8 + 4, 5, 8 + 5>),
               low(*this).shuffle(high(*this), csizes<2, 8 + 2, 3, 8 + 3, 6, 8 + 6, 7, 8 + 7>));

    return permutegroups<(4), 0, 2, 1, 3>(xx);
}

template <>
template <>
inline vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23, 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13,
             29, 14, 30, 15, 31>) const noexcept
{
    const vec<f32, 32> xx = permutegroups<(8), 0, 2, 1, 3>(*this);

    return concat(interleavehalves(low(xx)), interleavehalves(high(xx)));
}

#ifdef __clang__
template <>
template <>
inline vec<f64, 64> vec<f64, 64>::shuffle(csizes_t<0, 8, 16, 24, 32, 40, 48, 56, //
                                                   1, 9, 17, 25, 33, 41, 49, 57, //
                                                   2, 10, 18, 26, 34, 42, 50, 58, //
                                                   3, 11, 19, 27, 35, 43, 51, 59, //
                                                   4, 12, 20, 28, 36, 44, 52, 60, //
                                                   5, 13, 21, 29, 37, 45, 53, 61, //
                                                   6, 14, 22, 30, 38, 46, 54, 62, //
                                                   7, 15, 23, 31, 39, 47, 55, 63>) const noexcept
{
    // std::atomic_thread_fence(std::memory_order_seq_cst);
    // ── Step 0: extract 8 rows of 8 from the flat 64-vector ─────────────
    auto r0 = __builtin_shufflevector(v, v, 0, 1, 2, 3, 4, 5, 6, 7);
    auto r1 = __builtin_shufflevector(v, v, 8, 9, 10, 11, 12, 13, 14, 15);
    auto r2 = __builtin_shufflevector(v, v, 16, 17, 18, 19, 20, 21, 22, 23);
    auto r3 = __builtin_shufflevector(v, v, 24, 25, 26, 27, 28, 29, 30, 31);
    auto r4 = __builtin_shufflevector(v, v, 32, 33, 34, 35, 36, 37, 38, 39);
    auto r5 = __builtin_shufflevector(v, v, 40, 41, 42, 43, 44, 45, 46, 47);
    auto r6 = __builtin_shufflevector(v, v, 48, 49, 50, 51, 52, 53, 54, 55);
    auto r7 = __builtin_shufflevector(v, v, 56, 57, 58, 59, 60, 61, 62, 63);


    // ── Pass 1: zip adjacent row-pairs at stride 1 ───────────────────────
    // t0 = [r0[0],r1[0],r0[1],r1[1],r0[2],r1[2],r0[3],r1[3]]
    // t1 = [r0[4],r1[4],r0[5],r1[5],r0[6],r1[6],r0[7],r1[7]]  …etc.
    auto t0 = __builtin_shufflevector(r0, r1, 0, 8, 1, 9, 2, 10, 3, 11);
    auto t1 = __builtin_shufflevector(r0, r1, 4, 12, 5, 13, 6, 14, 7, 15);
    auto t2 = __builtin_shufflevector(r2, r3, 0, 8, 1, 9, 2, 10, 3, 11);
    auto t3 = __builtin_shufflevector(r2, r3, 4, 12, 5, 13, 6, 14, 7, 15);
    auto t4 = __builtin_shufflevector(r4, r5, 0, 8, 1, 9, 2, 10, 3, 11);
    auto t5 = __builtin_shufflevector(r4, r5, 4, 12, 5, 13, 6, 14, 7, 15);
    auto t6 = __builtin_shufflevector(r6, r7, 0, 8, 1, 9, 2, 10, 3, 11);
    auto t7 = __builtin_shufflevector(r6, r7, 4, 12, 5, 13, 6, 14, 7, 15);

    // ── Pass 2: merge quad groups at stride 2 ────────────────────────────
    // s0 = [r0[0],r1[0],r2[0],r3[0], r0[1],r1[1],r2[1],r3[1]]  …etc.
    auto s0 = __builtin_shufflevector(t0, t2, 0, 1, 8, 9, 2, 3, 10, 11);
    auto s1 = __builtin_shufflevector(t0, t2, 4, 5, 12, 13, 6, 7, 14, 15);
    auto s2 = __builtin_shufflevector(t1, t3, 0, 1, 8, 9, 2, 3, 10, 11);
    auto s3 = __builtin_shufflevector(t1, t3, 4, 5, 12, 13, 6, 7, 14, 15);
    auto s4 = __builtin_shufflevector(t4, t6, 0, 1, 8, 9, 2, 3, 10, 11);
    auto s5 = __builtin_shufflevector(t4, t6, 4, 5, 12, 13, 6, 7, 14, 15);
    auto s6 = __builtin_shufflevector(t5, t7, 0, 1, 8, 9, 2, 3, 10, 11);
    auto s7 = __builtin_shufflevector(t5, t7, 4, 5, 12, 13, 6, 7, 14, 15);

    // ── Pass 3: merge halves at stride 4 → each u_k is output column k ──
    // u0 = [r0[0],r1[0],r2[0],r3[0],r4[0],r5[0],r6[0],r7[0]]  ✓ col 0
    // u1 = [r0[1],r1[1],r2[1],r3[1],r4[1],r5[1],r6[1],r7[1]]  ✓ col 1 …
    auto u0 = __builtin_shufflevector(s0, s4, 0, 1, 2, 3, 8, 9, 10, 11);
    auto u1 = __builtin_shufflevector(s0, s4, 4, 5, 6, 7, 12, 13, 14, 15);
    auto u2 = __builtin_shufflevector(s1, s5, 0, 1, 2, 3, 8, 9, 10, 11);
    auto u3 = __builtin_shufflevector(s1, s5, 4, 5, 6, 7, 12, 13, 14, 15);
    auto u4 = __builtin_shufflevector(s2, s6, 0, 1, 2, 3, 8, 9, 10, 11);
    auto u5 = __builtin_shufflevector(s2, s6, 4, 5, 6, 7, 12, 13, 14, 15);
    auto u6 = __builtin_shufflevector(s3, s7, 0, 1, 2, 3, 8, 9, 10, 11);
    auto u7 = __builtin_shufflevector(s3, s7, 4, 5, 6, 7, 12, 13, 14, 15);

    // ── Step 4: reassemble into vec64d (trivial concatenations) ─────────
    auto w01   = __builtin_shufflevector(u0, u1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    auto w23   = __builtin_shufflevector(u2, u3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    auto w45   = __builtin_shufflevector(u4, u5, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    auto w67   = __builtin_shufflevector(u6, u7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    auto w0123 = __builtin_shufflevector(w01, w23, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                                         17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
    auto w4567 = __builtin_shufflevector(w45, w67, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
                                         17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31);
    return __builtin_shufflevector(w0123, w4567, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                   18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
                                   37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
                                   56, 57, 58, 59, 60, 61, 62, 63);
}
#endif

template <>
template <>
inline vec<f32, 128> vec<f32, 128>::shuffle(
    csizes_t<0, 1, 16, 17, 32, 33, 48, 49, 64, 65, 80, 81, 96, 97, 112, 113, 2, 3, 18, 19, 34, 35, 50, 51, 66,
             67, 82, 83, 98, 99, 114, 115, 4, 5, 20, 21, 36, 37, 52, 53, 68, 69, 84, 85, 100, 101, 116, 117,
             6, 7, 22, 23, 38, 39, 54, 55, 70, 71, 86, 87, 102, 103, 118, 119, 8, 9, 24, 25, 40, 41, 56, 57,
             72, 73, 88, 89, 104, 105, 120, 121, 10, 11, 26, 27, 42, 43, 58, 59, 74, 75, 90, 91, 106, 107,
             122, 123, 12, 13, 28, 29, 44, 45, 60, 61, 76, 77, 92, 93, 108, 109, 124, 125, 14, 15, 30, 31, 46,
             47, 62, 63, 78, 79, 94, 95, 110, 111, 126, 127>) const noexcept
{
    return bitcast<f32>(bitcast<f64>(*this).shuffle(csizes_t<0, 8, 16, 24, 32, 40, 48, 56, //
                                                             1, 9, 17, 25, 33, 41, 49, 57, //
                                                             2, 10, 18, 26, 34, 42, 50, 58, //
                                                             3, 11, 19, 27, 35, 43, 51, 59, //
                                                             4, 12, 20, 28, 36, 44, 52, 60, //
                                                             5, 13, 21, 29, 37, 45, 53, 61, //
                                                             6, 14, 22, 30, 38, 46, 54, 62, //
                                                             7, 15, 23, 31, 39, 47, 55, 63>()));
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
#endif
