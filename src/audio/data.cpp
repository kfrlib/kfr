/** @addtogroup audio
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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
#include <kfr/audio/data.hpp>

namespace kfr
{

template <bool Interleaved>
audio_data<Interleaved>::audio_data(std::span<fbase* const> pointers, size_t size)
    requires(!Interleaved)
    : channels(pointers.size()), data{}, size(size), capacity(size)
{
    KFR_ASSERT(channels > 0);
    KFR_ASSERT(channels <= max_audio_channels);
    std::copy(pointers.begin(), pointers.end(), data.begin());
}

template <bool Interleaved>
audio_data<Interleaved>::audio_data(size_t channels, size_t size)
    : channels(channels), size(size), capacity(size)
{
    KFR_ASSERT(channels > 0);
    KFR_ASSERT(channels <= max_audio_channels);
    if (!empty())
    {
        constexpr size_t sampleAlignment = Interleaved ? 1 : 64 / sizeof(fbase);
        std::shared_ptr<details::aligned_deallocator> dealloc(new details::aligned_deallocator{
            kfr::aligned_allocate<fbase>(channels * align_up(capacity, sampleAlignment)) });
        if constexpr (Interleaved)
        {
            data = dealloc->ptr;
        }
        else
        {
            for (uint32_t i = 0; i < channels; ++i)
            {
                data[i] = dealloc->ptr + i * align_up(capacity, sampleAlignment);
            }
        }
        deallocator = std::move(dealloc);
    }
}

template <bool Interleaved>
audio_data<Interleaved>::audio_data(size_t channels, size_t size, fbase value) : audio_data(channels, size)
{
    fill(value);
}

template <bool Interleaved>
size_t audio_data<Interleaved>::total_samples() const noexcept
{
    return size * channel_count();
}

template <bool Interleaved>
void audio_data<Interleaved>::fill(fbase value)
{
    for_channel([value](univector_ref<fbase> data) { data = scalar(value); });
}

template <bool Interleaved>
void audio_data<Interleaved>::multiply(fbase value)
{
    if (value == fbase(1))
        return;
    if (value == fbase(0))
    {
        fill(fbase(0));
        return;
    }
    for_channel([value](univector_ref<fbase> data) { data *= value; });
}
template <bool Interleaved>
void audio_data<Interleaved>::clear()
{
    size     = 0;
    position = 0;
}

template <bool Interleaved>
void audio_data<Interleaved>::resize(size_t new_size)
{
    if (new_size <= capacity)
    {
        size = new_size;
        return;
    }
    reserve(details::round_capacity(new_size));
    size = new_size;
}

template <bool Interleaved>
void audio_data<Interleaved>::resize(size_t new_size, fbase value)
{
    size_t old_size = size;
    resize(new_size);
    if (new_size > old_size)
    {
        if constexpr (Interleaved)
        {
            make_univector(data + old_size * channels, (new_size - old_size) * channels) = scalar(value);
        }
        else
        {
            for (uint32_t ch = 0; ch < channels; ++ch)
            {
                make_univector(data[ch] + old_size, new_size - old_size) = scalar(value);
            }
        }
    }
}

template <bool Interleaved>
void audio_data<Interleaved>::reserve(size_t new_capacity)
{
    if (new_capacity <= capacity)
    {
        capacity = new_capacity;
        return;
    }

    audio_data result(channels, new_capacity);
    result.size     = size;
    result.position = position;
    if (size > 0)
    {
        if constexpr (Interleaved)
        {
            if (result.data)
                std::memcpy(result.data, data, size * channels * sizeof(fbase));
        }
        else
        {
            for (uint32_t i = 0; i < channels; ++i)
            {
                if (result.data[i])
                    std::memcpy(result.data[i], data[i], size * sizeof(fbase));
            }
        }
    }
    swap(result);
}

template <bool Interleaved>
void audio_data<Interleaved>::append(const audio_data& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (Interleaved)
    {
        std::memcpy(data + old_size * channel_count(), other.data,
                    other.size * channel_count() * sizeof(fbase));
    }
    else
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            std::memcpy(data[ch] + old_size, other.data[ch], other.size * sizeof(fbase));
        }
    }
}

template <bool Interleaved>
void audio_data<Interleaved>::prepend(const audio_data& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (Interleaved)
    {
        std::memmove(data + other.size * channel_count(), data, old_size * channel_count() * sizeof(fbase));
        std::memcpy(data, other.data, other.size * channel_count() * sizeof(fbase));
    }
    else
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            std::memmove(data[ch] + other.size, data[ch], old_size * sizeof(fbase));
            std::memcpy(data[ch], other.data[ch], other.size * sizeof(fbase));
        }
    }
    position -= other.size;
}

[[maybe_unused]] static chan<fbase*, false> operator+(const chan<fbase*, false>& arr, size_t offset)
{
    chan<fbase*, false> result = arr;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = result[i] ? result[i] + offset : nullptr;
    }
    return result;
}

template <bool Interleaved>
void audio_data<Interleaved>::append(const audio_data<!Interleaved>& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (Interleaved)
    {
        samples_store(data + old_size * channel_count(), other.pointers(), other.channel_count(), other.size);
    }
    else
    {
        samples_load(pointers() + old_size, other.data, other.channel_count(), other.size);
    }
}

template <bool Interleaved>
void audio_data<Interleaved>::prepend(const audio_data<!Interleaved>& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (Interleaved)
    {
        std::memmove(data + other.size * channel_count(), data, old_size * channel_count() * sizeof(fbase));
        samples_store(data, other.pointers(), other.channel_count(), other.size);
    }
    else
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            std::memmove(data[ch] + other.size, data[ch], old_size * sizeof(fbase));
        }
        samples_load(pointers(), other.data, other.channel_count(), other.size);
    }
    position -= other.size;
}

template <bool Interleaved>
void audio_data<Interleaved>::swap(audio_data& other) noexcept
{
    std::swap(channels, other.channels);
    std::swap(data, other.data);
    std::swap(size, other.size);
    std::swap(capacity, other.capacity);
    std::swap(position, other.position);
    std::swap(deallocator, other.deallocator);
}

template <bool Interleaved>
void audio_data<Interleaved>::reset()
{
    *this = audio_data<Interleaved>{};
}

template <bool Interleaved>
size_t audio_data<Interleaved>::channel_count() const noexcept
{
    return channels;
}

template <bool Interleaved>
audio_data<Interleaved> audio_data<Interleaved>::truncate(size_t length) const
{
    return slice(0, length);
}

template <bool Interleaved>
audio_data<Interleaved> audio_data<Interleaved>::slice(size_t start, size_t length) const
{
    KFR_LOGIC_CHECK(start <= size, "Slice out of range");
    audio_data result = *this;
    result.size       = std::min(length, size - start);
    if constexpr (Interleaved)
    {
        result.data += start * channel_count();
    }
    else
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            result.data[ch] += start;
        }
    }
    result.position += start;
    return result;
}

template <bool Interleaved>
audio_data<Interleaved> audio_data<Interleaved>::slice_past_end(size_t length)
{
    reserve(details::round_capacity(size + length));
    audio_data result = *this;
    result.size       = length;
    if constexpr (Interleaved)
    {
        result.data += size * channel_count();
    }
    else
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            result.data[ch] += size;
        }
    }
    result.position = position + size;
    return result;
}

template <bool Interleaved>
size_t audio_data<Interleaved>::find_peak() const noexcept
{
    size_t peakIndex = 0;
    fbase peakValue  = 0.0;
    for (size_t i = 0; i < size; ++i)
    {
        fbase sum = fbase(0);
        for (size_t ch = 0; ch < channels; ++ch)
        {
            if constexpr (Interleaved)
            {
                sum += std::abs(data[i * channels + ch]);
            }
            else
            {
                sum += std::abs(data[ch][i]);
            }
        }
        if (sum > peakValue)
        {
            peakValue = sum;
            peakIndex = i;
        }
    }
    return peakIndex;
}

template <bool Interleaved>
audio_stat audio_data<Interleaved>::stat() const noexcept
{
    audio_stat result{ 0, 0 };
    if (empty())
    {
        return result;
    }
    if constexpr (Interleaved)
    {
        result.peak = std::max(result.peak, absmaxof(interlaved()));
        result.rms  = rms(interlaved());
    }
    else
    {
        for (uint32_t ch = 0; ch < channels; ++ch)
        {
            result.peak = std::max(result.peak, absmaxof(channel(ch)));
            result.rms += sumsqr(channel(ch));
        }
        result.rms = std::sqrt(result.rms / (size * channels));
    }
    return result;
}

template <bool Interleaved>
bool audio_data<Interleaved>::is_silent(fbase threshold) const noexcept
{
    if constexpr (Interleaved)
    {
        for (size_t i = 0; i < size * channels; ++i)
        {
            if (std::abs(data[i]) > threshold)
                return false;
        }
    }
    else
    {
        for (size_t i = 0; i < size; ++i)
        {
            for (size_t ch = 0; ch < channels; ++ch)
            {
                if (std::abs(data[ch][i]) > threshold)
                    return false;
            }
        }
    }
    return true;
}

template struct audio_data<false>;
template struct audio_data<true>;

namespace
{

inline void cvt_sample(int32_t& sample, fbase value, const audio_quantization& quant) noexcept
{
    sample = std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * fbase(2147483647.0));
}
inline void cvt_sample(int16_t& sample, fbase value, const audio_quantization& quant) noexcept
{
    sample = std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * fbase(32767.0));
}
inline void cvt_sample(kfr::i24& sample, fbase value, const audio_quantization& quant) noexcept
{
    sample = std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * fbase(8388607.0));
}
inline void cvt_sample(float& sample, fbase value, const audio_quantization&) noexcept { sample = value; }
inline void cvt_sample(double& sample, fbase value, const audio_quantization&) noexcept { sample = value; }

inline void cvt_sample(int32_t& sample, fbase value) noexcept
{
    sample = std::llround(std::clamp(value, fbase(-1.0), fbase(+1.0)) * fbase(2147483647.0));
}
inline void cvt_sample(int16_t& sample, fbase value) noexcept
{
    sample = std::llround(std::clamp(value, fbase(-1.0), fbase(+1.0)) * fbase(32767.0));
}
inline void cvt_sample(kfr::i24& sample, fbase value) noexcept
{
    sample = std::llround(std::clamp(value, fbase(-1.0), fbase(+1.0)) * fbase(8388607.0));
}
[[maybe_unused]] inline void cvt_sample(float& sample, double value) noexcept { sample = value; }
[[maybe_unused]] inline void cvt_sample(double& sample, double value) noexcept { sample = value; }
[[maybe_unused]] inline void cvt_sample(float& value, float sample) noexcept { value = sample; }
[[maybe_unused]] inline void cvt_sample(double& value, float sample) noexcept { value = sample; }

inline void cvt_sample(fbase& value, int16_t sample) noexcept { value = sample / fbase(32767.0); }
inline void cvt_sample(fbase& value, kfr::i24 sample) noexcept { value = sample / fbase(8388607.0); }
inline void cvt_sample(fbase& value, int32_t sample) noexcept { value = sample / fbase(2147483647.0); }
} // namespace

template <typename Tout>
void samples_store(Tout* out, const fbase* in, size_t size, const audio_quantization& quantization,
                   bool swap_bytes) noexcept
{
    for (size_t i = 0; i < size; ++i)
    {
        Tout tmp;
        cvt_sample(tmp, in[i], quantization);
        if (swap_bytes)
            details::convert_endianness(tmp);
        out[i] = tmp;
    }
}

template <typename Tin>
void samples_load(fbase* out, const Tin* in, size_t size, bool swap_bytes) noexcept
{
    for (size_t i = 0; i < size; ++i)
    {
        Tin tmp = in[i];
        if (swap_bytes)
            details::convert_endianness(tmp);
        cvt_sample(out[i], tmp);
    }
}

template <typename Tout>
void samples_store(Tout* out, const fbase* const in[], size_t channels, size_t size,
                   const audio_quantization& quantization, bool swap_bytes) noexcept
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
        {
            Tout tmp;
            cvt_sample(tmp, in[ch][i], quantization);
            if (swap_bytes)
                details::convert_endianness(tmp);
            out[i * channels + ch] = tmp;
        }
    }
}
template <typename Tout>
void samples_store(Tout* out, const fbase* const in[], size_t channels, size_t size, bool swap_bytes) noexcept
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
        {
            Tout tmp;
            cvt_sample(tmp, in[ch][i]);
            if (swap_bytes)
                details::convert_endianness(tmp);
            out[i * channels + ch] = tmp;
        }
    }
}

template <typename Tin>
void samples_load(fbase* const out[], const Tin* in, size_t channels, size_t size, bool swap_bytes) noexcept
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
        {
            Tin tmp = in[i * channels + ch];
            if (swap_bytes)
                details::convert_endianness(tmp);
            cvt_sample(out[ch][i], tmp);
        }
    }
}

template void samples_load(fbase* const[], const i16*, size_t, size_t, bool) noexcept;
template void samples_load(fbase* const[], const i24*, size_t, size_t, bool) noexcept;
template void samples_load(fbase* const[], const i32*, size_t, size_t, bool) noexcept;
template void samples_load(fbase* const[], const f32*, size_t, size_t, bool) noexcept;
template void samples_load(fbase* const[], const f64*, size_t, size_t, bool) noexcept;

template void samples_store(i16*, const fbase* const[], size_t, size_t, bool) noexcept;
template void samples_store(i24*, const fbase* const[], size_t, size_t, bool) noexcept;
template void samples_store(i32*, const fbase* const[], size_t, size_t, bool) noexcept;
template void samples_store(f32*, const fbase* const[], size_t, size_t, bool) noexcept;
template void samples_store(f64*, const fbase* const[], size_t, size_t, bool) noexcept;

template void samples_store(i16*, const fbase* const[], size_t, size_t, const audio_quantization&,
                            bool) noexcept;
template void samples_store(i24*, const fbase* const[], size_t, size_t, const audio_quantization&,
                            bool) noexcept;
template void samples_store(i32*, const fbase* const[], size_t, size_t, const audio_quantization&,
                            bool) noexcept;
template void samples_store(f32*, const fbase* const[], size_t, size_t, const audio_quantization&,
                            bool) noexcept;
template void samples_store(f64*, const fbase* const[], size_t, size_t, const audio_quantization&,
                            bool) noexcept;

template void samples_load(fbase*, const i16*, size_t, bool) noexcept;
template void samples_load(fbase*, const i24*, size_t, bool) noexcept;
template void samples_load(fbase*, const i32*, size_t, bool) noexcept;
template void samples_load(fbase*, const f32*, size_t, bool) noexcept;
template void samples_load(fbase*, const f64*, size_t, bool) noexcept;

template void samples_store(i16*, const fbase*, size_t, const audio_quantization&, bool) noexcept;
template void samples_store(i24*, const fbase*, size_t, const audio_quantization&, bool) noexcept;
template void samples_store(i32*, const fbase*, size_t, const audio_quantization&, bool) noexcept;
template void samples_store(f32*, const fbase*, size_t, const audio_quantization&, bool) noexcept;
template void samples_store(f64*, const fbase*, size_t, const audio_quantization&, bool) noexcept;

audio_sample_type audiofile_format::sample_type_lpcm() const
{
    if (bit_depth > 32)
        return audio_sample_type::unknown;
    else if (bit_depth > 24)
        return audio_sample_type::i32;
    else if (bit_depth > 16)
        return audio_sample_type::i24;
    else if (bit_depth > 8)
        return audio_sample_type::i16;
    return audio_sample_type::unknown;
}
audio_sample_type audiofile_format::sample_type() const
{
    if (codec == audiofile_codec::ieee_float)
    {
        if (bit_depth == 32)
            return audio_sample_type::f32;
        else if (bit_depth == 64)
            return audio_sample_type::f64;
    }
    else if (codec == audiofile_codec::lpcm)
    {
        if (bit_depth > 24)
            return audio_sample_type::i32;
        else if (bit_depth > 16)
            return audio_sample_type::i24;
        else if (bit_depth > 8)
            return audio_sample_type::i16;
    }
    return audio_sample_type::unknown;
}
bool audiofile_format::valid() const noexcept
{
    if (channels == 0 || channels > max_audio_channels)
        return false;
    if (sample_rate == 0)
        return false;
    if (codec == audiofile_codec::lpcm || codec == audiofile_codec::flac)
    {
        if (bit_depth <= 8 || bit_depth > 32)
            return false;
        if (codec == audiofile_codec::flac && channels > 8)
            return false;
    }
    else if (codec == audiofile_codec::ieee_float)
    {
        if (bit_depth != 32 && bit_depth != 64)
            return false;
    }
    else if (codec == audiofile_codec::alac)
    {
        if (bit_depth != 16 && bit_depth != 20 && bit_depth != 24 && bit_depth != 32)
            return false;
    }
    else
    {
        // if (bit_depth != 0)
        // return false;
    }
    return true;
}
} // namespace kfr
