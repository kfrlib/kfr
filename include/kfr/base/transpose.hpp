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

namespace internal
{

template <typename T, size_t N, index_t Dims>
void matrix_transpose(vec<T, N>* out, const vec<T, N>* in, shape<Dims> tshape);

template <typename T, size_t width, size_t N>
void matrix_transpose_block_one(vec<T, N>* out, const vec<T, N>* in, size_t i, size_t stride)
{
    if constexpr (width == 1)
    {
        write(ptr_cast<T>(out + i), kfr::read<N>(ptr_cast<T>(in + i)));
    }
    else
    {
        vec<T, (N * width * width)> vi = read_group<width, width, N>(ptr_cast<T>(in + i), stride);
        vi                             = transpose<width, N>(vi);
        write_group<width, width, N>(ptr_cast<T>(out + i), stride, vi);
    }
}

template <typename T, size_t width, size_t N>
void matrix_transpose_block_two(vec<T, N>* out, const vec<T, N>* in, size_t i, size_t j, size_t stride)
{
    if constexpr (width == 1)
    {
        vec<T, N> vi = kfr::read<N>(ptr_cast<T>(in + i));
        vec<T, N> vj = kfr::read<N>(ptr_cast<T>(in + j));
        write(ptr_cast<T>(out + i), vj);
        write(ptr_cast<T>(out + j), vi);
    }
    else
    {
        vec<T, (N * width * width)> vi = read_group<width, width, N>(ptr_cast<T>(in + i), stride);
        vec<T, (N * width * width)> vj = read_group<width, width, N>(ptr_cast<T>(in + j), stride);
        vi                             = transpose<width, N>(vi);
        vj                             = transpose<width, N>(vj);
        write_group<width, width, N>(ptr_cast<T>(out + i), stride, vj);
        write_group<width, width, N>(ptr_cast<T>(out + j), stride, vi);
    }
}

template <typename T, size_t N>
void matrix_transpose_square_small(vec<T, N>* out, const vec<T, N>* in, size_t n)
{
    cswitch(csizeseq<6, 1>, n, // 1, 2, 3, 4, 5 or 6
            [&](auto n_) CMT_INLINE_LAMBDA
            {
                constexpr size_t n = CMT_CVAL(n_);
                write(ptr_cast<T>(out), transpose<n, N>(kfr::read<n * n * N>(ptr_cast<T>(in))));
            });
}

template <typename T, size_t N>
void matrix_transpose_square(vec<T, N>* out, const vec<T, N>* in, size_t n, size_t stride)
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
        matrix_transpose_block_one<T, width>(out, in, istridei, stride);

        size_t j        = i + width;
        size_t istridej = istridei + width;
        size_t jstridei = istridei + wstride;
        CMT_LOOP_NOUNROLL
        for (; j < nw; j += width)
        {
            matrix_transpose_block_two<T, width>(out, in, istridej, jstridei, stride);
            istridej += width;
            jstridei += wstride;
        }
        CMT_LOOP_NOUNROLL
        for (; j < n; ++j)
        {
            CMT_LOOP_NOUNROLL
            for (size_t ii = i; ii < i + width; ++ii)
            {
                matrix_transpose_block_two<T, 1>(out, in, istridej, jstridei, stride);
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
        matrix_transpose_block_one<T, 1>(out, in, i * stride + i, stride);
        CMT_LOOP_NOUNROLL
        for (size_t j = i + 1; j < n; ++j)
        {
            matrix_transpose_block_two<T, 1>(out, in, i * stride + j, j * stride + i, stride);
        }
    }
#else
    constexpr size_t width = 4;
    const size_t nw        = align_down(n, width);

    size_t i = 0;
    CMT_LOOP_NOUNROLL
    for (; i < nw; i += width)
    {
        matrix_transpose_block_one<T, width>(out, in, i * stride + i, stride);

        size_t j = i + width;
        CMT_LOOP_NOUNROLL
        for (; j < nw; j += width)
        {
            matrix_transpose_block_two<T, width>(out, in, i * stride + j, j * stride + i, stride);
        }
        CMT_LOOP_NOUNROLL
        for (; j < n; ++j)
        {
            CMT_LOOP_NOUNROLL
            for (size_t ii = i; ii < i + width; ++ii)
            {
                matrix_transpose_block_two<T, 1>(out, in, ii * stride + j, j * stride + ii, stride);
            }
        }
    }

    CMT_LOOP_NOUNROLL
    for (; i < n; ++i)
    {
        matrix_transpose_block_one<T, 1>(out, in, i * stride + i, stride);
        CMT_LOOP_NOUNROLL
        for (size_t j = i + 1; j < n; ++j)
        {
            matrix_transpose_block_two<T, 1>(out, in, i * stride + j, j * stride + i, stride);
        }
    }
#endif
}

template <typename T, size_t N>
CMT_ALWAYS_INLINE void do_reverse(vec<T, N>* first, vec<T, N>* last)
{
    constexpr size_t width = vector_capacity<T> / 4 / N;
    for (; first + width - 1 < last - width; first += width, last -= width)
    {
        vec<T, (N * width)> a = read<N * width>(first);
        vec<T, (N * width)> b = read<N * width>(last - width);
        write(first, reverse<N>(b));
        write(last - width, reverse<N>(a));
    }
    for (; first < last; first += 1, last -= 1)
    {
        vec<T, N> a = read<N>(ptr_cast<T>(first));
        vec<T, N> b = read<N>(ptr_cast<T>(last - 1));
        write(ptr_cast<T>(first), b);
        write(ptr_cast<T>(last - 1), a);
    }
}

template <typename T, size_t N>
CMT_ALWAYS_INLINE void ranges_swap(vec<T, N>* x, vec<T, N>* y, size_t size)
{
    block_process(size, csizes<const_max(vector_capacity<T> / 4 / N, 2), 1>,
                  [x, y](size_t index, auto w) CMT_INLINE_LAMBDA
                  {
                      constexpr size_t width = CMT_CVAL(w);
                      vec<T, N* width> xx    = read<N * width>(ptr_cast<T>(x + index));
                      vec<T, N* width> yy    = read<N * width>(ptr_cast<T>(y + index));
                      write(ptr_cast<T>(x + index), yy);
                      write(ptr_cast<T>(y + index), xx);
                  });
}

template <typename T>
CMT_ALWAYS_INLINE void do_swap(T* arr, size_t a, size_t b, size_t k)
{
    ranges_swap(arr + a, arr + b, k);
}
template <typename T>
CMT_ALWAYS_INLINE void do_block_swap(T* arr, size_t k, size_t n)
{
    if (k == 0 || k == n)
        return;

    for (;;)
    {
        if (k == n - k)
        {
            do_swap(arr, 0, n - k, k);
            return;
        }
        else if (k < n - k)
        {
            do_swap(arr, 0, n - k, k);
            n = n - k;
        }
        else
        {
            do_swap(arr, 0, k, n - k);
            arr += n - k;
            const size_t newk = 2 * k - n;
            n                 = k;
            k                 = newk;
        }
    }
}

template <typename T, size_t N>
CMT_ALWAYS_INLINE void range_rotate(vec<T, N>* first, vec<T, N>* middle, vec<T, N>* last)
{
#ifndef KFR_T_REV
    do_block_swap(first, middle - first, last - first);
#else
    do_reverse<group>(first, middle);
    do_reverse<group>(middle, last);
    do_reverse<group>(first, last);
#endif
}

struct matrix_size
{
    size_t rows;
    size_t cols;
};

template <typename T, size_t N>
void matrix_transpose_copy(vec<T, N>* out, const vec<T, N>* in, matrix_size size, matrix_size done)
{
    if (size.cols != done.cols)
    {
        for (size_t r = 0; r < size.rows; ++r)
            builtin_memcpy(out + r * size.cols + done.cols, //
                           in + r * size.cols + done.cols, //
                           (size.cols - done.cols) * N * sizeof(T));
    }

    for (size_t r = done.rows; r < size.rows; ++r)
        builtin_memcpy(out + r * size.cols, //
                       in + r * size.cols, //
                       (size.cols) * N * sizeof(T));
}

template <typename T, size_t N>
void matrix_transpose_shift_rows(vec<T, N>* out, size_t done, matrix_size size)
{
    const size_t remaining = size.cols - done;
    vec<T, N>* p           = out + done;
    for (size_t r = 1; r < size.rows; ++r)
    {
        range_rotate(p, p + r * remaining, p + done + r * remaining);
        p += done;
    }
}

template <typename T, size_t N>
void matrix_transpose_shift_cols(vec<T, N>* out, size_t done, matrix_size size)
{
    const size_t remaining = size.rows - done;
    vec<T, N>* p           = out + done * (size.cols - 1);
    for (size_t c = size.cols - 1; c >= 1; --c)
    {
        range_rotate(p, p + done, p + done + c * remaining);
        p -= done;
    }
}

class matrix_cycles
{
public:
    matrix_cycles(const matrix_cycles&)            = delete;
    matrix_cycles(matrix_cycles&&)                 = delete;
    matrix_cycles& operator=(const matrix_cycles&) = delete;
    matrix_cycles& operator=(matrix_cycles&&)      = delete;

    CMT_INLINE_MEMBER explicit matrix_cycles(shape<2> size) : size(size), flat_size(size.product())
    {
        size_t bits  = (flat_size + 1) / 2;
        size_t words = (bits + word_bits - 1) / word_bits;
        if (words <= std::size(on_stack))
            data = on_stack;
        else
            data = new word_t[words];
        builtin_memset(data, 0, sizeof(word_t) * words);
    }

    ~matrix_cycles()
    {
        if (data != on_stack)
            delete data;
    }

    size_t next_cycle_origin(size_t origin = 0)
    {
        for (; origin < (flat_size + 1) / 2; ++origin)
        {
            if (!test_and_set(origin))
                return origin;
        }
        return static_cast<size_t>(-1);
    }

    template <bool first_pass = true, typename Start, typename Iterate, typename Stop>
    void iterate(size_t origin, Start&& start, Iterate&& iterate, Stop&& stop, bool skip_fixed = false)
    {
        shape<2> transposed_size = size.transpose();
        size_t next              = transposed_size.to_flat(size.from_flat(origin).transpose());
        if (next == origin)
        {
            bool is_fixed = next != flat_size - 1 - next;
            if (!(is_fixed && skip_fixed))
            {
                start(origin, flat_size - 1 - origin, is_fixed);
                stop(next != flat_size - 1 - next);
            }
        }
        else
        {
            size_t inv_next = flat_size - 1 - next;
            size_t min_next = std::min(next, inv_next);
            if (min_next == origin)
            {
                bool is_fixed = next == origin;
                if (!(is_fixed && skip_fixed))
                {
                    start(origin, flat_size - 1 - origin, next == origin);
                    stop(next == origin);
                }
            }
            else
            {
                start(origin, flat_size - 1 - origin, false);
                for (;;)
                {
                    if constexpr (first_pass)
                    {
                        set(min_next);
                    }
                    iterate(next, inv_next);
                    next     = transposed_size.to_flat(size.from_flat(next).transpose());
                    inv_next = flat_size - 1 - next;
                    min_next = std::min(next, inv_next);
                    if (min_next == origin)
                    {
                        stop(next == origin);
                        break;
                    }
                }
            }
        }
    }

private:
    using word_t                      = uint32_t;
    constexpr static size_t word_bits = sizeof(word_t) * 8;
    shape<2> size;
    size_t flat_size;
    word_t* data;
CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wattributes")
    [[maybe_unused]] uint8_t cache_line__[64];
CMT_PRAGMA_GNU(GCC diagnostic pop)    
    alignas(16) word_t on_stack[1024];

    CMT_INLINE_MEMBER void set(size_t index)
    {
        word_t& word = data[index / word_bits];
        word_t mask  = 1u << (index % word_bits);
        word |= mask;
    }

    CMT_INLINE_MEMBER bool test_and_set(size_t index)
    {
        word_t& word = data[index / word_bits];
        word_t mask  = 1u << (index % word_bits);
        if (word & mask)
            return true;
        word |= mask;
        return false;
    }
};

template <typename T, size_t N, bool horizontal>
CMT_INTRINSIC void matrix_merge_squares_fast(vec<T, N>* out, size_t side, size_t squares, matrix_size size,
                                             size_t stride, cbool_t<horizontal>)
{
    if constexpr (!horizontal)
    {
        stride = stride * side;
    }
    for (size_t i = 0; i < side; ++i)
    {
        for (size_t j = i + 1; j < side; ++j)
        {
            size_t index1 = i * stride + j * side;
            size_t index2 = j * stride + i * side;
            ranges_swap(out + index1, out + index2, side);
        }
    }
}

static CMT_INTRINSIC size_t matrix_offset(size_t flat_index, size_t side, size_t stride1, size_t stride2)
{
    size_t i = flat_index / side;
    size_t j = flat_index % side;
    return i * stride1 + j * stride2;
}

template <typename T, size_t N, bool horizontal>
void matrix_merge_squares(vec<T, N>* out, size_t side, size_t squares, matrix_size size, size_t stride,
                          cbool_t<horizontal>)
{
    if (side == squares)
    {
        return matrix_merge_squares_fast(out, side, squares, size, stride, cbool<horizontal>);
    }
    if constexpr (!horizontal)
    {
        stride = stride * side;
    }
    shape sh         = horizontal ? shape{ squares, side } : shape{ side, squares };
    size_t flat_side = sh[0];
    matrix_cycles cycles(sh);

    size_t origin = 0;
    do
    {
        block_process(
            side, csizes<const_max(2, vector_capacity<T> / 8 / N), 1>,
            [&](size_t offset, auto width_)
            {
                constexpr size_t width = CMT_CVAL(width_);

                vec<T, width * N> temp;
                vec<T, width * N> temp_inv;
                size_t previous;
                size_t previous_inv;
                cycles.iterate(
                    origin,
                    [&](size_t origin, size_t origin_inv, bool /* fixed */) CMT_INLINE_LAMBDA
                    {
#ifdef CMT_COMPILER_IS_MSVC
                        constexpr size_t width = CMT_CVAL(width_);
#endif
                        temp = read<width * N>(
                            ptr_cast<T>(out + matrix_offset(origin, flat_side, stride, side) + offset));
                        temp_inv = read<width * N>(
                            ptr_cast<T>(out + matrix_offset(origin_inv, flat_side, stride, side) + offset));
                        previous     = origin;
                        previous_inv = origin_inv;
                    },
                    [&](size_t current, size_t current_inv) CMT_INLINE_LAMBDA
                    {
#ifdef CMT_COMPILER_IS_MSVC
                        constexpr size_t width = CMT_CVAL(width_);
#endif
                        vec<T, (width * N)> val = read<width * N>(
                            ptr_cast<T>(out + matrix_offset(current, flat_side, stride, side) + offset));
                        vec<T, (width * N)> val_inv = read<width * N>(
                            ptr_cast<T>(out + matrix_offset(current_inv, flat_side, stride, side) + offset));
                        write(ptr_cast<T>(out + matrix_offset(previous, flat_side, stride, side) + offset),
                              val);
                        write(
                            ptr_cast<T>(out + matrix_offset(previous_inv, flat_side, stride, side) + offset),
                            val_inv);
                        previous     = current;
                        previous_inv = current_inv;
                    },
                    [&](bool symmetric) CMT_INLINE_LAMBDA
                    {
                        if (!symmetric)
                            std::swap(temp, temp_inv);
                        write(ptr_cast<T>(out + matrix_offset(previous, flat_side, stride, side) + offset),
                              temp);
                        write(
                            ptr_cast<T>(out + matrix_offset(previous_inv, flat_side, stride, side) + offset),
                            temp_inv);
                    },
                    true);
            });
        origin = cycles.next_cycle_origin(origin + 1);
    } while (origin != static_cast<size_t>(-1));
}

template <typename T, size_t N>
void matrix_transpose_any(vec<T, N>* out, const vec<T, N>* in, matrix_size size)
{
    if (size.cols > size.rows)
    {
        // 1. transpose square sub-matrices
        const size_t side    = size.rows;
        const size_t squares = size.cols / side;
        for (size_t i = 0; i < squares; ++i)
        {
            matrix_transpose_square(out + i * side, in + i * side, side, size.cols);
        }
        if (squares > 1)
            matrix_merge_squares(out, side, squares, size, size.cols, ctrue);
        const size_t done = side * squares;
        if (in != out)
            matrix_transpose_copy(out, in, size, { side, done });

        const size_t remaining = size.cols - done;
        if (remaining == 0)
            return;

        // 2. shift rows
        matrix_transpose_shift_rows(out, done, size);

        // 3. transpose remainder
        internal::matrix_transpose(out + done * size.rows, out + done * size.rows,
                                   shape{ size.rows, remaining });
    }
    else // if (cols < rows)
    {
        // 1. transpose square sub-matrices
        const size_t side    = size.cols;
        const size_t squares = size.rows / side;
        for (size_t i = 0; i < squares; ++i)
        {
            matrix_transpose_square(out + i * side * side, in + i * side * side, side, size.cols);
        }
        if (squares > 1)
            matrix_merge_squares(out, side, squares, size, size.cols, cfalse);
        const size_t done = side * squares;
        if (in != out)
            matrix_transpose_copy(out, in, size, { done, side });

        const size_t remaining = size.rows - done;
        if (remaining == 0)
            return;

        // 2. transpose remainder
        internal::matrix_transpose(out + done * size.cols, out + done * size.cols,
                                   shape{ remaining, size.cols });

        // 3. shift cols
        matrix_transpose_shift_cols(out, done, size);
    }
}

template <typename T>
KFR_INTRINSIC void matrix_transpose_noop(T* out, const T* in, size_t total)
{
    if (out == in)
        return;
    builtin_memcpy(out, in, total * sizeof(T));
}

template <typename T, size_t N, index_t Dims>
void matrix_transpose(vec<T, N>* out, const vec<T, N>* in, shape<Dims> tshape)
{
    if constexpr (Dims <= 1)
    {
        return internal::matrix_transpose_noop(out, in, tshape.product());
    }
    else if constexpr (Dims == 2)
    {
        const index_t rows = tshape[0];
        const index_t cols = tshape[1];
        if (cols == 1 || rows == 1)
        {
            return internal::matrix_transpose_noop(out, in, tshape.product());
        }
        // TODO: special cases for tall or wide matrices
        if (cols == rows)
        {
            if (cols <= 6)
                return internal::matrix_transpose_square_small(out, in, cols);
            return internal::matrix_transpose_square(out, in, cols, cols);
        }
        return internal::matrix_transpose_any(out, in, { rows, cols });
    }
    else
    {
        shape<Dims - 1> x = tshape.template slice<0, Dims - 1>();
        index_t xproduct  = x.product();
        index_t y         = tshape.back();
        internal::matrix_transpose(out, in, shape<2>{ xproduct, y });
        for (index_t i = 0; i < y; ++i)
        {
            internal::matrix_transpose(out, out, x);
            out += xproduct;
        }
    }
}
} // namespace internal

/// @brief Matrix transpose.
/// Accepts vec, complex and other compound types
template <typename T, index_t Dims>
void matrix_transpose(T* out, const T* in, shape<Dims> shape)
{
    using U                = typename compound_type_traits<T>::deep_subtype;
    constexpr size_t width = compound_type_traits<T>::deep_width;
    return internal::matrix_transpose<U, width, Dims>(ptr_cast<vec<U, width>>(out),
                                                      ptr_cast<vec<U, width>>(in), shape);
}

} // namespace CMT_ARCH_NAME

} // namespace kfr
