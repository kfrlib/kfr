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

struct RawDecoder : public audio_decoder
{
public:
    RawDecoder(audiofile_metadata info) { m_metadata = std::move(info); }
    [[nodiscard]] expected<audiofile_metadata, audiofile_error> open(const file_path& path) override;
    [[nodiscard]] expected<audio_data, audiofile_error> read() override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

protected:
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
};

std::unique_ptr<audio_decoder> create_raw_decoder(const audiofile_metadata& info)
{
    if (!info.valid())
        return nullptr;
    return std::unique_ptr<audio_decoder>(new RawDecoder(info));
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
void RawDecoder::close()
{
    file.reset();
    m_metadata.reset();
}

} // namespace kfr
