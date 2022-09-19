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

#include "expression.hpp"

namespace kfr
{

// ----------------------------------------------------------------------------

template <typename T, typename Arg>
struct xcastto : public xwitharguments<Arg>
{
    using xwitharguments<Arg>::xwitharguments;
};

template <typename T, typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
KFR_INTRINSIC xcastto<T, Arg> x_castto(Arg&& arg)
{
    return { std::forward<Arg>(arg) };
}

template <typename T, typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
KFR_INTRINSIC xcastto<T, Arg> x_castto(Arg&& arg, ctype_t<T>)
{
    return { std::forward<Arg>(arg) };
}

template <typename T, typename Arg>
struct expression_traits<xcastto<T, Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type             = T;
    constexpr static size_t dims = ArgTraits::dims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xcastto<T, Arg>& self)
    {
        return ArgTraits::shapeof(self.first());
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return ArgTraits::shapeof(); }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xcastto<T, Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index, sh));
}

template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const xcastto<T, Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    set_elements(self.first(), index, sh, value);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename T, index_t Dims, typename Fn>
struct xlambda
{
    Fn&& fn;
};

template <typename T, index_t Dims = 1, typename Fn>
KFR_INTRINSIC xlambda<T, Dims, Fn> x_lambda(Fn&& fn)
{
    return { std::forward<Fn>(fn) };
}

template <typename T, index_t Dims, typename Fn>
struct expression_traits<xlambda<T, Dims, Fn>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    KFR_MEM_INTRINSIC constexpr static shape<Dims> shapeof(const xlambda<T, Dims, Fn>& self)
    {
        return infinite_size;
    }
    KFR_MEM_INTRINSIC constexpr static shape<Dims> shapeof() { return infinite_size; }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t Dims, typename Fn, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xlambda<T, Dims, Fn>& self, const shape<Dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    if constexpr (std::is_callable_v<Fn, shape<Dims>, csize_t<N>>)
        return self.fn(index, sh);
    else if constexpr (std::is_callable_v<Fn, shape<Dims>>)
        return vec<T, N>{ [&](size_t idx) { return self.fn(index.add(idx)); } };
    else if constexpr (std::is_callable_v<Fn>)
        return apply<N>(self.fn);
    else
        return czeros;
}

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename Arg>
struct xpadded : public xwitharguments<Arg>
{
    using ArgTraits = typename xwitharguments<Arg>::first_arg_trait;
    typename ArgTraits::value_type fill_value;
    shape<ArgTraits::dims> input_shape;

    KFR_MEM_INTRINSIC xpadded(Arg&& arg, typename ArgTraits::value_type fill_value)
        : xwitharguments<Arg>{ std::forward<Arg>(arg) }, fill_value(std::move(fill_value)),
          input_shape(ArgTraits::shapeof(this->first()))
    {
    }
};

template <typename Arg, typename T = expression_value_type<Arg>>
KFR_INTRINSIC xpadded<Arg> x_padded(Arg&& arg, T fill_value = T{})
{
    static_assert(expression_dims<Arg> >= 1);
    return { std::forward<Arg>(arg), std::move(fill_value) };
}

template <typename Arg>
struct expression_traits<xpadded<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type             = typename ArgTraits::value_type;
    constexpr static size_t dims = ArgTraits::dims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xpadded<Arg>& self) { return infinite_size; }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return infinite_size; }
};

inline namespace CMT_ARCH_NAME
{

template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<xpadded<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xpadded<Arg>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    if (index.ge(self.input_size))
    {
        return self.fill_value;
    }
    else if (CMT_LIKELY(index.add(N).le(self.input_size)))
    {
        return get_elements(self.first(), index, sh);
    }
    else
    {
        vec<T, N> x = self.fill_value;
        for (size_t i = 0; i < N; i++)
        {
            shape ish = index.add(i);
            if (ish.back() < self.input_size.back())
                x[i] = get_elements(self.first(), ish, csize_t<1>()).front();
        }
        return x;
    }
}

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename Arg>
struct xreverse : public xwitharguments<Arg>
{
    using ArgTraits = typename xwitharguments<Arg>::first_arg_trait;
    shape<ArgTraits::dims> input_shape;

    KFR_MEM_INTRINSIC xreverse(Arg&& arg)
        : xwitharguments<Arg>{ std::forward<Arg>(arg) }, input_shape(ArgTraits::shapeof(this->first()))
    {
    }
};

template <typename Arg>
KFR_INTRINSIC xreverse<Arg> x_reverse(Arg&& arg)
{
    static_assert(expression_dims<Arg> >= 1);
    return { std::forward<Arg>(arg) };
}

template <typename Arg>
struct expression_traits<xreverse<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type             = typename ArgTraits::value_type;
    constexpr static size_t dims = ArgTraits::dims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xreverse<Arg>& self)
    {
        return ArgTraits::shapeof(self.first());
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return ArgTraits::shapeof(); }
};

inline namespace CMT_ARCH_NAME
{

template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<xreverse<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xreverse<Arg>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return reverse(get_elements(self.first(), self.input_shape.sub(index).sub(shape<Traits::dims>(N)), sh));
}

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <index_t... Values>
struct fixed_shape
{
    constexpr static shape<sizeof...(Values)> get() { return { Values... }; }
};

template <typename Arg, typename Shape>
struct xfixshape : public xwitharguments<Arg>
{
    using ArgTraits = typename xwitharguments<Arg>::first_arg_trait;

    KFR_MEM_INTRINSIC xfixshape(Arg&& arg) : xwitharguments<Arg>{ std::forward<Arg>(arg) } {}
};

template <typename Arg, index_t... ShapeValues>
KFR_INTRINSIC xfixshape<Arg, fixed_shape<ShapeValues...>> x_fixshape(Arg&& arg,
                                                                     const fixed_shape<ShapeValues...>&)
{
    return { std::forward<Arg>(arg) };
}

template <typename Arg, index_t... ShapeValues>
struct expression_traits<xfixshape<Arg, fixed_shape<ShapeValues...>>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type             = typename ArgTraits::value_type;
    constexpr static size_t dims = ArgTraits::dims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(
        const xfixshape<Arg, fixed_shape<ShapeValues...>>& self)
    {
        return fixed_shape<ShapeValues...>::get();
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return fixed_shape<ShapeValues...>::get(); }
};

inline namespace CMT_ARCH_NAME
{

template <typename Arg, typename Shape, index_t Axis, size_t N,
          typename Traits = expression_traits<xfixshape<Arg, Shape>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xfixshape<Arg, Shape>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return get_elements(self.first(), index, sh);
}

template <typename Arg, typename Shape, index_t Axis, size_t N,
          typename Traits = expression_traits<xfixshape<Arg, Shape>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC void set_elements(xfixshape<Arg, Shape>& self, const shape<Traits::dims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    set_elements(self.first(), index, sh, value);
}

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename Arg, index_t OutDims>
struct xreshape : public xwitharguments<Arg>
{
    using ArgTraits = typename xwitharguments<Arg>::first_arg_trait;
    shape<ArgTraits::dims> in_shape;
    shape<OutDims> out_shape;

    KFR_MEM_INTRINSIC xreshape(Arg&& arg, const shape<OutDims>& out_shape)
        : xwitharguments<Arg>{ std::forward<Arg>(arg) }, in_shape(ArgTraits::shapeof(arg)),
          out_shape(out_shape)
    {
    }
};

template <typename Arg, index_t OutDims>
KFR_INTRINSIC xreshape<Arg, OutDims> x_reshape(Arg&& arg, const shape<OutDims>& out_shape)
{
    return { std::forward<Arg>(arg), out_shape };
}

template <typename Arg, index_t OutDims>
struct expression_traits<xreshape<Arg, OutDims>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type             = typename ArgTraits::value_type;
    constexpr static size_t dims = OutDims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xreshape<Arg, OutDims>& self)
    {
        return self.out_shape;
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return shape<dims>{ 0 }; }
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{
} // namespace internal

template <typename Arg, index_t outdims, index_t Axis, size_t N,
          typename Traits = expression_traits<xreshape<Arg, outdims>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xreshape<Arg, outdims>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
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
                     [&](auto n) CMT_INLINE_LAMBDA
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
            CMT_LOOP_NOUNROLL
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

template <typename Arg, index_t outdims, index_t Axis, size_t N,
          typename Traits = expression_traits<xreshape<Arg, outdims>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC void set_elements(xreshape<Arg, outdims>& self, const shape<Traits::dims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
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
                 [&](auto n) CMT_INLINE_LAMBDA
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
            CMT_LOOP_NOUNROLL
            for (size_t i = 0; i < N; ++i)
            {
                set_elements(self.first(),
                             self.in_shape.from_flat(self.out_shape.to_flat(index.add_at(i, cindex<Axis>))),
                             axis_params<indims - 1, 1>{}, vec<T, 1>{ value[i] });
            }
        }
    }
}

} // namespace CMT_ARCH_NAME

} // namespace kfr
