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

#include "../base/memory.hpp"
#include "../base/univector.hpp"
#include "../simd/complex.hpp"
#include "../simd/constants.hpp"
#include "../simd/read_write.hpp"
#include "../simd/vec.hpp"
#include <cmath>
#include <vector>

namespace kfr
{

namespace internal_generic
{

template <typename T>
void reference_dft_po2_pass(size_t N, int flag, const complex<T>* in, complex<T>* out, complex<T>* scratch,
                            size_t in_delta = 1, size_t out_delta = 1, size_t scratch_delta = 1)
{
    const T pi2        = c_pi<T, 2, 1>;
    const size_t N2    = N / 2;
    const complex<T> w = pi2 * complex<T>{ 0, -T(flag) };

    if (N != 2)
    {
        reference_dft_po2_pass(N2, flag, in, scratch, out, 2 * in_delta, 2 * scratch_delta, 2 * out_delta);
        reference_dft_po2_pass(N2, flag, in + in_delta, scratch + scratch_delta, out + out_delta,
                               2 * in_delta, 2 * scratch_delta, 2 * out_delta);

        for (size_t k = 0; k < N2; k++)
        {
            const T m                 = static_cast<T>(k) / N;
            const complex<T> tw       = std::exp(w * m);
            const complex<T> tmp      = scratch[(2 * k + 1) * scratch_delta] * tw;
            out[(k + N2) * out_delta] = scratch[(2 * k) * scratch_delta] - tmp;
            out[(k)*out_delta]        = scratch[(2 * k) * scratch_delta] + tmp;
        }
    }
    else
    {
        out[out_delta] = in[0] - in[in_delta];
        out[0]         = in[0] + in[in_delta];
    }
}

template <typename T>
void reference_dft_po2(complex<T>* out, const complex<T>* in, size_t size, bool inversion,
                       size_t out_delta = 1, size_t in_delta = 1)
{
    if (size < 1)
        return;
    if (size == 1)
    {
        out[0] = in[0];
        return;
    }
    std::vector<complex<T>> temp(size);
    reference_dft_po2_pass(size, inversion ? -1 : +1, in, out, temp.data(), in_delta, out_delta, 1);
}

/// @brief Performs Complex FFT using reference implementation (slow, used for testing)
template <typename T>
void reference_dft_nonpo2(complex<T>* out, const complex<T>* in, size_t size, bool inversion,
                          size_t out_delta = 1, size_t in_delta = 1)
{
    constexpr T pi2    = c_pi<T, 2>;
    const complex<T> w = pi2 * complex<T>{ 0, T(inversion ? +1 : -1) };
    if (size < 2)
        return;
    {
        complex<T> sum = 0;
        for (size_t j = 0; j < size; j++)
            sum += in[j * in_delta];
        out[0] = sum;
    }
    for (size_t i = 1; i < size; i++)
    {
        complex<T> sum = in[0];
        for (size_t j = 1; j < size; j++)
        {
            complex<T> tw = std::exp(w * (static_cast<T>(i) * j / size));
            sum += tw * in[j * in_delta];
        }
        out[i * out_delta] = sum;
    }
}
} // namespace internal_generic

/// @brief Performs Complex DFT using reference implementation (slow, used for testing)
template <typename T>
void reference_dft(complex<T>* out, const complex<T>* in, size_t size, bool inversion = false,
                   size_t out_delta = 1, size_t in_delta = 1)
{
    if (in == out)
    {
        std::vector<complex<T>> tmpin(size);
        for (int i = 0; i < size; ++i)
            tmpin[i] = in[i * in_delta];
        return reference_dft(out, tmpin.data(), size, inversion, out_delta, 1);
    }
    if (is_poweroftwo(size))
    {
        return internal_generic::reference_dft_po2(out, in, size, inversion, out_delta, in_delta);
    }
    else
    {
        return internal_generic::reference_dft_nonpo2(out, in, size, inversion, out_delta, in_delta);
    }
}

/// @brief Performs Direct Real DFT using reference implementation (slow, used for testing)
template <typename T>
void reference_dft(complex<T>* out, const T* in, size_t size, size_t out_delta = 1, size_t in_delta = 1)
{
    if (size < 1)
        return;
    std::vector<complex<T>> tmpin(size);
    for (index_t i = 0; i < size; ++i)
        tmpin[i] = in[i * in_delta];
    std::vector<complex<T>> tmpout(size);
    reference_dft(tmpout.data(), tmpin.data(), size, false, 1, 1);
    for (index_t i = 0; i < size / 2 + 1; i++)
        out[i * out_delta] = tmpout[i];
}

/// @brief Performs Multidimensional Complex DFT using reference implementation (slow, used for testing)
template <typename T>
void reference_dft_md(complex<T>* out, const complex<T>* in, shape<dynamic_shape> size,
                      bool inversion = false, size_t out_delta = 1, size_t in_delta = 1)
{
    index_t total = size.product();
    if (total < 1)
        return;
    if (total == 1)
    {
        out[0] = in[0];
        return;
    }
    index_t inner = 1;
    index_t outer = total;
    for (int axis = size.dims() - 1; axis >= 0; --axis)
    {
        index_t d = size[axis];
        outer /= d;
        for (index_t o = 0; o < outer; ++o)
        {
            for (index_t i = 0; i < inner; ++i)
            {
                reference_dft(out + (i + o * inner * d) * out_delta, in + (i + o * inner * d) * in_delta, d,
                              inversion, out_delta * inner, in_delta * inner);
            }
        }
        in       = out;
        in_delta = out_delta;
        inner *= d;
    }
}

/// @brief Performs Multidimensional Direct Real DFT using reference implementation (slow, used for testing)
template <typename T>
void reference_dft_md(complex<T>* out, const T* in, shape<dynamic_shape> shape, bool inversion = false,
                      size_t out_delta = 1, size_t in_delta = 1)
{
    index_t size = shape.product();
    if (size < 1)
        return;
    std::vector<complex<T>> tmpin(size);
    for (index_t i = 0; i < size; ++i)
        tmpin[i] = in[i * in_delta];
    std::vector<complex<T>> tmpout(size);
    reference_dft_md(tmpout.data(), tmpin.data(), shape, inversion, 1, 1);
    index_t last = shape.back() / 2 + 1;
    for (index_t i = 0; i < std::max(index_t(1), shape.remove_back().product()); ++i)
        for (index_t j = 0; j < last; j++)
            out[(i * last + j) * out_delta] = tmpout[i * shape.back() + j];
}

} // namespace kfr
