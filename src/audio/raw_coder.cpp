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
#include <kfr/audio/encoder.hpp>
// #include "riff.hpp"

namespace kfr
{

struct RawDecoder : public audio_decoder
{
public:
    RawDecoder(raw_decoding_options options)
    {
        this->options = std::move(options);
        m_format      = this->options.raw.to_format();
    }
    [[nodiscard]] expected<audiofile_format, audiofile_error> open(const file_path& path) override;
    [[nodiscard]] expected<size_t, audiofile_error> read_to(const audio_data_interleaved& data) override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

protected:
    raw_decoding_options options;
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
};

struct RawEncoder : public audio_encoder
{
public:
    RawEncoder(raw_encoding_options options) { this->options = std::move(options); }
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path, const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom) override;
    [[nodiscard]] expected<void, audiofile_error> write(const audio_data_interleaved& data) override;
    expected<uint64_t, audiofile_error> close() override;

protected:
    raw_encoding_options options;
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
};

std::unique_ptr<audio_decoder> create_raw_decoder(const raw_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new RawDecoder(options));
}

std::unique_ptr<audio_encoder> create_raw_encoder(const raw_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new RawEncoder(options));
}

expected<audiofile_format, audiofile_error> RawDecoder::open(const file_path& path)
{
    auto f = fopen_path(path, open_file_mode::read_existing);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);

    m_format = this->options.raw.to_format();

    if (KFR_IO_SEEK_64(file.get(), 0, SEEK_END))
        return unexpected(audiofile_error::io_error);
    int64_t size = KFR_IO_TELL_64(file.get());
    if (size < m_format->bytes_per_pcm_frame())
        return unexpected(audiofile_error::format_error);
    if (KFR_IO_SEEK_64(file.get(), 0, SEEK_SET))
        return unexpected(audiofile_error::io_error);
    m_format->total_frames = size / m_format->bytes_per_pcm_frame();

    return *m_format;
}

expected<size_t, audiofile_error> RawDecoder::read_to(const audio_data_interleaved& data)
{
    // borrowed from RIFF::readPCMAudio
    size_t framesToRead = default_audio_frames_to_read;

    kfr::univector<std::byte> interleaved(framesToRead * m_format->bytes_per_pcm_frame());
    size_t sz         = fread(interleaved.data(), 1, interleaved.size(), file.get());
    size_t framesRead = sz / m_format->bytes_per_pcm_frame();
    if (framesRead == 0)
        return unexpected(audiofile_error::end_of_file);

    sample_t typ = m_format->sample_type();
    if (typ == sample_t::unknown)
        return unexpected(audiofile_error::format_error);

    samples_load(typ, data.data, interleaved.data(), m_format->channels * framesRead,
                 m_format->endianness != audiofile_endianness::little);

    return framesRead;
}

expected<void, audiofile_error> RawDecoder::seek(uint64_t position)
{
    if (position > m_format->total_frames)
        return unexpected(audiofile_error::end_of_file);
    if (KFR_IO_SEEK_64(file.get(), position * m_format->bytes_per_pcm_frame(), SEEK_SET))
        return unexpected(audiofile_error::io_error);
    return {};
}
void RawDecoder::close()
{
    file.reset();
    m_format.reset();
}

expected<void, audiofile_error> RawEncoder::open(const file_path& path, const audiofile_format& format,
                                                 audio_decoder* copyMetadataFrom)
{
    m_format = options.raw.to_format();

    auto f = fopen_path(path, open_file_mode::write_new);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);
    return {};
}

expected<void, audiofile_error> RawEncoder::write(const audio_data_interleaved& audio)
{
    if (!m_format)
        return unexpected(audiofile_error::closed);
    if (audio.channels != m_format->channels)
        return unexpected(audiofile_error::invalid_argument);
    // borrowed from RIFF::writePCMAudio

    size_t framesToWrite = audio.size;
    univector<std::byte> interleaved(framesToWrite * m_format->bytes_per_pcm_frame());
    audio_quantization quant(m_format->bit_depth, options.dithering);
    sample_t typ = m_format->sample_type();
    if (typ == sample_t::unknown)
        return unexpected(audiofile_error::format_error);
    samples_store(typ, interleaved.data(), audio.data, audio.total_samples(), quant);
    m_format->total_frames += framesToWrite;
    size_t wr = fwrite(interleaved.data(), 1, interleaved.size(), file.get());
    if (wr != interleaved.size())
        return unexpected(audiofile_error::io_error);
    return {};
}

expected<uint64_t, audiofile_error> RawEncoder::close()
{
    file.reset();
    uint64_t total_frames = m_format->total_frames;
    m_format.reset();
    return total_frames;
}

} // namespace kfr
