/** @addtogroup conversion
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

#include "../simd/clamp.hpp"
#include "../simd/types.hpp"
#include "../simd/vec.hpp"
#include "univector.hpp"

namespace kfr
{

enum class audio_sample_type
{
    unknown,
    i8,
    i16,
    i24,
    i32,
    i64,
    f32,
    f64,

    first_float = f32
};

inline constexpr size_t audio_sample_sizeof(audio_sample_type type)
{
    switch (type)
    {
    case audio_sample_type::i8:
        return 1;
    case audio_sample_type::i16:
        return 2;
    case audio_sample_type::i24:
        return 3;
    case audio_sample_type::i32:
    case audio_sample_type::f32:
        return 4;
    case audio_sample_type::i64:
    case audio_sample_type::f64:
        return 8;
    default:
        return 0;
    }
}

inline constexpr size_t audio_sample_bit_depth(audio_sample_type type)
{
    return audio_sample_sizeof(type) * 8;
}

inline namespace CMT_ARCH_NAME
{

using audio_sample_type_clist =
    cvals_t<audio_sample_type, audio_sample_type::i8, audio_sample_type::i16, audio_sample_type::i24,
            audio_sample_type::i32, audio_sample_type::i64, audio_sample_type::f32, audio_sample_type::f64>;

template <audio_sample_type type>
struct audio_sample_get_type;

template <>
struct audio_sample_get_type<audio_sample_type::i8>
{
    using type = i8;
};
template <>
struct audio_sample_get_type<audio_sample_type::i16>
{
    using type = i16;
};
template <>
struct audio_sample_get_type<audio_sample_type::i24>
{
    using type = i24;
};
template <>
struct audio_sample_get_type<audio_sample_type::i32>
{
    using type = i32;
};
template <>
struct audio_sample_get_type<audio_sample_type::i64>
{
    using type = i64;
};
template <>
struct audio_sample_get_type<audio_sample_type::f32>
{
    using type = f32;
};
template <>
struct audio_sample_get_type<audio_sample_type::f64>
{
    using type = f64;
};

template <typename T>
struct audio_sample_traits;

template <>
struct audio_sample_traits<i8>
{
    constexpr static f32 scale              = 127.f;
    constexpr static audio_sample_type type = audio_sample_type::i8;
};

template <>
struct audio_sample_traits<i16>
{
    constexpr static f32 scale              = 32767.f;
    constexpr static audio_sample_type type = audio_sample_type::i16;
};

template <>
struct audio_sample_traits<i24>
{
    constexpr static f32 scale              = 8388607.f;
    constexpr static audio_sample_type type = audio_sample_type::i24;
};

template <>
struct audio_sample_traits<i32>
{
    constexpr static f64 scale              = 2147483647.0;
    constexpr static audio_sample_type type = audio_sample_type::i32;
};

template <>
struct audio_sample_traits<i64>
{
    constexpr static f64 scale              = 9223372036854775807.0;
    constexpr static audio_sample_type type = audio_sample_type::i64;
};

template <>
struct audio_sample_traits<f32>
{
    constexpr static f32 scale              = 1;
    constexpr static audio_sample_type type = audio_sample_type::f32;
};

template <>
struct audio_sample_traits<f64>
{
    constexpr static f64 scale              = 1;
    constexpr static audio_sample_type type = audio_sample_type::f64;
};

template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>, KFR_ENABLE_IF(std::is_same_v<Tin, Tout>)>
inline Tout convert_sample(const Tin& in)
{
    return in;
}

template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>, KFR_ENABLE_IF(!std::is_same_v<Tin, Tout>)>
inline Tout convert_sample(const Tin& in)
{
    constexpr auto scale = Tout_traits::scale / Tin_traits::scale;
    return broadcastto<Tout>(clamp(in * scale, -Tout_traits::scale, +Tout_traits::scale));
}

/// @brief Deinterleaves and converts audio samples
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

/// @brief Deinterleaves and converts audio samples
template <typename Tout, univector_tag Tag1, univector_tag Tag2, typename Tin, univector_tag Tag3>
void deinterleave(univector2d<Tout, Tag1, Tag2>& out, const univector<Tin, Tag3>& in)
{
    if (CMT_UNLIKELY(in.empty() || out.empty()))
        return;
    std::vector<Tout*> ptrs(out.size());
    for (size_t i = 0; i < out.size(); ++i)
    {
        ptrs[i] = out[i].data();
    }
    return deinterleave(ptrs.data(), in.data(), out.size(), in.size() / out.size());
}

/// @brief Interleaves and converts audio samples
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

/// @brief Interleaves and converts audio samples
template <typename Tout, univector_tag Tag1, typename Tin, univector_tag Tag2, univector_tag Tag3>
void interleave(univector<Tout, Tag1>& out, const univector2d<Tin, Tag2, Tag3>& in)
{
    if (CMT_UNLIKELY(in.empty() || out.empty()))
        return;
    std::vector<const Tin*> ptrs(in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        ptrs[i] = in[i].data();
    }
    return interleave(out.data(), ptrs.data(), in.size(), out.size() / in.size());
}

/// @brief Interleaves and converts audio samples
template <typename Tin, univector_tag Tag1, univector_tag Tag2>
univector<Tin> interleave(const univector2d<Tin, Tag1, Tag2>& in)
{
    if (CMT_UNLIKELY(in.empty()))
        return {};
    univector<Tin> result(in.size() * in[0].size());
    interleave(result, in);
    return result;
}

/// @brief Converts audio samples (both formats are known at compile time)
template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
void convert(Tout* out, const Tin* in, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        out[i] = convert_sample<Tout, Tin, Tout_traits, Tin_traits>(in[i]);
    }
}

/// @brief Converts audio samples (input format is known at runtime)
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

/// @brief Converts audio samples (output format is known at runtime)
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
} // namespace CMT_ARCH_NAME
} // namespace kfr
