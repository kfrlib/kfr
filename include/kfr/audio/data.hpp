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
#pragma once

#include <array>
#include <memory>
#include <map>
#include <bit>
#include <span>
#include <concepts>
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

template <typename T, size_t Interleaved>
using chan = std::conditional_t<Interleaved, T, std::array<T, max_audio_channels>>;

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

struct audio_quantization;

template <typename Tin>
void samples_load(fbase* out, const Tin* in, size_t size, bool swap_bytes = false) noexcept;
template <typename Tin>
void samples_load(fbase* const out[], const Tin* in, size_t channels, size_t size,
                  bool swap_bytes = false) noexcept;

template <typename Tout>
void samples_store(Tout* out, const fbase* in, size_t size, const audio_quantization& quantization,
                   bool swap_bytes = false) noexcept;
template <typename Tout>
void samples_store(Tout* out, const fbase* const in[], size_t channels, size_t size,
                   const audio_quantization& quantization, bool swap_bytes = false) noexcept;
template <typename Tout>
void samples_store(Tout* out, const fbase* const in[], size_t channels, size_t size,
                   bool swap_bytes = false) noexcept;

KFR_INTRINSIC void samples_load(audio_sample_type type, fbase* out, const std::byte* in, size_t size,
                                bool swap_bytes = false) noexcept
{
    switch (type)
    {
    case audio_sample_type::i16:
        samples_load(out, reinterpret_cast<const i16*>(in), size, swap_bytes);
        break;
    case audio_sample_type::i24:
        samples_load(out, reinterpret_cast<const i24*>(in), size, swap_bytes);
        break;
    case audio_sample_type::i32:
        samples_load(out, reinterpret_cast<const i32*>(in), size, swap_bytes);
        break;
    case audio_sample_type::f32:
        samples_load(out, reinterpret_cast<const f32*>(in), size, swap_bytes);
        break;
    case audio_sample_type::f64:
        samples_load(out, reinterpret_cast<const f64*>(in), size, swap_bytes);
        break;
    default:
        KFR_UNREACHABLE;
    }
}
KFR_INTRINSIC void samples_load(audio_sample_type type, fbase* const out[], const std::byte* in,
                                size_t channels, size_t size, bool swap_bytes = false) noexcept
{
    switch (type)
    {
    case audio_sample_type::i16:
        samples_load(out, reinterpret_cast<const i16*>(in), channels, size, swap_bytes);
        break;
    case audio_sample_type::i24:
        samples_load(out, reinterpret_cast<const i24*>(in), channels, size, swap_bytes);
        break;
    case audio_sample_type::i32:
        samples_load(out, reinterpret_cast<const i32*>(in), channels, size, swap_bytes);
        break;
    case audio_sample_type::f32:
        samples_load(out, reinterpret_cast<const f32*>(in), channels, size, swap_bytes);
        break;
    case audio_sample_type::f64:
        samples_load(out, reinterpret_cast<const f64*>(in), channels, size, swap_bytes);
        break;
    default:
        KFR_UNREACHABLE;
    }
}

KFR_INTRINSIC void samples_store(audio_sample_type type, std::byte* out, const fbase* in, size_t size,
                                 const audio_quantization& quantization, bool swap_bytes = false) noexcept
{
    switch (type)
    {
    case audio_sample_type::i16:
        samples_store(reinterpret_cast<i16*>(out), in, size, quantization, swap_bytes);
        break;
    case audio_sample_type::i24:
        samples_store(reinterpret_cast<i24*>(out), in, size, quantization, swap_bytes);
        break;
    case audio_sample_type::i32:
        samples_store(reinterpret_cast<i32*>(out), in, size, quantization, swap_bytes);
        break;
    case audio_sample_type::f32:
        samples_store(reinterpret_cast<f32*>(out), in, size, quantization, swap_bytes);
        break;
    case audio_sample_type::f64:
        samples_store(reinterpret_cast<f64*>(out), in, size, quantization, swap_bytes);
        break;
    default:
        KFR_UNREACHABLE;
    }
}
KFR_INTRINSIC void samples_store(audio_sample_type type, std::byte* out, const fbase* const in[],
                                 size_t channels, size_t size, const audio_quantization& quantization,
                                 bool swap_bytes = false) noexcept
{
    switch (type)
    {
    case audio_sample_type::i16:
        samples_store(reinterpret_cast<i16*>(out), in, channels, size, quantization, swap_bytes);
        break;
    case audio_sample_type::i24:
        samples_store(reinterpret_cast<i24*>(out), in, channels, size, quantization, swap_bytes);
        break;
    case audio_sample_type::i32:
        samples_store(reinterpret_cast<i32*>(out), in, channels, size, quantization, swap_bytes);
        break;
    case audio_sample_type::f32:
        samples_store(reinterpret_cast<f32*>(out), in, channels, size, quantization, swap_bytes);
        break;
    case audio_sample_type::f64:
        samples_store(reinterpret_cast<f64*>(out), in, channels, size, quantization, swap_bytes);
        break;
    default:
        KFR_UNREACHABLE;
    }
}
KFR_INTRINSIC void samples_store(audio_sample_type type, std::byte* out, const fbase* const in[],
                                 size_t channels, size_t size, bool swap_bytes = false) noexcept
{
    switch (type)
    {
    case audio_sample_type::i16:
        samples_store(reinterpret_cast<i16*>(out), in, channels, size, swap_bytes);
        break;
    case audio_sample_type::i24:
        samples_store(reinterpret_cast<i24*>(out), in, channels, size, swap_bytes);
        break;
    case audio_sample_type::i32:
        samples_store(reinterpret_cast<i32*>(out), in, channels, size, swap_bytes);
        break;
    case audio_sample_type::f32:
        samples_store(reinterpret_cast<f32*>(out), in, channels, size, swap_bytes);
        break;
    case audio_sample_type::f64:
        samples_store(reinterpret_cast<f64*>(out), in, channels, size, swap_bytes);
        break;
    default:
        KFR_UNREACHABLE;
    }
}

struct audiofile_format
{
    audiofile_container container   = audiofile_container::unknown; /**< Container format. */
    audiofile_codec codec           = audiofile_codec::unknown; /**< Audio codec. */
    audiofile_endianness endianness = audiofile_endianness::little; /**< Endianness of the audio data. */
    uint8_t bit_depth               = 0; /**< Bits per sample. */

    uint32_t channels            = 0; /**< Number of channels. */
    uint32_t sample_rate         = 0; /**< Sample rate in Hz. */
    speaker_arrangement speakers = speaker_arrangement::None; /**< Speaker arrangement. */

    uint64_t total_frames = 0;
    metadata_map metadata; /**< Key-value metadata pairs. */

    size_t bytes_per_pcm_frame() const noexcept { return channels * ((bit_depth + 7) / 8); }

    bool valid() const noexcept;

    audio_sample_type sample_type() const;
    audio_sample_type sample_type_lpcm() const;

    bool operator==(const audiofile_format& other) const noexcept = default;
};

struct audio_stat
{
    fbase peak;
    fbase rms;
};

namespace details
{

inline size_t round_capacity(size_t size) { return std::bit_ceil(size); }

struct aligned_deallocator
{
    fbase* ptr;
    ~aligned_deallocator() { kfr::aligned_deallocate(ptr); }
};
template <typename Fn>
struct lambda_deallocator
{
    Fn fn;
    ~lambda_deallocator() { fn(); }
};
} // namespace details

template <typename T>
struct strided_channel
{
    T* data;
    size_t size;
    size_t stride;
};

template <typename T>
struct expression_traits<strided_channel<T>> : public expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;
    constexpr static shape<dims> get_shape(const strided_channel<T>& u) { return shape<1>(u.size); }
    constexpr static shape<dims> get_shape() { return shape<1>{ undefined_size }; }
};

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const strided_channel<T>& self, const shape<1>& index,
                                     const axis_params<0, N>&)
{
    return gather_stride<N>(self.data + index.front() * self.stride, self.stride);
}

template <typename T, size_t N>
KFR_INTRINSIC void set_elements(strided_channel<T>& self, const shape<1>& index, const axis_params<0, N>&,
                                const std::type_identity_t<vec<T, N>>& value)
{
    scatter_stride<N>(self.data + index.front() * self.stride, value, self.stride);
}

template <bool Interleaved = false>
struct audio_data
{
    uint32_t channels = 0; /**< Number of channels. */
    chan<fbase*, Interleaved> data{}; /**< Pointers to channel data. */
    size_t size; /**< Number of samples per channel. */
    size_t capacity; /**< Allocated capacity per channel. */
    int64_t position = 0; /**< Position of the first sample in the audio data. */
    std::shared_ptr<void> deallocator; /**< Deallocator for the data. */

    bool operator==(const audio_data& other) const noexcept = default;

    [[nodiscard]] constexpr audio_data() noexcept : size(0), capacity(0) {}

    [[nodiscard]] audio_data(const audio_data<!Interleaved>& other) : audio_data(other.channels, other.size)
    {
        if (other.empty())
            return;
        if constexpr (Interleaved)
        {
            samples_store(data, other.pointers(), other.channel_count(), other.size);
        }
        else
        {
            samples_load(pointers(), other.data, other.channel_count(), other.size);
        }
    }

    audio_data(const audio_data&) noexcept            = default;
    audio_data(audio_data&&) noexcept                 = default;
    audio_data& operator=(const audio_data&) noexcept = default;
    audio_data& operator=(audio_data&&) noexcept      = default;

    template <std::invocable Fn>
    [[nodiscard]] audio_data(std::span<fbase* const> pointers, size_t size, Fn&& deallocator)
        requires(!Interleaved)
        : audio_data(pointers, size)
    {
        deallocator.reset(new details::lambda_deallocator<Fn>{ std::forward<Fn>(deallocator) });
    }

    [[nodiscard]] audio_data(std::span<fbase* const> pointers, size_t size)
        requires(!Interleaved);

    [[nodiscard]] explicit audio_data(size_t channels, size_t size = 0);

    [[nodiscard]] audio_data(size_t channels, size_t size, fbase value);

    /**
     * @brief Calculates the total number of audio samples.
     *
     * This function computes the total number of samples by multiplying the size
     * (number of frames) by the number of channels in the audio data.
     *
     * @return The total number of samples as a size_t value.
     */
    size_t total_samples() const noexcept;

    /**
     * @brief Resets the object to its default state.
     *
     * This function assigns a default-constructed instance of the object
     * to itself, effectively resetting all its members to their default values.
     */
    void reset();

    /**
     * @brief Fills the audio data with the specified value.
     *
     * This function sets all elements of the audio data to the given value.
     *
     * @param value The value to fill the audio data with.
     */
    void fill(fbase value);

    /**
     * @brief Multiplies the audio data by a specified scalar value.
     *
     * This function scales the audio data by the given factor, modifying
     * the current data in place.
     *
     * @param value The scalar value to multiply the audio data by.
     */
    void multiply(fbase value);

    /**
     * Clears all audio data, leaving the container empty (size == 0).
     */
    void clear();

    void resize(size_t new_size);

    void resize(size_t new_size, fbase value);

    void reserve(size_t new_capacity);

    void append(const audio_data& other);

    void prepend(const audio_data& other);

    void append(const audio_data<!Interleaved>& other);

    void prepend(const audio_data<!Interleaved>& other);

    friend void swap(audio_data& a, audio_data& b) noexcept { a.swap(b); }

    void swap(audio_data& other) noexcept;

    [[nodiscard]] bool empty() const noexcept { return channels == 0 || size == 0; }

    /**
     * @brief Retrieves a reference to the audio data of a specific channel.
     *
     * This function returns a `univector_ref<fbase>` representing the audio data
     * for the specified channel index. It is only available when the audio data
     * is not interleaved (i.e., `Interleaved` is false).
     *
     * @param index The index of the channel to retrieve. Must be less than the
     *              total number of channels.
     * @return A `univector_ref<fbase>` representing the audio data for the specified channel.
     *
     */
    [[nodiscard]] univector_ref<fbase> channel(size_t index) const noexcept
        requires(!Interleaved)
    {
        KFR_ASSERT(index < channels);
        return univector_ref<fbase>(data[index], size);
    }
    [[nodiscard]] strided_channel<fbase> channel(size_t index) const noexcept
        requires(Interleaved)
    {
        KFR_ASSERT(index < channels);
        return strided_channel<fbase>{ data + index, size, channels };
    }

    /**
     * @brief Returns a reference to the interleaved audio data.
     *
     * This function provides access to the interleaved audio data as a `univector_ref<fbase>`.
     * It is only available when the audio data is interleaved (i.e., `Interleaved` is true).
     *
     * @return A `univector_ref<fbase>` representing the interleaved audio data.
     *         The size of the returned reference is calculated as `size * channels`.
     */
    [[nodiscard]] univector_ref<fbase> interlaved() const noexcept
        requires(Interleaved)
    {
        return univector_ref<fbase>(data, size * channels);
    }

    /**
     * @brief Retrieves an array of pointers to the base type of the audio data.
     *
     * This function returns a pointer to the underlying data array, which contains
     * pointers to the base type (`fbase*`). It is only available when the audio data
     * is not interleaved (i.e., `Interleaved` is false).
     *
     * @return A pointer to the array of `fbase*` representing the audio data.
     */
    [[nodiscard]] fbase* const* pointers() const noexcept
        requires(!Interleaved)
    {
        return data.data();
    }

    /**
     * @brief Retrieves the number of audio channels.
     */
    [[nodiscard]] size_t channel_count() const noexcept;

    /**
     * @brief Creates a slice of the audio data starting at a specified position and with a specified length.
     *
     * @param start The starting position of the slice (in samples).
     * @param length The length of the slice (in samples). Defaults to SIZE_MAX, which means the slice will
     * extend to the end of the audio data if not specified.
     * @return audio_data A new audio_data object representing the sliced portion of the original data.
     *
     * @note The function ensures that the slice does not exceed the bounds of the audio data. If the
     * requested length exceeds the available data, the slice will be truncated to fit within the bounds.
     *
     * @throws std::logic_error If the starting position is out of range.
     *
     * @details
     * - The position of the resulting slice is updated to reflect the starting position of the slice.
     */
    [[nodiscard]] audio_data slice(size_t start, size_t length = SIZE_MAX) const;

    /**
     * @brief Truncates the audio data to the specified length.
     *
     * This function creates a new audio_data object that contains only the
     * first `length` samples from the current audio data. If the specified
     * length is greater than the size of the current audio data, the entire
     * audio data is returned.
     *
     * @param length The number of samples to retain in the truncated audio data.
     * @return A new audio_data object containing the truncated data.
     */
    [[nodiscard]] audio_data truncate(size_t length) const;

    /**
     * @brief Creates a new audio_data object representing a slice of audio data
     *        starting past the end of the current data.
     *
     * This function reserves additional space in the current audio data to
     * accommodate the specified length, then creates a new audio_data object
     * that represents a slice of the specified length starting from the end
     * of the current data. The new slice shares the same underlying data buffer
     * as the original object.
     *
     * @param length The length of the slice to create, in samples.
     * @return A new audio_data object representing the slice.
     */
    [[nodiscard]] audio_data slice_past_end(size_t length);

    /**
     * @brief Retrieves the statistical information of the audio data.
     */
    [[nodiscard]] audio_stat stat() const noexcept;

    /**
     * @brief Checks whether all samples in the buffer are effectively silent within a given amplitude
     * threshold.
     *
     * @param threshold Non-negative amplitude threshold. Any sample with absolute value
     *                  strictly greater than this threshold makes the buffer non-silent. Values exactly
     *                  equal to the threshold are treated as silent. Default: 1e-5.
     * @return true if every sample lies within [-threshold, threshold]; false otherwise.
     */
    [[nodiscard]] bool is_silent(fbase threshold = fbase(1e-5)) const noexcept;

    [[nodiscard]] size_t find_peak() const noexcept;

    /**
     * @brief Executes a provided function for each audio channel's data.
     *
     * This function applies the given callable object `fn` to the audio data.
     * If the audio data is interleaved, the function is called once with the
     * entire data. Otherwise, the function is called for each channel's data
     * individually.
     *
     * @tparam Fn The type of the callable object.
     * @param fn The callable object to be executed. It should accept either
     *           the entire data (if interleaved) or a single channel's data
     *           (if not interleaved).
     */
    template <std::invocable<fbase*> Fn>
    void for_channel(Fn&& fn)
    {
        if constexpr (Interleaved)
        {
            fn(data);
        }
        else
        {
            for (size_t ch = 0; ch < channels; ++ch)
            {
                fn(data[ch]);
            }
        }
    }

    /**
     * @brief Applies a given function to the audio data for each channel.
     *
     * This function iterates over the audio channels and applies the provided
     * function object `fn` to the data. The behavior depends on whether the
     * audio data is interleaved or not:
     *
     * - If the data is interleaved, the function is called once with a single
     *   univector containing all the interleaved data.
     * - If the data is not interleaved, the function is called for each channel
     *   separately with a univector containing the data for that specific channel.
     *
     * @tparam Fn The type of the function object to be applied.
     *
     * @param fn The function object to be applied to the audio data. It should
     *           accept a univector as its argument.
     */
    template <std::invocable<univector_ref<fbase>> Fn>
    void for_channel(Fn&& fn)
    {
        if constexpr (Interleaved)
        {
            fn(make_univector(data, size * channels));
        }
        else
        {
            for (size_t ch = 0; ch < channels; ++ch)
            {
                fn(make_univector(data[ch], size));
            }
        }
    }
};

/**
 * @typedef audio_data_planar
 * @brief Alias for audio_data with planar (non-interleaved) storage format.
 *
 * This type alias represents audio data stored in a planar format, where
 * each channel's samples are stored in a separate contiguous block of memory.
 */
using audio_data_planar = audio_data<false>;

/**
 * @typedef audio_data_interleaved
 * @brief Alias for audio_data with interleaved storage.
 *
 * This type alias represents audio data where the samples are stored
 * in an interleaved format. Interleaved audio data means that the
 * samples for multiple channels are stored sequentially in memory.
 * For example, in a stereo audio signal, the data would be stored as:
 * L1, R1, L2, R2, ..., where L and R represent the left and right
 * channel samples, respectively.
 *
 * @see audio_data
 */
using audio_data_interleaved = audio_data<true>;

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

struct audio_quantization
{
    audio_dithering_state dither;

    audio_quantization(int bit_depth, audio_dithering dithering)
        : dither{ dithering, fbase(1.0) / (1ull << bit_depth) }
    {
    }
};

namespace details
{

struct stdFILE_deleter
{
    void operator()(std::FILE* f) const noexcept
    {
        if (f)
            std::fclose(f);
    }
};

} // namespace details

} // namespace kfr
