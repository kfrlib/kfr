/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "basic_expressions.hpp"
#include "function.hpp"
#include "min_max.hpp"
#include "operators.hpp"
#include "vec.hpp"

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

    using value_type = T;

    expression_reduce(ReduceFn&& reducefn, TransformFn&& transformfn, FinalFn&& finalfn)
        : counter(0), reducefn(std::move(reducefn)), transformfn(std::move(transformfn)),
          finalfn(std::move(finalfn)), value(resize<width>(make_vector(reducefn(initialvalue<T>{}))))
    {
    }

    template <size_t N>
    CMT_INLINE void operator()(coutput_t, size_t, const vec<T, N>& x) const
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
}

template <typename ReduceFn, typename TransformFn = fn::pass_through, typename FinalFn = fn::pass_through,
          typename E1, typename T = value_type_of<E1>>
KFR_SINTRIN T reduce(E1&& e1, ReduceFn&& reducefn, TransformFn&& transformfn = fn::pass_through(),
                     FinalFn&& finalfn = fn::pass_through())
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    using reducer_t = internal::expression_reduce<T, decay<ReduceFn>, decay<TransformFn>, decay<FinalFn>>;
    reducer_t red(std::forward<ReduceFn>(reducefn), std::forward<TransformFn>(transformfn),
                  std::forward<FinalFn>(finalfn));
    process<T>(red, std::forward<E1>(e1));

    return red.get();
}

KFR_FN(reduce)

/**
 * @brief Returns the sum of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
 *  x_0 + x_1 + \ldots + x_{N-1}
 * \f]
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T sum(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::add());
}

/**
 * @brief Returns the arithmetic mean of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
 *  \frac{1}{N}(x_0 + x_1 + \ldots + x_{N-1})
 * \f]
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T mean(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::add(), fn::pass_through(), fn::final_mean());
}

/**
 * @brief Returns the smallest of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T minof(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::min());
}

/**
 * @brief Returns the greatest of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T maxof(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::max());
}

/**
 * @brief Returns the smallest in magnitude of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T absminof(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::absmin());
}

/**
 * @brief Returns the greatest in magnitude of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T absmaxof(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::absmax());
}

/**
 * @brief Returns the dot product of two vectors.
 *
 * x and y must have their sizes and types specified.
 * \f[
 *  x_0y_0 + x_1y_1 + \ldots + x_{N-1}y_{N-1}
 * \f]
 */
template <typename E1, typename E2, typename T = value_type_of<E1>,
          KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_SINTRIN T dotproduct(E1&& x, E2&& y)
{
    auto m    = std::forward<E1>(x) * std::forward<E2>(y);
    using E12 = decltype(m);
    static_assert(!is_infinite<E12>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::move(m), fn::add());
}

/**
 * @brief Returns the root mean square of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
   \sqrt{\frac{1}{N}( x_0^2 + x_1^2 + \ldots + x_{N-1}^2)}
   \f]
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T rms(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::add(), fn::sqr(), fn::final_rootmean());
}

/**
 * @brief Returns the sum of squares of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
    x_0^2 + x_1^2 + \ldots + x_{N-1}^2
   \f]
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T sumsqr(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::add(), fn::sqr());
}

/**
 * @brief Returns the product of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
    x_0 \cdot x_1 \cdot \ldots \cdot x_{N-1}
   \f]
 */
template <typename E1, typename T = value_type_of<E1>, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_SINTRIN T product(E1&& x)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return reduce(std::forward<E1>(x), fn::mul());
}
}
