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
#include "abs.hpp"
#include "constants.hpp"
#include "function.hpp"
#include "log_exp.hpp"
#include "min_max.hpp"
#include "operators.hpp"
#include "select.hpp"

namespace kfr
{

namespace internal
{

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> sinh(vec<T, N> x)
{
    return (exp(x) - exp(-x)) * T(0.5);
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> cosh(vec<T, N> x)
{
    return (exp(x) + exp(-x)) * T(0.5);
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> tanh(vec<T, N> x)
{
    x = -2 * x;
    return (1 - exp(x)) / (1 + exp(x));
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> coth(vec<T, N> x)
{
    x = -2 * x;
    return (1 + exp(x)) / (1 - exp(x));
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1)>
KFR_SINTRIN vec<T, N> sinhcosh(vec<T, N> x)
{
    const vec<T, N> a = exp(x);
    const vec<T, N> b = exp(-x);
    return subadd(a, b) * T(0.5);
}

template <typename T, size_t N, KFR_ENABLE_IF(N > 1)>
KFR_SINTRIN vec<T, N> coshsinh(vec<T, N> x)
{
    const vec<T, N> a = exp(x);
    const vec<T, N> b = exp(-x);
    return addsub(a, b) * T(0.5);
}

KFR_I_CONVERTER(sinh)
KFR_I_CONVERTER(cosh)
KFR_I_CONVERTER(tanh)
KFR_I_CONVERTER(coth)
KFR_I_CONVERTER(sinhcosh)
KFR_I_CONVERTER(coshsinh)
KFR_I_FN(sinh)
KFR_I_FN(cosh)
KFR_I_FN(tanh)
KFR_I_FN(coth)
KFR_I_FN(sinhcosh)
KFR_I_FN(coshsinh)
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 sinh(const T1& x)
{
    return internal::sinh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_sinh, E1> sinh(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cosh(const T1& x)
{
    return internal::cosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_cosh, E1> cosh(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 tanh(const T1& x)
{
    return internal::tanh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_tanh, E1> tanh(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 coth(const T1& x)
{
    return internal::coth(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_coth, E1> coth(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 sinhcosh(const T1& x)
{
    return internal::sinhcosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_sinhcosh, E1> sinhcosh(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 coshsinh(const T1& x)
{
    return internal::coshsinh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<internal::fn_coshsinh, E1> coshsinh(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}
