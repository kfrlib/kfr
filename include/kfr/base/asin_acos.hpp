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
#include "atan.hpp"
#include "constants.hpp"
#include "function.hpp"
#include "min_max.hpp"
#include "operators.hpp"
#include "select.hpp"
#include "shuffle.hpp"
#include "sqrt.hpp"

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

namespace internal
{

template <cpu_t cpu = cpu_t::native>
struct in_asin_acos : private in_select<cpu>, private in_atan<cpu>, private in_sqrt<cpu>
{
private:
    using in_atan<cpu>::atan2;
    using in_sqrt<cpu>::sqrt;

public:
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
    KFR_SPEC_FN(in_asin_acos, asin)
    KFR_SPEC_FN(in_asin_acos, acos)
};
}

namespace native
{
using fn_asin = internal::in_asin_acos<>::fn_asin;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> asin(const T1& x)
{
    return internal::in_asin_acos<>::asin(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_asin, E1> asin(E1&& x)
{
    return { fn_asin(), std::forward<E1>(x) };
}

using fn_acos = internal::in_asin_acos<>::fn_acos;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> acos(const T1& x)
{
    return internal::in_asin_acos<>::acos(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_acos, E1> acos(E1&& x)
{
    return { fn_acos(), std::forward<E1>(x) };
}
}
}

#pragma clang diagnostic pop
