/** @addtogroup dsp
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

/**
 * @enum sample_rate_conversion_quality
 * @brief Defines the quality levels for sample rate conversion.
 *
 * Higher values indicate better quality but increased computational cost.
 */
enum class sample_rate_conversion_quality : int
{
    draft   = 4, /**< Draft quality (lowest, fastest). */
    low     = 6, /**< Low quality. */
    normal  = 8, /**< Normal quality (balanced). */
    high    = 10, /**< High quality. */
    perfect = 12 /**< Perfect quality (highest, slowest). */
};

/**
 * @brief Alias for sample_rate_conversion_quality.
 */
using resample_quality = sample_rate_conversion_quality;

/**
 * @class samplerate_converter
 * @brief A template class for performing sample rate conversion on audio signals.
 *
 * This class supports both push and pull methods for resampling audio data. It maintains an internal
 * state to handle contiguous signals split into buffers of varying sizes.
 *
 * @tparam T The data type of the audio samples (e.g., float, double).
 */
template <typename T>
struct samplerate_converter
{
    using itype = i64; /**< Integer type for positions and factors. */
    using ftype = subtype<T>; /**< Floating-point subtype of T. */

protected:
    /**
     * @brief Computes the Kaiser window function for a given sample position.
     * @param n Normalized sample position.
     * @return The window value.
     */
    KFR_MEM_INTRINSIC fbase window(fbase n) const
    {
        return modzerobessel(kaiser_beta * sqrt(1 - sqr(2 * n - 1))) * reciprocal(modzerobessel(kaiser_beta));
    }

    /**
     * @brief Calculates the sidelobe attenuation based on the Kaiser beta parameter.
     * @return Sidelobe attenuation in dB.
     */
    KFR_MEM_INTRINSIC ftype sidelobe_att() const { return static_cast<ftype>(kaiser_beta / 0.1102 + 8.7); }

    /**
     * @brief Calculates the transition width based on sidelobe attenuation and depth.
     * @return Transition width in radians.
     */
    KFR_MEM_INTRINSIC ftype transition_width() const
    {
        return static_cast<ftype>((sidelobe_att() - 8) / (depth - 1) / 2.285);
    }

public:
    /**
     * @brief Resets the converter to its initial state.
     *
     * Clears the internal input/output positions and zeroes the delay line, so the
     * next call to `process` or `skip` behaves as if the converter were freshly
     * constructed (without reinitializing the filter coefficients).
     */
    void reset() noexcept
    {
        this->input_position  = 0;
        this->output_position = 0;
        this->delay           = zeros<T>();
    }

    /**
     * @brief Computes the filter order for a given quality level.
     * @param quality The sample rate conversion quality.
     * @return The filter order as a size_t.
     */
    static KFR_MEM_INTRINSIC size_t filter_order(sample_rate_conversion_quality quality)
    {
        return size_t(1) << (static_cast<int>(quality) + 1);
    }

    /**
     * @brief Returns the sidelobe attenuation for a given quality level.
     * @param quality The sample rate conversion quality.
     * @return Sidelobe attenuation in dB.
     */
    static KFR_MEM_INTRINSIC fbase sidelobe_attenuation(sample_rate_conversion_quality quality)
    {
        return (static_cast<int>(quality) - 3) * fbase(20);
    }

    /**
     * @brief Returns the transition width for a given quality level.
     * @param quality The sample rate conversion quality.
     * @return Transition width in radians.
     */
    static KFR_MEM_INTRINSIC fbase transition_width(sample_rate_conversion_quality quality)
    {
        return (sidelobe_attenuation(quality) - 8) / (filter_order(quality) - 1) / fbase(2.285);
    }

    /**
     * @brief Computes the Kaiser window parameter for a given quality level.
     * @param quality The sample rate conversion quality.
     * @return The Kaiser beta parameter.
     */
    static KFR_MEM_INTRINSIC fbase window_param(sample_rate_conversion_quality quality)
    {
        return window_param(sidelobe_attenuation(quality));
    }
    static KFR_MEM_INTRINSIC fbase window_param(fbase att)
    {
        if (att > 50)
            return fbase(0.1102) * (att - fbase(8.7));
        if (att >= 21)
            return fbase(0.5842) * pow(att - 21, fbase(0.4)) + fbase(0.07886) * (att - 21);
        return 0;
    }

    /**
     * @brief Constructs a sample rate converter.
     * @param quality The desired conversion quality.
     * @param interpolation_factor Factor by which to interpolate the input signal.
     * @param decimation_factor Factor by which to decimate the output signal.
     * @param scale Scaling factor for the output (default: 1).
     * @param cutoff Cutoff frequency as a fraction of the Nyquist frequency (default: 0.5).
     */
    samplerate_converter(sample_rate_conversion_quality quality, itype interpolation_factor,
                         itype decimation_factor, fbase scale = ftype(1), fbase cutoff = 0.5f);

    /**
     * @brief Constructs a sample rate converter from explicit filter parameters.
     * @param taps Number of taps per polyphase branch (processing depth).
     * @param interpolation_factor Factor by which to interpolate the input signal.
     * @param decimation_factor Factor by which to decimate the output signal.
     * @param scale Scaling factor applied to the output.
     * @param cutoff Cutoff frequency as a fraction of the Nyquist frequency.
     * @param sidelobe_attenuation Sidelobe attenuation in dB used to derive the Kaiser window.
     * @param transition_width Transition width in radians used to adjust the cutoff.
     */
    samplerate_converter(int taps, itype interpolation_factor, itype decimation_factor, fbase scale,
                         fbase cutoff, fbase sidelobe_attenuation, fbase transition_width);

    /**
     * @brief Default constructor.
     *
     * Leaves the converter in an uninitialized state: no filter coefficients,
     * zero factors, and an empty delay line. Any non-static member function
     * other than `reset` must not be called until the converter is assigned a
     * fully-constructed instance (e.g. via move assignment).
     */
    samplerate_converter() = default;

    /**
     * @brief Move constructor.
     *
     * Transfers ownership of the filter coefficients, delay line, and current
     * input/output positions from `other`. The moved-from object is left in a
     * valid but unspecified state.
     */
    samplerate_converter(samplerate_converter&&) noexcept = default;

    /**
     * @brief Move assignment operator.
     *
     * Transfers ownership of the filter coefficients, delay line, and current
     * input/output positions from `other`, releasing any resources previously
     * held by `*this`. The moved-from object is left in a valid but unspecified
     * state.
     * @param other The converter to move from.
     * @return A reference to `*this`.
     */
    samplerate_converter& operator=(samplerate_converter&&) noexcept = default;

    /**
     * @brief Converts an input position to an intermediate (interpolated) position.
     *
     * The intermediate domain is the high-rate domain obtained after
     * interpolation and before decimation. This maps an input sample index to
     * its corresponding index in that domain.
     * @param in_pos Input position.
     * @return Intermediate position `in_pos * interpolation_factor`.
     */
    KFR_MEM_INTRINSIC itype input_position_to_intermediate(itype in_pos) const
    {
        return in_pos * interpolation_factor;
    }

    /**
     * @brief Converts an output position to an intermediate (pre-decimation) position.
     *
     * Maps an output sample index back to its corresponding index in the
     * high-rate intermediate domain.
     * @param out_pos Output position.
     * @return Intermediate position `out_pos * decimation_factor`.
     */
    KFR_MEM_INTRINSIC itype output_position_to_intermediate(itype out_pos) const
    {
        return out_pos * decimation_factor;
    }

    /**
     * @brief Converts an input position to the corresponding output position.
     *
     * Uses floor division of the intermediate position by the decimation factor,
     * so the result is the index of the output sample that consumes the given
     * input sample (or the last output sample produced at or before it).
     * @param in_pos Input position.
     * @return Corresponding output position.
     */
    KFR_MEM_INTRINSIC itype input_position_to_output(itype in_pos) const
    {
        return floor_div(input_position_to_intermediate(in_pos), decimation_factor).quot;
    }

    /**
     * @brief Converts an output position to the corresponding input position.
     *
     * Uses floor division of the intermediate position by the interpolation
     * factor, so the result is the index of the input sample needed to produce
     * the given output sample.
     * @param out_pos Output position.
     * @return Corresponding input position.
     */
    KFR_MEM_INTRINSIC itype output_position_to_input(itype out_pos) const
    {
        return floor_div(output_position_to_intermediate(out_pos), interpolation_factor).quot;
    }

    /**
     * @brief Calculates the output size produced for a given input size (push method).
     *
     * Accounts for the current input/output positions, so the returned value is
     * the exact number of output samples that `process` will write when given
     * `input_size` new input samples.
     * @param input_size Size of the input buffer.
     * @return Required output buffer size.
     */
    KFR_MEM_INTRINSIC itype output_size_for_input(itype input_size) const
    {
        return input_position_to_output(input_position + input_size - 1) -
               input_position_to_output(input_position - 1);
    }

    /**
     * @brief Calculates the input size required to produce a given output size (pull method).
     *
     * Accounts for the current input/output positions, so the returned value is
     * the exact number of input samples that must be supplied to `process` in
     * order to produce `output_size` output samples.
     * @param output_size Size of the output buffer.
     * @return Required input buffer size.
     */
    KFR_MEM_INTRINSIC itype input_size_for_output(itype output_size) const
    {
        return output_position_to_input(output_position + output_size - 1) -
               output_position_to_input(output_position - 1);
    }

    /**
     * @brief Skips a specified number of output samples, updating internal state.
     *
     * Consumes the corresponding input samples (as given by
     * `input_size_for_output`) without producing any output, and updates the
     * delay line so that subsequent calls to `process` continue seamlessly.
     * This is typically used to discard the FIR filter's group delay at the
     * start of a stream.
     * @param output_size Number of output samples to skip.
     * @param input Input buffer to consume.
     * @return Number of input samples consumed.
     */
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

    /**
     * @brief Processes input data to produce resampled output (pull or push method).
     *
     * Reads samples from `input`, advances the internal input/output positions,
     * and writes the resampled result to `output`. The number of input samples
     * consumed is given by `input_size_for_output(output.size())`; the number of
     * output samples produced equals `output.size()`. The delay line is updated
     * so that consecutive calls operate on a contiguous stream.
     * @tparam Tag Type tag for the univector output.
     * @param output Output buffer to write resampled data.
     * @param input Input buffer to read samples from.
     * @return Number of input samples processed.
     */
    template <univector_tag Tag>
    size_t process(univector<T, Tag>& output, univector_ref<const T> input)
    {
        return process_impl(output.slice(), input);
    }

    /**
     * @brief Gets the fractional delay introduced by the resampler.
     *
     * The delay is the group delay of the symmetric FIR filter, expressed in
     * output-sample units.
     * @return Fractional delay in output samples.
     */
    KFR_MEM_INTRINSIC fbase get_fractional_delay() const { return (taps - 1) * 0.5 / decimation_factor; }

    /**
     * @brief Gets the integer delay introduced by the resampler.
     *
     * Returns `get_fractional_delay()` truncated to an integer. This is the
     * number of leading output samples that should be discarded (e.g. via
     * `skip`) to align the output with the input.
     * @return Delay in output samples.
     */
    KFR_MEM_INTRINSIC size_t get_delay() const { return static_cast<size_t>(get_fractional_delay()); }

    fbase kaiser_beta; /**< Kaiser window beta parameter. */
    itype depth; /**< Processing depth. */
    itype taps; /**< Number of filter taps. */
    size_t order; /**< Filter order. */
    itype interpolation_factor; /**< Interpolation factor. */
    itype decimation_factor; /**< Decimation factor. */
    univector<T> filter; /**< Filter coefficients. */
    univector<T> delay; /**< Delay line buffer. */

protected:
    itype input_position; /**< Current input position. */
    itype output_position; /**< Current output position. */

    /**
     * @brief Internal implementation of the process function.
     * @param output Output buffer slice.
     * @param input Input buffer slice.
     * @return Number of input samples processed.
     */
    size_t process_impl(univector_ref<T> output, univector_ref<const T> input);
};

inline namespace KFR_ARCH_NAME
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

    KFR_MEM_INTRINSIC size_t size() const noexcept { return expression_with_arguments<E>::size() * 2; }

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

    KFR_MEM_INTRINSIC size_t size() const noexcept { return expression_with_arguments<E>::size() * 4; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, N>) noexcept
    {
        const vec<T, N / 4> x  = get_elements(self.first(), index / 4, axis_params<0, N / 4>());
        const vec<T, N / 2> xx = interleave(x, zerovector(x));
        return interleave(xx, zerovector(xx));
    }
    KFR_INTRINSIC friend vec<T, 2> get_elements(const expression_upsample& self, index_t index,
                                                axis_params<0, 2>) noexcept
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
                                                axis_params<0, 1>) noexcept
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

    KFR_MEM_INTRINSIC size_t size() const noexcept { return expression_with_arguments<E>::size() / 2; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_downsample& self, size_t index,
                                                axis_params<0, N>) noexcept
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

    KFR_MEM_INTRINSIC size_t size() const noexcept { return expression_with_arguments<E>::size() / 4; }

    template <size_t N>
    KFR_INTRINSIC friend vec<T, N> get_elements(const expression_downsample& self, index_t index,
                                                axis_params<0, N>) noexcept
    {
        const vec<T, N * 4> x = get_elements(self.first(), index * 4, axis_params<0, N * 4>());
        return x.shuffle(csizeseq<N, offset, 4>);
    }
};
} // namespace internal

/**
 * @brief Downsamples a signal by a factor of 2 by selecting every other sample.
 *
 * @warning This function does NOT apply any anti-aliasing filter. It simply drops
 * samples (keeping samples at positions `offset`, `offset+2`, `offset+4`, ...).
 * Use this only when no filtering is desired; otherwise prefer a proper
 * `samplerate_converter` with a decimation factor of 2.
 *
 * @tparam E1 The input expression type.
 * @tparam offset Index of the first sample to keep (0 or 1).
 * @param e1 The input expression.
 * @return An expression producing the downsampled signal.
 */
template <typename E1, size_t offset = 0>
KFR_FUNCTION internal::expression_downsample<2, offset, E1> downsample2(E1&& e1,
                                                                        csize_t<offset> = csize_t<0>())
{
    return internal::expression_downsample<2, offset, E1>(std::forward<E1>(e1));
}

/**
 * @brief Downsamples a signal by a factor of 4 by selecting every fourth sample.
 *
 * @warning This function does NOT apply any anti-aliasing filter. It simply drops
 * samples (keeping samples at positions `offset`, `offset+4`, `offset+8`, ...).
 * Use this only when no filtering is desired; otherwise prefer a proper
 * `samplerate_converter` with a decimation factor of 4.
 *
 * @tparam E1 The input expression type.
 * @tparam offset Index of the first sample to keep (0, 1, 2, or 3).
 * @param e1 The input expression.
 * @return An expression producing the downsampled signal.
 */
template <typename E1, size_t offset = 0>
KFR_FUNCTION internal::expression_downsample<4, offset, E1> downsample4(E1&& e1,
                                                                        csize_t<offset> = csize_t<0>())
{
    return internal::expression_downsample<4, offset, E1>(std::forward<E1>(e1));
}

/**
 * @brief Upsamples a signal by a factor of 2 by inserting zero samples.
 *
 * @warning This function does NOT apply any interpolation filter. It inserts a
 * zero sample between each pair of consecutive input samples. Use this only when
 * no filtering is desired; otherwise prefer a proper `samplerate_converter`
 * with an interpolation factor of 2.
 *
 * @tparam E1 The input expression type.
 * @param e1 The input expression.
 * @return An expression producing the upsampled signal.
 */
template <typename E1>
KFR_FUNCTION internal::expression_upsample<2, E1> upsample2(E1&& e1)
{
    return internal::expression_upsample<2, E1>(std::forward<E1>(e1));
}

/**
 * @brief Upsamples a signal by a factor of 4 by inserting zero samples.
 *
 * @warning This function does NOT apply any interpolation filter. It inserts three
 * zero samples between each pair of consecutive input samples. Use this only when
 * no filtering is desired; otherwise prefer a proper `samplerate_converter` with
 * an interpolation factor of 4.
 *
 * @tparam E1 The input expression type.
 * @param e1 The input expression.
 * @return An expression producing the upsampled signal.
 */
template <typename E1>
KFR_FUNCTION internal::expression_upsample<4, E1> upsample4(E1&& e1)
{
    return internal::expression_upsample<4, E1>(std::forward<E1>(e1));
}

/**
 * @brief Helper function to create a sample rate converter instance.
 * @tparam T Data type of the audio samples (default: fbase).
 * @param quality The desired conversion quality.
 * @param interpolation_factor Factor by which to interpolate the input signal.
 * @param decimation_factor Factor by which to decimate the output signal.
 * @param scale Scaling factor for the output (default: 1).
 * @param cutoff Cutoff frequency as a fraction of the Nyquist frequency (default: 0.5).
 * @return A configured samplerate_converter instance.
 */
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
/**
 * @brief Helper function to create a sample rate converter instance.
 * @deprecated Deprecated in 0.9.2. Use `sample_rate_converter` instead.
 * @tparam T Data type of the audio samples (default: fbase).
 * @param quality The desired conversion quality.
 * @param interpolation_factor Factor by which to interpolate the input signal.
 * @param decimation_factor Factor by which to decimate the output signal.
 * @param scale Scaling factor for the output (default: 1).
 * @param cutoff Cutoff frequency as a fraction of the Nyquist frequency (default: 0.5).
 * @return A configured samplerate_converter instance.
 */
template <typename T = fbase>
KFR_FUNCTION samplerate_converter<T> resampler(sample_rate_conversion_quality quality,
                                               size_t interpolation_factor, size_t decimation_factor,
                                               subtype<T> scale = subtype<T>(1), subtype<T> cutoff = 0.5f)
{
    using itype = typename samplerate_converter<T>::itype;
    return samplerate_converter<T>(quality, itype(interpolation_factor), itype(decimation_factor), scale,
                                   cutoff);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
