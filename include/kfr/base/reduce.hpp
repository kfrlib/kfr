/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Finalizer for @ref mean.
 *
 * Divides the accumulated sum by the number of elements.
 * @param value Accumulated sum.
 * @param size Number of elements.
 * @return The arithmetic mean.
 */
template <typename T>
KFR_INTRINSIC T final_mean(T value, size_t size)
{
    return value / T(size);
}
KFR_FN(final_mean)

/**
 * @brief Finalizer for @ref rms.
 *
 * Divides the accumulated sum of squares by the number of elements and takes the square root.
 * @param value Accumulated sum of squares.
 * @param size Number of elements.
 * @return The root mean square.
 */
template <typename T>
KFR_INTRINSIC T final_rootmean(T value, size_t size)
{
    return sqrt(value / T(size));
}
KFR_FN(final_rootmean)

namespace internal
{
/**
 * @brief Helper that invokes the finalizer function with or without the element count.
 *
 * If @p FinalFn is invocable with `(T, size_t)` it is called as `finalfn(value, size)`,
 * otherwise it is called as `finalfn(value)`. This allows reducers to use finalizers
 * that do not need the element count.
 */
struct reduce_final_helper
{
    template <typename T, typename FinalFn>
    auto operator()(FinalFn&& finalfn, size_t size, T value) const
    {
        if constexpr (std::is_invocable_v<FinalFn, T, size_t>)
            return std::forward<FinalFn>(finalfn)(value, size);
        else
            return std::forward<FinalFn>(finalfn)(value);
    }
};
} // namespace internal

/**
 * @brief Expression that reduces an input expression to a single value.
 *
 * Applies @p TransformFn to each element, accumulates the result with @p ReduceFn, and
 * applies @p FinalFn to the accumulated value via @ref internal::reduce_final_helper.
 * The accumulator is kept as a vector of width `vector_width<Tin>` so that horizontal
 * reduction is performed only once, in @ref get.
 *
 * @tparam Tout Return type after the finalizer.
 * @tparam Dims Dimensionality of the input expression.
 * @tparam Twork Working type used for the accumulator.
 * @tparam Tin Input element type.
 * @tparam ReduceFn Binary reduction functor.
 * @tparam TransformFn Unary transform applied to each input element before reduction.
 * @tparam FinalFn Finalizer applied to the accumulated value.
 */
template <typename Tout, index_t Dims, typename Twork, typename Tin, typename ReduceFn, typename TransformFn,
          typename FinalFn>
struct expression_reduce : public expression_traits_defaults
{
    using value_type             = Tin;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_reduce&) { return shape<dims>(infinite_size); }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }

    constexpr static size_t width = vector_width<Tin> * bitness_const(1, 2);

    /**
     * @brief Constructs the reducer.
     * @param reducefn Binary reduction functor.
     * @param transformfn Unary transform applied to each input element.
     * @param finalfn Finalizer applied to the accumulated value.
     */
        expression_reduce(ReduceFn reducefn, TransformFn transformfn, FinalFn finalfn)
        : counter(0), reducefn(std::move(reducefn)), transformfn(std::move(transformfn)),
          finalfn(std::move(finalfn)), value(resize<width>(make_vector(reducefn(initialvalue<Twork>{}))))
    {
    }

    /**
     * @brief Returns the reduced value.
     *
     * Horizontally reduces the internal accumulator vector and applies the finalizer.
     * @return The final reduced value.
     */
    KFR_MEM_INTRINSIC Tout get()
    {
        return internal::reduce_final_helper{}(finalfn, counter, horizontal(value, reducefn));
    }

    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC void set_elements(expression_reduce& self, shape<Dims>, axis_params<VecAxis, N>,
                                           const std::type_identity_t<vec<Tin, N>>& x)
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

    template <size_t N>
        requires(N < width)
    KFR_MEM_INTRINSIC void process(const vec<Tin, N>& x) const
    {
        value = combine(value, reducefn(transformfn(x), narrow<N>(value)));
    }

    template <size_t N>
        requires(N > width)
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

/**
 * @brief Reduces an input expression to a single value.
 *
 * Iterates over @p e1, applying @p transformfn to each element, accumulating the result
 * with @p reducefn, and finally applying @p finalfn to the accumulated value. The
 * reduction is performed using @ref expression_reduce for SIMD-accelerated accumulation.
 *
 * @tparam ReduceFn Binary reduction functor.
 * @tparam TransformFn Unary transform applied to each element (defaults to pass-through).
 * @tparam FinalFn Finalizer applied to the accumulated value (defaults to pass-through).
 * @tparam E1 Input expression type.
 * @param e1 Sized input expression to reduce.
 * @param reducefn Binary reduction functor.
 * @param transformfn Unary transform applied to each element.
 * @param finalfn Finalizer applied to the accumulated value.
 * @return The reduced value.
 */
template <
    typename ReduceFn, typename TransformFn = fn_generic::pass_through,
    typename FinalFn = fn_generic::pass_through, input_expression E1,
    typename Tin     = expression_value_type<E1>,
    typename Twork   = std::decay_t<std::invoke_result_t<TransformFn, Tin>>,
    typename Tout = std::decay_t<std::invoke_result_t<internal::reduce_final_helper, FinalFn, size_t, Twork>>>
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

/**
 * @brief Reduces a range to a single value.
 *
 * Overload for non-expression ranges (e.g. standard containers). Iterates over @p e1,
 * applying @p transformfn to each element, accumulating with @p reducefn, and applying
 * @p finalfn to the result. Unlike the expression overload, this performs a scalar
 * reduction without SIMD acceleration.
 *
 * @tparam ReduceFn Binary reduction functor.
 * @tparam TransformFn Unary transform applied to each element (defaults to pass-through).
 * @tparam FinalFn Finalizer applied to the accumulated value (defaults to pass-through).
 * @tparam E1 Range type.
 * @param e1 Range to reduce.
 * @param reducefn Binary reduction functor.
 * @param transformfn Unary transform applied to each element.
 * @param finalfn Finalizer applied to the accumulated value.
 * @return The reduced value.
 */
template <
    typename ReduceFn, typename TransformFn = fn_generic::pass_through,
    typename FinalFn = fn_generic::pass_through, typename E1, typename Tin = expression_value_type<E1>,
    typename Twork = std::decay_t<std::invoke_result_t<TransformFn, Tin>>,
    typename Tout = std::decay_t<std::invoke_result_t<internal::reduce_final_helper, FinalFn, size_t, Twork>>>
    requires(!input_expression<E1>)
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
    return internal::reduce_final_helper{}(finalfn, counter, result);
}

/**
 * @brief Holds the bin counts for a histogram.
 *
 * Stores counts for values that fall below the range, above the range, and within each
 * of the @p Bins bins. When @p Bins is 0 the number of bins is specified at runtime via
 * the constructor; otherwise it is fixed at compile time.
 *
 * For floating-point inputs, values are expected in the range [0, 1] and are mapped to
 * equal-width bins. Integer value @c i maps directly to bin @c i when
 * @c 0 <= i < Bins; Define @c KFR_HISTOGRAM_OLD to
 * restore the previous floating-point nearest-bin mapping.
 *
 * @tparam Bins Number of bins, or 0 for runtime-sized bins.
 * @tparam TCount Integer type used for bin counts.
 */
template <size_t Bins = 0, typename TCount = uint32_t>
struct histogram_data
{
    using vector_type = univector<TCount, Bins == 0 ? tag_dynamic_vector : 2 + Bins>;

    /**
     * @brief Constructs a runtime-sized histogram.
     * @param steps Number of bins. Only used when @p Bins is 0.
     */
    KFR_MEM_INTRINSIC histogram_data(size_t steps)
    {
        if constexpr (Bins == 0)
        {
            m_values = vector_type(2 + steps, 0);
        }
    }

    /**
     * @brief Returns the count in bin @p n.
     * @param n Bin index, must be less than @ref size.
     * @return The count in bin @p n.
     */
    KFR_MEM_INTRINSIC TCount operator[](size_t n) const
    {
        KFR_LOGIC_CHECK(n < size(), "n is outside histogram size");
        return m_values[1 + n];
    }
    /**
     * @brief Returns the count of values that fell below the histogram range.
     */
    KFR_MEM_INTRINSIC TCount below() const { return m_values.front(); }
    /**
     * @brief Returns the count of values that fell above the histogram range.
     */
    KFR_MEM_INTRINSIC TCount above() const { return m_values.back(); }
    /**
     * @brief Returns the number of bins.
     */
    KFR_MEM_INTRINSIC size_t size() const { return m_values.size() - 2; }
    /**
     * @brief Returns a reference to the bin counts, excluding the below/above counters.
     */
    KFR_MEM_INTRINSIC univector_ref<const TCount> values() const { return m_values.slice(1, size()); }
    /**
     * @brief Returns the total number of values added to the histogram.
     */
    KFR_MEM_INTRINSIC uint64_t total() const { return m_total; }

    /**
     * @brief Adds a vector of values to the histogram, incrementing the appropriate bins.
     * @param value Vector of values to add. Floating-point values should be in [0, 1].
     */
    template <typename T, size_t N>
    KFR_MEM_INTRINSIC void put(const vec<T, N>& value)
    {
        vec<u64, N> indices;
        if constexpr (is_f_class<T>)
        {
            const vec<T, N> x = value * size();
#ifdef KFR_HISTOGRAM_OLD
            indices           = cast<uint64_t>(round(clamp(x, 0, size() - 1)));
#else
            indices           = cast<uint64_t>(floor(clamp(x, 0, size() - 1)));
#endif
            indices           = select(value < 0, 0, select(value > 1, size() + 1, 1 + indices));
        }
        else
        {
            const vec<T, N> x = 1 + value;
            indices           = cast<uint64_t>(clamp(x, T(0), T(size() + 1)));
        }
        KFR_LOOP_UNROLL
        for (size_t i = 0; i < N; ++i)
            ++m_values[indices[i]];
        m_total += N;
    }
    /**
     * @brief Adds a single value to the histogram.
     * @param value Value to add.
     */
    template <typename T>
    KFR_MEM_INTRINSIC void put(T value)
    {
        put(vec{ value });
    }

private:
    vector_type m_values{};
    uint64_t m_total = 0;
};

/**
 * @brief Expression that computes a histogram as data flows through it.
 *
 * Wraps an input expression @p E and, as elements are read from it, updates an internal
 * @ref histogram_data with the bin counts. The expression is transparent: it returns the
 * original values while accumulating the histogram as a side effect.
 *
 * @tparam Bins Number of bins, or 0 for runtime-sized bins.
 * @tparam E Wrapped input expression type.
 * @tparam TCount Integer type used for bin counts.
 */
template <size_t Bins, typename E, typename TCount = uint32_t>
struct expression_histogram : public expression_with_traits<E>
{
    mutable histogram_data<Bins, TCount> data{};

    using expression_with_traits<E>::expression_with_traits;

    /**
     * @brief Constructs a runtime-sized histogram expression.
     * @param e Input expression to wrap.
     * @param steps Number of bins. Only used when @p Bins is 0.
     */
    KFR_MEM_INTRINSIC expression_histogram(E&& e, size_t steps)
        : expression_with_traits<E>{ std::forward<E>(e) }, data(steps)
    {
    }

    using value_type = typename expression_with_traits<E>::value_type;

    template <index_t Axis, size_t N>
    friend KFR_INTRINSIC vec<value_type, N> get_elements(const expression_histogram& self,
                                                         const shape<expression_with_traits<E>::dims>& index,
                                                         const axis_params<Axis, N>& sh)
    {
        vec<value_type, N> v = get_elements(self.first(), index, sh);
        self.data.put(v);
        return v;
    }
};

/**
 * @brief Returns the sum of all the elements in x.
 *
 * x must have its size and type specified.
 * \f[
 *  x_0 + x_1 + \ldots + x_{N-1}
 * \f]
 */
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
          typename T = expression_value_type<decltype(std::declval<E1>() * std::declval<E2>())>>
    requires expression_arguments<E1, E2>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
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
template <input_expression E1, typename T = expression_value_type<E1>>
KFR_FUNCTION T product(const E1& x)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return reduce(x, fn::mul());
}

namespace internal
{

/**
 * @brief Transform helper for @ref variance.
 *
 * For each input value @c x, returns a pair @c (x - k, (x - k)^2) where @c k is the
 * reference value (typically the first element). The pair is packed so that a single
 * reduction accumulates both the sum of deviations and the sum of squared deviations.
 */
template <typename T>
struct variance_helper
{
    template <size_t N>
    KFR_MEM_INTRINSIC vec<vec<T, 2>, N> operator()(const vec<T, N>& x) const
    {
        vec<T, N> xmk  = x - k;
        vec<T, N> xmk2 = xmk * xmk;
        return vec<vec<T, 2>, N>::frombits(interleave(xmk, xmk2));
    }
    KFR_MEM_INTRINSIC vec<T, 2> operator()(const T& x) const
    {
        T xmk  = x - k;
        T xmk2 = xmk * xmk;
        return vec<T, 2>(xmk, xmk2);
    }

    T k;
};
} // namespace internal

/**
 * @brief Computes the variance of the given input expression.
 *
 * Uses the shifted-data algorithm with the first element as the reference value @c k:
 * @f[ \frac{1}{N}\sum_{i=0}^{N-1}(x_i - k)^2 - \left(\frac{1}{N}\sum_{i=0}^{N-1}(x_i - k)\right)^2 @f]
 *
 * @tparam E1 The type of the input expression.
 * @tparam T The value type of the expression elements.
 * @param x The input expression for which the variance is to be computed.
 *          Must be a sized expression.
 * @return The variance of the elements in the input expression.
 */
template <input_expression E1, typename T = expression_value_type<E1>>
KFR_FUNCTION T variance(const E1& x)
{
    static_assert(!is_infinite<decltype(x)>, "e1 must be a sized expression (use slice())");
    T k = get_element(x, shape{ 0 });
    return reduce(x, fn::add(), internal::variance_helper<T>{ k },
                  [](const vec<T, 2>& x, size_t count) KFR_INLINE_LAMBDA -> T
                  {
                      // x[0] = sum(x - mean), x[1] = sum((x - mean)^2)
                      return (x[1] - sqr(x[0]) / T(count)) / T(count);
                  });
}

/**
 * @brief Computes the standard deviation of the given expression.
 *
 * Returns the square root of @ref variance.
 *
 * @tparam E1 The type of the input expression.
 * @tparam T The value type of the expression elements.
 * @param x The input expression for which the standard deviation is computed.
 *          Must be a sized expression.
 * @return The standard deviation of the input expression.
 */
template <input_expression E1, typename T = expression_value_type<E1>>
KFR_FUNCTION T stddev(const E1& x)
{
    static_assert(!is_infinite<decltype(x)>, "e1 must be a sized expression (use slice())");
    return sqrt(variance(x));
}

/**
 * @brief Creates an expression that computes a histogram as data flows through it.
 *
 * The number of bins is defined at runtime.
 */
template <typename E, typename TCount = uint32_t>
KFR_FUNCTION expression_histogram<0, E, TCount> histogram_expression(E&& expr, size_t bins)
{
    return { std::forward<E>(expr), bins };
}

/**
 * @brief Creates an expression that computes a histogram as data flows through it.
 *
 * The number of bins is defined at compile time.
 */
template <size_t Bins, typename E, typename TCount = uint32_t>
KFR_FUNCTION expression_histogram<Bins, E, TCount> histogram_expression(E&& expr)
{
    return { std::forward<E>(expr), Bins };
}

/**
 * @brief Returns the histogram of the expression data.
 *
 * The number of bins is defined at runtime.
 */
template <typename E, typename TCount = uint32_t>
KFR_FUNCTION histogram_data<0, TCount> histogram(E&& expr, size_t bins)
{
    return sink(histogram_expression(std::forward<E>(expr), bins)).data;
}

/**
 * @brief Returns the histogram of the expression data.
 *
 * The number of bins is defined at compile time.
 */
template <size_t Bins, typename E, typename TCount = uint32_t>
KFR_FUNCTION histogram_data<Bins, TCount> histogram(E&& expr)
{
    return sink(histogram_expression<Bins>(std::forward<E>(expr))).data;
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
