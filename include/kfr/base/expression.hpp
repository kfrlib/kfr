/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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
#include "../simd/shuffle.hpp"
#include "../simd/types.hpp"
#include "../simd/vec.hpp"
#include "shape.hpp"

#include <tuple>
#ifdef KFR_STD_COMPLEX
#include <complex>
#endif

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wparentheses")

namespace kfr
{

#ifdef KFR_STD_COMPLEX

template <typename T>
using complex = std::complex<T>;

#else
#ifndef KFR_CUSTOM_COMPLEX

template <typename>
struct complex;
#endif
#endif

struct accepts_any
{
};

template <typename T, typename V = void>
struct expression_traits;

template <typename T>
using expression_value_type = typename expression_traits<T>::value_type;

template <typename T>
constexpr inline size_t expression_dims = expression_traits<T>::dims;

template <typename T>
constexpr inline shape<expression_dims<T>> shapeof(T&& expr)
{
    return expression_traits<T>::shapeof(expr);
}
template <typename T>
constexpr inline shape<expression_dims<T>> shapeof()
{
    return expression_traits<T>::shapeof();
}

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

struct expression_traits_defaults
{
    // using value_type = accepts_any;
    // constexpr static size_t dims = 0;
    // constexpr static shape<dims> shapeof(const T&);
    // constexpr static shape<dims> shapeof();

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
using enable_if_inout_output_expression =
    std::void_t<enable_if_input_expression<T>, enable_if_output_expression<T>>;

template <typename... T>
using enable_if_input_expressions = std::void_t<enable_if_input_expression<T>...>;

template <typename... T>
using enable_if_output_expressions = std::void_t<enable_if_output_expression<T>...>;

#define KFR_ACCEPT_EXPRESSIONS(...) internal_generic::expressions_check<__VA_ARGS__>* = nullptr

template <typename T>
constexpr inline bool is_expr_element = std::is_same_v<std::remove_cv_t<T>, T>&& is_vec_element<T>;

template <typename T>
struct expression_traits<T, std::enable_if_t<is_expr_element<T>>> : expression_traits_defaults
{
    using value_type                              = T;
    constexpr static size_t dims                  = 0;
    constexpr static inline bool explicit_operand = false;

    KFR_MEM_INTRINSIC constexpr static shape<0> shapeof(const T& self) { return {}; }
    KFR_MEM_INTRINSIC constexpr static shape<0> shapeof() { return {}; }
};

inline namespace CMT_ARCH_NAME
{
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
} // namespace CMT_ARCH_NAME

inline namespace CMT_ARCH_NAME
{

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
                set_elements(out, outidx, axis_params_v<OutAxis, w>,
                             get_elements(in, inidx, axis_params_v<InAxis, w>));
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
    set_elements(out, shape<0>{}, axis_params_v<0, 1>, get_elements(in, shape<0>{}, axis_params_v<0, 1>));
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

    return std::min(vec_width, last_dim_size);
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
    using Trin  = expression_traits<In>;
    using Trout = expression_traits<Out>;
    using Tin   = typename Trin::value_type;

    using internal::axis_start;
    using internal::axis_stop;

    constexpr index_t indims = expression_dims<In>;
    static_assert(outdims >= indims);

    constexpr index_t last_dim_size = prev_poweroftwo(Trout::shapeof().back());

#ifdef NDEBUG
    constexpr size_t vec_width = maximum_vector_size<Tin>;
#else
    constexpr size_t vec_width = vector_width<Tin>;
#endif

    constexpr size_t w = internal::select_process_width(width, vec_width, last_dim_size);

    constexpr index_t out_axis = internal::select_axis(outdims, Axis);
    constexpr index_t in_axis  = out_axis + indims - outdims;

    const shape<outdims> outshape = Trout::shapeof(out);
    const shape<indims> inshape   = Trin::shapeof(in);
    if (CMT_UNLIKELY(!internal_generic::can_assign_from(outshape, inshape)))
        return shape<outdims>{ 0 };
    shape<outdims> stop = min(start.add_inf(size), outshape);

    index_t in_size = 0;
    if constexpr (indims > 0)
        in_size = inshape[in_axis];

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
    return stop;
}
} // namespace CMT_ARCH_NAME

template <typename... Args>
struct xwitharguments
{
    constexpr static size_t count = sizeof...(Args);

    using type_list = ctypes_t<Args...>;

    template <size_t idx>
    using nth = typename type_list::template nth<idx>;

    using first_arg = typename type_list::template nth<0>;

    template <size_t idx>
    using nth_trait = expression_traits<typename type_list::template nth<idx>>;

    using first_arg_trait = expression_traits<first_arg>;

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
            return -1;
        }
        else
        {
            constexpr shape<Traits::dims> sh = Traits::shapeof();
            if constexpr (sh.cproduct() > 0)
            {
                return sh.tomask();
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

    KFR_INTRINSIC xwitharguments(Args&&... args) : args{ std::forward<Args>(args)... }
    {
        cforeach(csizeseq<count>,
                 [&](auto idx_) CMT_INLINE_LAMBDA
                 {
                     constexpr size_t idx = val_of(decltype(idx_)());
                     shape sh             = expression_traits<nth<idx>>::shapeof(std::get<idx>(this->args));
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
struct xwitharguments<Arg>
{
    constexpr static size_t count = 1;

    using type_list = ctypes_t<Arg>;

    template <size_t idx>
    using nth = Arg;

    using first_arg = Arg;

    template <size_t idx>
    using nth_trait = expression_traits<Arg>;

    using first_arg_trait = expression_traits<first_arg>;

    std::tuple<Arg> args;

    KFR_MEM_INTRINSIC auto& first() { return std::get<0>(args); }
    KFR_MEM_INTRINSIC const auto& first() const { return std::get<0>(args); }

    template <size_t idx>
    KFR_MEM_INTRINSIC dimset getmask(csize_t<idx> = {}) const
    {
        return -1;
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

    KFR_MEM_INTRINSIC xwitharguments(Arg&& arg) : args{ std::forward<Arg>(arg) } {}

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
xwitharguments(Args&&... args) -> xwitharguments<Args...>;

template <typename Fn, typename... Args>
struct xfunction : public xwitharguments<Args...>
{
    Fn fn;

    KFR_MEM_INTRINSIC xfunction(xwitharguments<Args...> args, Fn&& fn)
        : xwitharguments<Args...>{ std::move(args) }, fn(std::move(fn))
    {
    }
    KFR_MEM_INTRINSIC xfunction(Fn&& fn, Args&&... args)
        : xwitharguments<Args...>{ std::forward<Args>(args)... }, fn(std::move(fn))
    {
    }
};

template <typename... Args, typename Fn>
xfunction(const xwitharguments<Args...>& args, Fn&& fn) -> xfunction<Fn, Args...>;
template <typename... Args, typename Fn>
xfunction(xwitharguments<Args...>&& args, Fn&& fn) -> xfunction<Fn, Args...>;
template <typename... Args, typename Fn>
xfunction(xwitharguments<Args...>& args, Fn&& fn) -> xfunction<Fn, Args...>;

template <typename Fn, typename... Args>
struct expression_traits<xfunction<Fn, Args...>> : expression_traits_defaults
{
    using E = xfunction<Fn, Args...>;

    using value_type =
        typename std::invoke_result_t<Fn,
                                      vec<typename expression_traits<Args>::value_type, 1>...>::value_type;
    constexpr static size_t dims = const_max(expression_traits<Args>::dims...);

    constexpr static shape<dims> shapeof(const E& self)
    {
        return self.fold([&](auto&&... args) CMT_INLINE_LAMBDA -> auto {
            return internal_generic::common_shape(expression_traits<decltype(args)>::shapeof(args)...);
        });
    }
    constexpr static shape<dims> shapeof()
    {
        return xfunction<Fn, Args...>::fold_idx([&](auto... args) CMT_INLINE_LAMBDA -> auto {
            return internal_generic::common_shape(
                expression_traits<typename E::template nth<val_of(decltype(args)())>>::shapeof()...);
        });
    }

    constexpr static inline bool random_access = (expression_traits<Args>::random_access && ...);
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{
template <index_t outdims, typename Fn, typename... Args, index_t Axis, size_t N, index_t Dims, size_t idx,
          typename Traits = expression_traits<typename xfunction<Fn, Args...>::template nth<idx>>>
KFR_MEM_INTRINSIC vec<typename Traits::value_type, N> get_arg(const xfunction<Fn, Args...>& self,
                                                              const shape<Dims>& index,
                                                              const axis_params<Axis, N>& sh, csize_t<idx>)
{
    if constexpr (Traits::dims == 0)
    {
        return repeat<N>(get_elements(std::get<idx>(self.args), {}, axis_params<Axis, 1>{}));
    }
    else
    {
        auto indices               = internal_generic::adapt<Traits::dims>(index, self.getmask(csize<idx>));
        constexpr index_t last_dim = Traits::shapeof().back();
        if constexpr (last_dim > 0)
        {
            return repeat<N / std::min(last_dim, N)>(
                get_elements(std::get<idx>(self.args), indices, axis_params<Axis, std::min(last_dim, N)>{}));
        }
        else
        {
            if constexpr (sizeof...(Args) > 1 && N > 1)
            {
                if (CMT_UNLIKELY(self.masks[idx].back() == 0))
                    return get_elements(std::get<idx>(self.args), indices, axis_params<Axis, 1>{}).front();
                else
                    return get_elements(std::get<idx>(self.args), indices, sh);
            }
            else
            {
                return get_elements(std::get<idx>(self.args), indices, sh);
            }
        }
    }
}
} // namespace internal

template <typename Fn, typename... Args, index_t Axis, size_t N, index_t Dims,
          typename Tr = expression_traits<xfunction<Fn, Args...>>, typename T = typename Tr::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xfunction<Fn, Args...>& self, const shape<Dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    constexpr index_t outdims = Tr::dims;
    return self.fold_idx(
        [&](auto... idx) CMT_INLINE_LAMBDA -> vec<T, N> {
            return self.fn(internal::get_arg<outdims>(self, index, sh, idx)...);
        });
}

template <typename Fn, typename... Args>
KFR_FUNCTION xfunction<decay<Fn>, Args...> bind_expression(Fn&& fn, Args&&... args)
{
    return xfunction<decay<Fn>, Args...>(std::forward<Fn>(fn), std::forward<Args>(args)...);
}
/**
 * @brief Construct a new expression using the same function as in @c e and new arguments
 * @param e an expression
 * @param args new arguments for the function
 */
template <typename Fn, typename... OldArgs, typename... NewArgs>
KFR_FUNCTION xfunction<Fn, NewArgs...> rebind(const xfunction<Fn, OldArgs...>& e, NewArgs&&... args)
{
    return xfunction<Fn, NewArgs...>(e.fn, std::forward<NewArgs>(args)...);
}
template <typename Fn, typename... OldArgs, typename... NewArgs>
KFR_FUNCTION xfunction<Fn, NewArgs...> rebind(xfunction<Fn, OldArgs...>&& e, NewArgs&&... args)
{
    return xfunction<Fn, NewArgs...>(std::move(e.fn), std::forward<NewArgs>(args)...);
}

template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
KFR_FUNCTION xfunction<fn::interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return xfunction<Fn, E1, E2>{ fn::interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
