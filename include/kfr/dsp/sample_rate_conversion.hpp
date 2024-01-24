/** @addtogroup dsp
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

#include "../base/memory.hpp"
#include "../base/reduce.hpp"
#include "../base/univector.hpp"
#include "../math/modzerobessel.hpp"
#include "../math/sqrt.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/vec.hpp"
#include "window.hpp"

namespace kfr
{

enum class sample_rate_conversion_quality : int
{
    draft   = 4,
    low     = 6,
    normal  = 8,
    high    = 10,
    perfect = 12,
};

using resample_quality = sample_rate_conversion_quality;

/// @brief Sample Rate converter
template <typename T>
struct samplerate_converter
{
    using itype = i64;
    using ftype = subtype<T>;

protected:
    KFR_MEM_INTRINSIC ftype window(ftype n) const
    {
        return modzerobessel(kaiser_beta * sqrt(1 - sqr(2 * n - 1))) * reciprocal(modzerobessel(kaiser_beta));
    }
    KFR_MEM_INTRINSIC ftype sidelobe_att() const { return static_cast<ftype>(kaiser_beta / 0.1102 + 8.7); }
    KFR_MEM_INTRINSIC ftype transition_width() const
    {
        return static_cast<ftype>((sidelobe_att() - 8) / (depth - 1) / 2.285);
    }

public:
    static KFR_MEM_INTRINSIC size_t filter_order(sample_rate_conversion_quality quality)
    {
        return size_t(1) << (static_cast<int>(quality) + 1);
    }

    /// @brief Returns sidelobe attenuation for the given quality (in dB)
    static KFR_MEM_INTRINSIC ftype sidelobe_attenuation(sample_rate_conversion_quality quality)
    {
        return (static_cast<int>(quality) - 3) * ftype(20);
    }

    /// @brief Returns transition width for the given quality (in rad)
    static KFR_MEM_INTRINSIC ftype transition_width(sample_rate_conversion_quality quality)
    {
        return (sidelobe_attenuation(quality) - 8) / (filter_order(quality) - 1) / ftype(2.285);
    }

    static KFR_MEM_INTRINSIC ftype window_param(sample_rate_conversion_quality quality)
    {
        const ftype att = sidelobe_attenuation(quality);
        if (att > 50)
            return ftype(0.1102) * (att - ftype(8.7));
        if (att >= 21)
            return ftype(0.5842) * pow(att - 21, ftype(0.4)) + ftype(0.07886) * (att - 21);
        return 0;
    }

    samplerate_converter(sample_rate_conversion_quality quality, itype interpolation_factor,
                         itype decimation_factor, ftype scale = ftype(1), ftype cutoff = 0.5f);

    KFR_MEM_INTRINSIC itype input_position_to_intermediate(itype in_pos) const
    {
        return in_pos * interpolation_factor;
    }
    KFR_MEM_INTRINSIC itype output_position_to_intermediate(itype out_pos) const
    {
        return out_pos * decimation_factor;
    }

    KFR_MEM_INTRINSIC itype input_position_to_output(itype in_pos) const
    {
        return floor_div(input_position_to_intermediate(in_pos), decimation_factor).quot;
    }
    KFR_MEM_INTRINSIC itype output_position_to_input(itype out_pos) const
    {
        return floor_div(output_position_to_intermediate(out_pos), interpolation_factor).quot;
    }

    KFR_MEM_INTRINSIC itype output_size_for_input(itype input_size) const
    {
        return input_position_to_output(input_position + input_size - 1) -
               input_position_to_output(input_position - 1);
    }

    KFR_MEM_INTRINSIC itype input_size_for_output(itype output_size) const
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
        return process_impl(output.slice(), input);
    }

    KFR_MEM_INTRINSIC double get_fractional_delay() const { return (taps - 1) * 0.5 / decimation_factor; }
    KFR_MEM_INTRINSIC size_t get_delay() const { return static_cast<size_t>(get_fractional_delay()); }

    ftype kaiser_beta;
    itype depth;
    itype taps;
    size_t order;
    itype interpolation_factor;
    itype decimation_factor;
    univector<T> filter;
    univector<T> delay;

protected:
    itype input_position;
    itype output_position;

    size_t process_impl(univector_ref<T> output, univector_ref<const T> input);
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{

template <size_t factor, typename E>
struct expression_upsample;

template <size_t factor, size_t offset, typename E>
struct expression_downsample;

template <typename E>
struct expression_upsample<2, E> : expression_with_arguments<E>, expression_traits_defaults
{
    using expression_with_arguments<E>::expression_with_arguments;
    using value_type = expression_value_type<E>;
    using T          = value_type;

    KFR_MEM_INTRINSIC size_t size() const CMT_NOEXCEPT { return expression_with_arguments<E>::size() * 2; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, N>)
    {
        const vec<T, N / 2> x = get_elements(self.first(), index / 2, axis_params<0, N / 2>());
        return interleave(x, zerovector(x));
    }
    KFR_INTRINSIC friend vec<T, 1> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, 1>)
    {
        if (index & 1)
            return 0;
        else
            return get_elements(self.first(), index / 2, axis_params<0, 1>());
    }
};

template <typename E>
struct expression_upsample<4, E> : expression_with_arguments<E>
{
    using expression_with_arguments<E>::expression_with_arguments;
    using value_type = expression_value_type<E>;
    using T          = value_type;

    KFR_MEM_INTRINSIC size_t size() const CMT_NOEXCEPT { return expression_with_arguments<E>::size() * 4; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, N>) CMT_NOEXCEPT
    {
        const vec<T, N / 4> x  = get_elements(self.first(), index / 4, axis_params<0, N / 4>());
        const vec<T, N / 2> xx = interleave(x, zerovector(x));
        return interleave(xx, zerovector(xx));
    }
    KFR_INTRINSIC friend vec<T, 2> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, 2>) CMT_NOEXCEPT
    {
        switch (index & 3)
        {
        case 0:
            return interleave(get_elements(self.first(), index / 4, axis_params<0, 1>()), zerovector<T, 1>());
        case 3:
            return interleave(zerovector<T, 1>(), get_elements(self.first(), index / 4, axis_params<0, 1>()));
        default:
            return 0;
        }
    }
    KFR_INTRINSIC friend vec<T, 1> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, 1>) CMT_NOEXCEPT
    {
        if (index & 3)
            return 0;
        else
            return get_elements(self.first(), index / 4, axis_params<0, 1>());
    }
};

template <typename E, size_t offset>
struct expression_downsample<2, offset, E> : expression_with_arguments<E>
{
    using expression_with_arguments<E>::expression_with_arguments;
    using value_type = expression_value_type<E>;
    using T          = value_type;

    KFR_MEM_INTRINSIC size_t size() const CMT_NOEXCEPT { return expression_with_arguments<E>::size() / 2; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_downsample& self, size_t index,
                                                axis_params<0, N>) CMT_NOEXCEPT
    {
        const vec<T, N * 2> x = get_elements(self.first(), index * 2, axis_params<0, N * 2>());
        return x.shuffle(csizeseq<N, offset, 2>);
    }
};

template <typename E, size_t offset>
struct expression_downsample<4, offset, E> : expression_with_arguments<E>
{
    using expression_with_arguments<E>::expression_with_arguments;
    using value_type = expression_value_type<E>;
    using T          = value_type;

    KFR_MEM_INTRINSIC size_t size() const CMT_NOEXCEPT { return expression_with_arguments<E>::size() / 4; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_downsample& self, index_t index,
                                                axis_params<0, N>) CMT_NOEXCEPT
    {
        const vec<T, N * 4> x = get_elements(self.first(), index * 4, axis_params<0, N * 4>());
        return x.shuffle(csizeseq<N, offset, 4>);
    }
};
} // namespace internal

template <typename E1, size_t offset = 0>
KFR_FUNCTION internal::expression_downsample<2, offset, E1> downsample2(E1&& e1,
                                                                        csize_t<offset> = csize_t<0>())
{
    return internal::expression_downsample<2, offset, E1>(std::forward<E1>(e1));
}

template <typename E1, size_t offset = 0>
KFR_FUNCTION internal::expression_downsample<4, offset, E1> downsample4(E1&& e1,
                                                                        csize_t<offset> = csize_t<0>())
{
    return internal::expression_downsample<4, offset, E1>(std::forward<E1>(e1));
}

template <typename E1>
KFR_FUNCTION internal::expression_upsample<2, E1> upsample2(E1&& e1)
{
    return internal::expression_upsample<2, E1>(std::forward<E1>(e1));
}

template <typename E1>
KFR_FUNCTION internal::expression_upsample<4, E1> upsample4(E1&& e1)
{
    return internal::expression_upsample<4, E1>(std::forward<E1>(e1));
}

template <typename T = fbase>
KFR_FUNCTION samplerate_converter<T> sample_rate_converter(sample_rate_conversion_quality quality,
                                                           size_t interpolation_factor,
                                                           size_t decimation_factor,
                                                           subtype<T> scale  = subtype<T>(1),
                                                           subtype<T> cutoff = 0.5f)
{
    using itype = typename samplerate_converter<T>::itype;
    return samplerate_converter<T>(quality, itype(interpolation_factor), itype(decimation_factor), scale,
                                   cutoff);
}

// Deprecated in 0.9.2
template <typename T = fbase>
KFR_FUNCTION samplerate_converter<T> resampler(sample_rate_conversion_quality quality,
                                               size_t interpolation_factor, size_t decimation_factor,
                                               subtype<T> scale = subtype<T>(1), subtype<T> cutoff = 0.5f)
{
    using itype = typename samplerate_converter<T>::itype;
    return samplerate_converter<T>(quality, itype(interpolation_factor), itype(decimation_factor), scale,
                                   cutoff);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
