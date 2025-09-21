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

    [[nodiscard]] virtual expected<void, audiofile_error> open(std::shared_ptr<binary_writer> writer,
                                                               const audiofile_format& format,
                                                               audio_decoder* copyMetadataFrom = nullptr) = 0;

    /// @brief open file for writing
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path, const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom = nullptr);

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
    [[nodiscard]] expected<void, audiofile_error> open(const std::string& path,
                                                       const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom = nullptr);
#endif

    /// @brief write chunk of data
    [[nodiscard]] virtual expected<void, audiofile_error> write(const audio_data_interleaved& data) = 0;

    [[nodiscard]] virtual expected<uint64_t, audiofile_error> close() = 0;

    [[nodiscard]] const std::optional<audiofile_format>& format() const noexcept { return m_format; }

    std::shared_ptr<binary_writer> writer() const noexcept { return m_writer; }

protected:
    std::optional<audiofile_format> m_format;
    std::shared_ptr<binary_writer> m_writer;
};

[[nodiscard]] std::unique_ptr<audio_encoder> create_encoder_for_container(
    audiofile_container container, const audio_encoding_options& options = {});

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

#ifdef KFR_AUDIO_FLAC
struct flac_encoding_options : public audio_encoding_options
{
};

std::unique_ptr<audio_encoder> create_flac_encoder(const flac_encoding_options& options = {});
#endif

struct caff_encoding_options : public audio_encoding_options
{
};

std::unique_ptr<audio_encoder> create_caff_encoder(const caff_encoding_options& options = {});

/**
 * @brief Encodes and writes audio data to a file.
 *
 * This function encodes the provided interleaved audio data into the specified format
 * and writes it to the given file path. Optionally, metadata can be copied from an
 * existing audio decoder.
 *
 * @param path The file path where the encoded audio file will be written.
 * @param data The interleaved audio data to be encoded.
 * @param format The desired audio file format for encoding.
 * @param copyMetadataFrom Optional pointer to an audio decoder from which metadata
 *                         will be copied. Pass nullptr if no metadata copying is needed.
 * @param options Encoding options to customize the encoding process.
 *
 * @return An `expected<void, audiofile_error>` object. If the operation succeeds,
 *         it contains no value. If an error occurs, it contains an `audiofile_error`.
 */
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const file_path& path,
                                                                const audio_data_interleaved& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const std::string& path,
                                                                const audio_data_interleaved& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});
#endif

/**
 * @brief Encodes and writes audio data to a file.
 *
 * This function encodes the provided planar audio data into the specified format
 * and writes it to the given file path. Optionally, metadata can be copied from an
 * existing audio decoder.
 *
 * @param path The file path where the encoded audio file will be written.
 * @param data The planar audio data to be encoded.
 * @param format The desired audio file format for encoding.
 * @param copyMetadataFrom Optional pointer to an audio decoder from which metadata
 *                         will be copied. Pass nullptr if no metadata copying is needed.
 * @param options Encoding options to customize the encoding process.
 *
 * @return An `expected<void, audiofile_error>` object. If the operation succeeds,
 *         it contains no value. If an error occurs, it contains an `audiofile_error`.
 */
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const file_path& path,
                                                                const audio_data_planar& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});
#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const std::string& path,
                                                                const audio_data_planar& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});
#endif

} // namespace kfr
