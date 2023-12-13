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

#include "expression.hpp"

namespace kfr
{
// ----------------------------------------------------------------------------

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

template <typename T>
KFR_INTRINSIC expression_scalar<T> scalar(T value)
{
    return { std::move(value) };
}

template <typename T = fbase>
KFR_INTRINSIC expression_scalar<T> zeros()
{
    return { static_cast<T>(0) };
}

template <typename T = fbase>
KFR_INTRINSIC expression_scalar<T> ones()
{
    return { static_cast<T>(1) };
}

inline namespace CMT_ARCH_NAME
{
template <typename T, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_scalar<T>& self, const shape<0>& index,
                                     const axis_params<Axis, N>&)
{
    return self.value;
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <typename T = int, typename Tout = T>
KFR_INTRINSIC expression_counter<Tout, 1> counter(T start = 0)
{
    return { static_cast<Tout>(std::move(start)), { static_cast<Tout>(1) } };
}

template <typename T = int, typename Arg = T, typename... Args,
          typename Tout = std::common_type_t<T, Arg, Args...>>
KFR_INTRINSIC expression_counter<Tout, 1 + sizeof...(Args)> counter(T start, Arg step, Args... steps)
{
    return { static_cast<Tout>(std::move(start)),
             { static_cast<Tout>(std::move(step)), static_cast<Tout>(std::move(steps))... } };
}

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_counter<T, 1>& self, const shape<1>& index,
                                     const axis_params<Axis, N>&)
{
    T acc = self.start;
    acc += static_cast<T>(index.back()) * self.back();
    return acc + enumerate(vec_shape<T, N>(), self.back());
}
template <typename T, index_t dims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_counter<T, dims>& self, const shape<dims>& index,
                                     const axis_params<Axis, N>&)
{
    T acc                 = self.start;
    vec<T, dims> tindices = cast<T>(to_vec(index));
    cfor(csize<0>, csize<dims>, [&](auto i) CMT_INLINE_LAMBDA { acc += tindices[i] * self.steps[i]; });
    return acc + enumerate(vec_shape<T, N>(), self.steps[Axis]);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg), index_t Dims = expression_dims<Arg>>
KFR_INTRINSIC expression_slice<Arg> slice(Arg&& arg, identity<shape<Dims>> start,
                                          identity<shape<Dims>> size = shape<Dims>(infinite_size))
{
    static_assert(Dims > 0);
    return { std::forward<Arg>(arg), start, size };
}

template <typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg), index_t Dims = expression_dims<Arg>>
KFR_INTRINSIC expression_slice<Arg> truncate(Arg&& arg, identity<shape<Dims>> size)
{
    static_assert(Dims > 0);
    return { std::forward<Arg>(arg), shape<Dims>{ 0 }, size };
}

inline namespace CMT_ARCH_NAME
{

template <typename Arg, index_t NDims, index_t Axis, size_t N,
          typename T = typename expression_traits<expression_slice<Arg>>::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_slice<Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index.add(self.start), sh));
}

template <typename Arg, index_t NDims, index_t Axis, size_t N, enable_if_output_expression<Arg>* = nullptr,
          typename T = typename expression_traits<expression_slice<Arg>>::value_type>
KFR_INTRINSIC void set_elements(const expression_slice<Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    set_elements(self.first(), index.add(self.start), sh, value);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <typename T, typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
KFR_INTRINSIC expression_cast<T, Arg> cast(Arg&& arg)
{
    return { std::forward<Arg>(arg) };
}

template <typename T, typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
KFR_INTRINSIC expression_cast<T, Arg> cast(Arg&& arg, ctype_t<T>)
{
    return { std::forward<Arg>(arg) };
}

inline namespace CMT_ARCH_NAME
{

template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_cast<T, Arg>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return static_cast<vec<T, N>>(get_elements(self.first(), index, sh));
}

template <typename T, typename Arg, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const expression_cast<T, Arg>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    set_elements(self.first(), index, sh, value);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <typename T, index_t Dims = 1, typename Fn, bool RandomAccess = true>
KFR_INTRINSIC expression_lambda<T, Dims, Fn, RandomAccess> lambda(Fn&& fn, cbool_t<RandomAccess> = {})
{
    return { std::forward<Fn>(fn) };
}
template <typename T, index_t Dims = 1, typename Fn>
KFR_INTRINSIC expression_lambda<T, Dims, Fn, false> lambda_generator(Fn&& fn)
{
    return { std::forward<Fn>(fn) };
}

template <typename... Ts, typename T = std::common_type_t<Ts...>>
KFR_INTRINSIC auto sequence(const Ts&... list)
{
    return lambda<T>([seq = std::array<T, sizeof...(Ts)>{ { static_cast<T>(list)... } }](index_t index) { //
        return seq[index % seq.size()];
    });
}

inline namespace CMT_ARCH_NAME
{

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

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <typename Arg>
struct expression_padded : public expression_with_arguments<Arg>
{
    using ArgTraits = typename expression_with_arguments<Arg>::first_arg_traits;
    typename ArgTraits::value_type fill_value;
    shape<ArgTraits::dims> input_shape;

    KFR_MEM_INTRINSIC expression_padded(Arg&& arg, typename ArgTraits::value_type fill_value)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) }, fill_value(std::move(fill_value)),
          input_shape(ArgTraits::get_shape(this->first()))
    {
    }
};

template <typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg), typename T = expression_value_type<Arg>>
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

inline namespace CMT_ARCH_NAME
{

template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<expression_padded<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_padded<Arg>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    if (index.ge(self.input_shape))
    {
        return self.fill_value;
    }
    else if (CMT_LIKELY(index.add(N).le(self.input_shape)))
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

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <typename Arg, KFR_ACCEPT_EXPRESSIONS(Arg)>
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

inline namespace CMT_ARCH_NAME
{

template <typename Arg, index_t Axis, size_t N, typename Traits = expression_traits<expression_reverse<Arg>>,
          typename T = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_reverse<Arg>& self, const shape<Traits::dims>& index,
                                     const axis_params<Axis, N>& sh)
{
    return reverse(get_elements(self.first(), self.input_shape.sub(index).sub(shape<Traits::dims>(N)), sh));
}

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

template <index_t... Values>
struct fixed_shape_t
{
    constexpr fixed_shape_t() = default;
    constexpr static shape<sizeof...(Values)> get() { return { Values... }; }
};

template <index_t... Values>
constexpr inline fixed_shape_t<Values...> fixed_shape{};

template <typename Arg, typename Shape>
struct expression_fixshape : public expression_with_arguments<Arg>
{
    using ArgTraits = typename expression_with_arguments<Arg>::first_arg_traits;

    KFR_MEM_INTRINSIC expression_fixshape(Arg&& arg)
        : expression_with_arguments<Arg>{ std::forward<Arg>(arg) }
    {
    }
};

template <typename Arg, index_t... ShapeValues, KFR_ACCEPT_EXPRESSIONS(Arg)>
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

inline namespace CMT_ARCH_NAME
{

template <typename Arg, typename Shape, index_t Axis, size_t N,
          typename Traits = expression_traits<expression_fixshape<Arg, Shape>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC vec<T, N> get_elements(const expression_fixshape<Arg, Shape>& self,
                                     const shape<Traits::dims>& index, const axis_params<Axis, N>& sh)
{
    using ArgTraits = expression_traits<Arg>;
    return get_elements(self.first(), index.template trim<ArgTraits::dims>(), sh);
}

template <typename Arg, typename Shape, index_t Axis, size_t N,
          typename Traits = expression_traits<expression_fixshape<Arg, Shape>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC void set_elements(expression_fixshape<Arg, Shape>& self, const shape<Traits::dims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
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

} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <typename Arg, index_t OutDims, KFR_ACCEPT_EXPRESSIONS(Arg)>
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

inline namespace CMT_ARCH_NAME
{

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
          typename Traits = expression_traits<expression_reshape<Arg, outdims>>,
          typename T      = typename Traits::value_type>
KFR_INTRINSIC void set_elements(expression_reshape<Arg, outdims>& self, const shape<Traits::dims>& index,
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

struct symmetric_linspace_t
{
};
constexpr inline const symmetric_linspace_t symmetric_linspace{};

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

/** @brief Returns evenly spaced numbers over a specified interval.
 *
 * @param start The starting value of the sequence
 * @param stop The end value of the sequence. if ``endpoint`` is ``false``, the last value is excluded
 * @param size Number of samples to generate
 * @param endpoint If ``true``, ``stop`` is the last sample. Otherwise, it is not included
 * @tparam truncated If ``true``, linspace returns exactly size elements, otherwise, returns infinite sequence
 * @tparam precise No longer used since KFR5, calculations are always precise
 */
template <typename T = void, bool precise = false, bool truncated = false, typename T1, typename T2,
          typename Tout = or_type<T, ftype<std::common_type_t<T1, T2>>>>
KFR_INTRINSIC expression_linspace<Tout, truncated> linspace(T1 start, T2 stop, size_t size,
                                                            bool endpoint = false, cbool_t<truncated> = {})
{
    return { static_cast<Tout>(start), static_cast<Tout>(stop), size, endpoint };
}

/** @brief Returns evenly spaced numbers over a specified interval.
 *
 * @param symsize The sequence will have interval [-symsize..symsize]
 * @param size Number of samples to generate
 * @tparam truncated If ``true``, linspace returns exactly size elements, otherwise, returns infinite sequence
 * @tparam precise No longer used since KFR5, calculations are always precise
 */
template <typename T, bool precise = false, bool truncated = false, typename Tout = ftype<T>>
KFR_INTRINSIC expression_linspace<Tout, truncated> symmlinspace(T symsize, size_t size,
                                                                cbool_t<truncated> = {})
{
    return { symmetric_linspace, static_cast<Tout>(symsize), size, true };
}

inline namespace CMT_ARCH_NAME
{

template <typename T, bool truncated, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_linspace<T, truncated>& self, const shape<1>& index,
                                     const axis_params<0, N>&)
{
    using TI = itype<T>;
    return mix((enumerate(vec_shape<T, N>()) + static_cast<T>(static_cast<TI>(index.front()))) * self.invsize,
               self.start, self.stop);
}
} // namespace CMT_ARCH_NAME

// ----------------------------------------------------------------------------

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

template <index_t ConcatAxis = 0, typename Arg1, typename Arg2, KFR_ACCEPT_EXPRESSIONS(Arg1, Arg2)>
KFR_INTRINSIC expression_concatenate<Arg1, Arg2, ConcatAxis> concatenate(Arg1&& arg1, Arg2&& arg2)
{
    return { std::forward<Arg1>(arg1), std::forward<Arg2>(arg2) };
}

template <index_t ConcatAxis = 0, typename Arg1, typename Arg2, typename Arg3,
          KFR_ACCEPT_EXPRESSIONS(Arg1, Arg2, Arg3)>
KFR_INTRINSIC expression_concatenate<Arg1, expression_concatenate<Arg2, Arg3, ConcatAxis>, ConcatAxis>
concatenate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
{
    return { std::forward<Arg1>(arg1), { std::forward<Arg2>(arg2), std::forward<Arg3>(arg3) } };
}

inline namespace CMT_ARCH_NAME
{

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

// ----------------------------------------------------------------------------

template <typename... Args>
using expression_pack = expression_make_function<fn::packtranspose, Args...>;

template <typename... Args, KFR_ACCEPT_EXPRESSIONS(Args...)>
KFR_INTRINSIC expression_pack<Args...> pack(Args&&... args)
{
    return { std::forward<Args>(args)... };
}

namespace internal
{
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

template <typename... Args, index_t Axis, size_t N,
          typename Tr = expression_traits<expression_function<fn::packtranspose, Args...>>>
KFR_INTRINSIC void set_elements(expression_function<fn::packtranspose, Args...>& self, shape<Tr::dims> index,
                                axis_params<Axis, N> sh, const identity<vec<typename Tr::value_type, N>>& x)
{
    internal::set_elements_packed(self, index, sh, x, csizeseq<sizeof...(Args)>);
}

// ----------------------------------------------------------------------------

template <typename... E>
struct expression_unpack : expression_with_arguments<E...>, expression_traits_defaults
{
    constexpr static size_t count = sizeof...(E);

    using first_arg_traits = typename expression_with_arguments<E...>::first_arg_traits;

    constexpr static index_t dims = first_arg_traits::dims;
    using first_value_type        = typename first_arg_traits::value_type;

    using value_type = vec<first_value_type, count>;

    static_assert(((expression_dims<E> == dims) && ...));
    static_assert(((std::is_same_v<expression_value_type<E>, first_value_type>)&&...));

    constexpr static shape<dims> get_shape(const expression_unpack& self)
    {
        return first_arg_traits::get_shape(self.first());
    }
    constexpr static shape<dims> get_shape() { return first_arg_traits::get_shape(); }

    expression_unpack(E&&... e) : expression_with_arguments<E...>(std::forward<E>(e)...) {}

    template <index_t Axis, size_t N>
    KFR_INTRINSIC friend void set_elements(expression_unpack& self, shape<dims> index,
                                           axis_params<Axis, N> sh, const identity<vec<value_type, N>>& x)
    {
        self.output(index, sh, x, csizeseq<count>);
    }

    template <typename Input, KFR_ACCEPT_EXPRESSIONS(Input)>
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

template <typename... E, enable_if_output_expressions<E...>* = nullptr>
KFR_FUNCTION expression_unpack<E...> unpack(E&&... e)
{
    return { std::forward<E>(e)... };
}

// ----------------------------------------------------------------------------

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
 * @brief Returns template expression that returns the result of calling \f$ fn(x_i, x_{i-1}) \f$
 */
template <typename Fn, typename E1>
KFR_INTRINSIC expression_adjacent<Fn, E1> adjacent(Fn&& fn, E1&& e1)
{
    return { std::forward<Fn>(fn), std::forward<E1>(e1) };
}

// ----------------------------------------------------------------------------

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
        println("[", cometa::fmt<'s', 16>(array_to_string(dims, index.data(), INT_MAX, INT_MAX, ",", "", "")),
                "] = ", in);
        return in;
    }
};

/**
 * @brief Returns template expression that prints all processed values for debug
 */
template <typename E1>
KFR_INTRINSIC expression_trace<E1> trace(E1&& e1)
{
    return { std::forward<E1>(e1) };
}

// ----------------------------------------------------------------------------

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
 * @brief Returns template expression with gien number of dimensions
 */
template <index_t Dims, typename E1>
KFR_INTRINSIC expression_dimensions<Dims, E1> dimensions(E1&& e1)
{
    static_assert(Dims >= expression_dims<E1>, "Number of dimensions must be greater or equal");
    return { std::forward<E1>(e1) };
}

// ----------------------------------------------------------------------------

} // namespace CMT_ARCH_NAME
} // namespace kfr
