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
#include <kfr/audio/decoder.hpp>

namespace kfr
{
bool audio_decoder::seek_is_precise() const { return true; }

expected<audio_data_interleaved, audiofile_error> audio_decoder::read_all()
{
    if (!m_format.has_value())
        return unexpected(audiofile_error::closed);
    audio_data_interleaved result(m_format->channels);

    for (;;)
    {
        auto read = read_to(result.slice_past_end(default_audio_frames_to_read));
        if (!read)
        {
            if (read.error() == audiofile_error::end_of_file)
            {
                return result;
            }
            else
            {
                return unexpected(read.error());
            }
        }
        result.resize(result.size + *read);
    }
}

expected<audio_data_planar, audiofile_error> audio_decoder::read_all_planar()
{
    if (!m_format.has_value())
        return unexpected(audiofile_error::closed);
    audio_data_planar result(m_format->channels);

    audio_data_interleaved buffer(m_format->channels, default_audio_frames_to_read);

    for (;;)
    {
        auto read = read_to(buffer);
        if (!read)
        {
            if (read.error() == audiofile_error::end_of_file)
            {
                return result;
            }
            else
            {
                return unexpected(read.error());
            }
        }
        result.append(buffer.truncate(*read));
    }
}

const std::optional<audiofile_format>& audio_decoder::format() const { return m_format; }

expected<void, audiofile_error> audio_decoder::read_chunk(
    std::span<const std::byte> chunk_id, const std::function<bool(std::span<const std::byte>)>& handler,
    size_t buffer_size)
{
    return unexpected(audiofile_error::not_implemented);
}

std::unique_ptr<audio_decoder> create_decoder_for_container(audiofile_container container,
                                                            const audio_decoding_options& options)
{
    switch (container)
    {
    case audiofile_container::wave:
    case audiofile_container::rf64:
    case audiofile_container::bw64:
        return create_wave_decoder({ options });
    case audiofile_container::w64:
        return create_w64_decoder({ options });
    case audiofile_container::aiff:
        return create_aiff_decoder({ options });
    case audiofile_container::caf:
        return create_caff_decoder({ options });
#ifdef KFR_AUDIO_FLAC
    case audiofile_container::flac:
        return create_flac_decoder({ options });
#endif
#ifdef KFR_AUDIO_MP3
    case audiofile_container::mp3:
        return create_mp3_decoder({ options });
#endif
    default:
        return nullptr;
    }
}

namespace details
{
bool header_is(const audiofile_header& header, const char (&h)[17])
{
    for (size_t i = 0; i < 16; ++i)
    {
        if (!(static_cast<char>(header[i]) == h[i] || h[i] == '.'))
            return false;
    }
    return true;
}
} // namespace details

using details::header_is;

std::unique_ptr<audio_decoder> create_decoder_from_header(const audiofile_header& header,
                                                          const audio_decoding_options& options)
{
    if (header_is(header, "RIFF....WAVE....") || header_is(header, "RF64....WAVE....") ||
        header_is(header, "BW64....WAVE...."))
        return create_wave_decoder({ options });
    if (header_is(header, "riff\x2E\x91\xCF\x11\xA5\xD6\x28\xDB\x04\xC1\x00\x00"))
        return create_w64_decoder({ options });
    if (header_is(header, "FORM....AIFF....") || header_is(header, "FORM....AIFC...."))
        return create_aiff_decoder({ options });
    if (header_is(header, "caff............"))
        return create_caff_decoder({ options });
#ifdef KFR_AUDIO_FLAC
    if (header_is(header, "fLaC............"))
        return create_flac_decoder({ options });
#endif
#ifdef KFR_AUDIO_MP3
    if (header_is(header, "ID3.............") ||
        (header[0] == std::byte(0xFF) && (header[1] & std::byte(0xE0)) == std::byte(0xE0)))
        return create_mp3_decoder({ options });
#endif
    return nullptr;
}

static std::string extension(const file_path& path)
{
#ifdef KFR_USE_STD_FILESYSTEM
    return path.extension().string();
#else
#ifdef KFR_OS_WIN
    // Convert from wide string to ascii (no proper utf-8 conversion needed)
    std::string p(path.begin(), path.end());
#else
    std::string p = path;
#endif
    size_t pos = p.rfind('.');
    if (pos == std::string::npos)
        return "";
    return p.substr(pos);
#endif
}

std::unique_ptr<audio_decoder> create_decoder_for_file(const file_path& path,
                                                       const audio_decoding_options& options)
{
    std::unique_ptr<audio_decoder> decoder;
    expected<audiofile_header, std::error_code> header = read_audiofile_header(path);
    if (!header)
        return decoder;
    decoder = create_decoder_from_header(*header, options);
    if (decoder)
        return decoder;
    audiofile_container container = audiofile_container_from_extension(extension(path));
    if (container != audiofile_container::unknown)
        decoder = create_decoder_for_container(container, options);
    return decoder;
}

static bool match_nocase(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (std::tolower(a[i]) != std::tolower(b[i]))
            return false;
    }
    return true;
}

audiofile_container audiofile_container_from_extension(std::string_view extension)
{
    if (match_nocase(extension, ".flac"))
        return audiofile_container::flac;
    if (match_nocase(extension, ".mp3"))
        return audiofile_container::mp3;
    if (match_nocase(extension, ".wav") || match_nocase(extension, ".wave"))
        return audiofile_container::wave;
    if (match_nocase(extension, ".w64"))
        return audiofile_container::w64;
    if (match_nocase(extension, ".rf64"))
        return audiofile_container::rf64;
    if (match_nocase(extension, ".bw64"))
        return audiofile_container::bw64;
    if (match_nocase(extension, ".aif") || match_nocase(extension, ".aiff") ||
        match_nocase(extension, ".aifc"))
        return audiofile_container::aiff;
    if (match_nocase(extension, ".caf"))
        return audiofile_container::caf;
    return audiofile_container::unknown;
}

audio_decoder::~audio_decoder() {}

expected<audiofile_header, std::error_code> read_audiofile_header(const file_path& path)
{
    expected<FILE*, std::error_code> f = fopen_path(path, open_file_mode::read_existing);
    if (f)
    {
        audiofile_header header{};
        if (fread(&header, 1, sizeof(header), *f) != sizeof(header))
        {
            fclose(*f);
            return unexpected(std::error_code(errno, std::generic_category()));
        }
        fclose(*f);
        return header;
    }
    return unexpected(f.error());
}
expected<audio_data_interleaved, audiofile_error> audio_decoder::read(size_t maximum_frames)
{
    if (!m_format.has_value())
        return unexpected(audiofile_error::closed);
    audio_data_interleaved result(m_format->channels, maximum_frames);
    auto rd = read_to(result);
    if (!rd)
        return unexpected(rd.error());
    result.resize(*rd);
    return result;
}

expected<size_t, audiofile_error> audio_decoder::read_buffered(
    const audio_data_interleaved& output,
    const std::function<expected<audio_data_interleaved, audiofile_error>()>& read_packet,
    audio_data_interleaved& buffer)
{
    if (!m_format.has_value())
        return unexpected(audiofile_error::closed);

    if (output.size == 0 || output.channels != m_format->channels)
        return unexpected(audiofile_error::invalid_argument);
    size_t total_read = 0;
    while (total_read < output.size)
    {
        if (!buffer.empty())
        {
            size_t to_copy = std::min(output.size - total_read, buffer.size);
            std::memcpy(output.data + total_read * output.channels, buffer.data,
                        to_copy * output.channels * sizeof(fbase));
            total_read += to_copy;
            if (to_copy < buffer.size)
            {
                buffer = buffer.slice(to_copy);
            }
            else
            {
                buffer = audio_data_interleaved();
            }
        }
        else
        {
            auto packet = read_packet();
            if (!packet)
            {
                if (packet.error() == audiofile_error::end_of_file && total_read > 0)
                {
                    return total_read;
                }
                return unexpected(packet.error());
            }
            if (packet->empty())
            {
                return unexpected(audiofile_error::end_of_file);
            }
            buffer.swap(*packet);
        }
    }
    return total_read;
}
std::optional<uint64_t> audio_decoder::has_chunk(std::span<const std::byte> chunk_id) const
{
    return std::nullopt;
}
expected<std::vector<uint8_t>, audiofile_error> audio_decoder::read_chunk_bytes(
    std::span<const std::byte> chunk_id)
{
    std::vector<uint8_t> result;
    if (auto e = read_chunk(chunk_id,
                            [&result](std::span<const std::byte> data)
                            {
                                size_t oldSize = result.size();
                                result.resize(oldSize + data.size());
                                std::memcpy(result.data() + oldSize, data.data(), data.size());
                                return true;
                            });
        !e)
        return unexpected(e.error());
    return result;
}
expected<audio_data_interleaved, audiofile_error> decode_audio_file(const file_path& path,
                                                                    audiofile_format* out_format,
                                                                    const audio_decoding_options& options)
{
    auto decoder = create_decoder_for_file(path, options);
    if (!decoder)
        return unexpected(audiofile_error::format_error);

    auto format_result = decoder->open(path);
    if (!format_result)
        return unexpected(format_result.error());

    if (out_format)
        *out_format = *format_result;

    return decoder->read_all();
}
#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
expected<audio_data_interleaved, audiofile_error> decode_audio_file(const std::string& path,
                                                                    audiofile_format* out_format,
                                                                    const audio_decoding_options& options)
{
    return decode_audio_file(details::utf8_to_wstring(path), out_format, options);
}
expected<audiofile_format, audiofile_error> audio_decoder::open(const std::string& path)
{
    return open(details::utf8_to_wstring(path));
}
#endif
expected<audiofile_format, audiofile_error> audio_decoder::open(const file_path& path)
{
    auto file = fopen_path(path, open_file_mode::read_existing);
    if (!file)
    {
        return unexpected(from_error_code(file.error()));
    }
    return open(std::make_shared<file_reader<>>(*file));
}
} // namespace kfr
