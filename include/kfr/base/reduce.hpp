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
#include "../base/min_max.hpp"
#include "../base/operators.hpp"
#include "../base/vec.hpp"
#include "basic_expressions.hpp"

namespace kfr
{

template <typename T>
CMT_INLINE T final_mean(T value, size_t size)
{
    return value / T(size);
}
KFR_FN(final_mean)

template <typename T>
CMT_INLINE T final_rootmean(T value, size_t size)
{
    return internal::builtin_sqrt(value / T(size));
}
KFR_FN(final_rootmean)

namespace internal
{
template <typename FinalFn, typename T, KFR_ENABLE_IF(is_callable<FinalFn, T, size_t>::value)>
CMT_INLINE auto reduce_call_final(FinalFn&& finalfn, size_t size, T value)
{
    return finalfn(value, size);
}
template <typename FinalFn, typename T, KFR_ENABLE_IF(!is_callable<FinalFn, T, size_t>::value)>
CMT_INLINE auto reduce_call_final(FinalFn&& finalfn, size_t, T value)
{
    return finalfn(value);
}

template <typename T, typename ReduceFn, typename TransformFn, typename FinalFn, cpu_t cpu = cpu_t::native>
struct expression_reduce : output_expression
{
    constexpr static size_t width = vector_width<T, cpu> * bitness_const(1, 2);

    expression_reduce(ReduceFn&& reducefn, TransformFn&& transformfn, FinalFn&& finalfn)
        : counter(0), reducefn(std::move(reducefn)), transformfn(std::move(transformfn)),
          finalfn(std::move(finalfn)), value(resize<width>(make_vector(reducefn(initialvalue<T>{}))))
    {
    }

    template <typename U, size_t N>
    CMT_INLINE void operator()(coutput_t, size_t, const vec<U, N>& x) const
    {
        counter += N;
        process(x);
    }

    CMT_INLINE T get() { return internal::reduce_call_final(finalfn, counter, horizontal(value, reducefn)); }

protected:
    void reset() { counter = 0; }
    CMT_INLINE void process(vec<T, width> x) const { value = reducefn(transformfn(x), value); }

    template <size_t N, KFR_ENABLE_IF(N < width)>
    CMT_INLINE void process(vec<T, N> x) const
    {
        value = combine(value, reducefn(transformfn(x), narrow<N>(value)));
    }

    template <size_t N, KFR_ENABLE_IF(N > width)>
    CMT_INLINE void process(vec<T, N> x) const
    {
        process(low(x));
        process(high(x));
    }

    mutable size_t counter;
    ReduceFn reducefn;
    TransformFn transformfn;
    FinalFn finalfn;
    mutable vec<T, width> value;
};

template <typename ReduceFn, typename TransformFn = fn_pass_through, typename FinalFn = fn_pass_through,
          typename E1, typename T = value_type_of<E1>>
KFR_SINTRIN T reduce(E1&& e1, ReduceFn&& reducefn, TransformFn&& transformfn = fn_pass_through(),
                     FinalFn&& finalfn = fn_pass_through())
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    const size_t size = e1.size();
    using reducer_t   = expression_reduce<T, decay<ReduceFn>, decay<TransformFn>, decay<FinalFn>>;
    reducer_t red(std::forward<ReduceFn>(reducefn), std::forward<TransformFn>(transformfn),
                  std::forward<FinalFn>(finalfn));
    process<T>(red, std::forward<E1>(e1), size);

    return red.get();
}

KFR_FN(reduce)
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T sum(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn_add());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T mean(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn_add(), fn_pass_through(), fn_final_mean());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T minof(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn::min());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T maxof(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn::max());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T absminof(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn::absmin());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T absmaxof(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn::absmax());
}

template <typename E1, typename E2, typename T = value_type_of<E1>,
          KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_SINTRIN T dotproduct(E1&& x, E2&& y)
{
    auto m    = std::forward<E1>(x) * std::forward<E2>(y);
    using E12 = decltype(m);
    static_assert(!is_generic<E12>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E12>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::move(m), fn_add());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T rms(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn_add(), fn_sqr(), fn_final_rootmean());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T sumsqr(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn_add(), fn_sqr());
}

template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T product(E1&& x)
{
    static_assert(!is_generic<E1>::value, "e1 must be a typed expression (use typed<T>())");
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use typed<T>())");
    return internal::reduce(std::forward<E1>(x), fn_mul());
}
}
