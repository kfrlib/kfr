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
#include <kfr/audio/encoder.hpp>
#include <kfr/audio/decoder.hpp>

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
#ifdef KFR_AUDIO_FLAC
    case audiofile_container::flac:
        return create_flac_encoder(flac_encoding_options{ options });
#endif
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
    audiofile_format i_format = format;
    i_format.channels         = data.channels;
    if (i_format.container == audiofile_container::unknown)
    {
        i_format.container = audiofile_container_from_extension(file_extension(path));
    }
    // Create the appropriate encoder based on the format
    std::unique_ptr<audio_encoder> encoder = create_encoder_for_container(i_format.container, options);
    if (!encoder)
    {
        return unexpected(audiofile_error::invalid_argument);
    }

    // Open the file for writing
    auto result = encoder->open(path, i_format, copyMetadataFrom);
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
    // Create the appropriate encoder based on the format or file extension
    audiofile_format i_format = format;
    i_format.channels         = data.channels;

    if (i_format.container == audiofile_container::unknown)
    {
        i_format.container = audiofile_container_from_extension(file_extension(path));
    }

    std::unique_ptr<audio_encoder> encoder = create_encoder_for_container(i_format.container, options);
    if (!encoder)
    {
        return unexpected(audiofile_error::invalid_argument);
    }

    // Open the file for writing
    auto result = encoder->open(path, i_format, copyMetadataFrom);
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
#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
expected<void, audiofile_error> encode_audio_file(const std::string& path, const audio_data_interleaved& data,
                                                  const audiofile_format& format,
                                                  audio_decoder* copyMetadataFrom,
                                                  const audio_encoding_options& options)
{
    return encode_audio_file(details::utf8_to_wstring(path), data, format, copyMetadataFrom, options);
}
expected<void, audiofile_error> encode_audio_file(const std::string& path, const audio_data_planar& data,
                                                  const audiofile_format& format,
                                                  audio_decoder* copyMetadataFrom,
                                                  const audio_encoding_options& options)
{
    return encode_audio_file(details::utf8_to_wstring(path), data, format, copyMetadataFrom, options);
}
expected<void, audiofile_error> audio_encoder::open(const std::string& path, const audiofile_format& format,
                                                    audio_decoder* copyMetadataFrom)
{
    return open(details::utf8_to_wstring(path), format, copyMetadataFrom);
}
#endif
expected<void, audiofile_error> audio_encoder::open(const file_path& path, const audiofile_format& format,
                                                    audio_decoder* copyMetadataFrom)
{
    auto file = fopen_path(path, open_file_mode::write_new);
    if (!file)
        return unexpected(from_error_code(file.error()));
    return open(std::make_shared<file_writer<>>(*file), format, copyMetadataFrom);
}
} // namespace kfr
