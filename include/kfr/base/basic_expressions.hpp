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

#include "expression.hpp"

namespace kfr
{
// ----------------------------------------------------------------------------

/**
 * @brief Expression that wraps a single scalar value of type @p T.
 *
 * The expression has zero dimensions and yields the same value regardless of
 * the index passed to @ref get_elements.
 *
 * @tparam T scalar value type
 */
template <typename T>
struct expression_scalar
{
    T value;
};

template <typename T>
struct expression_traits<expression_scalar<T>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 0;

    constexpr static shape<0> get_shape(const expression_scalar<T>& self) { return {}; }
    constexpr static shape<0> get_shape() { return {}; }
};

/**
 * @brief Creates an expression that wraps a single value.
 *
 * @tparam T    value type (deduced from the argument)
 * @param  value the scalar value to wrap
 * @return an @ref expression_scalar holding @p value
 */
template <typename T>
KFR_INTRINSIC expression_scalar<T> scalar(T value)
{
    return { std::move(value) };
}

/**
 * @brief Creates an expression that always evaluates to zero.
 *
 * @tparam T value type (defaults to @ref fbase)
 */
template <typename T = fbase>
KFR_INTRINSIC expression_scalar<T> zeros()
{
    return { static_cast<T>(0) };
}

/**
 * @brief Creates an expression that always evaluates to one.
 *
 * @tparam T value type (defaults to @ref fbase)
 */
template <typename T = fbase>
KFR_INTRINSIC expression_scalar<T> ones()
{
    return { static_cast<T>(1) };
}

inline namespace KFR_ARCH_NAME
{
/** Internal ADL-provided implementation for `expression_scalar<T>` expressions */
template <typename T, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_scalar<T>& self, const shape<0>& index,
                                     const axis_params<Axis, N>&)
{
    return self.value;
}
} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that produces a sequence of values
 *        `start, start + step, start + 2*step, ...` for one or more dimensions.
 *
 * Each dimension has its own step; the produced value for an index is
 * `start + sum(steps[i] * index[i])`.
 *
 * @tparam T     value type
 * @tparam Dims  number of dimensions (1 or more)
 */
template <typename T, index_t Dims = 1>
struct expression_counter
{
    T start;
    T steps[Dims];

    T back() const { return steps[Dims - 1]; }
    T front() const { return steps[0]; }
};

template <typename T, index_t Dims>
struct expression_traits<expression_counter<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    constexpr static shape<dims> get_shape(const expression_counter<T, Dims>& self)
    {
        return shape<dims>(infinite_size);
    }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }
};

/**
 * @brief Creates a 1-D @ref expression_counter with step 1.
 *
 * @tparam T    integer value type (defaults to `int`)
 * @tparam Tout output value type (defaults to T)
 * @param  start starting value of the counter
 * @return an @ref expression_counter producing `start, start+1, start+2, ...`
 */
template <typename T = int, typename Tout = T>
KFR_INTRINSIC expression_counter<Tout, 1> counter(T start = 0)
{
    return { static_cast<Tout>(std::move(start)), { static_cast<Tout>(1) } };
}

/**
 * @brief Creates an N-dimensional @ref expression_counter with one step per dimension.
 *
 * The produced value is `start + step*index[0] + steps*index[1] + ...`.
 *
 * @tparam T    integer value type of @p start
 * @tparam Arg  type of the first step
 * @tparam Args types of the remaining steps
 * @tparam Tout common value type used in the resulting expression
 * @param  start starting value
 * @param  step  step for the first dimension
 * @param  steps steps for the remaining dimensions
 * @return an @ref expression_counter
 */
template <typename T = int, typename Arg = T, typename... Args,
          typename Tout = std::common_type_t<T, Arg, Args...>>
KFR_INTRINSIC expression_counter<Tout, 1 + sizeof...(Args)> counter(T start, Arg step, Args... steps)
{
    return { static_cast<Tout>(std::move(start)),
             { static_cast<Tout>(std::move(step)), static_cast<Tout>(std::move(steps))... } };
}

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_counter<T, 1>` expressions */
template <typename T, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_counter<T, 1>& self, const shape<1>& index,
                                     const axis_params<Axis, N>&)
{
    T acc = self.start;
    acc += static_cast<T>(index.back()) * self.back();
    return acc + enumerate(vec_shape<T, N>(), self.back());
}
/** Internal ADL-provided implementation for `expression_counter<T, dims>` expressions */
template <typename T, index_t dims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_counter<T, dims>& self, const shape<dims>& index,
                                     const axis_params<Axis, N>&)
{
    T acc                 = self.start;
    vec<T, dims> tindices = cast<T>(to_vec(index));
    cfor(csize<0>, csize<dims>, [&](auto i) KFR_INLINE_LAMBDA { acc += tindices[i] * self.steps[i]; });
    return acc + enumerate(vec_shape<T, N>(), self.steps[Axis]);
}
} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that exposes a sub-region of another expression.
 *
 * Access is forwarded to the wrapped expression with the index offset by
 * @c start and clamped to @c size.
 *
 * @tparam Arg wrapped expression type
 */
template <typename Arg>
struct expression_slice : public expression_with_arguments<Arg>
{
    constexpr static index_t dims = expression_dims<Arg>;
    static_assert(dims > 0);
    shape<dims> start;
    shape<dims> size;

    KFR_MEM_INTRINSIC expression_slice(Arg&& arg, shape<dims> start, shape<dims> size)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) }, start(start), size(size)
    {
    }
};

template <typename Arg>
struct expression_traits<expression_slice<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const expression_slice<Arg>& self)
    {
        return min(sub_shape(ArgTraits::get_shape(self.first()), self.start), self.size);
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return shape<dims>(undefined_size); }
};

/**
 * @brief Creates an expression that exposes a sub-region of another expression.
 *
 * @param  arg   the input expression
 * @param  start starting offset of the slice
 * @param  size  size of the slice (defaults to infinite_size)
 * @return an @ref expression_slice
 */
template <expression_argument Arg, index_t Dims = expression_dims<Arg>>
KFR_INTRINSIC expression_slice<Arg> slice(Arg&& arg, std::type_identity_t<shape<Dims>> start,
                                          std::type_identity_t<shape<Dims>> size = shape<Dims>(infinite_size))
{
    static_assert(Dims > 0);
    return { std::forward<Arg>(arg), start, size };
}

/**
 * @brief Creates an expression that exposes @p arg starting at the origin with the given @p size.
 *
 * @param  arg  the input expression
 * @param  size size of the truncated region
 * @return an @ref expression_slice
 */
template <expression_argument Arg, index_t Dims = expression_dims<Arg>>
KFR_INTRINSIC expression_slice<Arg> truncate(Arg&& arg, std::type_identity_t<shape<Dims>> size)
{
    static_assert(Dims > 0);
    return { std::forward<Arg>(arg), shape<Dims>{ 0 }, size };
}

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_slice<Arg>` expressions */
template <typename Arg, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<expression_slice<Arg>>::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_slice<Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index.add(self.start), sh));
}

/** Internal ADL-provided implementation for `expression_slice<Arg>` expressions */
template <output_expression Arg, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<expression_slice<Arg>>::value_type>
KFR_INTRINSIC void set_elements(const expression_slice<Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const std::type_identity_t<vec<T, N>>& value)
{
    set_elements(self.first(), index.add(self.start), sh, value);
}
} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that converts the values of the wrapped expression to type @p T.
 *
 * @tparam T   target value type
 * @tparam Arg wrapped expression type
 */
template <typename T, typename Arg>
struct expression_cast : public expression_with_arguments<Arg>
{
    using expression_with_arguments<Arg>::expression_with_arguments;
};

template <typename T, typename Arg>
struct expression_traits<expression_cast<T, Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = T;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const expression_cast<T, Arg>& self)
    {
        return ArgTraits::get_shape(self.first());
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return ArgTraits::get_shape(); }
};

/**
 * @brief Creates an expression that converts the values of @p arg to type @p T.
 *
 * @tparam T   target value type
 * @param  arg input expression
 * @return an @ref expression_cast
 */
template <typename T, expression_argument Arg>
KFR_INTRINSIC expression_cast<T, Arg> cast(Arg&& arg)
{
    return { std::forward<Arg>(arg) };
}

/**
 * @brief Creates an expression that converts the values of @p arg to type @p T.
 *
 * Overload accepting a @ref ctype_t tag for explicit type specification.
 *
 * @tparam T   target value type
 * @param  arg input expression
 * @return an @ref expression_cast
 */
template <typename T, expression_argument Arg>
KFR_INTRINSIC expression_cast<T, Arg> cast(Arg&& arg, ctype_t<T>)
{
    return { std::forward<Arg>(arg) };
}

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_cast<T, Arg>` expressions */
template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_cast<T, Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index, sh));
}

/** Internal ADL-provided implementation for `expression_cast<T, Arg>` expressions */
template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const expression_cast<T, Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const std::type_identity_t<vec<T, N>>& value)
{
    set_elements(self.first(), index, sh, value);
}
} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression backed by a user-provided callable.
 *
 * The callable is invoked with the current index (and optionally an
 * @ref axis_params) to produce values on demand.
 *
 * @tparam T     value type produced by the lambda
 * @tparam Dims  number of dimensions
 * @tparam Fn    callable type
 * @tparam Rnd   whether the lambda supports random access
 */
template <typename T, index_t Dims, typename Fn, bool Rnd>
struct expression_lambda
{
    Fn fn;
};

template <typename T, index_t Dims, typename Fn, bool Rnd>
struct expression_traits<expression_lambda<T, Dims, Fn, Rnd>> : expression_traits_defaults
{
    using value_type                           = T;
    constexpr static size_t dims               = Dims;
    constexpr static inline bool random_access = Rnd;

    KFR_MEM_INTRINSIC constexpr static shape<Dims> get_shape(const expression_lambda<T, Dims, Fn, Rnd>& self)
    {
        return shape<Dims>(infinite_size);
    }
    KFR_MEM_INTRINSIC constexpr static shape<Dims> get_shape() { return shape<Dims>(infinite_size); }
};

/**
 * @brief Creates an expression backed by a callable.
 *
 * The callable may be invoked with `(shape<Dims>, axis_params<Axis, N>)`,
 * `(shape<Dims>, csize_t<N>)`, `(shape<Dims>)` or `()`.
 *
 * @tparam T     value type produced by the lambda
 * @tparam Dims  number of dimensions
 * @param  fn    callable used to produce values
 * @return an @ref expression_lambda
 */
template <typename T, index_t Dims = 1, typename Fn, bool RandomAccess = true>
KFR_INTRINSIC expression_lambda<T, Dims, Fn, RandomAccess> lambda(Fn&& fn, cbool_t<RandomAccess> = {})
{
    return { std::forward<Fn>(fn) };
}
/**
 * @brief Creates an expression backed by a non-random-access callable.
 *
 * The resulting expression has @c random_access set to @c false.
 *
 * @tparam T     value type produced by the lambda
 * @tparam Dims  number of dimensions
 * @param  fn    callable used to produce values
 * @return an @ref expression_lambda
 */
template <typename T, index_t Dims = 1, typename Fn>
KFR_INTRINSIC expression_lambda<T, Dims, Fn, false> lambda_generator(Fn&& fn)
{
    return { std::forward<Fn>(fn) };
}

/**
 * @brief Creates an expression that cycles through the provided list of values.
 *
 * @tparam Ts  types of the values in @p list
 * @param  list values to cycle through
 * @return an @ref expression_lambda producing @c list[index % size]
 */
template <typename... Ts, typename T = std::common_type_t<Ts...>>
KFR_INTRINSIC auto sequence(const Ts&... list)
{
    return lambda<T>([seq = std::array<T, sizeof...(Ts)>{ { static_cast<T>(list)... } }](index_t index) { //
        return seq[index % seq.size()];
    });
}

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_lambda<T, Dims, Fn, Rnd>` expressions */
template <typename T, index_t Dims, typename Fn, bool Rnd, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_lambda<T, Dims, Fn, Rnd>& self,
                                     const shape<Dims>& index, const axis_params<Axis, N>& sh)
{
    if constexpr (std::is_invocable_v<Fn, shape<Dims>, axis_params<Axis, N>>)
        return self.fn(index, sh);
    else if constexpr (std::is_invocable_v<Fn, shape<Dims>, csize_t<N>>)
        return self.fn(index, csize<N>);
    else if constexpr (std::is_invocable_v<Fn, shape<Dims>>)
    {
        portable_vec<T, N> result;
        shape<Dims> cur_index = index;
        for (index_t i = 0; i < N; ++i)
        {
            result[i] = self.fn(cur_index);
            ++cur_index.back();
        }
        return result;
    }
    else if constexpr (std::is_invocable_v<Fn>)
        return apply<N>(self.fn);
    else
    {
        static_assert(std::is_invocable_v<Fn, shape<Dims>, axis_params<Axis, N>> ||
                          std::is_invocable_v<Fn, shape<Dims>, csize_t<N>> ||
                          std::is_invocable_v<Fn, shape<Dims>> || std::is_invocable_v<Fn>,
                      "Lambda must be callable");
        return czeros;
    }
}

} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that extends another expression with a constant fill value.
 *
 * Out-of-range positions return @c fill_value; positions near the boundary are
 * fetched element-wise from the wrapped expression.
 *
 * @tparam Arg wrapped expression type
 */
template <typename Arg>
struct expression_padded : public expression_with_arguments<Arg>
{
    using ArgTraits = typename expression_with_arguments<Arg>::first_arg_traits;
    typename ArgTraits::value_type fill_value;
    shape<ArgTraits::dims> input_shape;

    KFR_MEM_INTRINSIC expression_padded(Arg&& arg, typename ArgTraits::value_type fill_value)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) }, fill_value(std::move(fill_value)),
          input_shape(ArgTraits::get_shape((this->first())))
    {
    }
};

/**
 * @brief Creates an expression that extends @p arg with a constant fill value.
 *
 * @param  arg        input expression
 * @param  fill_value value used for out-of-range positions (defaults to a default-constructed value)
 * @return an @ref expression_padded
 */
template <expression_argument Arg, typename T = expression_value_type<Arg>>
KFR_INTRINSIC expression_padded<Arg> padded(Arg&& arg, T fill_value = T{})
{
    static_assert(expression_dims<Arg> >= 1);
    return { std::forward<Arg>(arg), std::move(fill_value) };
}

template <typename Arg>
struct expression_traits<expression_padded<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const expression_padded<Arg>& self)
    {
        return shape<dims>(infinite_size);
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }
};

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_padded<Arg>` expressions */
template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<expression_padded<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_padded<Arg>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    if (index.ge(self.input_shape))
    {
        return self.fill_value;
    }
    else if (KFR_LIKELY(index.add(N).le(self.input_shape)))
    {
        return get_elements(self.first(), index, sh);
    }
    else
    {
        vec<T, N> x = self.fill_value;
        for (size_t i = 0; i < N; i++)
        {
            shape ish = index.add(i);
            if (ish.back() < self.input_shape.back())
                x[i] = get_elements(self.first(), ish, axis_params_v<Axis, 1>).front();
        }
        return x;
    }
}

} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that exposes another expression with its trailing dimension reversed.
 *
 * @tparam Arg wrapped expression type (must have @c random_access)
 */
template <typename Arg>
struct expression_reverse : public expression_with_arguments<Arg>
{
    using ArgTraits = typename expression_with_arguments<Arg>::first_arg_traits;
    shape<ArgTraits::dims> input_shape;

    KFR_MEM_INTRINSIC expression_reverse(Arg&& arg)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) },
          input_shape(ArgTraits::get_shape(this->first()))
    {
    }
};

/**
 * @brief Creates an expression that exposes @p arg with its trailing dimension reversed.
 *
 * @param  arg input expression
 * @return an @ref expression_reverse
 */
template <expression_argument Arg>
KFR_INTRINSIC expression_reverse<Arg> reverse(Arg&& arg)
{
    static_assert(expression_dims<Arg> >= 1);
    return { std::forward<Arg>(arg) };
}

template <typename Arg>
struct expression_traits<expression_reverse<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type             = typename ArgTraits::value_type;
    constexpr static size_t dims = ArgTraits::dims;
    static_assert(ArgTraits::random_access, "expression_reverse requires an expression with random access");

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const expression_reverse<Arg>& self)
    {
        return ArgTraits::get_shape(self.first());
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return ArgTraits::get_shape(); }
};

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_reverse<Arg>` expressions */
template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<expression_reverse<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_reverse<Arg>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return reverse(get_elements(self.first(), self.input_shape.sub(index).sub(shape<Traits::dims>(N)), sh));
}
/** Internal ADL-provided implementation for `expression_reverse<Arg>` expressions */
template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<expression_reverse<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC void set_elements(expression_reverse<Arg>& self, const shape<Traits::dims>& index,
                                const axis_params<Axis, N>& sh, const std::type_identity_t<vec<T, N>>& value)
{
    set_elements(self.first(), self.input_shape.sub(index).sub(shape<Traits::dims>(N)), sh, reverse(value));
}

} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Type-level storage for a fixed @ref shape built from a list of constants.
 *
 * @tparam Values the dimensions of the fixed shape
 */
template <index_t... Values>
struct fixed_shape_t
{
    constexpr fixed_shape_t() = default;
    constexpr static shape<sizeof...(Values)> get() { return { Values... }; }
};

/**
 * @brief Convenience variable template for instantiating a @ref fixed_shape_t.
 *
 * @tparam Values the dimensions of the fixed shape
 */
template <index_t... Values>
constexpr inline fixed_shape_t<Values...> fixed_shape{};

/**
 * @brief Expression that overrides the shape of another expression with a compile-time shape.
 *
 * The wrapped expression's trailing dimensions are trimmed to match the inner
 * expression, while the result reports the dimensions given by @c Shape.
 *
 * @tparam Arg   wrapped expression type
 * @tparam Shape compile-time shape descriptor (typically @ref fixed_shape_t)
 */
template <typename Arg, typename Shape>
struct expression_fixshape : public expression_with_arguments<Arg>
{
    using ArgTraits = typename expression_with_arguments<Arg>::first_arg_traits;

    KFR_MEM_INTRINSIC expression_fixshape(Arg&& arg)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) }
    {
    }
};

/**
 * @brief Creates an expression that overrides the shape of @p arg with a compile-time shape.
 *
 * @tparam Arg          input expression type
 * @tparam ShapeValues  dimension values of the fixed shape
 * @param  arg          input expression
 * @return an @ref expression_fixshape
 */
template <expression_argument Arg, index_t... ShapeValues>
KFR_INTRINSIC expression_fixshape<Arg, fixed_shape_t<ShapeValues...>> fixshape(
    Arg&& arg, const fixed_shape_t<ShapeValues...>&)
{
    return { std::forward<Arg>(arg) };
}

template <typename Arg, index_t... ShapeValues>
struct expression_traits<expression_fixshape<Arg, fixed_shape_t<ShapeValues...>>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = sizeof...(ShapeValues); // ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(
        const expression_fixshape<Arg, fixed_shape_t<ShapeValues...>>& self)
    {
        return fixed_shape_t<ShapeValues...>::get();
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape()
    {
        return fixed_shape_t<ShapeValues...>::get();
    }
};

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_fixshape<Arg, Shape>` expressions */
template <typename Arg, typename Shape, index_t Axis, size_t N,
          typename Traits = expression_traits<expression_fixshape<Arg, Shape>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_fixshape<Arg, Shape>& self,
                                     const shape<Traits::dims>& index, const axis_params<Axis, N>& sh)
{
    using ArgTraits = expression_traits<Arg>;
    return get_elements(self.first(), index.template trim<ArgTraits::dims>(), sh);
}

/** Internal ADL-provided implementation for `expression_fixshape<Arg, Shape>` expressions */
template <typename Arg, typename Shape, index_t Axis, size_t N,
          typename Traits = expression_traits<expression_fixshape<Arg, Shape>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC void set_elements(expression_fixshape<Arg, Shape>& self, const shape<Traits::dims>& index,
                                const axis_params<Axis, N>& sh, const std::type_identity_t<vec<T, N>>& value)
{
    using ArgTraits = expression_traits<Arg>;
    if constexpr (is_output_expression<Arg>)
    {
        set_elements(self.first(), index.template trim<ArgTraits::dims>(), sh, value);
    }
    else
    {
    }
}

} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that exposes the same data as another expression with a different shape.
 *
 * Elements are addressed by flattening the output index into the input's
 * flat layout and unflattening it back into the input's shape.
 *
 * @tparam Arg      wrapped expression type
 * @tparam OutDims  number of dimensions of the result
 */
template <typename Arg, index_t OutDims>
struct expression_reshape : public expression_with_arguments<Arg>
{
    using ArgTraits = typename expression_with_arguments<Arg>::first_arg_traits;
    shape<ArgTraits::dims> in_shape;
    shape<OutDims> out_shape;

    KFR_MEM_INTRINSIC expression_reshape(Arg&& arg, const shape<OutDims>& out_shape)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) }, in_shape(ArgTraits::get_shape(arg)),
          out_shape(out_shape)
    {
    }
};

/**
 * @brief Creates an expression that exposes the same data as @p arg with a different shape.
 *
 * @param  arg       input expression
 * @param  out_shape desired output shape
 * @return an @ref expression_reshape
 */
template <expression_argument Arg, index_t OutDims>
KFR_INTRINSIC expression_reshape<Arg, OutDims> reshape(Arg&& arg, const shape<OutDims>& out_shape)
{
    return { std::forward<Arg>(arg), out_shape };
}

template <typename Arg, index_t OutDims>
struct expression_traits<expression_reshape<Arg, OutDims>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = OutDims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const expression_reshape<Arg, OutDims>& self)
    {
        return self.out_shape;
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return shape<dims>{ undefined_size }; }
};

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_reshape<Arg, outdims>` expressions */
template <typename Arg, index_t outdims, index_t Axis, size_t N,
          typename Traits = expression_traits<expression_reshape<Arg, outdims>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_reshape<Arg, outdims>& self,
                                     const shape<Traits::dims>& index, const axis_params<Axis, N>& sh)
{
    using ArgTraits          = typename Traits::ArgTraits;
    constexpr index_t indims = ArgTraits::dims;
    if constexpr (N == 1)
    {
        const shape<indims> idx = self.in_shape.from_flat(self.out_shape.to_flat(index));
        return get_elements(self.first(), idx, axis_params<indims - 1, 1>{});
    }
    else
    {
        const shape<indims> first_idx = self.in_shape.from_flat(self.out_shape.to_flat(index));
        const shape<indims> last_idx =
            self.in_shape.from_flat(self.out_shape.to_flat(index.add_at(N - 1, cindex<Axis>)));

        const shape<indims> diff_idx = last_idx.sub(first_idx);

        vec<T, N> result;
        bool done = false;

        if (diff_idx.sum() == N - 1)
        {
            cforeach(cvalseq_t<index_t, indims, 0>{},
                     [&](auto n) KFR_INLINE_LAMBDA
                     {
                         constexpr index_t axis = val_of<decltype(n)>({});
                         if (!done && diff_idx[axis] == N - 1)
                         {
                             result = get_elements(self.first(), first_idx, axis_params<axis, N>{});
                             done   = true;
                         }
                     });
        }

        if (!done)
        {
            portable_vec<T, N> tmp;
            KFR_LOOP_NOUNROLL
            for (size_t i = 0; i < N; ++i)
            {
                shape<Traits::dims> idx = index.add_at(i, cindex<Axis>);
                tmp[i] = get_elements(self.first(), self.in_shape.from_flat(self.out_shape.to_flat(idx)),
                                      axis_params<indims - 1, 1>{})
                             .front();
            }
            result = tmp;
        }
        return result;
    }
}

/** Internal ADL-provided implementation for `expression_reshape<Arg, outdims>` expressions */
template <typename Arg, index_t outdims, index_t Axis, size_t N,
          typename Traits = expression_traits<expression_reshape<Arg, outdims>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC void set_elements(expression_reshape<Arg, outdims>& self, const shape<Traits::dims>& index,
                                const axis_params<Axis, N>& sh, const std::type_identity_t<vec<T, N>>& value)
{
    using ArgTraits          = typename Traits::ArgTraits;
    constexpr index_t indims = ArgTraits::dims;
    if constexpr (N == 1)
    {
        const shape<indims> idx = self.in_shape.from_flat(self.out_shape.to_flat(index));
        set_elements(self.first(), idx, axis_params<indims - 1, 1>{}, value);
    }
    else
    {
        const shape<indims> first_idx = self.in_shape.from_flat(self.out_shape.to_flat(index));
        const shape<indims> last_idx =
            self.in_shape.from_flat(self.out_shape.to_flat(index.add_at(N - 1, cindex<Axis>)));

        const shape<indims> diff_idx = last_idx.sub(first_idx);

        bool done = false;

        cforeach(cvalseq_t<index_t, indims, 0>{},
                 [&](auto n) KFR_INLINE_LAMBDA
                 {
                     constexpr index_t axis = val_of<decltype(n)>({});
                     if (!done && diff_idx[axis] == N - 1)
                     {
                         set_elements(self.first(), first_idx, axis_params<axis, N>{}, value);
                         done = true;
                     }
                 });

        if (!done)
        {
            KFR_LOOP_NOUNROLL
            for (size_t i = 0; i < N; ++i)
            {
                set_elements(self.first(),
                             self.in_shape.from_flat(self.out_shape.to_flat(index.add_at(i, cindex<Axis>))),
                             axis_params<indims - 1, 1>{}, vec<T, 1>{ value[i] });
            }
        }
    }
}

} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Tag type used to select the symmetric-linspace constructor of @ref expression_linspace.
 */
struct symmetric_linspace_t
{
};
constexpr inline const symmetric_linspace_t symmetric_linspace{};

/**
 * @brief Expression that produces evenly spaced values from @c start to @c stop.
 *
 * @tparam T         value type
 * @tparam truncated whether the expression has a fixed size or is infinite
 */
template <typename T, bool truncated = true>
struct expression_linspace
{
    T start;
    T stop;
    index_t size;
    bool endpoint;
    T invsize;

    expression_linspace(T start, T stop, size_t size, bool endpoint = false)
        : start(start), stop(stop), size(size), invsize(T(1.0) / T(endpoint ? size - 1 : size))
    {
    }

    expression_linspace(symmetric_linspace_t, T symsize, size_t size, bool endpoint = false)
        : expression_linspace(-symsize, +symsize, size, endpoint)
    {
    }
};

template <typename T, bool truncated>
struct expression_traits<expression_linspace<T, truncated>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;

    constexpr static shape<dims> get_shape(const expression_linspace<T, truncated>& self)
    {
        return shape<dims>(truncated ? self.size : infinite_size);
    }
    constexpr static shape<dims> get_shape()
    {
        return shape<dims>(truncated ? undefined_size : infinite_size);
    }
};

/**
 * @brief Creates an expression that returns evenly spaced numbers over a specified interval.
 *
 * @param  start    the starting value of the sequence
 * @param  stop     the end value of the sequence; if ``endpoint`` is ``false``, the last value is excluded
 * @param  size     number of samples to generate
 * @param  endpoint if ``true``, ``stop`` is the last sample; otherwise, it is not included
 * @tparam truncated if ``true``, linspace returns exactly size elements, otherwise, returns an infinite
 * sequence
 * @tparam precise   no longer used since KFR5; calculations are always precise
 */
template <typename T = void, bool precise = false, bool truncated = false, typename T1, typename T2,
          typename Tout = or_type<T, ftype<std::common_type_t<T1, T2>>>>
KFR_INTRINSIC expression_linspace<Tout, truncated> linspace(T1 start, T2 stop, size_t size,
                                                            bool endpoint = false, cbool_t<truncated> = {})
{
    return { static_cast<Tout>(start), static_cast<Tout>(stop), size, endpoint };
}

/**
 * @brief Creates an expression that returns evenly spaced numbers over a symmetric interval.
 *
 * @param  symsize the resulting sequence spans `[-symsize, +symsize]`
 * @param  size    number of samples to generate
 * @tparam truncated if ``true``, symmlinspace returns exactly size elements, otherwise, returns an infinite
 * sequence
 * @tparam precise   no longer used since KFR5; calculations are always precise
 */
template <typename T, bool precise = false, bool truncated = false, typename Tout = ftype<T>>
KFR_INTRINSIC expression_linspace<Tout, truncated> symmlinspace(T symsize, size_t size,
                                                                cbool_t<truncated> = {})
{
    return { symmetric_linspace, static_cast<Tout>(symsize), size, true };
}

/**
 * @brief Creates an expression that returns values spanning `[start, stop)` with the given step.
 *
 * @tparam T         value type
 * @tparam precise   unused, kept for backwards compatibility
 * @tparam truncated always @c true for the result of @c arange
 * @param  start     first value
 * @param  stop      end value (exclusive)
 * @param  step      step between consecutive values
 * @return an @ref expression_linspace of exactly the right number of elements
 */
template <typename T, bool precise = false, bool truncated = false, typename Tout = ftype<T>>
KFR_INTRINSIC expression_linspace<Tout, true> arange(T start, T stop, T step = 1, cbool_t<truncated> = {})
{
    return linspace<T, precise>(start, stop, static_cast<size_t>(std::ceil((stop - start) / step)), false,
                                ctrue);
}

/**
 * @brief Creates an expression that returns integer values spanning `[0, stop)` with step 1.
 *
 * @tparam T         value type
 * @tparam precise   unused, kept for backwards compatibility
 * @tparam truncated always @c true for the result of @c arange
 * @param  stop      end value (exclusive)
 * @return an @ref expression_linspace containing the integers `[0, stop)`
 */
template <typename T, bool precise = false, bool truncated = false, typename Tout = ftype<T>>
KFR_INTRINSIC expression_linspace<Tout, true> arange(T stop, cbool_t<truncated> = {})
{
    return linspace<T, precise>(static_cast<T>(0), stop, static_cast<size_t>(std::ceil(stop)), false, ctrue);
}

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_linspace<T, truncated>` expressions */
template <typename T, bool truncated, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_linspace<T, truncated>& self, const shape<1>& index,
                                     const axis_params<0, N>&)
{
    using TI = itype<T>;
    return mix((enumerate(vec_shape<T, N>()) + static_cast<T>(static_cast<TI>(index.front()))) * self.invsize,
               self.start, self.stop);
}
} // namespace KFR_ARCH_NAME

// ----------------------------------------------------------------------------

/**
 * @brief Expression that joins two expressions along a single axis.
 *
 * Indices in @c ConcatAxis below @c size1 are served by the first argument;
 * indices at or above @c size1 are served by the second argument (offset by @c size1).
 *
 * @tparam Arg1         first wrapped expression type
 * @tparam Arg2         second wrapped expression type
 * @tparam ConcatAxis   axis along which the expressions are joined
 */
template <typename Arg1, typename Arg2, index_t ConcatAxis>
struct expression_concatenate : public expression_with_arguments<Arg1, Arg2>
{
    static_assert(expression_dims<Arg1> == expression_dims<Arg2>);
    static_assert(std::is_same_v<expression_value_type<Arg1>, expression_value_type<Arg2>>);
    constexpr static index_t dims = expression_dims<Arg1>;
    shape<dims> size1;

    KFR_MEM_INTRINSIC expression_concatenate(Arg1&& arg1, Arg2&& arg2)
        : expression_with_arguments<Arg1, Arg2>{ std::forward<Arg1>(arg1), std::forward<Arg2>(arg2) },
          size1(expression_traits<Arg1>::get_shape(arg1))
    {
    }
};

template <typename Arg1, typename Arg2, index_t ConcatAxis>
struct expression_traits<expression_concatenate<Arg1, Arg2, ConcatAxis>> : expression_traits_defaults
{
    using ArgTraits1 = expression_traits<Arg1>;
    using ArgTraits2 = expression_traits<Arg2>;

    using value_type                    = typename ArgTraits1::value_type;
    constexpr static size_t dims        = ArgTraits1::dims;
    constexpr static bool random_access = ArgTraits1::random_access && ArgTraits2::random_access;

    KFR_INTRINSIC static shape<dims> concat_shape(const shape<dims>& sh1, const shape<dims>& sh2)
    {
        shape<dims> result = min(sh1, sh2);
        shape<dims> sum    = add_shape_undef(sh1, sh2);
        result[ConcatAxis] = sum[ConcatAxis];
        return result;
    }

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(
        const expression_concatenate<Arg1, Arg2, ConcatAxis>& self)
    {
        return concat_shape(ArgTraits1::get_shape(std::get<0>(self.args)),
                            ArgTraits2::get_shape(std::get<1>(self.args)));
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape()
    {
        return concat_shape(ArgTraits1::get_shape(), ArgTraits2::get_shape());
    }
};

/**
 * @brief Creates an expression that concatenates two input expressions along the given axis.
 *
 * @tparam ConcatAxis axis along which the expressions are joined (defaults to 0)
 * @param  arg1       first input expression
 * @param  arg2       second input expression
 * @return an @ref expression_concatenate
 */
template <index_t ConcatAxis = 0, input_expression Arg1, input_expression Arg2>
    requires expression_arguments<Arg1, Arg2>
KFR_INTRINSIC expression_concatenate<Arg1, Arg2, ConcatAxis> concatenate(Arg1&& arg1, Arg2&& arg2)
{
    return { std::forward<Arg1>(arg1), std::forward<Arg2>(arg2) };
}

/**
 * @brief Creates an expression that concatenates three input expressions along the given axis.
 *
 * Implemented as a left-associative concatenation: `concat(arg1, concat(arg2, arg3))`.
 *
 * @tparam ConcatAxis axis along which the expressions are joined (defaults to 0)
 * @param  arg1       first input expression
 * @param  arg2       second input expression
 * @param  arg3       third input expression
 * @return an @ref expression_concatenate
 */
template <index_t ConcatAxis = 0, input_expression Arg1, input_expression Arg2, input_expression Arg3>
    requires expression_arguments<Arg1, Arg2, Arg3>
KFR_INTRINSIC expression_concatenate<Arg1, expression_concatenate<Arg2, Arg3, ConcatAxis>, ConcatAxis>
concatenate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
{
    return { std::forward<Arg1>(arg1), { std::forward<Arg2>(arg2), std::forward<Arg3>(arg3) } };
}

inline namespace KFR_ARCH_NAME
{

/** Internal ADL-provided implementation for `expression_concatenate<Arg1, Arg2, ConcatAxis>` expressions */
template <typename Arg1, typename Arg2, index_t ConcatAxis, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<expression_concatenate<Arg1, Arg2, ConcatAxis>>::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_concatenate<Arg1, Arg2, ConcatAxis>& self,
                                     const shape<NDims>& index, const axis_params<Axis, N>& sh)
{
    const shape<NDims> size1 = self.size1;
    constexpr index_t Naxis  = ConcatAxis == Axis ? N : 1;
    if (index[ConcatAxis] >= size1[ConcatAxis])
    {
        shape index1 = index;
        index1[ConcatAxis] -= size1[ConcatAxis];
        return get_elements(std::get<1>(self.args), index1, sh);
    }
    else if (KFR_LIKELY(index[ConcatAxis] + Naxis <= size1[ConcatAxis]))
    {
        return get_elements(std::get<0>(self.args), index, sh);
    }
    else // (index < size1) && (index + N > size1)
    {
        vec<T, N> result;
        // Here Axis == ConcatAxis
        shape index1 = index;
        for (index_t i = 0; i < size1[ConcatAxis] - index[ConcatAxis]; ++i)
        {
            result[i] = get_elements(std::get<0>(self.args), index1, axis_params<Axis, 1>{})[0];
            ++index1[ConcatAxis];
        }
        index1[ConcatAxis] -= size1[ConcatAxis];
        for (index_t i = size1[ConcatAxis] - index[ConcatAxis]; i < N; ++i)
        {
            result[i] = get_elements(std::get<1>(self.args), index1, axis_params<Axis, 1>{})[0];
            ++index1[ConcatAxis];
        }
        return result;
    }
}

// ----------------------------------------------------------------------------

/**
 * @brief Type alias for the expression that packs several expressions into a single
 *        vector-valued expression.
 *
 * Implemented as an @ref expression_make_function with the @c fn::packtranspose functor.
 *
 * @tparam Args wrapped expression types
 */
template <typename... Args>
using expression_pack = expression_make_function<fn::packtranspose, Args...>;

/**
 * @brief Creates an expression that packs several input expressions into a single
 *        vector-valued expression.
 *
 * @tparam Args input expression types
 * @param  args input expressions
 * @return an @ref expression_pack
 */
template <typename... Args>
    requires expression_arguments<Args...>
KFR_INTRINSIC expression_pack<Args...> pack(Args&&... args)
{
    return { std::forward<Args>(args)... };
}

namespace internal
{
/** Internal ADL-provided implementation for `expression_function<fn::packtranspose, Args...>` expressions */
template <typename... Args, index_t Axis, size_t N,
          typename Tr = expression_traits<expression_function<fn::packtranspose, Args...>>, size_t... Indices>
KFR_INTRINSIC void set_elements_packed(expression_function<fn::packtranspose, Args...>& self,
                                       shape<Tr::dims> index, axis_params<Axis, N> sh,
                                       const vec<typename Tr::value_type, N>& x, csizes_t<Indices...>)
{
    constexpr size_t count          = sizeof...(Args);
    using ST                        = subtype<typename Tr::value_type>;
    const vec<vec<ST, N>, count> xx = vec<vec<ST, N>, count>::from_flatten(transpose<count>(flatten(x)));
    (set_elements(std::get<Indices>(self.args), index, sh, xx[Indices]), ...);
}
} // namespace internal

/** Internal ADL-provided implementation for `expression_function<fn::packtranspose, Args...>` expressions */
template <typename... Args, index_t Axis, size_t N,
          typename Tr = expression_traits<expression_function<fn::packtranspose, Args...>>>
KFR_INTRINSIC void set_elements(expression_function<fn::packtranspose, Args...>& self, shape<Tr::dims> index,
                                axis_params<Axis, N> sh,
                                const std::type_identity_t<vec<typename Tr::value_type, N>>& x)
{
    internal::set_elements_packed(self, index, sh, x, csizeseq<sizeof...(Args)>);
}

// ----------------------------------------------------------------------------

/**
 * @brief Expression that writes a single vector-valued input into several output expressions.
 *
 * Each lane of the input vector is dispatched to the corresponding output
 * expression via the @c set_elements path. Only supports being assigned to.
 *
 * @tparam E wrapped output expression types
 */
template <typename... E>
struct expression_unpack : expression_with_arguments<E...>, expression_traits_defaults
{
    constexpr static size_t count = sizeof...(E);

    using first_arg_traits = typename expression_with_arguments<E...>::first_arg_traits;

    constexpr static index_t dims = first_arg_traits::dims;
    using first_value_type        = typename first_arg_traits::value_type;

    using value_type = vec<first_value_type, count>;

    static_assert(((expression_dims<E> == dims) && ...));
    static_assert(((std::is_same_v<expression_value_type<E>, first_value_type>) && ...));

    constexpr static shape<dims> get_shape(const expression_unpack& self)
    {
        return first_arg_traits::get_shape(self.first());
    }
    constexpr static shape<dims> get_shape() { return first_arg_traits::get_shape(); }

    expression_unpack(E&&... e) : expression_with_arguments<E...>(std::forward<E>(e)...) {}

    template <index_t Axis, size_t N>
    KFR_INTRINSIC friend void set_elements(expression_unpack& self, shape<dims> index,
                                           axis_params<Axis, N> sh,
                                           const std::type_identity_t<vec<value_type, N>>& x)
    {
        self.output(index, sh, x, csizeseq<count>);
    }

    /**
     * @brief Assigns the values of @p input to all wrapped output expressions.
     */
    template <expression_argument Input>
    KFR_MEM_INTRINSIC expression_unpack& operator=(Input&& input)
    {
        process(*this, std::forward<Input>(input));
        return *this;
    }

private:
    template <index_t Axis, size_t N, size_t... indices>
    KFR_MEM_INTRINSIC void output(shape<dims> index, axis_params<Axis, N> sh, const vec<value_type, N>& x,
                                  csizes_t<indices...>)
    {
        const vec<vec<first_value_type, N>, count> xx =
            vec<vec<first_value_type, N>, count>::from_flatten(transpose<count>(flatten(x)));
        (set_elements(std::get<indices>(this->args), index, sh, xx[indices]), ...);
    }
};

// ----------------------------------------------------------------------------

/**
 * @brief Creates an expression that fans out a vector input into several output expressions.
 *
 * @tparam E output expression types
 * @param  e output expressions
 * @return an @ref expression_unpack
 */
template <output_expression... E>
KFR_FUNCTION expression_unpack<E...> unpack(E&&... e)
{
    return { std::forward<E>(e)... };
}

// ----------------------------------------------------------------------------

/**
 * @brief Expression that applies a binary function to each value and its previous value.
 *
 * The previous value is carried between calls via the mutable @c data member
 * (initialized to zero), so the expression is not random-access.
 *
 * @tparam Fn callable type (invoked as `fn(current, previous)`)
 * @tparam E  wrapped expression type
 */
template <typename Fn, typename E>
struct expression_adjacent : expression_with_traits<E>
{
    using value_type                           = typename expression_with_traits<E>::value_type;
    constexpr static inline index_t dims       = expression_with_traits<E>::dims;
    constexpr static inline bool random_access = false;

    expression_adjacent(Fn&& fn, E&& e)
        : expression_with_traits<E>(std::forward<E>(e)), fn(std::forward<Fn>(fn))
    {
    }

    template <size_t N, index_t VecAxis>
    KFR_INTRINSIC friend vec<value_type, N> get_elements(const expression_adjacent& self, shape<dims> index,
                                                         axis_params<VecAxis, N> sh)
    {
        const vec<value_type, N> in      = get_elements(self.first(), index, sh);
        const vec<value_type, N> delayed = insertleft(self.data, in);
        self.data                        = in.back();
        return self.fn(in, delayed);
    }
    Fn fn;
    mutable value_type data = value_type(0);
};

/**
 * @brief Creates an expression that returns the result of calling \f$ fn(x_i, x_{i-1}) \f$.
 *
 * @tparam Fn  binary callable applied to consecutive values
 * @tparam E1  input expression type
 * @param  fn  callable receiving `(current, previous)`
 * @param  e1  input expression
 * @return an @ref expression_adjacent
 */
template <typename Fn, typename E1>
KFR_INTRINSIC expression_adjacent<Fn, E1> adjacent(Fn&& fn, E1&& e1)
{
    return { std::forward<Fn>(fn), std::forward<E1>(e1) };
}

// ----------------------------------------------------------------------------

/**
 * @brief Expression that prints the requested values to the console for debugging purposes.
 *
 * Behaves as the identity for its value type and writes a one-line trace each
 * time a block of values is requested.
 *
 * @tparam E wrapped expression type
 */
template <typename E>
struct expression_trace : public expression_with_traits<E>
{
    using expression_with_traits<E>::expression_with_traits;
    using value_type                     = typename expression_with_traits<E>::value_type;
    constexpr static inline index_t dims = expression_with_traits<E>::dims;

    template <size_t N, index_t VecAxis>
    KFR_INTRINSIC friend vec<value_type, N> get_elements(const expression_trace& self, shape<dims> index,
                                                         axis_params<VecAxis, N> sh)
    {
        const vec<value_type, N> in = get_elements(self.first(), index, sh);
        println("[", kfr::fmt<'s', 16>(array_to_string(dims, index.data(), INT_MAX, INT_MAX, ",", "", "")),
                "] = ", in);
        return in;
    }
};

/**
 * @brief Creates an expression that prints the requested values to the console for debugging purposes.
 *
 * @tparam E1 input expression type
 * @param  e1 input expression
 * @return an @ref expression_trace
 */
template <typename E1>
KFR_INTRINSIC expression_trace<E1> trace(E1&& e1)
{
    return { std::forward<E1>(e1) };
}

// ----------------------------------------------------------------------------

/**
 * @brief Expression that adjusts the dimensionality of another expression.
 *
 * Extra leading dimensions are added with @ref infinite_size and broadcast the
 * underlying values when accessed along a new axis.
 *
 * @tparam Dims new dimensionality
 * @tparam E    wrapped expression type
 */
template <index_t Dims, typename E>
struct expression_dimensions : public expression_with_traits<E>
{
    using expression_with_traits<E>::expression_with_traits;
    using value_type                        = typename expression_with_traits<E>::value_type;
    constexpr static inline index_t in_dims = expression_with_traits<E>::dims;
    constexpr static inline index_t dims    = Dims;
    using first_arg_traits                  = typename expression_with_traits<E>::first_arg_traits;

    constexpr static shape<dims> get_shape(const expression_dimensions& self)
    {
        return first_arg_traits::get_shape(self.first()).template extend<dims>(infinite_size);
    }
    constexpr static shape<dims> get_shape()
    {
        return first_arg_traits::get_shape().template extend<dims>(infinite_size);
    }

    template <size_t N, index_t VecAxis>
    KFR_INTRINSIC friend vec<value_type, N> get_elements(const expression_dimensions& self, shape<dims> index,
                                                         axis_params<VecAxis, N> sh)
    {
        shape<in_dims> inindex = index.template trim<in_dims>();
        if constexpr (VecAxis >= in_dims)
        {
            return repeat<N>(get_elements(self.first(), inindex, axis_params_v<0, 1>));
        }
        else
        {
            return get_elements(self.first(), inindex, sh);
        }
    }
};

/**
 * @brief Creates an expression that adjusts the dimensionality of another expression.
 *
 * @tparam Dims new dimensionality
 * @tparam E1   input expression type
 * @param  e1   input expression
 * @return an @ref expression_dimensions
 */
template <index_t Dims, typename E1>
KFR_INTRINSIC expression_dimensions<Dims, E1> dimensions(E1&& e1)
{
    static_assert(Dims >= expression_dims<E1>, "Number of dimensions must be greater or equal");
    return { std::forward<E1>(e1) };
}

// ----------------------------------------------------------------------------

} // namespace KFR_ARCH_NAME
} // namespace kfr
