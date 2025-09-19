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
#include <kfr/audio/encoder.hpp>

namespace kfr
{

std::string audio_writing_software = KFR_VERSION_FULL;

audio_encoder::~audio_encoder() {}

std::unique_ptr<audio_encoder> create_encoder_for_container(audiofile_container container,
                                                            const audio_encoding_options& options)
{
    switch (container)
    {
    case audiofile_container::wave:
        return create_wave_encoder(wave_encoding_options{ options });
    case audiofile_container::w64:
        return create_w64_encoder(w64_encoding_options{ options });
    case audiofile_container::aiff:
        return create_aiff_encoder(aiff_encoding_options{ options });
    case audiofile_container::flac:
        return create_flac_encoder(flac_encoding_options{ options });
    case audiofile_container::caf:
        return create_caff_encoder(caff_encoding_options{ options });
    case audiofile_container::unknown:
    default:
        return nullptr;
    }
}
expected<void, audiofile_error> encode_audio_file(const file_path& path, const audio_data_interleaved& data,
                                                  const audiofile_format& format,
                                                  audio_decoder* copyMetadataFrom,
                                                  const audio_encoding_options& options)
{
    // Create the appropriate encoder based on the format
    std::unique_ptr<audio_encoder> encoder = create_encoder_for_container(format.container, options);
    if (!encoder)
    {
        return unexpected(audiofile_error::invalid_argument);
    }

    // Open the file for writing
    auto result = encoder->open(path, format, copyMetadataFrom);
    if (!result)
    {
        return unexpected(result.error());
    }

    // Write the audio data
    result = encoder->write(data);
    if (!result)
    {
        return unexpected(result.error());
    }

    // Close the encoder and finalize the file
    auto close_result = encoder->close();
    if (!close_result)
    {
        return unexpected(close_result.error());
    }

    return {};
}

[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const file_path& path,
                                                                const audio_data_planar& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom,
                                                                const audio_encoding_options& options)
{
    // Create the appropriate encoder based on the format
    std::unique_ptr<audio_encoder> encoder = create_encoder_for_container(format.container, options);
    if (!encoder)
    {
        return unexpected(audiofile_error::invalid_argument);
    }

    // Open the file for writing
    auto result = encoder->open(path, format, copyMetadataFrom);
    if (!result)
    {
        return unexpected(result.error());
    }

    for (size_t chunk = 0; chunk < data.size; chunk += 65536)
    {
        audio_data_interleaved interleaved_data = data.slice(chunk, 65536);
        // Write the audio data
        result = encoder->write(interleaved_data);
        if (!result)
        {
            return unexpected(result.error());
        }
    }

    // Close the encoder and finalize the file
    auto close_result = encoder->close();
    if (!close_result)
    {
        return unexpected(close_result.error());
    }

    return {};
}
} // namespace kfr
