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
#include <kfr/audio/data.hpp>
#include <kfr/dsp/units.hpp>

namespace kfr
{

template <bool IsInterleaved>
audio_data<IsInterleaved>::audio_data(std::span<fbase* const> pointers, size_t size)
    requires(!IsInterleaved)
    : channels(pointers.size()), data{}, size(size), capacity(size)
{
    KFR_ASSERT(channels > 0);
    KFR_ASSERT(channels <= max_audio_channels);
    std::copy(pointers.begin(), pointers.end(), data.begin());
}

template <bool IsInterleaved>
audio_data<IsInterleaved>::audio_data(fbase* pointer, size_t channels, size_t size)
    requires(IsInterleaved)
    : channels(channels), data(pointer), size(size), capacity(size)
{
    KFR_ASSERT(channels > 0);
    KFR_ASSERT(channels <= max_audio_channels);
}

template <bool IsInterleaved>
audio_data<IsInterleaved>::audio_data(size_t channels, size_t size)
    : channels(channels), size(size), capacity(size)
{
    KFR_ASSERT(channels > 0);
    KFR_ASSERT(channels <= max_audio_channels);
    if (!empty())
    {
        constexpr size_t sampleAlignment = IsInterleaved ? 1 : KFR_CACHE_LINE_SIZE / sizeof(fbase);
        std::shared_ptr<details::aligned_deallocator> dealloc(new details::aligned_deallocator{
            kfr::aligned_allocate<fbase>(channels * align_up(capacity, sampleAlignment)) });
        if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
audio_data<IsInterleaved>::audio_data(size_t channels, size_t size, fbase value) : audio_data(channels, size)
{
    fill(value);
}

template <bool IsInterleaved>
size_t audio_data<IsInterleaved>::total_samples() const noexcept
{
    return size * channel_count();
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::fill(fbase value)
{
    for_channel([value](univector_ref<fbase> data) { data = scalar(value); });
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::multiply(fbase value)
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

template <bool IsInterleaved>
void audio_data<IsInterleaved>::apply_gain_dB(fbase gain_db)
{
    multiply(dB_to_amp(gain_db));
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::normalize(fbase target_peak)
{
    if (empty())
        return;
    audio_stat s = stat();
    if (s.peak > fbase(0))
    {
        multiply(target_peak / s.peak);
    }
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::clamp(fbase min_val, fbase max_val)
{
    for_channel([min_val, max_val](univector_ref<fbase> data) { data = kfr::clamp(data, min_val, max_val); });
}

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::to_mono() const
{
    if (empty()) [[unlikely]]
        return audio_data<IsInterleaved>(1, 0);
    if (channels == 1) [[unlikely]]
        return clone();

    audio_data<IsInterleaved> result(1, size);
    result.position = position;

    if constexpr (IsInterleaved)
    {
        if (channels == 2) [[likely]]
        {
            block_process( //
                size, csizes<vector_width<fbase>, 1>,
                [in = this->data, out = result.data]<size_t w>(size_t offset, csize_t<w>) KFR_INLINE_LAMBDA
                {
                    vec<fbase, 2 * w> stereo = read<2 * w>(in + offset * 2);
                    vec<fbase, w> mono       = (even(stereo) + odd(stereo)) * fbase(0.5f);
                    write(out + offset, mono);
                });
            return result;
        }

        for (size_t i = 0; i < size; ++i)
        {
            fbase sum = fbase(0);
            for (size_t ch = 0; ch < channels; ++ch)
            {
                sum += data[i * channels + ch];
            }
            result.data[i] = sum / static_cast<fbase>(channels);
        }
    }
    else
    {
        if (channels == 2) [[likely]]
        {
            block_process( //
                size, csizes<vector_width<fbase>, 1>,
                [left = this->data[0], right = this->data[1],
                 out = result.data[0]]<size_t w>(size_t offset, csize_t<w>) KFR_INLINE_LAMBDA
                {
                    vec<fbase, w> l    = read<w>(left + offset);
                    vec<fbase, w> r    = read<w>(right + offset);
                    vec<fbase, w> mono = (l + r) * fbase(0.5f);
                    write(out + offset, mono);
                });
            return result;
        }

        const fbase scale                    = fbase(1) / static_cast<fbase>(channels);
        make_univector(result.data[0], size) = make_univector(data[0], size) * scale;
        for (size_t ch = 1; ch < channels; ++ch)
        {
            make_univector(result.data[0], size) += make_univector(data[ch], size) * scale;
        }
    }
    return result;
}

template <bool IsInterleaved>
audio_data<true> audio_data<IsInterleaved>::to_interleaved() const
{
    if constexpr (IsInterleaved)
    {
        return clone();
    }
    else
    {
        return audio_data<true>(*this);
    }
}

template <bool IsInterleaved>
audio_data<false> audio_data<IsInterleaved>::to_planar() const
{
    if constexpr (!IsInterleaved)
    {
        return clone();
    }
    else
    {
        return audio_data<false>(*this);
    }
}

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::select_channel(size_t ch) const
    requires(!IsInterleaved)
{
    return select_channels(ch, ch);
}

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::select_channels(size_t min_ch, size_t max_ch) const
    requires(!IsInterleaved)
{
    KFR_LOGIC_CHECK(min_ch <= max_ch, "min_ch must be <= max_ch");
    KFR_LOGIC_CHECK(max_ch < channels, "max_ch must be < channels");

    size_t count = max_ch - min_ch + 1;
    audio_data<false> result;
    result.channels    = static_cast<uint32_t>(count);
    result.size        = size;
    result.capacity    = capacity;
    result.position    = position;
    result.deallocator = deallocator;
    for (size_t i = 0; i < count; ++i)
    {
        result.data[i] = data[min_ch + i];
    }
    return result;
}

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::clone() const
{
    if (empty())
        return audio_data<IsInterleaved>(channels, size);

    audio_data<IsInterleaved> result(channels, size);
    result.position = position;
    if constexpr (IsInterleaved)
    {
        std::memcpy(result.data, data, size * channels * sizeof(fbase));
    }
    else
    {
        for (size_t ch = 0; ch < channels; ++ch)
        {
            std::memcpy(result.data[ch], data[ch], size * sizeof(fbase));
        }
    }
    return result;
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::clear()
{
    size     = 0;
    position = 0;
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::resize(size_t new_size)
{
    if (new_size <= capacity)
    {
        size = new_size;
        return;
    }
    reserve(details::round_capacity(new_size));
    size = new_size;
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::resize(size_t new_size, fbase value)
{
    size_t old_size = size;
    resize(new_size);
    if (new_size > old_size)
    {
        if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
void audio_data<IsInterleaved>::reserve(size_t new_capacity)
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
        if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
void audio_data<IsInterleaved>::append(const audio_data& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
void audio_data<IsInterleaved>::prepend(const audio_data& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
void audio_data<IsInterleaved>::append(const audio_data<!IsInterleaved>& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (IsInterleaved)
    {
        samples_store(data + old_size * channel_count(), other.pointers(), other.channel_count(), other.size);
    }
    else
    {
        samples_load(pointers() + old_size, other.data, other.channel_count(), other.size);
    }
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::prepend(const audio_data<!IsInterleaved>& other)
{
    if (empty())
    {
        *this = other;
        return;
    }
    const size_t old_size = size;
    resize(size + other.size);
    if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
void audio_data<IsInterleaved>::swap(audio_data& other) noexcept
{
    std::swap(channels, other.channels);
    std::swap(data, other.data);
    std::swap(size, other.size);
    std::swap(capacity, other.capacity);
    std::swap(position, other.position);
    std::swap(deallocator, other.deallocator);
}

template <bool IsInterleaved>
void audio_data<IsInterleaved>::reset()
{
    *this = audio_data<IsInterleaved>{};
}

template <bool IsInterleaved>
size_t audio_data<IsInterleaved>::channel_count() const noexcept
{
    return channels;
}

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::truncate(size_t length) const
{
    return slice(0, length);
}

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::slice(size_t start, size_t length) const
{
    KFR_LOGIC_CHECK(start <= size, "Slice out of range");
    audio_data result = *this;
    result.size       = std::min(length, size - start);
    if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
audio_data<IsInterleaved> audio_data<IsInterleaved>::slice_past_end(size_t length)
{
    reserve(details::round_capacity(size + length));
    audio_data result = *this;
    result.size       = length;
    if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
size_t audio_data<IsInterleaved>::find_peak() const noexcept
{
    size_t peakIndex = 0;
    fbase peakValue  = 0.0;
    for (size_t i = 0; i < size; ++i)
    {
        fbase sum = fbase(0);
        for (size_t ch = 0; ch < channels; ++ch)
        {
            if constexpr (IsInterleaved)
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

template <bool IsInterleaved>
audio_stat audio_data<IsInterleaved>::stat() const noexcept
{
    audio_stat result{ 0, 0 };
    if (empty())
    {
        return result;
    }
    if constexpr (IsInterleaved)
    {
        result.peak = std::max(result.peak, absmaxof(interleaved()));
        result.rms  = rms(interleaved());
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

template <bool IsInterleaved>
bool audio_data<IsInterleaved>::is_silent(fbase threshold) const noexcept
{
    if constexpr (IsInterleaved)
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
