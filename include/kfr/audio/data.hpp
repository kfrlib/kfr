/** @addtogroup audio
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

/**
 * @brief Determines the channel type based on interleaving.
 * @tparam T Data type.
 * @tparam Interleaved Whether the data is interleaved.
 */
template <typename T, size_t Interleaved>
using chan = std::conditional_t<Interleaved, T, std::array<T, max_audio_channels>>;

/**
 * @brief Supported audio file container formats.
 */
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

/**
 * @brief Supported audio file codecs.
 */
enum class audiofile_codec : uint8_t
{
    unknown = 0,
    lpcm,
    ieee_float,
    flac,
    alac,
    mp3,
};

/**
 * @brief Endianness of audio data.
 */
enum class audiofile_endianness : uint8_t
{
    little,
    big,
};

/**
 * @brief Checks if a container supports a single codec.
 * @param container Audio file container format.
 * @return True if the container supports a single codec, false otherwise.
 */
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

/**
 * @brief Metadata map for storing key-value pairs.
 */
using metadata_map = std::map<std::string, std::string>;

struct audio_quantization;

/**
 * @brief Loads audio samples into a floating-point buffer.
 * @tparam Tin Input sample type.
 * @param out Destination buffer.
 * @param in Source buffer.
 * @param size Number of samples.
 * @param swap_bytes Whether to swap bytes.
 */
template <typename Tin>
void samples_load(fbase* out, const Tin* in, size_t size, bool swap_bytes = false) noexcept;

/**
 * @brief Loads interleaved audio samples into a planar floating-point buffer.
 * @tparam Tin Input sample type.
 * @param out Destination buffers for each channel.
 * @param in Source buffer.
 * @param channels Number of channels.
 * @param size Number of samples per channel.
 * @param swap_bytes Whether to swap bytes.
 */
template <typename Tin>
void samples_load(fbase* const out[], const Tin* in, size_t channels, size_t size,
                  bool swap_bytes = false) noexcept;

/**
 * @brief Stores floating-point samples into a buffer with quantization.
 * @tparam Tout Output sample type.
 * @param out Destination buffer.
 * @param in Source buffer.
 * @param size Number of samples.
 * @param quantization Quantization parameters.
 * @param swap_bytes Whether to swap bytes.
 */
template <typename Tout>
void samples_store(Tout* out, const fbase* in, size_t size, const audio_quantization& quantization,
                   bool swap_bytes = false) noexcept;

/**
 * @brief Stores planar floating-point samples into an interleaved buffer with quantization.
 * @tparam Tout Output sample type.
 * @param out Destination buffer.
 * @param in Source buffers for each channel.
 * @param channels Number of channels.
 * @param size Number of samples per channel.
 * @param quantization Quantization parameters.
 * @param swap_bytes Whether to swap bytes.
 */
template <typename Tout>
void samples_store(Tout* out, const fbase* const in[], size_t channels, size_t size,
                   const audio_quantization& quantization, bool swap_bytes = false) noexcept;

/**
 * @brief Stores planar floating-point samples into a interleaved buffer.
 * @tparam Tout Output sample type.
 * @param out Destination buffer.
 * @param in Source buffers for each channel.
 * @param channels Number of channels.
 * @param size Number of samples per channel.
 * @param swap_bytes Whether to swap bytes.
 */
template <typename Tout>
void samples_store(Tout* out, const fbase* const in[], size_t channels, size_t size,
                   bool swap_bytes = false) noexcept;

/**
 * @brief Loads audio samples based on sample type.
 * @param type Audio sample type.
 * @param out Destination buffer.
 * @param in Source buffer.
 * @param size Number of samples.
 * @param swap_bytes Whether to swap bytes.
 */
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

/**
 * @brief Loads interleaved audio samples based on sample type.
 * @param type Audio sample type.
 * @param out Destination buffers for each channel.
 * @param in Source buffer.
 * @param channels Number of channels.
 * @param size Number of samples per channel.
 * @param swap_bytes Whether to swap bytes.
 */
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

/**
 * @brief Stores audio samples based on sample type with quantization.
 * @param type Audio sample type.
 * @param out Destination buffer.
 * @param in Source buffer.
 * @param size Number of samples.
 * @param quantization Quantization parameters.
 * @param swap_bytes Whether to swap bytes.
 */
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

/**
 * @brief Stores interleaved audio samples based on sample type with quantization.
 * @param type Audio sample type.
 * @param out Destination buffer.
 * @param in Source buffers for each channel.
 * @param channels Number of channels.
 * @param size Number of samples per channel.
 * @param quantization Quantization parameters.
 * @param swap_bytes Whether to swap bytes.
 */
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

/**
 * @brief Stores interleaved audio samples based on sample type.
 * @param type Audio sample type.
 * @param out Destination buffer.
 * @param in Source buffers for each channel.
 * @param channels Number of channels.
 * @param size Number of samples per channel.
 * @param swap_bytes Whether to swap bytes.
 */

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

/**
 * @brief Represents the format of an audio file.
 */
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

/**
 * @brief Represents audio statistics such as peak and RMS values.
 */
struct audio_stat
{
    fbase peak; ///< Peak absolute sample value across all channels.
    fbase rms; ///< Root mean square of all samples across all channels.
};

namespace details
{

/**
 * @brief Rounds a size up to the next power of two.
 * @param size Requested size in elements.
 * @return Smallest power of two greater than or equal to @p size.
 */
inline size_t round_capacity(size_t size) { return std::bit_ceil(size); }

/**
 * @brief RAII wrapper that deallocates an aligned fbase buffer on destruction.
 */
struct aligned_deallocator
{
    fbase* ptr; ///< Pointer to the aligned buffer to deallocate.
    ~aligned_deallocator() { kfr::aligned_deallocate(ptr); }
};
/**
 * @brief RAII wrapper that invokes a callable on destruction.
 * @tparam Fn Callable type with no arguments and void return.
 */
template <typename Fn>
struct lambda_deallocator
{
    Fn fn; ///< Callable invoked in the destructor.
    ~lambda_deallocator() { fn(); }
};
} // namespace details

/**
 * @brief Represents a strided audio channel.
 *
 * Provides a view over a single channel of an interleaved audio buffer, where
 * successive samples are separated by a fixed stride in memory.
 *
 * @tparam T Data type.
 */
template <typename T>
struct strided_channel
{
    T* data; ///< Pointer to the first sample of the channel.
    size_t size; ///< Number of samples in the channel.
    size_t stride; ///< Stride (in elements) between successive samples.
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

/**
 * @brief Contiguous audio buffer with optional interleaving.
 *
 * Stores multi-channel audio either as planar (separate channel buffers) or interleaved
 * (single strided buffer). Provides allocation, slicing, arithmetic, and traversal utilities.
 *
 * @tparam Interleaved If true, samples are interleaved; otherwise planar per-channel pointers.
 */
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
    /**
     * @brief Converts between planar and interleaved layouts.
     *
     * Constructs an audio_data with the same channel count and frame count as @p other,
     * converting sample storage between interleaved and non-interleaved layouts as needed.
     * If @p other is empty, the result is empty.
     *
     * @param other Source buffer with the opposite interleaving layout.
     */
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

    /**
     * @brief Constructs a planar audio_data view from external channel pointers with a custom deallocator.
     *
     * Takes ownership of the provided channel pointers and registers @p deallocator to be invoked
     * when this audio_data is destroyed. The buffer is treated as planar (one pointer per channel).
     *
     * @param pointers Span of per-channel fbase pointers (one per channel).
     * @param size Number of samples per channel.
     * @param deallocator Callable invoked on destruction to release the underlying storage.
     *
     * @pre pointers.size() > 0 && pointers.size() <= max_audio_channels
     */
    template <std::invocable Fn>
    [[nodiscard]] audio_data(std::span<fbase* const> pointers, size_t size, Fn&& deallocator)
        requires(!Interleaved)
        : audio_data(pointers, size)
    {
        deallocator.reset(new details::lambda_deallocator<Fn>{ std::forward<Fn>(deallocator) });
    }

    /**
     * @brief Constructs a planar audio_data view from external channel pointers.
     *
     * The resulting audio_data does not own the storage; the caller must keep the buffers alive
     * for the lifetime of this object. Channel count is taken from @p pointers.size().
     *
     * @param pointers Span of per-channel fbase pointers (one per channel).
     * @param size Number of samples per channel.
     *
     * @pre pointers.size() > 0 && pointers.size() <= max_audio_channels
     */
    [[nodiscard]] audio_data(std::span<fbase* const> pointers, size_t size)
        requires(!Interleaved);

    /**
     * @brief Constructs an interleaved audio_data view from an external buffer.
     *
     * The resulting audio_data does not own the storage; the caller must keep the buffer alive
     * for the lifetime of this object. Samples are assumed to be interleaved across @p channels.
     *
     * @param pointer Pointer to the interleaved sample buffer (size * channels elements).
     * @param channels Number of interleaved channels.
     * @param size Number of frames (samples per channel).
     *
     * @pre channels > 0 && channels <= max_audio_channels
     */
    [[nodiscard]] audio_data(fbase* pointer, size_t channels, size_t size)
        requires(Interleaved);

    /**
     * @brief Constructs an interleaved audio_data view from an external buffer with a custom deallocator.
     *
     * Takes ownership of the provided interleaved buffer and registers @p deallocator to be invoked
     * when this audio_data is destroyed.
     *
     * @param pointer Pointer to the interleaved sample buffer (size * channels elements).
     * @param channels Number of interleaved channels.
     * @param size Number of frames (samples per channel).
     * @param deallocator Callable invoked on destruction to release the underlying storage.
     *
     * @pre channels > 0 && channels <= max_audio_channels
     */
    template <std::invocable Fn>
    [[nodiscard]] audio_data(fbase* pointer, size_t channels, size_t size, Fn&& deallocator)
        requires(Interleaved)
        : audio_data(pointer, channels, size)
    {
        deallocator.reset(new details::lambda_deallocator<Fn>{ std::forward<Fn>(deallocator) });
    }

    /**
     * @brief Constructs an audio_data buffer with the specified channel count and optional initial size.
     *
     * Allocates aligned storage and initializes channel pointers according to layout:
     * - Interleaved: a single contiguous block.
     * - Planar: per-channel blocks aligned to 64 bytes (allocated as single memory block).
     *
     * If size is 0, an empty buffer is created without allocating sample storage.
     *
     * Capacity is set to size, and can be increased later via reserve().
     *
     * @param channels Number of audio channels (1..max_audio_channels).
     * @param size Optional initial number of samples per channel (capacity); 0 defers allocation.
     *
     * @pre channels > 0 && channels <= max_audio_channels
     * @post Channel pointers are initialized; storage (if allocated) is owned and freed automatically.
     * @throws std::bad_alloc If memory allocation fails.
     */
    [[nodiscard]] explicit audio_data(size_t channels, size_t size = 0);

    /**
     * @brief Constructs an audio buffer and initializes all samples to a constant value.
     * @param channels Number of channels to allocate.
     * @param size Number of samples per channel.
     * @param value Initial sample value applied to every element.
     * @note Equivalent to constructing with (channels, size) and then filling with value.
     * @throws std::bad_alloc If memory allocation fails.
     */
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

    /**
     * Resizes the container to hold exactly new_size elements.
     *
     * - If new_size <= current capacity, adjusts size without reallocating.
     * - Otherwise, increases capacity (rounded up) via reserve() and then updates size.
     *
     * Preserves existing elements up to min(old_size, new_size). When growing, newly
     * added elements may be left uninitialized. Shrinking does not reduce capacity.
     * May reallocate on growth, invalidating pointers/references to elements.
     *
     * @param new_size Number of elements desired.
     */
    void resize(size_t new_size);

    /**
     * @brief Resize to the specified length and initialize newly added samples.
     *
     * Preserves existing data. If the size grows, the appended region is filled with the given value;
     * if it shrinks, the buffer is truncated. Works with both interleaved and planar layouts, applying
     * initialization across all channels as appropriate.
     *
     * @param new_size Target number of frames (samples per channel).
     * @param value Sample value used to initialize newly created elements.
     */
    void resize(size_t new_size, fbase value);

    /**
     * @brief Increases the allocated capacity, preserving existing samples.
     *
     * If @p new_capacity is greater than the current capacity, allocates a new buffer, copies the
     * existing @p size samples into it, and replaces the storage. If @p new_capacity is less than
     * or equal to the current capacity, the capacity is lowered to @p new_capacity without
     * reallocating (existing samples up to @p size are preserved). The size is unchanged.
     *
     * @param new_capacity Target capacity in frames (samples per channel).
     */
    void reserve(size_t new_capacity);

    /**
     * @brief Appends samples from another buffer with the same layout.
     *
     * Grows this buffer to fit @p other.size additional frames and copies @p other's samples
     * after the current contents. If this buffer is empty, it becomes a copy of @p other.
     *
     * @param other Source buffer (same interleaving layout) to append.
     */
    void append(const audio_data& other);

    /**
     * @brief Prepends samples from another buffer with the same layout.
     *
     * Grows this buffer to fit @p other.size additional frames, shifts existing samples forward,
     * and copies @p other's samples at the beginning. The position is decreased by @p other.size.
     * If this buffer is empty, it becomes a copy of @p other.
     *
     * @param other Source buffer (same interleaving layout) to prepend.
     */
    void prepend(const audio_data& other);

    /**
     * @brief Appends samples from a buffer with the opposite layout.
     *
     * Grows this buffer to fit @p other.size additional frames and converts @p other's samples
     * (interleaved <-> planar) into this buffer's layout. If this buffer is empty, it becomes
     * a layout-converted copy of @p other.
     *
     * @param other Source buffer with the opposite interleaving layout to append.
     */
    void append(const audio_data<!Interleaved>& other);

    /**
     * @brief Prepends samples from a buffer with the opposite layout.
     *
     * Grows this buffer to fit @p other.size additional frames, shifts existing samples forward,
     * converts @p other's samples (interleaved <-> planar) into this buffer's layout, and places
     * them at the beginning. The position is decreased by @p other.size. If this buffer is empty,
     * it becomes a layout-converted copy of @p other.
     *
     * @param other Source buffer with the opposite interleaving layout to prepend.
     */
    void prepend(const audio_data<!Interleaved>& other);

    /**
     * @brief Swaps two audio_data buffers.
     * @param a First buffer.
     * @param b Second buffer.
     */
    friend void swap(audio_data& a, audio_data& b) noexcept { a.swap(b); }

    /**
     * @brief Exchanges the contents of this buffer with @p other.
     * @param other Buffer to swap contents with.
     */
    void swap(audio_data& other) noexcept;

    /**
     * @brief Check whether this audio data container is empty.
     * @details Considered empty if either the number of channels is zero or the size (samples/frames) is
     * zero.
     * @returns true if no channels or no samples.
     */
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

/**
 * @brief Supported audio dithering methods.
 */
enum class audio_dithering
{
    none,
    rectangular,
    triangular,
};

/**
 * @brief Represents the state of audio dithering.
 *
 * Holds the dithering method, amplitude scale, and a random number generator used to
 * produce dither noise during sample quantization.
 */
struct audio_dithering_state
{
    audio_dithering dithering; ///< Active dithering method.
    fbase scale; ///< Amplitude scale applied to the generated noise.
    mutable std::mt19937_64 rnd{ std::random_device{}() }; ///< Random number generator.
    mutable std::uniform_real_distribution<fbase> dist{
        fbase(-0.5), fbase(+0.5)
    }; ///< Uniform distribution in [-0.5, +0.5).
    /**
     * @brief Generates one dither sample according to the configured method.
     * @return A dither noise value scaled by @ref scale; zero for audio_dithering::none.
     */
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

/**
 * @brief Represents audio quantization parameters.
 *
 * Combines a dithering configuration with the bit depth used to derive the noise scale.
 */
struct audio_quantization
{
    audio_dithering_state dither; ///< Dithering state used during quantization.

    /**
     * @brief Constructs quantization parameters for the given bit depth and dithering method.
     * @param bit_depth Target bit depth; sets the dither noise scale to 1 / 2^bit_depth.
     * @param dithering Dithering method to apply.
     */
    audio_quantization(int bit_depth, audio_dithering dithering)
        : dither{ dithering, fbase(1.0) / (1ull << bit_depth) }
    {
    }
};

namespace details
{

/**
 * @brief Deleter for std::FILE* used with smart pointers (e.g. std::unique_ptr).
 */
struct stdFILE_deleter
{
    /**
     * @brief Closes the file if the pointer is non-null.
     * @param f File pointer to close (may be null).
     */
    void operator()(std::FILE* f) const noexcept
    {
        if (f)
            std::fclose(f);
    }
};

} // namespace details

} // namespace kfr
