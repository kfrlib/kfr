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
#include <kfr/audio/decoder.hpp>
#include "riff.hpp"

namespace kfr
{
bool audio_decoder::seek_is_precise() const { return true; }

expected<audio_data, audiofile_error> audio_decoder::read_buffered(size_t size, audio_data& buffer)
{
    while (buffer.size < size)
    {
        expected<audio_data, audiofile_error> data = read();
        if (!data)
            return data;
        buffer.append(*data);
    }
    audio_data result = buffer.truncate(size);
    buffer            = buffer.slice(size);
    return result;
}
expected<audio_data, audiofile_error> audio_decoder::read_all()
{
    audio_data result;
    KFR_ASSERT(m_metadata.has_value());
    result.metadata = &*m_metadata;
    auto e          = read();
    while (e)
    {
        result.append(*e);
        e = read();
    }
    if (e.error() == audiofile_error::end_of_file)
        return result;
    else
        return unexpected(e.error());
}
const audiofile_metadata& audio_decoder::metadata() const
{
    KFR_ASSERT(m_metadata.has_value());
    return *m_metadata;
}

std::unique_ptr<audio_decoder> create_decoder_for_container(audiofile_container container,
                                                            const audio_decoding_options& options)
{
    switch (container)
    {
    case audiofile_container::wave:
    case audiofile_container::rf64:
        return create_wave_decoder({ options });
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
} // namespace kfr
