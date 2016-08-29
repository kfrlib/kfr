/** @addtogroup dsp
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

#include "../base/expression.hpp"
#include "../base/univector.hpp"

namespace kfr
{

namespace internal
{
template <size_t delay, typename E>
struct expression_delay : expression<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;
    using expression<E>::expression;

    template <size_t N, KFR_ENABLE_IF(N <= delay)>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        vec<T, N> out;
        size_t c = cursor;
        data.ringbuf_read(c, out);
        const vec<T, N> in = this->argument_first(index, vec_t<T, N>());
        data.ringbuf_write(cursor, in);
        return out;
    }
    vec<T, 1> operator()(cinput_t, size_t index, vec_t<T, 1>) const
    {
        T out;
        size_t c = cursor;
        data.ringbuf_read(c, out);
        const T in = this->argument_first(index, vec_t<T, 1>())[0];
        data.ringbuf_write(cursor, in);
        return out;
    }
    template <size_t N, KFR_ENABLE_IF(N > delay)>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        vec<T, delay> out;
        size_t c = cursor;
        data.ringbuf_read(c, out);
        const vec<T, N> in = this->argument_first(index, vec_t<T, N>());
        data.ringbuf_write(cursor, slice<N - delay, delay>(in));
        return concat_and_slice<0, N>(out, in);
    }

    mutable univector<value_type, delay> data = scalar(value_type(0));
    mutable size_t cursor = 0;
};

template <typename E>
struct expression_delay<1, E> : expression<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;
    using expression<E>::expression;

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        const vec<T, N> in  = this->argument_first(index, vec_t<T, N>());
        const vec<T, N> out = insertleft(data, in);
        data = in[N - 1];
        return out;
    }
    mutable value_type data = value_type(0);
};
}

/**
 * @brief Returns template expression that applies delay to the input (uses ring buffer internally)
 * @param e1 an input expression
 * @param samples delay in samples (must be a compile time value)
 * @code
 * univector<double, 10> v = counter();
 * auto d = delay(v, csize<4>);
 * @endcode
 */
template <size_t samples = 1, typename E1>
CMT_INLINE internal::expression_delay<samples, E1> delay(E1&& e1, csize_t<samples> = csize<samples>)
{
    static_assert(samples >= 1 && samples < 1024, "");
    return internal::expression_delay<samples, E1>(std::forward<E1>(e1));
}
}
