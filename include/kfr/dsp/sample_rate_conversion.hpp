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

#include "../base/function.hpp"
#include "../base/memory.hpp"
#include "../base/reduce.hpp"
#include "../base/vec.hpp"
#include "window.hpp"

namespace kfr
{
namespace sample_rate_conversion_quality
{
constexpr csize_t<4> draft{};
constexpr csize_t<6> low{};
constexpr csize_t<8> normal{};
constexpr csize_t<10> high{};
}

namespace resample_quality = sample_rate_conversion_quality;

namespace internal
{
template <typename T1, typename T2>
KFR_SINTRIN T1 sample_rate_converter_blackman(T1 n, T2 a)
{
    const T1 a0 = (1 - a) * 0.5;
    const T1 a1 = 0.5;
    const T1 a2 = a * 0.5;
    n           = n * c_pi<T1, 2>;
    return a0 - a1 * cos(n) + a2 * cos(2 * n);
}

template <typename T, size_t quality, KFR_ARCH_DEP>
struct sample_rate_converter
{
    using itype = i64;

    constexpr static itype depth = static_cast<itype>(1 << (quality + 1));

    sample_rate_converter(itype interpolation_factor, itype decimation_factor, T scale = T(1),
                          T cutoff = 0.49)
        : input_position(0), output_position(0)
    {
        const i64 gcf = gcd(interpolation_factor, decimation_factor);
        interpolation_factor /= gcf;
        decimation_factor /= gcf;

        taps  = depth * interpolation_factor;
        order = size_t(depth * interpolation_factor - 1);

        this->interpolation_factor = interpolation_factor;
        this->decimation_factor    = decimation_factor;

        const itype halftaps = taps / 2;
        filter               = univector<T>(size_t(taps), T());
        delay                = univector<T>(size_t(depth), T());

        cutoff = cutoff / std::max(decimation_factor, interpolation_factor);

        for (itype j = 0, jj = 0; j < taps; j++)
        {
            filter[size_t(j)] = scale * 2 * interpolation_factor * cutoff *
                                sinc((jj - halftaps) * cutoff * c_pi<T, 2>) *
                                sample_rate_converter_blackman(T(jj) / T(taps - 1), T(0.16));
            jj += size_t(interpolation_factor);
            if (jj >= taps)
                jj = jj - taps + 1;
        }

        const T s = reciprocal(sum(filter)) * interpolation_factor;
        filter    = filter * s;
    }
    CMT_INLINE size_t operator()(T* dest, size_t zerosize)
    {
        size_t outputsize   = 0;
        const itype srcsize = itype(zerosize);

        for (size_t i = 0;; i++)
        {
            const itype ii                 = itype(i) + output_position;
            const itype workindex          = ii * (decimation_factor);
            const itype workindex_rem      = workindex % (interpolation_factor);
            const itype start              = workindex_rem ? (interpolation_factor)-workindex_rem : 0;
            itype srcindex                 = workindex / (interpolation_factor);
            srcindex                       = workindex_rem ? srcindex + 1 : srcindex;
            const univector_ref<T> tap_ptr = filter.slice(static_cast<size_t>(start * depth));
            srcindex                       = srcindex - (depth - 1);

            if (srcindex + depth >= input_position + srcsize)
                break;
            outputsize++;

            if (dest)
            {
                if (srcindex >= input_position)
                {
                    dest[i] = T(0);
                }
                else
                {
                    const itype prev_count = input_position - srcindex;
                    dest[i]                = dotproduct(delay.slice(size_t(depth - prev_count)), tap_ptr);
                }
            }
        }
        if (srcsize >= depth)
        {
            delay = zeros();
        }
        else
        {
            delay.truncate(size_t(depth - srcsize)) = delay.slice(size_t(srcsize));
            delay.slice(size_t(depth - srcsize))    = zeros();
        }

        input_position += srcsize;
        output_position += outputsize;
        return outputsize;
    }
    CMT_INLINE size_t operator()(T* dest, univector_ref<const T> src)
    {
        size_t outputsize   = 0;
        const itype srcsize = itype(src.size());

        for (size_t i = 0;; i++)
        {
            const itype ii                 = itype(i) + output_position;
            const itype workindex          = ii * (decimation_factor);
            const itype workindex_rem      = workindex % (interpolation_factor);
            const itype start              = workindex_rem ? (interpolation_factor)-workindex_rem : 0;
            itype srcindex                 = workindex / (interpolation_factor);
            srcindex                       = workindex_rem ? srcindex + 1 : srcindex;
            const univector_ref<T> tap_ptr = filter.slice(static_cast<size_t>(start * depth));
            srcindex                       = srcindex - (depth - 1);

            if (srcindex + depth >= input_position + srcsize)
                break;
            outputsize++;

            if (dest)
            {
                if (srcindex >= input_position)
                {
                    dest[i] =
                        dotproduct(src.slice(size_t(srcindex - input_position), size_t(depth)), tap_ptr);
                }
                else
                {
                    const itype prev_count = input_position - srcindex;
                    dest[i]                = dotproduct(delay.slice(size_t(depth - prev_count)), tap_ptr) +
                              dotproduct(src, tap_ptr.slice(size_t(prev_count), size_t(depth - prev_count)));
                }
            }
        }
        if (srcsize >= depth)
        {
            delay = src.slice(size_t(srcsize - depth));
        }
        else
        {
            delay.truncate(size_t(depth - srcsize)) = delay.slice(size_t(srcsize));
            delay.slice(size_t(depth - srcsize))    = src;
        }

        input_position += srcsize;
        output_position += outputsize;
        return outputsize;
    }
    itype taps;
    size_t order;
    itype interpolation_factor;
    itype decimation_factor;
    univector<T> filter;
    univector<T> delay;
    itype input_position;
    itype output_position;
};

template <size_t factor, typename E>
struct expression_upsample;

template <size_t factor, size_t offset, typename E>
struct expression_downsample;

template <typename E>
struct expression_upsample<2, E> : expression<E>
{
    using expression<E>::expression;
    using value_type = value_type_of<E>;
    using T          = value_type;

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        const vec<T, N / 2> x = this->argument_first(index / 2, vec_t<T, N / 2>());
        return interleave(x, zerovector(x));
    }
    vec<T, 1> operator()(cinput_t, size_t index, vec_t<T, 1>) const
    {
        if (index & 1)
            return 0;
        else
            return this->argument_first(index / 2, vec_t<T, 1>());
    }
};

template <typename E>
struct expression_upsample<4, E> : expression<E>
{
    using expression<E>::expression;
    using value_type = value_type_of<E>;
    using T          = value_type;

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        const vec<T, N / 4> x  = this->argument_first(index / 4, vec_t<T, N / 4>());
        const vec<T, N / 2> xx = interleave(x, zerovector(x));
        return interleave(xx, zerovector(xx));
    }
    vec<T, 2> operator()(cinput_t, size_t index, vec_t<T, 2>) const
    {
        switch (index & 3)
        {
        case 0:
            return interleave(this->argument_first(index / 4, vec_t<T, 1>()), zerovector<T, 1>());
        case 3:
            return interleave(zerovector<T, 1>(), this->argument_first(index / 4, vec_t<T, 1>()));
        default:
            return 0;
        }
    }
    template <typename T>
    vec<T, 1> operator()(cinput_t, size_t index, vec_t<T, 1>) const
    {
        if (index & 3)
            return 0;
        else
            return this->argument_first(index / 4, vec_t<T, 1>());
    }
};

template <typename E, size_t offset>
struct expression_downsample<2, offset, E> : expression<E>
{
    using expression<E>::expression;
    using value_type = value_type_of<E>;
    using T          = value_type;

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        const vec<T, N* 2> x = this->argument_first(index * 2, vec_t<T, N * 2>());
        return shufflevector<N, internal::shuffle_index<offset, 2>>(x);
    }
};

template <typename E, size_t offset>
struct expression_downsample<4, offset, E> : expression<E>
{
    using expression<E>::expression;
    using value_type = value_type_of<E>;
    using T          = value_type;

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        const vec<T, N* 4> x = this->argument_first(index * 4, vec_t<T, N * 4>());
        return shufflevector<N, internal::shuffle_index<offset, 4>>(x);
    }
};
}

template <typename E1, size_t offset = 0>
CMT_INLINE internal::expression_downsample<2, offset, E1> downsample2(E1&& e1, csize_t<offset> = csize<0>)
{
    return internal::expression_downsample<2, offset, E1>(std::forward<E1>(e1));
}

template <typename E1, size_t offset = 0>
CMT_INLINE internal::expression_downsample<4, offset, E1> downsample4(E1&& e1, csize_t<offset> = csize<0>)
{
    return internal::expression_downsample<4, offset, E1>(std::forward<E1>(e1));
}

template <typename E1>
CMT_INLINE internal::expression_upsample<2, E1> upsample2(E1&& e1)
{
    return internal::expression_upsample<2, E1>(std::forward<E1>(e1));
}

template <typename E1>
CMT_INLINE internal::expression_upsample<4, E1> upsample4(E1&& e1)
{
    return internal::expression_upsample<4, E1>(std::forward<E1>(e1));
}

template <typename T = fbase, size_t quality>
inline internal::sample_rate_converter<T, quality> sample_rate_converter(csize_t<quality>,
                                                                         size_t interpolation_factor,
                                                                         size_t decimation_factor,
                                                                         T scale = T(1), T cutoff = 0.49)
{
    using itype = typename internal::sample_rate_converter<T, quality>::itype;
    return internal::sample_rate_converter<T, quality>(itype(interpolation_factor), itype(decimation_factor),
                                                       scale, cutoff);
}

// Deprecated in 0.9.2
template <typename T = fbase, size_t quality>
inline internal::sample_rate_converter<T, quality> resampler(csize_t<quality>, size_t interpolation_factor,
                                                             size_t decimation_factor, T scale = T(1),
                                                             T cutoff = 0.49)
{
    using itype = typename internal::sample_rate_converter<T, quality>::itype;
    return internal::sample_rate_converter<T, quality>(itype(interpolation_factor), itype(decimation_factor),
                                                       scale, cutoff);
}
}
