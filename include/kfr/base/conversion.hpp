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

#include "../simd/clamp.hpp"
#include "../simd/types.hpp"
#include "../simd/vec.hpp"
#include "univector.hpp"

namespace kfr
{

/**
 * @brief Identifies the numeric format of an audio sample.
 *
 * Positive values denote signed integer samples whose bit width equals the
 * enumerator value. Negative values denote floating-point samples; their
 * absolute value is the bit width. `unknown` represents an invalid or
 * unspecified format.
 */
enum class audio_sample_type
{
    f32     = -32,
    f64     = -64,
    unknown = 0,
    // i8      = 8,
    i16 = 16,
    i24 = 24,
    i32 = 32,
    // i64     = 64,
};

/**
 * @brief Returns the bit depth of the given audio sample type.
 *
 * For integer sample types the enumerator value is the bit width; for
 * floating-point sample types the absolute value of the enumerator is the
 * bit width. Returns 0 for `audio_sample_type::unknown`.
 *
 * @param type The audio sample type to query.
 * @return The bit depth of @p type, or 0 if unknown.
 */
constexpr size_t audio_sample_bit_depth(audio_sample_type type) noexcept
{
    if (static_cast<int>(type) > 0)
        return static_cast<int>(type);
    if (static_cast<int>(type) < 0)
        return -static_cast<int>(type);
    return 0;
}

/**
 * @brief Returns the storage size in bytes of a sample of the given type.
 *
 * The size is computed by rounding the bit depth up to the next multiple
 * of 8.
 *
 * @param type The audio sample type to query.
 * @return The storage size of @p type, in bytes.
 */
constexpr size_t audio_sample_sizeof(audio_sample_type type) noexcept
{
    return align_up(audio_sample_bit_depth(type), 8);
}

/**
 * @brief Reports whether the given sample type is floating point.
 *
 * @param t The audio sample type to query.
 * @return `true` if @p t is a floating-point sample type, `false` otherwise.
 */
constexpr bool audio_sample_is_float(audio_sample_type t) noexcept { return static_cast<int>(t) < 0; }

/**
 * @brief Concept matching the C++ types accepted as audio samples.
 *
 * Matches `f32`, `f64`, `int16_t`, `kfr::i24` and `int32_t`.
 */
template <typename T>
concept audio_sample = std::floating_point<f32> || std::floating_point<f64> || std::same_as<T, int16_t> ||
                       std::same_as<T, kfr::i24> || std::same_as<T, int32_t>;

/**
 * @brief List of all supported audio sample types.
 *
 * Used with @ref cswitch to dispatch on the audio sample type at runtime.
 */
using audio_sample_type_clist =
    cvals_t<audio_sample_type, audio_sample_type::i16, audio_sample_type::i24, audio_sample_type::i32,
            audio_sample_type::f32, audio_sample_type::f64>;

/**
 * @brief Maps an audio_sample_type enumerator to its native C++ type.
 *
 * Specializations provide `using type = ...` for each enumerator value.
 *
 * @tparam type The audio_sample_type enumerator.
 */
template <audio_sample_type type>
struct audio_sample_get_type;

template <>
struct audio_sample_get_type<audio_sample_type::i16>
{
    /** @brief Native C++ type corresponding to `audio_sample_type::i16`. */
    using type = i16;
};
template <>
struct audio_sample_get_type<audio_sample_type::i24>
{
    /** @brief Native C++ type corresponding to `audio_sample_type::i24`. */
    using type = i24;
};
template <>
struct audio_sample_get_type<audio_sample_type::i32>
{
    /** @brief Native C++ type corresponding to `audio_sample_type::i32`. */
    using type = i32;
};
template <>
struct audio_sample_get_type<audio_sample_type::f32>
{
    /** @brief Native C++ type corresponding to `audio_sample_type::f32`. */
    using type = f32;
};
template <>
struct audio_sample_get_type<audio_sample_type::f64>
{
    /** @brief Native C++ type corresponding to `audio_sample_type::f64`. */
    using type = f64;
};

/**
 * @brief Compile-time properties of an audio sample C++ type.
 *
 * Specializations provide the maximum representable magnitude (`scale`)
 * used for conversion, and the matching `audio_sample_type` enumerator.
 *
 * @tparam T The native audio sample C++ type.
 */
template <typename T>
struct audio_sample_traits;

template <>
struct audio_sample_traits<i16>
{
    /** @brief Maximum representable magnitude used as conversion scale. */
    constexpr static f32 scale = 32767.f;
    /** @brief Matching `audio_sample_type` enumerator. */
    constexpr static audio_sample_type type = audio_sample_type::i16;
};

template <>
struct audio_sample_traits<i24>
{
    /** @brief Maximum representable magnitude used as conversion scale. */
    constexpr static f32 scale = 8388607.f;
    /** @brief Matching `audio_sample_type` enumerator. */
    constexpr static audio_sample_type type = audio_sample_type::i24;
};

template <>
struct audio_sample_traits<i32>
{
    /** @brief Maximum representable magnitude used as conversion scale. */
    constexpr static f64 scale = 2147483647.0;
    /** @brief Matching `audio_sample_type` enumerator. */
    constexpr static audio_sample_type type = audio_sample_type::i32;
};

template <>
struct audio_sample_traits<f32>
{
    /** @brief Scale factor for float samples (always 1). */
    constexpr static f32 scale = 1;
    /** @brief Matching `audio_sample_type` enumerator. */
    constexpr static audio_sample_type type = audio_sample_type::f32;
};

template <>
struct audio_sample_traits<f64>
{
    /** @brief Scale factor for double samples (always 1). */
    constexpr static f64 scale = 1;
    /** @brief Matching `audio_sample_type` enumerator. */
    constexpr static audio_sample_type type = audio_sample_type::f64;
};

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Converts a single audio sample from one native type to another.
 *
 * The value is rescaled from `Tin_traits::scale` to `Tout_traits::scale` and
 * clamped to the representable range of @p Tout. When `Tin` and `Tout` are
 * the same type the function returns @p in unchanged.
 *
 * @tparam Tout        Destination sample C++ type.
 * @tparam Tin         Source sample C++ type.
 * @tparam Tout_traits Traits specialization for @p Tout.
 * @tparam Tin_traits  Traits specialization for @p Tin.
 * @param in The input sample to convert.
 * @return The converted sample, clamped to the destination range.
 */
template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
    requires(std::is_same_v<Tin, Tout>)
inline Tout convert_sample(const Tin& in)
{
    return in;
}

/**
 * @brief Converts a single audio sample between two different native types.
 *
 * The value is rescaled and clamped to fit into the destination type.
 *
 * @tparam Tout        Destination sample C++ type.
 * @tparam Tin         Source sample C++ type.
 * @tparam Tout_traits Traits specialization for @p Tout.
 * @tparam Tin_traits  Traits specialization for @p Tin.
 * @param in The input sample to convert.
 * @return The converted sample, clamped to the destination range.
 */
template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
    requires(!std::is_same_v<Tin, Tout>)
inline Tout convert_sample(const Tin& in)
{
    constexpr auto scale = Tout_traits::scale / Tin_traits::scale;
    return broadcastto<Tout>(clamp(in * scale, -Tout_traits::scale, +Tout_traits::scale));
}

/**
 * @brief Deinterleaves and converts audio samples from a single buffer.
 *
 * Reads `size * channels` interleaved samples from @p in and writes
 * `channels` deinterleaved buffers of `size` samples to @p out, performing
 * a sample conversion on each value.
 *
 * @tparam Tout        Destination sample C++ type.
 * @tparam Tin         Source sample C++ type.
 * @tparam Tout_traits Traits specialization for @p Tout.
 * @tparam Tin_traits  Traits specialization for @p Tin.
 * @param out      Array of @p channels pointers to the destination buffers.
 * @param in       Pointer to the interleaved input buffer.
 * @param channels Number of interleaved channels.
 * @param size     Number of samples per channel.
 */
template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
void deinterleave(Tout* out[], const Tin* in, size_t channels, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            out[ch][i] = convert_sample<Tout, Tin, Tout_traits, Tin_traits>(in[i * channels + ch]);
    }
}

/**
 * @brief Deinterleaves and converts audio samples into a 2D univector.
 *
 * The outer dimension of @p out is interpreted as the channel count and
 * the inner dimension as the per-channel sample count. The input is
 * expected to be `out.size() * in.size() / out.size()` interleaved samples;
 * if either container is empty the call is a no-op.
 *
 * @tparam Tout Destination sample C++ type.
 * @tparam Tag1 Outer-dimension tag of @p out.
 * @tparam Tag2 Inner-dimension tag of @p out.
 * @tparam Tin  Source sample C++ type.
 * @tparam Tag3 Tag of @p in.
 * @param out Destination 2D container; each row receives one channel.
 * @param in  Source interleaved container.
 */
template <typename Tout, univector_tag Tag1, univector_tag Tag2, typename Tin, univector_tag Tag3>
void deinterleave(univector2d<Tout, Tag1, Tag2>& out, const univector<Tin, Tag3>& in)
{
    if (KFR_UNLIKELY(in.empty() || out.empty()))
        return;
    std::vector<Tout*> ptrs(out.size());
    for (size_t i = 0; i < out.size(); ++i)
    {
        ptrs[i] = out[i].data();
    }
    return deinterleave(ptrs.data(), in.data(), out.size(), in.size() / out.size());
}

/**
 * @brief Interleaves and converts audio samples into a single buffer.
 *
 * Reads `size` samples from each of @p channels input buffers and writes
 * `size * channels` interleaved converted samples to @p out.
 *
 * @tparam Tout        Destination sample C++ type.
 * @tparam Tin         Source sample C++ type.
 * @tparam Tout_traits Traits specialization for @p Tout.
 * @tparam Tin_traits  Traits specialization for @p Tin.
 * @param out     Pointer to the interleaved destination buffer.
 * @param in      Array of @p channels pointers to per-channel input buffers.
 * @param channels Number of channels to interleave.
 * @param size     Number of samples per channel.
 */
template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
void interleave(Tout* out, const Tin* in[], size_t channels, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            out[i * channels + ch] = convert_sample<Tout, Tin, Tout_traits, Tin_traits>(in[ch][i]);
    }
}

/**
 * @brief Interleaves and converts audio samples from a 2D univector.
 *
 * The outer dimension of @p in is the channel count and the inner
 * dimension is the per-channel sample count. The result is written to
 * @p out as a single interleaved buffer of length
 * `in.size() * in[0].size()`. No-op when @p out is empty.
 *
 * @tparam Tout Destination sample C++ type.
 * @tparam Tag1 Tag of @p out.
 * @tparam Tin  Source sample C++ type.
 * @tparam Tag2 Outer-dimension tag of @p in.
 * @tparam Tag3 Inner-dimension tag of @p in.
 * @param out Destination interleaved buffer.
 * @param in  Source 2D container of per-channel buffers.
 */
template <typename Tout, univector_tag Tag1, typename Tin, univector_tag Tag2, univector_tag Tag3>
void interleave(univector<Tout, Tag1>& out, const univector2d<Tin, Tag2, Tag3>& in)
{
    if (KFR_UNLIKELY(in.empty() || out.empty()))
        return;
    std::vector<const Tin*> ptrs(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        ptrs[i] = in[i].data();
    }
    return interleave(out.data(), ptrs.data(), in.size(), out.size() / in.size());
}

/**
 * @brief Interleaves audio samples of a 2D univector into a new buffer.
 *
 * The input is left unchanged; the returned vector contains
 * `in.size() * in[0].size()` samples laid out channel-interleaved.
 * Returns an empty vector when @p in is empty.
 *
 * @tparam Tin  Sample C++ type.
 * @tparam Tag1 Outer-dimension tag of @p in.
 * @tparam Tag2 Inner-dimension tag of @p in.
 * @param in Source 2D container of per-channel buffers.
 * @return A new vector holding the interleaved samples.
 */
template <typename Tin, univector_tag Tag1, univector_tag Tag2>
univector<Tin> interleave(const univector2d<Tin, Tag1, Tag2>& in)
{
    if (KFR_UNLIKELY(in.empty()))
        return {};
    univector<Tin> result(in.size() * in[0].size());
    interleave(result, in);
    return result;
}

/**
 * @brief Converts audio samples between two compile-time-known formats.
 *
 * @tparam Tout        Destination sample C++ type.
 * @tparam Tin         Source sample C++ type.
 * @tparam Tout_traits Traits specialization for @p Tout.
 * @tparam Tin_traits  Traits specialization for @p Tin.
 * @param out  Pointer to the destination buffer.
 * @param in   Pointer to the source buffer.
 * @param size Number of samples to convert.
 */
template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
void convert(Tout* out, const Tin* in, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        out[i] = convert_sample<Tout, Tin, Tout_traits, Tin_traits>(in[i]);
    }
}

/**
 * @brief Converts audio samples whose input format is selected at runtime.
 *
 * Dispatches on @p in_type to the matching compile-time conversion
 * specialization.
 *
 * @tparam Tout        Destination sample C++ type.
 * @tparam Tout_traits Traits specialization for @p Tout.
 * @param out    Pointer to the destination buffer.
 * @param in     Pointer to the source buffer.
 * @param in_type Source audio sample type.
 * @param size   Number of samples to convert.
 */
template <typename Tout, typename Tout_traits = audio_sample_traits<Tout>>
void convert(Tout* out, const void* in, audio_sample_type in_type, size_t size)
{
    cswitch(audio_sample_type_clist{}, in_type,
            [&](auto t)
            {
                using type = typename audio_sample_get_type<val_of(decltype(t)())>::type;
                convert(out, reinterpret_cast<const type*>(in), size);
            });
}

/**
 * @brief Converts audio samples whose output format is selected at runtime.
 *
 * Dispatches on @p out_type to the matching compile-time conversion
 * specialization.
 *
 * @tparam Tin        Source sample C++ type.
 * @tparam Tin_traits Traits specialization for @p Tin.
 * @param out     Pointer to the destination buffer.
 * @param out_type Destination audio sample type.
 * @param in      Pointer to the source buffer.
 * @param size    Number of samples to convert.
 */
template <typename Tin, typename Tin_traits = audio_sample_traits<Tin>>
void convert(void* out, audio_sample_type out_type, const Tin* in, size_t size)
{
    cswitch(audio_sample_type_clist{}, out_type,
            [&](auto t)
            {
                using type = typename audio_sample_get_type<val_of(decltype(t)())>::type;
                convert(reinterpret_cast<type*>(out), in, size);
            });
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
