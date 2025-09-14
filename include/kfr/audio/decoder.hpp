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

struct audio_decoder
{
public:
    virtual ~audio_decoder();

    /// @brief open audio file for reading and read info
    [[nodiscard]] virtual expected<audiofile_metadata, audiofile_error> open(const file_path& path) = 0;

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
    [[nodiscard]] expected<audiofile_metadata, audiofile_error> open(const std::string& path)
    {
        return open(details::utf8_to_wstring(path));
    }
#endif

    /// @brief read chunk of audio data
    /// @returns audiofile_error::end_of_file if total_samples have been read
    /// @returns audiofile_error::io_error if an I/O error occurred
    [[nodiscard]] virtual expected<audio_data, audiofile_error> read() = 0;

    [[nodiscard]] virtual expected<audio_data, audiofile_error> read_buffered(size_t size,
                                                                              audio_data& buffer);

    [[nodiscard]] virtual bool seek_is_precise() const;

    /// @brief seek to specific position (in sample frames)
    /// @returns audiofile_error::EndOfFile if position is greater than sample number
    [[nodiscard]] virtual expected<void, audiofile_error> seek(uint64_t position) = 0;

    /// @brief read entire audio
    [[nodiscard]] expected<audio_data, audiofile_error> read_all();

    /// @brief get audio metadata
    [[nodiscard]] const audiofile_metadata& metadata() const;

    /// @brief read chunk of data by its ID
    [[nodiscard]] virtual expected<std::vector<uint8_t>, audiofile_error> read_chunk(
        std::span<const std::byte> chunk_id);

    /// @brief close underlying file
    virtual void close() = 0;

protected:
    audio_decoder() = default;
    std::optional<audiofile_metadata> m_metadata;
};

using audiofile_header = std::array<std::byte, 16>;

[[nodiscard]] expected<audiofile_header, std::error_code> read_audiofile_header(const file_path& path);

[[nodiscard]] audiofile_container audiofile_container_from_extension(std::string_view extension);

struct audio_decoding_options
{
    bool read_metadata = false;
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_decoder_for_container(
    audiofile_container container, const audio_decoding_options& options = {});

[[nodiscard]] std::unique_ptr<audio_decoder> create_decoder_for_file(
    const file_path& path, const audio_decoding_options& options = {});

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
[[nodiscard]] inline expected<audiofile_header, std::error_code> read_audiofile_header(
    const std::string& path)
{
    return read_audiofile_header(details::utf8_to_wstring(path));
}
[[nodiscard]] inline std::unique_ptr<audio_decoder> create_decoder_for_file(
    const std::string& path, const audio_decoding_options& options = {})
{
    return create_decoder_for_file(details::utf8_to_wstring(path), options);
}
#endif

[[nodiscard]] std::unique_ptr<audio_decoder> create_decoder_from_header(
    const audiofile_header& header, const audio_decoding_options& options = {});

struct raw_decoding_options : public audio_decoding_options
{
    raw_stream_options raw;
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_raw_decoder(const raw_decoding_options& options = {});

struct wave_decoding_options : public audio_decoding_options
{
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_wave_decoder(const wave_decoding_options& options = {});

struct w64_decoding_options : public audio_decoding_options
{
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_w64_decoder(const w64_decoding_options& options = {});

struct aiff_decoding_options : public audio_decoding_options
{
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_aiff_decoder(const aiff_decoding_options& options = {});

struct caff_decoding_options : public audio_decoding_options
{
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_caff_decoder(const caff_decoding_options& options = {});

#ifdef KFR_AUDIO_FLAC
struct flac_decoding_options : public audio_decoding_options
{
};
[[nodiscard]] std::unique_ptr<audio_decoder> create_flac_decoder(const flac_decoding_options& options = {});
#endif

#ifdef KFR_AUDIO_MP3
struct mp3_decoding_options : public audio_decoding_options
{
};
[[nodiscard]] std::unique_ptr<audio_decoder> create_mp3_decoder(const mp3_decoding_options& options = {});
#endif

#ifdef KFR_OS_WIN
struct mediafoundation_decoding_options : public audio_decoding_options
{
};

[[nodiscard]] std::unique_ptr<audio_decoder> create_mediafoundation_decoder(
    const mediafoundation_decoding_options& options = {});
#endif

namespace details
{
[[nodiscard]] bool header_is(const audiofile_header& header, const char (&h)[17]);
}
} // namespace kfr
