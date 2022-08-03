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
    // using value_type = void;
    // constexpr static size_t dims = 0;
    // constexpr static shape<dims> shapeof(const T&);
    // constexpr static shape<dims> shapeof();

    constexpr static inline bool explicit_operand = true;
};

namespace internal_generic
{
template <typename... Xs>
using expressions_check = std::enable_if_t<(expression_traits<Xs>::explicit_operand || ...)>;
}

#define KFR_ACCEPT_EXPRESSIONS(...) internal_generic::expressions_check<__VA_ARGS__>* = nullptr

template <typename T>
struct expression_traits<T, std::enable_if_t<is_simd_type<T>>> : expression_traits_defaults
{
    using value_type                              = T;
    constexpr static size_t dims                  = 0;
    constexpr static inline bool explicit_operand = false;

    KFR_MEM_INTRINSIC constexpr static shape<0> shapeof(const T& self) { return {}; }
    KFR_MEM_INTRINSIC constexpr static shape<0> shapeof() { return {}; }
};

inline namespace CMT_ARCH_NAME
{
template <typename T, typename U, size_t N, KFR_ENABLE_IF(is_simd_type<std::decay_t<T>>)>
KFR_MEM_INTRINSIC vec<U, N> get_elements(T&& self, const shape<0>& index, vec_shape<U, N> sh)
{
    return self;
}
template <typename T, typename U, size_t N, KFR_ENABLE_IF(is_simd_type<std::decay_t<T>>)>
KFR_MEM_INTRINSIC void set_elements(T& self, const shape<0>& index, const vec<U, N>& val)
{
    static_assert(N == 1);
    static_assert(!std::is_const_v<T>);
    self = val.front();
}
} // namespace CMT_ARCH_NAME

inline namespace CMT_ARCH_NAME
{

template <typename Out, typename In, size_t w, size_t gw, typename Tin, index_t outdims, index_t indims>
KFR_INTRINSIC static void tprocess_body(Out&& out, In&& in, size_t start, size_t stop, size_t insize,
                                        shape<outdims> outidx, shape<indims> inidx)
{
    size_t x = start;
    if constexpr (w > gw)
    {
        CMT_LOOP_NOUNROLL
        for (; x < stop / w * w; x += w)
        {
            outidx.set_revindex(0, x);
            inidx.set_revindex(0, std::min(x, insize - 1));
            set_elements(out, outidx, get_elements(in, inidx, vec_shape<Tin, w>()));
        }
    }
    CMT_LOOP_NOUNROLL
    for (; x < stop / gw * gw; x += gw)
    {
        outidx.set_revindex(0, x);
        inidx.set_revindex(0, std::min(x, insize - 1));
        set_elements(out, outidx, get_elements(in, inidx, vec_shape<Tin, gw>()));
    }
}

template <size_t width = 0, typename Out, typename In, size_t gw = 1,
          CMT_ENABLE_IF(expression_traits<Out>::dims == 0)>
static auto tprocess(Out&& out, In&& in, shape<0> = {}, shape<0> = {}, csize_t<gw> = {}) -> shape<0>
{
    set_elements(out, shape<0>{},
                 get_elements(in, shape<0>{}, vec_shape<typename expression_traits<In>::value_type, 1>()));
    return {};
}

namespace internal
{

constexpr size_t select_process_width(size_t width, size_t vec_width, index_t last_dim_size)
{
    if (width != 0)
        return width;
    if (last_dim_size == 0)
        return vec_width;

    return std::min(vec_width, last_dim_size);
}
} // namespace internal

template <size_t width = 0, typename Out, typename In, size_t gw = 1,
          typename Tin = expression_value_type<In>, typename Tout = expression_value_type<Out>,
          index_t outdims = expression_dims<Out>, CMT_ENABLE_IF(expression_dims<Out> > 0)>
static auto tprocess(Out&& out, In&& in, shape<outdims> start = 0, shape<outdims> size = infinite_size,
                     csize_t<gw> = {}) -> shape<outdims>
{
    constexpr index_t indims = expression_dims<In>;
    static_assert(outdims >= indims);

    constexpr index_t last_dim_size = prev_poweroftwo(expression_traits<Out>::shapeof().back());

#ifdef NDEBUG
    constexpr size_t vec_width = maximum_vector_size<Tin>;
#else
    constexpr size_t vec_width = vector_width<Tin>;
#endif

    constexpr size_t w = internal::select_process_width(width, vec_width, last_dim_size);

    const shape<outdims> outshape = shapeof(out);
    const shape<indims> inshape   = shapeof(in);
    if (CMT_UNLIKELY(!internal_generic::can_assign_from(outshape, inshape)))
        return { 0 };
    shape<outdims> stop = min(start + size, outshape);

    // min(out, in, size + start) - start

    shape<outdims> outidx;
    if constexpr (outdims == 1)
    {
        outidx = shape<outdims>{ 0 };
        tprocess_body<Out, In, w, gw, Tin, outdims, indims>(
            std::forward<Out>(out), std::forward<In>(in), start.revindex(0), stop.revindex(0),
            inshape.revindex(0), outidx, inshape.adapt(outidx));
    }
    else if constexpr (outdims == 2)
    {
        for (index_t x = start.revindex(1); x < stop.revindex(1); ++x)
        {
            outidx = shape<outdims>{ x, 0 };
            tprocess_body<Out, In, w, gw, Tin, outdims, indims>(
                std::forward<Out>(out), std::forward<In>(in), start.revindex(0), stop.revindex(0),
                inshape.revindex(0), outidx, inshape.adapt(outidx));
        }
    }
    else if constexpr (outdims == 3)
    {
        for (index_t x = start.revindex(2); x < stop.revindex(2); ++x)
        {
            for (index_t y = start.revindex(1); y < stop.revindex(1); ++y)
            {
                outidx = shape<outdims>{ x, y, 0 };
                tprocess_body<Out, In, w, gw, Tin, outdims, indims>(
                    std::forward<Out>(out), std::forward<In>(in), start.revindex(0), stop.revindex(0),
                    inshape.revindex(0), outidx, inshape.adapt(outidx));
            }
        }
    }
    else if constexpr (outdims == 4)
    {
        for (index_t x = start.revindex(3); x < stop.revindex(3); ++x)
        {
            for (index_t y = start.revindex(2); y < stop.revindex(2); ++y)
            {
                for (index_t z = start.revindex(1); z < stop.revindex(1); ++z)
                {
                    outidx = shape<outdims>{ x, y, z, 0 };
                    tprocess_body<Out, In, w, gw, Tin, outdims, indims>(
                        std::forward<Out>(out), std::forward<In>(in), start.revindex(0), stop.revindex(0),
                        inshape.revindex(0), outidx, inshape.adapt(outidx));
                }
            }
        }
    }
    else
    {
        shape<outdims> outidx = start;
        if (CMT_UNLIKELY(!internal_generic::compare_indices(outidx, stop, outdims - 2)))
            return stop;
        do
        {
            tprocess_body<Out, In, w, gw, Tin, outdims, indims>(
                std::forward<Out>(out), std::forward<In>(in), start.revindex(0), stop.revindex(0),
                inshape.revindex(0), outidx, inshape.adapt(outidx));
        } while (internal_generic::increment_indices(outidx, start, stop, outdims - 2));
    }
    return stop;
}
} // namespace CMT_ARCH_NAME

struct coutput_context
{
};

struct cinput_context
{
};

using coutput_t = const coutput_context*;
using cinput_t  = const cinput_context*;

constexpr cinput_t cinput   = nullptr;
constexpr coutput_t coutput = nullptr;

/// @brief Base class of all input expressoins
struct input_expression
{
    KFR_MEM_INTRINSIC constexpr static size_t size() CMT_NOEXCEPT { return infinite_size; }

    constexpr static bool is_incremental = false;

    KFR_MEM_INTRINSIC constexpr void begin_block(cinput_t, size_t) const {}
    KFR_MEM_INTRINSIC constexpr void end_block(cinput_t, size_t) const {}
};

/// @brief Base class of all output expressoins
struct output_expression
{
    KFR_MEM_INTRINSIC constexpr static size_t size() CMT_NOEXCEPT { return infinite_size; }

    constexpr static bool is_incremental = false;

    KFR_MEM_INTRINSIC constexpr void begin_block(coutput_t, size_t) const {}
    KFR_MEM_INTRINSIC constexpr void end_block(coutput_t, size_t) const {}
};

/// @brief Check if the type argument is an input expression
template <typename E>
constexpr inline bool is_input_expression = is_base_of<input_expression, decay<E>>;

/// @brief Check if the type arguments are an input expressions
template <typename... Es>
constexpr inline bool is_input_expressions = (is_base_of<input_expression, decay<Es>> || ...);

/// @brief Check if the type argument is an output expression
template <typename E>
constexpr inline bool is_output_expression = is_base_of<output_expression, decay<E>>;

/// @brief Check if the type arguments are an output expressions
template <typename... Es>
constexpr inline bool is_output_expressions = (is_base_of<output_expression, decay<Es>> || ...);

inline namespace CMT_ARCH_NAME
{

#ifdef KFR_TESTING
namespace internal
{
template <typename T, size_t N, typename Fn>
inline vec<T, N> get_fn_value(size_t index, Fn&& fn)
{
    return apply(fn, enumerate<size_t, N>() + index);
}
} // namespace internal

template <typename E, typename Fn>
void test_expression(const E& expr, size_t size, Fn&& fn, const char* expression = nullptr)
{
    using T                  = value_type_of<E>;
    ::testo::test_case* test = ::testo::active_test();
    auto&& c                 = ::testo::make_comparison();
    test->check(c <= expr.size() == size, expression);
    if (expr.size() != size)
        return;
    size                     = size_min(size, 200);
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
                    test->check(c <= get_elements(expr, cinput, i, vec_shape<T, nsize>()) ==
                                    internal::get_fn_value<T, nsize>(i, fn),
                                expression);
                });
        i += next_size;
    }
}
#define TESTO_CHECK_EXPRESSION(expr, size, ...) ::kfr::test_expression(expr, size, __VA_ARGS__, #expr)

#ifndef TESTO_NO_SHORT_MACROS
#define CHECK_EXPRESSION TESTO_CHECK_EXPRESSION
#endif
#endif

namespace internal
{

template <typename T, typename Fn>
struct expression_lambda : input_expression
{
    using value_type = T;
    KFR_MEM_INTRINSIC expression_lambda(Fn&& fn) : fn(std::move(fn)) {}

    template <size_t N, KFR_ENABLE_IF(N&& is_callable<Fn, cinput_t, size_t, vec_shape<T, N>>)>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_lambda& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return self.fn(cinput, index, y);
    }

    template <size_t N, KFR_ENABLE_IF(N&& is_callable<Fn, size_t>)>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_lambda& self, cinput_t, size_t index,
                                                vec_shape<T, N>)
    {
        return apply(self.fn, enumerate<size_t, N>() + index);
    }
    template <size_t N, KFR_ENABLE_IF(N&& is_callable<Fn>)>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_lambda& self, cinput_t, size_t,
                                                vec_shape<T, N>)
    {
        return apply<N>(self.fn);
    }

    Fn fn;
};
} // namespace internal

template <typename T, typename Fn>
internal::expression_lambda<T, decay<Fn>> lambda(Fn&& fn)
{
    return internal::expression_lambda<T, decay<Fn>>(std::move(fn));
}

} // namespace CMT_ARCH_NAME

namespace internal_generic
{

template <typename T, typename = void>
struct has_static_size_impl : std::false_type
{
};

template <typename T>
struct has_static_size_impl<T, std::void_t<decltype(T::size())>> : std::true_type
{
};

template <typename T, typename = void>
struct is_infinite_impl : std::false_type
{
};

template <typename T>
struct is_infinite_impl<T, void_t<decltype(T::size())>>
    : std::integral_constant<bool, T::size() == infinite_size>
{
};
} // namespace internal_generic

template <typename T>
constexpr inline bool is_infinite = internal_generic::is_infinite_impl<T>::value;

template <typename T>
constexpr inline bool has_static_size = internal_generic::has_static_size_impl<T>::value;

template <typename T>
struct expression_traits<T, std::enable_if_t<std::is_base_of_v<input_expression, T>>>
    : expression_traits_defaults
{
    using value_type             = value_type_of<T>;
    constexpr static size_t dims = 1;

    constexpr static shape<1> shapeof(const T& self) { return self.size(); }
    constexpr static shape<1> shapeof()
    {
        if constexpr (has_static_size<T>)
        {
            return { T::size() };
        }
        else
        {
            return { 0 };
        }
    }
};

inline namespace CMT_ARCH_NAME
{
template <typename T, typename U, size_t N, KFR_ENABLE_IF(is_input_expression<T>)>
KFR_MEM_INTRINSIC vec<U, N> get_elements(T&& self, const shape<1>& index, vec_shape<U, N> sh)
{
    return get_elements(self, cinput_t{}, index[0], sh);
}
} // namespace CMT_ARCH_NAME

template <typename... Args>
struct xwitharguments
{
    constexpr static size_t count = sizeof...(Args);

    using type_list = ctypes_t<Args...>;

    template <size_t idx>
    using nth = typename type_list::template nth<idx>;

    std::tuple<Args...> args;
    std::array<dimset, count> masks;

    KFR_INTRINSIC auto& first() { return std::get<0>(args); }
    KFR_INTRINSIC const auto& first() const { return std::get<0>(args); }

    template <size_t idx>
    KFR_INTRINSIC dimset getmask(csize_t<idx> = {}) const
    {
        static_assert(idx < count);
        using Traits = expression_traits<nth<idx>>;
        if constexpr (Traits::dims == 0)
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

template <typename... Args>
xwitharguments(Args&&... args) -> xwitharguments<Args...>;

template <index_t Dims, typename Arg>
struct xreshape : public xwitharguments<Arg>
{
    shape<Dims> old_shape;
    shape<Dims> new_shape;
};

template <typename Fn, typename... Args>
struct xfunction : public xwitharguments<Args...>
{
    Fn fn;

    KFR_INTRINSIC xfunction(xwitharguments<Args...> args, Fn&& fn)
        : xwitharguments<Args...>{ std::move(args) }, fn(std::move(fn))
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
};

template <index_t Dims, typename Arg>
struct expression_traits<xreshape<Dims, Arg>> : expression_traits_defaults
{
    using value_type             = typename expression_traits<Arg>::value_type;
    constexpr static size_t dims = Dims;

    constexpr static shape<dims> shapeof(const xreshape<Dims, Arg>& self) { return self.new_shape; }
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{
template <index_t outdims, typename Fn, typename... Args, typename U, size_t N, index_t Dims, size_t idx>
KFR_MEM_INTRINSIC vec<U, N> get_arg(const xfunction<Fn, Args...>& self, const shape<Dims>& index,
                                    vec_shape<U, N> sh, csize_t<idx>)
{
    using Traits = expression_traits<typename xfunction<Fn, Args...>::template nth<idx>>;
    if constexpr (Traits::dims == 0)
    {
        return repeat<N>(get_elements(std::get<idx>(self.args), {}, vec_shape<U, 1>{}));
    }
    else
    {
        auto indices               = internal_generic::adapt<Traits::dims>(index, self.getmask(csize<idx>));
        constexpr index_t last_dim = Traits::shapeof().back();
        if constexpr (last_dim > 0)
        {
            return repeat<N / std::min(last_dim, N)>(
                get_elements(std::get<idx>(self.args), indices, vec_shape<U, std::min(last_dim, N)>{}));
        }
        else
        {
            if constexpr (N > 1)
            {
                if (CMT_UNLIKELY(self.masks[idx].back() == 0))
                    return get_elements(std::get<idx>(self.args), indices, vec_shape<U, 1>{}).front();
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

template <typename Fn, typename... Args, typename U, size_t N, index_t Dims>
KFR_MEM_INTRINSIC vec<U, N> get_elements(const xfunction<Fn, Args...>& self, const shape<Dims>& index,
                                         vec_shape<U, N> sh)
{
    constexpr index_t outdims = expression_traits<xfunction<Fn, Args...>>::dims;
    return self.fold_idx(
        [&](auto... idx) CMT_INLINE_LAMBDA -> vec<U, N> {
            return self.fn(internal::get_arg<outdims>(self, index, sh, idx)...);
        });
}

namespace internal
{

template <typename... Args>
struct expression_with_arguments : input_expression
{
    KFR_MEM_INTRINSIC constexpr size_t size() const CMT_NOEXCEPT
    {
        return size_impl(indicesfor_t<Args...>());
    }

    constexpr static size_t count = sizeof...(Args);
    expression_with_arguments()   = delete;
    KFR_MEM_INTRINSIC constexpr expression_with_arguments(Args&&... args) CMT_NOEXCEPT
        : args(std::forward<Args>(args)...)
    {
    }

    KFR_MEM_INTRINSIC void begin_block(cinput_t cinput, size_t size) const
    {
        begin_block_impl(cinput, size, indicesfor_t<Args...>());
    }
    KFR_MEM_INTRINSIC void end_block(cinput_t cinput, size_t size) const
    {
        end_block_impl(cinput, size, indicesfor_t<Args...>());
    }

    std::tuple<Args...> args;

protected:
    template <size_t... indices>
    KFR_MEM_INTRINSIC constexpr size_t size_impl(csizes_t<indices...>) const CMT_NOEXCEPT
    {
        return size_min(std::get<indices>(this->args).size()...);
    }

    template <typename Fn, typename T, size_t N>
    KFR_MEM_INTRINSIC vec<T, N> call(cinput_t cinput, Fn&& fn, size_t index, vec_shape<T, N> x) const
    {
        return call_impl(cinput, std::forward<Fn>(fn), indicesfor_t<Args...>(), index, x);
    }
    template <size_t ArgIndex, typename U, size_t N,
              typename T = value_type_of<typename details::get_nth_type<ArgIndex, Args...>::type>>
    KFR_MEM_INTRINSIC vec<U, N> argument(cinput_t cinput, csize_t<ArgIndex>, size_t index,
                                         vec_shape<U, N>) const
    {
        static_assert(ArgIndex < count, "Incorrect ArgIndex");
        return static_cast<vec<U, N>>(
            get_elements(std::get<ArgIndex>(this->args), cinput, index, vec_shape<T, N>()));
    }
    template <typename U, size_t N,
              typename T = value_type_of<typename details::get_nth_type<0, Args...>::type>>
    KFR_MEM_INTRINSIC vec<U, N> argument_first(cinput_t cinput, size_t index, vec_shape<U, N>) const
    {
        return static_cast<vec<U, N>>(
            get_elements(std::get<0>(this->args), cinput, index, vec_shape<T, N>()));
    }

private:
    template <typename Fn, typename T, size_t N, size_t... indices>
    KFR_MEM_INTRINSIC vec<T, N> call_impl(cinput_t cinput, Fn&& fn, csizes_t<indices...>, size_t index,
                                          vec_shape<T, N>) const
    {
        return fn(get_elements(std::get<indices>(this->args), cinput, index,
                               vec_shape<value_type_of<Args>, N>())...);
    }
    template <size_t... indices>
    KFR_MEM_INTRINSIC void begin_block_impl(cinput_t cinput, size_t size, csizes_t<indices...>) const
    {
        swallow{ (std::get<indices>(args).begin_block(cinput, size), 0)... };
    }
    template <size_t... indices>
    KFR_MEM_INTRINSIC void end_block_impl(cinput_t cinput, size_t size, csizes_t<indices...>) const
    {
        swallow{ (std::get<indices>(args).end_block(cinput, size), 0)... };
    }
};

template <typename T>
struct expression_scalar : input_expression
{
    using value_type    = T;
    expression_scalar() = delete;
    constexpr expression_scalar(const T& val) CMT_NOEXCEPT : val(val) {}
    T val;

    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_scalar& self, cinput_t, size_t,
                                                vec_shape<T, N>)
    {
        return broadcast<N>(self.val);
    }
};

template <typename T1, typename T2, typename = void>
struct arg_impl
{
    using type       = T2;
    using value_type = typename T1::value_type;
};

template <typename T1, typename T2>
struct arg_impl<T1, T2, void_t<enable_if<is_vec_element<T1>>>>
{
    using type       = expression_scalar<T1>;
    using value_type = T1;
};

template <typename T>
using arg = typename internal::arg_impl<decay<T>, T>::type;

template <typename T>
using arg_type = typename internal::arg_impl<decay<T>, T>::value_type;

template <typename Fn, typename... Args>
struct function_value_type
{
    using type = typename invoke_result<Fn, vec<arg_type<Args>, 1>...>::value_type;
};

template <typename Fn, typename... Args>
struct expression_function : expression_with_arguments<arg<Args>...>
{
    using value_type = typename function_value_type<Fn, Args...>::type;
    // subtype<decltype(std::declval<Fn>()(std::declval<vec<value_type_of<arg<Args>>, 1>>()...))>;
    using T = value_type;

    expression_function(Fn&& fn, arg<Args>&&... args) CMT_NOEXCEPT
        : expression_with_arguments<arg<Args>...>(std::forward<arg<Args>>(args)...),
          fn(std::forward<Fn>(fn))
    {
    }
    expression_function(const Fn& fn, arg<Args>&&... args) CMT_NOEXCEPT
        : expression_with_arguments<arg<Args>...>(std::forward<arg<Args>>(args)...),
          fn(fn)
    {
    }
    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_function& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> x)
    {
        return self.call(cinput, self.fn, index, x);
    }

    const Fn& get_fn() const CMT_NOEXCEPT { return fn; }

protected:
    Fn fn;
};
} // namespace internal

template <typename A>
CMT_INTRINSIC internal::arg<A> e(A&& a)
{
    return internal::arg<A>(std::forward<A>(a));
}

template <typename T>
CMT_INTRINSIC internal::expression_scalar<T> scalar(const T& val)
{
    return internal::expression_scalar<T>(val);
}

template <typename Fn, typename... Args>
CMT_INTRINSIC internal::expression_function<decay<Fn>, Args...> bind_expression(Fn&& fn, Args&&... args)
{
    return internal::expression_function<decay<Fn>, Args...>(std::forward<Fn>(fn),
                                                             std::forward<Args>(args)...);
}
/**
 * @brief Construct a new expression using the same function as in @c e and new arguments
 * @param e an expression
 * @param args new arguments for the function
 */
template <typename Fn, typename... OldArgs, typename... NewArgs>
CMT_INTRINSIC internal::expression_function<Fn, NewArgs...> rebind(
    const internal::expression_function<Fn, OldArgs...>& e, NewArgs&&... args)
{
    return internal::expression_function<Fn, NewArgs...>(e.get_fn(), std::forward<NewArgs>(args)...);
}

template <size_t width = 0, typename OutputExpr, typename InputExpr, size_t groupsize = 1,
          typename Tvec = vec<value_type_of<InputExpr>, 1>>
static size_t process(OutputExpr&& out, const InputExpr& in, size_t start = 0, size_t size = infinite_size,
                      coutput_t coutput = nullptr, cinput_t cinput = nullptr,
                      csize_t<groupsize> = csize_t<groupsize>())
{
    using Tin = value_type_of<InputExpr>;
    static_assert(is_output_expression<OutputExpr>, "OutFn must be an expression");
    static_assert(is_input_expression<InputExpr>, "Fn must be an expression");

    size = size_sub(size_min(out.size(), in.size(), size_add(size, start)), start);
    if (CMT_UNLIKELY(size == 0 || size == infinite_size))
        return size;
    out.begin_block(coutput, size);
    in.begin_block(cinput, size);

#ifdef NDEBUG
    constexpr size_t w = width == 0 ? maximum_vector_size<Tin> : width;
#else
    constexpr size_t w         = width == 0 ? vector_width<Tin> : width;
#endif

    static_assert(w > 0 && is_poweroftwo(w), "");

    size_t i = start;

    CMT_LOOP_NOUNROLL
    for (; i < start + size / w * w; i += w)
        set_elements(out, coutput, i, get_elements(in, cinput, i, vec_shape<Tin, w>()));
    CMT_LOOP_NOUNROLL
    for (; i < start + size / groupsize * groupsize; i += groupsize)
        set_elements(out, coutput, i, get_elements(in, cinput, i, vec_shape<Tin, groupsize>()));

    in.end_block(cinput, size);
    out.end_block(coutput, size);
    return size;
}

template <typename T>
struct input_expression_base : input_expression
{
    virtual ~input_expression_base() {}
    virtual T input(size_t index) const = 0;
    template <typename U, size_t N>
    friend KFR_INTRINSIC vec<U, N> get_elements(const input_expression_base& self, cinput_t, size_t index,
                                                vec_shape<U, N>)
    {
        vec<U, N> out;
        for (size_t i = 0; i < N; i++)
            out[i] = static_cast<U>(self.input(index + i));
        return out;
    }
};

template <typename T>
struct output_expression_base : output_expression
{
    virtual ~output_expression_base() {}
    virtual void output(size_t index, const T& value) = 0;

    template <typename U, size_t N>
    friend KFR_INTRINSIC void set_elements(const output_expression_base& self, coutput_t, size_t index,
                                           const vec<U, N>& value)
    {
        for (size_t i = 0; i < N; i++)
            self.output(index + i, static_cast<T>(value[i]));
    }
};

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>)>
CMT_INTRINSIC internal::expression_function<fn::interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return { fn::interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
