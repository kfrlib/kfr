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

#include "../base/expression.hpp"
#include "../base/univector.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace internal
{
template <size_t delay, typename E>
struct expression_delay : expression_with_arguments<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;
    using expression_with_arguments<E>::expression_with_arguments;

    template <size_t N, KFR_ENABLE_IF(N <= delay)>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                  vec_shape<T, N>)
    {
        vec<T, N> out;
        size_t c = self.cursor;
        self.data.ringbuf_read(c, out);
        const vec<T, N> in = self.argument_first(cinput, index, vec_shape<T, N>());
        self.data.ringbuf_write(self.cursor, in);
        return out;
    }
    friend vec<T, 1> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                  vec_shape<T, 1>)
    {
        T out;
        size_t c = self.cursor;
        self.data.ringbuf_read(c, out);
        const T in = self.argument_first(cinput, index, vec_shape<T, 1>())[0];
        self.data.ringbuf_write(self.cursor, in);
        return out;
    }
    template <size_t N, KFR_ENABLE_IF(N > delay)>
    friend vec<T, N> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                  vec_shape<T, N>)
    {
        vec<T, delay> out;
        size_t c = self.cursor;
        self.data.ringbuf_read(c, out);
        const vec<T, N> in = self.argument_first(cinput, index, vec_shape<T, N>());
        self.data.ringbuf_write(self.cursor, slice<N - delay, delay>(in));
        return concat_and_slice<0, N>(out, in);
    }

    mutable univector<value_type, delay> data = scalar(value_type(0));
    mutable size_t cursor                     = 0;
};

template <typename E>
struct expression_delay<1, E> : expression_with_arguments<E>
{
    using value_type = value_type_of<E>;
    using T          = value_type;
    using expression_with_arguments<E>::expression_with_arguments;

    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_delay& self, cinput_t cinput, size_t index,
                                  vec_shape<T, N>)
    {
        const vec<T, N> in  = self.argument_first(cinput, index, vec_shape<T, N>());
        const vec<T, N> out = insertleft(self.data, in);
        self.data           = in[N - 1];
        return out;
    }
    mutable value_type data = value_type(0);
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
template <size_t samples = 1, typename E1>
KFR_INTRINSIC internal::expression_delay<samples, E1> delay(E1&& e1, csize_t<samples> = csize_t<samples>())
{
    static_assert(samples >= 1 && samples < 1024, "");
    return internal::expression_delay<samples, E1>(std::forward<E1>(e1));
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
