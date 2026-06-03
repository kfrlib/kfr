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

#if defined __clang__ && defined __AVX__
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

#ifdef __AVX__

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 4, 16, 20, 32, 36, 48, 52, 8, 12, 24, 28, 40, 44, 56, 60, 1, 5, 17, 21, 33, 37, 49, 53, 9, 13,
             25, 29, 41, 45, 57, 61, 2, 6, 18, 22, 34, 38, 50, 54, 10, 14, 26, 30, 42, 46, 58, 62, 3, 7, 19,
             23, 35, 39, 51, 55, 11, 15, 27, 31, 43, 47, 59, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: unpack within even/odd pairs ────────────────────────────────
    // unpacklo(a,b) = { a[0],b[0],a[1],b[1] | a[4],b[4],a[5],b[5] }
    // unpackhi(a,b) = { a[2],b[2],a[3],b[3] | a[6],b[6],a[7],b[7] }

    //  Even group  (feeds output rows 0,2,4,6)
    __m256 t0 = _mm256_unpacklo_ps(v0, v2); // a0,a16,a1,a17 | a4,a20,a5,a21
    __m256 t1 = _mm256_unpackhi_ps(v0, v2); // a2,a18,a3,a19 | a6,a22,a7,a23
    __m256 t2 = _mm256_unpacklo_ps(v4, v6); // a32,a48,a33,a49| a36,a52,a37,a53
    __m256 t3 = _mm256_unpackhi_ps(v4, v6); // a34,a50,a35,a51| a38,a54,a39,a55
    __m256 t4 = _mm256_unpacklo_ps(v1, v3); // a8,a24,a9,a25  | a12,a28,a13,a29
    __m256 t5 = _mm256_unpackhi_ps(v1, v3); // a10,a26,a11,a27| a14,a30,a15,a31
    __m256 t6 = _mm256_unpacklo_ps(v5, v7); // a40,a56,a41,a57| a44,a60,a45,a61
    __m256 t7 = _mm256_unpackhi_ps(v5, v7); // a42,a58,a43,a59| a46,a62,a47,a63

    // ── Stage 2: permute2f128 — bring matching lanes together ────────────────
    // 0x20 → { src1_lo | src2_lo },   0x31 → { src1_hi | src2_hi }

    __m256 p0 = _mm256_permute2f128_ps(t0, t2, 0x20); // a0,a16,a1,a17  | a32,a48,a33,a49
    __m256 p1 = _mm256_permute2f128_ps(t0, t2, 0x31); // a4,a20,a5,a21  | a36,a52,a37,a53
    __m256 p2 = _mm256_permute2f128_ps(t1, t3, 0x20); // a2,a18,a3,a19  | a34,a50,a35,a51
    __m256 p3 = _mm256_permute2f128_ps(t1, t3, 0x31); // a6,a22,a7,a23  | a38,a54,a39,a55
    __m256 p4 = _mm256_permute2f128_ps(t4, t6, 0x20); // a8,a24,a9,a25  | a40,a56,a41,a57
    __m256 p5 = _mm256_permute2f128_ps(t4, t6, 0x31); // a12,a28,a13,a29| a44,a60,a45,a61
    __m256 p6 = _mm256_permute2f128_ps(t5, t7, 0x20); // a10,a26,a11,a27| a42,a58,a43,a59
    __m256 p7 = _mm256_permute2f128_ps(t5, t7, 0x31); // a14,a30,a15,a31| a46,a62,a47,a63

    // ── Stage 3: final unpack → output registers ─────────────────────────────
    __m256 out0 = _mm256_unpacklo_ps(p0, p1); // a0, a4, a16,a20 | a32,a36,a48,a52
    __m256 out2 = _mm256_unpackhi_ps(p0, p1); // a1, a5, a17,a21 | a33,a37,a49,a53
    __m256 out4 = _mm256_unpacklo_ps(p2, p3); // a2, a6, a18,a22 | a34,a38,a50,a54
    __m256 out6 = _mm256_unpackhi_ps(p2, p3); // a3, a7, a19,a23 | a35,a39,a51,a55
    __m256 out1 = _mm256_unpacklo_ps(p4, p5); // a8, a12,a24,a28 | a40,a44,a56,a60
    __m256 out3 = _mm256_unpackhi_ps(p4, p5); // a9, a13,a25,a29 | a41,a45,a57,a61
    __m256 out5 = _mm256_unpacklo_ps(p6, p7); // a10,a14,a26,a30 | a42,a46,a58,a62
    __m256 out7 = _mm256_unpackhi_ps(p6, p7); // a11,a15,a27,a31 | a43,a47,a59,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 16, 32, 48, 1, 17, 33, 49, 8, 24, 40, 56, 9, 25, 41, 57, 2, 18, 34, 50, 3, 19, 35, 51, 10, 26,
             42, 58, 11, 27, 43, 59, 4, 20, 36, 52, 5, 21, 37, 53, 12, 28, 44, 60, 13, 29, 45, 61, 6, 22, 38,
             54, 7, 23, 39, 55, 14, 30, 46, 62, 15, 31, 47, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: unpack across stride-4 register pairs ───────────────────────
    __m256 t0 = _mm256_unpacklo_ps(v0, v4); // a0,a32,a1,a33   | a4,a36,a5,a37
    __m256 t1 = _mm256_unpacklo_ps(v2, v6); // a16,a48,a17,a49  | a20,a52,a21,a53
    __m256 t2 = _mm256_unpackhi_ps(v0, v4); // a2,a34,a3,a35    | a6,a38,a7,a39
    __m256 t3 = _mm256_unpackhi_ps(v2, v6); // a18,a50,a19,a51  | a22,a54,a23,a55
    __m256 t4 = _mm256_unpacklo_ps(v1, v5); // a8,a40,a9,a41    | a12,a44,a13,a45
    __m256 t5 = _mm256_unpacklo_ps(v3, v7); // a24,a56,a25,a57  | a28,a60,a29,a61
    __m256 t6 = _mm256_unpackhi_ps(v1, v5); // a10,a42,a11,a43  | a14,a46,a15,a47
    __m256 t7 = _mm256_unpackhi_ps(v3, v7); // a26,a58,a27,a59  | a30,a62,a31,a63

    // ── Stage 2: unpack again to group quads ─────────────────────────────────
    __m256 p0 = _mm256_unpacklo_ps(t0, t1); // a0,a16,a32,a48   | a4,a20,a36,a52
    __m256 p1 = _mm256_unpackhi_ps(t0, t1); // a1,a17,a33,a49   | a5,a21,a37,a53
    __m256 p2 = _mm256_unpacklo_ps(t2, t3); // a2,a18,a34,a50   | a6,a22,a38,a54
    __m256 p3 = _mm256_unpackhi_ps(t2, t3); // a3,a19,a35,a51   | a7,a23,a39,a55
    __m256 p4 = _mm256_unpacklo_ps(t4, t5); // a8,a24,a40,a56   | a12,a28,a44,a60
    __m256 p5 = _mm256_unpackhi_ps(t4, t5); // a9,a25,a41,a57   | a13,a29,a45,a61
    __m256 p6 = _mm256_unpacklo_ps(t6, t7); // a10,a26,a42,a58  | a14,a30,a46,a62
    __m256 p7 = _mm256_unpackhi_ps(t6, t7); // a11,a27,a43,a59  | a15,a31,a47,a63

    // ── Stage 3: permute2f128 — combine matching lanes ───────────────────────
    __m256 out0 = _mm256_permute2f128_ps(p0, p1, 0x20); // a0,a16,a32,a48  | a1,a17,a33,a49
    __m256 out4 = _mm256_permute2f128_ps(p0, p1, 0x31); // a4,a20,a36,a52  | a5,a21,a37,a53
    __m256 out2 = _mm256_permute2f128_ps(p2, p3, 0x20); // a2,a18,a34,a50  | a3,a19,a35,a51
    __m256 out6 = _mm256_permute2f128_ps(p2, p3, 0x31); // a6,a22,a38,a54  | a7,a23,a39,a55
    __m256 out1 = _mm256_permute2f128_ps(p4, p5, 0x20); // a8,a24,a40,a56  | a9,a25,a41,a57
    __m256 out5 = _mm256_permute2f128_ps(p4, p5, 0x31); // a12,a28,a44,a60 | a13,a29,a45,a61
    __m256 out3 = _mm256_permute2f128_ps(p6, p7, 0x20); // a10,a26,a42,a58 | a11,a27,a43,a59
    __m256 out7 = _mm256_permute2f128_ps(p6, p7, 0x31); // a14,a30,a46,a62 | a15,a31,a47,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 8, 16, 24, 32, 40, 48, 56, 1, 9, 17, 25, 33, 41, 49, 57, 2, 10, 18, 26, 34, 42, 50, 58, 3, 11,
             19, 27, 35, 43, 51, 59, 4, 12, 20, 28, 36, 44, 52, 60, 5, 13, 21, 29, 37, 45, 53, 61, 6, 14, 22,
             30, 38, 46, 54, 62, 7, 15, 23, 31, 39, 47, 55, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave adjacent register pairs ──────────────────────────
    __m256 t0 = _mm256_unpacklo_ps(v0, v1); // a0,a8,a1,a9     | a4,a12,a5,a13
    __m256 t1 = _mm256_unpackhi_ps(v0, v1); // a2,a10,a3,a11   | a6,a14,a7,a15
    __m256 t2 = _mm256_unpacklo_ps(v2, v3); // a16,a24,a17,a25 | a20,a28,a21,a29
    __m256 t3 = _mm256_unpackhi_ps(v2, v3); // a18,a26,a19,a27 | a22,a30,a23,a31
    __m256 t4 = _mm256_unpacklo_ps(v4, v5); // a32,a40,a33,a41 | a36,a44,a37,a45
    __m256 t5 = _mm256_unpackhi_ps(v4, v5); // a34,a42,a35,a43 | a38,a46,a39,a47
    __m256 t6 = _mm256_unpacklo_ps(v6, v7); // a48,a56,a49,a57 | a52,a60,a53,a61
    __m256 t7 = _mm256_unpackhi_ps(v6, v7); // a50,a58,a51,a59 | a54,a62,a55,a63

    // ── Stage 2: shuffle to group quads ──────────────────────────────────────
    __m256 p0 = _mm256_shuffle_ps(t0, t2, 0x44); // a0,a8,a16,a24   | a4,a12,a20,a28
    __m256 p1 = _mm256_shuffle_ps(t0, t2, 0xEE); // a1,a9,a17,a25   | a5,a13,a21,a29
    __m256 p2 = _mm256_shuffle_ps(t1, t3, 0x44); // a2,a10,a18,a26  | a6,a14,a22,a30
    __m256 p3 = _mm256_shuffle_ps(t1, t3, 0xEE); // a3,a11,a19,a27  | a7,a15,a23,a31
    __m256 p4 = _mm256_shuffle_ps(t4, t6, 0x44); // a32,a40,a48,a56 | a36,a44,a52,a60
    __m256 p5 = _mm256_shuffle_ps(t4, t6, 0xEE); // a33,a41,a49,a57 | a37,a45,a53,a61
    __m256 p6 = _mm256_shuffle_ps(t5, t7, 0x44); // a34,a42,a50,a58 | a38,a46,a54,a62
    __m256 p7 = _mm256_shuffle_ps(t5, t7, 0xEE); // a35,a43,a51,a59 | a39,a47,a55,a63

    // ── Stage 3: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(p0, p4, 0x20); // a0,a8,a16,a24   | a32,a40,a48,a56
    __m256 out4 = _mm256_permute2f128_ps(p0, p4, 0x31); // a4,a12,a20,a28  | a36,a44,a52,a60
    __m256 out1 = _mm256_permute2f128_ps(p1, p5, 0x20); // a1,a9,a17,a25   | a33,a41,a49,a57
    __m256 out5 = _mm256_permute2f128_ps(p1, p5, 0x31); // a5,a13,a21,a29  | a37,a45,a53,a61
    __m256 out2 = _mm256_permute2f128_ps(p2, p6, 0x20); // a2,a10,a18,a26  | a34,a42,a50,a58
    __m256 out6 = _mm256_permute2f128_ps(p2, p6, 0x31); // a6,a14,a22,a30  | a38,a46,a54,a62
    __m256 out3 = _mm256_permute2f128_ps(p3, p7, 0x20); // a3,a11,a19,a27  | a35,a43,a51,a59
    __m256 out7 = _mm256_permute2f128_ps(p3, p7, 0x31); // a7,a15,a23,a31  | a39,a47,a55,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 8, 9, 16, 17, 24, 25, 32, 33, 40, 41, 48, 49, 56, 57, 2, 3, 10, 11, 18, 19, 26, 27, 34, 35,
             42, 43, 50, 51, 58, 59, 4, 5, 12, 13, 20, 21, 28, 29, 36, 37, 44, 45, 52, 53, 60, 61, 6, 7, 14,
             15, 22, 23, 30, 31, 38, 39, 46, 47, 54, 55, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: shuffle_ps to gather stride-8 pairs within each 128-bit lane ──
    // 0x44 = select [0,1] from src1 then [0,1] from src2 (per 128-bit lane)
    // 0xEE = select [2,3] from src1 then [2,3] from src2 (per 128-bit lane)
    __m256 t0 = _mm256_shuffle_ps(v0, v1, 0x44); // a0,a1,a8,a9     | a4,a5,a12,a13
    __m256 t1 = _mm256_shuffle_ps(v0, v1, 0xEE); // a2,a3,a10,a11   | a6,a7,a14,a15
    __m256 t2 = _mm256_shuffle_ps(v2, v3, 0x44); // a16,a17,a24,a25 | a20,a21,a28,a29
    __m256 t3 = _mm256_shuffle_ps(v2, v3, 0xEE); // a18,a19,a26,a27 | a22,a23,a30,a31
    __m256 t4 = _mm256_shuffle_ps(v4, v5, 0x44); // a32,a33,a40,a41 | a36,a37,a44,a45
    __m256 t5 = _mm256_shuffle_ps(v4, v5, 0xEE); // a34,a35,a42,a43 | a38,a39,a46,a47
    __m256 t6 = _mm256_shuffle_ps(v6, v7, 0x44); // a48,a49,a56,a57 | a52,a53,a60,a61
    __m256 t7 = _mm256_shuffle_ps(v6, v7, 0xEE); // a50,a51,a58,a59 | a54,a55,a62,a63

    // ── Stage 2: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(t0, t2, 0x20); // a0,a1,a8,a9     | a16,a17,a24,a25
    __m256 out1 = _mm256_permute2f128_ps(t4, t6, 0x20); // a32,a33,a40,a41 | a48,a49,a56,a57
    __m256 out2 = _mm256_permute2f128_ps(t1, t3, 0x20); // a2,a3,a10,a11   | a18,a19,a26,a27
    __m256 out3 = _mm256_permute2f128_ps(t5, t7, 0x20); // a34,a35,a42,a43 | a50,a51,a58,a59
    __m256 out4 = _mm256_permute2f128_ps(t0, t2, 0x31); // a4,a5,a12,a13   | a20,a21,a28,a29
    __m256 out5 = _mm256_permute2f128_ps(t4, t6, 0x31); // a36,a37,a44,a45 | a52,a53,a60,a61
    __m256 out6 = _mm256_permute2f128_ps(t1, t3, 0x31); // a6,a7,a14,a15   | a22,a23,a30,a31
    __m256 out7 = _mm256_permute2f128_ps(t5, t7, 0x31); // a38,a39,a46,a47 | a54,a55,a62,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 32, 16, 48, 2, 34, 18, 50, 1, 33, 17, 49, 3, 35, 19, 51, 4, 36, 20, 52, 6, 38, 22, 54, 5, 37,
             21, 53, 7, 39, 23, 55, 8, 40, 24, 56, 10, 42, 26, 58, 9, 41, 25, 57, 11, 43, 27, 59, 12, 44, 28,
             60, 14, 46, 30, 62, 13, 45, 29, 61, 15, 47, 31, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;
    
    // ── Stage 1: shuffle_ps to gather even/odd elements across stride-32 pairs ──
    // 0x88 = select [0,2] from src1 then [0,2] from src2 (per 128-bit lane)
    // 0xDD = select [1,3] from src1 then [1,3] from src2 (per 128-bit lane)
    __m256 t0 = _mm256_shuffle_ps(v0, v4, 0x88); // a0,a2,a32,a34   | a4,a6,a36,a38
    __m256 t1 = _mm256_shuffle_ps(v0, v4, 0xDD); // a1,a3,a33,a35   | a5,a7,a37,a39
    __m256 t2 = _mm256_shuffle_ps(v2, v6, 0x88); // a16,a18,a48,a50 | a20,a22,a52,a54
    __m256 t3 = _mm256_shuffle_ps(v2, v6, 0xDD); // a17,a19,a49,a51 | a21,a23,a53,a55
    __m256 t4 = _mm256_shuffle_ps(v1, v5, 0x88); // a8,a10,a40,a42  | a12,a14,a44,a46
    __m256 t5 = _mm256_shuffle_ps(v1, v5, 0xDD); // a9,a11,a41,a43  | a13,a15,a45,a47
    __m256 t6 = _mm256_shuffle_ps(v3, v7, 0x88); // a24,a26,a56,a58 | a28,a30,a60,a62
    __m256 t7 = _mm256_shuffle_ps(v3, v7, 0xDD); // a25,a27,a57,a59 | a29,a31,a61,a63

    // ── Stage 2: shuffle_ps to interleave stride-16 quadruples ───────────────
    __m256 s0 = _mm256_shuffle_ps(t0, t2, 0x88); // a0,a32,a16,a48  | a4,a36,a20,a52
    __m256 s1 = _mm256_shuffle_ps(t0, t2, 0xDD); // a2,a34,a18,a50  | a6,a38,a22,a54
    __m256 s2 = _mm256_shuffle_ps(t1, t3, 0x88); // a1,a33,a17,a49  | a5,a37,a21,a53
    __m256 s3 = _mm256_shuffle_ps(t1, t3, 0xDD); // a3,a35,a19,a51  | a7,a39,a23,a55
    __m256 s4 = _mm256_shuffle_ps(t4, t6, 0x88); // a8,a40,a24,a56  | a12,a44,a28,a60
    __m256 s5 = _mm256_shuffle_ps(t4, t6, 0xDD); // a10,a42,a26,a58 | a14,a46,a30,a62
    __m256 s6 = _mm256_shuffle_ps(t5, t7, 0x88); // a9,a41,a25,a57  | a13,a45,a29,a61
    __m256 s7 = _mm256_shuffle_ps(t5, t7, 0xDD); // a11,a43,a27,a59 | a15,a47,a31,a63

    // ── Stage 3: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(s0, s1, 0x20); // a0,a32,a16,a48  | a2,a34,a18,a50
    __m256 out1 = _mm256_permute2f128_ps(s2, s3, 0x20); // a1,a33,a17,a49  | a3,a35,a19,a51
    __m256 out2 = _mm256_permute2f128_ps(s0, s1, 0x31); // a4,a36,a20,a52  | a6,a38,a22,a54
    __m256 out3 = _mm256_permute2f128_ps(s2, s3, 0x31); // a5,a37,a21,a53  | a7,a39,a23,a55
    __m256 out4 = _mm256_permute2f128_ps(s4, s5, 0x20); // a8,a40,a24,a56  | a10,a42,a26,a58
    __m256 out5 = _mm256_permute2f128_ps(s6, s7, 0x20); // a9,a41,a25,a57  | a11,a43,a27,a59
    __m256 out6 = _mm256_permute2f128_ps(s4, s5, 0x31); // a12,a44,a28,a60 | a14,a46,a30,a62
    __m256 out7 = _mm256_permute2f128_ps(s6, s7, 0x31); // a13,a45,a29,a61 | a15,a47,a31,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 16, 17, 32, 33, 48, 49, 2, 3, 18, 19, 34, 35, 50, 51, 4, 5, 20, 21, 36, 37, 52, 53, 6, 7,
             22, 23, 38, 39, 54, 55, 8, 9, 24, 25, 40, 41, 56, 57, 10, 11, 26, 27, 42, 43, 58, 59, 12, 13, 28,
             29, 44, 45, 60, 61, 14, 15, 30, 31, 46, 47, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: shuffle_ps to gather stride-16 pairs within each 128-bit lane ──
    // 0x44 = select [0,1] from src1 then [0,1] from src2 (per 128-bit lane)
    // 0xEE = select [2,3] from src1 then [2,3] from src2 (per 128-bit lane)
    __m256 t0 = _mm256_shuffle_ps(v0, v2, 0x44); // a0,a1,a16,a17   | a4,a5,a20,a21
    __m256 t1 = _mm256_shuffle_ps(v0, v2, 0xEE); // a2,a3,a18,a19   | a6,a7,a22,a23
    __m256 t2 = _mm256_shuffle_ps(v4, v6, 0x44); // a32,a33,a48,a49 | a36,a37,a52,a53
    __m256 t3 = _mm256_shuffle_ps(v4, v6, 0xEE); // a34,a35,a50,a51 | a38,a39,a54,a55
    __m256 t4 = _mm256_shuffle_ps(v1, v3, 0x44); // a8,a9,a24,a25   | a12,a13,a28,a29
    __m256 t5 = _mm256_shuffle_ps(v1, v3, 0xEE); // a10,a11,a26,a27 | a14,a15,a30,a31
    __m256 t6 = _mm256_shuffle_ps(v5, v7, 0x44); // a40,a41,a56,a57 | a44,a45,a60,a61
    __m256 t7 = _mm256_shuffle_ps(v5, v7, 0xEE); // a42,a43,a58,a59 | a46,a47,a62,a63

    // ── Stage 2: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(t0, t2, 0x20); // a0,a1,a16,a17   | a32,a33,a48,a49
    __m256 out1 = _mm256_permute2f128_ps(t1, t3, 0x20); // a2,a3,a18,a19   | a34,a35,a50,a51
    __m256 out2 = _mm256_permute2f128_ps(t0, t2, 0x31); // a4,a5,a20,a21   | a36,a37,a52,a53
    __m256 out3 = _mm256_permute2f128_ps(t1, t3, 0x31); // a6,a7,a22,a23   | a38,a39,a54,a55
    __m256 out4 = _mm256_permute2f128_ps(t4, t6, 0x20); // a8,a9,a24,a25   | a40,a41,a56,a57
    __m256 out5 = _mm256_permute2f128_ps(t5, t7, 0x20); // a10,a11,a26,a27 | a42,a43,a58,a59
    __m256 out6 = _mm256_permute2f128_ps(t4, t6, 0x31); // a12,a13,a28,a29 | a44,a45,a60,a61
    __m256 out7 = _mm256_permute2f128_ps(t5, t7, 0x31); // a14,a15,a30,a31 | a46,a47,a62,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 16, 17, 4, 5, 20, 21, 8, 9, 24, 25, 12, 13, 28, 29, 32, 33, 48, 49, 36, 37, 52, 53, 40, 41,
             56, 57, 44, 45, 60, 61, 2, 3, 18, 19, 6, 7, 22, 23, 10, 11, 26, 27, 14, 15, 30, 31, 34, 35, 50,
             51, 38, 39, 54, 55, 42, 43, 58, 59, 46, 47, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // Single stage: _mm256_shuffle_ps pairs consecutive elements from stride-2 register pairs
    // 0x44 = select [0,1] from src1 then [0,1] from src2 (per 128-bit lane)
    // 0xEE = select [2,3] from src1 then [2,3] from src2 (per 128-bit lane)
    __m256 out0 = _mm256_shuffle_ps(v0, v2, 0x44); // a0,a1,a16,a17   | a4,a5,a20,a21
    __m256 out1 = _mm256_shuffle_ps(v1, v3, 0x44); // a8,a9,a24,a25   | a12,a13,a28,a29
    __m256 out2 = _mm256_shuffle_ps(v4, v6, 0x44); // a32,a33,a48,a49 | a36,a37,a52,a53
    __m256 out3 = _mm256_shuffle_ps(v5, v7, 0x44); // a40,a41,a56,a57 | a44,a45,a60,a61
    __m256 out4 = _mm256_shuffle_ps(v0, v2, 0xEE); // a2,a3,a18,a19   | a6,a7,a22,a23
    __m256 out5 = _mm256_shuffle_ps(v1, v3, 0xEE); // a10,a11,a26,a27 | a14,a15,a30,a31
    __m256 out6 = _mm256_shuffle_ps(v4, v6, 0xEE); // a34,a35,a50,a51 | a38,a39,a54,a55
    __m256 out7 = _mm256_shuffle_ps(v5, v7, 0xEE); // a42,a43,a58,a59 | a46,a47,a62,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 32, 33, 4, 5, 36, 37, 8, 9, 40, 41, 12, 13, 44, 45, 2, 3, 34, 35, 6, 7, 38, 39, 10, 11, 42,
             43, 14, 15, 46, 47, 16, 17, 48, 49, 20, 21, 52, 53, 24, 25, 56, 57, 28, 29, 60, 61, 18, 19, 50,
             51, 22, 23, 54, 55, 26, 27, 58, 59, 30, 31, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // Single stage: _mm256_shuffle_ps pairs consecutive elements from stride-4 register pairs
    // 0x44 = select [0,1] from src1 then [0,1] from src2 (per 128-bit lane)
    // 0xEE = select [2,3] from src1 then [2,3] from src2 (per 128-bit lane)
    __m256 out0 = _mm256_shuffle_ps(v0, v4, 0x44); // a0,a1,a32,a33   | a4,a5,a36,a37
    __m256 out1 = _mm256_shuffle_ps(v1, v5, 0x44); // a8,a9,a40,a41   | a12,a13,a44,a45
    __m256 out2 = _mm256_shuffle_ps(v0, v4, 0xEE); // a2,a3,a34,a35   | a6,a7,a38,a39
    __m256 out3 = _mm256_shuffle_ps(v1, v5, 0xEE); // a10,a11,a42,a43 | a14,a15,a46,a47
    __m256 out4 = _mm256_shuffle_ps(v2, v6, 0x44); // a16,a17,a48,a49 | a20,a21,a52,a53
    __m256 out5 = _mm256_shuffle_ps(v3, v7, 0x44); // a24,a25,a56,a57 | a28,a29,a60,a61
    __m256 out6 = _mm256_shuffle_ps(v2, v6, 0xEE); // a18,a19,a50,a51 | a22,a23,a54,a55
    __m256 out7 = _mm256_shuffle_ps(v3, v7, 0xEE); // a26,a27,a58,a59 | a30,a31,a62,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}
template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 2, 32, 34, 16, 18, 48, 50, 8, 10, 40, 42, 24, 26, 56, 58, 1, 3, 33, 35, 17, 19, 49, 51, 9, 11,
             41, 43, 25, 27, 57, 59, 4, 6, 36, 38, 20, 22, 52, 54, 12, 14, 44, 46, 28, 30, 60, 62, 5, 7, 37,
             39, 21, 23, 53, 55, 13, 15, 45, 47, 29, 31, 61, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: unpacklo/hi across stride-4 register pairs ──────────────────
    __m256 t0 = _mm256_unpacklo_ps(v0, v4); // a0,a32,a1,a33  | a4,a36,a5,a37
    __m256 t1 = _mm256_unpackhi_ps(v0, v4); // a2,a34,a3,a35  | a6,a38,a7,a39
    __m256 t2 = _mm256_unpacklo_ps(v2, v6); // a16,a48,a17,a49 | a20,a52,a21,a53
    __m256 t3 = _mm256_unpackhi_ps(v2, v6); // a18,a50,a19,a51 | a22,a54,a23,a55
    __m256 t4 = _mm256_unpacklo_ps(v1, v5); // a8,a40,a9,a41  | a12,a44,a13,a45
    __m256 t5 = _mm256_unpackhi_ps(v1, v5); // a10,a42,a11,a43 | a14,a46,a15,a47
    __m256 t6 = _mm256_unpacklo_ps(v3, v7); // a24,a56,a25,a57 | a28,a60,a29,a61
    __m256 t7 = _mm256_unpackhi_ps(v3, v7); // a26,a58,a27,a59 | a30,a62,a31,a63

    // ── Stage 2: unpacklo/hi again to interleave even/odd elements ───────────
    __m256 p0 = _mm256_unpacklo_ps(t0, t1); // a0,a2,a32,a34   | a4,a6,a36,a38
    __m256 p1 = _mm256_unpackhi_ps(t0, t1); // a1,a3,a33,a35   | a5,a7,a37,a39
    __m256 p2 = _mm256_unpacklo_ps(t2, t3); // a16,a18,a48,a50 | a20,a22,a52,a54
    __m256 p3 = _mm256_unpackhi_ps(t2, t3); // a17,a19,a49,a51 | a21,a23,a53,a55
    __m256 p4 = _mm256_unpacklo_ps(t4, t5); // a8,a10,a40,a42  | a12,a14,a44,a46
    __m256 p5 = _mm256_unpackhi_ps(t4, t5); // a9,a11,a41,a43  | a13,a15,a45,a47
    __m256 p6 = _mm256_unpacklo_ps(t6, t7); // a24,a26,a56,a58 | a28,a30,a60,a62
    __m256 p7 = _mm256_unpackhi_ps(t6, t7); // a25,a27,a57,a59 | a29,a31,a61,a63

    // ── Stage 3: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(p0, p2, 0x20); // a0,a2,a32,a34   | a16,a18,a48,a50
    __m256 out1 = _mm256_permute2f128_ps(p4, p6, 0x20); // a8,a10,a40,a42  | a24,a26,a56,a58
    __m256 out2 = _mm256_permute2f128_ps(p1, p3, 0x20); // a1,a3,a33,a35   | a17,a19,a49,a51
    __m256 out3 = _mm256_permute2f128_ps(p5, p7, 0x20); // a9,a11,a41,a43  | a25,a27,a57,a59
    __m256 out4 = _mm256_permute2f128_ps(p0, p2, 0x31); // a4,a6,a36,a38   | a20,a22,a52,a54
    __m256 out5 = _mm256_permute2f128_ps(p4, p6, 0x31); // a12,a14,a44,a46 | a28,a30,a60,a62
    __m256 out6 = _mm256_permute2f128_ps(p1, p3, 0x31); // a5,a7,a37,a39   | a21,a23,a53,a55
    __m256 out7 = _mm256_permute2f128_ps(p5, p7, 0x31); // a13,a15,a45,a47 | a29,a31,a61,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

#if 0
template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 2, 8, 10, 4, 6, 12, 14, 1, 3, 9, 11, 5, 7, 13, 15, 16, 18, 24, 26, 20, 22, 28, 30, 17, 19, 25,
             27, 21, 23, 29, 31, 32, 34, 40, 42, 36, 38, 44, 46, 33, 35, 41, 43, 37, 39, 45, 47, 48, 50, 56,
             58, 52, 54, 60, 62, 49, 51, 57, 59, 53, 55, 61, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // Single stage: select even (0x88) or odd (0xDD) elements from adjacent register pairs
    // 0x88 = pick [0,2] from src1 then [0,2] from src2 (per 128-bit lane)
    // 0xDD = pick [1,3] from src1 then [1,3] from src2 (per 128-bit lane)
    __m256 out0 = _mm256_shuffle_ps(v0, v1, 0x88); // a0,a2,a8,a10   | a4,a6,a12,a14
    __m256 out1 = _mm256_shuffle_ps(v0, v1, 0xDD); // a1,a3,a9,a11   | a5,a7,a13,a15
    __m256 out2 = _mm256_shuffle_ps(v2, v3, 0x88); // a16,a18,a24,a26 | a20,a22,a28,a30
    __m256 out3 = _mm256_shuffle_ps(v2, v3, 0xDD); // a17,a19,a25,a27 | a21,a23,a29,a31
    __m256 out4 = _mm256_shuffle_ps(v4, v5, 0x88); // a32,a34,a40,a42 | a36,a38,a44,a46
    __m256 out5 = _mm256_shuffle_ps(v4, v5, 0xDD); // a33,a35,a41,a43 | a37,a39,a45,a47
    __m256 out6 = _mm256_shuffle_ps(v6, v7, 0x88); // a48,a50,a56,a58 | a52,a54,a60,a62
    __m256 out7 = _mm256_shuffle_ps(v6, v7, 0xDD); // a49,a51,a57,a59 | a53,a55,a61,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}
#endif

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 8, 16, 24, 32, 40, 48, 56, 1, 9, 17, 25, 33, 41, 49, 57, 4, 12, 20, 28, 36, 44, 52, 60, 5, 13,
             21, 29, 37, 45, 53, 61, 2, 10, 18, 26, 34, 42, 50, 58, 3, 11, 19, 27, 35, 43, 51, 59, 6, 14, 22,
             30, 38, 46, 54, 62, 7, 15, 23, 31, 39, 47, 55, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave adjacent register pairs ──────────────────────────
    __m256 t0 = _mm256_unpacklo_ps(v0, v1); // a0,a8,a1,a9     | a4,a12,a5,a13
    __m256 t1 = _mm256_unpackhi_ps(v0, v1); // a2,a10,a3,a11   | a6,a14,a7,a15
    __m256 t2 = _mm256_unpacklo_ps(v2, v3); // a16,a24,a17,a25 | a20,a28,a21,a29
    __m256 t3 = _mm256_unpackhi_ps(v2, v3); // a18,a26,a19,a27 | a22,a30,a23,a31
    __m256 t4 = _mm256_unpacklo_ps(v4, v5); // a32,a40,a33,a41 | a36,a44,a37,a45
    __m256 t5 = _mm256_unpackhi_ps(v4, v5); // a34,a42,a35,a43 | a38,a46,a39,a47
    __m256 t6 = _mm256_unpacklo_ps(v6, v7); // a48,a56,a49,a57 | a52,a60,a53,a61
    __m256 t7 = _mm256_unpackhi_ps(v6, v7); // a50,a58,a51,a59 | a54,a62,a55,a63

    // ── Stage 2: shuffle to group quads ──────────────────────────────────────
    __m256 p0 = _mm256_shuffle_ps(t0, t2, 0x44); // a0,a8,a16,a24   | a4,a12,a20,a28
    __m256 p1 = _mm256_shuffle_ps(t0, t2, 0xEE); // a1,a9,a17,a25   | a5,a13,a21,a29
    __m256 p2 = _mm256_shuffle_ps(t1, t3, 0x44); // a2,a10,a18,a26  | a6,a14,a22,a30
    __m256 p3 = _mm256_shuffle_ps(t1, t3, 0xEE); // a3,a11,a19,a27  | a7,a15,a23,a31
    __m256 p4 = _mm256_shuffle_ps(t4, t6, 0x44); // a32,a40,a48,a56 | a36,a44,a52,a60
    __m256 p5 = _mm256_shuffle_ps(t4, t6, 0xEE); // a33,a41,a49,a57 | a37,a45,a53,a61
    __m256 p6 = _mm256_shuffle_ps(t5, t7, 0x44); // a34,a42,a50,a58 | a38,a46,a54,a62
    __m256 p7 = _mm256_shuffle_ps(t5, t7, 0xEE); // a35,a43,a51,a59 | a39,a47,a55,a63

    // ── Stage 3: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(p0, p4, 0x20); // a0,a8,a16,a24   | a32,a40,a48,a56
    __m256 out1 = _mm256_permute2f128_ps(p1, p5, 0x20); // a1,a9,a17,a25   | a33,a41,a49,a57
    __m256 out2 = _mm256_permute2f128_ps(p2, p6, 0x20); // a2,a10,a18,a26  | a34,a42,a50,a58
    __m256 out3 = _mm256_permute2f128_ps(p3, p7, 0x20); // a3,a11,a19,a27  | a35,a43,a51,a59
    __m256 out4 = _mm256_permute2f128_ps(p0, p4, 0x31); // a4,a12,a20,a28  | a36,a44,a52,a60
    __m256 out5 = _mm256_permute2f128_ps(p1, p5, 0x31); // a5,a13,a21,a29  | a37,a45,a53,a61
    __m256 out6 = _mm256_permute2f128_ps(p2, p6, 0x31); // a6,a14,a22,a30  | a38,a46,a54,a62
    __m256 out7 = _mm256_permute2f128_ps(p3, p7, 0x31); // a7,a15,a23,a31  | a39,a47,a55,a63

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15, 16, 20, 18, 22, 17, 21, 19, 23, 24, 28, 26,
             30, 25, 29, 27, 31, 32, 36, 34, 38, 33, 37, 35, 39, 40, 44, 42, 46, 41, 45, 43, 47, 48, 52, 50,
             54, 49, 53, 51, 55, 56, 60, 58, 62, 57, 61, 59, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // Apply (0,4,2,6,1,5,3,7) permutation independently to each __m256 (3 instructions):
    //   Step 1: vperm2f128 0x01 → swap 128-bit lanes:         [a4,a5,a6,a7 | a0,a1,a2,a3]
    //   Step 2: vshufps    0xB1 → swap adjacent pairs in each lane: [a5,a4,a7,a6 | a1,a0,a3,a2]
    //   Step 3: vblendps   0x5A → pick positions 1,3,4,6 from shuffled, rest from original
    //                             → [a0,a4,a2,a6 | a1,a5,a3,a7]
    auto do_perm = [](const __m256 v) -> __m256
    {
        __m256 swapped  = _mm256_permute2f128_ps(v, v, 0x01); // swap 128-bit lanes
        __m256 shuffled = _mm256_shuffle_ps(swapped, swapped, 0xB1); // swap adjacent pairs within lanes
        return _mm256_blend_ps(v, shuffled, 0x5A); // blend → [a0,a4,a2,a6 | a1,a5,a3,a7]
    };

    return concat(concat( //
                      concat(vec<f32, 8>(do_perm(v0)), vec<f32, 8>(do_perm(v1))), //
                      concat(vec<f32, 8>(do_perm(v2)), vec<f32, 8>(do_perm(v3))) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(do_perm(v4)), vec<f32, 8>(do_perm(v5))), //
                      concat(vec<f32, 8>(do_perm(v6)), vec<f32, 8>(do_perm(v7)))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 8, 32, 40, 1, 9, 33, 41, 2, 10, 34, 42, 3, 11, 35, 43, 4, 12, 36, 44, 5, 13, 37, 45, 6, 14,
             38, 46, 7, 15, 39, 47, 16, 24, 48, 56, 17, 25, 49, 57, 18, 26, 50, 58, 19, 27, 51, 59, 20, 28,
             52, 60, 21, 29, 53, 61, 22, 30, 54, 62, 23, 31, 55, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave adjacent pairs within {v0,v1}, {v4,v5}, {v2,v3}, {v6,v7} ──
    __m256 t0 = _mm256_unpacklo_ps(v0, v1); // v0[0],v1[0],v0[1],v1[1] | v0[4],v1[4],v0[5],v1[5]
    __m256 t1 = _mm256_unpackhi_ps(v0, v1); // v0[2],v1[2],v0[3],v1[3] | v0[6],v1[6],v0[7],v1[7]
    __m256 t4 = _mm256_unpacklo_ps(v4, v5); // v4[0],v5[0],v4[1],v5[1] | v4[4],v5[4],v4[5],v5[5]
    __m256 t5 = _mm256_unpackhi_ps(v4, v5); // v4[2],v5[2],v4[3],v5[3] | v4[6],v5[6],v4[7],v5[7]
    __m256 t2 = _mm256_unpacklo_ps(v2, v3); // v2[0],v3[0],v2[1],v3[1] | v2[4],v3[4],v2[5],v3[5]
    __m256 t3 = _mm256_unpackhi_ps(v2, v3); // v2[2],v3[2],v2[3],v3[3] | v2[6],v3[6],v2[7],v3[7]
    __m256 t6 = _mm256_unpacklo_ps(v6, v7); // v6[0],v7[0],v6[1],v7[1] | v6[4],v7[4],v6[5],v7[5]
    __m256 t7 = _mm256_unpackhi_ps(v6, v7); // v6[2],v7[2],v6[3],v7[3] | v6[6],v7[6],v6[7],v7[7]

    // ── Stage 2: shuffle to form quads {v0,v1,v4,v5} and {v2,v3,v6,v7} ────────
    __m256 p0 = _mm256_shuffle_ps(t0, t4, 0x44); // v0[0],v1[0],v4[0],v5[0] | v0[4],v1[4],v4[4],v5[4]
    __m256 p1 = _mm256_shuffle_ps(t0, t4, 0xEE); // v0[1],v1[1],v4[1],v5[1] | v0[5],v1[5],v4[5],v5[5]
    __m256 p4 = _mm256_shuffle_ps(t1, t5, 0x44); // v0[2],v1[2],v4[2],v5[2] | v0[6],v1[6],v4[6],v5[6]
    __m256 p5 = _mm256_shuffle_ps(t1, t5, 0xEE); // v0[3],v1[3],v4[3],v5[3] | v0[7],v1[7],v4[7],v5[7]
    __m256 p2 = _mm256_shuffle_ps(t2, t6, 0x44); // v2[0],v3[0],v6[0],v7[0] | v2[4],v3[4],v6[4],v7[4]
    __m256 p3 = _mm256_shuffle_ps(t2, t6, 0xEE); // v2[1],v3[1],v6[1],v7[1] | v2[5],v3[5],v6[5],v7[5]
    __m256 p6 = _mm256_shuffle_ps(t3, t7, 0x44); // v2[2],v3[2],v6[2],v7[2] | v2[6],v3[6],v6[6],v7[6]
    __m256 p7 = _mm256_shuffle_ps(t3, t7, 0xEE); // v2[3],v3[3],v6[3],v7[3] | v2[7],v3[7],v6[7],v7[7]

    // ── Stage 3: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(p0, p1, 0x20); // v0[0],v1[0],v4[0],v5[0] | v0[1],v1[1],v4[1],v5[1]
    __m256 out1 = _mm256_permute2f128_ps(p4, p5, 0x20); // v0[2],v1[2],v4[2],v5[2] | v0[3],v1[3],v4[3],v5[3]
    __m256 out2 = _mm256_permute2f128_ps(p0, p1, 0x31); // v0[4],v1[4],v4[4],v5[4] | v0[5],v1[5],v4[5],v5[5]
    __m256 out3 = _mm256_permute2f128_ps(p4, p5, 0x31); // v0[6],v1[6],v4[6],v5[6] | v0[7],v1[7],v4[7],v5[7]
    __m256 out4 = _mm256_permute2f128_ps(p2, p3, 0x20); // v2[0],v3[0],v6[0],v7[0] | v2[1],v3[1],v6[1],v7[1]
    __m256 out5 = _mm256_permute2f128_ps(p6, p7, 0x20); // v2[2],v3[2],v6[2],v7[2] | v2[3],v3[3],v6[3],v7[3]
    __m256 out6 = _mm256_permute2f128_ps(p2, p3, 0x31); // v2[4],v3[4],v6[4],v7[4] | v2[5],v3[5],v6[5],v7[5]
    __m256 out7 = _mm256_permute2f128_ps(p6, p7, 0x31); // v2[6],v3[6],v6[6],v7[6] | v2[7],v3[7],v6[7],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29, 32, 33, 36, 37, 40, 41, 44, 45, 48, 49,
             52, 53, 56, 57, 60, 61, 2, 3, 6, 7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31, 34, 35, 38,
             39, 42, 43, 46, 47, 50, 51, 54, 55, 58, 59, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: permute2f128 to pair low/high 128-bit lanes across register pairs ──
    __m256 t0 = _mm256_permute2f128_ps(v0, v1, 0x20); // v0[0..3] | v1[0..3]
    __m256 t1 = _mm256_permute2f128_ps(v0, v1, 0x31); // v0[4..7] | v1[4..7]
    __m256 t2 = _mm256_permute2f128_ps(v2, v3, 0x20); // v2[0..3] | v3[0..3]
    __m256 t3 = _mm256_permute2f128_ps(v2, v3, 0x31); // v2[4..7] | v3[4..7]
    __m256 t4 = _mm256_permute2f128_ps(v4, v5, 0x20); // v4[0..3] | v5[0..3]
    __m256 t5 = _mm256_permute2f128_ps(v4, v5, 0x31); // v4[4..7] | v5[4..7]
    __m256 t6 = _mm256_permute2f128_ps(v6, v7, 0x20); // v6[0..3] | v7[0..3]
    __m256 t7 = _mm256_permute2f128_ps(v6, v7, 0x31); // v6[4..7] | v7[4..7]

    // ── Stage 2: shuffle to select even/odd pairs (0x44 = [0,1,0,1], 0xEE = [2,3,2,3]) ──
    __m256 out0 = _mm256_shuffle_ps(t0, t1, 0x44); // v0[0],v0[1],v0[4],v0[5] | v1[0],v1[1],v1[4],v1[5]
    __m256 out4 = _mm256_shuffle_ps(t0, t1, 0xEE); // v0[2],v0[3],v0[6],v0[7] | v1[2],v1[3],v1[6],v1[7]
    __m256 out1 = _mm256_shuffle_ps(t2, t3, 0x44); // v2[0],v2[1],v2[4],v2[5] | v3[0],v3[1],v3[4],v3[5]
    __m256 out5 = _mm256_shuffle_ps(t2, t3, 0xEE); // v2[2],v2[3],v2[6],v2[7] | v3[2],v3[3],v3[6],v3[7]
    __m256 out2 = _mm256_shuffle_ps(t4, t5, 0x44); // v4[0],v4[1],v4[4],v4[5] | v5[0],v5[1],v5[4],v5[5]
    __m256 out6 = _mm256_shuffle_ps(t4, t5, 0xEE); // v4[2],v4[3],v4[6],v4[7] | v5[2],v5[3],v5[6],v5[7]
    __m256 out3 = _mm256_shuffle_ps(t6, t7, 0x44); // v6[0],v6[1],v6[4],v6[5] | v7[0],v7[1],v7[4],v7[5]
    __m256 out7 = _mm256_shuffle_ps(t6, t7, 0xEE); // v6[2],v6[3],v6[6],v6[7] | v7[2],v7[3],v7[6],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15, 16, 18, 20, 22, 24, 26, 28, 30, 17, 19, 21,
             23, 25, 27, 29, 31, 32, 34, 36, 38, 40, 42, 44, 46, 33, 35, 37, 39, 41, 43, 45, 47, 48, 50, 52,
             54, 56, 58, 60, 62, 49, 51, 53, 55, 57, 59, 61, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: permute2f128 to pair low/high 128-bit lanes across register pairs ──
    __m256 t0 = _mm256_permute2f128_ps(v0, v1, 0x20); // v0[0..3] | v1[0..3]
    __m256 t1 = _mm256_permute2f128_ps(v0, v1, 0x31); // v0[4..7] | v1[4..7]
    __m256 t2 = _mm256_permute2f128_ps(v2, v3, 0x20); // v2[0..3] | v3[0..3]
    __m256 t3 = _mm256_permute2f128_ps(v2, v3, 0x31); // v2[4..7] | v3[4..7]
    __m256 t4 = _mm256_permute2f128_ps(v4, v5, 0x20); // v4[0..3] | v5[0..3]
    __m256 t5 = _mm256_permute2f128_ps(v4, v5, 0x31); // v4[4..7] | v5[4..7]
    __m256 t6 = _mm256_permute2f128_ps(v6, v7, 0x20); // v6[0..3] | v7[0..3]
    __m256 t7 = _mm256_permute2f128_ps(v6, v7, 0x31); // v6[4..7] | v7[4..7]

    // ── Stage 2: shuffle to deinterleave even/odd elements (0x88 = [0,2,0,2], 0xDD = [1,3,1,3]) ──
    __m256 out0 = _mm256_shuffle_ps(t0, t1, 0x88); // v0[0],v0[2],v0[4],v0[6] | v1[0],v1[2],v1[4],v1[6]
    __m256 out1 = _mm256_shuffle_ps(t0, t1, 0xDD); // v0[1],v0[3],v0[5],v0[7] | v1[1],v1[3],v1[5],v1[7]
    __m256 out2 = _mm256_shuffle_ps(t2, t3, 0x88); // v2[0],v2[2],v2[4],v2[6] | v3[0],v3[2],v3[4],v3[6]
    __m256 out3 = _mm256_shuffle_ps(t2, t3, 0xDD); // v2[1],v2[3],v2[5],v2[7] | v3[1],v3[3],v3[5],v3[7]
    __m256 out4 = _mm256_shuffle_ps(t4, t5, 0x88); // v4[0],v4[2],v4[4],v4[6] | v5[0],v5[2],v5[4],v5[6]
    __m256 out5 = _mm256_shuffle_ps(t4, t5, 0xDD); // v4[1],v4[3],v4[5],v4[7] | v5[1],v5[3],v5[5],v5[7]
    __m256 out6 = _mm256_shuffle_ps(t6, t7, 0x88); // v6[0],v6[2],v6[4],v6[6] | v7[0],v7[2],v7[4],v7[6]
    __m256 out7 = _mm256_shuffle_ps(t6, t7, 0xDD); // v6[1],v6[3],v6[5],v6[7] | v7[1],v7[3],v7[5],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 32, 33, 2, 3, 34, 35, 4, 5, 36, 37, 6, 7, 38, 39, 8, 9, 40, 41, 10, 11, 42, 43, 12, 13, 44,
             45, 14, 15, 46, 47, 16, 17, 48, 49, 18, 19, 50, 51, 20, 21, 52, 53, 22, 23, 54, 55, 24, 25, 56,
             57, 26, 27, 58, 59, 28, 29, 60, 61, 30, 31, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: shuffle to interleave pairs within each 128-bit lane ──
    __m256 t0 = _mm256_shuffle_ps(v0, v4, 0x44); // v0[0],v0[1],v4[0],v4[1] | v0[4],v0[5],v4[4],v4[5]
    __m256 t1 = _mm256_shuffle_ps(v0, v4, 0xEE); // v0[2],v0[3],v4[2],v4[3] | v0[6],v0[7],v4[6],v4[7]
    __m256 t2 = _mm256_shuffle_ps(v1, v5, 0x44); // v1[0],v1[1],v5[0],v5[1] | v1[4],v1[5],v5[4],v5[5]
    __m256 t3 = _mm256_shuffle_ps(v1, v5, 0xEE); // v1[2],v1[3],v5[2],v5[3] | v1[6],v1[7],v5[6],v5[7]
    __m256 t4 = _mm256_shuffle_ps(v2, v6, 0x44); // v2[0],v2[1],v6[0],v6[1] | v2[4],v2[5],v6[4],v6[5]
    __m256 t5 = _mm256_shuffle_ps(v2, v6, 0xEE); // v2[2],v2[3],v6[2],v6[3] | v2[6],v2[7],v6[6],v6[7]
    __m256 t6 = _mm256_shuffle_ps(v3, v7, 0x44); // v3[0],v3[1],v7[0],v7[1] | v3[4],v3[5],v7[4],v7[5]
    __m256 t7 = _mm256_shuffle_ps(v3, v7, 0xEE); // v3[2],v3[3],v7[2],v7[3] | v3[6],v3[7],v7[6],v7[7]

    // ── Stage 2: permute2f128 to select low/high 128-bit lanes ──
    __m256 out0 = _mm256_permute2f128_ps(t0, t1, 0x20); // t0_low | t1_low  = [0,1,32,33 | 2,3,34,35]
    __m256 out1 = _mm256_permute2f128_ps(t0, t1, 0x31); // t0_high | t1_high = [4,5,36,37 | 6,7,38,39]
    __m256 out2 = _mm256_permute2f128_ps(t2, t3, 0x20); // [8,9,40,41 | 10,11,42,43]
    __m256 out3 = _mm256_permute2f128_ps(t2, t3, 0x31); // [12,13,44,45 | 14,15,46,47]
    __m256 out4 = _mm256_permute2f128_ps(t4, t5, 0x20); // [16,17,48,49 | 18,19,50,51]
    __m256 out5 = _mm256_permute2f128_ps(t4, t5, 0x31); // [20,21,52,53 | 22,23,54,55]
    __m256 out6 = _mm256_permute2f128_ps(t6, t7, 0x20); // [24,25,56,57 | 26,27,58,59]
    __m256 out7 = _mm256_permute2f128_ps(t6, t7, 0x31); // [28,29,60,61 | 30,31,62,63]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 16, 1, 17, 32, 48, 33, 49, 8, 24, 9, 25, 40, 56, 41, 57, 4, 20, 5, 21, 36, 52, 37, 53, 12, 28,
             13, 29, 44, 60, 45, 61, 2, 18, 3, 19, 34, 50, 35, 51, 10, 26, 11, 27, 42, 58, 43, 59, 6, 22, 7,
             23, 38, 54, 39, 55, 14, 30, 15, 31, 46, 62, 47, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave stride-2 register pairs ──────────────────────────
    __m256 t0 = _mm256_unpacklo_ps(v0, v2); // v0[0],v2[0],v0[1],v2[1] | v0[4],v2[4],v0[5],v2[5]
    __m256 t1 = _mm256_unpackhi_ps(v0, v2); // v0[2],v2[2],v0[3],v2[3] | v0[6],v2[6],v0[7],v2[7]
    __m256 t2 = _mm256_unpacklo_ps(v1, v3); // v1[0],v3[0],v1[1],v3[1] | v1[4],v3[4],v1[5],v3[5]
    __m256 t3 = _mm256_unpackhi_ps(v1, v3); // v1[2],v3[2],v1[3],v3[3] | v1[6],v3[6],v1[7],v3[7]
    __m256 t4 = _mm256_unpacklo_ps(v4, v6); // v4[0],v6[0],v4[1],v6[1] | v4[4],v6[4],v4[5],v6[5]
    __m256 t5 = _mm256_unpackhi_ps(v4, v6); // v4[2],v6[2],v4[3],v6[3] | v4[6],v6[6],v4[7],v6[7]
    __m256 t6 = _mm256_unpacklo_ps(v5, v7); // v5[0],v7[0],v5[1],v7[1] | v5[4],v7[4],v5[5],v7[5]
    __m256 t7 = _mm256_unpackhi_ps(v5, v7); // v5[2],v7[2],v5[3],v7[3] | v5[6],v7[6],v5[7],v7[7]

    // ── Stage 2: permute2f128 — combine low/high 128-bit halves ──────────────
    __m256 out0 = _mm256_permute2f128_ps(t0, t4, 0x20); // v0[0],v2[0],v0[1],v2[1] | v4[0],v6[0],v4[1],v6[1]
    __m256 out1 = _mm256_permute2f128_ps(t2, t6, 0x20); // v1[0],v3[0],v1[1],v3[1] | v5[0],v7[0],v5[1],v7[1]
    __m256 out2 = _mm256_permute2f128_ps(t0, t4, 0x31); // v0[4],v2[4],v0[5],v2[5] | v4[4],v6[4],v4[5],v6[5]
    __m256 out3 = _mm256_permute2f128_ps(t2, t6, 0x31); // v1[4],v3[4],v1[5],v3[5] | v5[4],v7[4],v5[5],v7[5]
    __m256 out4 = _mm256_permute2f128_ps(t1, t5, 0x20); // v0[2],v2[2],v0[3],v2[3] | v4[2],v6[2],v4[3],v6[3]
    __m256 out5 = _mm256_permute2f128_ps(t3, t7, 0x20); // v1[2],v3[2],v1[3],v3[3] | v5[2],v7[2],v5[3],v7[3]
    __m256 out6 = _mm256_permute2f128_ps(t1, t5, 0x31); // v0[6],v2[6],v0[7],v2[7] | v4[6],v6[6],v4[7],v6[7]
    __m256 out7 = _mm256_permute2f128_ps(t3, t7, 0x31); // v1[6],v3[6],v1[7],v3[7] | v5[6],v7[6],v5[7],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 16, 17, 8, 9, 24, 25, 4, 5, 20, 21, 12, 13, 28, 29, 2, 3, 18, 19, 10, 11, 26, 27, 6, 7, 22,
             23, 14, 15, 30, 31, 32, 33, 48, 49, 40, 41, 56, 57, 36, 37, 52, 53, 44, 45, 60, 61, 34, 35, 50,
             51, 42, 43, 58, 59, 38, 39, 54, 55, 46, 47, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: shuffle_ps to pair consecutive 2-element groups across stride-2 registers ──
    __m256 t0 = _mm256_shuffle_ps(v0, v2, 0x44); // v0[0],v0[1],v2[0],v2[1] | v0[4],v0[5],v2[4],v2[5]
    __m256 t1 = _mm256_shuffle_ps(v0, v2, 0xEE); // v0[2],v0[3],v2[2],v2[3] | v0[6],v0[7],v2[6],v2[7]
    __m256 t2 = _mm256_shuffle_ps(v1, v3, 0x44); // v1[0],v1[1],v3[0],v3[1] | v1[4],v1[5],v3[4],v3[5]
    __m256 t3 = _mm256_shuffle_ps(v1, v3, 0xEE); // v1[2],v1[3],v3[2],v3[3] | v1[6],v1[7],v3[6],v3[7]
    __m256 t4 = _mm256_shuffle_ps(v4, v6, 0x44); // v4[0],v4[1],v6[0],v6[1] | v4[4],v4[5],v6[4],v6[5]
    __m256 t5 = _mm256_shuffle_ps(v4, v6, 0xEE); // v4[2],v4[3],v6[2],v6[3] | v4[6],v4[7],v6[6],v6[7]
    __m256 t6 = _mm256_shuffle_ps(v5, v7, 0x44); // v5[0],v5[1],v7[0],v7[1] | v5[4],v5[5],v7[4],v7[5]
    __m256 t7 = _mm256_shuffle_ps(v5, v7, 0xEE); // v5[2],v5[3],v7[2],v7[3] | v5[6],v5[7],v7[6],v7[7]

    // ── Stage 2: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(t0, t2, 0x20); // v0[0],v0[1],v2[0],v2[1] | v1[0],v1[1],v3[0],v3[1]
    __m256 out1 = _mm256_permute2f128_ps(t0, t2, 0x31); // v0[4],v0[5],v2[4],v2[5] | v1[4],v1[5],v3[4],v3[5]
    __m256 out2 = _mm256_permute2f128_ps(t1, t3, 0x20); // v0[2],v0[3],v2[2],v2[3] | v1[2],v1[3],v3[2],v3[3]
    __m256 out3 = _mm256_permute2f128_ps(t1, t3, 0x31); // v0[6],v0[7],v2[6],v2[7] | v1[6],v1[7],v3[6],v3[7]
    __m256 out4 = _mm256_permute2f128_ps(t4, t6, 0x20); // v4[0],v4[1],v6[0],v6[1] | v5[0],v5[1],v7[0],v7[1]
    __m256 out5 = _mm256_permute2f128_ps(t4, t6, 0x31); // v4[4],v4[5],v6[4],v6[5] | v5[4],v5[5],v7[4],v7[5]
    __m256 out6 = _mm256_permute2f128_ps(t5, t7, 0x20); // v4[2],v4[3],v6[2],v6[3] | v5[2],v5[3],v7[2],v7[3]
    __m256 out7 = _mm256_permute2f128_ps(t5, t7, 0x31); // v4[6],v4[7],v6[6],v6[7] | v5[6],v5[7],v7[6],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 8, 32, 40, 16, 24, 48, 56, 1, 9, 33, 41, 17, 25, 49, 57, 2, 10, 34, 42, 18, 26, 50, 58, 3, 11,
             35, 43, 19, 27, 51, 59, 4, 12, 36, 44, 20, 28, 52, 60, 5, 13, 37, 45, 21, 29, 53, 61, 6, 14, 38,
             46, 22, 30, 54, 62, 7, 15, 39, 47, 23, 31, 55, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave adjacent pairs within {v0,v1}, {v4,v5}, {v2,v3}, {v6,v7} ──
    __m256 t0 = _mm256_unpacklo_ps(v0, v1); // v0[0],v1[0],v0[1],v1[1] | v0[4],v1[4],v0[5],v1[5]
    __m256 t1 = _mm256_unpackhi_ps(v0, v1); // v0[2],v1[2],v0[3],v1[3] | v0[6],v1[6],v0[7],v1[7]
    __m256 t4 = _mm256_unpacklo_ps(v4, v5); // v4[0],v5[0],v4[1],v5[1] | v4[4],v5[4],v4[5],v5[5]
    __m256 t5 = _mm256_unpackhi_ps(v4, v5); // v4[2],v5[2],v4[3],v5[3] | v4[6],v5[6],v4[7],v5[7]
    __m256 t2 = _mm256_unpacklo_ps(v2, v3); // v2[0],v3[0],v2[1],v3[1] | v2[4],v3[4],v2[5],v3[5]
    __m256 t3 = _mm256_unpackhi_ps(v2, v3); // v2[2],v3[2],v2[3],v3[3] | v2[6],v3[6],v2[7],v3[7]
    __m256 t6 = _mm256_unpacklo_ps(v6, v7); // v6[0],v7[0],v6[1],v7[1] | v6[4],v7[4],v6[5],v7[5]
    __m256 t7 = _mm256_unpackhi_ps(v6, v7); // v6[2],v7[2],v6[3],v7[3] | v6[6],v7[6],v6[7],v7[7]

    // ── Stage 2: shuffle to form quads {v0,v1,v4,v5} and {v2,v3,v6,v7} ────────
    __m256 p0 = _mm256_shuffle_ps(t0, t4, 0x44); // v0[0],v1[0],v4[0],v5[0] | v0[4],v1[4],v4[4],v5[4]
    __m256 p1 = _mm256_shuffle_ps(t0, t4, 0xEE); // v0[1],v1[1],v4[1],v5[1] | v0[5],v1[5],v4[5],v5[5]
    __m256 p2 = _mm256_shuffle_ps(t1, t5, 0x44); // v0[2],v1[2],v4[2],v5[2] | v0[6],v1[6],v4[6],v5[6]
    __m256 p3 = _mm256_shuffle_ps(t1, t5, 0xEE); // v0[3],v1[3],v4[3],v5[3] | v0[7],v1[7],v4[7],v5[7]
    __m256 p4 = _mm256_shuffle_ps(t2, t6, 0x44); // v2[0],v3[0],v6[0],v7[0] | v2[4],v3[4],v6[4],v7[4]
    __m256 p5 = _mm256_shuffle_ps(t2, t6, 0xEE); // v2[1],v3[1],v6[1],v7[1] | v2[5],v3[5],v6[5],v7[5]
    __m256 p6 = _mm256_shuffle_ps(t3, t7, 0x44); // v2[2],v3[2],v6[2],v7[2] | v2[6],v3[6],v6[6],v7[6]
    __m256 p7 = _mm256_shuffle_ps(t3, t7, 0xEE); // v2[3],v3[3],v6[3],v7[3] | v2[7],v3[7],v6[7],v7[7]

    // ── Stage 3: permute2f128 — combine low/high 128-bit lanes ───────────────
    __m256 out0 = _mm256_permute2f128_ps(p0, p4, 0x20); // v0[0],v1[0],v4[0],v5[0] | v2[0],v3[0],v6[0],v7[0]
    __m256 out1 = _mm256_permute2f128_ps(p1, p5, 0x20); // v0[1],v1[1],v4[1],v5[1] | v2[1],v3[1],v6[1],v7[1]
    __m256 out2 = _mm256_permute2f128_ps(p2, p6, 0x20); // v0[2],v1[2],v4[2],v5[2] | v2[2],v3[2],v6[2],v7[2]
    __m256 out3 = _mm256_permute2f128_ps(p3, p7, 0x20); // v0[3],v1[3],v4[3],v5[3] | v2[3],v3[3],v6[3],v7[3]
    __m256 out4 = _mm256_permute2f128_ps(p0, p4, 0x31); // v0[4],v1[4],v4[4],v5[4] | v2[4],v3[4],v6[4],v7[4]
    __m256 out5 = _mm256_permute2f128_ps(p1, p5, 0x31); // v0[5],v1[5],v4[5],v5[5] | v2[5],v3[5],v6[5],v7[5]
    __m256 out6 = _mm256_permute2f128_ps(p2, p6, 0x31); // v0[6],v1[6],v4[6],v5[6] | v2[6],v3[6],v6[6],v7[6]
    __m256 out7 = _mm256_permute2f128_ps(p3, p7, 0x31); // v0[7],v1[7],v4[7],v5[7] | v2[7],v3[7],v6[7],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 16, 17, 32, 33, 48, 49, 8, 9, 24, 25, 40, 41, 56, 57, 4, 5, 20, 21, 36, 37, 52, 53, 12, 13,
             28, 29, 44, 45, 60, 61, 2, 3, 18, 19, 34, 35, 50, 51, 10, 11, 26, 27, 42, 43, 58, 59, 6, 7, 22,
             23, 38, 39, 54, 55, 14, 15, 30, 31, 46, 47, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave adjacent pairs within even {v0,v2,v4,v6} and odd {v1,v3,v5,v7} groups ──
    __m256 t02lo = _mm256_shuffle_ps(v0, v2, 0x44); // v0[0],v0[1],v2[0],v2[1] | v0[4],v0[5],v2[4],v2[5]
    __m256 t02hi = _mm256_shuffle_ps(v0, v2, 0xEE); // v0[2],v0[3],v2[2],v2[3] | v0[6],v0[7],v2[6],v2[7]
    __m256 t13lo = _mm256_shuffle_ps(v1, v3, 0x44); // v1[0],v1[1],v3[0],v3[1] | v1[4],v1[5],v3[4],v3[5]
    __m256 t13hi = _mm256_shuffle_ps(v1, v3, 0xEE); // v1[2],v1[3],v3[2],v3[3] | v1[6],v1[7],v3[6],v3[7]
    __m256 t46lo = _mm256_shuffle_ps(v4, v6, 0x44); // v4[0],v4[1],v6[0],v6[1] | v4[4],v4[5],v6[4],v6[5]
    __m256 t46hi = _mm256_shuffle_ps(v4, v6, 0xEE); // v4[2],v4[3],v6[2],v6[3] | v4[6],v4[7],v6[6],v6[7]
    __m256 t57lo = _mm256_shuffle_ps(v5, v7, 0x44); // v5[0],v5[1],v7[0],v7[1] | v5[4],v5[5],v7[4],v7[5]
    __m256 t57hi = _mm256_shuffle_ps(v5, v7, 0xEE); // v5[2],v5[3],v7[2],v7[3] | v5[6],v5[7],v7[6],v7[7]

    // ── Stage 2: combine 128-bit lanes across quad-groups ────────────────────
    __m256 out0 =
        _mm256_permute2f128_ps(t02lo, t46lo, 0x20); // v0[0],v0[1],v2[0],v2[1] | v4[0],v4[1],v6[0],v6[1]
    __m256 out1 =
        _mm256_permute2f128_ps(t13lo, t57lo, 0x20); // v1[0],v1[1],v3[0],v3[1] | v5[0],v5[1],v7[0],v7[1]
    __m256 out2 =
        _mm256_permute2f128_ps(t02lo, t46lo, 0x31); // v0[4],v0[5],v2[4],v2[5] | v4[4],v4[5],v6[4],v6[5]
    __m256 out3 =
        _mm256_permute2f128_ps(t13lo, t57lo, 0x31); // v1[4],v1[5],v3[4],v3[5] | v5[4],v5[5],v7[4],v7[5]
    __m256 out4 =
        _mm256_permute2f128_ps(t02hi, t46hi, 0x20); // v0[2],v0[3],v2[2],v2[3] | v4[2],v4[3],v6[2],v6[3]
    __m256 out5 =
        _mm256_permute2f128_ps(t13hi, t57hi, 0x20); // v1[2],v1[3],v3[2],v3[3] | v5[2],v5[3],v7[2],v7[3]
    __m256 out6 =
        _mm256_permute2f128_ps(t02hi, t46hi, 0x31); // v0[6],v0[7],v2[6],v2[7] | v4[6],v4[7],v6[6],v6[7]
    __m256 out7 =
        _mm256_permute2f128_ps(t13hi, t57hi, 0x31); // v1[6],v1[7],v3[6],v3[7] | v5[6],v5[7],v7[6],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 1, 32, 33, 16, 17, 48, 49, 8, 9, 40, 41, 24, 25, 56, 57, 2, 3, 34, 35, 18, 19, 50, 51, 10, 11,
             42, 43, 26, 27, 58, 59, 4, 5, 36, 37, 20, 21, 52, 53, 12, 13, 44, 45, 28, 29, 60, 61, 6, 7, 38,
             39, 22, 23, 54, 55, 14, 15, 46, 47, 30, 31, 62, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // ── Stage 1: interleave adjacent pairs within distance-2 groups {v0,v4}, {v2,v6}, {v1,v5}, {v3,v7} ──
    __m256 t04lo = _mm256_shuffle_ps(v0, v4, 0x44); // v0[0],v0[1],v4[0],v4[1] | v0[4],v0[5],v4[4],v4[5]
    __m256 t04hi = _mm256_shuffle_ps(v0, v4, 0xEE); // v0[2],v0[3],v4[2],v4[3] | v0[6],v0[7],v4[6],v4[7]
    __m256 t26lo = _mm256_shuffle_ps(v2, v6, 0x44); // v2[0],v2[1],v6[0],v6[1] | v2[4],v2[5],v6[4],v6[5]
    __m256 t26hi = _mm256_shuffle_ps(v2, v6, 0xEE); // v2[2],v2[3],v6[2],v6[3] | v2[6],v2[7],v6[6],v6[7]
    __m256 t15lo = _mm256_shuffle_ps(v1, v5, 0x44); // v1[0],v1[1],v5[0],v5[1] | v1[4],v1[5],v5[4],v5[5]
    __m256 t15hi = _mm256_shuffle_ps(v1, v5, 0xEE); // v1[2],v1[3],v5[2],v5[3] | v1[6],v1[7],v5[6],v5[7]
    __m256 t37lo = _mm256_shuffle_ps(v3, v7, 0x44); // v3[0],v3[1],v7[0],v7[1] | v3[4],v3[5],v7[4],v7[5]
    __m256 t37hi = _mm256_shuffle_ps(v3, v7, 0xEE); // v3[2],v3[3],v7[2],v7[3] | v3[6],v3[7],v7[6],v7[7]

    // ── Stage 2: combine 128-bit lanes ───────────────────────────────────────
    __m256 out0 =
        _mm256_permute2f128_ps(t04lo, t26lo, 0x20); // v0[0],v0[1],v4[0],v4[1] | v2[0],v2[1],v6[0],v6[1]
    __m256 out1 =
        _mm256_permute2f128_ps(t15lo, t37lo, 0x20); // v1[0],v1[1],v5[0],v5[1] | v3[0],v3[1],v7[0],v7[1]
    __m256 out2 =
        _mm256_permute2f128_ps(t04hi, t26hi, 0x20); // v0[2],v0[3],v4[2],v4[3] | v2[2],v2[3],v6[2],v6[3]
    __m256 out3 =
        _mm256_permute2f128_ps(t15hi, t37hi, 0x20); // v1[2],v1[3],v5[2],v5[3] | v3[2],v3[3],v7[2],v7[3]
    __m256 out4 =
        _mm256_permute2f128_ps(t04lo, t26lo, 0x31); // v0[4],v0[5],v4[4],v4[5] | v2[4],v2[5],v6[4],v6[5]
    __m256 out5 =
        _mm256_permute2f128_ps(t15lo, t37lo, 0x31); // v1[4],v1[5],v5[4],v5[5] | v3[4],v3[5],v7[4],v7[5]
    __m256 out6 =
        _mm256_permute2f128_ps(t04hi, t26hi, 0x31); // v0[6],v0[7],v4[6],v4[7] | v2[6],v2[7],v6[6],v6[7]
    __m256 out7 =
        _mm256_permute2f128_ps(t15hi, t37hi, 0x31); // v1[6],v1[7],v5[6],v5[7] | v3[6],v3[7],v7[6],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 64> vec<f32, 64>::shuffle(
    csizes_t<0, 8, 1, 9, 4, 12, 5, 13, 2, 10, 3, 11, 6, 14, 7, 15, 16, 24, 17, 25, 20, 28, 21, 29, 18, 26, 19,
             27, 22, 30, 23, 31, 32, 40, 33, 41, 36, 44, 37, 45, 34, 42, 35, 43, 38, 46, 39, 47, 48, 56, 49,
             57, 52, 60, 53, 61, 50, 58, 51, 59, 54, 62, 55, 63>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;
    __m256 v4 = slice<32, 8>(*this).v;
    __m256 v5 = slice<40, 8>(*this).v;
    __m256 v6 = slice<48, 8>(*this).v;
    __m256 v7 = slice<56, 8>(*this).v;

    // Pairwise interleave of adjacent 8-element groups
    __m256 out0 = _mm256_unpacklo_ps(v0, v1); // v0[0],v1[0],v0[1],v1[1] | v0[4],v1[4],v0[5],v1[5]
    __m256 out1 = _mm256_unpackhi_ps(v0, v1); // v0[2],v1[2],v0[3],v1[3] | v0[6],v1[6],v0[7],v1[7]
    __m256 out2 = _mm256_unpacklo_ps(v2, v3); // v2[0],v3[0],v2[1],v3[1] | v2[4],v3[4],v2[5],v3[5]
    __m256 out3 = _mm256_unpackhi_ps(v2, v3); // v2[2],v3[2],v2[3],v3[3] | v2[6],v3[6],v2[7],v3[7]
    __m256 out4 = _mm256_unpacklo_ps(v4, v5); // v4[0],v5[0],v4[1],v5[1] | v4[4],v5[4],v4[5],v5[5]
    __m256 out5 = _mm256_unpackhi_ps(v4, v5); // v4[2],v5[2],v4[3],v5[3] | v4[6],v5[6],v4[7],v5[7]
    __m256 out6 = _mm256_unpacklo_ps(v6, v7); // v6[0],v7[0],v6[1],v7[1] | v6[4],v7[4],v6[5],v7[5]
    __m256 out7 = _mm256_unpackhi_ps(v6, v7); // v6[2],v7[2],v6[3],v7[3] | v6[6],v7[6],v6[7],v7[7]

    return concat(concat( //
                      concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), //
                      concat(vec<f32, 8>(out2), vec<f32, 8>(out3)) //
                      ),
                  concat( //
                      concat(vec<f32, 8>(out4), vec<f32, 8>(out5)), //
                      concat(vec<f32, 8>(out6), vec<f32, 8>(out7))));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 1, 16, 17, 8, 9, 24, 25, 4, 5, 20, 21, 12, 13, 28, 29, 2, 3, 18, 19, 10, 11, 26, 27, 6, 7, 22,
             23, 14, 15, 30, 31>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;

    // ── Stage 1: interleave adjacent pairs within {v0,v2} and {v1,v3} ──────
    __m256 t02lo = _mm256_shuffle_ps(v0, v2, 0x44); // v0[0],v0[1],v2[0],v2[1] | v0[4],v0[5],v2[4],v2[5]
    __m256 t02hi = _mm256_shuffle_ps(v0, v2, 0xEE); // v0[2],v0[3],v2[2],v2[3] | v0[6],v0[7],v2[6],v2[7]
    __m256 t13lo = _mm256_shuffle_ps(v1, v3, 0x44); // v1[0],v1[1],v3[0],v3[1] | v1[4],v1[5],v3[4],v3[5]
    __m256 t13hi = _mm256_shuffle_ps(v1, v3, 0xEE); // v1[2],v1[3],v3[2],v3[3] | v1[6],v1[7],v3[6],v3[7]

    // ── Stage 2: permute2f128 — combine low/high 128-bit lanes ──────────────
    __m256 out0 =
        _mm256_permute2f128_ps(t02lo, t13lo, 0x20); // v0[0],v0[1],v2[0],v2[1] | v1[0],v1[1],v3[0],v3[1]
    __m256 out1 =
        _mm256_permute2f128_ps(t02lo, t13lo, 0x31); // v0[4],v0[5],v2[4],v2[5] | v1[4],v1[5],v3[4],v3[5]
    __m256 out2 =
        _mm256_permute2f128_ps(t02hi, t13hi, 0x20); // v0[2],v0[3],v2[2],v2[3] | v1[2],v1[3],v3[2],v3[3]
    __m256 out3 =
        _mm256_permute2f128_ps(t02hi, t13hi, 0x31); // v0[6],v0[7],v2[6],v2[7] | v1[6],v1[7],v3[6],v3[7]

    return concat(concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), concat(vec<f32, 8>(out2), vec<f32, 8>(out3)));
}

template <>
template <>
KFR_INTRINSIC vec<f32, 32> vec<f32, 32>::shuffle(
    csizes_t<0, 1, 16, 17, 8, 9, 24, 25, 2, 3, 18, 19, 10, 11, 26, 27, 4, 5, 20, 21, 12, 13, 28, 29, 6, 7, 22,
             23, 14, 15, 30, 31>) const noexcept
{
    __m256 v0 = slice<0, 8>(*this).v;
    __m256 v1 = slice<8, 8>(*this).v;
    __m256 v2 = slice<16, 8>(*this).v;
    __m256 v3 = slice<24, 8>(*this).v;

    // ── Stage 1: interleave adjacent pairs within {v0,v2} and {v1,v3} ──────
    __m256 t02lo = _mm256_shuffle_ps(v0, v2, 0x44); // v0[0],v0[1],v2[0],v2[1] | v0[4],v0[5],v2[4],v2[5]
    __m256 t02hi = _mm256_shuffle_ps(v0, v2, 0xEE); // v0[2],v0[3],v2[2],v2[3] | v0[6],v0[7],v2[6],v2[7]
    __m256 t13lo = _mm256_shuffle_ps(v1, v3, 0x44); // v1[0],v1[1],v3[0],v3[1] | v1[4],v1[5],v3[4],v3[5]
    __m256 t13hi = _mm256_shuffle_ps(v1, v3, 0xEE); // v1[2],v1[3],v3[2],v3[3] | v1[6],v1[7],v3[6],v3[7]

    // ── Stage 2: combine lanes in sequential (non-bit-reversed) pair order ──
    __m256 out0 =
        _mm256_permute2f128_ps(t02lo, t13lo, 0x20); // v0[0],v0[1],v2[0],v2[1] | v1[0],v1[1],v3[0],v3[1]
    __m256 out1 =
        _mm256_permute2f128_ps(t02hi, t13hi, 0x20); // v0[2],v0[3],v2[2],v2[3] | v1[2],v1[3],v3[2],v3[3]
    __m256 out2 =
        _mm256_permute2f128_ps(t02lo, t13lo, 0x31); // v0[4],v0[5],v2[4],v2[5] | v1[4],v1[5],v3[4],v3[5]
    __m256 out3 =
        _mm256_permute2f128_ps(t02hi, t13hi, 0x31); // v0[6],v0[7],v2[6],v2[7] | v1[6],v1[7],v3[6],v3[7]

    return concat(concat(vec<f32, 8>(out0), vec<f32, 8>(out1)), concat(vec<f32, 8>(out2), vec<f32, 8>(out3)));
}

#endif

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

} // namespace KFR_ARCH_NAME
} // namespace kfr
#endif
