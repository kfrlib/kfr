/** @addtogroup dft
 *  @{
 */
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

#include "../../simd/complex.hpp"
#include "../../simd/constants.hpp"
#include "../../simd/digitreverse.hpp"
#include "../../simd/vec.hpp"

#include "../data/bitrev.hpp"

#include "ft.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

constexpr inline static bool fft_reorder_aligned = false;

constexpr inline static size_t bitrev_table_log2N = ilog2(arraysize(data::bitrev_table));

template <size_t Bits>
CMT_GNU_CONSTEXPR inline u32 bitrev_using_table(u32 x)
{
    if (Bits > bitrev_table_log2N)
        return bitreverse<Bits>(x);

    return data::bitrev_table[x] >> (bitrev_table_log2N - Bits);
}

CMT_GNU_CONSTEXPR inline u32 bitrev_using_table(u32 x, size_t bits)
{
    if (bits > bitrev_table_log2N)
        return bitreverse<32>(x) >> (32 - bits);

    return data::bitrev_table[x] >> (bitrev_table_log2N - bits);
}

CMT_GNU_CONSTEXPR inline u32 dig4rev_using_table(u32 x, size_t bits)
{
    if (bits > bitrev_table_log2N)
        return digitreverse4<32>(x) >> (32 - bits);

    x = data::bitrev_table[x];
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = x >> (bitrev_table_log2N - bits);
    return x;
}

template <size_t log2n, size_t bitrev, typename T>
KFR_INTRINSIC void fft_reorder_swap(T* inout, size_t i)
{
    using cxx           = cvec<T, 16>;
    constexpr size_t N  = 1 << log2n;
    constexpr size_t N4 = 2 * N / 4;

    cxx vi = cread_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i));
    vi     = digitreverse<bitrev, 2>(vi);
    cwrite_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i), vi);
}

template <size_t log2n, size_t bitrev, typename T>
KFR_INTRINSIC void fft_reorder_swap_two(T* inout, size_t i, size_t j)
{
    CMT_ASSUME(i != j);
    using cxx           = cvec<T, 16>;
    constexpr size_t N  = 1 << log2n;
    constexpr size_t N4 = 2 * N / 4;

    cxx vi = cread_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i));
    cxx vj = cread_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + j));

    vi = digitreverse<bitrev, 2>(vi);
    cwrite_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i), vi);
    vj = digitreverse<bitrev, 2>(vj);
    cwrite_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + j), vj);
}

template <size_t log2n, size_t bitrev, typename T>
KFR_INTRINSIC void fft_reorder_swap(T* inout, size_t i, size_t j)
{
    CMT_ASSUME(i != j);
    using cxx           = cvec<T, 16>;
    constexpr size_t N  = 1 << log2n;
    constexpr size_t N4 = 2 * N / 4;

    cxx vi = cread_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i));
    cxx vj = cread_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + j));

    vi = digitreverse<bitrev, 2>(vi);
    cwrite_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + j), vi);
    vj = digitreverse<bitrev, 2>(vj);
    cwrite_group<4, 4, N4 / 2, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i), vj);
}

template <size_t log2n, size_t bitrev, typename T>
KFR_INTRINSIC void fft_reorder_swap(complex<T>* inout, size_t i)
{
    fft_reorder_swap<log2n, bitrev>(ptr_cast<T>(inout), i * 2);
}

template <size_t log2n, size_t bitrev, typename T>
KFR_INTRINSIC void fft_reorder_swap_two(complex<T>* inout, size_t i0, size_t i1)
{
    fft_reorder_swap_two<log2n, bitrev>(ptr_cast<T>(inout), i0 * 2, i1 * 2);
}

template <size_t log2n, size_t bitrev, typename T>
KFR_INTRINSIC void fft_reorder_swap(complex<T>* inout, size_t i, size_t j)
{
    fft_reorder_swap<log2n, bitrev>(ptr_cast<T>(inout), i * 2, j * 2);
}

template <typename T>
KFR_INTRINSIC void fft_reorder(complex<T>* inout, csize_t<11>)
{
    fft_reorder_swap_two<11>(inout, 0 * 4, 8 * 4);
    fft_reorder_swap<11>(inout, 1 * 4, 64 * 4);
    fft_reorder_swap<11>(inout, 2 * 4, 32 * 4);
    fft_reorder_swap<11>(inout, 3 * 4, 96 * 4);
    fft_reorder_swap<11>(inout, 4 * 4, 16 * 4);
    fft_reorder_swap<11>(inout, 5 * 4, 80 * 4);
    fft_reorder_swap<11>(inout, 6 * 4, 48 * 4);
    fft_reorder_swap<11>(inout, 7 * 4, 112 * 4);
    fft_reorder_swap<11>(inout, 9 * 4, 72 * 4);
    fft_reorder_swap<11>(inout, 10 * 4, 40 * 4);
    fft_reorder_swap<11>(inout, 11 * 4, 104 * 4);
    fft_reorder_swap<11>(inout, 12 * 4, 24 * 4);
    fft_reorder_swap<11>(inout, 13 * 4, 88 * 4);
    fft_reorder_swap<11>(inout, 14 * 4, 56 * 4);
    fft_reorder_swap<11>(inout, 15 * 4, 120 * 4);
    fft_reorder_swap<11>(inout, 17 * 4, 68 * 4);
    fft_reorder_swap<11>(inout, 18 * 4, 36 * 4);
    fft_reorder_swap<11>(inout, 19 * 4, 100 * 4);
    fft_reorder_swap_two<11>(inout, 20 * 4, 28 * 4);
    fft_reorder_swap<11>(inout, 21 * 4, 84 * 4);
    fft_reorder_swap<11>(inout, 22 * 4, 52 * 4);
    fft_reorder_swap<11>(inout, 23 * 4, 116 * 4);
    fft_reorder_swap<11>(inout, 25 * 4, 76 * 4);
    fft_reorder_swap<11>(inout, 26 * 4, 44 * 4);
    fft_reorder_swap<11>(inout, 27 * 4, 108 * 4);
    fft_reorder_swap<11>(inout, 29 * 4, 92 * 4);
    fft_reorder_swap<11>(inout, 30 * 4, 60 * 4);
    fft_reorder_swap<11>(inout, 31 * 4, 124 * 4);
    fft_reorder_swap<11>(inout, 33 * 4, 66 * 4);
    fft_reorder_swap_two<11>(inout, 34 * 4, 42 * 4);
    fft_reorder_swap<11>(inout, 35 * 4, 98 * 4);
    fft_reorder_swap<11>(inout, 37 * 4, 82 * 4);
    fft_reorder_swap<11>(inout, 38 * 4, 50 * 4);
    fft_reorder_swap<11>(inout, 39 * 4, 114 * 4);
    fft_reorder_swap<11>(inout, 41 * 4, 74 * 4);
    fft_reorder_swap<11>(inout, 43 * 4, 106 * 4);
    fft_reorder_swap<11>(inout, 45 * 4, 90 * 4);
    fft_reorder_swap<11>(inout, 46 * 4, 58 * 4);
    fft_reorder_swap<11>(inout, 47 * 4, 122 * 4);
    fft_reorder_swap<11>(inout, 49 * 4, 70 * 4);
    fft_reorder_swap<11>(inout, 51 * 4, 102 * 4);
    fft_reorder_swap<11>(inout, 53 * 4, 86 * 4);
    fft_reorder_swap_two<11>(inout, 54 * 4, 62 * 4);
    fft_reorder_swap<11>(inout, 55 * 4, 118 * 4);
    fft_reorder_swap<11>(inout, 57 * 4, 78 * 4);
    fft_reorder_swap<11>(inout, 59 * 4, 110 * 4);
    fft_reorder_swap<11>(inout, 61 * 4, 94 * 4);
    fft_reorder_swap<11>(inout, 63 * 4, 126 * 4);
    fft_reorder_swap_two<11>(inout, 65 * 4, 73 * 4);
    fft_reorder_swap<11>(inout, 67 * 4, 97 * 4);
    fft_reorder_swap<11>(inout, 69 * 4, 81 * 4);
    fft_reorder_swap<11>(inout, 71 * 4, 113 * 4);
    fft_reorder_swap<11>(inout, 75 * 4, 105 * 4);
    fft_reorder_swap<11>(inout, 77 * 4, 89 * 4);
    fft_reorder_swap<11>(inout, 79 * 4, 121 * 4);
    fft_reorder_swap<11>(inout, 83 * 4, 101 * 4);
    fft_reorder_swap_two<11>(inout, 85 * 4, 93 * 4);
    fft_reorder_swap<11>(inout, 87 * 4, 117 * 4);
    fft_reorder_swap<11>(inout, 91 * 4, 109 * 4);
    fft_reorder_swap<11>(inout, 95 * 4, 125 * 4);
    fft_reorder_swap_two<11>(inout, 99 * 4, 107 * 4);
    fft_reorder_swap<11>(inout, 103 * 4, 115 * 4);
    fft_reorder_swap<11>(inout, 111 * 4, 123 * 4);
    fft_reorder_swap_two<11>(inout, 119 * 4, 127 * 4);
}

template <typename T>
KFR_INTRINSIC void fft_reorder(complex<T>* inout, csize_t<7>)
{
    constexpr size_t bitrev = 2;
    fft_reorder_swap_two<7, bitrev>(inout, 0 * 4, 2 * 4);
    fft_reorder_swap<7, bitrev>(inout, 1 * 4, 4 * 4);
    fft_reorder_swap<7, bitrev>(inout, 3 * 4, 6 * 4);
    fft_reorder_swap_two<7, bitrev>(inout, 5 * 4, 7 * 4);
}

template <typename T>
KFR_INTRINSIC void fft_reorder(complex<T>* inout, csize_t<8>)
{
    constexpr size_t bitrev = 4;
    fft_reorder_swap_two<8, bitrev>(inout, 0 * 4, 5 * 4);
    fft_reorder_swap<8, bitrev>(inout, 1 * 4, 4 * 4);
    fft_reorder_swap<8, bitrev>(inout, 2 * 4, 8 * 4);
    fft_reorder_swap<8, bitrev>(inout, 3 * 4, 12 * 4);
    fft_reorder_swap<8, bitrev>(inout, 6 * 4, 9 * 4);
    fft_reorder_swap<8, bitrev>(inout, 7 * 4, 13 * 4);
    fft_reorder_swap_two<8, bitrev>(inout, 10 * 4, 15 * 4);
    fft_reorder_swap<8, bitrev>(inout, 11 * 4, 14 * 4);
}

template <typename T>
KFR_INTRINSIC void fft_reorder(complex<T>* inout, csize_t<9>)
{
    constexpr size_t bitrev = 2;
    fft_reorder_swap_two<9, bitrev>(inout, 0 * 4, 4 * 4);
    fft_reorder_swap<9, bitrev>(inout, 1 * 4, 16 * 4);
    fft_reorder_swap<9, bitrev>(inout, 2 * 4, 8 * 4);
    fft_reorder_swap<9, bitrev>(inout, 3 * 4, 24 * 4);
    fft_reorder_swap<9, bitrev>(inout, 5 * 4, 20 * 4);
    fft_reorder_swap<9, bitrev>(inout, 6 * 4, 12 * 4);
    fft_reorder_swap<9, bitrev>(inout, 7 * 4, 28 * 4);
    fft_reorder_swap<9, bitrev>(inout, 9 * 4, 18 * 4);
    fft_reorder_swap_two<9, bitrev>(inout, 10 * 4, 14 * 4);
    fft_reorder_swap<9, bitrev>(inout, 11 * 4, 26 * 4);
    fft_reorder_swap<9, bitrev>(inout, 13 * 4, 22 * 4);
    fft_reorder_swap<9, bitrev>(inout, 15 * 4, 30 * 4);
    fft_reorder_swap_two<9, bitrev>(inout, 17 * 4, 21 * 4);
    fft_reorder_swap<9, bitrev>(inout, 19 * 4, 25 * 4);
    fft_reorder_swap<9, bitrev>(inout, 23 * 4, 29 * 4);
    fft_reorder_swap_two<9, bitrev>(inout, 27 * 4, 31 * 4);
}

template <typename T, bool use_br2>
KFR_INTRINSIC void cwrite_reordered(T* out, const cvec<T, 16>& value, size_t N4, cbool_t<use_br2>)
{
    cwrite_group<4, 4, fft_reorder_aligned>(ptr_cast<complex<T>>(out), N4,
                                            digitreverse<(use_br2 ? 2 : 4), 2>(value));
}

template <typename T, bool use_br2>
KFR_INTRINSIC void fft_reorder_swap_n4(T* inout, size_t i, size_t j, size_t N4, cbool_t<use_br2>)
{
    CMT_ASSUME(i != j);
    const cvec<T, 16> vi = cread_group<4, 4, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + i), N4);
    const cvec<T, 16> vj = cread_group<4, 4, fft_reorder_aligned>(ptr_cast<complex<T>>(inout + j), N4);
    cwrite_reordered(inout + j, vi, N4, cbool_t<use_br2>());
    cwrite_reordered(inout + i, vj, N4, cbool_t<use_br2>());
}

template <typename T>
KFR_INTRINSIC void fft_reorder(complex<T>* inout, size_t log2n, ctrue_t use_br2)
{
    const size_t N         = size_t(1) << log2n;
    const size_t N4        = N / 4;
    const size_t iend      = N / 16 * 4 * 2;
    constexpr size_t istep = 2 * 4;
    const size_t jstep1    = (1 << (log2n - 5)) * 4 * 2;
    const size_t jstep2 = size_t(size_t(1) << (log2n - 5)) * 4 * 2 - size_t(size_t(1) << (log2n - 6)) * 4 * 2;
    T* io               = ptr_cast<T>(inout);

    for (size_t i = 0; i < iend;)
    {
        size_t j = bitrev_using_table(static_cast<u32>(i >> 3), log2n - 4) << 3;
        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep;
        j = j + jstep1;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep;
        j = j - jstep2;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep;
        j = j + jstep1;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep;
    }
}

template <typename T>
KFR_INTRINSIC void fft_reorder(complex<T>* inout, size_t log2n, cfalse_t use_br2)
{
    const size_t N         = size_t(1) << log2n;
    const size_t N4        = N / 4;
    const size_t N16       = N * 2 / 16;
    size_t iend            = N16;
    constexpr size_t istep = 2 * 4;
    const size_t jstep     = N / 64 * 4 * 2;
    T* io                  = ptr_cast<T>(inout);

    size_t i = 0;
    CMT_PRAGMA_CLANG(clang loop unroll_count(2))
    for (; i < iend;)
    {
        size_t j = dig4rev_using_table(static_cast<u32>(i >> 3), log2n - 4) << 3;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep * 4;
    }
    iend += N16;
    CMT_PRAGMA_CLANG(clang loop unroll_count(2))
    for (; i < iend;)
    {
        size_t j = dig4rev_using_table(static_cast<u32>(i >> 3), log2n - 4) << 3;

        fft_reorder_swap_n4(io, i, j, N4, use_br2);

        i += istep;
        j = j + jstep;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep * 3;
    }
    iend += N16;
    CMT_PRAGMA_CLANG(clang loop unroll_count(2))
    for (; i < iend;)
    {
        size_t j = dig4rev_using_table(static_cast<u32>(i >> 3), log2n - 4) << 3;

        fft_reorder_swap_n4(io, i, j, N4, use_br2);

        i += istep;
        j = j + jstep;

        fft_reorder_swap_n4(io, i, j, N4, use_br2);

        i += istep;
        j = j + jstep;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep * 2;
    }
    iend += N16;
    CMT_PRAGMA_CLANG(clang loop unroll_count(2))
    for (; i < iend;)
    {
        size_t j = dig4rev_using_table(static_cast<u32>(i >> 3), log2n - 4) << 3;

        fft_reorder_swap_n4(io, i, j, N4, use_br2);

        i += istep;
        j = j + jstep;

        fft_reorder_swap_n4(io, i, j, N4, use_br2);

        i += istep;
        j = j + jstep;

        fft_reorder_swap_n4(io, i, j, N4, use_br2);

        i += istep;
        j = j + jstep;

        if (i >= j)
            fft_reorder_swap_n4(io, i, j, N4, use_br2);
        i += istep;
    }
}
} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
