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

#include "../base/memory.hpp"
#include "../base/sin_cos.hpp"
#include "../base/vec.hpp"
#include "../expressions/basic.hpp"
#include "../expressions/operators.hpp"
#include "../expressions/reduce.hpp"
#include "window.hpp"

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

template <typename T, size_t Size>
using fir_taps = univector<T, Size>;

namespace internal
{
template <cpu_t cpu = cpu_t::native>
struct in_fir : in_sqrt<cpu>, in_abs<cpu>, in_log_exp<cpu>, in_sin_cos<cpu>, in_window<cpu>, in_reduce<cpu>
{
private:
    using in_sqrt<cpu>::sqrt;
    using in_abs<cpu>::abs;
    using in_log_exp<cpu>::log;
    using in_log_exp<cpu>::exp;
    using in_log_exp<cpu>::log_fmadd;
    using in_log_exp<cpu>::exp_fmadd;
    using in_log_exp<cpu>::exp10;
    using typename in_sin_cos<cpu>::fn_sinc;
    using in_reduce<cpu>::reduce;
    using in_reduce<cpu>::dotproduct;
    using in_reduce<cpu>::sum;

public:
    template <typename T>
    KFR_SINTRIN void fir_lowpass(univector_ref<T> taps, T cutoff, const expression_pointer<T>& window,
                                 bool normalize = true)
    {
        const T scale = 2.0 * cutoff;
        taps          = bind_expression(fn_sinc(), symmlinspace<T, true>((taps.size() - 1) * cutoff * c_pi<T>,
                                                                taps.size(), true)) *
               scale * window;

        if (is_odd(taps.size()))
            taps[taps.size() / 2] = scale;

        if (normalize)
        {
            const T invsum = reciprocal(sum(taps));
            taps           = taps * invsum;
        }
    }
    template <typename T>
    KFR_SINTRIN void fir_highpass(univector_ref<T> taps, T cutoff, const expression_pointer<T>& window,
                                  bool normalize = true)
    {
        const T scale = 2.0 * -cutoff;
        taps          = bind_expression(fn_sinc(), symmlinspace<T, true>((taps.size() - 1) * cutoff * c_pi<T>,
                                                                taps.size(), true)) *
               scale * window;

        if (is_odd(taps.size()))
            taps[taps.size() / 2] = 1 - 2.0 * cutoff;

        if (normalize)
        {
            const T invsum = reciprocal(sum(taps) + 1);
            taps           = taps * invsum;
        }
    }

    template <typename T>
    KFR_SINTRIN void fir_bandpass(univector_ref<T> taps, T frequency1, T frequency2,
                                  const expression_pointer<T>& window, bool normalize = true)
    {
        const T scale1 = 2.0 * frequency1;
        const T scale2 = 2.0 * frequency2;
        const T sc     = c_pi<T> * T(taps.size() - 1);
        const T start1 = sc * frequency1;
        const T start2 = sc * frequency2;

        taps = (bind_expression(fn_sinc(), symmlinspace<T, true>(start2, taps.size(), true)) * scale2 -
                bind_expression(fn_sinc(), symmlinspace<T, true>(start1, taps.size(), true)) * scale1) *
               window;

        if (is_odd(taps.size()))
            taps[taps.size() / 2] = 2 * (frequency2 - frequency1);

        if (normalize)
        {
            const T invsum = reciprocal(sum(taps) + 1);
            taps           = taps * invsum;
        }
    }

    template <typename T>
    KFR_SINTRIN void fir_bandstop(univector_ref<T> taps, T frequency1, T frequency2,
                                  const expression_pointer<T>& window, bool normalize = true)
    {
        const T scale1 = 2.0 * frequency1;
        const T scale2 = 2.0 * frequency2;
        const T sc     = c_pi<T> * T(taps.size() - 1);
        const T start1 = sc * frequency1;
        const T start2 = sc * frequency2;

        taps = (bind_expression(fn_sinc(), symmlinspace<T, true>(start1, taps.size(), true)) * scale1 -
                bind_expression(fn_sinc(), symmlinspace<T, true>(start2, taps.size(), true)) * scale2) *
               window;

        if (is_odd(taps.size()))
            taps[taps.size() / 2] = 1 - 2 * (frequency2 - frequency1);

        if (normalize)
        {
            const T invsum = reciprocal(sum(taps));
            taps           = taps * invsum;
        }
    }

    template <size_t index, size_t order, typename T, size_t N>
    KFR_SINTRIN void convole_round(vec<T, N>& output, vec<T, order> input, vec<T, order> taps,
                                   vec<T, order> delay)
    {
        output(index) = dot(taps, rotatetwo<index + 1>(delay, input));
    }

    template <size_t index, size_t order, typename T, size_t N, KFR_ENABLE_IF(index >= N)>
    KFR_SINTRIN void convole_rounds(vec<T, N>& /*output*/, vec<T, order> /*input*/, vec<T, order> /*taps*/,
                                    vec<T, order> /*delay*/)
    {
    }

    template <size_t index, size_t order, typename T, size_t N, KFR_ENABLE_IF(index < N)>
    KFR_SINTRIN void convole_rounds(vec<T, N>& output, vec<T, order> input, vec<T, order> taps,
                                    vec<T, order> delay)
    {
        convole_round<index, order, T, N>(output, input, taps, delay);
        convole_rounds<index + 1, order, T, N>(output, input, taps, delay);
    }

    template <size_t tapcount, typename T, typename E1>
    struct expression_short_fir : expression<E1>
    {
        static_assert(is_poweroftwo(tapcount), "tapcount must be a power of two");
        template <cpu_t newcpu>
        using retarget_this =
            typename in_fir<newcpu>::template expression_short_fir<tapcount, T, retarget<E1, newcpu>>;

        expression_short_fir(E1&& e1, const array_ref<T>& taps)
            : expression<E1>(std::forward<E1>(e1)), taps(taps)
        {
        }
        template <typename U, size_t N>
        KFR_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N> x)
        {
            const vec<T, N> in = cast<T>(this->argument_first(index, x));

            vec<T, N> out;
            vec<T, tapcount> winput = widen<tapcount>(in);
            winput = reverse(winput);
            convole_rounds<0, tapcount, T, N>(out, winput, taps, delayline);
            delayline = rotatetwo<N>(delayline, winput);

            return cast<U>(out);
        }
        const vec<T, tapcount> taps;
        vec<T, tapcount> delayline;
    };

    template <typename T, typename E1>
    struct expression_fir : expression<E1>
    {
        template <cpu_t newcpu>
        using retarget_this = typename in_fir<newcpu>::template expression_fir<T, retarget<E1, newcpu>>;

        expression_fir(E1&& e1, const array_ref<const T>& taps)
            : expression<E1>(std::forward<E1>(e1)), taps(taps), delayline(taps.size(), T()),
              delayline_cursor(0)
        {
        }
        template <typename U, size_t N>
        KFR_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N> x)
        {
            const size_t tapcount = taps.size();
            const vec<T, N> input = cast<T>(this->argument_first(index, x));

            vec<T, N> output;
            size_t cursor = delayline_cursor;
            KFR_LOOP_NOUNROLL
            for (size_t i = 0; i < N; i++)
            {
                delayline.ringbuf_write(cursor, input[i]);
                output(i) = dotproduct(taps, delayline.slice(cursor) /*, tapcount - cursor*/) +
                            dotproduct(taps.slice(tapcount - cursor), delayline /*, cursor*/);
            }
            delayline_cursor = cursor;
            return cast<U>(output);
        }
        const univector_dyn<T> taps;
        univector_dyn<T> delayline;
        size_t delayline_cursor;
    };
    KFR_SPEC_FN(in_fir, fir_lowpass)
    KFR_SPEC_FN(in_fir, fir_highpass)
    KFR_SPEC_FN(in_fir, fir_bandpass)
    KFR_SPEC_FN(in_fir, fir_bandstop)
};
}

namespace native
{
template <typename T, size_t Tag>
KFR_INLINE void fir_lowpass(univector<T, Tag>& taps, identity<T> cutoff, const expression_pointer<T>& window,
                            bool normalize = true)
{
    return internal::in_fir<>::fir_lowpass(taps.slice(), cutoff, window, normalize);
}
template <typename T, size_t Tag>
KFR_INLINE void fir_highpass(univector<T, Tag>& taps, identity<T> cutoff, const expression_pointer<T>& window,
                             bool normalize = true)
{
    return internal::in_fir<>::fir_highpass(taps.slice(), cutoff, window, normalize);
}
template <typename T, size_t Tag>
KFR_INLINE void fir_bandpass(univector<T, Tag>& taps, identity<T> frequency1, identity<T> frequency2,
                             const expression_pointer<T>& window, bool normalize = true)
{
    return internal::in_fir<>::fir_bandpass(taps.slice(), frequency1, frequency2, window, normalize);
}
template <typename T, size_t Tag>
KFR_INLINE void fir_bandstop(univector<T, Tag>& taps, identity<T> frequency1, identity<T> frequency2,
                             const expression_pointer<T>& window, bool normalize = true)
{
    return internal::in_fir<>::fir_bandstop(taps.slice(), frequency1, frequency2, window, normalize);
}

template <typename T, typename E1, size_t Tag>
KFR_INLINE internal::in_fir<>::expression_fir<T, E1> fir(E1&& e1, const univector<T, Tag>& taps)
{
    return internal::in_fir<>::expression_fir<T, E1>(std::forward<E1>(e1), taps.ref());
}
template <typename T, size_t TapCount, typename E1>
KFR_INLINE internal::in_fir<>::expression_short_fir<TapCount, T, E1> short_fir(
    E1&& e1, const univector<T, TapCount>& taps)
{
    static_assert(TapCount >= 1 && TapCount < 16, "Use short_fir only for small FIR filters");
    return internal::in_fir<>::expression_short_fir<TapCount, T, E1>(std::forward<E1>(e1), taps.ref());
}
}
}

#pragma clang diagnostic pop
