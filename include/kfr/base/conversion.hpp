/** @addtogroup math
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "types.hpp"
#include "vec.hpp"

namespace kfr
{

template <typename T>
struct audio_sample_traits;

template <>
struct audio_sample_traits<i8>
{
    constexpr static f32 scale = 127.f;
};

template <>
struct audio_sample_traits<i16>
{
    constexpr static f32 scale = 32767.f;
};

template <>
struct audio_sample_traits<i24>
{
    constexpr static f32 scale = 8388607.f;
};

template <>
struct audio_sample_traits<i32>
{
    constexpr static f64 scale = 2147483647.0;
};

template <>
struct audio_sample_traits<i64>
{
    constexpr static f64 scale = 9223372036854775807.0;
};

template <>
struct audio_sample_traits<f32>
{
    constexpr static f32 scale = 1;
};

template <>
struct audio_sample_traits<f64>
{
    constexpr static f64 scale = 1;
};

template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>, KFR_ENABLE_IF(is_same<Tin, Tout>::value)>
inline Tout convert_sample(const Tin& in)
{
    return in;
}

template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>, KFR_ENABLE_IF(!is_same<Tin, Tout>::value)>
inline Tout convert_sample(const Tin& in)
{
    constexpr auto scale = Tout_traits::scale / Tin_traits::scale;
    return cast<Tout>(in * scale);
}

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

template <typename Tout, typename Tin, typename Tout_traits = audio_sample_traits<Tout>,
          typename Tin_traits = audio_sample_traits<Tin>>
void convert(Tout* out, const Tin* in, size_t size)
{
    for (size_t i = 0; i < size; ++i)
    {
        out[i] = convert_sample<Tout, Tin, Tout_traits, Tin_traits>(in[i]);
    }
}
} // namespace kfr
