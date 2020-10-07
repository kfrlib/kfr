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

#include "../simd/operators.hpp"
#include "../simd/vec.hpp"
#include "univector.hpp"
#include <algorithm>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace internal
{
template <size_t width, typename Fn>
KFR_INTRINSIC void block_process_impl(size_t& i, size_t size, Fn&& fn)
{
    CMT_LOOP_NOUNROLL
    for (; i < size / width * width; i += width)
        fn(i, csize_t<width>());
}
} // namespace internal

template <size_t... widths, typename Fn>
KFR_INTRINSIC void block_process(size_t size, csizes_t<widths...>, Fn&& fn)
{
    size_t i = 0;
    swallow{ (internal::block_process_impl<widths>(i, size, std::forward<Fn>(fn)), 0)... };
}

namespace internal
{

template <typename To, typename E>
struct expression_cast : expression_with_arguments<E>
{
    using value_type = To;
    KFR_MEM_INTRINSIC expression_cast(E&& expr) CMT_NOEXCEPT
        : expression_with_arguments<E>(std::forward<E>(expr))
    {
    }

    template <size_t N>
    friend KFR_INTRINSIC vec<To, N> get_elements(const expression_cast& self, cinput_t input, size_t index,
                                                 vec_shape<To, N>)
    {
        return self.argument_first(input, index, vec_shape<To, N>());
    }
};

template <typename T, typename E1>
struct expression_iterator
{
    constexpr expression_iterator(E1&& e1) : e1(std::forward<E1>(e1)) {}
    struct iterator
    {
        T operator*() const { return get(); }
        T get() const { return get_elements(expr.e1, cinput, position, vec_shape<T, 1>()).front(); }
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
} // namespace internal

template <typename To, typename E, KFR_ENABLE_IF(is_input_expression<E>)>
KFR_INTRINSIC internal::expression_cast<To, E> cast(E&& expr)
{
    return internal::expression_cast<To, E>(std::forward<E>(expr));
}

template <typename E1, typename T = value_type_of<E1>>
KFR_INTRINSIC internal::expression_iterator<T, E1> to_iterator(E1&& e1)
{
    return internal::expression_iterator<T, E1>(std::forward<E1>(e1));
}

template <typename... Ts, typename T = common_type<Ts...>>
inline auto sequence(const Ts&... list)
{
    return lambda<T>([seq = std::array<T, sizeof...(Ts)>{ { static_cast<T>(list)... } }](size_t index) {
        return seq[index % seq.size()];
    });
}

template <typename T = int>
KFR_INTRINSIC auto zeros()
{
    return lambda<T>([](cinput_t, size_t, auto x) { return zerovector(x); });
}

template <typename T = int>
KFR_INTRINSIC auto ones()
{
    return lambda<T>([](cinput_t, size_t, auto) { return 1; });
}

template <typename T = int>
KFR_INTRINSIC auto counter()
{
    return lambda<T>([](cinput_t, size_t index, auto x) { return enumerate(x) + index; });
}

template <typename T1>
KFR_INTRINSIC auto counter(T1 start)
{
    return lambda<T1>([start](cinput_t, size_t index, auto x) { return enumerate(x) + index + start; });
}
template <typename T1, typename T2>
KFR_INTRINSIC auto counter(T1 start, T2 step)
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
    constexpr expression_reader(E1&& e1) CMT_NOEXCEPT : e1(std::forward<E1>(e1)) {}
    T read() const
    {
        const T result = get_elements(e1, cinput, m_position, vec_shape<T, 1>());
        m_position++;
        return result;
    }
    mutable size_t m_position = 0;
    E1 e1;
};
template <typename T, typename E1>
struct expression_writer
{
    constexpr expression_writer(E1&& e1) CMT_NOEXCEPT : e1(std::forward<E1>(e1)) {}
    template <typename U>
    void write(U value)
    {
        e1(coutput, m_position, vec<U, 1>(value));
        m_position++;
    }
    size_t m_position = 0;
    E1 e1;
};
} // namespace internal

template <typename T, typename E1>
internal::expression_reader<T, E1> reader(E1&& e1)
{
    static_assert(is_input_expression<E1>, "E1 must be an expression");
    return internal::expression_reader<T, E1>(std::forward<E1>(e1));
}

template <typename T, typename E1>
internal::expression_writer<T, E1> writer(E1&& e1)
{
    static_assert(is_output_expression<E1>, "E1 must be an output expression");
    return internal::expression_writer<T, E1>(std::forward<E1>(e1));
}

namespace internal
{

template <typename E1>
struct expression_slice : expression_with_arguments<E1>
{
    using value_type = value_type_of<E1>;
    using T          = value_type;
    expression_slice(E1&& e1, size_t start, size_t size)
        : expression_with_arguments<E1>(std::forward<E1>(e1)), start(start),
          new_size(size_min(size, size_sub(std::get<0>(this->args).size(), start)))
    {
    }
    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_slice& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return self.argument_first(cinput, index + self.start, y);
    }
    size_t size() const { return new_size; }
    size_t start;
    size_t new_size;
};

template <typename E1>
struct expression_reverse : expression_with_arguments<E1>
{
    using value_type = value_type_of<E1>;
    using T          = value_type;
    expression_reverse(E1&& e1) : expression_with_arguments<E1>(std::forward<E1>(e1)), expr_size(e1.size()) {}
    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_reverse& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return reverse(self.argument_first(cinput, self.expr_size - index - N, y));
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

    KFR_MEM_INTRINSIC constexpr size_t size() const CMT_NOEXCEPT { return truncate_size; }

    expression_linspace(T start, T stop, size_t size, bool endpoint = false, bool truncate = false)
        : start(start), offset((stop - start) / T(endpoint ? size - 1 : size)),
          truncate_size(truncate ? size : infinite_size)
    {
    }

    expression_linspace(symmetric_linspace_t, T symsize, size_t size, bool endpoint = false)
        : expression_linspace(-symsize, +symsize, size, endpoint)
    {
    }

    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_linspace& self, cinput_t, size_t index,
                                                vec_shape<T, N> x)
    {
        using TI = itype<T>;
        return T(self.start) + (enumerate(x) + static_cast<T>(static_cast<TI>(index))) * T(self.offset);
    }

    T start;
    T offset;
    size_t truncate_size;
};

template <typename T>
struct expression_linspace<T, true> : input_expression
{
    using value_type = T;

    KFR_MEM_INTRINSIC constexpr size_t size() const CMT_NOEXCEPT { return truncate_size; }

    expression_linspace(T start, T stop, size_t size, bool endpoint = false, bool truncate = false)
        : start(start), stop(stop), invsize(1.0 / T(endpoint ? size - 1 : size)),
          truncate_size(truncate ? size : infinite_size)
    {
    }

    expression_linspace(symmetric_linspace_t, T symsize, size_t size, bool endpoint = false)
        : expression_linspace(-symsize, +symsize, size, endpoint)
    {
    }

    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_linspace& self, cinput_t, size_t index,
                                                vec_shape<T, N> x)
    {
        using TI = itype<T>;
        return mix((enumerate(x) + static_cast<T>(static_cast<TI>(index))) * self.invsize, self.start,
                   self.stop);
    }
    template <typename U, size_t N>
    KFR_MEM_INTRINSIC static vec<U, N> mix(const vec<U, N>& t, U x, U y)
    {
        return (U(1.0) - t) * x + t * y;
    }

    T start;
    T stop;
    T invsize;
    size_t truncate_size;
};

template <typename... E>
struct expression_sequence : expression_with_arguments<E...>
{
public:
    using base = expression_with_arguments<E...>;

    using value_type = common_type<value_type_of<E>...>;
    using T          = value_type;

    template <typename... Expr_>
    KFR_MEM_INTRINSIC expression_sequence(const size_t (&segments)[base::count], Expr_&&... expr) CMT_NOEXCEPT
        : base(std::forward<Expr_>(expr)...)
    {
        std::copy(std::begin(segments), std::end(segments), this->segments.begin() + 1);
        this->segments[0]               = 0;
        this->segments[base::count + 1] = size_t(-1);
    }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_sequence& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        std::size_t sindex =
            size_t(std::upper_bound(std::begin(self.segments), std::end(self.segments), index) - 1 -
                   std::begin(self.segments));
        if (self.segments[sindex + 1] - index >= N)
            return get_elements(self, cinput, index, sindex - 1, y);
        else
        {
            vec<T, N> result;
            CMT_PRAGMA_CLANG(clang loop unroll_count(4))
            for (size_t i = 0; i < N; i++)
            {
                sindex           = self.segments[sindex + 1] == index ? sindex + 1 : sindex;
                result.data()[i] = get_elements(self, cinput, index, sindex - 1, vec_shape<T, 1>()).front();
                index++;
            }
            return result;
        }
    }

protected:
    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_sequence& self, cinput_t cinput,
                                                size_t index, size_t expr_index, vec_shape<T, N> y)
    {
        return cswitch(
            indicesfor_t<E...>(), expr_index, [&](auto val) { return self.argument(cinput, val, index, y); },
            [&]() { return zerovector(y); });
    }

    std::array<size_t, base::count + 2> segments;
};

template <typename Fn, typename E>
struct expression_adjacent : expression_with_arguments<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;

    expression_adjacent(Fn&& fn, E&& e)
        : expression_with_arguments<E>(std::forward<E>(e)), fn(std::forward<Fn>(fn))
    {
    }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_adjacent& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N>)
    {
        const vec<T, N> in      = self.argument_first(cinput, index, vec_shape<T, N>());
        const vec<T, N> delayed = insertleft(self.data, in);
        self.data               = in[N - 1];
        return self.fn(in, delayed);
    }
    Fn fn;
    mutable value_type data = value_type(0);
};
} // namespace internal

/** @brief Returns the subrange of the given expression
 */
template <typename E1>
KFR_INTRINSIC internal::expression_slice<E1> slice(E1&& e1, size_t start, size_t size = infinite_size)
{
    return internal::expression_slice<E1>(std::forward<E1>(e1), start, size);
}

/** @brief Returns the expression truncated to the given size
 */
template <typename E1>
KFR_INTRINSIC internal::expression_slice<E1> truncate(E1&& e1, size_t size)
{
    return internal::expression_slice<E1>(std::forward<E1>(e1), 0, size);
}

/** @brief Returns the reversed expression
 */
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>)>
KFR_INTRINSIC internal::expression_reverse<E1> reverse(E1&& e1)
{
    static_assert(!is_infinite<E1>, "e1 must be a sized expression (use slice())");
    return internal::expression_reverse<E1>(std::forward<E1>(e1));
}

/** @brief Returns evenly spaced numbers over a specified interval.
 *
 * @param start The starting value of the sequence
 * @param stop The end value of the sequence. if ``endpoint`` is ``false``, the last value is excluded
 * @param size Number of samples to generate
 * @param endpoint If ``true``, ``stop`` is the last sample. Otherwise, it is not included
 * @param truncate If ``true``, linspace returns exactly size elements, otherwise, returns infinite sequence
 */
template <typename T1, typename T2, bool precise = false, typename TF = ftype<common_type<T1, T2>>>
KFR_INTRINSIC internal::expression_linspace<TF, precise> linspace(T1 start, T2 stop, size_t size,
                                                                  bool endpoint = false,
                                                                  bool truncate = false)
{
    return internal::expression_linspace<TF, precise>(start, stop, size, endpoint, truncate);
}
KFR_FN(linspace)

template <typename T, bool precise = false, typename TF = ftype<T>>
KFR_INTRINSIC internal::expression_linspace<TF, precise> symmlinspace(T symsize, size_t size,
                                                                      bool endpoint = false)
{
    return internal::expression_linspace<TF, precise>(symmetric_linspace, symsize, size, endpoint);
}
KFR_FN(symmlinspace)

template <size_t size, typename... E>
KFR_INTRINSIC internal::expression_sequence<decay<E>...> gen_sequence(const size_t (&list)[size], E&&... gens)
{
    static_assert(size == sizeof...(E), "Lists must be of equal length");
    return internal::expression_sequence<decay<E>...>(list, std::forward<E>(gens)...);
}
KFR_FN(gen_sequence)

/**
 * @brief Returns template expression that returns the result of calling \f$ fn(x_i, x_{i-1}) \f$
 */
template <typename Fn, typename E1>
KFR_INTRINSIC internal::expression_adjacent<Fn, E1> adjacent(Fn&& fn, E1&& e1)
{
    return internal::expression_adjacent<Fn, E1>(std::forward<Fn>(fn), std::forward<E1>(e1));
}

namespace internal
{
template <typename E>
struct expression_padded : expression_with_arguments<E>
{
    using value_type = value_type_of<E>;

    KFR_MEM_INTRINSIC constexpr static size_t size() CMT_NOEXCEPT { return infinite_size; }

    expression_padded(value_type fill_value, E&& e)
        : expression_with_arguments<E>(std::forward<E>(e)), fill_value(fill_value), input_size(e.size())
    {
    }

    template <size_t N>
    KFR_INTRINSIC friend vec<value_type, N> get_elements(const expression_padded& self, cinput_t cinput,
                                                         size_t index, vec_shape<value_type, N> y)
    {
        if (index >= self.input_size)
        {
            return self.fill_value;
        }
        else if (index + N <= self.input_size)
        {
            return self.argument_first(cinput, index, y);
        }
        else
        {
            vec<value_type, N> x{};
            for (size_t i = 0; i < N; i++)
            {
                if (index + i < self.input_size)
                    x[i] = self.argument_first(cinput, index + i, vec_shape<value_type, 1>()).front();
                else
                    x[i] = self.fill_value;
            }
            return x;
        }
    }
    value_type fill_value;
    const size_t input_size;
};
} // namespace internal

/**
 * @brief Returns infinite template expression that pads e with fill_value (default value = 0)
 */
template <typename E, typename T = value_type_of<E>>
internal::expression_padded<E> padded(E&& e, const T& fill_value = T(0))
{
    static_assert(is_input_expression<E>, "E must be an input expression");
    return internal::expression_padded<E>(fill_value, std::forward<E>(e));
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
    void operator()(coutput_t coutput, size_t index, const vec<T, N>& x)
    {
        cfor(csize_t<0>(), csize_t<sizeof...(E)>(),
             [&](auto n) { std::get<val_of(decltype(n)())>(outputs)(coutput, index, x); });
    }
    std::tuple<E...> outputs;

private:
};

template <typename... E>
struct expression_pack : expression_with_arguments<E...>
{
    constexpr static size_t count = sizeof...(E);

    expression_pack(E&&... e) : expression_with_arguments<E...>(std::forward<E>(e)...) {}
    using value_type = vec<common_type<value_type_of<E>...>, count>;
    using T          = value_type;

    using expression_with_arguments<E...>::size;

    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_pack& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N> y)
    {
        return self.call(cinput, fn::packtranspose(), index, y);
    }
};

template <typename... E>
struct expression_unpack : private expression_with_arguments<E...>, output_expression
{
    using expression_with_arguments<E...>::begin_block;
    using expression_with_arguments<E...>::end_block;
    using output_expression::begin_block;
    using output_expression::end_block;
    constexpr static size_t count = sizeof...(E);

    expression_unpack(E&&... e) : expression_with_arguments<E...>(std::forward<E>(e)...) {}

    using expression_with_arguments<E...>::size;

    template <typename U, size_t N>
    KFR_MEM_INTRINSIC void operator()(coutput_t coutput, size_t index, const vec<vec<U, count>, N>& x)
    {
        output(coutput, index, x, csizeseq<count>);
    }

    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    KFR_MEM_INTRINSIC expression_unpack& operator=(Input&& input)
    {
        process(*this, std::forward<Input>(input));
        return *this;
    }

private:
    template <typename U, size_t N, size_t... indices>
    void output(coutput_t coutput, size_t index, const vec<vec<U, count>, N>& x, csizes_t<indices...>)
    {
        const vec<vec<U, N>, count> xx = vec<vec<U, N>, count>::from_flatten(transpose<count>(flatten(x)));
        swallow{ (std::get<indices>(this->args)(coutput, index, xx[indices]), void(), 0)... };
    }
};
} // namespace internal

template <typename... E, KFR_ENABLE_IF(is_output_expressions<E...>)>
internal::expression_unpack<E...> unpack(E&&... e)
{
    return internal::expression_unpack<E...>(std::forward<E>(e)...);
}

template <typename... E, KFR_ENABLE_IF(is_input_expressions<E...>)>
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
        if (index >= count)
            return 0;
        return process(output, input, index * chunk_size,
                       index == count - 1 ? size - (count - 1) * chunk_size : chunk_size);
    }
};

template <typename OutExpr, typename InExpr, typename T = value_type_of<InExpr>>
task_partition<OutExpr, InExpr> partition(OutExpr&& output, InExpr&& input, size_t count,
                                          size_t minimum_size = 0)
{
    static_assert(!is_infinite<OutExpr> || !is_infinite<InExpr>, "");

    minimum_size            = minimum_size == 0 ? vector_width<T> * 8 : minimum_size;
    const size_t size       = size_min(output.size(), input.size());
    const size_t chunk_size = align_up(std::max(size / count, minimum_size), vector_width<T>);

    task_partition<OutExpr, InExpr> result(std::forward<OutExpr>(output), std::forward<InExpr>(input), size,
                                           chunk_size, (size + chunk_size - 1) / chunk_size);
    return result;
}

namespace internal
{

template <typename E1, typename E2>
struct concatenate_expression : expression_with_arguments<E1, E2>
{
    using value_type = common_type<value_type_of<E1>, value_type_of<E2>>;
    using T          = value_type;

    KFR_MEM_INTRINSIC constexpr size_t size() const CMT_NOEXCEPT
    {
        return size_add(std::get<0>(this->args).size(), std::get<1>(this->args).size());
    }
    template <typename E1_, typename E2_>
    concatenate_expression(E1_&& e1, E2_&& e2)
        : expression_with_arguments<E1, E2>(std::forward<E1_>(e1), std::forward<E2_>(e2))
    {
    }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const concatenate_expression& self, cinput_t cinput,
                                                size_t index, vec_shape<T, N> y)
    {
        const size_t size0 = std::get<0>(self.args).size();
        if (index >= size0)
        {
            return self.argument(cinput, csize<1>, index - size0, y);
        }
        else if (index + N <= size0)
        {
            return self.argument(cinput, csize<0>, index, y);
        }
        else // (index < size0) && (index + N > size0)
        {
            vec<T, N> result;
            for (size_t i = 0; i < size0 - index; ++i)
            {
                result[i] = self.argument(cinput, csize<0>, index + i, vec_shape<T, 1>{})[0];
            }
            for (size_t i = size0 - index; i < N; ++i)
            {
                result[i] = self.argument(cinput, csize<1>, index + i - size0, vec_shape<T, 1>{})[0];
            }
            return result;
        }
    }
};
} // namespace internal

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expression<E1>&& is_input_expression<E2>)>
internal::concatenate_expression<E1, E2> concatenate(E1&& e1, E2&& e2)
{
    return { std::forward<E1>(e1), std::forward<E2>(e2) };
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
