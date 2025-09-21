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

#include <cstdint>
#include <kfr/io/file.hpp>
#include <kfr/audio/data.hpp>

namespace kfr
{

enum class audiofile_error : uint32_t
{
    unknown          = 0,
    io_error         = 1,
    format_error     = 2,
    internal_error   = 3,
    too_large        = 4, // data chunk too large for standard WAV
    end_of_file      = 5,
    abort            = 6,
    not_implemented  = 7,
    invalid_argument = 8,
    closed           = 9,
    empty_file       = 10, // file is empty or does not contain any audio data
    not_found        = 11,
    access_denied    = 12,
};

audiofile_error from_error_code(std::error_code ec);

std::string to_string(audiofile_error err);

constexpr inline size_t default_audio_frames_to_read = 16384;

struct raw_stream_options
{
    uint32_t channels               = 2;
    uint32_t sample_rate            = 44100;
    audiofile_codec codec           = audiofile_codec::lpcm; // only lpcm and ieee_float supported
    audiofile_endianness endianness = audiofile_endianness::little;
    int8_t bit_depth                = 16;

    audiofile_format to_format() const noexcept
    {
        audiofile_format result{};
        result.channels    = channels;
        result.sample_rate = sample_rate;
        result.codec       = codec;
        result.endianness  = endianness;
        result.bit_depth   = bit_depth;
        return result;
    }
};

} // namespace kfr
