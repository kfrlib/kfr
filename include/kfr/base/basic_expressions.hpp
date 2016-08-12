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

#include "../base/univector.hpp"
#include "../base/vec.hpp"

namespace kfr
{

namespace internal
{
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

template <typename E1, typename T = value_type_of<E1>>
CMT_INLINE internal::expression_iterator<T, E1> to_iterator(E1&& e1)
{
    return internal::expression_iterator<T, E1>(std::forward<E1>(e1));
}

template <typename T, typename... Ts>
CMT_INLINE auto sequence(T x, Ts... rest)
{
    const T seq[]      = { x, static_cast<T>(rest)... };
    constexpr size_t N = arraysize(seq);
    return typed<T>(lambda([=](size_t index) { return seq[index % N]; }));
}
CMT_INLINE auto zeros()
{
    return lambda([](cinput_t, size_t, auto x) { return zerovector(x); });
}
CMT_INLINE auto ones()
{
    return lambda([](cinput_t, size_t, auto x) {
        using U = subtype<decltype(x)>;
        return U(1);
    });
}
CMT_INLINE auto counter()
{
    return lambda([](cinput_t, size_t index, auto x) {
        using T    = subtype<decltype(x)>;
        using Tsub = subtype<T>;
        using TI   = subtype<itype<T>>;
        return cast<T>(enumerate<Tsub, x.size()>() + cast<Tsub>(cast<TI>(index)));
    });
}
template <typename T1>
CMT_INLINE auto counter(T1 start)
{
    return lambda([start](cinput_t, size_t index, auto x) {
        using T    = subtype<decltype(x)>;
        using Tsub = subtype<T>;
        using TI   = subtype<itype<T>>;
        return cast<T>(enumerate<Tsub, x.size()>() + cast<Tsub>(start) + cast<Tsub>(cast<TI>(index)));
    });
}
template <typename T1, typename T2>
CMT_INLINE auto counter(T1 start, T2 step)
{
    return lambda([start, step](cinput_t, size_t index, auto x) {
        using T    = subtype<decltype(x)>;
        using Tsub = subtype<T>;
        using TI   = subtype<itype<T>>;
        return cast<T>(enumerate<Tsub, x.size()>() * step + cast<Tsub>(start) + cast<Tsub>(cast<TI>(index)));
    });
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

template <typename E1, typename = void>
struct inherit_value_type
{
};

template <typename E1>
struct inherit_value_type<E1, void_t<typename decay<E1>::value_type>>
{
    using value_type = typename decay<E1>::value_type;
};

template <typename E1>
struct expression_skip : expression<E1>, inherit_value_type<E1>
{
    expression_skip(E1&& e1, size_t count) : expression<E1>(std::forward<E1>(e1)), count(count) {}
    template <typename T, size_t N>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N> y) const
    {
        return this->argument_first(index + count, y);
    }
    size_t count;
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

    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N> x) const
    {
        using UI = itype<U>;
        return U(start) + (enumerate(x) + cast<U>(cast<UI>(index))) * U(offset);
    }

    T start;
    T offset;
};

template <typename T>
struct expression_linspace<T, true> : input_expression
{
    expression_linspace(T start, T stop, size_t size, bool endpoint = false)
        : start(start), stop(stop), invsize(1.0 / T(endpoint ? size - 1 : size))
    {
    }

    expression_linspace(symmetric_linspace_t, T symsize, size_t size, bool endpoint = false)
        : expression_linspace(-symsize, +symsize, size, endpoint)
    {
    }

    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N> x) const
    {
        using UI = itype<U>;
        return mix((enumerate(x) + cast<U>(cast<UI>(index))) * invsize, cast<U>(start), cast<U>(stop));
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

    template <typename... Expr_>
    CMT_INLINE expression_sequence(const size_t (&segments)[base::size], Expr_&&... expr) noexcept
        : base(std::forward<Expr_>(expr)...)
    {
        std::copy(std::begin(segments), std::end(segments), this->segments.begin() + 1);
        this->segments[0]              = 0;
        this->segments[base::size + 1] = size_t(-1);
    }

    template <typename T, size_t N>
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
    template <typename T, size_t N>
    CMT_NOINLINE vec<T, N> get(size_t index, size_t expr_index, vec_t<T, N> y)
    {
        return cswitch(indicesfor<E...>, expr_index, [&](auto val) { return this->argument(val, index, y); },
                       [&]() { return zerovector(y); });
    }

    std::array<size_t, base::size + 2> segments;
};
}

template <typename E1>
CMT_INLINE internal::expression_skip<E1> skip(E1&& e1, size_t count = 1)
{
    return internal::expression_skip<E1>(std::forward<E1>(e1), count);
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
}
}
