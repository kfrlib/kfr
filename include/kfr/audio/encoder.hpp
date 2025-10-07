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

#include <kfr/audio/io.hpp>
#include <kfr/dsp/sample_rate_conversion.hpp>

namespace kfr
{

struct audio_decoder;

/**
 * @brief Represents options for audio encoding, including dithering settings.
 */
struct audio_encoding_options
{
    audio_dithering dithering = audio_dithering::triangular;
};

/**
 * @brief Abstract base class for audio encoders, providing methods for opening, writing, and closing audio
 * files.
 */
struct audio_encoder
{
public:
    /// @brief Destructor to close and finalize file encoding.
    virtual ~audio_encoder();

    /// @brief Opens the encoder with a binary writer and format.
    /// @param writer Shared pointer to the binary writer.
    /// @param format Audio file format to use.
    /// @param copyMetadataFrom Optional decoder to copy metadata from.
    /// @return Expected result indicating success or an audiofile error.
    [[nodiscard]] virtual expected<void, audiofile_error> open(std::shared_ptr<binary_writer> writer,
                                                               const audiofile_format& format,
                                                               audio_decoder* copyMetadataFrom = nullptr) = 0;

    /// @brief Opens the encoder with a file path and format.
    /// @param path Path to the file to open.
    /// @param format Audio file format to use.
    /// @param copyMetadataFrom Optional decoder to copy metadata from.
    /// @return Expected result indicating success or an audiofile error.
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path, const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom = nullptr);

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM

    /// @brief Opens the encoder with a string path and format (Windows-specific).
    /// @param path String path to the file to open.
    /// @param format Audio file format to use.
    /// @param copyMetadataFrom Optional decoder to copy metadata from.
    /// @return Expected result indicating success or an audiofile error.
    [[nodiscard]] expected<void, audiofile_error> open(const std::string& path,
                                                       const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom = nullptr);
#endif

    /// @brief Writes a chunk of interleaved audio data to the encoder.
    /// @param data Interleaved audio data to write.
    /// @return Expected result indicating success or an audiofile error.
    [[nodiscard]] virtual expected<void, audiofile_error> write(const audio_data_interleaved& data) = 0;

    /// @brief Closes the encoder and finalizes the file.
    /// @return Expected result with the total number of frames written or an audiofile error.
    [[nodiscard]] virtual expected<uint64_t, audiofile_error> close() = 0;

    /// @brief Retrieves the current audio file format.
    /// @return Optional containing the audio file format if set.
    [[nodiscard]] const std::optional<audiofile_format>& format() const noexcept { return m_format; }

    /// @brief Retrieves the binary writer associated with the encoder.
    /// @return Shared pointer to the binary writer.
    std::shared_ptr<binary_writer> writer() const noexcept { return m_writer; }

protected:
    std::optional<audiofile_format> m_format;
    std::shared_ptr<binary_writer> m_writer;
};

/**
 * @brief Creates an audio encoder for the specified container format with optional encoding options.
 * @param container The audio file container format.
 * @param options Optional encoding options.
 * @return A unique pointer to the created audio encoder.
 */
[[nodiscard]] std::unique_ptr<audio_encoder> create_encoder_for_container(
    audiofile_container container, const audio_encoding_options& options = {});

/// Software string to write into metadata, if supported by format
extern std::string audio_writing_software;

/**
 * @brief Represents options for raw audio encoding.
 */
struct raw_encoding_options : public audio_encoding_options
{
};

/**
 * @brief Creates a raw audio encoder with optional encoding options.
 * @param options Optional raw encoding options.
 * @return A unique pointer to the created raw audio encoder.
 */
std::unique_ptr<audio_encoder> create_raw_encoder(const raw_encoding_options& options = {});

/**
 * @brief Represents options for WAVE audio encoding, including RF64 support for large files.
 */
struct wave_encoding_options : public audio_encoding_options
{
    bool switch_to_rf64_if_over_4gb = true; // default true
};

/**
 * @brief Creates a WAVE audio encoder with optional encoding options.
 * @param options Optional WAVE encoding options.
 * @return A unique pointer to the created WAVE audio encoder.
 */
std::unique_ptr<audio_encoder> create_wave_encoder(const wave_encoding_options& options = {});

/**
 * @brief Represents options for W64 audio encoding.
 */
struct w64_encoding_options : public audio_encoding_options
{
};

/**
 * @brief Creates a W64 audio encoder with optional encoding options.
 * @param options Optional W64 encoding options.
 * @return A unique pointer to the created W64 audio encoder.
 */
std::unique_ptr<audio_encoder> create_w64_encoder(const w64_encoding_options& options = {});

/**
 * @brief Represents options for AIFF audio encoding.
 */
struct aiff_encoding_options : public audio_encoding_options
{
};

/**
 * @brief Creates an AIFF audio encoder with optional encoding options.
 * @param options Optional AIFF encoding options.
 * @return A unique pointer to the created AIFF audio encoder.
 */
std::unique_ptr<audio_encoder> create_aiff_encoder(const aiff_encoding_options& options = {});

#ifdef KFR_AUDIO_FLAC
/**
 * @brief Represents options for FLAC audio encoding.
 */
struct flac_encoding_options : public audio_encoding_options
{
};

/**
 * @brief Creates a FLAC audio encoder with optional encoding options.
 * @param options Optional FLAC encoding options.
 * @return A unique pointer to the created FLAC audio encoder.
 */
std::unique_ptr<audio_encoder> create_flac_encoder(const flac_encoding_options& options = {});
#endif

/**
 * @brief Represents options for CAFF audio encoding.
 */
struct caff_encoding_options : public audio_encoding_options
{
};

/**
 * @brief Creates a CAFF audio encoder with optional encoding options.
 * @param options Optional CAFF encoding options.
 * @return A unique pointer to the created CAFF audio encoder.
 */
std::unique_ptr<audio_encoder> create_caff_encoder(const caff_encoding_options& options = {});

/**
 * @brief Encodes interleaved audio data and writes it to a file.
 * @param path The file path for the encoded audio.
 * @param data The interleaved audio data to encode.
 * @param format The desired audio file format.
 * @param copyMetadataFrom Optional audio decoder to copy metadata from.
 * @param options Optional encoding options.
 * @return An expected object containing an audiofile_error on failure.
 */
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const file_path& path,
                                                                const audio_data_interleaved& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
/**
 * @brief Encodes interleaved audio data and writes it to a file (Windows-specific overload).
 * @param path The file path for the encoded audio.
 * @param data The interleaved audio data to encode.
 * @param format The desired audio file format.
 * @param copyMetadataFrom Optional audio decoder to copy metadata from.
 * @param options Optional encoding options.
 * @return An expected object containing void on success or an audiofile_error on failure.
 */
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const std::string& path,
                                                                const audio_data_interleaved& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});
#endif

/**
 * @brief Encodes planar audio data and writes it to a file.
 * @param path The file path for the encoded audio.
 * @param data The planar audio data to encode.
 * @param format The desired audio file format.
 * @param copyMetadataFrom Optional audio decoder to copy metadata from.
 * @param options Optional encoding options.
 * @return An expected object containing an audiofile_error on failure.
 */
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const file_path& path,
                                                                const audio_data_planar& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
/**
 * @brief Encodes planar audio data and writes it to a file (Windows-specific overload).
 * @param path The file path for the encoded audio.
 * @param data The planar audio data to encode.
 * @param format The desired audio file format.
 * @param copyMetadataFrom Optional audio decoder to copy metadata from.
 * @param options Optional encoding options.
 * @return An expected object containing void on success or an audiofile_error on failure.
 */
[[nodiscard]] expected<void, audiofile_error> encode_audio_file(const std::string& path,
                                                                const audio_data_planar& data,
                                                                const audiofile_format& format,
                                                                audio_decoder* copyMetadataFrom = nullptr,
                                                                const audio_encoding_options& options = {});
#endif

} // namespace kfr
