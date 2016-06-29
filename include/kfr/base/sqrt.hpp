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

#include "function.hpp"

namespace kfr
{

namespace internal
{

template <cpu_t c = cpu_t::native>
struct in_sqrt : in_sqrt<older(c)>
{
    struct fn_sqrt : fn_disabled
    {
    };
};

template <>
struct in_sqrt<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse2;

    KFR_SINTRIN f32sse sqrt(f32sse x) { return _mm_sqrt_ps(*x); }
    KFR_SINTRIN f64sse sqrt(f64sse x) { return _mm_sqrt_pd(*x); }

    KFR_HANDLE_ALL(sqrt)
    KFR_HANDLE_SCALAR(sqrt)
    KFR_SPEC_FN(in_sqrt, sqrt)
};

template <>
struct in_sqrt<cpu_t::avx1> : in_sqrt<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::avx1;
    using in_sqrt<cpu_t::sse2>::sqrt;

    KFR_SINTRIN f32avx KFR_USE_CPU(avx) sqrt(f32avx x) { return _mm256_sqrt_ps(*x); }
    KFR_SINTRIN f64avx KFR_USE_CPU(avx) sqrt(f64avx x) { return _mm256_sqrt_pd(*x); }

    KFR_HANDLE_ALL(sqrt)
    KFR_HANDLE_SCALAR(sqrt)
    KFR_SPEC_FN(in_sqrt, sqrt)
};
}
namespace native
{
using fn_sqrt = internal::in_sqrt<>::fn_sqrt;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>

KFR_INTRIN ftype<T1> sqrt(const T1& x)
{
    return internal::in_sqrt<>::sqrt(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>

KFR_INTRIN expr_func<fn_sqrt, E1> sqrt(E1&& x)
{
    return { fn_sqrt(), std::forward<E1>(x) };
}
}
}
