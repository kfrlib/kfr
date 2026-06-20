/** @addtogroup dft
 *  @{
 */
/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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
#include <kfr/cident.h>
#if !defined KFR_SKIP_IF_NON_X86 || defined(KFR_ARCH_X86)

#include <kfr/dsp/sample_rate_conversion.hpp>
#include <kfr/multiarch.h>

namespace kfr
{
KFR_MULTI_PROTO(namespace impl {
    template <typename T>
    struct samplerate_converter : public kfr::samplerate_converter<T>
    {
    public:
        using itype = typename kfr::samplerate_converter<T>::itype;
        using ftype = typename kfr::samplerate_converter<T>::ftype;
        void init(sample_rate_conversion_quality quality, itype interpolation_factor, itype decimation_factor,
                  fbase scale, fbase cutoff);
        void init(int taps, itype interpolation_factor, itype decimation_factor, fbase scale, fbase cutoff,
                  fbase sidelobe_attenuation, fbase transition_width);
        size_t process_impl(univector_ref<T> output, univector_ref<const T> input);
    };
} // namespace impl
)

inline namespace KFR_ARCH_NAME
{
namespace impl
{

template <typename T>
void samplerate_converter<T>::init(int taps, itype interpolation_factor, itype decimation_factor, fbase scale,
                                   fbase cutoff, fbase sidelobe_attenuation, fbase transition_width)
{
    const i64 gcf = std::gcd(interpolation_factor, decimation_factor);
    interpolation_factor /= gcf;
    decimation_factor /= gcf;

    this->kaiser_beta = this->window_param(sidelobe_attenuation);

    this->depth           = taps;
    this->input_position  = 0;
    this->output_position = 0;

    this->taps  = this->depth * interpolation_factor;
    this->order = size_t(this->depth * interpolation_factor - 1);

    this->interpolation_factor = interpolation_factor;
    this->decimation_factor    = decimation_factor;

    const itype halftaps = this->taps / 2;
    this->filter         = univector<T>(size_t(this->taps), T());
    this->delay          = univector<T>(size_t(this->depth), T());

    fbase omega_c = cutoff * c_pi<fbase> / std::max(decimation_factor, interpolation_factor);

    // Generate filter coefficients directly in reordered polyphase layout.
    // Phase for output step k is stored at position k*depth, so process_impl reads
    // the filter array linearly from [0] to [taps-1] then wraps to [0].
    for (itype k = 0; k < interpolation_factor; k++)
    {
        const itype phase = interpolation_factor - 1 - (k * decimation_factor) % interpolation_factor;
        for (itype d = 0, jj = phase; d < this->depth; d++, jj += interpolation_factor)
        {
            this->filter[size_t(k * this->depth + d)] =
                sinc((jj - halftaps) * omega_c) * this->window(fbase(jj) / fbase(this->taps - 1));
        }
    }

    const T s    = reciprocal(sum(this->filter)) * static_cast<ftype>(interpolation_factor * scale);
    this->filter = this->filter * s;
}

template <typename T>
void samplerate_converter<T>::init(sample_rate_conversion_quality quality, itype interpolation_factor,
                                   itype decimation_factor, fbase scale, fbase cutoff)
{
    this->kaiser_beta     = this->window_param(quality);
    this->depth           = static_cast<itype>(this->filter_order(quality));
    this->input_position  = 0;
    this->output_position = 0;

    const i64 gcf = std::gcd(interpolation_factor, decimation_factor);
    interpolation_factor /= gcf;
    decimation_factor /= gcf;

    this->taps  = this->depth * interpolation_factor;
    this->order = size_t(this->depth * interpolation_factor - 1);

    this->interpolation_factor = interpolation_factor;
    this->decimation_factor    = decimation_factor;

    const itype halftaps = this->taps / 2;
    this->filter         = univector<T>(size_t(this->taps), T());
    this->delay          = univector<T>(size_t(this->depth), T());

    cutoff = cutoff - this->transition_width() / c_pi<fbase, 4>;

    cutoff = cutoff / std::max(decimation_factor, interpolation_factor);

    // Generate filter coefficients directly in reordered polyphase layout.
    for (itype k = 0; k < interpolation_factor; k++)
    {
        const itype phase = interpolation_factor - 1 - (k * decimation_factor) % interpolation_factor;
        for (itype d = 0, jj = phase; d < this->depth; d++, jj += interpolation_factor)
        {
            this->filter[size_t(k * this->depth + d)] = sinc((jj - halftaps) * cutoff * c_pi<fbase, 2>) *
                                                        this->window(fbase(jj) / fbase(this->taps - 1));
        }
    }

    const T s    = reciprocal(sum(this->filter)) * static_cast<ftype>(interpolation_factor * scale);
    this->filter = this->filter * s;
}

template <typename T>
KFR_NOINLINE static T safe_dotproduct(univector_ref<const T> a, univector_ref<const T> b)
{
    size_t min_length = std::min(a.size(), b.size());
    if (min_length == 0)
        return T(0);

#ifdef __clang__
    using reducer_t =
        expression_reduce<T, 1, T, T, fn::add, fn_generic::pass_through, fn_generic::pass_through>;

    reducer_t red(fn::add{}, fn_generic::pass_through{}, fn_generic::pass_through{});
    constexpr size_t w = maximum_vector_size<T> * 4;
    process<w>(red, a.truncate(min_length) * b.truncate(min_length));
    return red.get();
#else
    return dotproduct(a.truncate(min_length), b.truncate(min_length));
#endif
}

template <typename T>
size_t samplerate_converter<T>::process_impl(univector_ref<T> output, univector_ref<const T> input)
{
    const itype required_input_size = this->input_size_for_output(output.size());

    // printf("Interpolation factor: %lld, Decimation factor: %lld\n", this->interpolation_factor,
    //    this->decimation_factor);

    const itype input_size    = input.size();
    const itype interp_factor = this->interpolation_factor;
    const itype decim_factor  = this->decimation_factor;
    const itype depth         = this->depth;
    itype in_pos              = this->input_position;
    itype out_pos             = this->output_position;
    itype taps                = depth * interp_factor;

    itype phase_index = (out_pos) % interp_factor;

    for (size_t i = 0; i < output.size(); i++)
    {
        const itype intermediate_start = (static_cast<itype>(i) + out_pos) * decim_factor - taps + 1;
        const std::lldiv_t input_pos   = floor_div(intermediate_start + interp_factor - 1, interp_factor);
        // if (i < 2 * std::max(interp_factor, decim_factor))
        // printf("  i=%zu, intermediate_start=%lld, input_pos=%lld\n", i, intermediate_start,
        //    input_pos.quot);
        const itype input_start        = input_pos.quot; // first input sample
        const univector_ref<T> tap_ptr = this->filter.slice(static_cast<size_t>(phase_index * depth));

        if (input_start >= in_pos + input_size)
        {
            output[i] = T(0);
        }
        else if (input_start >= in_pos)
        {
            output[i] = safe_dotproduct<T>(input.slice(input_start - in_pos, depth), tap_ptr);
        }
        else
        {
            const itype prev_count = in_pos - input_start;
            output[i] =
                safe_dotproduct<T>(this->delay.slice(size_t(depth - prev_count)),
                                   tap_ptr.truncate(prev_count)) +
                safe_dotproduct<T>(input, tap_ptr.slice(size_t(prev_count), size_t(depth - prev_count)));
        }
        ++phase_index;
        if (phase_index == interp_factor)
            phase_index = 0;
    }

    if (required_input_size >= depth)
    {
        this->delay.slice(0, this->delay.size()) = padded(input.slice(size_t(required_input_size - depth)));
    }
    else
    {
        this->delay.truncate(size_t(depth - required_input_size)) =
            this->delay.slice(size_t(required_input_size));
        this->delay.slice(size_t(depth - required_input_size)) = padded(input);
    }

    this->input_position  = in_pos + required_input_size;
    this->output_position = out_pos + output.size();

    return required_input_size;
}

template struct samplerate_converter<float>;
template struct samplerate_converter<double>;
template struct samplerate_converter<complex<float>>;
template struct samplerate_converter<complex<double>>;

} // namespace impl
} // namespace KFR_ARCH_NAME

#ifdef KFR_MULTI_NEEDS_GATE

template <typename T>
samplerate_converter<T>::samplerate_converter(sample_rate_conversion_quality quality,
                                              itype interpolation_factor, itype decimation_factor,
                                              fbase scale, fbase cutoff)
{
    KFR_MULTI_GATE(reinterpret_cast<ns::impl::samplerate_converter<T>*>(this)->init(
        quality, interpolation_factor, decimation_factor, scale, cutoff));
}

template <typename T>
samplerate_converter<T>::samplerate_converter(int taps, itype interpolation_factor, itype decimation_factor,
                                              fbase scale, fbase cutoff, fbase sidelobe_attenuation,
                                              fbase transition_width)
{
    KFR_MULTI_GATE(reinterpret_cast<ns::impl::samplerate_converter<T>*>(this)->init(
        taps, interpolation_factor, decimation_factor, scale, cutoff, sidelobe_attenuation,
        transition_width));
}

template <typename T>
size_t samplerate_converter<T>::process_impl(univector_ref<T> output, univector_ref<const T> input)
{
    KFR_MULTI_GATE(
        return reinterpret_cast<ns::impl::samplerate_converter<T>*>(this)->process_impl(output, input));
}

template struct samplerate_converter<float>;
template struct samplerate_converter<double>;
template struct samplerate_converter<complex<float>>;
template struct samplerate_converter<complex<double>>;

#endif

} // namespace kfr

#endif
