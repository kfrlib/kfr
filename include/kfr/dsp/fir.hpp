/** @addtogroup fir
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

#include "../base/basic_expressions.hpp"
#include "../base/filter.hpp"
#include "../base/memory.hpp"
#include "../base/reduce.hpp"
#include "../base/univector.hpp"
#include "../simd/vec.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, size_t Size>
using fir_taps = univector<T, Size>;

template <size_t tapcount, typename T, typename U = T>
struct short_fir_state
{
    template <size_t N>
    short_fir_state(const univector<T, N>& taps)
        : taps(widen<tapcount>(read<N>(taps.data()), T(0))), delayline(0)
    {
    }
    template <size_t N>
    short_fir_state(const univector<const T, N>& taps)
        : taps(widen<tapcount>(read<N>(taps.data()), T(0))), delayline(0)
    {
    }
    vec<T, tapcount> taps;
    mutable vec<U, tapcount - 1> delayline;
};

template <typename T, typename U = T>
struct fir_state
{
    fir_state(const array_ref<const T>& taps)
        : taps(taps.size()), delayline(taps.size(), U(0)), delayline_cursor(0)
    {
        this->taps = reverse(make_univector(taps.data(), taps.size()));
    }
    univector<T> taps;
    mutable univector<U> delayline;
    mutable size_t delayline_cursor;
};

namespace internal
{

template <typename T, bool stateless>
struct state_holder
{
    state_holder()                    = delete;
    state_holder(const state_holder&) = default;
    state_holder(state_holder&&)      = default;
    constexpr state_holder(const T& state) CMT_NOEXCEPT : s(state) {}
    T s;
};

template <typename T>
struct state_holder<T, true>
{
    state_holder()                    = delete;
    state_holder(const state_holder&) = default;
    state_holder(state_holder&&)      = default;
    constexpr state_holder(const T& state) CMT_NOEXCEPT : s(state) {}
    const T& s;
};

template <size_t tapcount, typename T, typename U, typename E1, bool stateless = false, KFR_ARCH_DEP>
struct expression_short_fir : expression_with_arguments<E1>
{
    using value_type = U;

    expression_short_fir(E1&& e1, const short_fir_state<tapcount, T, U>& state)
        : expression_with_arguments<E1>(std::forward<E1>(e1)), state(state)
    {
    }

    template <size_t N>
    KFR_INTRINSIC friend vec<U, N> get_elements(const expression_short_fir& self, cinput_t cinput,
                                                size_t index, vec_shape<U, N> x)
    {
        vec<U, N> in = self.argument_first(cinput, index, x);

        vec<U, N> out = in * self.state.s.taps.front();
        cforeach(csizeseq<tapcount - 1, 1>, [&](auto I) {
            out = out +
                  concat_and_slice<tapcount - 1 - I, N>(self.state.s.delayline, in) * self.state.s.taps[I];
        });
        self.state.s.delayline = concat_and_slice<N, tapcount - 1>(self.state.s.delayline, in);

        return out;
    }
    state_holder<short_fir_state<tapcount, T, U>, stateless> state;
};

template <typename T, typename U, typename E1, bool stateless = false, KFR_ARCH_DEP>
struct expression_fir : expression_with_arguments<E1>
{
    using value_type = U;

    expression_fir(E1&& e1, const fir_state<T, U>& state)
        : expression_with_arguments<E1>(std::forward<E1>(e1)), state(state)
    {
    }

    template <size_t N>
    KFR_INTRINSIC friend vec<U, N> get_elements(const expression_fir& self, cinput_t cinput, size_t index,
                                                vec_shape<U, N> x)
    {
        const size_t tapcount = self.state.s.taps.size();
        const vec<U, N> input = self.argument_first(cinput, index, x);

        vec<U, N> output;
        size_t cursor = self.state.s.delayline_cursor;
        CMT_LOOP_NOUNROLL
        for (size_t i = 0; i < N; i++)
        {
            self.state.s.delayline.ringbuf_write(cursor, input[i]);
            output[i] =
                dotproduct(self.state.s.taps, self.state.s.delayline.slice(cursor) /*, tapcount - cursor*/) +
                dotproduct(self.state.s.taps.slice(tapcount - cursor), self.state.s.delayline /*, cursor*/);
        }
        self.state.s.delayline_cursor = cursor;
        return output;
    }
    state_holder<fir_state<T, U>, stateless> state;
};
} // namespace internal

/**
 * @brief Returns template expression that applies FIR filter to the input
 * @param e1 an input expression
 * @param taps coefficients for the FIR filter
 */
template <typename T, typename E1, univector_tag Tag>
KFR_INTRINSIC internal::expression_fir<T, value_type_of<E1>, E1> fir(E1&& e1, const univector<T, Tag>& taps)
{
    return internal::expression_fir<T, value_type_of<E1>, E1>(std::forward<E1>(e1), taps.ref());
}

/**
 * @brief Returns template expression that applies FIR filter to the input
 * @param state FIR filter state
 * @param e1 an input expression
 */
template <typename T, typename U, typename E1>
KFR_INTRINSIC internal::expression_fir<T, U, E1, true> fir(fir_state<T, U>& state, E1&& e1)
{
    return internal::expression_fir<T, U, E1, true>(std::forward<E1>(e1), state);
}

/**
 * @brief Returns template expression that applies FIR filter to the input (count of coefficients must be in
 * range 2..32)
 * @param e1 an input expression
 * @param taps coefficients for the FIR filter
 */
template <typename T, size_t TapCount, typename E1>
KFR_INTRINSIC internal::expression_short_fir<next_poweroftwo(TapCount - 1) + 1, T, value_type_of<E1>, E1>
short_fir(E1&& e1, const univector<T, TapCount>& taps)
{
    static_assert(TapCount >= 2 && TapCount <= 33, "Use short_fir only for small FIR filters");
    return internal::expression_short_fir<next_poweroftwo(TapCount - 1) + 1, T, value_type_of<E1>, E1>(
        std::forward<E1>(e1), taps);
}

template <typename T, typename U = T>
class filter_fir : public filter<U>
{
public:
    filter_fir(const array_ref<const T>& taps) : state(taps) {}

    void set_taps(const array_ref<const T>& taps) { state = fir_state<T, U>(taps); }

    /// Reset internal filter state
    void reset() final
    {
        state.delayline        = scalar(0);
        state.delayline_cursor = 0;
    }

protected:
    void process_buffer(U* dest, const U* src, size_t size) final
    {
        make_univector(dest, size) = fir(state, make_univector(src, size));
    }
    void process_expression(U* dest, const expression_pointer<U>& src, size_t size) final
    {
        make_univector(dest, size) = fir(state, src);
    }

private:
    fir_state<T, U> state;
};
} // namespace CMT_ARCH_NAME
} // namespace kfr
