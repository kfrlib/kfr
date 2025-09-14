/** @addtogroup audio
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
#include <kfr/audio/data.hpp>

namespace kfr
{

static size_t round_capacity(size_t size) { return std::bit_ceil(size); }

audio_interleaved_data::audio_interleaved_data(const audio_metadata* metadata, size_t size)
    : metadata(metadata), size(size), capacity(size)
{
    KFR_ASSERT(metadata);
    if (!empty())
    {
        std::shared_ptr<details::aligned_deallocator> dealloc(
            new details::aligned_deallocator{ kfr::aligned_allocate<fbase>(metadata->channels * capacity) });
        data        = dealloc->ptr;
        deallocator = std::move(dealloc);
    }
}
audio_interleaved_data::audio_interleaved_data(const audio_metadata* metadata, size_t size, fbase value)
    : audio_interleaved_data(metadata, size)
{
    fill(value);
}

void audio_interleaved_data::reset() { *this = {}; }

void audio_interleaved_data::fill(fbase value) { interlaved() = scalar(value); }

void audio_interleaved_data::multiply(fbase value)
{
    if (value == fbase(1))
        return;
    if (value == fbase(0))
    {
        fill(fbase(0));
        return;
    }
    interlaved() *= scalar(value);
}

void audio_interleaved_data::resize(size_t new_size) {}

void audio_interleaved_data::clear()
{
    size     = 0;
    position = 0;
}

void audio_interleaved_data::resize(size_t new_size, fbase value)
{
    KFR_ASSERT(metadata);
    if (new_size <= capacity)
    {
        size = new_size;
        return;
    }
    reserve(round_capacity(new_size));
    size = new_size;
}

void audio_interleaved_data::reserve(size_t new_capacity)
{
    KFR_ASSERT(metadata);
    if (new_capacity <= capacity)
    {
        capacity = new_capacity;
        return;
    }

    audio_interleaved_data result(metadata, new_capacity);
    result.size     = size;
    result.position = position;
    if (size > 0)
    {
        if (result.data)
            std::memcpy(result.data, data, size * channel_count() * sizeof(fbase));
    }
    swap(result);
}

void audio_interleaved_data::append(const audio_interleaved_data& other)
{
    if (!metadata)
    {
        *this = other;
        return;
    }
    KFR_ASSERT(metadata == other.metadata);
    const size_t old_size = size;
    resize(size + other.size);
    std::memcpy(data + old_size * channel_count(), other.data, other.size * channel_count() * sizeof(fbase));
}

void audio_interleaved_data::prepend(const audio_interleaved_data& other)
{
    if (!metadata)
    {
        *this = other;
        return;
    }
    KFR_ASSERT(metadata == other.metadata);
    const size_t old_size = size;
    resize(size + other.size);
    std::memmove(data + other.size * channel_count(), data, old_size * channel_count() * sizeof(fbase));
    std::memcpy(data, other.data, other.size * channel_count() * sizeof(fbase));
    position -= other.size;
}

void audio_interleaved_data::swap(audio_interleaved_data& other) noexcept
{
    std::swap(metadata, other.metadata);
    std::swap(data, other.data);
    std::swap(size, other.size);
    std::swap(capacity, other.capacity);
    std::swap(position, other.position);
    std::swap(deallocator, other.deallocator);
}

audio_interleaved_data audio_interleaved_data::slice(size_t start, size_t length) const
{
    KFR_ASSERT(metadata);
    KFR_LOGIC_CHECK(start <= size, "Slice out of range");
    audio_interleaved_data result = *this;
    result.size                   = std::min(length, size - start);
    result.data += start * channel_count();
    result.position += start;
    return result;
}

audio_stat audio_interleaved_data::stat() const noexcept
{
    KFR_ASSERT(metadata);
    audio_stat result{ 0, 0 };
    if (empty())
    {
        return result;
    }
    result.peak = std::max(result.peak, absmaxof(interlaved()));
    result.rms  = rms(interlaved());
    return result;
}

audio_data::audio_data(const audio_metadata* metadata, size_t size)
    : metadata(metadata), size(size), capacity(size)
{
    KFR_ASSERT(metadata);
    if (!empty())
    {
        std::shared_ptr<details::aligned_deallocator> dealloc(
            new details::aligned_deallocator{ kfr::aligned_allocate<fbase>(metadata->channels * capacity) });
        for (uint32_t i = 0; i < metadata->channels; ++i)
        {
            data[i] = dealloc->ptr + i * capacity;
        }
        deallocator = std::move(dealloc);
    }
}
audio_data::audio_data(const audio_metadata* metadata, size_t size, fbase value) : audio_data(metadata, size)
{
    fill(value);
}
chan<fbase*> audio_data::from_initializer_list(std::initializer_list<fbase*> data)
{
    KFR_ASSERT(data.size() <= max_audio_channels);
    chan<fbase*> result{};
    std::copy(data.begin(), data.end(), result.begin());
    return result;
}
void audio_data::fill(fbase value)
{
    KFR_ASSERT(metadata);
    for (uint32_t ch = 0; ch < metadata->channels; ++ch)
    {
        make_univector(data[ch], size) = scalar(value);
    }
}
void audio_data::multiply(fbase value)
{
    KFR_ASSERT(metadata);
    if (value == fbase(1))
        return;
    if (value == fbase(0))
    {
        fill(fbase(0));
        return;
    }
    for (uint32_t ch = 0; ch < metadata->channels; ++ch)
    {
        make_univector(data[ch], size) *= scalar(value);
    }
}

void audio_data::resize(size_t new_size)
{
    KFR_ASSERT(metadata);
    if (new_size <= capacity)
    {
        size = new_size;
        return;
    }
    reserve(round_capacity(new_size));
    size = new_size;
}
void audio_data::clear()
{
    size     = 0;
    position = 0;
}
void audio_data::resize(size_t new_size, fbase value)
{
    KFR_ASSERT(metadata);
    const size_t old_size = size;
    resize(new_size);
    if (new_size > old_size)
    {
        for (uint32_t ch = 0; ch < metadata->channels; ++ch)
        {
            make_univector(data[ch] + old_size, new_size - old_size) = scalar(value);
        }
    }
}

void audio_data::reserve(size_t new_capacity)
{
    KFR_ASSERT(metadata);
    if (new_capacity <= capacity)
    {
        capacity = new_capacity;
        return;
    }

    audio_data result(metadata, new_capacity);
    result.size     = size;
    result.position = position;
    if (size > 0)
    {
        for (uint32_t i = 0; i < metadata->channels; ++i)
        {
            if (result.data[i])
                std::memcpy(result.data[i], data[i], size * sizeof(fbase));
        }
    }
    swap(result);
}
void audio_data::append(const audio_data& other)
{
    if (!metadata)
    {
        *this = other;
        return;
    }
    KFR_ASSERT(metadata == other.metadata);
    const size_t old_size = size;
    resize(size + other.size);
    for (uint32_t ch = 0; ch < metadata->channels; ++ch)
    {
        std::memcpy(data[ch] + old_size, other.data[ch], other.size * sizeof(fbase));
    }
}
void audio_data::prepend(const audio_data& other)
{
    if (!metadata)
    {
        *this = other;
        return;
    }
    KFR_ASSERT(metadata == other.metadata);
    const size_t old_size = size;
    resize(size + other.size);
    for (uint32_t ch = 0; ch < metadata->channels; ++ch)
    {
        std::memmove(data[ch] + other.size, data[ch], old_size * sizeof(fbase));
        std::memcpy(data[ch], other.data[ch], other.size * sizeof(fbase));
    }
    position -= other.size;
}
void audio_data::swap(audio_data& other) noexcept
{
    std::swap(metadata, other.metadata);
    std::swap(data, other.data);
    std::swap(size, other.size);
    std::swap(capacity, other.capacity);
    std::swap(position, other.position);
    std::swap(deallocator, other.deallocator);
}
fbase* const* audio_data::pointers() const noexcept { return data.data(); }

audio_data audio_data::slice(size_t start, size_t length) const
{
    KFR_ASSERT(metadata);
    KFR_LOGIC_CHECK(start <= size, "Slice out of range");
    audio_data result = *this;
    result.size       = std::min(length, size - start);
    for (uint32_t ch = 0; ch < metadata->channels; ++ch)
    {
        result.data[ch] += start;
    }
    result.position += start;
    return result;
}
audio_stat audio_data::stat() const noexcept
{
    KFR_ASSERT(metadata);
    audio_stat result{ 0, 0 };
    if (empty())
    {
        return result;
    }
    for (uint32_t ch = 0; ch < metadata->channels; ++ch)
    {
        result.peak = std::max(result.peak, absmaxof(channel(ch)));
        result.rms += sumsqr(channel(ch));
    }
    result.rms = std::sqrt(result.rms / (size * metadata->channels));
    return result;
}

void audio_data::reset() { *this = {}; }

audio_interleaved_data::audio_interleaved_data(const audio_data& deinterleaved)
    : audio_interleaved_data(deinterleaved.metadata, deinterleaved.size)
{
    KFR_ASSERT(metadata);
    if (!deinterleaved.empty())
    {
        interleave_samples(data, deinterleaved.pointers(), deinterleaved.channel_count(), deinterleaved.size);
    }
}

audio_data::audio_data(const audio_interleaved_data& interleaved)
    : audio_data(interleaved.metadata, interleaved.size)
{
    KFR_ASSERT(metadata);
    if (!interleaved.empty())
    {
        deinterleave_samples(pointers(), interleaved.data, interleaved.channel_count(), interleaved.size);
    }
}
} // namespace kfr
