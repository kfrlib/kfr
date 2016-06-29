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

#include "../base/function.hpp"
#include "../base/operators.hpp"
#include "../base/vec.hpp"

namespace kfr
{

#define KFR_EXPR_UNARY(fn, op)                                                                               \
    template <typename A1, KFR_ENABLE_IF(is_input_expression<A1>::value)>                                    \
    KFR_INLINE auto operator op(A1&& a1)->decltype(bind_expression(fn(), std::forward<A1>(a1)))              \
    {                                                                                                        \
        return bind_expression(fn(), std::forward<A1>(a1));                                                  \
    }

#define KFR_EXPR_BINARY(fn, op)                                                                              \
    template <typename A1, typename A2, KFR_ENABLE_IF(is_input_expressions<A1, A2>::value)>                  \
    KFR_INLINE auto operator op(A1&& a1, A2&& a2)                                                            \
        ->decltype(bind_expression(fn(), std::forward<A1>(a1), std::forward<A2>(a2)))                        \
    {                                                                                                        \
        return bind_expression(fn(), std::forward<A1>(a1), std::forward<A2>(a2));                            \
    }

KFR_EXPR_UNARY(fn_neg, -)
KFR_EXPR_UNARY(fn_bitwisenot, ~)

KFR_EXPR_BINARY(fn_add, +)
KFR_EXPR_BINARY(fn_sub, -)
KFR_EXPR_BINARY(fn_mul, *)
KFR_EXPR_BINARY(fn_div, /)
KFR_EXPR_BINARY(fn_bitwiseand, &)
KFR_EXPR_BINARY(fn_bitwiseor, |)
KFR_EXPR_BINARY(fn_bitwisexor, ^)
KFR_EXPR_BINARY(fn_shl, <<)
KFR_EXPR_BINARY(fn_shr, >>)

KFR_EXPR_BINARY(fn_equal, ==)
KFR_EXPR_BINARY(fn_notequal, !=)
KFR_EXPR_BINARY(fn_less, <)
KFR_EXPR_BINARY(fn_greater, >)
KFR_EXPR_BINARY(fn_lessorequal, <=)
KFR_EXPR_BINARY(fn_greaterorequal, >=)
}
