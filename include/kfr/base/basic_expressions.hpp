/** @addtogroup expressions
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

#include "expression.hpp"

namespace kfr
{
// ----------------------------------------------------------------------------

template <typename T>
struct xscalar
{
    T value;
};

template <typename T>
struct expression_traits<xscalar<T>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 0;

    constexpr static shape<0> shapeof(const xscalar<T>& self) { return {}; }
    constexpr static shape<0> shapeof() { return {}; }
};

template <typename T>
KFR_INTRINSIC xscalar<T> scalar(T value)
{
    return { std::move(value) };
}

template <typename T = fbase>
KFR_INTRINSIC xscalar<T> zeros()
{
    return { static_cast<T>(0) };
}

template <typename T = fbase>
KFR_INTRINSIC xscalar<T> ones()
{
    return { static_cast<T>(1) };
}

inline namespace CMT_ARCH_NAME
{
template <typename T, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xscalar<T>& self, const shape<0>& index,
                                     const axis_params<Axis, N>&)
{
    return self.value;
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename T, index_t Dims = 1>
struct xcounter
{
    T start;
    T steps[Dims];

    T back() const { return steps[Dims - 1]; }
    T front() const { return steps[0]; }
};

template <typename T, index_t Dims>
struct expression_traits<xcounter<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    constexpr static shape<dims> shapeof(const xcounter<T, Dims>& self) { return shape<dims>(infinite_size); }
    constexpr static shape<dims> shapeof() { return shape<dims>(infinite_size); }
};

template <typename T, typename... Args, typename Tout = std::common_type_t<T, Args...>>
KFR_INTRINSIC xcounter<Tout, sizeof...(Args)> counter(T start, Args... steps)
{
    return { static_cast<Tout>(std::move(start)), { static_cast<Tout>(std::move(steps))... } };
}

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xcounter<T, 1>& self, const shape<1>& index,
                                     const axis_params<Axis, N>&)
{
    T acc = self.start;
    acc += static_cast<T>(index.back()) * self.back();
    return acc + enumerate(vec_shape<T, N>(), self.back());
}
template <typename T, index_t dims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xcounter<T, dims>& self, const shape<dims>& index,
                                     const axis_params<Axis, N>&)
{
    T acc                 = self.start;
    vec<T, dims> tindices = cast<T>(*index);
    cfor(csize<0>, csize<dims>, [&](auto i) CMT_INLINE_LAMBDA { acc += tindices[i] * self.steps[i]; });
    return acc + enumerate(vec_shape<T, N>(), self.steps[Axis]);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename Arg>
struct xslice : public xwitharguments<Arg>
{
    constexpr static index_t dims = expression_dims<Arg>;
    shape<dims> start;
    shape<dims> size;

    KFR_MEM_INTRINSIC xslice(Arg&& arg, shape<dims> start, shape<dims> size)
        : xwitharguments<Arg>{ std::forward<Arg>(arg) }, start(start), size(size)
    {
    }
};

template <typename Arg>
struct expression_traits<xslice<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xslice<Arg>& self)
    {
        return min(sub_shape(ArgTraits::shapeof(self.first()), self.start), self.size);
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return shape<dims>(undefined_size); }
};

template <typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg), index_t Dims = expression_dims<Arg>>
KFR_INTRINSIC xslice<Arg> slice(Arg&& arg, shape<Dims> start, shape<Dims> size)
{
    return { std::forward<Arg>(arg), start, size };
}

template <typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg), index_t Dims = expression_dims<Arg>>
KFR_INTRINSIC xslice<Arg> truncate(Arg&& arg, shape<Dims> size)
{
    return { std::forward<Arg>(arg), shape<Dims>{ 0 }, size };
}

inline namespace CMT_ARCH_NAME
{

template <typename Arg, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<xslice<Arg>>::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xslice<Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index.add(self.start), sh));
}

template <typename Arg, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<xslice<Arg>>::value_type>
KFR_INTRINSIC void set_elements(const xslice<Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    set_elements(self.first(), index.add(self.start), sh, value);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename T, typename Arg>
struct xcast : public xwitharguments<Arg>
{
    using xwitharguments<Arg>::xwitharguments;
};

template <typename T, typename Arg>
struct expression_traits<xcast<T, Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = T;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xcast<T, Arg>& self)
    {
        return ArgTraits::shapeof(self.first());
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return ArgTraits::shapeof(); }
};

template <typename T, typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
KFR_INTRINSIC xcast<T, Arg> cast(Arg&& arg)
{
    return { std::forward<Arg>(arg) };
}

template <typename T, typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
KFR_INTRINSIC xcast<T, Arg> cast(Arg&& arg, ctype_t<T>)
{
    return { std::forward<Arg>(arg) };
}

inline namespace CMT_ARCH_NAME
{

template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xcast<T, Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index, sh));
}

template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const xcast<T, Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    set_elements(self.first(), index, sh, value);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename T, index_t Dims, typename Fn, bool Rnd>
struct xlambda
{
    Fn&& fn;
};

template <typename T, index_t Dims, typename Fn, bool Rnd>
struct expression_traits<xlambda<T, Dims, Fn, Rnd>> : expression_traits_defaults
{
    using value_type                           = T;
    constexpr static size_t dims               = Dims;
    constexpr static inline bool random_access = Rnd;

    KFR_MEM_INTRINSIC constexpr static shape<Dims> shapeof(const xlambda<T, Dims, Fn, Rnd>& self)
    {
        return shape<Dims>(infinite_size);
    }
    KFR_MEM_INTRINSIC constexpr static shape<Dims> shapeof() { return shape<Dims>(infinite_size); }
};

template <typename T, index_t Dims = 1, typename Fn, bool RandomAccess = true>
KFR_INTRINSIC xlambda<T, Dims, Fn, RandomAccess> lambda(Fn&& fn, cbool_t<RandomAccess> = {})
{
    return { std::forward<Fn>(fn) };
}
template <typename T, index_t Dims = 1, typename Fn>
KFR_INTRINSIC xlambda<T, Dims, Fn, false> lambda_generator(Fn&& fn)
{
    return { std::forward<Fn>(fn) };
}

template <typename... Ts, typename T = std::common_type_t<Ts...>>
KFR_INTRINSIC auto sequence(const Ts&... list)
{
    return lambda<T>([seq = std::array<T, sizeof...(Ts)>{ { static_cast<T>(list)... } }](size_t index)
                     { return seq[index % seq.size()]; });
}

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t Dims, typename Fn, bool Rnd, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xlambda<T, Dims, Fn, Rnd>& self, const shape<Dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    if constexpr (std::is_invocable_v<Fn, shape<Dims>, csize_t<N>>)
        return self.fn(index, sh);
    else if constexpr (std::is_invocable_v<Fn, shape<Dims>>)
        return vec<T, N>{ [&](size_t idx) { return self.fn(index.add(idx)); } };
    else if constexpr (std::is_invocable_v<Fn>)
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
KFR_INTRINSIC xpadded<Arg> padded(Arg&& arg, T fill_value = T{})
{
    static_assert(expression_dims<Arg> >= 1);
    return { std::forward<Arg>(arg), std::move(fill_value) };
}

template <typename Arg>
struct expression_traits<xpadded<Arg>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xpadded<Arg>& self)
    {
        return shape<dims>(infinite_size);
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return shape<dims>(infinite_size); }
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
    static_assert(ArgTraits::random_access, "xreverse requires an expression with random access");

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
KFR_INTRINSIC xfixshape<Arg, fixed_shape<ShapeValues...>> fixshape(Arg&& arg,
                                                                   const fixed_shape<ShapeValues...>&)
{
    return { std::forward<Arg>(arg) };
}

template <typename Arg, index_t... ShapeValues>
struct expression_traits<xfixshape<Arg, fixed_shape<ShapeValues...>>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = ArgTraits::dims;
    constexpr static bool random_access = ArgTraits::random_access;

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
KFR_INTRINSIC xreshape<Arg, OutDims> reshape(Arg&& arg, const shape<OutDims>& out_shape)
{
    return { std::forward<Arg>(arg), out_shape };
}

template <typename Arg, index_t OutDims>
struct expression_traits<xreshape<Arg, OutDims>> : expression_traits_defaults
{
    using ArgTraits = expression_traits<Arg>;

    using value_type                    = typename ArgTraits::value_type;
    constexpr static size_t dims        = OutDims;
    constexpr static bool random_access = ArgTraits::random_access;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xreshape<Arg, OutDims>& self)
    {
        return self.out_shape;
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return shape<dims>{ undefined_size }; }
};

inline namespace CMT_ARCH_NAME
{

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

// ----------------------------------------------------------------------------

template <typename T, bool truncated = true>
struct xlinspace
{
    T start;
    T stop;
    index_t size;
    bool endpoint;
};

template <typename T, bool truncated>
struct expression_traits<xlinspace<T, truncated>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;

    constexpr static shape<dims> shapeof(const xlinspace<T, truncated>& self)
    {
        return shape<dims>(truncated ? self.size : infinite_size);
    }
    constexpr static shape<dims> shapeof() { return shape<dims>(truncated ? undefined_size : infinite_size); }
};

template <bool truncated = false, typename T1, typename T2, typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC xlinspace<Tout, truncated> linspace(T1 start, T2 stop, size_t size, bool endpoint = false)
{
    return { static_cast<Tout>(start), static_cast<Tout>(stop), size, endpoint };
}

inline namespace CMT_ARCH_NAME
{

template <typename T, bool truncated, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xlinspace<T, truncated>& self, const shape<1>& index,
                                     const axis_params<0, N>&)
{
    T acc    = self.start;
    using TI = itype<T>;
    return mix(enumerate(vec_shape<T, N>(), static_cast<T>(static_cast<TI>(index))) * self.invsize,
               self.start, self.stop);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename Arg1, typename Arg2, index_t ConcatAxis>
struct xconcatenate : public xwitharguments<Arg1, Arg2>
{
    static_assert(expression_dims<Arg1> == expression_dims<Arg2>);
    static_assert(std::is_same_v<expression_value_type<Arg1>, expression_value_type<Arg2>>);
    constexpr static index_t dims = expression_dims<Arg1>;
    shape<dims> size1;

    KFR_MEM_INTRINSIC xconcatenate(Arg1&& arg1, Arg2&& arg2)
        : xwitharguments<Arg1, Arg2>{ std::forward<Arg1>(arg1), std::forward<Arg2>(arg2) },
          size1(expression_traits<Arg1>::shapeof(arg1))
    {
    }
};

template <typename Arg1, typename Arg2, index_t ConcatAxis>
struct expression_traits<xconcatenate<Arg1, Arg2, ConcatAxis>> : expression_traits_defaults
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

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const xconcatenate<Arg1, Arg2, ConcatAxis>& self)
    {
        return concat_shape(ArgTraits1::shapeof(std::get<0>(self)), ArgTraits2::shapeof(std::get<1>(self)));
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof()
    {
        return concat_shape(ArgTraits1::shapeof(), ArgTraits2::shapeof());
    }
};

template <index_t ConcatAxis = 0, typename Arg1, typename Arg2, KFR_ACCEPT_EXPRESSIONS(Arg1, Arg2)>
KFR_INTRINSIC xconcatenate<Arg1, Arg2, ConcatAxis> concatenate(Arg1&& arg1, Arg2&& arg2)
{
    return { std::forward<Arg1>(arg1), std::forward<Arg2>(arg2) };
}

template <index_t ConcatAxis = 0, typename Arg1, typename Arg2, typename Arg3,
          KFR_ACCEPT_EXPRESSIONS(Arg1, Arg2, Arg3)>
KFR_INTRINSIC xconcatenate<Arg1, xconcatenate<Arg2, Arg3, ConcatAxis>, ConcatAxis> concatenate(Arg1&& arg1,
                                                                                               Arg2&& arg2,
                                                                                               Arg3&& arg3)
{
    return { std::forward<Arg1>(arg1), { std::forward<Arg2>(arg2), std::forward<Arg3>(arg3) } };
}

inline namespace CMT_ARCH_NAME
{

template <typename Arg1, typename Arg2, index_t ConcatAxis, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<xconcatenate<Arg1, Arg2, ConcatAxis>>::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const xconcatenate<Arg1, Arg2, ConcatAxis>& self,
                                     const shape<NDims>& index, const axis_params<Axis, N>& sh)
{
    const index_t size1     = self.size1;
    constexpr index_t Naxis = ConcatAxis == Axis ? N : 1;
    if (index[ConcatAxis] >= size1[ConcatAxis])
    {
        shape index1 = index;
        index1[ConcatAxis] -= size1[ConcatAxis];
        return get_elements(std::get<1>(self.args), index1, sh);
    }
    else if (CMT_LIKELY(index[ConcatAxis] + Naxis <= size1[ConcatAxis]))
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

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

} // namespace kfr
