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
#include <kfr/audio/filter.hpp>

namespace kfr
{

audio_filter::audio_filter() = default;

audio_filter::audio_filter(size_t channels, std::function<std::unique_ptr<filter<fbase>>(size_t)> fn)
    : channels(channels)
{
    KFR_LOGIC_CHECK(channels > 0, "audio_filter: channels must be > 0");
    KFR_LOGIC_CHECK(channels <= max_audio_channels, "audio_filter: channels must be <= max_audio_channels");
    for (size_t ch = 0; ch < channels; ++ch)
        filters[ch] = fn(ch);
}

audio_filter audio_filter::iir(size_t channels, const iir_params<fbase, tag_dynamic_vector>& params)
{
    return audio_filter(channels, [&](size_t) { return std::make_unique<iir_filter<fbase>>(params); });
}

audio_filter audio_filter::iir(size_t channels, const biquad_section<fbase>& section)
{
    return audio_filter(channels,
                        [&](size_t) { return std::make_unique<iir_filter<fbase>>(iir_params{ section }); });
}

audio_filter audio_filter::fir(size_t channels, const fir_params<fbase>& taps)
{
    return audio_filter(channels, [&](size_t) { return std::make_unique<fir_filter<fbase>>(taps); });
}

#ifdef KFR_HAVE_DFT
audio_filter audio_filter::convolution(size_t channels, const univector_ref<const fbase>& impulse_response,
                                       size_t block_size)
{
    return audio_filter(channels, [&](size_t)
                        { return std::make_unique<convolve_filter<fbase>>(impulse_response, block_size); });
}
#endif

void audio_filter::reset()
{
    for (auto& f : filters)
    {
        if (f)
            f->reset();
    }
}

void audio_filter::apply(audio_data_planar& data)
{
    KFR_LOGIC_CHECK(data.channels == channels, "audio_filter: channel count mismatch");
    for (size_t ch = 0; ch < data.channels; ++ch)
        filters[ch]->apply(data.data[ch], data.size);
}

void audio_filter::apply(audio_data_planar& dest, const audio_data_planar& src)
{
    KFR_LOGIC_CHECK(src.channels == channels, "audio_filter: channel count mismatch");
    KFR_LOGIC_CHECK(dest.channels == channels, "audio_filter: channel count mismatch");
    KFR_LOGIC_CHECK(src.size == dest.size, "audio_filter: size mismatch");
    for (size_t ch = 0; ch < src.channels; ++ch)
        filters[ch]->apply(dest.data[ch], src.data[ch], src.size);
}

} // namespace kfr
