/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
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
#include "../simd/types.hpp"
#include "../simd/vec.hpp"
#include "shape.hpp"

#include <tuple>
#include <complex>

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wparentheses")

namespace kfr
{

#ifndef KFR_CUSTOM_COMPLEX
template <typename T>
using complex = std::complex<T>;
#endif

template <typename T, typename V = void>
struct expression_traits;

template <typename T>
using expression_value_type = typename expression_traits<T>::value_type;

template <typename T>
constexpr inline size_t expression_dims = expression_traits<T>::dims;

template <typename T>
constexpr inline shape<expression_dims<T>> get_shape(T&& expr)
{
    return expression_traits<T>::get_shape(expr);
}
template <typename T>
constexpr inline shape<expression_dims<T>> get_shape()
{
    return expression_traits<T>::get_shape();
}

template <typename T>
struct expression_traits<const T, std::void_t<expression_value_type<T>>> : expression_traits<T>
{
};
template <typename T>
struct expression_traits<T&, std::void_t<expression_value_type<T>>> : expression_traits<T>
{
};
template <typename T>
struct expression_traits<T&&, std::void_t<expression_value_type<T>>> : expression_traits<T>
{
};
template <typename T>
struct expression_traits<const T&, std::void_t<expression_value_type<T>>> : expression_traits<T>
{
};
template <typename T>
struct expression_traits<const T&&, std::void_t<typename expression_traits<T>::value_type>>
    : expression_traits<T>
{
};

// This allows old style expressions+traits
template <typename T>
struct expression_traits<T, std::void_t<decltype(T::random_access), decltype(T::get_shape())>>
{
    using value_type             = typename T::value_type;
    constexpr static size_t dims = T::dims;
    constexpr static shape<dims> get_shape(const T& self) { return T::get_shape(self); }
    constexpr static shape<dims> get_shape() { return T::get_shape(); }

    constexpr static inline bool explicit_operand = T::explicit_operand;
    constexpr static inline bool random_access    = T::random_access;
};

struct expression_traits_defaults
{
    // using value_type = /* ... */;
    // constexpr static size_t dims = 0;
    // constexpr static shape<dims> get_shape(const T&);
    // constexpr static shape<dims> get_shape();

    constexpr static inline bool explicit_operand = true;
    constexpr static inline bool random_access    = true;
};

template <typename T, typename = void>
constexpr inline bool has_expression_traits = false;

template <typename T>
constexpr inline bool has_expression_traits<T, std::void_t<typename expression_traits<T>::value_type>> = true;

namespace internal_generic
{
template <typename... Xs>
using expressions_condition = std::void_t<expression_traits<Xs>...>;
template <typename... Xs>
using expressions_check = std::enable_if_t<(expression_traits<Xs>::explicit_operand || ...)>;
} // namespace internal_generic

template <typename T>
using enable_if_input_expression =
    std::void_t<expression_traits<T>,
                decltype(get_elements(std::declval<T>(), shape<expression_traits<T>::dims>(),
                                      axis_params<0, 1>{}))>;

template <typename T>
using enable_if_output_expression =
    std::void_t<expression_traits<T>,
                decltype(set_elements(std::declval<T&>(), shape<expression_traits<T>::dims>(),
                                      axis_params<0, 1>{},
                                      vec<typename expression_traits<T>::value_type, 1>{}))>;

template <typename T>
using enable_if_input_output_expression =
    std::void_t<enable_if_input_expression<T>, enable_if_output_expression<T>>;

template <typename... T>
using enable_if_input_expressions = std::void_t<enable_if_input_expression<T>...>;

template <typename... T>
using enable_if_output_expressions = std::void_t<enable_if_output_expression<T>...>;

template <typename... T>
using enable_if_input_output_expressions = std::void_t<enable_if_input_output_expression<T>...>;

template <typename E, typename = void>
constexpr inline bool is_input_expression = false;

template <typename E>
constexpr inline bool is_input_expression<E, enable_if_input_expression<E>> = true;

template <typename E, typename = void>
constexpr inline bool is_output_expression = false;

template <typename E>
constexpr inline bool is_output_expression<E, enable_if_output_expression<E>> = true;

template <typename E, typename = void>
constexpr inline bool is_input_output_expression = false;

template <typename E>
constexpr inline bool is_input_output_expression<E, enable_if_input_output_expression<E>> = true;

#define KFR_ACCEPT_EXPRESSIONS(...) internal_generic::expressions_check<__VA_ARGS__>* = nullptr

#define KFR_ACCEPT_ASGN_EXPRESSIONS(E1, E2)                                                                  \
    KFR_ENABLE_IF(is_input_output_expression<E1>&& is_input_expression<E2>)

template <typename T>
constexpr inline bool is_expr_element = std::is_same_v<std::remove_cv_t<T>, T> && is_vec_element<T>;

template <typename E>
constexpr inline bool is_infinite = expression_traits<E>::get_shape().has_infinity();

template <typename T>
struct expression_traits<T, std::enable_if_t<is_expr_element<T>>> : expression_traits_defaults
{
    using value_type                              = T;
    constexpr static size_t dims                  = 0;
    constexpr static inline bool explicit_operand = false;

    KFR_MEM_INTRINSIC constexpr static shape<0> get_shape(const T& self) { return {}; }
    KFR_MEM_INTRINSIC constexpr static shape<0> get_shape() { return {}; }
};

template <typename E, enable_if_input_expression<E>* = nullptr, index_t Dims = expression_dims<E>>
inline expression_value_type<E> get_element(E&& expr, shape<Dims> index)
{
    return get_elements(expr, index, axis_params_v<0, 1>).front();
}

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

inline namespace CMT_ARCH_NAME
{

template <index_t Dims, typename U = unsigned_type<sizeof(index_t) * 8>>
KFR_INTRINSIC vec<U, Dims> to_vec(const shape<Dims>& sh)
{
    return read<Dims>(reinterpret_cast<const U*>(sh.data()));
}

namespace internal
{
template <size_t width, typename Fn>
KFR_INTRINSIC void block_process_impl(size_t& i, size_t size, Fn&& fn)
{
    CMT_LOOP_NOUNROLL
    for (; i < size / width * width; i += width)
        fn(i, csize_t<width>());
}
} // namespace internal

template <size_t... widths, typename Fn>
KFR_INTRINSIC void block_process(size_t size, csizes_t<widths...>, Fn&& fn)
{
    size_t i = 0;
    swallow{ (internal::block_process_impl<widths>(i, size, std::forward<Fn>(fn)), 0)... };
}

template <index_t Dims>
KFR_INTRINSIC void begin_pass(const internal_generic::anything&, shape<Dims> start, shape<Dims> stop)
{
}
template <index_t Dims>
KFR_INTRINSIC void end_pass(const internal_generic::anything&, shape<Dims> start, shape<Dims> stop)
{
}

template <typename T, index_t Axis, size_t N, KFR_ENABLE_IF(is_expr_element<std::decay_t<T>>)>
KFR_INTRINSIC vec<std::decay_t<T>, N> get_elements(T&& self, const shape<0>& index,
                                                   const axis_params<Axis, N>&)
{
    return self;
}
template <typename T, index_t Axis, size_t N, KFR_ENABLE_IF(is_expr_element<std::decay_t<T>>)>
KFR_INTRINSIC void set_elements(T& self, const shape<0>& index, const axis_params<Axis, N>&,
                                const identity<vec<T, N>>& val)
{
    static_assert(N == 1);
    static_assert(!std::is_const_v<T>);
    self = val.front();
}

template <typename T>
constexpr inline bool is_arg = is_numeric_or_bool<std::decay_t<T>>;

template <typename T>
using arg = std::conditional_t<is_arg<T>, std::decay_t<T>, T>;

template <typename... Args>
struct expression_with_arguments
{
    constexpr static size_t count = sizeof...(Args);

    using type_list = ctypes_t<Args...>;

    template <size_t idx>
    using nth = typename type_list::template nth<idx>;

    using first_arg = typename type_list::template nth<0>;

    template <size_t idx>
    using nth_trait = expression_traits<typename type_list::template nth<idx>>;

    using first_arg_traits = expression_traits<first_arg>;

    std::tuple<Args...> args;
    std::array<dimset, count> masks;

    KFR_INTRINSIC auto& first() { return std::get<0>(args); }
    KFR_INTRINSIC const auto& first() const { return std::get<0>(args); }

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

    template <typename Fn>
    KFR_INTRINSIC constexpr auto fold(Fn&& fn) const
    {
        return fold_impl(std::forward<Fn>(fn), csizeseq<count>);
    }
    template <typename Fn>
    KFR_INTRINSIC constexpr static auto fold_idx(Fn&& fn)
    {
        return fold_idx_impl(std::forward<Fn>(fn), csizeseq<count>);
    }

    KFR_INTRINSIC expression_with_arguments(arg<Args&&>... args) : args{ std::forward<Args>(args)... }
    {
        cforeach(csizeseq<count>,
                 [&](auto idx_) CMT_INLINE_LAMBDA
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

template <typename Arg>
struct expression_with_arguments<Arg>
{
    constexpr static size_t count = 1;

    using type_list = ctypes_t<Arg>;

    template <size_t idx>
    using nth = Arg;

    using first_arg = Arg;

    template <size_t idx>
    using nth_trait = expression_traits<Arg>;

    using first_arg_traits = expression_traits<first_arg>;

    std::tuple<Arg> args;

    KFR_MEM_INTRINSIC auto& first() { return std::get<0>(args); }
    KFR_MEM_INTRINSIC const auto& first() const { return std::get<0>(args); }

    template <size_t idx>
    KFR_MEM_INTRINSIC dimset getmask(csize_t<idx> = {}) const
    {
        return dimset(-1);
    }

    template <typename Fn>
    KFR_MEM_INTRINSIC constexpr auto fold(Fn&& fn) const
    {
        return fold_impl(std::forward<Fn>(fn), csizeseq<count>);
    }
    template <typename Fn>
    KFR_INTRINSIC constexpr static auto fold_idx(Fn&& fn)
    {
        return fold_idx_impl(std::forward<Fn>(fn), csizeseq<count>);
    }

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

template <typename... Args>
expression_with_arguments(Args&&... args) -> expression_with_arguments<Args...>;

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

template <typename Fn, typename... Args>
struct expression_function : expression_with_arguments<Args...>, expression_traits_defaults
{
    using value_type =
        typename std::invoke_result_t<Fn,
                                      vec<typename expression_traits<Args>::value_type, 1>...>::value_type;
    constexpr static size_t dims = const_max(expression_traits<Args>::dims...);

#if defined CMT_COMPILER_IS_MSVC || defined CMT_COMPILER_GCC
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
    constexpr static shape<dims> get_shape(const expression_function& self)
    {
        return self.fold(lambda_get_shape_self{ self });
    }
    constexpr static shape<dims> get_shape() { return expression_function::fold_idx(lambda_get_shape{}); }
#else
    constexpr static shape<dims> get_shape(const expression_function& self)
    {
        return self.fold(
            [&](auto&&... args) CMT_INLINE_LAMBDA constexpr -> auto {
                return internal_generic::common_shape<true>(
                    expression_traits<decltype(args)>::get_shape(args)...);
            });
    }
    constexpr static shape<dims> get_shape()
    {
        return expression_function::fold_idx(
            [&](auto... args) CMT_INLINE_LAMBDA constexpr -> auto
            {
                return internal_generic::common_shape(
                    expression_traits<typename expression_function::template nth<val_of(decltype(args)())>>::
                        get_shape()...);
            });
    }
#endif

    constexpr static inline bool random_access = (expression_traits<Args>::random_access && ...);

    Fn fn;

    KFR_MEM_INTRINSIC expression_function(expression_with_arguments<Args...> args, Fn&& fn)
        : expression_with_arguments<Args...>{ std::move(args) }, fn(std::forward<Fn>(fn))
    {
    }
    KFR_MEM_INTRINSIC expression_function(Fn&& fn, arg<Args&&>... args)
        : expression_with_arguments<Args...>{ std::forward<Args>(args)... }, fn(std::forward<Fn>(fn))
    {
    }
    KFR_MEM_INTRINSIC expression_function(arg<Args&&>... args)
        : expression_with_arguments<Args...>{ std::forward<Args>(args)... }, fn{}
    {
    }

    template <typename In, enable_if_input_expression<In>* = nullptr>
    expression_function& operator=(In&& in)
    {
        static_assert(is_output_expression<expression_function>);
        process(*this, std::forward<In>(in));
        return *this;
    }
};

template <typename... Args, typename Fn>
expression_function(const expression_with_arguments<Args...>& args, Fn&& fn)
    -> expression_function<Fn, Args...>;
template <typename... Args, typename Fn>
expression_function(expression_with_arguments<Args...>&& args, Fn&& fn) -> expression_function<Fn, Args...>;
template <typename... Args, typename Fn>
expression_function(expression_with_arguments<Args...>& args, Fn&& fn) -> expression_function<Fn, Args...>;

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
                if (CMT_UNLIKELY(self.masks[idx].back() == 0))
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

template <typename... Args, index_t Dims>
KFR_INTRINSIC void begin_pass(const expression_with_arguments<Args...>& self, shape<Dims> start,
                              shape<Dims> stop)
{
    internal::begin_pass_args(self, start, stop, indicesfor<Args...>);
}

template <typename... Args, index_t Dims>
KFR_INTRINSIC void end_pass(const expression_with_arguments<Args...>& self, shape<Dims> start,
                            shape<Dims> stop)
{
    internal::end_pass_args(self, start, stop, indicesfor<Args...>);
}

template <typename Fn, typename... Args, index_t Axis, size_t N, index_t Dims,
          typename Tr = expression_traits<expression_function<Fn, Args...>>,
          typename T  = typename Tr::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_function<Fn, Args...>& self, const shape<Dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return self.fold_idx([&](auto... idx) CMT_INLINE_LAMBDA -> vec<T, N>
                         { return self.fn(internal::get_arg<Tr::dims>(self, index, sh, idx)...); });
}

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
            CMT_LOOP_NOUNROLL
            for (; x < stop / w * w; x += w)
            {
                outidx[OutAxis] = x;
                set_elements(out, outidx, axis_params_v<OutAxis, w>, repeat<w>(val));
            }
        }
        CMT_LOOP_NOUNROLL
        for (; x < stop / gw * gw; x += gw)
        {
            outidx[OutAxis] = x;
            set_elements(out, outidx, axis_params_v<OutAxis, gw>, repeat<gw>(val));
        }
    }
    else
    {
        constexpr index_t InAxis = OutAxis + indims - outdims;
        size_t x                 = start;
        if constexpr (w > gw)
        {
            CMT_LOOP_NOUNROLL
            for (; x < stop / w * w; x += w)
            {
                outidx[OutAxis] = x;
                inidx[InAxis]   = std::min(x, insize - 1);
                auto v          = get_elements(in, inidx, axis_params_v<InAxis, w>);
                // println("## i=", x, "\n", v);
                set_elements(out, outidx, axis_params_v<OutAxis, w>, v);
            }
        }
        CMT_LOOP_NOUNROLL
        for (; x < stop / gw * gw; x += gw)
        {
            outidx[OutAxis] = x;
            inidx[InAxis]   = std::min(x, insize - 1);
            set_elements(out, outidx, axis_params_v<OutAxis, gw>,
                         get_elements(in, inidx, axis_params_v<InAxis, gw>));
        }
    }
}

template <size_t width = 0, index_t Axis = 0, typename Out, typename In, size_t gw = 1,
          CMT_ENABLE_IF(expression_traits<Out>::dims == 0)>
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

template <size_t width = 0, index_t Axis = infinite_size, typename Out, typename In, size_t gw = 1,
          index_t outdims = expression_dims<Out>, CMT_ENABLE_IF(expression_dims<Out> > 0)>
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
    if (CMT_UNLIKELY(!internal_generic::can_assign_from(outshape, inshape)))
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
        if (CMT_UNLIKELY(!internal_generic::compare_indices(outidx, stop)))
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

template <typename Tin, index_t Dims>
struct expression_discard : public expression_traits_defaults
{
    using value_type             = Tin;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_discard&) { return shape<dims>(infinite_size); }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }

    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC void set_elements(const expression_discard& self, shape<Dims>,
                                           axis_params<VecAxis, N>, const identity<vec<Tin, N>>& x)
    {
    }
};

/// @brief Read the expression @c expr through the whole range.
/// @param expr the input expression
/// @return the input expression is returned
template <size_t width = 0, index_t Axis = infinite_size, typename E, typename Traits = expression_traits<E>>
KFR_FUNCTION const E& sink(E&& expr)
{
    static_assert(!Traits::get_shape().has_infinity());
    process<width, Axis>(expression_discard<expression_value_type<E>, expression_dims<E>>{}, expr);
    return expr;
}

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
template <typename Fn, typename... OldArgs, typename... NewArgs>
KFR_FUNCTION expression_function<Fn, NewArgs...> rebind(expression_function<Fn, OldArgs...>&& e,
                                                        NewArgs&&... args)
{
    return expression_function<Fn, NewArgs...>(std::move(e.fn), std::forward<NewArgs>(args)...);
}

#ifdef KFR_TESTING
namespace internal
{
template <typename T, size_t N, typename Fn>
inline vec<T, N> get_fn_value(size_t index, Fn&& fn)
{
    return apply(fn, enumerate<size_t, N>() + index);
}
} // namespace internal

template <typename E, typename Fn, KFR_ENABLE_IF(std::is_invocable_v<Fn, size_t>)>
void test_expression(const E& expr, size_t size, Fn&& fn, const char* expression = nullptr,
                     const char* file = nullptr, int line = 0)
{
    static_assert(expression_dims<E> == 1, "CHECK_EXPRESSION supports only 1-dim expressions");
    using T                  = expression_value_type<E>;
    size_t expr_size         = get_shape(expr).front();
    ::testo::test_case* test = ::testo::active_test();
    auto&& c                 = ::testo::make_comparison();
    test->check(c <= expr_size == size, expression, file, line);
    if (expr_size != size)
        return;
    size                     = min(shape<1>(size), shape<1>(200)).front();
    constexpr size_t maxsize = 2 + ilog2(vector_width<T> * 2);
    size_t g                 = 1;
    for (size_t i = 0; i < size;)
    {
        const size_t next_size = std::min(prev_poweroftwo(size - i), g);
        g *= 2;
        if (g > (1 << (maxsize - 1)))
            g = 1;

        cswitch(csize<1> << csizeseq<maxsize>, next_size,
                [&](auto x)
                {
                    constexpr size_t nsize = val_of(decltype(x)());
                    ::testo::scope s(as_string("i = ", i, " width = ", nsize));
                    test->check(c <= get_elements(expr, shape<1>(i), axis_params_v<0, nsize>) ==
                                    internal::get_fn_value<T, nsize>(i, fn),
                                expression, file, line);
                });
        i += next_size;
    }
}

template <typename E, typename T = expression_value_type<E>>
void test_expression(const E& expr, std::initializer_list<cometa::identity<T>> list,
                     const char* expression = nullptr, const char* file = nullptr, int line = 0)
{
    test_expression(
        expr, list.size(), [&](size_t i) { return list.begin()[i]; }, expression, file, line);
}
#define TESTO_CHECK_EXPRESSION(expr, ...) ::kfr::test_expression(expr, __VA_ARGS__, #expr, __FILE__, __LINE__)

#ifndef TESTO_NO_SHORT_MACROS
#define CHECK_EXPRESSION TESTO_CHECK_EXPRESSION
#endif
#endif

} // namespace CMT_ARCH_NAME

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
