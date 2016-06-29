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

template <cpu_t c = cpu_t::native>
struct in_sinh_cosh : in_log_exp<c>
{
    constexpr static cpu_t cur = c;

private:
    using in_log_exp<c>::exp;

public:
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
    KFR_SPEC_FN(in_sinh_cosh, sinh)
    KFR_SPEC_FN(in_sinh_cosh, cosh)
    KFR_SPEC_FN(in_sinh_cosh, sinhcosh)
    KFR_SPEC_FN(in_sinh_cosh, coshsinh)
};
}

namespace native
{
using fn_sinh = internal::in_sinh_cosh<>::fn_sinh;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>

KFR_INTRIN ftype<T1> sinh(const T1& x)
{
    return internal::in_sinh_cosh<>::sinh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>

KFR_INTRIN expr_func<fn_sinh, E1> sinh(E1&& x)
{
    return { fn_sinh(), std::forward<E1>(x) };
}

using fn_cosh = internal::in_sinh_cosh<>::fn_cosh;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>

KFR_INTRIN ftype<T1> cosh(const T1& x)
{
    return internal::in_sinh_cosh<>::cosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>

KFR_INTRIN expr_func<fn_cosh, E1> cosh(E1&& x)
{
    return { fn_cosh(), std::forward<E1>(x) };
}

using fn_sinhcosh = internal::in_sinh_cosh<>::fn_sinhcosh;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>

KFR_INTRIN ftype<T1> sinhcosh(const T1& x)
{
    return internal::in_sinh_cosh<>::sinhcosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>

KFR_INTRIN expr_func<fn_sinhcosh, E1> sinhcosh(E1&& x)
{
    return { fn_sinhcosh(), std::forward<E1>(x) };
}

using fn_coshsinh = internal::in_sinh_cosh<>::fn_coshsinh;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>

KFR_INTRIN ftype<T1> coshsinh(const T1& x)
{
    return internal::in_sinh_cosh<>::coshsinh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>

KFR_INTRIN expr_func<fn_coshsinh, E1> coshsinh(E1&& x)
{
    return { fn_coshsinh(), std::forward<E1>(x) };
}
}
}
