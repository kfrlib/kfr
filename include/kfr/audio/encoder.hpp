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

#include <kfr/audio/io.hpp>
#include <kfr/dsp/sample_rate_conversion.hpp>

namespace kfr
{

struct audio_decoder;

struct audio_encoding_options
{
    audio_dithering dithering = audio_dithering::triangular;
};

struct audio_encoder
{
public:
    /// @brief close and finish file encoding
    virtual ~audio_encoder();

    /// @brief open file for writing
    [[nodiscard]] virtual expected<void, audiofile_error> open(const file_path& path,
                                                               audio_decoder* copyMetadataFrom = nullptr) = 0;

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
    [[nodiscard]] expected<void, audiofile_error> open(const std::string& path,
                                                       audio_decoder* copyMetadataFrom = nullptr)
    {
        return open(details::utf8_to_wstring(path), copyMetadataFrom);
    }
#endif

    /// @brief prepare encoding, write format
    [[nodiscard]] virtual expected<void, audiofile_error> prepare(const audiofile_metadata& info) = 0;

    /// @brief write chunk of data
    [[nodiscard]] virtual expected<void, audiofile_error> write(const audio_data& data) = 0;

    [[nodiscard]] virtual expected<void, audiofile_error> close() = 0;
};

/// Software string to write into metadata, if supported by format
extern std::string audio_writing_software;

struct raw_encoding_options : public audio_encoding_options
{
    raw_stream_options raw;
};

std::unique_ptr<audio_encoder> create_raw_encoder(const raw_encoding_options& options = {});

struct wave_encoding_options : public audio_encoding_options
{
    bool switch_to_rf64_if_over_4gb = true; // default true
};

std::unique_ptr<audio_encoder> create_wave_encoder(const wave_encoding_options& options = {});

struct w64_encoding_options : public audio_encoding_options
{
};

std::unique_ptr<audio_encoder> create_w64_encoder(const w64_encoding_options& options = {});

struct aiff_encoding_options : public audio_encoding_options
{
};

std::unique_ptr<audio_encoder> create_aiff_encoder(const aiff_encoding_options& options = {});

struct flac_encoding_options : public audio_encoding_options
{
};

std::unique_ptr<audio_encoder> create_flac_encoder(const flac_encoding_options& options = {});

struct caff_encoding_options : public audio_encoding_options
{
};

std::unique_ptr<audio_encoder> create_caff_encoder(const caff_encoding_options& options = {});

} // namespace kfr
