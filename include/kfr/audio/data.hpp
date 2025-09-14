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
#pragma once

#include <array>
#include <memory>
#include <map>
#include <string>
#include <kfr/test/assert.hpp>
#include <kfr/base.hpp>
#include <kfr/dsp/speaker.hpp>

namespace kfr
{

#ifndef KFR_MAX_AUDIO_CHANNELS
constexpr inline size_t max_audio_channels = 16; /**< Maximum number of channels in audio data. */
#else
constexpr inline size_t max_audio_channels =
    KFR_MAX_AUDIO_CHANNELS; /**< Maximum number of channels in audio data. */
static_assert(max_audio_channels >= 2, "KFR_MAX_AUDIO_CHANNELS must be >= 2");
static_assert(max_audio_channels <= 64, "KFR_MAX_AUDIO_CHANNELS must be <= 64");
#endif

template <typename T>
using chan = std::array<T, max_audio_channels>;

enum class audio_metadata_type : int
{
    none       = 0,
    audio_file = 1, // audiofile_metadata
};

struct audio_metadata
{
    audio_metadata_type type    = audio_metadata_type::none; /**< Type of metadata. */
    uint32_t channels           = 0; /**< Number of channels. */
    uint32_t sample_rate        = 0; /**< Sample rate in Hz. */
    SpeakerArrangement speakers = SpeakerArrangement::None; /**< Speaker arrangement. */
};

enum class audiofile_container : uint8_t
{
    unknown = 0,
    wave, ///< RIFF WAVE
    w64, ///< Sony WAVE64
    rf64, ///< RF64 by EBU
    bw64, ///< BW64 as per EBU Tech 3285 v2 (no metadata support yet)
    flac, ///< FLAC
    caf, ///< Apple CAF
    aiff, /// AIFF/C
    mp3, ///< MPEG audio
};

enum class audiofile_codec : uint8_t
{
    unknown = 0,
    lpcm,
    ieee_float,
    flac,
    alac,
    mp3,
};

enum class audiofile_endianness : uint8_t
{
    little,
    big,
};

constexpr bool is_single_codec(audiofile_container container)
{
    switch (container)
    {
    case audiofile_container::flac:
    case audiofile_container::mp3:
        return true;
    default:
        return false;
    }
}

using metadata_map = std::map<std::string, std::string>;

struct audiofile_metadata : public audio_metadata
{
    constexpr static audio_metadata_type metadata_type = audio_metadata_type::audio_file;

    audiofile_metadata() noexcept { type = audio_metadata_type::audio_file; }

    uint64_t total_frames;
    audiofile_container container   = audiofile_container::unknown; /**< Container format. */
    audiofile_codec codec           = audiofile_codec::unknown; /**< Audio codec. */
    audiofile_endianness endianness = audiofile_endianness::little; /**< Endianness of the audio data. */
    uint8_t bit_depth               = 0; /**< Bits per sample. */
    metadata_map metadata; /**< Key-value metadata pairs. */

    size_t bytes_per_pcm_frame() const noexcept { return channels * ((bit_depth + 7) / 8); }

    bool compatible(const audiofile_metadata& format) const noexcept
    {
        if (channels != format.channels)
            return false;
        if (sample_rate != format.sample_rate)
            return false;
        return true;
    }

    bool valid() const noexcept
    {
        if (channels == 0 || channels > max_audio_channels)
            return false;
        if (sample_rate == 0)
            return false;
        if (bit_depth > 64)
            return false;
        return true;
    }
};

struct audio_stat
{
    fbase peak;
    fbase rms;
};

struct audio_data
{
    const audio_metadata* metadata; /**< Metadata associated with the audio data. */
    chan<fbase*> data{}; /**< Pointers to channel data. */
    size_t size; /**< Number of samples per channel. */
    size_t capacity; /**< Allocated capacity per channel. */
    int64_t position = 0; /**< Position of the first sample in the audio data. */
    std::shared_ptr<void> deallocator; /**< Deallocator for the data. */

    template <std::derived_from<audio_metadata> T>
    [[nodiscard]] const T* typed_metadata() const noexcept
    {
        if (metadata->type == T::metadata_type)
            return static_cast<const T*>(metadata);
        return nullptr;
    }

    [[nodiscard]] constexpr audio_data() noexcept : metadata(nullptr), size(0), capacity(0) {}

    audio_data(const audio_data&) noexcept            = default;
    audio_data(audio_data&&) noexcept                 = default;
    audio_data& operator=(const audio_data&) noexcept = default;
    audio_data& operator=(audio_data&&) noexcept      = default;

    template <std::invocable Fn>
    [[nodiscard]] audio_data(const audio_metadata* metadata, chan<fbase*> data, size_t size, Fn&& deallocator)
        : metadata(metadata), data(data), size(size), capacity(size),
          deallocator(new lambda_deallocator<Fn>{ std::forward<Fn>(deallocator) })
    {
        KFR_ASSERT(metadata);
    }
    template <std::invocable Fn>
    [[nodiscard]] audio_data(const audio_metadata* metadata, std::initializer_list<fbase*> data, size_t size,
                             Fn&& deallocator)
        : audio_data(metadata, from_initializer_list(data), size, std::forward<Fn>(deallocator))
    {
    }
    [[nodiscard]] audio_data(const audio_metadata* metadata, chan<fbase*> data, size_t size)
        : metadata(metadata), data(data), size(size), capacity(size)
    {
        KFR_ASSERT(metadata);
    }
    [[nodiscard]] audio_data(const audio_metadata* metadata, std::initializer_list<fbase*> data, size_t size)
        : audio_data(metadata, from_initializer_list(data), size)
    {
    }
    [[nodiscard]] audio_data(const audio_metadata* metadata, size_t size = 0);
    [[nodiscard]] audio_data(const audio_metadata* metadata, size_t size, fbase value);

    static chan<fbase*> from_initializer_list(std::initializer_list<fbase*> data);

    void reset();

    void fill(fbase value);

    void multiply(fbase value);

    void resize(size_t new_size);

    void clear();

    void resize(size_t new_size, fbase value);

    void reserve(size_t new_capacity);

    void append(const audio_data& other);

    void prepend(const audio_data& other);

    friend void swap(audio_data& a, audio_data& b) noexcept { a.swap(b); }

    void swap(audio_data& other) noexcept;

    struct flat_deallocator
    {
        fbase* ptr;
        ~flat_deallocator() { kfr::aligned_deallocate(ptr); }
    };
    template <typename Fn>
    struct lambda_deallocator
    {
        Fn fn;
        ~lambda_deallocator() { fn(); }
    };

    [[nodiscard]] bool empty() const noexcept { return !metadata || size == 0; }

    [[nodiscard]] univector_ref<fbase> channel(size_t index) const
    {
        KFR_ASSERT(metadata);
        KFR_ASSERT(index < metadata->channels);
        return univector_ref<fbase>(data[index], size);
    }

    [[nodiscard]] fbase* const* pointers() const noexcept;

    [[nodiscard]] size_t channel_count() const noexcept
    {
        KFR_ASSERT(metadata);
        return metadata->channels;
    }

    [[nodiscard]] audio_data truncate(size_t length) const { return slice(0, length); }

    [[nodiscard]] audio_data slice(size_t start, size_t length = SIZE_MAX) const;

    [[nodiscard]] audio_stat stat() const noexcept;
};

enum class audio_dithering
{
    none,
    rectangular,
    triangular,
};

struct audio_dithering_state
{
    audio_dithering dithering;
    fbase scale;
    mutable std::mt19937_64 rnd{ std::random_device{}() };
    mutable std::uniform_real_distribution<fbase> dist{ fbase(-0.5), fbase(+0.5) };
    fbase operator()() const
    {
        switch (dithering)
        {
        case audio_dithering::rectangular:
            return dist(rnd) * scale;
        case audio_dithering::triangular:
            return (dist(rnd) + dist(rnd)) * scale;
        default:
            return fbase(0.0);
        }
    }
};

struct audio_quantinization
{
    audio_dithering_state dither;

    audio_quantinization(int bit_depth, audio_dithering dithering)
        : dither{ dithering, fbase(1.0) / (1ull << bit_depth) }
    {
    }
};

namespace details
{

inline void cvt_sample(int32_t& sample, fbase value, const audio_quantinization& quant)
{
    sample = std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * fbase(2147483647.0));
}
inline void cvt_sample(int16_t& sample, fbase value, const audio_quantinization& quant)
{
    sample = std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * fbase(32767.0));
}
inline void cvt_sample(kfr::i24& sample, fbase value, const audio_quantinization& quant)
{
    sample = std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * fbase(8388607.0));
}
inline void cvt_sample(float& sample, fbase value, const audio_quantinization&) { sample = value; }
inline void cvt_sample(double& sample, fbase value, const audio_quantinization&) { sample = value; }

inline void cvt_sample(fbase& value, double sample) { value = sample; }
inline void cvt_sample(fbase& value, float sample) { value = sample; }

inline void cvt_sample(fbase& value, int16_t sample) { value = sample / fbase(32767.0); }
inline void cvt_sample(fbase& value, kfr::i24 sample) { value = sample / fbase(8388607.0); }
inline void cvt_sample(fbase& value, int32_t sample) { value = sample / fbase(2147483647.0); }

struct stdFILE_deleter
{
    void operator()(std::FILE* f) const
    {
        if (f)
            std::fclose(f);
    }
};

} // namespace details

/// @brief Interleaves and converts audio samples
template <typename Tout>
void interleave_samples(Tout* out, const fbase* const in[], size_t channels, size_t size,
                        const audio_quantinization& quantinization)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            details::cvt_sample(out[i * channels + ch], in[ch][i], quantinization);
    }
}

template <typename Tin>
void deinterleave_samples(fbase* const out[], const Tin* in, size_t channels, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            details::cvt_sample(out[ch][i], in[i * channels + ch]);
    }
}

} // namespace kfr
