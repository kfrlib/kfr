/** @addtogroup fir
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

#include "../base/basic_expressions.hpp"
#include "../base/expression.hpp"
#include "../base/univector.hpp"
#include "state_holder.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, size_t samples, univector_tag Tag = samples>
struct delay_state
{
    template <size_t S2 = samples, KFR_ENABLE_IF(S2 == Tag)>
    delay_state() : data({ 0 }), cursor(0)
    {
    }

    template <size_t S2 = samples, KFR_ENABLE_IF(S2 != Tag)>
    delay_state() : data(samples), cursor(0)
    {
    }

    mutable univector<T, Tag> data;
    mutable size_t cursor;
};

template <typename T>
struct delay_state<T, 1, 1>
{
    mutable T data = T(0);
};

namespace internal
{

template <size_t delay, typename E, bool stateless, univector_tag STag>
struct expression_delay : expression_with_arguments<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;
    using expression_with_arguments<E>::expression_with_arguments;

    expression_delay(E&& e, const delay_state<T, delay, STag>& state)
        : expression_with_arguments<E>(std::forward<E>(e)), state(state)
    {
    }

    template <size_t N, KFR_ENABLE_IF(N <= delay)>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N>)
    {
        vec<T, N> out;
        size_t c = self.state.s.cursor;
        self.state.s.data.ringbuf_read(c, out);
        const vec<T, N> in = self.argument_first(cinput, index, vec_shape<T, N>());
        self.state.s.data.ringbuf_write(self.state.s.cursor, in);
        return out;
    }
    friend vec<T, 1> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                  vec_shape<T, 1>)
    {
        T out;
        size_t c = self.state.s.cursor;
        self.state.s.data.ringbuf_read(c, out);
        const T in = self.argument_first(cinput, index, vec_shape<T, 1>())[0];
        self.state.s.data.ringbuf_write(self.state.s.cursor, in);
        return out;
    }
    template <size_t N, KFR_ENABLE_IF(N > delay)>
    friend vec<T, N> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                  vec_shape<T, N>)
    {
        vec<T, delay> out;
        size_t c = self.state.s.cursor;
        self.state.s.data.ringbuf_read(c, out);
        const vec<T, N> in = self.argument_first(cinput, index, vec_shape<T, N>());
        self.state.s.data.ringbuf_write(self.state.s.cursor, slice<N - delay, delay>(in));
        return concat_and_slice<0, N>(out, in);
    }

    state_holder<delay_state<T, delay, STag>, stateless> state;
};

template <typename E, bool stateless, univector_tag STag>
struct expression_delay<1, E, stateless, STag> : expression_with_arguments<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;
    using expression_with_arguments<E>::expression_with_arguments;

    expression_delay(E&& e, const delay_state<T, 1, STag>& state)
        : expression_with_arguments<E>(std::forward<E>(e)), state(state)
    {
    }

    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N>)
    {
        const vec<T, N> in  = self.argument_first(cinput, index, vec_shape<T, N>());
        const vec<T, N> out = insertleft(self.state.s.data, in);
        self.state.s.data   = in[N - 1];
        return out;
    }
    state_holder<delay_state<T, 1, STag>, stateless> state;
};
} // namespace internal

/**
 * @brief Returns template expression that applies delay to the input (uses ring buffer internally)
 * @param e1 an input expression
 * @param samples delay in samples (must be a compile time value)
 * @code
 * univector<double, 10> v = counter();
 * auto d = delay(v, csize<4>);
 * @endcode
 */
template <size_t samples = 1, typename E1, typename T = value_type_of<E1>>
KFR_INTRINSIC internal::expression_delay<samples, E1, false, samples> delay(E1&& e1)
{
    static_assert(samples >= 1 && samples < 1024, "");
    return internal::expression_delay<samples, E1, false, samples>(std::forward<E1>(e1),
                                                                   delay_state<T, samples>());
}

/**
 * @brief Returns template expression that applies delay to the input (uses ring buffer in state)
 * @param state delay filter state
 * @param e1 an input expression
 * @code
 * univector<double, 10> v = counter();
 * delay_state<double, 4> state;
 * auto d = delay(state, v);
 * @endcode
 */
template <size_t samples, typename T, typename E1, univector_tag STag>
KFR_INTRINSIC internal::expression_delay<samples, E1, true, STag> delay(delay_state<T, samples, STag>& state,
                                                                        E1&& e1)
{
    static_assert(STag == tag_dynamic_vector || (samples >= 1 && samples < 1024), "");
    return internal::expression_delay<samples, E1, true, STag>(std::forward<E1>(e1), state);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
