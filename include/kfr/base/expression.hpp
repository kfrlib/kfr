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

constexpr size_t inout_context_size = 16;

struct coutput_context
{
    pconstvoid data[inout_context_size];
};

struct cinput_context
{
    pconstvoid data[inout_context_size];
};

using coutput_t = const coutput_context*;
using cinput_t  = const cinput_context*;

constexpr cinput_t cinput   = nullptr;
constexpr coutput_t coutput = nullptr;

constexpr size_t infinite_size = static_cast<size_t>(-1);

CMT_INTRINSIC constexpr size_t size_add(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : x + y;
}

CMT_INTRINSIC constexpr size_t size_sub(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : (x > y ? x - y : 0);
}

CMT_INTRINSIC constexpr size_t size_min(size_t x) CMT_NOEXCEPT { return x; }

template <typename... Ts>
CMT_INTRINSIC constexpr size_t size_min(size_t x, size_t y, Ts... rest) CMT_NOEXCEPT
{
    return size_min(x < y ? x : y, rest...);
}

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

/// @brief Check if the type argument is a number or a vector of numbers
template <typename T>
constexpr inline bool is_numeric = is_number<deep_subtype<T>>;

/// @brief Check if the type arguments are a numbers or a vectors of numbers
template <typename... Ts>
constexpr inline bool is_numeric_args = (is_numeric<Ts> && ...);

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

        cswitch(csize<1> << csizeseq<maxsize>, next_size, [&](auto x) {
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

namespace internal
{

template <typename T, typename = void>
struct is_infinite_impl : std::false_type
{
};

template <typename T>
struct is_infinite_impl<T, void_t<decltype(T::size())>>
    : std::integral_constant<bool, T::size() == infinite_size>
{
};
} // namespace internal

template <typename T>
constexpr inline bool is_infinite = internal::is_infinite_impl<T>::value;

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
    constexpr expression_with_arguments(Args&&... args) CMT_NOEXCEPT : args(std::forward<Args>(args)...) {}

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
CMT_INTRINSIC static size_t process(OutputExpr&& out, const InputExpr& in, size_t start = 0,
                                    size_t size = infinite_size, coutput_t coutput = nullptr,
                                    cinput_t cinput = nullptr, csize_t<groupsize> = csize_t<groupsize>())
{
    using Tin = value_type_of<InputExpr>;
    static_assert(is_output_expression<OutputExpr>, "OutFn must be an expression");
    static_assert(is_input_expression<InputExpr>, "Fn must be an expression");

    size = size_sub(size_min(out.size(), in.size(), size_add(size, start)), start);
    if (size == 0 || size == infinite_size)
        return size;
    out.begin_block(coutput, size);
    in.begin_block(cinput, size);

#ifdef NDEBUG
    constexpr size_t w = width == 0 ? maximum_vector_size<Tin> : width;
#else
    constexpr size_t w = width == 0 ? vector_width<Tin> : width;
#endif

    static_assert(w > 0 && is_poweroftwo(w), "");

    size_t i = start;

    CMT_LOOP_NOUNROLL
    for (; i < start + size / w * w; i += w)
        out(coutput, i, get_elements(in, cinput, i, vec_shape<Tin, w>()));
    CMT_LOOP_NOUNROLL
    for (; i < start + size / groupsize * groupsize; i += groupsize)
        out(coutput, i, get_elements(in, cinput, i, vec_shape<Tin, groupsize>()));

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
    KFR_MEM_INTRINSIC void operator()(coutput_t, size_t index, const vec<U, N>& value)
    {
        for (size_t i = 0; i < N; i++)
            output(index + i, static_cast<T>(value[i]));
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
