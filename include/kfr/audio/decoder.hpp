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
#include <optional>
#include <string_view>
#include <span>

#include <kfr/audio/io.hpp>

namespace kfr
{

/**
 * @brief Base class for audio decoders.
 *
 * Provides an interface for opening, reading, seeking, and closing audio files.
 * Derived classes implement format-specific decoding backends.
 */
struct audio_decoder
{
public:
    virtual ~audio_decoder();

    [[nodiscard]] virtual expected<audiofile_format, audiofile_error> open(
        std::shared_ptr<binary_reader> reader) = 0;

    /**
     * @brief Open an audio file for reading and retrieve its format information.
     *
     * @param path The path to the audio file.
     * @return The audio format on success, or an error code on failure.
     */
    [[nodiscard]] expected<audiofile_format, audiofile_error> open(const file_path& path);

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
    /**
     * @brief Open an audio file using a UTF-8 encoded string path.
     *
     * @param path The UTF-8 encoded file path.
     * @return The audio format on success, or an error code on failure.
     */
    [[nodiscard]] expected<audiofile_format, audiofile_error> open(const std::string& path);
#endif

    /**
     * @brief Read audio frames into an interleaved buffer.
     *
     * @param output Destination buffer for decoded samples.
     * @return The number of frames read on success, or an error code on failure.
     */
    [[nodiscard]] virtual expected<size_t, audiofile_error> read_to(const audio_data_interleaved& output) = 0;

    /**
     * @brief Read up to a maximum number of audio frames into a newly allocated buffer.
     *
     * @param maximum_frames The maximum number of frames to read.
     * @return The audio data on success, or an error code on failure.
     */
    [[nodiscard]] expected<audio_data_interleaved, audiofile_error> read(size_t maximum_frames);

    /**
     * @brief Determine whether seeking is precise for this decoder.
     *
     * @return True if seeking is precise, false otherwise.
     */
    [[nodiscard]] virtual bool seek_is_precise() const;

    /**
     * @brief Seek to a specific position in the audio stream.
     *
     * @param position Target position in sample frames.
     * @return Success, or audiofile_error::EndOfFile if beyond the stream length.
     */
    [[nodiscard]] virtual expected<void, audiofile_error> seek(uint64_t position) = 0;

    /**
     * @brief Read the entire audio stream into an interleaved buffer.
     *
     * @return The audio data on success, or an error code on failure.
     */
    [[nodiscard]] expected<audio_data_interleaved, audiofile_error> read_all();

    /**
     * @brief Read the entire audio stream into a planar buffer.
     *
     * @return The audio data on success, or an error code on failure.
     */
    [[nodiscard]] expected<audio_data_planar, audiofile_error> read_all_planar();

    /**
     * @brief Get the format of the opened audio file.
     *
     * @return The audio format if available, or std::nullopt.
     */
    [[nodiscard]] const std::optional<audiofile_format>& format() const;

    /**
     * @brief Check whether the file has a chunk with the specified ID.
     *
     * @param chunk_id Identifier of the chunk to search for.
     * @return The chunk size if found, or std::nullopt.
     */
    virtual std::optional<uint64_t> has_chunk(std::span<const std::byte> chunk_id) const;

    /**
     * @brief Read a chunk of data by its identifier.
     *
     * @param chunk_id The chunk ID to read.
     * @param handler Callback invoked with chunk data.
     * @param buffer_size Buffer size for reading, defaults to 64 KiB.
     * @return Success or an error code.
     */
    [[nodiscard]] virtual expected<void, audiofile_error> read_chunk(
        std::span<const std::byte> chunk_id, const std::function<bool(std::span<const std::byte>)>& handler,
        size_t buffer_size = 65536);

    /**
     * @brief Read a chunk of data into a byte vector.
     *
     * @param chunk_id The chunk ID to read.
     * @return The chunk data on success, or an error code on failure.
     */
    expected<std::vector<uint8_t>, audiofile_error> read_chunk_bytes(std::span<const std::byte> chunk_id);

    /**
     * @brief Close the underlying file and release resources.
     */
    virtual void close() = 0;

    std::shared_ptr<binary_reader> reader() const noexcept { return m_reader; }

protected:
    expected<size_t, audiofile_error> read_buffered(
        const audio_data_interleaved& output,
        const std::function<expected<audio_data_interleaved, audiofile_error>()>& read_packet,
        audio_data_interleaved& buffer);

    audio_decoder() = default;
    std::optional<audiofile_format> m_format;
    std::shared_ptr<binary_reader> m_reader;
};

/**
 * @brief Represents a fixed-size audio file header.
 */
using audiofile_header = std::array<std::byte, 16>;

/**
 * @brief Reads the header of an audio file.
 *
 * @param path The file path to the audio file.
 * @return The audio file header on success, or an error code on failure.
 */
[[nodiscard]] expected<audiofile_header, std::error_code> read_audiofile_header(const file_path& path);

/**
 * @brief Determines the audio container type from a file extension.
 *
 * @param extension The file extension (with leading dot).
 * @return The detected audio container type.
 */
[[nodiscard]] audiofile_container audiofile_container_from_extension(std::string_view extension);

/**
 * @brief Options for generic audio decoding.
 */
struct audio_decoding_options
{
    bool read_metadata = false; ///< Whether to read metadata tags during decoding.
};

/**
 * @brief Creates an audio decoder for a specific container type.
 *
 * @param container The audio container type.
 * @param options Optional decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_decoder_for_container(
    audiofile_container container, const audio_decoding_options& options = {});

/**
 * @brief Creates an audio decoder for a file.
 *
 * @param path The path to the audio file.
 * @param options Optional decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_decoder_for_file(
    const file_path& path, const audio_decoding_options& options = {});

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
/// @copydoc read_audiofile_header(const file_path&)
[[nodiscard]] inline expected<audiofile_header, std::error_code> read_audiofile_header(
    const std::string& path)
{
    return read_audiofile_header(details::utf8_to_wstring(path));
}
/// @copydoc create_decoder_for_file(const file_path&, const audio_decoding_options&)
[[nodiscard]] inline std::unique_ptr<audio_decoder> create_decoder_for_file(
    const std::string& path, const audio_decoding_options& options = {})
{
    return create_decoder_for_file(details::utf8_to_wstring(path), options);
}
#endif

/**
 * @brief Creates an audio decoder from a file header.
 *
 * @param header The audio file header.
 * @param options Optional decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_decoder_from_header(
    const audiofile_header& header, const audio_decoding_options& options = {});

/**
 * @brief Options for decoding raw audio streams.
 */
struct raw_decoding_options : public audio_decoding_options
{
    audiofile_format format;
};

/**
 * @brief Creates a decoder for raw audio streams.
 *
 * @param options Optional raw decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_raw_decoder(const raw_decoding_options& options = {});

/**
 * @brief Options for decoding WAVE audio files.
 */
struct wave_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates a decoder for WAVE audio files.
 *
 * @param options Optional wave decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_wave_decoder(const wave_decoding_options& options = {});

/**
 * @brief Options for decoding W64 audio files.
 */
struct w64_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates a decoder for W64 audio files.
 *
 * @param options Optional W64 decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_w64_decoder(const w64_decoding_options& options = {});

/**
 * @brief Options for decoding AIFF audio files.
 */
struct aiff_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates a decoder for AIFF audio files.
 *
 * @param options Optional AIFF decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_aiff_decoder(const aiff_decoding_options& options = {});

/**
 * @brief Options for decoding CAFF audio files.
 */
struct caff_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates a decoder for CAFF audio files.
 *
 * @param options Optional CAFF decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_caff_decoder(const caff_decoding_options& options = {});

#ifdef KFR_AUDIO_FLAC
/**
 * @brief Options for decoding FLAC audio files.
 */
struct flac_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates a decoder for FLAC audio files.
 *
 * @param options Optional FLAC decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_flac_decoder(const flac_decoding_options& options = {});
#endif

#ifdef KFR_AUDIO_MP3
/**
 * @brief Options for decoding MP3 audio files.
 */
struct mp3_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates an MP3 audio decoder with the specified decoding options.
 *
 * @param options Optional MP3 decoding options.
 * @return A unique pointer to the created MP3 decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_mp3_decoder(const mp3_decoding_options& options = {});
#endif

#ifdef KFR_OS_WIN
/**
 * @brief Options for decoding audio using Media Foundation.
 */
struct mediafoundation_decoding_options : public audio_decoding_options
{
};

/**
 * @brief Creates a Media Foundation-based audio decoder.
 *
 * @param options Optional Media Foundation decoding options.
 * @return A unique pointer to the created decoder.
 */
[[nodiscard]] std::unique_ptr<audio_decoder> create_mediafoundation_decoder(
    const mediafoundation_decoding_options& options = {});
#endif

/**
 * @brief Decodes an audio file and returns the audio data in an interleaved format.
 *
 * @param path The file path to the audio file.
 * @param out_format Optional pointer to store the decoded audio format.
 * @param options Optional decoding options.
 * @return The decoded audio data on success, or an error code on failure.
 */
[[nodiscard]] expected<audio_data_interleaved, audiofile_error> decode_audio_file(
    const file_path& path, audiofile_format* out_format = nullptr,
    const audio_decoding_options& options = {});

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
[[nodiscard]] expected<audio_data_interleaved, audiofile_error> decode_audio_file(
    const std::string& path, audiofile_format* out_format = nullptr,
    const audio_decoding_options& options = {});
#endif

namespace details
{
/**
 * @brief Check whether an audio file header matches a specific byte pattern.
 *
 * @param header The audio file header.
 * @param h The expected header string.
 * @return True if the header matches, false otherwise.
 */
[[nodiscard]] bool header_is(const audiofile_header& header, const char (&h)[17]);
} // namespace details
} // namespace kfr
