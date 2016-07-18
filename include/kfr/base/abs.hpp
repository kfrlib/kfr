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
#include "operators.hpp"
#include "select.hpp"

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

namespace internal
{

template <cpu_t cpu = cpu_t::native>
struct in_abs : in_abs<older(cpu)>
{
    struct fn_abs : in_abs<older(cpu)>::fn_abs, fn_disabled
    {
    };
};

template <>
struct in_abs<cpu_t::sse2> : in_select<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::sse2;

private:
    using in_select<cpu_t::sse2>::select;

public:
    template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> abs(vec<T, N> value)
    {
        return select(value >= T(), value, -value);
    }
    template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> abs(vec<T, N> value)
    {
        return value & invhighbitmask<T>;
    }

    KFR_HANDLE_ALL(abs)
    KFR_HANDLE_SCALAR(abs)
    KFR_SPEC_FN(in_abs, abs)
};

template <>
struct in_abs<cpu_t::ssse3> : in_abs<cpu_t::sse2>, in_select<cpu_t::sse2>
{
    constexpr static cpu_t cpu = cpu_t::ssse3;

private:
    using in_select<cpu_t::sse2>::select;

public:
    template <size_t N>
    KFR_SINTRIN vec<i64, N> abs(vec<i64, N> value)
    {
        return select(value >= 0, value, -value);
    }

    KFR_CPU_INTRIN(ssse3) i32sse abs(i32sse value) { return _mm_abs_epi32(*value); }
    KFR_CPU_INTRIN(ssse3) i16sse abs(i16sse value) { return _mm_abs_epi16(*value); }
    KFR_CPU_INTRIN(ssse3) i8sse abs(i8sse value) { return _mm_abs_epi8(*value); }

    template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
    KFR_SINTRIN vec<T, N> abs(vec<T, N> value)
    {
        return value & invhighbitmask<T>;
    }

    KFR_HANDLE_ALL(abs)
    KFR_HANDLE_SCALAR(abs)
    KFR_SPEC_FN(in_abs, abs)
};

template <>
struct in_abs<cpu_t::avx2> : in_abs<cpu_t::ssse3>
{
    constexpr static cpu_t cpu = cpu_t::avx2;
    using in_abs<cpu_t::ssse3>::abs;

    KFR_CPU_INTRIN(avx2) i32avx abs(i32avx value) { return _mm256_abs_epi32(*value); }
    KFR_CPU_INTRIN(avx2) i16avx abs(i16avx value) { return _mm256_abs_epi16(*value); }
    KFR_CPU_INTRIN(avx2) i8avx abs(i8avx value) { return _mm256_abs_epi8(*value); }

    KFR_HANDLE_ALL(abs)
    KFR_HANDLE_SCALAR(abs)
    KFR_SPEC_FN(in_abs, abs)
};
}

namespace native
{
using fn_abs = internal::in_abs<>::fn_abs;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>

KFR_INTRIN ftype<T1> abs(const T1& x)
{
    return internal::in_abs<>::abs(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>

KFR_INTRIN expr_func<fn_abs, E1> abs(E1&& x)
{
    return { fn_abs(), std::forward<E1>(x) };
}
}
}

#pragma clang diagnostic pop
