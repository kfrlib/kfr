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

#define MINIMP3_NO_STDIO 1
#define MINIMP3_FLOAT_OUTPUT 1
#define MINIMP3_IMPLEMENTATION 1
#include <minimp3/minimp3_ex.h>

namespace kfr
{

struct MP3Decoder : public audio_decoder
{
public:
    [[nodiscard]] expected<audiofile_format, audiofile_error> open(
        std::shared_ptr<binary_reader> reader) override;
    [[nodiscard]] expected<size_t, audiofile_error> read_to(const audio_data_interleaved& output) override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

    MP3Decoder(mp3_decoding_options opts = {});
    ~MP3Decoder();

protected:
    mp3_decoding_options options;
    mp3dec_io_t io;
    std::optional<mp3dec_ex_t> decex;
    audio_data_interleaved audio;

    static size_t mp3d_read_cb(void* buf, size_t size, void* user_data);
    static int mp3d_seek_cb(uint64_t position, void* user_data);
};

MP3Decoder::MP3Decoder(mp3_decoding_options opts) : options(std::move(opts))
{
    io.read      = &MP3Decoder::mp3d_read_cb;
    io.read_data = this;
    io.seek      = &MP3Decoder::mp3d_seek_cb;
    io.seek_data = this;
}

MP3Decoder::~MP3Decoder() { close(); }

void MP3Decoder::close()
{
    audio.reset();
    if (decex.has_value())
    {
        mp3dec_ex_close(&*decex);
        decex.reset();
    }
    m_reader.reset();
    m_format.reset();
}

size_t MP3Decoder::mp3d_read_cb(void* buf, size_t size, void* user_data)
{
    return reinterpret_cast<MP3Decoder*>(user_data)->m_reader->read(buf, size);
}
int MP3Decoder::mp3d_seek_cb(uint64_t position, void* user_data)
{
    return reinterpret_cast<MP3Decoder*>(user_data)->m_reader->seek(position, seek_origin::begin) ? 0 : -1;
}

expected<audiofile_format, audiofile_error> MP3Decoder::open(std::shared_ptr<binary_reader> reader)
{
    if (!reader)
        return unexpected(audiofile_error::invalid_argument);
    m_reader = std::move(reader);
    decex.emplace();
    if (int e = mp3dec_ex_open_cb(&*decex, &io, MP3D_SEEK_TO_SAMPLE); e != 0)
    {
        switch (e)
        {
        case MP3D_E_IOERROR:
            return unexpected(audiofile_error::io_error);
        case MP3D_E_DECODE:
            return unexpected(audiofile_error::format_error);
        default:
            return unexpected(audiofile_error::internal_error);
        }
    }
    audiofile_format info;
    info.container    = audiofile_container::mp3;
    info.channels     = decex->info.channels;
    info.sample_rate  = decex->info.hz;
    info.total_frames = decex->samples / decex->info.channels;
    info.bit_depth    = 0;
    info.codec        = audiofile_codec::mp3;
    m_format          = std::move(info);
    return *m_format;
}

static_assert(std::is_same_v<mp3d_sample_t, float>);

expected<size_t, audiofile_error> MP3Decoder::read_to(const audio_data_interleaved& output)
{
    if (!m_reader)
        return unexpected(audiofile_error::closed);
    if (output.channels != m_format->channels || output.size == 0)
    {
        return unexpected(audiofile_error::invalid_argument);
    }
#ifdef KFR_BASETYPE_F32
    size_t framesRead = mp3dec_ex_read(&*decex, output.data, output.total_samples()) / m_format->channels;
#else
    std::vector<float> temp(output.total_samples());
    size_t framesRead = mp3dec_ex_read(&*decex, temp.data(), output.total_samples()) / m_format->channels;
    samples_load(output.data, temp.data(), output.channels * framesRead, false);
    temp = {};
#endif
    if (framesRead == 0) /* normal eof or error condition */
    {
        if (decex->last_error)
        {
            return unexpected(audiofile_error::format_error);
        }
        else
        {
            return unexpected(audiofile_error::end_of_file);
        }
    }
    return framesRead;
}

expected<void, audiofile_error> MP3Decoder::seek(uint64_t position)
{
    if (!m_reader)
        return unexpected(audiofile_error::closed);
    if (mp3dec_ex_seek(&*decex, position * m_format->channels))
    {
        return unexpected(audiofile_error::format_error);
    }
    return {};
}

std::unique_ptr<audio_decoder> create_mp3_decoder(const mp3_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new MP3Decoder(options));
}
} // namespace kfr
