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

#include "univector.hpp"
#include "vec.hpp"
#include <algorithm>

namespace kfr
{

namespace internal
{

template <typename To, typename E>
struct expression_convert : expression<E>
{
    using value_type = To;
    CMT_INLINE expression_convert(E&& expr) noexcept : expression<E>(std::forward<E>(expr)) {}

    template <size_t N>
    CMT_INLINE vec<To, N> operator()(cinput_t, size_t index, vec_t<To, N>) const
    {
        return this->argument_first(index, vec_t<To, N>());
    }
};

template <typename T, typename E1>
struct expression_iterator
{
    constexpr expression_iterator(E1&& e1) : e1(std::forward<E1>(e1)) {}
    struct iterator
    {
        T operator*() const { return get(); }
        T get() const { return expr.e1(cinput, position, vec_t<T, 1>())[0]; }
        iterator& operator++()
        {
            ++position;
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy = *this;
            ++(*this);
            return copy;
        }
        bool operator!=(const iterator& other) const { return position != other.position; }
        const expression_iterator& expr;
        size_t position;
    };
    iterator begin() const { return { *this, 0 }; }
    iterator end() const { return { *this, e1.size() }; }
    E1 e1;
};
}

template <typename To, typename E>
CMT_INLINE internal::expression_convert<To, E> convert(E&& expr)
{
    return internal::expression_convert<To, E>(std::forward<E>(expr));
}

template <typename E1, typename T = value_type_of<E1>>
CMT_INLINE internal::expression_iterator<T, E1> to_iterator(E1&& e1)
{
    return internal::expression_iterator<T, E1>(std::forward<E1>(e1));
}

template <typename... Ts, typename T = common_type<Ts...>>
inline auto sequence(const Ts&... list)
{
    return lambda<T>([seq = std::array<T, sizeof...(Ts)>{ static_cast<T>(list)... }](size_t index) {
        return seq[index % seq.size()];
    });
}

template <typename T = int>
CMT_INLINE auto zeros()
{
    return lambda<T>([](cinput_t, size_t, auto x) { return zerovector(x); });
}

template <typename T = int>
CMT_INLINE auto ones()
{
    return lambda<T>([](cinput_t, size_t, auto x) { return 1; });
}

template <typename T = int>
CMT_INLINE auto counter()
{
    return lambda<T>([](cinput_t, size_t index, auto x) { return enumerate(x) + index; });
}

template <typename T1>
CMT_INLINE auto counter(T1 start)
{
    return lambda<T1>([start](cinput_t, size_t index, auto x) { return enumerate(x) + index + start; });
}
template <typename T1, typename T2>
CMT_INLINE auto counter(T1 start, T2 step)
{
    return lambda<common_type<T1, T2>>(
        [start, step](cinput_t, size_t index, auto x) { return (enumerate(x) + index) * step + start; });
}

template <typename Gen>
struct segment
{
    template <typename Gen_>
    constexpr segment(size_t start, Gen_&& gen) : start(start), gen(std::forward<Gen_>(gen))
    {
    }
    size_t start;
    Gen gen;
};

enum symmetric_linspace_t
{
    symmetric_linspace
};

namespace internal
{
template <typename T, typename E1>
struct expression_reader
{
    constexpr expression_reader(E1&& e1) noexcept : e1(std::forward<E1>(e1)) {}
    T read() const
    {
        const T result = e1(cinput, m_position, vec_t<T, 1>());
        m_position++;
        return result;
    }
    mutable size_t m_position = 0;
    E1 e1;
};
template <typename T, typename E1>
struct expression_writer
{
    constexpr expression_writer(E1&& e1) noexcept : e1(std::forward<E1>(e1)) {}
    template <typename U>
    void write(U value)
    {
        e1(coutput, m_position, vec<U, 1>(value));
        m_position++;
    }
    size_t m_position = 0;
    E1 e1;
};
}

template <typename T, typename E1>
internal::expression_reader<T, E1> reader(E1&& e1)
{
    static_assert(is_input_expression<E1>::value, "E1 must be an expression");
    return internal::expression_reader<T, E1>(std::forward<E1>(e1));
}

template <typename T, typename E1>
internal::expression_writer<T, E1> writer(E1&& e1)
{
    static_assert(is_output_expression<E1>::value, "E1 must be an output expression");
    return internal::expression_writer<T, E1>(std::forward<E1>(e1));
}

namespace internal
{

template <typename E1>
struct expression_slice : expression<E1>
{
    using value_type = value_type_of<E1>;
    using T          = value_type;
    expression_slice(E1&& e1, size_t start, size_t size)
        : expression<E1>(std::forward<E1>(e1)), start(start),
          new_size(size_min(size, size_sub(std::get<0>(this->args).size(), start)))
    {
    }
    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> y) const
    {
        return this->argument_first(index + start, y);
    }
    size_t size() const { return new_size; }
    size_t start;
    size_t new_size;
};

template <typename E1>
struct expression_reverse : expression<E1>
{
    using value_type = value_type_of<E1>;
    using T          = value_type;
    expression_reverse(E1&& e1) : expression<E1>(std::forward<E1>(e1)), expr_size(e1.size()) {}
    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> y) const
    {
        return reverse(this->argument_first(expr_size - index - N, y));
    }
    size_t size() const { return expr_size; }
    size_t expr_size;
};

template <typename T, bool precise = false>
struct expression_linspace;

template <typename T>
struct expression_linspace<T, false> : input_expression
{
    using value_type = T;

    expression_linspace(T start, T stop, size_t size, bool endpoint = false)
        : start(start), offset((stop - start) / T(endpoint ? size - 1 : size))
    {
    }

    expression_linspace(symmetric_linspace_t, T symsize, size_t size, bool endpoint = false)
        : expression_linspace(-symsize, +symsize, size, endpoint)
    {
    }

    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> x) const
    {
        using TI = itype<T>;
        return T(start) + (enumerate(x) + cast<T>(cast<TI>(index))) * T(offset);
    }

    T start;
    T offset;
};

template <typename T>
struct expression_linspace<T, true> : input_expression
{
    using value_type = T;

    expression_linspace(T start, T stop, size_t size, bool endpoint = false)
        : start(start), stop(stop), invsize(1.0 / T(endpoint ? size - 1 : size))
    {
    }

    expression_linspace(symmetric_linspace_t, T symsize, size_t size, bool endpoint = false)
        : expression_linspace(-symsize, +symsize, size, endpoint)
    {
    }

    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> x) const
    {
        using TI = itype<T>;
        return mix((enumerate(x) + cast<T>(cast<TI>(index))) * invsize, cast<T>(start), cast<T>(stop));
    }
    template <typename U, size_t N>
    CMT_INLINE static vec<U, N> mix(vec<U, N> t, U x, U y)
    {
        return (U(1.0) - t) * x + t * y;
    }

    T start;
    T stop;
    T invsize;
};

template <typename... E>
struct expression_sequence : expression<E...>
{
public:
    using base = expression<E...>;

    using value_type = common_type<value_type_of<E>...>;
    using T          = value_type;

    template <typename... Expr_>
    CMT_INLINE expression_sequence(const size_t (&segments)[base::size], Expr_&&... expr) noexcept
        : base(std::forward<Expr_>(expr)...)
    {
        std::copy(std::begin(segments), std::end(segments), this->segments.begin() + 1);
        this->segments[0]              = 0;
        this->segments[base::size + 1] = size_t(-1);
    }

    template <size_t N>
    CMT_NOINLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> y) const
    {
        std::size_t sindex = size_t(std::upper_bound(std::begin(segments), std::end(segments), index) - 1 -
                                    std::begin(segments));
        if (segments[sindex + 1] - index >= N)
            return get(index, sindex - 1, y);
        else
        {
            vec<T, N> result;
#pragma clang loop unroll_count(4)
            for (size_t i = 0; i < N; i++)
            {
                sindex           = segments[sindex + 1] == index ? sindex + 1 : sindex;
                result.data()[i] = get(index, sindex - 1, vec_t<T, 1>())[0];
                index++;
            }
            return result;
        }
    }

protected:
    template <size_t N>
    CMT_NOINLINE vec<T, N> get(size_t index, size_t expr_index, vec_t<T, N> y)
    {
        return cswitch(indicesfor<E...>, expr_index, [&](auto val) { return this->argument(val, index, y); },
                       [&]() { return zerovector(y); });
    }

    std::array<size_t, base::size + 2> segments;
};

template <typename Fn, typename E>
struct expression_adjacent : expression<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;

    expression_adjacent(Fn&& fn, E&& e) : expression<E>(std::forward<E>(e)), fn(std::forward<Fn>(fn)) {}

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        const vec<T, N> in      = this->argument_first(index, vec_t<T, N>());
        const vec<T, N> delayed = insertleft(data, in);
        data = in[N - 1];
        return this->fn(in, delayed);
    }
    Fn fn;
    mutable value_type data = value_type(0);
};
}

template <typename E1>
CMT_INLINE internal::expression_slice<E1> slice(E1&& e1, size_t start, size_t size = infinite_size)
{
    return internal::expression_slice<E1>(std::forward<E1>(e1), start, size);
}

template <typename E1>
CMT_INLINE internal::expression_slice<E1> truncate(E1&& e1, size_t size)
{
    return internal::expression_slice<E1>(std::forward<E1>(e1), 0, size);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
CMT_INLINE internal::expression_reverse<E1> reverse(E1&& e1)
{
    static_assert(!is_infinite<E1>::value, "e1 must be a sized expression (use slice())");
    return internal::expression_reverse<E1>(std::forward<E1>(e1));
}

template <typename T1, typename T2, bool precise = false, typename TF = ftype<common_type<T1, T2>>>
CMT_INLINE internal::expression_linspace<TF, precise> linspace(T1 start, T2 stop, size_t size,
                                                               bool endpoint = false)
{
    return internal::expression_linspace<TF, precise>(start, stop, size, endpoint);
}
KFR_FN(linspace)

template <typename T, bool precise = false, typename TF = ftype<T>>
CMT_INLINE internal::expression_linspace<TF, precise> symmlinspace(T symsize, size_t size,
                                                                   bool endpoint = false)
{
    return internal::expression_linspace<TF, precise>(symmetric_linspace, symsize, size, endpoint);
}
KFR_FN(symmlinspace)

template <size_t size, typename... E>
CMT_INLINE internal::expression_sequence<decay<E>...> gen_sequence(const size_t (&list)[size], E&&... gens)
{
    static_assert(size == sizeof...(E), "Lists must be of equal length");
    return internal::expression_sequence<decay<E>...>(list, std::forward<E>(gens)...);
}
KFR_FN(gen_sequence)

/**
 * @brief Returns template expression that returns the result of calling \f$ fn(x_i, x_{i-1}) \f$
 */
template <typename Fn, typename E1>
CMT_INLINE internal::expression_adjacent<Fn, E1> adjacent(Fn&& fn, E1&& e1)
{
    return internal::expression_adjacent<Fn, E1>(std::forward<Fn>(fn), std::forward<E1>(e1));
}

namespace internal
{
template <typename... E>
struct multioutput : output_expression
{
    template <typename... E_>
    multioutput(E_&&... e) : outputs(std::forward<E_>(e)...)
    {
    }
    template <typename T, size_t N>
    void operator()(coutput_t, size_t index, const vec<T, N>& x)
    {
        cfor(csize<0>, csize<sizeof...(E)>,
             [&](auto n) { std::get<val_of(decltype(n)())>(outputs)(coutput, index, x); });
    }
    std::tuple<E...> outputs;

private:
};

template <typename... E>
struct expression_pack : expression<E...>, output_expression
{
    constexpr static size_t count = sizeof...(E);

    expression_pack(E&&... e) : expression<E...>(std::forward<E>(e)...) {}
    using value_type = vec<common_type<value_type_of<E>...>, count>;
    using T          = value_type;

    using expression<E...>::size;

    template <size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> y) const
    {
        return this->call(fn::packtranspose(), index, y);
    }
};

template <typename... E>
struct expression_unpack : private expression<E...>, output_expression
{
    constexpr static size_t count = sizeof...(E);

    expression_unpack(E&&... e) : expression<E...>(std::forward<E>(e)...) {}

    using expression<E...>::size;

    template <typename U, size_t N>
    CMT_INLINE void operator()(coutput_t, size_t index, const vec<vec<U, count>, N>& x)
    {
        output(index, x, csizeseq<count>);
    }

    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>::value)>
    CMT_INLINE expression_unpack& operator=(Input&& input)
    {
        using value_type = vec<common_type<value_type_of<E>...>, count>;
        process<value_type>(*this, std::forward<Input>(input));
        return *this;
    }

private:
    template <typename U, size_t N, size_t... indices>
    void output(size_t index, const vec<vec<U, count>, N>& x, csizes_t<indices...>)
    {
        const vec<vec<U, N>, count> xx = compcast<vec<U, N>>(transpose<count>(flatten(x)));
        swallow{ (std::get<indices>(this->args)(coutput, index, xx[indices]), void(), 0)... };
    }
};
}

template <typename... E, KFR_ENABLE_IF(is_output_expressions<E...>::value)>
internal::expression_unpack<internal::arg<E>...> unpack(E&&... e)
{
    return internal::expression_unpack<internal::arg<E>...>(std::forward<E>(e)...);
}

template <typename... E, KFR_ENABLE_IF(is_input_expressions<E...>::value)>
internal::expression_pack<internal::arg<E>...> pack(E&&... e)
{
    return internal::expression_pack<internal::arg<E>...>(std::forward<E>(e)...);
}

template <typename OutExpr, typename InExpr>
struct task_partition
{
    task_partition(OutExpr&& output, InExpr&& input, size_t size, size_t chunk_size, size_t count)
        : output(std::forward<OutExpr>(output)), input(std::forward<InExpr>(input)), size(size),
          chunk_size(chunk_size), count(count)
    {
    }
    OutExpr output;
    InExpr input;
    size_t size;
    size_t chunk_size;
    size_t count;
    size_t operator()(size_t index)
    {
        if (index > count)
            return 0;
        using T = value_type_of<InExpr>;
        return process<T>(output, input, index * chunk_size, chunk_size);
    }
};

template <typename OutExpr, typename InExpr, typename T = value_type_of<InExpr>>
task_partition<OutExpr, InExpr> partition(OutExpr&& output, InExpr&& input, size_t count,
                                          size_t minimum_size = 0)
{
    static_assert(!is_infinite<OutExpr>::value || !is_infinite<InExpr>::value, "");

    minimum_size            = minimum_size == 0 ? vector_width<T> * 8 : minimum_size;
    const size_t size       = size_min(output.size(), input.size());
    const size_t chunk_size = align_up(std::max(size / count, minimum_size), vector_width<T>);

    task_partition<OutExpr, InExpr> result(std::forward<OutExpr>(output), std::forward<InExpr>(input), size,
                                           chunk_size, (size + chunk_size - 1) / chunk_size);
    return result;
}
}
