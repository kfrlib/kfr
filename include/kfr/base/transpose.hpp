/** @addtogroup tensor
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

#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include "expression.hpp"
#include "memory.hpp"
#include "shape.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
/// @brief Matrix transpose
template <size_t group = 1, typename T, index_t Dims>
void matrix_transpose(T* out, const T* in, shape<Dims> shape);

/// @brief Matrix transpose (complex)
template <size_t group = 1, typename T, index_t Dims>
void matrix_transpose(complex<T>* out, const complex<T>* in, shape<Dims> shape);

namespace internal
{

template <size_t group = 1, typename T, size_t N>
void matrix_transpose_block_one(T* out, const T* in, size_t i, size_t stride)
{
    if constexpr (N == 1)
    {
        write(out + group * i, kfr::read<group>(in + group * i));
    }
    else
    {
        vec<T, (group * N * N)> vi = read_group<N, N, group>(in + group * i, stride);
        vi                         = transpose<N, group>(vi);
        write_group<N, N, group>(out + group * i, stride, vi);
    }
}

template <size_t group = 1, typename T, size_t N>
void matrix_transpose_block_two(T* out, const T* in, size_t i, size_t j, size_t stride)
{
    if constexpr (N == 1)
    {
        vec<T, group> vi = kfr::read<group>(in + group * i);
        vec<T, group> vj = kfr::read<group>(in + group * j);
        write(out + group * i, vj);
        write(out + group * j, vi);
    }
    else
    {
        vec<T, (group * N * N)> vi = read_group<N, N, group>(in + group * i, stride);
        vec<T, (group * N * N)> vj = read_group<N, N, group>(in + group * j, stride);
        vi                         = transpose<N, group>(vi);
        vj                         = transpose<N, group>(vj);
        write_group<N, N, group>(out + group * i, stride, vj);
        write_group<N, N, group>(out + group * j, stride, vi);
    }
}

template <size_t group = 1, typename T>
void matrix_transpose_square_small(T* out, const T* in, size_t n)
{
    cswitch(csizeseq<6, 1>, n, // 1, 2, 3, 4, 5 or 6
            [&](auto n_) CMT_INLINE_LAMBDA
            {
                constexpr size_t n = CMT_CVAL(n_);
                write(out, transpose<n, group>(kfr::read<n * n * group>(in)));
            });
}

template <size_t group = 1, typename T>
void matrix_transpose_square(T* out, const T* in, size_t n, size_t stride)
{
#if 1
    constexpr size_t width = 4;
    const size_t nw        = align_down(n, width);
    const size_t wstride   = width * stride;

    size_t i        = 0;
    size_t istridei = 0;
    CMT_LOOP_NOUNROLL
    for (; i < nw; i += width)
    {
        matrix_transpose_block_one<group, T, width>(out, in, istridei, stride);

        size_t j        = i + width;
        size_t istridej = istridei + width;
        size_t jstridei = istridei + wstride;
        CMT_LOOP_NOUNROLL
        for (; j < nw; j += width)
        {
            matrix_transpose_block_two<group, T, width>(out, in, istridej, jstridei, stride);
            istridej += width;
            jstridei += wstride;
        }
        CMT_LOOP_NOUNROLL
        for (; j < n; ++j)
        {
            CMT_LOOP_NOUNROLL
            for (size_t ii = i; ii < i + width; ++ii)
            {
                matrix_transpose_block_two<group, T, 1>(out, in, istridej, jstridei, stride);
                istridej += stride;
                jstridei += 1;
            }
            istridej = istridej - stride * width + 1;
            jstridei = jstridei - width + stride;
        }
        istridei += width * (stride + 1);
    }

    CMT_LOOP_NOUNROLL
    for (; i < n; ++i)
    {
        matrix_transpose_block_one<group, T, 1>(out, in, i * stride + i, stride);
        CMT_LOOP_NOUNROLL
        for (size_t j = i + 1; j < n; ++j)
        {
            matrix_transpose_block_two<group, T, 1>(out, in, i * stride + j, j * stride + i, stride);
        }
    }
#else
    constexpr size_t width = 4;
    const size_t nw        = align_down(n, width);

    size_t i = 0;
    CMT_LOOP_NOUNROLL
    for (; i < nw; i += width)
    {
        matrix_transpose_block_one<group, T, width>(out, in, i * stride + i, stride);

        size_t j = i + width;
        CMT_LOOP_NOUNROLL
        for (; j < nw; j += width)
        {
            matrix_transpose_block_two<group, T, width>(out, in, i * stride + j, j * stride + i, stride);
        }
        CMT_LOOP_NOUNROLL
        for (; j < n; ++j)
        {
            CMT_LOOP_NOUNROLL
            for (size_t ii = i; ii < i + width; ++ii)
            {
                matrix_transpose_block_two<group, T, 1>(out, in, ii * stride + j, j * stride + ii, stride);
            }
        }
    }

    CMT_LOOP_NOUNROLL
    for (; i < n; ++i)
    {
        matrix_transpose_block_one<group, T, 1>(out, in, i * stride + i, stride);
        CMT_LOOP_NOUNROLL
        for (size_t j = i + 1; j < n; ++j)
        {
            matrix_transpose_block_two<group, T, 1>(out, in, i * stride + j, j * stride + i, stride);
        }
    }
#endif
}

template <size_t group = 1, typename T>
void matrix_transpose_any(T* out, const T* in, size_t rows, size_t cols)
{
    // 1. transpose square sub-matrix
    const size_t side = std::min(cols, rows);
    matrix_transpose_square<group>(out, in, side, cols);

    if (cols > rows)
    {
        // 2. copy remaining
        size_t remaining = cols - rows;
        if (in != out)
        {
            for (size_t r = 0; r < rows; ++r)
            {
                builtin_memcpy(out + group * (side + r * cols), in + group * (side + r * cols),
                               group * remaining * sizeof(T));
            }
        }

        // 3. shift rows
        auto* p = ptr_cast<vec<T, group>>(out) + side;
        for (size_t r = 0; r + 1 < rows; ++r)
        {
            std::rotate(p, p + remaining + r * remaining, p + side + remaining + r * remaining);
            p += side;
        }
        // 4. transpose remainder
        matrix_transpose<group>(out + group * side * side, out + group * side * side,
                                shape{ side, remaining });
    }
    else // if (cols < rows)
    {
        // 2. copy remaining
        size_t remaining = rows - cols;
        if (in != out)
        {
            for (size_t r = 0; r < remaining; ++r)
            {
                builtin_memcpy(out + group * ((cols + r) * cols), in + group * ((cols + r) * cols),
                               group * cols * sizeof(T));
            }
        }

        // 3. transpose remainder

        matrix_transpose<group>(out + group * side * side, out + group * side * side,
                                shape{ remaining, cols });

        // 4. shift cols
        auto* p = ptr_cast<vec<T, group>>(out) + side * (cols - 1);
        for (size_t c = cols - 1; c >= 1;)
        {
            --c;
            std::rotate(p, p + side, p + (side + remaining + c * remaining));
            p -= side;
        }
    }
}

template <size_t group = 1, typename T>
KFR_INTRINSIC void matrix_transpose_noop(T* out, const T* in, size_t total)
{
    if (out == in)
        return;
    builtin_memcpy(out, in, total * sizeof(T) * group);
}
} // namespace internal

template <size_t group, typename T, index_t Dims>
void matrix_transpose(T* out, const T* in, shape<Dims> tshape)
{
    if constexpr (Dims <= 1)
    {
        return internal::matrix_transpose_noop<group>(out, in, tshape.product());
    }
    else if constexpr (Dims == 2)
    {
        const index_t rows = tshape[0];
        const index_t cols = tshape[1];
        if (cols == 1 || rows == 1)
        {
            return internal::matrix_transpose_noop<group>(out, in, tshape.product());
        }
        // TODO: special cases for tall or wide matrices
        if (cols == rows)
        {
            if (cols <= 6)
                return internal::matrix_transpose_square_small<group>(out, in, cols);
            return internal::matrix_transpose_square<group>(out, in, cols, cols);
        }
        return internal::matrix_transpose_any<group>(out, in, rows, cols);
    }
    else
    {
        shape<Dims - 1> x = tshape.template slice<0, Dims - 1>();
        index_t xproduct  = x.product();
        index_t y         = tshape.back();
        matrix_transpose<group>(out, in, shape<2>{ xproduct, y });
        for (index_t i = 0; i < y; ++i)
        {
            matrix_transpose<group>(out, out, x);
            out += group * xproduct;
        }
    }
}

template <size_t group, typename T, index_t Dims>
void matrix_transpose(complex<T>* out, const complex<T>* in, shape<Dims> shape)
{
    return matrix_transpose<2 * group>(ptr_cast<T>(out), ptr_cast<T>(in), shape);
}

} // namespace CMT_ARCH_NAME

} // namespace kfr
