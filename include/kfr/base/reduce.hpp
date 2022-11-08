/** @addtogroup array
 *  @{
 */
/*
  Copyright (C) 2016-2022 Fractalium Ltd (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
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

#include "../simd/horizontal.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/min_max.hpp"
#include "../simd/operators.hpp"
#include "../simd/vec.hpp"
#include "basic_expressions.hpp"
#include "simd_expressions.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T>
KFR_INTRINSIC T final_mean(T value, size_t size)
{
    return value / T(size);
}
KFR_FN(final_mean)

template <typename T>
KFR_INTRINSIC T final_rootmean(T value, size_t size)
{
    return sqrt(value / T(size));
}
KFR_FN(final_rootmean)

template <typename FinalFn, typename T, KFR_ENABLE_IF(std::is_invocable_v<FinalFn, T, size_t>)>
KFR_INTRINSIC auto reduce_call_final(FinalFn&& finalfn, size_t size, T value)
{
    return finalfn(value, size);
}
template <typename FinalFn, typename T, KFR_ENABLE_IF(std::is_invocable_v<FinalFn, size_t>)>
KFR_INTRINSIC auto reduce_call_final(FinalFn&& finalfn, size_t, T value)
{
    return finalfn(value);
}

template <typename Tout, index_t Dims, typename Twork, typename Tin, typename ReduceFn, typename TransformFn,
          typename FinalFn>
struct expression_reduce : public expression_traits_defaults
{
    using value_type             = Tin;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> shapeof(const expression_reduce&) { return shape<dims>(infinite_size); }
    constexpr static shape<dims> shapeof() { return shape<dims>(infinite_size); }

    constexpr static size_t width = vector_width<Tin> * bitness_const(1, 2);

    expression_reduce(ReduceFn&& reducefn, TransformFn&& transformfn, FinalFn&& finalfn)
        : counter(0), reducefn(std::move(reducefn)), transformfn(std::move(transformfn)),
          finalfn(std::move(finalfn)), value(resize<width>(make_vector(reducefn(initialvalue<Twork>{}))))
    {
    }

    KFR_MEM_INTRINSIC Tout get() { return reduce_call_final(finalfn, counter, horizontal(value, reducefn)); }

    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC void set_elements(expression_reduce& self, shape<Dims>, axis_params<VecAxis, N>,
                                           const identity<vec<Tin, N>>& x)
    {
        self.counter += N;
        self.process(x);
    }

protected:
    void reset() { counter = 0; }
    KFR_MEM_INTRINSIC void process(const vec<Tin, width>& x) const
    {
        value = reducefn(transformfn(x), value);
    }

    template <size_t N, KFR_ENABLE_IF(N < width)>
    KFR_MEM_INTRINSIC void process(const vec<Tin, N>& x) const
    {
        value = combine(value, reducefn(transformfn(x), narrow<N>(value)));
    }

    template <size_t N, KFR_ENABLE_IF(N > width)>
    KFR_MEM_INTRINSIC void process(const vec<Tin, N>& x) const
    {
        process(low(x));
        process(high(x));
    }

    mutable size_t counter;
    ReduceFn reducefn;
    TransformFn transformfn;
    FinalFn finalfn;
    mutable vec<Twork, width> value;
};

template <typename ReduceFn, typename TransformFn = fn_generic::pass_through,
          typename FinalFn = fn_generic::pass_through, typename E1, typename Tin = expression_value_type<E1>,
          typename Twork = std::decay_t<decltype(std::declval<TransformFn>()(std::declval<Tin>()))>,
          typename Tout  = std::decay_t<decltype(reduce_call_final(
              std::declval<FinalFn>(), std::declval<size_t>(), std::declval<Twork>()))>,
          KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC Tout reduce(const E1& e1, ReduceFn&& reducefn,
                          TransformFn&& transformfn = fn_generic::pass_through(),
                          FinalFn&& finalfn         = fn_generic::pass_through())
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    using reducer_t = expression_reduce<Tout, expression_dims<E1>, Twork, Tin, std::decay_t<ReduceFn>,
                                        std::decay_t<TransformFn>, std::decay_t<FinalFn>>;
    reducer_t red(std::forward<ReduceFn>(reducefn), std::forward<TransformFn>(transformfn),
                  std::forward<FinalFn>(finalfn));
    process(red, e1);

    return red.get();
}

template <typename ReduceFn, typename TransformFn = fn_generic::pass_through,
          typename FinalFn = fn_generic::pass_through, typename E1, typename Tin = expression_value_type<E1>,
          typename Twork = std::decay_t<decltype(std::declval<TransformFn>()(std::declval<Tin>()))>,
          typename Tout  = std::decay_t<decltype(reduce_call_final(
              std::declval<FinalFn>(), std::declval<size_t>(), std::declval<Twork>()))>,
          KFR_ENABLE_IF(!is_input_expression<E1>)>
KFR_INTRINSIC Tout reduce(const E1& e1, ReduceFn&& reducefn,
                          TransformFn&& transformfn = fn_generic::pass_through(),
                          FinalFn&& finalfn         = fn_generic::pass_through())
{
    Twork result   = reducefn(initialvalue<Twork>());
    size_t counter = 0;
    for (const Tin& in : e1)
    {
        result = reducefn(result, transformfn(in));
        ++counter;
    }
    return reduce_call_final(finalfn, counter, result);
}

/**
 * @brief Returns the sum of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
 *  x_0 + x_1 + \ldots + x_{N-1}
 * \f]
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T sum(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::add());
}

/**
 * @brief Returns the arithmetic mean of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
 *  \frac{1}{N}(x_0 + x_1 + \ldots + x_{N-1})
 * \f]
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T mean(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::add(), fn_generic::pass_through(), fn::final_mean());
}

/**
 * @brief Returns the smallest of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T minof(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::min());
}

/**
 * @brief Returns the greatest of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T maxof(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::max());
}

/**
 * @brief Returns the smallest in magnitude of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T absminof(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::absmin());
}

/**
 * @brief Returns the greatest in magnitude of all the elements in x.
 *
 * x must have its size and type specified.
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T absmaxof(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::absmax());
}

/**
 * @brief Returns the dot product of two vectors.
 *
 * x and y must have their sizes and types specified.
 * \f[
 *  x_0y_0 + x_1y_1 + \ldots + x_{N-1}y_{N-1}
 * \f]
 */
template <typename E1, typename E2,
          typename T = expression_value_type<decltype(std::declval<E1>() * std::declval<E2>())>,
          KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION T dotproduct(E1&& x, E2&& y)
{
    auto m    = std::forward<E1>(x) * std::forward<E2>(y);
    using E12 = decltype(m);
    static_assert(!is_infinite<E12>, "e1 must be a sized expression (use slice())");
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
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T rms(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::add(), fn::sqr(), fn::final_rootmean());
}

/**
 * @brief Returns the sum of squares of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
    x_0^2 + x_1^2 + \ldots + x_{N-1}^2
   \f]
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T sumsqr(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::add(), fn::sqr());
}

/**
 * @brief Returns the product of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
    x_0 \cdot x_1 \cdot \ldots \cdot x_{N-1}
   \f]
 */
template <typename E1, typename T = expression_value_type<E1>, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_FUNCTION T product(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::mul());
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
