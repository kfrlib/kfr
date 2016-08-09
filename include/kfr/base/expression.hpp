/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "types.hpp"
#include "vec.hpp"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

namespace kfr
{

template <typename T>
using is_generic = is_same<generic, typename decay<T>::value_type>;

template <typename T>
using is_infinite = not_t<is_same<size_t, typename decay<T>::size_type>>;

namespace internal
{

template <typename T1>
constexpr inline T1 minsize(T1 x) noexcept
{
    return x;
}

template <typename T1, typename T2, typename... Ts>
constexpr inline common_type<T1, T2, Ts...> minsize(T1 x, T2 y, Ts... rest) noexcept
{
    return x < y ? minsize(x, rest...) : minsize(y, rest...);
}

template <typename... Args>
struct expression : input_expression
{
    using value_type = common_type<typename decay<Args>::value_type...>;

    using size_type = common_type<typename decay<Args>::size_type...>;

    constexpr size_type size() const noexcept { return size_impl(indicesfor_t<Args...>()); }

    constexpr static size_t count = sizeof...(Args);
    expression()                  = delete;
    constexpr expression(Args&&... args) noexcept : args(std::forward<Args>(args)...) {}

    CMT_INLINE void begin_block(size_t size) { begin_block_impl(size, indicesfor_t<Args...>()); }
    CMT_INLINE void end_block(size_t size) { end_block_impl(size, indicesfor_t<Args...>()); }

    CMT_INLINE void begin_block(size_t size) const { begin_block_impl(size, indicesfor_t<Args...>()); }
    CMT_INLINE void end_block(size_t size) const { end_block_impl(size, indicesfor_t<Args...>()); }

protected:
    std::tuple<Args...> args;

    template <size_t... indices>
    constexpr size_type size_impl(csizes_t<indices...>) const noexcept
    {
        return minsize(std::get<indices>(this->args).size()...);
    }

    template <typename Fn, typename T, size_t N>
    CMT_INLINE vec<T, N> call(Fn&& fn, size_t index, vec_t<T, N> x) const
    {
        return call_impl(std::forward<Fn>(fn), indicesfor_t<Args...>(), index, x);
    }
    template <size_t ArgIndex, typename T, size_t N>
    CMT_INLINE vec<T, N> argument(csize_t<ArgIndex>, size_t index, vec_t<T, N> x) const
    {
        static_assert(ArgIndex < count, "Incorrect ArgIndex");
        return std::get<ArgIndex>(this->args)(cinput, index, x);
    }
    template <typename T, size_t N>
    CMT_INLINE vec<T, N> argument_first(size_t index, vec_t<T, N> x) const
    {
        return std::get<0>(this->args)(cinput, index, x);
    }

private:
    template <typename Arg, size_t N, typename Tin,
              typename Tout = conditional<is_generic<Arg>::value, Tin, value_type_of<Arg>>>
    CMT_INLINE vec_t<Tout, N> vec_t_for() const
    {
        return {};
    }
    template <typename Fn, typename T, size_t N, size_t... indices>
    CMT_INLINE vec<T, N> call_impl(Fn&& fn, csizes_t<indices...>, size_t index, vec_t<T, N>) const
    {
        using ratio          = func_ratio<Fn>;
        constexpr size_t Nin = N * ratio::input / ratio::output;
        using Tout = conditional<is_same<generic, value_type>::value, T, common_type<T, value_type>>;

        return fn(std::get<indices>(this->args)(cinput, index * ratio::input / ratio::output,
                                                vec_t_for<Args, Nin, Tout>())...);
    }
    template <size_t... indices>
    CMT_INLINE void begin_block_impl(size_t size, csizes_t<indices...>)
    {
        swallow{ (std::get<indices>(args).begin_block(size), 0)... };
    }
    template <size_t... indices>
    CMT_INLINE void end_block_impl(size_t size, csizes_t<indices...>)
    {
        swallow{ (std::get<indices>(args).end_block(size), 0)... };
    }
    template <size_t... indices>
    CMT_INLINE void begin_block_impl(size_t size, csizes_t<indices...>) const
    {
        swallow{ (std::get<indices>(args).begin_block(size), 0)... };
    }
    template <size_t... indices>
    CMT_INLINE void end_block_impl(size_t size, csizes_t<indices...>) const
    {
        swallow{ (std::get<indices>(args).end_block(size), 0)... };
    }
};

template <typename T, size_t width = 1>
struct expression_scalar : input_expression
{
    using value_type    = T;
    expression_scalar() = delete;
    constexpr expression_scalar(const T& val) noexcept : val(val) {}
    constexpr expression_scalar(const vec<T, width>& val) noexcept : val(val) {}
    const vec<T, width> val;

    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t, vec_t<U, N>) const
    {
        return resize<N>(static_cast<vec<U, width>>(val));
    }
};

template <typename T>
using arg_impl = conditional<is_number<T>::value || is_vec<T>::value,
                             expression_scalar<subtype<decay<T>>, compound_type_traits<decay<T>>::width>, T>;

template <typename T>
using arg = internal::arg_impl<T>;

template <typename Fn, typename Args, typename Enable = void>
struct generic_result
{
    using type = generic;
};

template <typename Fn, typename... Args>
struct generic_result<Fn, ctypes_t<Args...>, void_t<enable_if<!or_t<is_same<generic, Args>...>::value>>>
{
    using type = subtype<decltype(std::declval<Fn>()(std::declval<vec<decay<Args>, 1>>()...))>;
};

template <typename Fn, typename... Args>
struct expression_function : expression<arg<Args>...>
{
    using ratio = func_ratio<Fn>;

    using value_type = typename generic_result<Fn, ctypes_t<value_type_of<arg<Args>>...>>::type;

    expression_function(Fn&& fn, arg<Args>&&... args) noexcept
        : expression<arg<Args>...>(std::forward<arg<Args>>(args)...),
          fn(std::forward<Fn>(fn))
    {
    }
    template <typename T, size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> x) const
    {
        static_assert(is_same<T, value_type_of<expression_function>>::value ||
                          is_generic<expression_function>::value,
                      "Can't cast from value_type to T");
        return this->call(fn, index, x);
    }

protected:
    Fn fn;
};

template <typename Tout, typename Tin, size_t width, typename OutFn, typename Fn>
CMT_INLINE void process_cycle(OutFn&& outfn, const Fn& fn, size_t& i, size_t size)
{
    const size_t count = size / width * width;
    CMT_LOOP_NOUNROLL
    for (; i < count; i += width)
    {
        outfn(coutput, i, fn(cinput, i, vec_t<Tin, width>()));
    }
}
}

template <typename A>
CMT_INLINE internal::arg<A> e(A&& a)
{
    return internal::arg<A>(std::forward<A>(a));
}

template <typename T>
CMT_INLINE internal::expression_scalar<T> scalar(const T& val)
{
    return internal::expression_scalar<T>(val);
}

template <typename T, size_t N>
CMT_INLINE internal::expression_scalar<T, N> scalar(const vec<T, N>& val)
{
    return internal::expression_scalar<T, N>(val);
}

template <typename Fn, typename... Args>
CMT_INLINE internal::expression_function<decay<Fn>, internal::arg<Args>...> bind_expression(Fn&& fn,
                                                                                            Args&&... args)
{
    return internal::expression_function<decay<Fn>, internal::arg<Args>...>(std::forward<Fn>(fn),
                                                                            std::forward<Args>(args)...);
}

template <typename Tout, cpu_t c = cpu_t::native, size_t width = 0, typename OutFn, typename Fn>
CMT_INLINE void process(OutFn&& outfn, const Fn& fn, size_t size)
{
    static_assert(is_output_expression<OutFn>::value, "OutFn must be an expression");
    static_assert(is_input_expression<Fn>::value, "Fn must be an expression");
    constexpr size_t comp = lcm(func_ratio<OutFn>::input, func_ratio<Fn>::output);
    size *= comp;
    outfn.output_begin_block(size);
    fn.begin_block(size);

    using Tin = conditional<is_generic<Fn>::value, Tout, value_type_of<Fn>>;

    size_t i = 0;
    internal::process_cycle<Tout, Tin, width == 0 ? internal::get_vector_width<Tout, c>(2, 4) : width>(
        std::forward<OutFn>(outfn), fn, i, size);
    internal::process_cycle<Tout, Tin, comp>(std::forward<OutFn>(outfn), fn, i, size);

    fn.end_block(size);
    outfn.output_end_block(size);
}

namespace internal
{

template <typename T, typename E1>
struct expressoin_typed : input_expression
{
    using value_type = T;

    expressoin_typed(E1&& e1) : e1(std::forward<E1>(e1)) {}

    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N>) const
    {
        return e1(cinput, index, vec_t<T, N>());
    }
    E1 e1;
};

template <typename T, typename E1>
struct expressoin_sized : input_expression
{
    using value_type = T;
    using size_type  = size_t;

    expressoin_sized(E1&& e1, size_t size) : e1(std::forward<E1>(e1)), m_size(size) {}

    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N>) const
    {
        auto val = e1(cinput, index, vec_t<T, N>());
        return val;
    }

    constexpr size_t size() const noexcept { return m_size; }
    E1 e1;
    size_t m_size;
};
}

template <typename T, typename E1>
inline internal::expressoin_typed<T, E1> typed(E1&& e1)
{
    return internal::expressoin_typed<T, E1>(std::forward<E1>(e1));
}
template <typename T, typename E1>
inline internal::expressoin_sized<T, E1> typed(E1&& e1, size_t size)
{
    return internal::expressoin_sized<T, E1>(std::forward<E1>(e1), size);
}
}
#pragma clang diagnostic pop
