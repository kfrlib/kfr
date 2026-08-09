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

#include "../simd/platform.hpp"
#include "../simd/read_write.hpp"
#include "../simd/shuffle.hpp"
#include "../simd/vec.hpp"
#include "shape.hpp"

#include <tuple>
#include <complex>

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wparentheses")

namespace kfr
{

#ifndef KFR_CUSTOM_COMPLEX
/**
 * @brief Complex number type alias.
 *
 * Alias for `std::complex<T>`. Can be overridden by defining `KFR_CUSTOM_COMPLEX`
 * before including KFR headers.
 * @tparam T Element type of the complex number.
 */
template <typename T>
using complex = std::complex<T>;
#endif

/**
 * @brief Primary traits template for expressions.
 *
 * Specializations of this template provide the value type, dimensionality and
 * shape of an expression. The primary template is intentionally left undefined;
 * users and library code provide specializations for concrete expression types.
 * @tparam T Expression type to query.
 */
template <typename T>
struct expression_traits;

/**
 * @brief Helper alias that extracts the value type of an expression.
 * @tparam T Expression type.
 */
template <typename T>
using expression_value_type = typename expression_traits<T>::value_type;

/**
 * @brief Concept that is satisfied when `expression_traits<T>` is well-formed.
 * @tparam T Type to check.
 */
template <typename T>
concept has_expression_traits = requires {
    typename expression_traits<T>::value_type;
    { expression_traits<T>::dims } -> std::convertible_to<size_t>;
};

/**
 * @brief Inline variable holding the number of dimensions of an expression.
 * @tparam T Expression type.
 */
template <typename T>
constexpr inline size_t expression_dims = expression_traits<T>::dims;

/**
 * @brief Inline variable indicating whether an expression supports random access.
 * @tparam T Expression type.
 */
template <typename T>
constexpr inline bool expression_random_access = expression_traits<T>::random_access;

/**
 * @brief Returns the shape of an expression instance.
 * @param expr Expression instance to query.
 * @tparam T Expression type.
 * @return Shape of @p expr.
 */
template <typename T>
constexpr inline shape<expression_dims<T>> get_shape(T&& expr)
{
    return expression_traits<T>::get_shape(expr);
}

/**
 * @brief Returns the static shape of an expression type.
 * @tparam T Expression type.
 * @return Static shape of @p T.
 */
template <typename T>
constexpr inline shape<expression_dims<T>> get_shape()
{
    return expression_traits<T>::get_shape();
}

/**
 * @brief Pass-through specialization that forwards traits for const-qualified
 *        expression types.
 * @tparam T Expression type.
 */
template <has_expression_traits T>
struct expression_traits<const T> : expression_traits<T>
{
};
/**
 * @brief Pass-through specialization that forwards traits for lvalue reference
 *        expression types.
 * @tparam T Expression type.
 */
template <has_expression_traits T>
struct expression_traits<T&> : expression_traits<T>
{
};
/**
 * @brief Pass-through specialization that forwards traits for rvalue reference
 *        expression types.
 * @tparam T Expression type.
 */
template <has_expression_traits T>
struct expression_traits<T&&> : expression_traits<T>
{
};
/**
 * @brief Pass-through specialization that forwards traits for const lvalue
 *        reference expression types.
 * @tparam T Expression type.
 */
template <has_expression_traits T>
struct expression_traits<const T&> : expression_traits<T>
{
};
/**
 * @brief Pass-through specialization that forwards traits for const rvalue
 *        reference expression types.
 * @tparam T Expression type.
 */
template <has_expression_traits T>
struct expression_traits<const T&&> : expression_traits<T>
{
};

/**
 * @brief Adapter specialization that exposes the old-style expression interface
 *        (static `random_access` and `get_shape` members) through
 *        `expression_traits`.
 * @tparam T Old-style expression type.
 */
template <typename T>
    requires requires {
        T::random_access;
        T::get_shape();
    }
struct expression_traits<T>
{
    using value_type             = typename T::value_type;
    constexpr static size_t dims = T::dims;
    constexpr static shape<dims> get_shape(const T& self) { return T::get_shape(self); }
    constexpr static shape<dims> get_shape() { return T::get_shape(); }

    constexpr static inline bool explicit_operand = T::explicit_operand;
    constexpr static inline bool random_access    = T::random_access;
};

/**
 * @brief Base struct providing default values for expression traits.
 *
 * Deriving from this struct lets an expression implementation omit the
 * `explicit_operand` and `random_access` members, which default to `true`.
 */
struct expression_traits_defaults
{
    // using value_type = /* ... */;
    // constexpr static size_t dims = 0;
    // constexpr static shape<dims> get_shape(const T&);
    // constexpr static shape<dims> get_shape();

    constexpr static inline bool explicit_operand = true;
    constexpr static inline bool random_access    = true;
};

namespace internal_generic
{
template <typename... Xs>
using expressions_condition = std::void_t<expression_traits<Xs>...>;
template <typename... Xs>
using expressions_check = std::enable_if_t<(expression_traits<Xs>::explicit_operand || ...)>;
} // namespace internal_generic

/**
 * @brief Concept satisfied by expressions that can be read from.
 *
 * Requires that `get_elements` is callable on @p T with a zero-dimensional
 * index and a unit-width axis parameter.
 * @tparam T Type to check.
 */
// Input expression concept
template <typename T>
concept input_expression = has_expression_traits<T> && requires(T expr) {
    get_elements(expr, shape<expression_traits<T>::dims>(), axis_params<0, 1>{});
};

/**
 * @brief Concept satisfied by expressions that can be written to.
 *
 * Requires that `set_elements` is callable on @p T with a zero-dimensional
 * index, a unit-width axis parameter and a single-element vector.
 * @tparam T Type to check.
 */
// Output expression concept
template <typename T>
concept output_expression = has_expression_traits<T> && requires(T expr) {
    set_elements(expr, shape<expression_traits<T>::dims>(), axis_params<0, 1>{},
                 vec<typename expression_traits<T>::value_type, 1>{});
};

/**
 * @brief Concept satisfied by input expressions that must appear as an explicit
 *        operand (i.e. not implicitly convertible from a scalar argument).
 * @tparam E Type to check.
 */
template <typename E>
concept expression_argument = input_expression<E> && expression_traits<E>::explicit_operand;

/**
 * @brief Concept satisfied when all @p E are input expressions and at least one
 *        of them is an explicit operand.
 * @tparam E Pack of types to check.
 */
template <typename... E>
concept expression_arguments = (input_expression<E> && ...) && (expression_argument<E> || ...);

/**
 * @brief Concept satisfied by expressions that are both readable and writable.
 * @tparam T Type to check.
 */
template <typename T>
concept input_output_expression = input_expression<T> && output_expression<T>;

/**
 * @brief Inline variable that is `true` when @p E satisfies `input_expression`.
 * @tparam E Type to check.
 */
template <typename E>
constexpr inline bool is_input_expression = input_expression<E>;

/**
 * @brief Inline variable that is `true` when @p E satisfies `output_expression`.
 * @tparam E Type to check.
 */
template <typename E>
constexpr inline bool is_output_expression = output_expression<E>;

/**
 * @brief Inline variable that is `true` when @p E satisfies
 *        `input_output_expression`.
 * @tparam E Type to check.
 */
template <typename E>
constexpr inline bool is_input_output_expression = input_output_expression<E>;

/**
 * @brief Inline variable that is `true` when @p T is a non-cv-qualified scalar
 *        vector element type.
 * @tparam T Type to check.
 */
template <typename T>
constexpr inline bool is_expr_element = std::is_same_v<std::remove_cv_t<T>, T> && is_vec_element<T>;

/**
 * @brief Concept satisfied by scalar vector element types.
 * @tparam T Type to check.
 */
template <typename T>
concept expr_element = is_expr_element<T>;

/**
 * @brief Inline variable that is `true` when the shape of @p E is infinite.
 * @tparam E Expression type.
 */
template <typename E>
constexpr inline bool is_infinite = expression_traits<E>::get_shape().has_infinity();

/**
 * @brief Specialization of `expression_traits` for scalar vector element types.
 *
 * A scalar element is treated as a zero-dimensional expression whose shape is
 * empty and which is not an explicit operand (it can be used as an implicit
 * argument to expression functions).
 * @tparam T Scalar element type.
 */
template <expr_element T>
struct expression_traits<T> : expression_traits_defaults
{
    using value_type                              = T;
    constexpr static size_t dims                  = 0;
    constexpr static inline bool explicit_operand = false;

    KFR_MEM_INTRINSIC constexpr static shape<0> get_shape(const T& self) { return {}; }
    KFR_MEM_INTRINSIC constexpr static shape<0> get_shape() { return {}; }
};

/**
 * @brief Reads a single element from an expression at the given index.
 * @param expr Expression to read from.
 * @param index Multi-dimensional index of the element.
 * @tparam E Expression type.
 * @tparam Dims Number of dimensions of @p expr.
 * @return The element value at @p index.
 */
template <input_expression E, index_t Dims = expression_dims<E>>
inline expression_value_type<E> get_element(E&& expr, shape<Dims> index)
{
    return get_elements(expr, index, axis_params_v<0, 1>).front();
}

/**
 * @brief Computes the vector of indices along an axis for a vectorized read.
 *
 * When @p Axis matches @p VecAxis the returned vector contains consecutive
 * indices starting at `index[Axis]`; otherwise every lane holds the scalar
 * `index[Axis]`.
 * @param index Multi-dimensional base index.
 * @tparam Axis Axis being read.
 * @tparam Dims Number of dimensions of @p index.
 * @tparam VecAxis Axis along which the read is vectorized.
 * @tparam N Vector width.
 * @return Vector of indices for the requested axis.
 */
template <index_t Axis, index_t Dims, index_t VecAxis, size_t N>
inline vec<index_t, N> indices(const shape<Dims>& index, axis_params<VecAxis, N>)
{
    if constexpr (Axis == VecAxis)
        return index[Axis] + enumerate<index_t, N, 0, 1>();
    else
        return index[Axis];
}

namespace internal_generic
{
struct anything
{
    template <typename Expr>
    constexpr anything(Expr&&)
    {
    }
};
} // namespace internal_generic

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Converts a shape to a vector of unsigned integers.
 * @param sh Shape to convert.
 * @tparam Dims Number of dimensions.
 * @tparam U Unsigned integer type used for the vector lanes.
 * @return Vector holding the dimension sizes of @p sh.
 */
template <index_t Dims, typename U = unsigned_type<sizeof(index_t) * 8>>
KFR_INTRINSIC vec<U, Dims> to_vec(const shape<Dims>& sh)
{
    return read<Dims>(reinterpret_cast<const U*>(sh.data()));
}

/**
 * @brief No-op pass begin/end hooks for the `anything` placeholder type.
 *
 * These overloads allow `begin_pass`/`end_pass` to be called uniformly on any
 * argument, including those that do not require pass notifications.
 * @param start Starting index of the pass.
 * @param stop Ending index of the pass.
 * @tparam Dims Number of dimensions of the pass range.
 */
template <index_t Dims>
KFR_INTRINSIC void begin_pass(const internal_generic::anything&, shape<Dims> start, shape<Dims> stop)
{
}
/**
 * @copydoc begin_pass(const internal_generic::anything&, shape<Dims>, shape<Dims>)
 */
template <index_t Dims>
KFR_INTRINSIC void end_pass(const internal_generic::anything&, shape<Dims> start, shape<Dims> stop)
{
}

/**
 * @brief No-op reset hook for the `anything` placeholder type.
 *
 * This overload allows `reset` to be called uniformly on any argument, including
 * those that do not require reset notifications.
 */
template <int dummy = 0>
KFR_INTRINSIC void reset(const internal_generic::anything&)
{
}

/**
 * @brief Returns a scalar element broadcast to a vector.
 *
 * When the expression is a scalar element, reading any index simply returns
 * the scalar value repeated across all lanes of the result vector.
 * @param self Scalar element value.
 * @param index Unused zero-dimensional index.
 * @tparam T Scalar element type.
 * @tparam Axis Axis parameter (unused).
 * @tparam N Vector width.
 * @return Vector with @p N lanes all equal to @p self.
 */
template <typename T, index_t Axis, size_t N>
    requires(is_expr_element<std::decay_t<T>>)
KFR_INTRINSIC vec<std::decay_t<T>, N> get_elements(T&& self, const shape<0>& index,
                                                   const axis_params<Axis, N>&)
{
    return self;
}
/**
 * @brief Stores a single-element vector into a scalar element.
 *
 * When the expression is a scalar element, writing a unit-width vector assigns
 * the scalar value. Wider writes are not supported.
 * @param self Scalar element lvalue to assign to.
 * @param index Unused zero-dimensional index.
 * @param val Vector value to store (must have width 1).
 * @tparam T Scalar element type.
 * @tparam Axis Axis parameter (unused).
 * @tparam N Vector width (must be 1).
 */
template <typename T, index_t Axis, size_t N>
    requires(is_expr_element<std::decay_t<T>>)
KFR_INTRINSIC void set_elements(T& self, const shape<0>& index, const axis_params<Axis, N>&,
                                const std::type_identity_t<vec<T, N>>& val)
{
    static_assert(N == 1);
    static_assert(!std::is_const_v<T>);
    self = val.front();
}

/**
 * @brief Inline variable that is `true` when @p T (after decay) is a numeric or
 *        boolean type and can therefore be passed by value as an argument.
 * @tparam T Type to check.
 */
template <typename T>
constexpr inline bool is_arg = is_numeric_or_bool<std::decay_t<T>>;

/**
 * @brief Alias that decays numeric/boolean argument types while preserving
 *        expression types unchanged.
 * @tparam T Argument type.
 */
template <typename T>
using arg = std::conditional_t<is_arg<T>, std::decay_t<T>, T>;

/**
 * @brief Holds the arguments of an expression along with their dimension masks.
 *
 * Stores a tuple of arguments and an array of masks used to adapt indices when
 * the arguments have different dimensionalities. Provides helpers to access the
 * first argument, query per-argument masks, and fold a callable over all
 * arguments.
 * @tparam Args Argument types stored by the expression.
 */
template <typename... Args>
struct expression_with_arguments
{
    /// Number of arguments held by the expression.
    constexpr static size_t count = sizeof...(Args);

    using type_list = ctypes_t<Args...>;

    /// Alias for the type of the argument at position @p idx.
    template <size_t idx>
    using nth = typename type_list::template nth<idx>;

    /// Alias for the type of the first argument.
    using first_arg = typename type_list::template nth<0>;

    /// Alias for the traits of the argument at position @p idx.
    template <size_t idx>
    using nth_trait = expression_traits<typename type_list::template nth<idx>>;

    /// Traits of the first argument.
    using first_arg_traits = expression_traits<first_arg>;

    /// Tuple holding all arguments.
    std::tuple<Args...> args;
    /// Per-argument dimension masks computed from each argument's shape.
    std::array<dimset, count> masks;

    /**
     * @brief Returns a reference to the first argument.
     * @return Reference to the first stored argument.
     */
    KFR_INTRINSIC auto& first() { return std::get<0>(args); }
    /**
     * @brief Returns a const reference to the first argument.
     * @return Const reference to the first stored argument.
     */
    KFR_INTRINSIC const auto& first() const { return std::get<0>(args); }

    /**
     * @brief Returns the dimension mask for the argument at position @p idx.
     *
     * For zero-dimensional or single-argument expressions a sentinel mask of
     * `-1` is returned. Otherwise the static shape mask is used when available,
     * falling back to the runtime mask stored in @ref masks.
     * @param idx Compile-time index of the argument.
     * @return Dimension mask for the requested argument.
     */
    template <size_t idx>
    KFR_INTRINSIC dimset getmask(csize_t<idx> = {}) const
    {
        static_assert(idx < count);
        using Traits = expression_traits<nth<idx>>;
        if constexpr (sizeof...(Args) <= 1 || Traits::dims == 0)
        {
            return dimset(-1);
        }
        else
        {
            if constexpr (Traits::get_shape().product() > 0)
            {
                return Traits::get_shape().tomask();
            }
            else
            {
                return std::get<idx>(masks);
            }
        }
    }

    /**
     * @brief Folds a callable over the stored arguments, passing them by value.
     * @param fn Callable invoked as `fn(args...)`.
     * @return Result of `fn(args...)`.
     */
    template <typename Fn>
    KFR_INTRINSIC constexpr auto fold(Fn&& fn) const
    {
        return fold_impl(std::forward<Fn>(fn), csizeseq<count>);
    }
    /**
     * @brief Folds a callable over compile-time indices of the arguments.
     * @param fn Callable invoked as `fn(csize<0>, csize<1>, ...)`.
     * @return Result of the fold.
     */
    template <typename Fn>
    KFR_INTRINSIC constexpr static auto fold_idx(Fn&& fn)
    {
        return fold_idx_impl(std::forward<Fn>(fn), csizeseq<count>);
    }

    /**
     * @brief Constructs the expression from a pack of arguments.
     *
     * Numeric/boolean arguments are decayed before being stored. The per-argument
     * masks are computed from each argument's shape at construction time.
     * @param args Arguments to store.
     */
    KFR_INTRINSIC expression_with_arguments(arg<Args&&>... args) : args{ std::forward<Args>(args)... }
    {
        cforeach(csizeseq<count>,
                 [&](auto idx_) KFR_INLINE_LAMBDA
                 {
                     constexpr size_t idx = val_of(decltype(idx_)());
                     shape sh             = expression_traits<nth<idx>>::get_shape(std::get<idx>(this->args));
                     masks[idx]           = sh.tomask();
                 });
    }

private:
    template <typename Fn, size_t... indices>
    KFR_INTRINSIC constexpr auto fold_impl(Fn&& fn, csizes_t<indices...>) const
    {
        return fn(std::get<indices>(args)...);
    }
    template <typename Fn, size_t... indices>
    KFR_INTRINSIC constexpr static auto fold_idx_impl(Fn&& fn, csizes_t<indices...>)
    {
        return fn(csize<indices>...);
    }
};

/**
 * @brief Single-argument specialization of `expression_with_arguments`.
 *
 * Simplifies storage and mask handling when an expression has exactly one
 * argument: the mask is always the sentinel `-1`.
 * @tparam Arg Argument type.
 */
template <typename Arg>
struct expression_with_arguments<Arg>
{
    /// Number of arguments held by the expression (always 1).
    constexpr static size_t count = 1;

    using type_list = ctypes_t<Arg>;

    /// Alias for the argument type (any index).
    template <size_t idx>
    using nth = Arg;

    /// Alias for the argument type.
    using first_arg = Arg;

    /// Alias for the traits of the argument.
    template <size_t idx>
    using nth_trait = expression_traits<Arg>;

    /// Traits of the argument.
    using first_arg_traits = expression_traits<first_arg>;

    /// Tuple holding the single argument.
    std::tuple<Arg> args;

    /**
     * @brief Returns a reference to the stored argument.
     * @return Reference to the stored argument.
     */
    KFR_MEM_INTRINSIC auto& first() { return std::get<0>(args); }
    /**
     * @brief Returns a const reference to the stored argument.
     * @return Const reference to the stored argument.
     */
    KFR_MEM_INTRINSIC const auto& first() const { return std::get<0>(args); }

    /**
     * @brief Returns the sentinel dimension mask for the single argument.
     * @param idx Compile-time index (ignored).
     * @return Sentinel mask of `-1`.
     */
    template <size_t idx>
    KFR_MEM_INTRINSIC dimset getmask(csize_t<idx> = {}) const
    {
        return dimset(-1);
    }

    /**
     * @brief Folds a callable over the stored argument.
     * @param fn Callable invoked as `fn(arg)`.
     * @return Result of `fn(arg)`.
     */
    template <typename Fn>
    KFR_MEM_INTRINSIC constexpr auto fold(Fn&& fn) const
    {
        return fold_impl(std::forward<Fn>(fn), csizeseq<count>);
    }
    /**
     * @brief Folds a callable over the compile-time index of the argument.
     * @param fn Callable invoked as `fn(csize<0>)`.
     * @return Result of the fold.
     */
    template <typename Fn>
    KFR_INTRINSIC constexpr static auto fold_idx(Fn&& fn)
    {
        return fold_idx_impl(std::forward<Fn>(fn), csizeseq<count>);
    }

    /**
     * @brief Constructs the expression from a single argument.
     * @param arg Argument to store.
     */
    KFR_MEM_INTRINSIC expression_with_arguments(Arg&& arg) : args{ std::forward<Arg>(arg) } {}

private:
    template <typename Fn, size_t... indices>
    KFR_MEM_INTRINSIC constexpr auto fold_impl(Fn&& fn, csizes_t<indices...>) const
    {
        return fn(std::get<indices>(args)...);
    }
    template <typename Fn, size_t... indices>
    KFR_INTRINSIC constexpr static auto fold_idx_impl(Fn&& fn, csizes_t<indices...>)
    {
        return fn(csize<indices>...);
    }
};

/**
 * @brief Deduction guide for `expression_with_arguments`.
 * @tparam Args Argument types.
 */
template <typename... Args>
expression_with_arguments(Args&&... args) -> expression_with_arguments<Args...>;

/**
 * @brief Wraps a single-argument expression and forwards its traits.
 *
 * Derives from `expression_with_arguments<Arg>` and provides `expression_traits`
 * members (`value_type`, `dims`, `get_shape`) by forwarding to the wrapped
 * argument's traits. The wrapped expression is an explicit operand with random
 * access.
 * @tparam Arg Argument type.
 */
template <typename Arg>
struct expression_with_traits : expression_with_arguments<Arg>
{
    constexpr static inline bool explicit_operand = true;
    constexpr static inline bool random_access    = true;

    using first_arg_traits       = expression_traits<Arg>;
    using value_type             = typename first_arg_traits::value_type;
    constexpr static size_t dims = first_arg_traits::dims;
    constexpr static shape<dims> get_shape(const expression_with_traits& self)
    {
        return first_arg_traits::get_shape(self.first());
    }
    constexpr static shape<dims> get_shape() { return first_arg_traits::get_shape(); }

    using expression_with_arguments<Arg>::expression_with_arguments;
};

/**
 * @brief Expression that applies a callable to its arguments.
 *
 * Combines `expression_with_arguments` with `expression_traits_defaults` and
 * stores a callable @p Fn. The value type is deduced from the result of invoking
 * @p Fn with unit-width vectors of each argument's value type. The dimensionality
 * is the maximum of the arguments' dimensionalities, and the shape is the common
 * shape of all arguments.
 * @tparam Fn Callable type.
 * @tparam Args Argument types.
 */
template <typename Fn, typename... Args>
struct expression_function : expression_with_arguments<Args...>, expression_traits_defaults
{
    /// Value type produced by the expression.
    using value_type =
        typename std::invoke_result_t<Fn,
                                      vec<typename expression_traits<Args>::value_type, 1>...>::value_type;
    /// Number of dimensions of the expression.
    constexpr static size_t dims = std::max({ expression_traits<Args>::dims... });

#if defined KFR_COMPILER_IS_MSVC || defined KFR_COMPILER_GCC
    struct lambda_get_shape
    {
        template <size_t... idx>
        constexpr auto operator()(csize_t<idx>...) const
        {
            return internal_generic::common_shape(
                expression_traits<typename expression_function::template nth<idx>>::get_shape()...);
        }
    };
    struct lambda_get_shape_self
    {
        const expression_function& self;
        template <typename... TArgs>
        constexpr auto operator()(const TArgs&... args) const
        {
            return internal_generic::common_shape<true>(expression_traits<Args>::get_shape(args)...);
        }
    };
    /**
     * @brief Returns the shape of the expression instance.
     *
     * Computes the common shape of all arguments at runtime.
     * @param self Expression instance.
     * @return Common shape of the arguments.
     */
    constexpr static shape<dims> get_shape(const expression_function& self)
    {
        return self.fold(lambda_get_shape_self{ self });
    }
    /**
     * @brief Returns the static shape of the expression type.
     *
     * Computes the common shape of all arguments at compile time.
     * @return Common shape of the arguments.
     */
    constexpr static shape<dims> get_shape() { return expression_function::fold_idx(lambda_get_shape{}); }
#else
    /**
     * @brief Returns the shape of the expression instance.
     *
     * Computes the common shape of all arguments at runtime.
     * @param self Expression instance.
     * @return Common shape of the arguments.
     */
    constexpr static shape<dims> get_shape(const expression_function& self)
    {
        return self.fold(
            [&](auto&&... args) KFR_INLINE_LAMBDA constexpr -> auto
            {
                return internal_generic::common_shape<true>(
                    expression_traits<decltype(args)>::get_shape(args)...);
            });
    }
    /**
     * @brief Returns the static shape of the expression type.
     *
     * Computes the common shape of all arguments at compile time.
     * @return Common shape of the arguments.
     */
    constexpr static shape<dims> get_shape()
    {
        return expression_function::fold_idx(
            [&](auto... args) KFR_INLINE_LAMBDA constexpr -> auto
            {
                return internal_generic::common_shape(
                    expression_traits<typename expression_function::template nth<val_of(decltype(args)())>>::
                        get_shape()...);
            });
    }
#endif

    /// `true` when all arguments support random access.
    constexpr static inline bool random_access = (expression_traits<Args>::random_access && ...);

    /// Callable applied to the arguments.
    Fn fn;

    /**
     * @brief Constructs the expression from an argument holder and a callable.
     * @param args Argument holder.
     * @param fn Callable to apply.
     */
    KFR_MEM_INTRINSIC expression_function(expression_with_arguments<Args...> args, Fn&& fn)
        : expression_with_arguments<Args...>{ std::move(args) }, fn(std::forward<Fn>(fn))
    {
    }
    /**
     * @brief Constructs the expression from a callable and a pack of arguments.
     * @param fn Callable to apply.
     * @param args Arguments to store.
     */
    KFR_MEM_INTRINSIC expression_function(Fn&& fn, arg<Args&&>... args)
        : expression_with_arguments<Args...>{ std::forward<Args>(args)... }, fn(std::forward<Fn>(fn))
    {
    }
    /**
     * @brief Constructs the expression from a pack of arguments with a
     *        default-constructed callable.
     * @param args Arguments to store.
     */
    KFR_MEM_INTRINSIC expression_function(arg<Args&&>... args)
        : expression_with_arguments<Args...>{ std::forward<Args>(args)... }, fn{}
    {
    }

    /**
     * @brief Processes an input expression into this output expression.
     * @param in Input expression to read from.
     * @return Reference to `*this`.
     */
    template <input_expression In>
    expression_function& operator=(In&& in)
    {
        static_assert(is_output_expression<expression_function>);
        process(*this, std::forward<In>(in));
        return *this;
    }
};

/**
 * @brief Deduction guide for constructing an `expression_function` from an
 *        argument holder and a callable.
 */
template <typename... Args, typename Fn>
expression_function(const expression_with_arguments<Args...>& args, Fn&& fn)
    -> expression_function<Fn, Args...>;
/**
 * @brief Deduction guide for constructing an `expression_function` from an
 *        rvalue argument holder and a callable.
 */
template <typename... Args, typename Fn>
expression_function(expression_with_arguments<Args...>&& args, Fn&& fn) -> expression_function<Fn, Args...>;
/**
 * @brief Deduction guide for constructing an `expression_function` from an
 *        lvalue argument holder and a callable.
 */
template <typename... Args, typename Fn>
expression_function(expression_with_arguments<Args...>& args, Fn&& fn) -> expression_function<Fn, Args...>;

/**
 * @brief Alias that produces an `expression_function` with decayed argument
 *        types.
 * @tparam Fn Callable type.
 * @tparam Args Argument types.
 */
template <typename Fn, typename... Args>
using expression_make_function = expression_function<Fn, arg<Args>...>;

namespace internal
{

template <typename... Args, index_t Dims, size_t... idx>
KFR_INTRINSIC void begin_pass_args(const expression_with_arguments<Args...>& self, shape<Dims> start,
                                   shape<Dims> stop, csizes_t<idx...>)
{
    (begin_pass(std::get<idx>(self.args), start, stop), ...);
}

template <typename... Args, index_t Dims, size_t... idx>
KFR_INTRINSIC void end_pass_args(const expression_with_arguments<Args...>& self, shape<Dims> start,
                                 shape<Dims> stop, csizes_t<idx...>)
{
    (end_pass(std::get<idx>(self.args), start, stop), ...);
}

template <typename... Args, size_t... idx>
KFR_INTRINSIC void reset_args(const expression_with_arguments<Args...>& self, csizes_t<idx...>)
{
    (reset(std::get<idx>(self.args)), ...);
}

template <index_t outdims, typename Fn, typename... Args, index_t VecAxis, size_t N, index_t Dims, size_t idx,
          typename Traits = expression_traits<typename expression_function<Fn, Args...>::template nth<idx>>>
KFR_MEM_INTRINSIC vec<typename Traits::value_type, N> get_arg(const expression_function<Fn, Args...>& self,
                                                              const shape<Dims>& index,
                                                              const axis_params<VecAxis, N>& sh, csize_t<idx>)
{
    if constexpr (Traits::dims == 0)
    {
        return repeat<N>(get_elements(std::get<idx>(self.args), {}, axis_params<0, 1>{}));
    }
    else
    {
        constexpr size_t NewVecAxis = Traits::dims - (Dims - VecAxis);
        auto indices                = internal_generic::adapt<Traits::dims>(index, self.getmask(csize<idx>));
        constexpr index_t last_dim  = Traits::get_shape().back();
        if constexpr (last_dim != undefined_size)
        {
            constexpr index_t last_dim_pot = prev_poweroftwo(last_dim);
            return repeat<N / std::min(last_dim_pot, static_cast<index_t>(N))>(
                get_elements(std::get<idx>(self.args), indices,
                             axis_params<NewVecAxis, std::min(last_dim_pot, static_cast<index_t>(N))>{}));
        }
        else
        {
            if constexpr (sizeof...(Args) > 1 && N > 1)
            {
                if (KFR_UNLIKELY(self.masks[idx].back() == 0))
                    return get_elements(std::get<idx>(self.args), indices, axis_params<NewVecAxis, 1>{})
                        .front();
                else
                    return get_elements(std::get<idx>(self.args), indices, axis_params<NewVecAxis, N>{});
            }
            else
            {
                return get_elements(std::get<idx>(self.args), indices, axis_params<NewVecAxis, N>{});
            }
        }
    }
}
} // namespace internal

/**
 * @brief Notifies all arguments of an `expression_with_arguments` that a pass
 *        is beginning.
 * @param self Argument holder.
 * @param start Starting index of the pass.
 * @param stop Ending index of the pass.
 * @tparam Args Argument types.
 * @tparam Dims Number of dimensions of the pass range.
 */
template <typename... Args, index_t Dims>
KFR_INTRINSIC void begin_pass(const expression_with_arguments<Args...>& self, shape<Dims> start,
                              shape<Dims> stop)
{
    internal::begin_pass_args(self, start, stop, indicesfor<Args...>);
}

/**
 * @brief Notifies all arguments of an `expression_with_arguments` that a pass
 *        has ended.
 * @param self Argument holder.
 * @param start Starting index of the pass.
 * @param stop Ending index of the pass.
 * @tparam Args Argument types.
 * @tparam Dims Number of dimensions of the pass range.
 */
template <typename... Args, index_t Dims>
KFR_INTRINSIC void end_pass(const expression_with_arguments<Args...>& self, shape<Dims> start,
                            shape<Dims> stop)
{
    internal::end_pass_args(self, start, stop, indicesfor<Args...>);
}

/**
 * @brief Resets all arguments of an `expression_with_arguments`.
 * @param self Argument holder.
 * @tparam Args Argument types.
 * @tparam Dims Number of dimensions of the reset operation.
 */
template <typename... Args>
KFR_INTRINSIC void reset(const expression_with_arguments<Args...>& self)
{
    internal::reset_args(self, indicesfor<Args...>);
}

/**
 * @brief Reads a vector of elements from an `expression_function`.
 *
 * Gathers the argument vectors via `internal::get_arg` and applies the stored
 * callable to produce the result vector.
 * @param self Expression instance.
 * @param index Multi-dimensional index of the first element to read.
 * @param sh Axis and width of the vectorized read.
 * @tparam Fn Callable type.
 * @tparam Args Argument types.
 * @tparam Axis Axis along which the read is vectorized.
 * @tparam N Vector width.
 * @tparam Dims Number of dimensions of @p index.
 * @return Vector of @p N elements produced by the callable.
 */
template <typename Fn, typename... Args, index_t Axis, size_t N, index_t Dims,
          typename Tr = expression_traits<expression_function<Fn, Args...>>,
          typename T  = typename Tr::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_function<Fn, Args...>& self, const shape<Dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return self.fold_idx([&](auto... idx) KFR_INLINE_LAMBDA -> vec<T, N>
                         { return self.fn(internal::get_arg<Tr::dims>(self, index, sh, idx)...); });
}

/**
 * @brief Core loop body that copies a vectorized slice from @p in to @p out.
 *
 * Processes the range `[start, stop)` along the output axis using a primary
 * width @p w and a tail width @p gw. When the input is zero-dimensional the
 * scalar value is broadcast; otherwise the input is read vectorized along the
 * corresponding input axis, clamping the index to the input size.
 * @param out Output expression.
 * @param in Input expression.
 * @param start First index to process (along the output axis).
 * @param stop One-past-the-last index to process.
 * @param insize Size of the input along the input axis.
 * @param outidx Output multi-dimensional index (output axis component updated in place).
 * @param inidx Input multi-dimensional index (input axis component updated in place).
 * @tparam Out Output expression type.
 * @tparam In Input expression type.
 * @tparam OutAxis Output axis being vectorized.
 * @tparam w Primary vector width.
 * @tparam gw Tail (granularity) vector width.
 * @tparam Tin Input value type.
 * @tparam outdims Number of output dimensions.
 * @tparam indims Number of input dimensions.
 */
template <typename Out, typename In, index_t OutAxis, size_t w, size_t gw, typename Tin, index_t outdims,
          index_t indims>
KFR_INTRINSIC static void tprocess_body(Out&& out, In&& in, size_t start, size_t stop, size_t insize,
                                        shape<outdims> outidx, shape<indims> inidx)
{
    if constexpr (indims == 0)
    {
        size_t x              = start;
        const vec<Tin, 1> val = get_elements(in, inidx, axis_params_v<0, 1>);
        if constexpr (w > gw)
        {
            const auto valvec = repeat<w>(val);
            KFR_LOOP_NOUNROLL
            for (; x < stop / w * w; x += w)
            {
                outidx[OutAxis] = x;
                set_elements(out, outidx, axis_params_v<OutAxis, w>, valvec);
            }
        }
        const auto valvec = repeat<gw>(val);
        KFR_LOOP_NOUNROLL
        for (; x < stop / gw * gw; x += gw)
        {
            outidx[OutAxis] = x;
            set_elements(out, outidx, axis_params_v<OutAxis, gw>, valvec);
        }
    }
    else
    {
        constexpr index_t InAxis = OutAxis + indims - outdims;
        size_t x                 = start;
        if constexpr (w > gw)
        {
            KFR_LOOP_NOUNROLL
            for (; x < stop / w * w; x += w)
            {
                outidx[OutAxis] = x;
                inidx[InAxis]   = std::min(x, insize - 1);
                auto v          = get_elements(in, inidx, axis_params_v<InAxis, w>);
                // println("## i=", x, "\n", v);
                set_elements(out, outidx, axis_params_v<OutAxis, w>, v);
            }
        }
        KFR_LOOP_NOUNROLL
        for (; x < stop / gw * gw; x += gw)
        {
            outidx[OutAxis] = x;
            inidx[InAxis]   = std::min(x, insize - 1);
            set_elements(out, outidx, axis_params_v<OutAxis, gw>,
                         get_elements(in, inidx, axis_params_v<InAxis, gw>));
        }
    }
}

/**
 * @brief Processes a zero-dimensional input expression into a zero-dimensional
 *        output expression.
 *
 * Reads a single element from @p in and writes it to @p out, bracketing the
 * transfer with `begin_pass`/`end_pass` calls.
 * @param out Output expression.
 * @param in Input expression.
 * @tparam width Vector width hint (0 = automatic).
 * @tparam Axis Axis hint (ignored for zero-dimensional expressions).
 * @tparam Out Output expression type.
 * @tparam In Input expression type.
 * @tparam gw Tail granularity.
 * @return Empty shape.
 */
template <size_t width = 0, index_t Axis = 0, typename Out, typename In, size_t gw = 1>
    requires(expression_traits<Out>::dims == 0)
static auto process(Out&& out, In&& in, shape<0> = {}, shape<0> = {}, csize_t<gw> = {}) -> shape<0>
{
    static_assert(is_input_expression<In>, "In must be an input expression");
    static_assert(is_output_expression<Out>, "Out must be an output expression");
    static_assert(expression_traits<In>::dims == 0);
    begin_pass(out, shape{}, shape{});
    begin_pass(in, shape{}, shape{});
    set_elements(out, shape<0>{}, axis_params_v<0, 1>, get_elements(in, shape<0>{}, axis_params_v<0, 1>));
    end_pass(in, shape{}, shape{});
    end_pass(out, shape{}, shape{});
    return {};
}

namespace internal
{

constexpr KFR_INTRINSIC size_t select_process_width(size_t width, size_t vec_width, index_t last_dim_size)
{
    if (width != 0)
        return width;
    if (last_dim_size == 0)
        return vec_width;

    return std::min(vec_width, static_cast<size_t>(last_dim_size));
}

constexpr KFR_INTRINSIC index_t select_axis(index_t ndims, index_t axis)
{
    if (axis >= ndims)
        return ndims - 1;
    return axis;
}

template <index_t VecAxis, index_t LoopAxis, index_t outdims>
KFR_INTRINSIC index_t axis_start(const shape<outdims>& sh)
{
    static_assert(VecAxis < outdims);
    static_assert(LoopAxis < outdims);
    if constexpr (VecAxis == LoopAxis)
        return 0;
    else
        return sh[LoopAxis];
}
template <index_t VecAxis, index_t LoopAxis, index_t outdims>
KFR_INTRINSIC index_t axis_stop(const shape<outdims>& sh)
{
    static_assert(VecAxis < outdims);
    static_assert(LoopAxis < outdims);
    if constexpr (VecAxis == LoopAxis)
        return 1;
    else
        return sh[LoopAxis];
}

} // namespace internal

/**
 * @brief Processes a multi-dimensional input expression into a multi-dimensional
 *        output expression.
 *
 * Copies elements from @p in to @p out over the region defined by @p start and
 * @p size, vectorized along @p Axis with width @p width. The loop is unrolled
 * for 1- to 4-dimensional outputs; higher dimensionalities use a generic
 * index-increment loop. Returns the shape of the processed region, or a shape
 * of `0` if the output and input shapes are incompatible.
 * @param out Output expression.
 * @param in Input expression.
 * @param start Starting index of the region to process.
 * @param size Number of elements to process along each dimension.
 * @tparam width Vector width hint (0 = automatic).
 * @tparam Axis Axis along which to vectorize (defaults to the last axis).
 * @tparam Out Output expression type.
 * @tparam In Input expression type.
 * @tparam gw Tail granularity.
 * @tparam outdims Number of output dimensions.
 * @return Shape of the processed region.
 */
template <size_t width = 0, index_t Axis = infinite_size, typename Out, typename In, size_t gw = 1,
          index_t outdims = expression_dims<Out>>
    requires(expression_dims<Out> > 0)
static auto process(Out&& out, In&& in, shape<outdims> start = shape<outdims>(0),
                    shape<outdims> size = shape<outdims>(infinite_size), csize_t<gw> = {}) -> shape<outdims>
{
    static_assert(is_input_expression<In>, "In must be an input expression");
    static_assert(is_output_expression<Out>, "Out must be an output expression");

    using Trin  = expression_traits<In>;
    using Trout = expression_traits<Out>;
    using Tin   = typename Trin::value_type;

    using internal::axis_start;
    using internal::axis_stop;

    constexpr index_t indims = expression_dims<In>;
    static_assert(outdims >= indims);

    constexpr index_t last_dim_size = prev_poweroftwo(Trout::get_shape().back());

#ifdef NDEBUG
    constexpr size_t vec_width = maximum_vector_size<Tin>;
#else
    constexpr size_t vec_width = vector_width<Tin>;
#endif

    constexpr size_t w = internal::select_process_width(width, vec_width, last_dim_size);

    constexpr index_t out_axis = internal::select_axis(outdims, Axis);
    constexpr index_t in_axis  = out_axis + indims - outdims;

    const shape<outdims> outshape = Trout::get_shape(out);
    const shape<indims> inshape   = Trin::get_shape(in);
    if (KFR_UNLIKELY(!internal_generic::can_assign_from(outshape, inshape)))
        return shape<outdims>{ 0 };
    shape<outdims> stop = min(min(add_shape(start, size), outshape), inshape.template extend<outdims>());

    index_t in_size = 0;
    if constexpr (indims > 0)
        in_size = inshape[in_axis];

    begin_pass(out, start, stop);
    begin_pass(in, inshape.adapt(start), inshape.adapt(stop, ctrue));

    shape<outdims> outidx;
    if constexpr (outdims == 1)
    {
        outidx = shape<outdims>{ 0 };
        tprocess_body<Out, In, out_axis, w, gw, Tin, outdims, indims>(
            std::forward<Out>(out), std::forward<In>(in), start[out_axis], stop[out_axis], in_size, outidx,
            inshape.adapt(outidx));
    }
    else if constexpr (outdims == 2)
    {
        for (index_t i0 = axis_start<out_axis, 0>(start); i0 < axis_stop<out_axis, 0>(stop); ++i0)
        {
            for (index_t i1 = axis_start<out_axis, 1>(start); i1 < axis_stop<out_axis, 1>(stop); ++i1)
            {
                outidx = shape<outdims>{ i0, i1 };
                tprocess_body<Out, In, out_axis, w, gw, Tin, outdims, indims>(
                    std::forward<Out>(out), std::forward<In>(in), start[out_axis], stop[out_axis], in_size,
                    outidx, inshape.adapt(outidx));
            }
        }
    }
    else if constexpr (outdims == 3)
    {
        for (index_t i0 = axis_start<out_axis, 0>(start); i0 < axis_stop<out_axis, 0>(stop); ++i0)
        {
            for (index_t i1 = axis_start<out_axis, 1>(start); i1 < axis_stop<out_axis, 1>(stop); ++i1)
            {
                for (index_t i2 = axis_start<out_axis, 2>(start); i2 < axis_stop<out_axis, 2>(stop); ++i2)
                {
                    outidx = shape<outdims>{ i0, i1, i2 };
                    tprocess_body<Out, In, out_axis, w, gw, Tin, outdims, indims>(
                        std::forward<Out>(out), std::forward<In>(in), start[out_axis], stop[out_axis],
                        in_size, outidx, inshape.adapt(outidx));
                }
            }
        }
    }
    else if constexpr (outdims == 4)
    {
        for (index_t i0 = axis_start<out_axis, 0>(start); i0 < axis_stop<out_axis, 0>(stop); ++i0)
        {
            for (index_t i1 = axis_start<out_axis, 1>(start); i1 < axis_stop<out_axis, 1>(stop); ++i1)
            {
                for (index_t i2 = axis_start<out_axis, 2>(start); i2 < axis_stop<out_axis, 2>(stop); ++i2)
                {
                    for (index_t i3 = axis_start<out_axis, 3>(start); i3 < axis_stop<out_axis, 3>(stop); ++i3)
                    {
                        outidx = shape<outdims>{ i0, i1, i2, i3 };
                        tprocess_body<Out, In, out_axis, w, gw, Tin, outdims, indims>(
                            std::forward<Out>(out), std::forward<In>(in), start[out_axis], stop[out_axis],
                            in_size, outidx, inshape.adapt(outidx));
                    }
                }
            }
        }
    }
    else
    {
        shape<outdims> outidx = start;
        if (KFR_UNLIKELY(!internal_generic::compare_indices(outidx, stop)))
            return stop;
        do
        {
            tprocess_body<Out, In, out_axis, w, gw, Tin, outdims, indims>(
                std::forward<Out>(out), std::forward<In>(in), start[out_axis], stop[out_axis], in_size,
                outidx, inshape.adapt(outidx));
            outidx[out_axis] = stop[out_axis] - 1;
        } while (internal_generic::increment_indices(outidx, start, stop));
    }
    end_pass(in, inshape.adapt(start), inshape.adapt(stop, ctrue));
    end_pass(out, start, stop);
    return stop;
}

/**
 * @brief Output expression that discards all written values.
 *
 * Used as a sink when an input expression needs to be evaluated but its
 * results are not needed. Reports an infinite shape and accepts writes of any
 * width without storing them.
 * @tparam Tin Value type accepted by the sink.
 * @tparam Dims Number of dimensions reported by the sink.
 */
template <typename Tin, index_t Dims>
struct expression_discard : public expression_traits_defaults
{
    using value_type             = Tin;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_discard&) { return shape<dims>(infinite_size); }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }

    /**
     * @brief Discards a vector write.
     *
     * Friend function found via ADL that accepts and ignores any vector value
     * written to the sink.
     * @param self The sink instance.
     * @param x Vector value being written (ignored).
     * @tparam N Vector width.
     * @tparam VecAxis Axis parameter.
     */
    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC void set_elements(const expression_discard& self, shape<Dims>,
                                           axis_params<VecAxis, N>,
                                           const std::type_identity_t<vec<Tin, N>>& x)
    {
    }
};

/**
 * @brief Reads the expression @c expr through the whole range.
 * @param expr the input expression
 * @return the input expression is returned
 */
template <size_t width = 0, index_t Axis = infinite_size, typename E, typename Traits = expression_traits<E>>
KFR_FUNCTION const E& sink(E&& expr)
{
    static_assert(!Traits::get_shape().has_infinity());
    process<width, Axis>(expression_discard<expression_value_type<E>, expression_dims<E>>{}, expr);
    return expr;
}

/**
 * @brief Constructs an `expression_function` from a callable and arguments.
 * @param fn Callable to apply.
 * @param args Arguments to bind to the expression.
 * @tparam Fn Callable type.
 * @tparam Args Argument types.
 * @return A new `expression_function` holding @p fn and @p args.
 */
template <typename Fn, typename... Args>
KFR_FUNCTION expression_function<std::decay_t<Fn>, Args...> bind_expression(Fn&& fn, Args&&... args)
{
    return expression_function<std::decay_t<Fn>, Args...>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}
/**
 * @brief Construct a new expression using the same function as in @c e and new arguments
 * @param e an expression
 * @param args new arguments for the function
 */
template <typename Fn, typename... OldArgs, typename... NewArgs>
KFR_FUNCTION expression_function<Fn, NewArgs...> rebind(const expression_function<Fn, OldArgs...>& e,
                                                        NewArgs&&... args)
{
    return expression_function<Fn, NewArgs...>(Fn{ e.fn }, std::forward<NewArgs>(args)...);
}
/**
 * @brief Construct a new expression using the same function as in @c e and new arguments.
 *
 * This overload moves the callable out of @p e.
 * @param e an expression (rvalue).
 * @param args new arguments for the function.
 */
template <typename Fn, typename... OldArgs, typename... NewArgs>
KFR_FUNCTION expression_function<Fn, NewArgs...> rebind(expression_function<Fn, OldArgs...>&& e,
                                                        NewArgs&&... args)
{
    return expression_function<Fn, NewArgs...>(std::move(e.fn), std::forward<NewArgs>(args)...);
}

#ifdef KFR_TESTING
namespace internal
{
/**
 * @brief Evaluates a callable at @p N consecutive indices starting at @p index.
 *
 * Helper used by the `CHECK_EXPRESSION` macro to compute reference values for
 * testing expressions.
 * @param index Starting index.
 * @param fn Callable invoked as `fn(index + 0, index + 1, ...)`.
 * @tparam T Value type of the result vector.
 * @tparam N Vector width.
 * @tparam Fn Callable type.
 * @return Vector of @p N values produced by @p fn.
 */
template <typename T, size_t N, typename Fn>
inline vec<T, N> get_fn_value(size_t index, Fn&& fn)
{
    return apply(fn, enumerate<size_t, N>() + index);
}
} // namespace internal

/**
 * @brief Macro that checks a 1-dimensional expression against a reference
 *        callable.
 *
 * Verifies that reading the expression at various vector widths produces the
 * same values as the reference callable @p fn. The expression is sampled at
 * multiple offsets and widths up to the native vector width.
 * @param expr Expression to check.
 * @param size Expected size of the expression.
 * @param fn Reference callable returning the expected value at a given index.
 */
#define CHECK_EXPRESSION(...)                                                                                \
    []<typename E, typename Fn>(const E& expr, size_t size, Fn&& fn)                                         \
    {                                                                                                        \
        static_assert(expression_dims<E> == 1, "CHECK_EXPRESSION supports only 1-dim expressions");          \
        using T          = expression_value_type<E>;                                                         \
        size_t expr_size = get_shape(expr).front();                                                          \
        CHECK(expr_size == size);                                                                            \
        if (expr_size != size)                                                                               \
            return;                                                                                          \
        size                     = min(shape<1>(size), shape<1>(200)).front();                               \
        constexpr size_t maxsize = 2 + ilog2(vector_width<T> * 2);                                           \
        size_t g                 = 1;                                                                        \
        for (size_t i = 0; i < size;)                                                                        \
        {                                                                                                    \
            const size_t next_size = std::min(prev_poweroftwo(size - i), g);                                 \
            g *= 2;                                                                                          \
            if (g > (1 << (maxsize - 1)))                                                                    \
                g = 1;                                                                                       \
                                                                                                             \
            cswitch(csize<1> << csizeseq<maxsize>, next_size,                                                \
                    [&](auto x)                                                                              \
                    {                                                                                        \
                        constexpr size_t nsize = val_of(decltype(x)());                                      \
                        INFO(as_string("i = ", i, " width = ", nsize));                                      \
                        CHECK_THAT(get_elements(expr, shape<1>(i), axis_params_v<0, nsize>),                 \
                                   DeepMatcher(internal::get_fn_value<T, nsize>(i, fn)));                    \
                    });                                                                                      \
            i += next_size;                                                                                  \
        }                                                                                                    \
    }(__VA_ARGS__)

/**
 * @brief Macro that checks a 1-dimensional expression against a literal list
 *        of expected values.
 * @param expr Expression to check.
 * @param list Initializer list of expected values.
 */
#define CHECK_EXPRESSION_LIST(...)                                                                           \
    []<typename E_, typename T_ = expression_value_type<E_>>(                                                \
        const E_& expr, std::initializer_list<std::type_identity_t<T_>> list)                                \
    { CHECK_EXPRESSION(expr, list.size(), [&](size_t i) { return list.begin()[i]; }); }(__VA_ARGS__)

#endif

} // namespace KFR_ARCH_NAME

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)
