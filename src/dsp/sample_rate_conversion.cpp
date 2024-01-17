/** @addtogroup dft
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
#include <kfr/dsp/sample_rate_conversion.hpp>
#include <kfr/multiarch.h>

namespace kfr
{
CMT_MULTI_PROTO(namespace impl {
    template <typename T>
    struct samplerate_converter : public kfr::samplerate_converter<T>
    {
    public:
        using itype = typename kfr::samplerate_converter<T>::itype;
        using ftype = typename kfr::samplerate_converter<T>::ftype;
        void init(sample_rate_conversion_quality quality, itype interpolation_factor, itype decimation_factor,
                  subtype<T> scale, subtype<T> cutoff);
        size_t process_impl(univector_ref<T> output, univector_ref<const T> input);
    };
} // namespace impl
)

inline namespace CMT_ARCH_NAME
{
namespace impl
{

template <typename T>
void samplerate_converter<T>::init(sample_rate_conversion_quality quality, itype interpolation_factor,
                                   itype decimation_factor, subtype<T> scale, subtype<T> cutoff)
{
    this->kaiser_beta     = this->window_param(quality);
    this->depth           = static_cast<itype>(this->filter_order(quality));
    this->input_position  = 0;
    this->output_position = 0;

    const i64 gcf = gcd(interpolation_factor, decimation_factor);
    interpolation_factor /= gcf;
    decimation_factor /= gcf;

    this->taps  = this->depth * interpolation_factor;
    this->order = size_t(this->depth * interpolation_factor - 1);

    this->interpolation_factor = interpolation_factor;
    this->decimation_factor    = decimation_factor;

    const itype halftaps = this->taps / 2;
    this->filter         = univector<T>(size_t(this->taps), T());
    this->delay          = univector<T>(size_t(this->depth), T());

    cutoff = cutoff - this->transition_width() / c_pi<ftype, 4>;

    cutoff = cutoff / std::max(decimation_factor, interpolation_factor);

    for (itype j = 0, jj = 0; j < this->taps; j++)
    {
        this->filter[size_t(j)] =
            sinc((jj - halftaps) * cutoff * c_pi<ftype, 2>) * this->window(ftype(jj) / ftype(this->taps - 1));
        jj += size_t(interpolation_factor);
        if (jj >= this->taps)
            jj = jj - this->taps + 1;
    }

    const T s    = reciprocal(sum(this->filter)) * static_cast<ftype>(interpolation_factor * scale);
    this->filter = this->filter * s;
}

template <typename T>
size_t samplerate_converter<T>::process_impl(univector_ref<T> output, univector_ref<const T> input)
{
    const itype required_input_size = this->input_size_for_output(output.size());

    const itype input_size = input.size();
    for (size_t i = 0; i < output.size(); i++)
    {
        const itype intermediate_index =
            this->output_position_to_intermediate(static_cast<itype>(i) + this->output_position);
        const itype intermediate_start = intermediate_index - this->taps + 1;
        const std::lldiv_t input_pos =
            floor_div(intermediate_start + this->interpolation_factor - 1, this->interpolation_factor);
        const itype input_start        = input_pos.quot; // first input sample
        const itype tap_start          = this->interpolation_factor - 1 - input_pos.rem;
        const univector_ref<T> tap_ptr = this->filter.slice(static_cast<size_t>(tap_start * this->depth));

        if (input_start >= this->input_position + input_size)
        {
            output[i] = T(0);
        }
        else if (input_start >= this->input_position)
        {
            output[i] = dotproduct(
                truncate(padded(input.slice(input_start - this->input_position, this->depth)), this->depth),
                tap_ptr.truncate(this->depth));
        }
        else
        {
            const itype prev_count = this->input_position - input_start;
            output[i]              = dotproduct(this->delay.slice(size_t(this->depth - prev_count)),
                                                tap_ptr.truncate(prev_count)) +
                        dotproduct(truncate(padded(input.truncate(size_t(this->depth - prev_count))),
                                            size_t(this->depth - prev_count)),
                                   tap_ptr.slice(size_t(prev_count), size_t(this->depth - prev_count)));
        }
    }

    if (required_input_size >= this->depth)
    {
        this->delay.slice(0, this->delay.size()) =
            padded(input.slice(size_t(required_input_size - this->depth)));
    }
    else
    {
        this->delay.truncate(size_t(this->depth - required_input_size)) =
            this->delay.slice(size_t(required_input_size));
        this->delay.slice(size_t(this->depth - required_input_size)) = padded(input);
    }

    this->input_position += required_input_size;
    this->output_position += output.size();

    return required_input_size;
}

template struct samplerate_converter<float>;
template struct samplerate_converter<double>;
template struct samplerate_converter<complex<float>>;
template struct samplerate_converter<complex<double>>;

} // namespace impl
} // namespace CMT_ARCH_NAME

#ifdef CMT_MULTI_NEEDS_GATE

template <typename T>
samplerate_converter<T>::samplerate_converter(sample_rate_conversion_quality quality,
                                              itype interpolation_factor, itype decimation_factor,
                                              ftype scale, ftype cutoff)
{
    CMT_MULTI_GATE(reinterpret_cast<ns::impl::samplerate_converter<T>*>(this)->init(
        quality, interpolation_factor, decimation_factor, scale, cutoff));
}

template <typename T>
size_t samplerate_converter<T>::process_impl(univector_ref<T> output, univector_ref<const T> input)
{
    CMT_MULTI_GATE(
        return reinterpret_cast<ns::impl::samplerate_converter<T>*>(this)->process_impl(output, input));
}

template struct samplerate_converter<float>;
template struct samplerate_converter<double>;
template struct samplerate_converter<complex<float>>;
template struct samplerate_converter<complex<double>>;

#endif

} // namespace kfr
