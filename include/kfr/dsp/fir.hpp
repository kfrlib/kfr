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

#include "../base/basic_expressions.hpp"
#include "../base/memory.hpp"
#include "../base/reduce.hpp"
#include "../base/sin_cos.hpp"
#include "../base/univector.hpp"
#include "../base/vec.hpp"
#include "window.hpp"

namespace kfr
{

template <typename T, size_t Size>
using fir_taps = univector<T, Size>;

namespace internal
{
template <size_t tapcount, typename T, typename E1, KFR_ARCH_DEP>
struct expression_short_fir : expression<E1>
{
    static_assert(is_poweroftwo(tapcount), "tapcount must be a power of two");

    expression_short_fir(E1&& e1, const array_ref<T>& taps)
        : expression<E1>(std::forward<E1>(e1)), taps(taps), delayline(0)
    {
    }
    expression_short_fir(E1&& e1, const array_ref<const T>& taps)
        : expression<E1>(std::forward<E1>(e1)), taps(taps), delayline(0)
    {
    }
    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N> x) const
    {
        vec<T, N> in = cast<T>(this->argument_first(index, x));

        vec<T, N> out = in * taps[0];
        cfor(csize<1>, csize<tapcount>,
             [&](auto I) { out = out + concat_and_slice<tapcount - 1 - I, N>(delayline, in) * taps[I]; });
        delayline = concat_and_slice<N, tapcount - 1>(delayline, in);

        return cast<U>(out);
    }
    vec<T, tapcount> taps;
    mutable vec<T, tapcount - 1> delayline;
};

template <typename T, typename E1, KFR_ARCH_DEP>
struct expression_fir : expression<E1>
{
    expression_fir(E1&& e1, const array_ref<const T>& taps)
        : expression<E1>(std::forward<E1>(e1)), taps(taps), delayline(taps.size(), T()), delayline_cursor(0)
    {
    }
    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N> x) const
    {
        const size_t tapcount = taps.size();
        const vec<T, N> input = cast<T>(this->argument_first(index, x));

        vec<T, N> output;
        size_t cursor = delayline_cursor;
        CMT_LOOP_NOUNROLL
        for (size_t i = 0; i < N; i++)
        {
            delayline.ringbuf_write(cursor, input[i]);
            output(i) = dotproduct(taps, delayline.slice(cursor) /*, tapcount - cursor*/) +
                        dotproduct(taps.slice(tapcount - cursor), delayline /*, cursor*/);
        }
        delayline_cursor = cursor;
        return cast<U>(output);
    }
    univector_dyn<T> taps;
    mutable univector_dyn<T> delayline;
    mutable size_t delayline_cursor;
};
}

template <typename T, typename E1, size_t Tag>
CMT_INLINE internal::expression_fir<T, E1> fir(E1&& e1, const univector<T, Tag>& taps)
{
    return internal::expression_fir<T, E1>(std::forward<E1>(e1), taps.ref());
}
template <typename T, size_t TapCount, typename E1>
CMT_INLINE internal::expression_short_fir<TapCount, T, E1> short_fir(E1&& e1,
                                                                     const univector<T, TapCount>& taps)
{
    static_assert(TapCount >= 1 && TapCount <= 32, "Use short_fir only for small FIR filters");
    return internal::expression_short_fir<TapCount, T, E1>(std::forward<E1>(e1), taps.ref());
}
}
