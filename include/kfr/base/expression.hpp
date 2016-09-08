/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "types.hpp"
#include "vec.hpp"
#include "platform.hpp"

#include <tuple>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

namespace kfr
{

namespace internal
{
template <typename T, typename = void>
struct is_fixed_size_impl : std::false_type
{
};

template <typename T>
struct is_fixed_size_impl<T, void_t<decltype(T::size())>> : std::true_type
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
}

template <typename T>
using is_fixed_size = typename internal::is_fixed_size_impl<T>::type;

template <typename T>
using is_infinite = typename internal::is_infinite_impl<T>::type;

namespace internal
{

template <typename... Args>
struct expression : input_expression
{
    using value_type = common_type<typename decay<Args>::value_type...>;

    constexpr size_t size() const noexcept { return size_impl(indicesfor_t<Args...>()); }

    constexpr static size_t count = sizeof...(Args);
    expression()                  = delete;
    constexpr expression(Args&&... args) noexcept : args(std::forward<Args>(args)...) {}

    CMT_INLINE void begin_block(size_t size) { begin_block_impl(size, indicesfor_t<Args...>()); }
    CMT_INLINE void end_block(size_t size) { end_block_impl(size, indicesfor_t<Args...>()); }

    CMT_INLINE void begin_block(size_t size) const { begin_block_impl(size, indicesfor_t<Args...>()); }
    CMT_INLINE void end_block(size_t size) const { end_block_impl(size, indicesfor_t<Args...>()); }

    std::tuple<Args...> args;

protected:
    template <size_t... indices>
    constexpr size_t size_impl(csizes_t<indices...>) const noexcept
    {
        return size_min(std::get<indices>(this->args).size()...);
    }

    template <typename Fn, typename T, size_t N>
    CMT_INLINE vec<T, N> call(Fn&& fn, size_t index, vec_t<T, N> x) const
    {
        return call_impl(std::forward<Fn>(fn), indicesfor_t<Args...>(), index, x);
    }
    template <size_t ArgIndex, typename U, size_t N,
              typename T = value_type_of<typename details::get_nth_type<ArgIndex, Args...>::type>>
    CMT_INLINE vec<U, N> argument(csize_t<ArgIndex>, size_t index, vec_t<U, N>) const
    {
        static_assert(ArgIndex < count, "Incorrect ArgIndex");
        return static_cast<vec<U, N>>(std::get<ArgIndex>(this->args)(cinput, index, vec_t<T, N>()));
    }
    template <typename U, size_t N,
              typename T = value_type_of<typename details::get_nth_type<0, Args...>::type>>
    CMT_INLINE vec<U, N> argument_first(size_t index, vec_t<U, N>) const
    {
        return static_cast<vec<U, N>>(std::get<0>(this->args)(cinput, index, vec_t<T, N>()));
    }

private:
    template <typename Arg, size_t N, typename Tout = value_type_of<Arg>>
    CMT_INLINE vec_t<Tout, N> vec_t_for() const
    {
        return {};
    }
    template <typename Fn, typename T, size_t N, size_t... indices>
    CMT_INLINE vec<T, N> call_impl(Fn&& fn, csizes_t<indices...>, size_t index, vec_t<T, N>) const
    {
        return fn(std::get<indices>(this->args)(cinput, index, vec_t_for<Args, N>())...);
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
    vec<T, width> val;

    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t, vec_t<T, N>) const
    {
        return resize<N>(val);
    }
};

template <typename T>
using arg_impl = conditional<is_number<T>::value || is_vec<T>::value,
                             expression_scalar<subtype<decay<T>>, compound_type_traits<decay<T>>::width>, T>;

template <typename T>
using arg = internal::arg_impl<T>;

template <typename Fn, typename... Args>
struct expression_function : expression<arg<Args>...>
{
    using value_type =
        subtype<decltype(std::declval<Fn>()(std::declval<vec<value_type_of<arg<Args>>, 1>>()...))>;
    using T = value_type;

    expression_function(Fn&& fn, arg<Args>&&... args) noexcept
        : expression<arg<Args>...>(std::forward<arg<Args>>(args)...),
          fn(std::forward<Fn>(fn))
    {
    }
    expression_function(const Fn& fn, arg<Args>&&... args) noexcept
        : expression<arg<Args>...>(std::forward<arg<Args>>(args)...),
          fn(fn)
    {
    }
    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> x) const
    {
        return this->call(fn, index, x);
    }

    const Fn& get_fn() const noexcept { return fn; }

protected:
    Fn fn;
};
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
/**
 * @brief Construct a new expression using the same function as in @c e and new arguments
 * @param e an expression
 * @param args new arguments for the function
 */
template <typename Fn, typename... OldArgs, typename... NewArgs>
CMT_INLINE internal::expression_function<Fn, NewArgs...> rebind(
    const internal::expression_function<Fn, OldArgs...>& e, NewArgs&&... args)
{
    return internal::expression_function<Fn, NewArgs...>(e.get_fn(), std::forward<NewArgs>(args)...);
}

namespace internal
{
template <size_t width, typename OutputExpr, typename InputExpr>
CMT_INLINE void process_cycle(OutputExpr&& outfn, const InputExpr& fn, size_t& i, size_t size)
{
    using Tin          = value_type_of<InputExpr>;
    const size_t count = size / width * width;
    CMT_LOOP_NOUNROLL
    for (; i < count; i += width)
    {
        outfn(coutput, i, fn(cinput, i, vec_t<Tin, width>()));
    }
}
}

template <typename Tout, cpu_t c = cpu_t::native, size_t width = 0, typename OutputExpr, typename InputExpr,
          size_t groupsize = 1>
CMT_INLINE size_t process(OutputExpr&& out, const InputExpr& in, csize_t<groupsize> = csize_t<groupsize>())
{
    const size_t size = size_min(out.size(), in.size()) * groupsize;
    static_assert(is_output_expression<OutputExpr>::value, "OutFn must be an expression");
    static_assert(is_input_expression<InputExpr>::value, "Fn must be an expression");
    out.output_begin_block(size);
    in.begin_block(size);

#ifdef NDEBUG
    constexpr size_t w = width == 0 ? internal::get_vector_width<Tout, c>(2, 4) : width;
#else
    constexpr size_t w = width == 0 ? internal::get_vector_width<Tout, c>(1, 1) : width;
#endif

    size_t i = 0;
    internal::process_cycle<w>(std::forward<OutputExpr>(out), in, i, size);
    internal::process_cycle<groupsize>(std::forward<OutputExpr>(out), in, i, size);

    in.end_block(size);
    out.output_end_block(size);
    return size;
}

template <typename T>
struct input_expression_base : input_expression
{
    virtual ~input_expression_base() {}
    virtual T input(size_t index) const = 0;
    template <typename U, size_t N>
    vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N>) const
    {
        vec<U, N> out;
        for (size_t i = 0; i < N; i++)
            out(i)    = static_cast<U>(input(index + i));
        return out;
    }
};

template <typename T>
struct output_expression_base : output_expression
{
    virtual ~output_expression_base() {}
    virtual void output(size_t index, const T& value) = 0;

    template <typename U, size_t N>
    void operator()(coutput_t, size_t index, const vec<U, N>& value)
    {
        for (size_t i = 0; i < N; i++)
            output(index + i, static_cast<T>(value[i]));
    }
};
}
#pragma clang diagnostic pop
