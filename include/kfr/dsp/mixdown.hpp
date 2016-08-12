/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "../base.hpp"

namespace kfr
{

template <typename... E>
internal::expression_function<fn_add, E...> mixdown(E&&... e)
{
    return internal::expression_function<fn_add, E...>(fn_add(), std::forward<E>(e)...);
}

namespace internal
{
struct stereo_matrix
{
    template <typename T, size_t N>
    CMT_INLINE vec<vec<T, 2>, N> operator()(const vec<vec<T, 2>, N>& x) const
    {
        return process(x, csizeseq<N>);
    }
    template <typename T, size_t N, size_t... indices>
    CMT_INLINE vec<vec<T, 2>, N> process(const vec<vec<T, 2>, N>& x, csizes_t<indices...>) const
    {
        return vec<vec<T, 2>, N>(hadd(transpose(x[indices] * matrix))...);
    }
    const f64x2x2 matrix;
};
}

constexpr f64x2x2 matrix_sum_diff() { return { f64x2{ 1, 1 }, f64x2{ 1, -1 } }; }
constexpr f64x2x2 matrix_halfsum_halfdiff() { return { f64x2{ 0.5, 0.5 }, f64x2{ 0.5, -0.5 } }; }

template <typename Left, typename Right,
          typename Result = internal::expression_function<
              internal::stereo_matrix, internal::expression_pack<internal::arg<Left>, internal::arg<Right>>>>
Result mixdown_stereo(Left&& left, Right&& right, const f64x2x2& matrix)
{
    return Result(internal::stereo_matrix{ matrix },
                  pack(std::forward<Left>(left), std::forward<Right>(right)));
}
}
