/** @addtogroup dsp_extra
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

#include "../base.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Returns a template expression that yields the sample-wise sum of all
 *        the inputs.
 *
 * @tparam E Input expression types.
 * @param e  Input expressions to be added together.
 * @return An `expression_function<fn::add, E...>` producing the sum.
 */
template <typename... E>
expression_function<fn::add, E...> mixdown(E&&... e)
{
    return expression_function<fn::add, E...>(fn::add(), std::forward<E>(e)...);
}

/**
 * @brief 2x2 mixing matrix functor for stereo channel transformations.
 *
 * Applies a 2x2 matrix to packed stereo samples (`vec<vec<T,2>, N>`) and
 * returns the resulting stereo samples. Used by `mixdown_stereo` to combine
 * left/right channels into a pair of output channels (e.g. mid/side).
 */
struct stereo_matrix
{
    /**
     * @brief Apply the matrix to a vector of stereo samples.
     * @tparam T Sample type.
     * @tparam N SIMD vector width.
     * @param x  Vector of `N` stereo samples.
     * @return Vector of `N` transformed stereo samples.
     */
    template <typename T, size_t N>
    KFR_MEM_INTRINSIC vec<vec<T, 2>, N> operator()(const vec<vec<T, 2>, N>& x) const
    {
        return process(x, csizeseq<N>);
    }
    /**
     * @brief Internal ADL-provided implementation for `stereo_matrix` expressions.
     */
    template <typename T, size_t N, size_t... indices>
    KFR_MEM_INTRINSIC vec<vec<T, 2>, N> process(const vec<vec<T, 2>, N>& x, csizes_t<indices...>) const
    {
        return vec<vec<T, 2>, N>(hadd(transpose(x[indices] * matrix))...);
    }
    const f64x2x2 matrix; ///< 2x2 mixing matrix (rows are output coefficients).
};

/**
 * @brief Returns a 2x2 matrix that computes sum/difference of stereo channels.
 *
 * Output row 0 = L + R, output row 1 = L - R.
 * @return `{ {1, 1}, {1, -1} }`.
 */
template <int = 0>
f64x2x2 matrix_sum_diff()
{
    return { f64x2{ 1, 1 }, f64x2{ 1, -1 } };
}
/**
 * @brief Returns a 2x2 matrix that computes half-sum/half-difference of
 *        stereo channels.
 *
 * Output row 0 = (L + R) / 2, output row 1 = (L - R) / 2.
 * @return `{ {0.5, 0.5}, {0.5, -0.5} }`.
 */
template <int = 0>
f64x2x2 matrix_halfsum_halfdiff()
{
    return { f64x2{ 0.5, 0.5 }, f64x2{ 0.5, -0.5 } };
}

/**
 * @brief Returns a template expression that produces a length-2 vector mixing
 *        the left and right channels according to `matrix`.
 *
 * The two output channels are computed as
 * `out0 = matrix[0][0]*L + matrix[0][1]*R` and
 * `out1 = matrix[1][0]*L + matrix[1][1]*R`.
 * For example, with `matrix_sum_diff()` the outputs are the mid (L+R) and
 * side (L-R) channels.
 *
 * @tparam Left  Left-channel expression type.
 * @tparam Right Right-channel expression type.
 * @param left   Left-channel input expression.
 * @param right  Right-channel input expression.
 * @param matrix 2x2 mixing matrix (e.g. from `matrix_sum_diff()`).
 * @return An expression yielding vectors of 2 samples per index.
 */
template <typename Left, typename Right,
          typename Result = expression_function<stereo_matrix, expression_pack<Left, Right>>>
Result mixdown_stereo(Left&& left, Right&& right, const f64x2x2& matrix)
{
    return Result(stereo_matrix{ matrix }, pack(std::forward<Left>(left), std::forward<Right>(right)));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
