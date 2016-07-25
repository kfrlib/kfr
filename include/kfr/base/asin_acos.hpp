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

#include "atan.hpp"
#include "function.hpp"
#include "select.hpp"
#include "sqrt.hpp"

namespace kfr
{

namespace internal
{

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> asin(vec<T, N> x)
{
    return atan2(x, sqrt(T(1) - x * x));
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> acos(vec<T, N> x)
{
    return atan2(sqrt(T(1) - x * x), x);
}
KFR_HANDLE_SCALAR(asin)
KFR_HANDLE_SCALAR(acos)
KFR_FN(asin)
KFR_FN(acos)
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 asin(const T1& x)
{
    return internal::asin(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_asin, E1> asin(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 acos(const T1& x)
{
    return internal::acos(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_acos, E1> acos(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}
