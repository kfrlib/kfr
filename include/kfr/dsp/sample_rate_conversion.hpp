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
} // namespace sample_rate_conversion_quality

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

/// @brief Sample Rate converter
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

    itype input_position_to_intermediate(itype in_pos) const { return in_pos * interpolation_factor; }
    itype output_position_to_intermediate(itype out_pos) const { return out_pos * decimation_factor; }

    itype input_position_to_output(itype in_pos) const
    {
        return floor_div(input_position_to_intermediate(in_pos), decimation_factor).quot;
    }
    itype output_position_to_input(itype out_pos) const
    {
        return floor_div(output_position_to_intermediate(out_pos), interpolation_factor).quot;
    }

    itype output_size_for_input(itype input_size) const
    {
        return input_position_to_output(input_position + input_size - 1) -
               input_position_to_output(input_position - 1);
    }

    itype input_size_for_output(itype output_size) const
    {
        return output_position_to_input(output_position + output_size - 1) -
               output_position_to_input(output_position - 1);
    }

    size_t skip(size_t output_size, univector_ref<const T> input)
    {
        const itype required_input_size = input_size_for_output(output_size);

        if (required_input_size >= depth)
        {
            delay.slice(0, delay.size()) = padded(input.slice(size_t(required_input_size - depth)));
        }
        else
        {
            delay.truncate(size_t(depth - required_input_size)) = delay.slice(size_t(required_input_size));
            delay.slice(size_t(depth - required_input_size))    = padded(input);
        }

        input_position += required_input_size;
        output_position += output_size;

        return required_input_size;
    }

    /// @brief Writes output.size() samples to output reading at most input.size(), then consuming zeros as
    /// input.
    /// @returns Number of processed input samples (may be less than input.size()).
    template <univector_tag Tag>
    size_t process(univector<T, Tag>& output, univector_ref<const T> input)
    {
        const itype required_input_size = input_size_for_output(output.size());

        const itype input_size = input.size();
        for (size_t i = 0; i < output.size(); i++)
        {
            const itype intermediate_index =
                output_position_to_intermediate(static_cast<itype>(i) + output_position);
            const itype intermediate_start = intermediate_index - taps + 1;
            const std::lldiv_t input_pos =
                floor_div(intermediate_start + interpolation_factor - 1, interpolation_factor);
            const itype input_start        = input_pos.quot; // first input sample
            const itype input_end          = input_start + depth;
            const itype tap_start          = interpolation_factor - 1 - input_pos.rem;
            const univector_ref<T> tap_ptr = filter.slice(static_cast<size_t>(tap_start * depth));

            if (input_start >= input_position + input_size)
            {
                output[i] = T(0);
            }
            else if (input_start >= input_position)
            {
                output[i] = dotproduct(input.slice(input_start - input_position, depth), tap_ptr);
            }
            else
            {
                const itype prev_count = input_position - input_start;
                output[i]              = dotproduct(delay.slice(size_t(depth - prev_count)), tap_ptr) +
                            dotproduct(input.slice(0, size_t(depth - prev_count)),
                                       tap_ptr.slice(size_t(prev_count), size_t(depth - prev_count)));
            }
        }

        if (required_input_size >= depth)
        {
            delay.slice(0, delay.size()) = padded(input.slice(size_t(required_input_size - depth)));
        }
        else
        {
            delay.truncate(size_t(depth - required_input_size)) = delay.slice(size_t(required_input_size));
            delay.slice(size_t(depth - required_input_size))    = padded(input);
        }

        input_position += required_input_size;
        output_position += output.size();

        return required_input_size;
    }
    size_t get_delay() const { return depth / 2 * interpolation_factor / decimation_factor; }
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
struct expression_upsample<2, E> : expression_base<E>
{
    using expression_base<E>::expression_base;
    using value_type = value_type_of<E>;
    using T          = value_type;

    size_t size() const noexcept { return expression_base<E>::size() * 2; }

    template <size_t N>
    vec<T, N> operator()(cinput_t cinput, size_t index, vec_t<T, N>) const
    {
        const vec<T, N / 2> x = this->argument_first(cinput, index / 2, vec_t<T, N / 2>());
        return interleave(x, zerovector(x));
    }
    vec<T, 1> operator()(cinput_t cinput, size_t index, vec_t<T, 1>) const
    {
        if (index & 1)
            return 0;
        else
            return this->argument_first(cinput, index / 2, vec_t<T, 1>());
    }
};

template <typename E>
struct expression_upsample<4, E> : expression_base<E>
{
    using expression_base<E>::expression_base;
    using value_type = value_type_of<E>;
    using T          = value_type;

    size_t size() const noexcept { return expression_base<E>::size() * 4; }

    template <size_t N>
    vec<T, N> operator()(cinput_t cinput, size_t index, vec_t<T, N>) const
    {
        const vec<T, N / 4> x  = this->argument_first(cinput, index / 4, vec_t<T, N / 4>());
        const vec<T, N / 2> xx = interleave(x, zerovector(x));
        return interleave(xx, zerovector(xx));
    }
    vec<T, 2> operator()(cinput_t cinput, size_t index, vec_t<T, 2>) const
    {
        switch (index & 3)
        {
        case 0:
            return interleave(this->argument_first(cinput, index / 4, vec_t<T, 1>()), zerovector<T, 1>());
        case 3:
            return interleave(zerovector<T, 1>(), this->argument_first(cinput, index / 4, vec_t<T, 1>()));
        default:
            return 0;
        }
    }
    vec<T, 1> operator()(cinput_t cinput, size_t index, vec_t<T, 1>) const
    {
        if (index & 3)
            return 0;
        else
            return this->argument_first(cinput, index / 4, vec_t<T, 1>());
    }
};

template <typename E, size_t offset>
struct expression_downsample<2, offset, E> : expression_base<E>
{
    using expression_base<E>::expression_base;
    using value_type = value_type_of<E>;
    using T          = value_type;

    size_t size() const noexcept { return expression_base<E>::size() / 2; }

    template <size_t N>
    vec<T, N> operator()(cinput_t cinput, size_t index, vec_t<T, N>) const
    {
        const vec<T, N* 2> x = this->argument_first(cinput, index * 2, vec_t<T, N * 2>());
        return x.shuffle(csizeseq_t<N, offset, 2>());
    }
};

template <typename E, size_t offset>
struct expression_downsample<4, offset, E> : expression_base<E>
{
    using expression_base<E>::expression_base;
    using value_type = value_type_of<E>;
    using T          = value_type;

    size_t size() const noexcept { return expression_base<E>::size() / 4; }

    template <size_t N>
    vec<T, N> operator()(cinput_t cinput, size_t index, vec_t<T, N>) const
    {
        const vec<T, N* 4> x = this->argument_first(cinput, index * 4, vec_t<T, N * 4>());
        return x.shuffle(csizeseq_t<N, offset, 4>());
    }
};
} // namespace internal

template <typename E1, size_t offset = 0>
CMT_INLINE internal::expression_downsample<2, offset, E1> downsample2(E1&& e1, csize_t<offset> = csize_t<0>())
{
    return internal::expression_downsample<2, offset, E1>(std::forward<E1>(e1));
}

template <typename E1, size_t offset = 0>
CMT_INLINE internal::expression_downsample<4, offset, E1> downsample4(E1&& e1, csize_t<offset> = csize_t<0>())
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
} // namespace kfr
