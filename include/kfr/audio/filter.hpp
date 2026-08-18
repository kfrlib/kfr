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

#include <array>
#include <functional>
#include <memory>

#include <kfr/audio/data.hpp>
#include <kfr/base/filter.hpp>
#ifdef KFR_HAVE_DFT
#include <kfr/dft/convolution.hpp>
#endif
#include <kfr/dsp/biquad.hpp>
#include <kfr/dsp/fir.hpp>

namespace kfr
{

/**
 * @brief Multi-channel audio filter that operates on `audio_data` and maintains
 *        a per-channel array of `filter<fbase>` instances.
 */
class audio_filter
{
public:
    /**
     * @brief Default constructor. Constructs an empty audio filter with no channels.
     */
    audio_filter();

    /**
     * @brief Constructs an audio_filter for the given number of channels, initializing
     *        each channel filter with the factory function `fn()`.
     * @param fn Factory function receiving the zero-based channel index and returning a
     *        `std::unique_ptr<filter<fbase>>`.
     * @param channels Number of audio channels.
     */
    audio_filter(size_t channels, std::function<std::unique_ptr<filter<fbase>>(size_t)> fn);

    /**
     * @brief Creates an audio filter using IIR biquad sections for all channels.
     * @param channels Number of audio channels.
     * @param params IIR biquad parameters.
     * @return Constructed `audio_filter`.
     */
    static audio_filter iir(size_t channels, const iir_params<fbase, tag_dynamic_vector>& params);

    /**
     * @brief Creates an audio filter using a single IIR biquad section for all channels.
     * @param channels Number of audio channels.
     * @param section Single biquad section.
     * @return Constructed `audio_filter`.
     */
    static audio_filter iir(size_t channels, const biquad_section<fbase>& section);

    /**
     * @brief Creates an audio filter using an FIR filter for all channels.
     * @param channels Number of audio channels.
     * @param taps FIR filter taps / parameters.
     * @return Constructed `audio_filter`.
     */
    static audio_filter fir(size_t channels, const fir_params<fbase>& taps);

  #ifdef KFR_HAVE_DFT
    /**
     * @brief Creates an audio filter using streaming overlap-add FFT convolution for all channels.
     * @param channels Number of audio channels.
     * @param impulse_response Filter impulse response / kernel.
     * @param block_size Processing block size (default: 1024).
     * @return Constructed `audio_filter`.
     */
    static audio_filter convolution(size_t channels, const univector_ref<const fbase>& impulse_response,
                                    size_t block_size = 1024);
  #endif

    /**
     * @brief Resets the internal state of all per-channel filter instances.
     */
    void reset();

    /**
     * @brief Applies the filter in-place to planar audio data.
     * @param data Planar audio data to process in place.
     */
    void apply(audio_data_planar& data);

    /**
     * @brief Applies the filter to planar audio data, writing the result to `dest`.
     * @param dest Destination planar audio data.
     * @param src Source planar audio data.
     */
    void apply(audio_data_planar& dest, const audio_data_planar& src);

    std::array<std::unique_ptr<filter<fbase>>, max_audio_channels> filters;
    size_t channels = 0;
};

} // namespace kfr
