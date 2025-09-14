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
#include "riff.hpp"

namespace kfr
{

struct RawDecoder : public audio_decoder
{
public:
    RawDecoder(raw_decoding_options options)
    {
        this->options = std::move(options);
        m_metadata    = this->options.raw.to_metadata();
    }
    [[nodiscard]] expected<audiofile_metadata, audiofile_error> open(const file_path& path) override;
    [[nodiscard]] expected<audio_data, audiofile_error> read() override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

protected:
    raw_decoding_options options;
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
};

struct RawEncoder : public audio_encoder
{
public:
    RawEncoder(raw_encoding_options options)
    {
        this->options = std::move(options);
        m_metadata    = this->options.raw.to_metadata();
    }
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path,
                                                       audio_decoder* copyMetadataFrom) override;
    [[nodiscard]] expected<void, audiofile_error> prepare(const audiofile_metadata& info) override;
    [[nodiscard]] expected<void, audiofile_error> write(const audio_data& data) override;
    expected<void, audiofile_error> close() override;

protected:
    raw_encoding_options options;
    audiofile_metadata m_metadata;
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

expected<audiofile_metadata, audiofile_error> RawDecoder::open(const file_path& path)
{
    auto f = fopen_path(path, open_file_mode::read_existing);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);

    if (KFR_IO_SEEK_64(file.get(), 0, SEEK_END))
        return unexpected(audiofile_error::io_error);
    int64_t size = KFR_IO_TELL_64(file.get());
    if (size < m_metadata->bytes_per_pcm_frame())
        return unexpected(audiofile_error::format_error);
    if (KFR_IO_SEEK_64(file.get(), 0, SEEK_SET))
        return unexpected(audiofile_error::io_error);
    m_metadata->total_frames = size / m_metadata->bytes_per_pcm_frame();

    return *m_metadata;
}

expected<audio_data, audiofile_error> RawDecoder::read()
{
    // borrowed from RIFF::readPCMAudio
    audio_data result;
    result.metadata     = &*m_metadata;
    size_t framesToRead = default_audio_frames_to_read;

    kfr::univector<uint8_t> interleaved(framesToRead * m_metadata->bytes_per_pcm_frame());
    size_t sz    = fread(interleaved.data(), 1, interleaved.size(), file.get());
    framesToRead = sz / m_metadata->bytes_per_pcm_frame();
    if (framesToRead == 0)
        return unexpected(audiofile_error::end_of_file);

    result.resize(framesToRead);

    if (!forPCMCodec(
            [&]<typename T>(ctype_t<T>)
            {
                T* interleavedAudio = reinterpret_cast<T*>(interleaved.data());
                if (m_metadata->endianness != audiofile_endianness::little)
                {
                    for (size_t i = 0; i < framesToRead * m_metadata->channels; i++)
                        convertEndianess(interleavedAudio[i]);
                }
                deinterleave_samples(result.pointers(), interleavedAudio, m_metadata->channels, framesToRead);
            },
            m_metadata->codec, m_metadata->bit_depth))
        return unexpected(audiofile_error::format_error);
    return result;
}

expected<void, audiofile_error> RawDecoder::seek(uint64_t position)
{
    if (position > m_metadata->total_frames)
        return unexpected(audiofile_error::end_of_file);
    if (KFR_IO_SEEK_64(file.get(), position * m_metadata->bytes_per_pcm_frame(), SEEK_SET))
        return unexpected(audiofile_error::io_error);
    return {};
}
void RawDecoder::close() { file.reset(); }

expected<void, audiofile_error> RawEncoder::open(const file_path& path, audio_decoder* copyMetadataFrom)
{
    auto f = fopen_path(path, open_file_mode::write_new);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);
    return {};
}

expected<void, audiofile_error> RawEncoder::prepare(const audiofile_metadata& info)
{
    if (!m_metadata.compatible(info))
        return unexpected(audiofile_error::format_error);
    return {};
}

expected<void, audiofile_error> RawEncoder::write(const audio_data& audio)
{
    // borrowed from RIFF::writePCMAudio

    const audiofile_metadata* meta = &m_metadata;
    size_t framesToWrite           = audio.size;
    univector<uint8_t> interleaved(framesToWrite * meta->bytes_per_pcm_frame());
    audio_quantinization quant(meta->bit_depth, options.dithering);
    if (!forPCMCodec(
            [&]<typename T>(ctype_t<T>)
            {
                T* interleavedAudio = reinterpret_cast<T*>(interleaved.data());
                interleave_samples(interleavedAudio, audio.pointers(), audio.channel_count(), framesToWrite,
                                   quant);

                if (meta->endianness != audiofile_endianness::little)
                {
                    for (size_t i = 0; i < framesToWrite * meta->channels; i++)
                        convertEndianess(interleavedAudio[i]);
                }
            },
            meta->codec, meta->bit_depth))
        return unexpected(audiofile_error::format_error);
    size_t wr = fwrite(interleaved.data(), 1, interleaved.size(), file.get());
    if (wr != interleaved.size())
        return unexpected(audiofile_error::io_error);
    return {};
}

expected<void, audiofile_error> RawEncoder::close()
{
    file.reset();
    return {};
}

} // namespace kfr
