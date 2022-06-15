/** @addtogroup biquad
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

#include "../base/filter.hpp"
#include "../base/pointer.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/operators.hpp"
#include "../simd/vec.hpp"
#include "../testo/assert.hpp"

namespace kfr
{

enum class biquad_type
{
    lowpass,
    highpass,
    bandpass,
    bandstop,
    peak,
    notch,
    lowshelf,
    highshelf
};

/**
 * @brief Structure for holding biquad filter coefficients.
 */
template <typename T>
struct biquad_params
{
    template <typename U>
    constexpr biquad_params(const biquad_params<U>& bq) CMT_NOEXCEPT : a0(static_cast<T>(bq.a0)),
                                                                       a1(static_cast<T>(bq.a1)),
                                                                       a2(static_cast<T>(bq.a2)),
                                                                       b0(static_cast<T>(bq.b0)),
                                                                       b1(static_cast<T>(bq.b1)),
                                                                       b2(static_cast<T>(bq.b2))
    {
    }
    constexpr static bool is_pod = true;

    static_assert(cometa::is_floating_point<T>, "T must be a floating point type");
    constexpr biquad_params() CMT_NOEXCEPT : a0(1), a1(0), a2(0), b0(1), b1(0), b2(0) {}
    constexpr biquad_params(T a0, T a1, T a2, T b0, T b1, T b2) CMT_NOEXCEPT : a0(a0),
                                                                               a1(a1),
                                                                               a2(a2),
                                                                               b0(b0),
                                                                               b1(b1),
                                                                               b2(b2)
    {
    }
    T a0;
    T a1;
    T a2;
    T b0;
    T b1;
    T b2;
    biquad_params<T> normalized_a0() const
    {
        vec<T, 5> v{ a1, a2, b0, b1, b2 };
        v = v / a0;
        return { T(1.0), v[0], v[1], v[2], v[3], v[4] };
    }
    biquad_params<T> normalized_b0() const { return { a0, a1, a2, T(1.0), b1 / b0, b2 / b0 }; }
    biquad_params<T> normalized_all() const { return normalized_a0().normalized_b0(); }
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{
template <typename T, size_t filters>
struct biquad_state
{
    vec<T, filters> s1;
    vec<T, filters> s2;
    vec<T, filters> out;
    constexpr biquad_state() CMT_NOEXCEPT : s1(0), s2(0), out(0) {}
};

template <typename T, size_t filters>
struct biquad_block
{
    vec<T, filters> a1;
    vec<T, filters> a2;
    vec<T, filters> b0;
    vec<T, filters> b1;
    vec<T, filters> b2;

    constexpr biquad_block() CMT_NOEXCEPT : a1(0), a2(0), b0(1), b1(0), b2(0) {}
    CMT_GNU_CONSTEXPR biquad_block(const biquad_params<T>* bq, size_t count) CMT_NOEXCEPT
    {
        count = count > filters ? filters : count;
        for (size_t i = 0; i < count; i++)
        {
            a1[i] = bq[i].a1;
            a2[i] = bq[i].a2;
            b0[i] = bq[i].b0;
            b1[i] = bq[i].b1;
            b2[i] = bq[i].b2;
        }
        for (size_t i = count; i < filters; i++)
        {
            a1[i] = T(0);
            a2[i] = T(0);
            b0[i] = T(1);
            b1[i] = T(0);
            b2[i] = T(0);
        }
    }

    template <size_t count>
    constexpr biquad_block(const biquad_params<T> (&bq)[count]) CMT_NOEXCEPT : biquad_block(bq, count)
    {
        static_assert(count <= filters, "count > filters");
    }
};

template <size_t filters, typename T, typename E1>
struct expression_biquads_l : public expression_with_arguments<E1>
{
    using value_type = T;

    expression_biquads_l(const biquad_block<T, filters>& bq, E1&& e1)
        : expression_with_arguments<E1>(std::forward<E1>(e1)), bq(bq)
    {
    }
    template <size_t width>
    friend KFR_INTRINSIC vec<T, width> get_elements(const expression_biquads_l& self, cinput_t cinput,
                                                    size_t index, vec_shape<T, width> t)
    {
        const vec<T, width> in = self.argument_first(cinput, index, t);
        vec<T, width> out;

        CMT_LOOP_UNROLL
        for (size_t i = 0; i < width; i++)
        {
            self.state.out = process(self.bq, self.state, insertleft(in[i], self.state.out));
            out[i]         = self.state.out[filters - 1];
        }

        return out;
    }
    static KFR_MEM_INTRINSIC vec<T, filters> process(const biquad_block<T, filters>& bq,
                                                     biquad_state<T, filters>& state,
                                                     const vec<T, filters>& in)
    {
        const vec<T, filters> out = bq.b0 * in + state.s1;
        state.s1                  = state.s2 + bq.b1 * in - bq.a1 * out;
        state.s2                  = bq.b2 * in - bq.a2 * out;
        return out;
    }
    biquad_block<T, filters> bq;
    mutable biquad_state<T, filters> state;
};

template <size_t filters, typename T, typename E1>
struct expression_biquads : expression_with_arguments<E1>
{
    using value_type = T;

    expression_biquads(const biquad_block<T, filters>& bq, E1&& e1)
        : expression_with_arguments<E1>(std::forward<E1>(e1)), bq(bq), block_end(0)
    {
    }

    void begin_block(cinput_t cinput, size_t size) const
    {
        block_end = size;
        for (size_t i = 0; i < filters - 1; i++)
        {
            const vec<T, 1> in = i < size ? this->argument_first(cinput, i, vec_shape<T, 1>()) : 0;
            state.out          = process(bq, state, insertleft(in[0], state.out));
        }
    }
    void end_block(cinput_t, size_t) const { state = saved_state; }

    template <size_t width>
    friend KFR_INTRINSIC vec<T, width> get_elements(const expression_biquads& self, cinput_t cinput,
                                                    size_t index, vec_shape<T, width> t)
    {
        index += filters - 1;
        vec<T, width> out{};
        if (index + width <= self.block_end)
        {
            const vec<T, width> in = self.argument_first(cinput, index, t);

            CMT_LOOP_UNROLL
            for (size_t i = 0; i < width; i++)
            {
                self.state.out = process(self.bq, self.state, insertleft(in[i], self.state.out));
                out[i]         = self.state.out[filters - 1];
            }
            if (index + width == self.block_end)
                self.saved_state = self.state;
        }
        else if (index >= self.block_end)
        {
            CMT_LOOP_UNROLL
            for (size_t i = 0; i < width; i++)
            {
                self.state.out = process(self.bq, self.state, insertleft(T(0), self.state.out));
                out[i]         = self.state.out[filters - 1];
            }
        }
        else
        {
            size_t i = 0;
            for (; i < std::min(width, self.block_end - index); i++)
            {
                const vec<T, 1> in = self.argument_first(cinput, index + i, vec_shape<T, 1>());
                self.state.out     = process(self.bq, self.state, insertleft(in[0], self.state.out));
                out[i]             = self.state.out[filters - 1];
            }
            self.saved_state = self.state;
            for (; i < width; i++)
            {
                self.state.out = process(self.bq, self.state, insertleft(T(0), self.state.out));
                out[i]         = self.state.out[filters - 1];
            }
        }
        return out;
    }
    static KFR_MEM_INTRINSIC vec<T, filters> process(const biquad_block<T, filters>& bq,
                                                     biquad_state<T, filters>& state, vec<T, filters> in)
    {
        const vec<T, filters> out = bq.b0 * in + state.s1;
        state.s1                  = state.s2 + bq.b1 * in - bq.a1 * out;
        state.s2                  = bq.b2 * in - bq.a2 * out;
        return out;
    }
    biquad_block<T, filters> bq;

    mutable biquad_state<T, filters> state;
    mutable biquad_state<T, filters> saved_state;
    mutable size_t block_end;
};
} // namespace internal

/**
 * @brief Returns template expressions that applies biquad filter to the input.
 * @param bq Biquad coefficients
 * @param e1 Input expression
 */
template <typename T, typename E1>
KFR_FUNCTION internal::expression_biquads<1, T, E1> biquad(const biquad_params<T>& bq, E1&& e1)
{
    const biquad_params<T> bqs[1] = { bq };
    return internal::expression_biquads<1, T, E1>(bqs, std::forward<E1>(e1));
}

/**
 * @brief Returns template expressions that applies cascade of biquad filters to the input.
 * @param bq Array of biquad coefficients
 * @param e1 Input expression
 * @note This implementation introduces delay of N - 1 samples, where N is the filter count.
 */
template <size_t filters, typename T, typename E1>
KFR_FUNCTION internal::expression_biquads_l<filters, T, E1> biquad_l(const biquad_params<T> (&bq)[filters],
                                                                     E1&& e1)
{
    return internal::expression_biquads_l<filters, T, E1>(bq, std::forward<E1>(e1));
}

/**
 * @brief Returns template expressions that applies cascade of biquad filters to the input.
 * @param bq Array of biquad coefficients
 * @param e1 Input expression
 * @note This implementation has zero latency
 */
template <size_t filters, typename T, typename E1>
KFR_FUNCTION internal::expression_biquads<filters, T, E1> biquad(const biquad_params<T> (&bq)[filters],
                                                                 E1&& e1)
{
    return internal::expression_biquads<filters, T, E1>(bq, std::forward<E1>(e1));
}

/**
 * @brief Returns template expressions that applies cascade of biquad filters to the input.
 * @param bq Array of biquad coefficients
 * @param e1 Input expression
 * @note This implementation has zero latency
 */
template <size_t maxfiltercount = 4, typename T, typename E1>
KFR_FUNCTION expression_pointer<T> biquad(const biquad_params<T>* bq, size_t count, E1&& e1)
{
    constexpr csizes_t<1, 2, 4, 8, 16, 32, 64> sizes;
    return cswitch(
        cfilter(sizes, sizes <= csize_t<maxfiltercount>{}), next_poweroftwo(count),
        [&](auto x) {
            constexpr size_t filters = x;
            return to_pointer(internal::expression_biquads<filters, T, E1>(
                internal::biquad_block<T, filters>(bq, count), std::forward<E1>(e1)));
        },
        [&] { return to_pointer(zeros<T>()); });
}

template <size_t maxfiltercount = 4, typename T, typename E1>
KFR_FUNCTION expression_pointer<T> biquad(const std::vector<biquad_params<T>>& bq, E1&& e1)
{
    return biquad<maxfiltercount>(bq.data(), bq.size(), std::forward<E1>(e1));
}

template <typename T, size_t maxfiltercount = 4>
class biquad_filter : public expression_filter<T>
{
public:
    biquad_filter(const biquad_params<T>* bq, size_t count)
        : expression_filter<T>(biquad<maxfiltercount>(bq, count, placeholder<T>()))
    {
    }

    template <size_t N>
    biquad_filter(const biquad_params<T> (&bq)[N]) : biquad_filter(bq, N)
    {
    }

    biquad_filter(const std::vector<biquad_params<T>>& bq)
        : biquad_filter(bq.data(), bq.size())
    {
    }

};

} // namespace CMT_ARCH_NAME

CMT_MULTI_PROTO(template <typename T, size_t maxfiltercount>
                filter<T>* make_biquad_filter(const biquad_params<T>* bq, size_t count);)

#ifdef CMT_MULTI
template <typename T, size_t maxfiltercount>
KFR_FUNCTION filter<T>* make_biquad_filter(cpu_t cpu, const biquad_params<T>* bq, size_t count)
{
    CMT_MULTI_PROTO_GATE(make_biquad_filter<T, maxfiltercount>(bq, count))
}
#endif
} // namespace kfr
