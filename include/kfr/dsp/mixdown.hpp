/** @addtogroup dsp_extra
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

#include "../base.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

/**
 * @brief Returns template expression that returns the sum of all the inputs
 */
template <typename... E>
expression_function<fn::add, E...> mixdown(E&&... e)
{
    return expression_function<fn::add, E...>(fn::add(), std::forward<E>(e)...);
}

struct stereo_matrix
{
    template <typename T, size_t N>
    KFR_MEM_INTRINSIC vec<vec<T, 2>, N> operator()(const vec<vec<T, 2>, N>& x) const
    {
        return process(x, csizeseq<N>);
    }
    template <typename T, size_t N, size_t... indices>
    KFR_MEM_INTRINSIC vec<vec<T, 2>, N> process(const vec<vec<T, 2>, N>& x, csizes_t<indices...>) const
    {
        return vec<vec<T, 2>, N>(hadd(transpose(x[indices] * matrix))...);
    }
    const f64x2x2 matrix;
};

template <int = 0>
CMT_GNU_CONSTEXPR f64x2x2 matrix_sum_diff()
{
    return { f64x2{ 1, 1 }, f64x2{ 1, -1 } };
}
template <int = 0>
CMT_GNU_CONSTEXPR f64x2x2 matrix_halfsum_halfdiff()
{
    return { f64x2{ 0.5, 0.5 }, f64x2{ 0.5, -0.5 } };
}

/**
 * @brief Returns template expression that returns the vector of length 2 containing mix of the left and right
 * channels
 */
template <typename Left, typename Right,
          typename Result = expression_function<stereo_matrix, expression_pack<Left, Right>>>
Result mixdown_stereo(Left&& left, Right&& right, const f64x2x2& matrix)
{
    return Result(stereo_matrix{ matrix }, pack(std::forward<Left>(left), std::forward<Right>(right)));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
