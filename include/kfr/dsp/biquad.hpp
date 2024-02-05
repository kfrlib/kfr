/** @addtogroup biquad
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

#include "../base/filter.hpp"
#include "../base/handle.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/operators.hpp"
#include "../base/state_holder.hpp"
#include "../simd/vec.hpp"
#include "../testo/assert.hpp"

namespace kfr
{

constexpr inline size_t maximum_iir_order    = 128;
constexpr inline size_t maximum_biquad_count = maximum_iir_order / 2;

namespace internal_generic
{
constexpr inline auto biquad_sizes = csize<1> << csizeseq<ilog2(maximum_biquad_count) + 1>;
}

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
struct biquad_section
{
    template <typename U>
    constexpr biquad_section(const biquad_section<U>& bq) CMT_NOEXCEPT : a0(static_cast<T>(bq.a0)),
                                                                         a1(static_cast<T>(bq.a1)),
                                                                         a2(static_cast<T>(bq.a2)),
                                                                         b0(static_cast<T>(bq.b0)),
                                                                         b1(static_cast<T>(bq.b1)),
                                                                         b2(static_cast<T>(bq.b2))
    {
    }

    static_assert(std::is_floating_point_v<T>, "T must be a floating point type");
    constexpr biquad_section() CMT_NOEXCEPT : a0(1), a1(0), a2(0), b0(1), b1(0), b2(0) {}
    constexpr biquad_section(T a0, T a1, T a2, T b0, T b1, T b2) CMT_NOEXCEPT : a0(a0),
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
    biquad_section<T> normalized_a0() const
    {
        vec<T, 5> v{ a1, a2, b0, b1, b2 };
        v = v / a0;
        return { T(1.0), v[0], v[1], v[2], v[3], v[4] };
    }
    biquad_section<T> normalized_b0() const { return { a0, a1, a2, T(1.0), b1 / b0, b2 / b0 }; }
    biquad_section<T> normalized_all() const { return normalized_a0().normalized_b0(); }
};

template <typename T, size_t filters>
struct biquad_state
{
    vec<T, filters> s1;
    vec<T, filters> s2;
    vec<T, filters> out;
    constexpr biquad_state() CMT_NOEXCEPT : s1(0), s2(0), out(0) {}
};

template <typename T, size_t filters = tag_dynamic_vector>
struct iir_params
{
    vec<T, filters> a1;
    vec<T, filters> a2;
    vec<T, filters> b0;
    vec<T, filters> b1;
    vec<T, filters> b2;

    constexpr iir_params() CMT_NOEXCEPT : a1(0), a2(0), b0(1), b1(0), b2(0) {}
    CMT_GNU_CONSTEXPR iir_params(const biquad_section<T>* bq, size_t count)
    {
        KFR_LOGIC_CHECK(count <= filters, "iir_params: too many biquad sections");
        count = const_min(filters, count);
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

    CMT_GNU_CONSTEXPR iir_params(const biquad_section<T>& one) CMT_NOEXCEPT : iir_params(&one, 1) {}

    template <typename Container, CMT_HAS_DATA_SIZE(Container)>
    constexpr iir_params(Container&& cont) CMT_NOEXCEPT : iir_params(std::data(cont), std::size(cont))
    {
    }
};

template <typename T>
struct iir_params<T, tag_dynamic_vector> : public std::vector<biquad_section<T>>
{
    using base = std::vector<biquad_section<T>>;

    iir_params()                  = default;
    iir_params(const iir_params&) = default;
    iir_params(iir_params&&)      = default;

    iir_params(size_t count) : base(count) {}

    iir_params(const biquad_section<T>* bq, size_t count) CMT_NOEXCEPT : base(bq, bq + count) {}

    iir_params(const biquad_section<T>& one) CMT_NOEXCEPT : iir_params(&one, 1) {}

    iir_params(std::vector<biquad_section<T>>&& sections) CMT_NOEXCEPT : base(std::move(sections)) {}

    template <typename Container, CMT_HAS_DATA_SIZE(Container)>
    constexpr iir_params(Container&& cont) CMT_NOEXCEPT : iir_params(std::data(cont), std::size(cont))
    {
    }

    template <size_t filters>
    iir_params(const iir_params<T, filters>& params) : base(filters)
    {
        for (size_t i = 0; i < filters; ++i)
        {
            this->operator[](i).a0 = T(1);
            this->operator[](i).a1 = params.a1[i];
            this->operator[](i).a2 = params.a2[i];
            this->operator[](i).b0 = params.b0[i];
            this->operator[](i).b1 = params.b1[i];
            this->operator[](i).b2 = params.b2[i];
        }
    }
};

template <typename T, size_t Size>
iir_params(const std::array<T, Size>&) -> iir_params<T, Size>;
template <typename T, size_t Size>
iir_params(const univector<T, Size>) -> iir_params<T, Size>;
template <typename T, size_t Size>
iir_params(const biquad_section<T> (&)[Size]) -> iir_params<T, Size>;
template <typename T>
iir_params(const biquad_section<T>&) -> iir_params<T, 1>;
template <typename T>
iir_params(const std::vector<biquad_section<T>>&) -> iir_params<T, tag_dynamic_vector>;
template <typename T>
iir_params(std::vector<biquad_section<T>>&&) -> iir_params<T, tag_dynamic_vector>;

template <typename T, size_t filters>
struct iir_state
{
    static_assert(filters >= 1 && filters <= maximum_biquad_count, "Incorrect number of biquad filters");

    iir_params<T, filters> params;

    template <typename... Args,
              std::enable_if_t<std::is_constructible_v<iir_params<T, filters>, Args...>>* = nullptr>
    iir_state(Args&&... args) : params(std::forward<Args>(args)...)
    {
    }

    biquad_state<T, filters> state;
    biquad_state<T, filters> saved_state;
    size_t block_end = 0;
};

template <typename T, size_t filters>
iir_state(const iir_params<T, filters>&) -> iir_state<T, filters>;
template <typename T, size_t filters>
iir_state(iir_params<T, filters>&&) -> iir_state<T, filters>;

inline namespace CMT_ARCH_NAME
{

template <size_t filters, typename T, typename E1, bool Stateless = false>
struct expression_iir_l : public expression_with_traits<E1>
{
    using value_type = T;

    expression_iir_l(E1&& e1, state_holder<iir_state<T, filters>, Stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    mutable state_holder<iir_state<T, filters>, Stateless> state;
};

template <size_t filters, typename T, typename E1, bool Stateless = false>
struct expression_iir : expression_with_traits<E1>
{
    using value_type = T;

    expression_iir(E1&& e1, state_holder<iir_state<T, filters>, Stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    mutable state_holder<iir_state<T, filters>, Stateless> state;
};

namespace internal
{

template <size_t filters, typename T>
KFR_INTRINSIC T biquad_process(vec<T, filters>& out, const iir_params<T, filters>& bq,
                               biquad_state<T, filters>& state, identity<T> in0,
                               const vec<T, filters>& delayline)
{
    vec<T, filters> in = insertleft(in0, delayline);
    out                = bq.b0 * in + state.s1;
    state.s1           = state.s2 + bq.b1 * in - bq.a1 * out;
    state.s2           = bq.b2 * in - bq.a2 * out;
    return out[filters - 1];
}
template <size_t filters, typename T, size_t N>
KFR_INTRINSIC vec<T, N> biquad_process(iir_state<T, filters>& state, const vec<T, N>& in,
                                       size_t save_state_after = static_cast<size_t>(-1))
{
    vec<T, N> out;
    if (CMT_LIKELY(save_state_after == static_cast<size_t>(-1)))
    {
        CMT_LOOP_UNROLL
        for (size_t i = 0; i < N; i++)
        {
            out[i] = biquad_process(state.state.out, state.params, state.state, in[i], state.state.out);
        }
    }
    else
    {
        for (size_t i = 0; i < save_state_after; i++)
        {
            out[i] = biquad_process(state.state.out, state.params, state.state, in[i], state.state.out);
        }
        state.saved_state = state.state;
        for (size_t i = save_state_after; i < N; i++)
        {
            out[i] = biquad_process(state.state.out, state.params, state.state, in[i], state.state.out);
        }
    }
    return out;
}
} // namespace internal

template <size_t filters, typename T, typename E1, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_iir_l<filters, T, E1>& self, shape<1> index,
                                     axis_params<0, N> t)
{
    const vec<T, N> in = get_elements(self.first(), index, t);
    return internal::biquad_process(*self.state, in);
}

template <typename T, typename E1>
KFR_INTRINSIC void begin_pass(const expression_iir<1, T, E1>&, shape<1>, shape<1>)
{
}
template <size_t filters, typename T, typename E1>
KFR_INTRINSIC void begin_pass(const expression_iir<filters, T, E1>& self, shape<1> start, shape<1> stop)
{
    size_t size           = stop.front();
    self.state->block_end = size;
    vec<T, filters - 1> in;
    for (index_t i = 0; i < filters - 1; i++)
    {
        in[i] = i < size ? get_elements(self.first(), shape<1>{ i }, axis_params_v<0, 1>).front() : 0;
    }
    internal::biquad_process(*self.state, in);
}

template <typename T, typename E1>
KFR_INTRINSIC void end_pass(const expression_iir<1, T, E1>&, shape<1>, shape<1>)
{
}
template <size_t filters, typename T, typename E1>
KFR_INTRINSIC void end_pass(const expression_iir<filters, T, E1>& self, shape<1> start, shape<1> stop)
{
    self.state->state = self.state->saved_state;
}

template <typename T, typename E1, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_iir<1, T, E1>& self, shape<1> index,
                                     axis_params<0, N> t)
{
    const vec<T, N> in = get_elements(self.first(), index, t);
    return internal::biquad_process(*self.state, in);
}

template <size_t filters, typename T, typename E1, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_iir<filters, T, E1>& self, shape<1> index,
                                     axis_params<0, N> t)
{
    using internal::biquad_process;
    index.front() += filters - 1;
    vec<T, N> out{};
    if (index.front() + N <= self.state->block_end)
    {
        const vec<T, N> in = get_elements(self.first(), shape<1>{ index.front() }, t);

        out = biquad_process(*self.state, in);
        if (index.front() + N == self.state->block_end)
            self.state->saved_state = self.state->state;
    }
    else if (index.front() >= self.state->block_end)
    {
        out = biquad_process(*self.state, vec<T, N>(0));
    }
    else
    {
        size_t save_at = std::min(N, self.state->block_end - static_cast<size_t>(index.front()));
        vec<T, N> in;
        for (size_t i = 0; i < save_at; ++i)
            in[i] =
                get_elements(self.first(), index.add_at(i, cval<index_t, 0>), axis_params_v<0, 1>).front();
        for (size_t i = save_at; i < N; ++i)
            in[i] = 0;
        out = biquad_process(*self.state, in, save_at);
    }
    return out;
}

/**
 * @brief Returns template expressions that applies biquad filter to the input.
 * @param e1 Input expression
 * @param params Biquad coefficients
 */
template <size_t filters, typename T, typename E1>
KFR_FUNCTION expression_iir<filters, T, E1> iir(E1&& e1, iir_params<T, filters> params)
{
    return expression_iir<filters, T, E1>(std::forward<E1>(e1), iir_state{ std::move(params) });
}

/**
 * @brief Returns template expressions that applies biquad filter to the input.
 * @param e1 Input expression
 * @param params Biquad coefficients
 */
template <typename T, typename E1>
KFR_FUNCTION expression_handle<T, 1> iir(E1&& e1, const iir_params<T, tag_dynamic_vector>& params)
{
    KFR_LOGIC_CHECK(next_poweroftwo(params.size()) <= maximum_biquad_count, "iir: too many biquad sections");
    return cswitch(
        internal_generic::biquad_sizes, next_poweroftwo(params.size()),
        [&](auto x)
        {
            constexpr size_t filters = x;
            return to_handle(expression_iir<filters, T, E1>(
                std::forward<E1>(e1), iir_state{ iir_params<T, filters>(params.data(), params.size()) }));
        },
        [&] { return to_handle(fixshape(zeros<T>(), fixed_shape<infinite_size>)); });
}

/**
 * @brief Returns template expressions that applies biquad filter to the input.
 * @param bq Biquad coefficients
 * @param e1 Input expression
 */
template <size_t filters, typename T, typename E1>
KFR_FUNCTION expression_iir<filters, T, E1, true> iir(E1&& e1,
                                                      std::reference_wrapper<iir_state<T, filters>> state)
{
    return expression_iir<filters, T, E1, true>(std::forward<E1>(e1), state);
}

#define KFR_BIQUAD_DEPRECATED                                                                                \
    [[deprecated("biquad(param, expr) prototype is deprecated. Use iir(expr, param) with swapped "           \
                 "arguments")]]

/**
 * @brief Returns template expressions that applies biquad filter to the input.
 * @param bq Biquad coefficients
 * @param e1 Input expression
 */
template <typename T, typename E1>
KFR_BIQUAD_DEPRECATED KFR_FUNCTION expression_iir<1, T, E1> biquad(const biquad_section<T>& bq, E1&& e1)
{
    const biquad_section<T> bqs[1] = { bq };
    return expression_iir<1, T, E1>(std::forward<E1>(e1), iir_state{ iir_params{ bqs } });
}

/**
 * @brief Returns template expressions that applies cascade of biquad filters to the input.
 * @param bq Array of biquad coefficients
 * @param e1 Input expression
 * @note This implementation introduces delay of N - 1 samples, where N is the filter count.
 */
template <size_t filters, typename T, typename E1>
KFR_BIQUAD_DEPRECATED KFR_FUNCTION expression_iir_l<filters, T, E1> biquad_l(
    const biquad_section<T> (&bq)[filters], E1&& e1)
{
    return expression_iir_l<filters, T, E1>(std::forward<E1>(e1), iir_state{ iir_params{ bq } });
}

/**
 * @brief Returns template expressions that applies cascade of biquad filters to the input.
 * @param bq Array of biquad coefficients
 * @param e1 Input expression
 * @note This implementation has zero latency
 */
template <size_t filters, typename T, typename E1>
KFR_BIQUAD_DEPRECATED KFR_FUNCTION expression_iir<filters, T, E1> biquad(
    const biquad_section<T> (&bq)[filters], E1&& e1)
{
    return expression_iir<filters, T, E1>(std::forward<E1>(e1), iir_state{ iir_params{ bq } });
}

/**
 * @brief Returns template expressions that applies cascade of biquad filters to the input.
 * @param bq Array of biquad coefficients
 * @param e1 Input expression
 * @note This implementation has zero latency
 */
template <size_t maxfiltercount = 4, typename T, typename E1>
KFR_BIQUAD_DEPRECATED KFR_FUNCTION expression_handle<T, 1> biquad(const biquad_section<T>* bq, size_t count,
                                                                  E1&& e1)
{
    KFR_LOGIC_CHECK(next_poweroftwo(count) <= maxfiltercount,
                    "biquad: too many biquad sections. Use higher maxfiltercount");
    return cswitch(
        cfilter(internal_generic::biquad_sizes, internal_generic::biquad_sizes <= csize_t<maxfiltercount>{}),
        next_poweroftwo(count),
        [&](auto x)
        {
            constexpr size_t filters = x;
            return to_handle(expression_iir<filters, T, E1>(std::forward<E1>(e1),
                                                            iir_state{ iir_params<T, filters>(bq, count) }));
        },
        [&] { return to_handle(fixshape(zeros<T>(), fixed_shape<infinite_size>)); });
}

template <size_t maxfiltercount = 4, typename T, typename E1>
KFR_BIQUAD_DEPRECATED KFR_FUNCTION expression_handle<T, 1> biquad(const std::vector<biquad_section<T>>& bq,
                                                                  E1&& e1)
{
    return biquad<maxfiltercount>(bq.data(), bq.size(), std::forward<E1>(e1));
}

template <size_t filters, typename T, typename E1>
using expression_biquads_l = expression_iir_l<filters, T, E1>;

template <size_t filters, typename T, typename E1>
using expression_biquads = expression_iir<filters, T, E1>;

} // namespace CMT_ARCH_NAME

template <typename T>
using biquad_params [[deprecated("biquad_params is deprecated. Use biquad_section")]] = biquad_section<T>;

template <typename T, size_t filters = tag_dynamic_vector>
using biquad_blocks [[deprecated("biquad_blocks is deprecated. Use iir_params")]] = iir_params<T, filters>;

template <typename T>
class iir_filter : public expression_filter<T>
{
public:
    iir_filter(const iir_params<T>& params);

    [[deprecated("iir_filter(bq, count) is deprecated. Use iir_filter(iir_params{bq, count})")]] iir_filter(
        const biquad_section<T>* bq, size_t count)
        : iir_filter(iir_params<T>(bq, count))
    {
    }
};

template <typename T>
using biquad_filter [[deprecated("biquad_filter is deprecated. Use iir_filter")]] = iir_filter<T>;
} // namespace kfr
