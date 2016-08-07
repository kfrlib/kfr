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

#include "../base/basic_expressions.hpp"
#include "../base/function.hpp"
#include "../base/operators.hpp"
#include "../base/vec.hpp"

namespace kfr
{
namespace internal
{
template <typename From, typename E>
struct expression_convert : expression<E>
{
    CMT_INLINE expression_convert(E&& expr) noexcept : expression<E>(std::forward<E>(expr)) {}

    template <typename T, size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        return this->argument_first(index, vec_t<From, N>());
    }
};
}

template <typename From, typename E>
CMT_INLINE internal::expression_convert<From, decay<E>> convert(E&& expr)
{
    return internal::expression_convert<From, decay<E>>(std::forward<E>(expr));
}
}
