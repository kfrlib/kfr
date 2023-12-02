/** @addtogroup io
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

#include <kfr/io/audiofile.hpp>
CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wimplicit-fallthrough")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wunused-function")

#ifndef KFR_DISABLE_WAV
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_IMPLEMENTATION
#include "dr/dr_wav.h"
#endif
#ifndef KFR_DISABLE_FLAC
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#include "dr/dr_flac.h"
#endif
#ifndef KFR_DISABLE_MP3
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "dr/dr_mp3.h"
#endif

namespace kfr
{

namespace internal_generic
{
#ifndef KFR_DISABLE_WAV
size_t drwav_writer_write_proc(abstract_writer<void>* file, const void* pData, size_t bytesToWrite)
{
    return file->write(pData, bytesToWrite);
}
drwav_bool32 drwav_writer_seek_proc(abstract_writer<void>* file, int offset, drwav_seek_origin origin)
{
    return file->seek(offset, origin == drwav_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
size_t drwav_reader_read_proc(abstract_reader<void>* file, void* pBufferOut, size_t bytesToRead)
{
    return file->read(pBufferOut, bytesToRead);
}
drwav_bool32 drwav_reader_seek_proc(abstract_reader<void>* file, int offset, drwav_seek_origin origin)
{
    return file->seek(offset, origin == drwav_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
#endif
#ifndef KFR_DISABLE_FLAC
size_t drflac_reader_read_proc(abstract_reader<void>* file, void* pBufferOut, size_t bytesToRead)
{
    return file->read(pBufferOut, bytesToRead);
}
drflac_bool32 drflac_reader_seek_proc(abstract_reader<void>* file, int offset, drflac_seek_origin origin)
{
    return file->seek(offset, origin == drflac_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
#endif
#ifndef KFR_DISABLE_MP3
size_t drmp3_reader_read_proc(abstract_reader<void>* file, void* pBufferOut, size_t bytesToRead)
{
    return file->read(pBufferOut, bytesToRead);
}
drmp3_bool32 drmp3_reader_seek_proc(abstract_reader<void>* file, int offset, drmp3_seek_origin origin)
{
    return file->seek(offset, origin == drmp3_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
#endif

struct wav_file : drwav
{
};
struct flac_file : drflac
{
};
struct mp3_file : drmp3
{
};

void wav_file_deleter::operator()(wav_file* f)
{
    drwav_uninit(f);
    delete f;
}

void flac_file_deleter::operator()(flac_file* f)
{
    drflac_close(f);
}
void mp3_file_deleter::operator()(mp3_file* f)
{
    drmp3_uninit(f);
    delete f;
}

} // namespace internal_generic

template <typename T>
audio_writer_wav<T>::audio_writer_wav(std::shared_ptr<abstract_writer<>>&& writer, const audio_format& fmt)
    : writer(std::move(writer)), fmt(fmt)
{
    drwav_data_format wav_fmt;
    wav_fmt.channels   = static_cast<drwav_uint32>(fmt.channels);
    wav_fmt.sampleRate = static_cast<drwav_uint32>(fmt.samplerate);
    wav_fmt.format =
        fmt.type >= audio_sample_type::first_float ? DR_WAVE_FORMAT_IEEE_FLOAT : DR_WAVE_FORMAT_PCM;
    wav_fmt.bitsPerSample = static_cast<drwav_uint32>(audio_sample_bit_depth(fmt.type));
    wav_fmt.container     = fmt.use_w64 ? drwav_container_w64 : drwav_container_riff;
    f.reset(new internal_generic::wav_file());
    if (!drwav_init_write(f.get(), &wav_fmt, (drwav_write_proc)&internal_generic::drwav_writer_write_proc,
                          (drwav_seek_proc)&internal_generic::drwav_writer_seek_proc, this->writer.get(),
                          nullptr))
    {
        delete f.release();
    }
}

template <typename T>
size_t audio_writer_wav<T>::write(const T* data, size_t size)
{
    if (!f)
        return 0;
    if (fmt.type == audio_sample_type::unknown)
        return 0;
    if (fmt.type == audio_sample_traits<T>::type)
    {
        const size_t sz = drwav_write_pcm_frames_le(f.get(), size, data);
        fmt.length += sz;
        return sz * fmt.channels;
    }
    else
    {
        univector<uint8_t> native(size * audio_sample_sizeof(fmt.type));
        convert(native.data(), fmt.type, data, size);
        const size_t sz = drwav_write_pcm_frames_le(f.get(), size / fmt.channels, native.data());
        fmt.length += sz;
        return sz * fmt.channels;
    }
}

template <typename T>
audio_writer_wav<T>::~audio_writer_wav()
{
}

template <typename T>
void audio_writer_wav<T>::close()
{
    f.reset();
    writer.reset();
}

template struct audio_writer_wav<i16>;
template struct audio_writer_wav<i24>;
template struct audio_writer_wav<i32>;
template struct audio_writer_wav<f32>;
template struct audio_writer_wav<f64>;

template <typename T>
audio_reader_wav<T>::audio_reader_wav(std::shared_ptr<abstract_reader<>>&& reader) : reader(std::move(reader))
{
    f.reset(new internal_generic::wav_file());
    drwav_init(f.get(), (drwav_read_proc)&internal_generic::drwav_reader_read_proc,
               (drwav_seek_proc)&internal_generic::drwav_reader_seek_proc, this->reader.get(), nullptr);
    fmt.channels   = f->channels;
    fmt.samplerate = f->sampleRate;
    fmt.length     = static_cast<imax>(f->totalPCMFrameCount);
    switch (f->translatedFormatTag)
    {
    case DR_WAVE_FORMAT_IEEE_FLOAT:
        switch (f->bitsPerSample)
        {
        case 32:
            fmt.type = audio_sample_type::f32;
            break;
        case 64:
            fmt.type = audio_sample_type::f64;
            break;
        default:
            fmt.type = audio_sample_type::unknown;
            break;
        }
        break;
    case DR_WAVE_FORMAT_PCM:
        switch (f->bitsPerSample)
        {
        case 8:
            fmt.type = audio_sample_type::i8;
            break;
        case 16:
            fmt.type = audio_sample_type::i16;
            break;
        case 24:
            fmt.type = audio_sample_type::i24;
            break;
        case 32:
            fmt.type = audio_sample_type::i32;
            break;
        case 64:
            fmt.type = audio_sample_type::i64;
            break;
        default:
            fmt.type = audio_sample_type::unknown;
            break;
        }
        break;
    default:
        fmt.type = audio_sample_type::unknown;
        break;
    }
}
template <typename T>
audio_reader_wav<T>::~audio_reader_wav()
{
}

template <typename T>
size_t audio_reader_wav<T>::read(T* data, size_t size)
{
    if (fmt.type == audio_sample_type::unknown)
        return 0;
    if (fmt.type == audio_sample_traits<T>::type)
    {
        const size_t sz = drwav_read_pcm_frames(f.get(), size / fmt.channels, data);
        position += sz;
        return sz * fmt.channels;
    }
    else
    {
        univector<uint8_t> native(size * audio_sample_sizeof(fmt.type));
        const size_t sz = drwav_read_pcm_frames(f.get(), size / fmt.channels, native.data());
        position += sz;
        convert(data, native.data(), fmt.type, sz * fmt.channels);
        return sz * fmt.channels;
    }
}

template <typename T>
bool audio_reader_wav<T>::seek(imax offset, seek_origin origin)
{
    switch (origin)
    {
    case seek_origin::current:
        return drwav_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(this->position + offset));
    case seek_origin::begin:
        return drwav_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(offset));
    case seek_origin::end:
        return drwav_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(fmt.length + offset));
    }
    return false;
}

template struct audio_reader_wav<i16>;
template struct audio_reader_wav<i24>;
template struct audio_reader_wav<i32>;
template struct audio_reader_wav<f32>;
template struct audio_reader_wav<f64>;

template <typename T>
audio_reader_flac<T>::audio_reader_flac(std::shared_ptr<abstract_reader<>>&& reader)
    : reader(std::move(reader))
{
    f.reset(reinterpret_cast<internal_generic::flac_file*>(drflac_open(
        (drflac_read_proc)&internal_generic::drflac_reader_read_proc,
        (drflac_seek_proc)&internal_generic::drflac_reader_seek_proc, this->reader.get(), nullptr)));
    fmt.channels   = f->channels;
    fmt.samplerate = f->sampleRate;
    fmt.length     = static_cast<imax>(f->totalPCMFrameCount);
    fmt.type       = audio_sample_type::i32;
}
template <typename T>
audio_reader_flac<T>::~audio_reader_flac()
{
}

template <typename T>
size_t audio_reader_flac<T>::read(T* data, size_t size)
{
    if (fmt.type == audio_sample_type::unknown)
        return 0;
    if (audio_sample_traits<T>::type == audio_sample_type::i32)
    {
        const size_t sz =
            drflac_read_pcm_frames_s32(f.get(), size / fmt.channels, reinterpret_cast<i32*>(data));
        position += sz;
        return sz * fmt.channels;
    }
    else
    {
        univector<i32> native(size * sizeof(i32));
        const size_t sz = drflac_read_pcm_frames_s32(f.get(), size / fmt.channels, native.data());
        position += sz;
        convert(data, native.data(), sz * fmt.channels);
        return sz * fmt.channels;
    }
}

template <typename T>
bool audio_reader_flac<T>::seek(imax offset, seek_origin origin)
{
    switch (origin)
    {
    case seek_origin::current:
        return drflac_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(this->position + offset));
    case seek_origin::begin:
        return drflac_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(offset));
    case seek_origin::end:
        return drflac_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(fmt.length + offset));
    }
    return false;
}

template struct audio_reader_flac<i16>;
template struct audio_reader_flac<i24>;
template struct audio_reader_flac<i32>;
template struct audio_reader_flac<f32>;
template struct audio_reader_flac<f64>;

static_assert(sizeof(drmp3_config) == sizeof(uint32_t) * 2);
static_assert(sizeof(mp3_config) == sizeof(uint32_t) * 2);

template <typename T>
audio_reader_mp3<T>::audio_reader_mp3(std::shared_ptr<abstract_reader<>>&& reader) : reader(std::move(reader))
{
    f.reset(new internal_generic::mp3_file());
    drmp3_init(f.get(), (drmp3_read_proc)&internal_generic::drmp3_reader_read_proc,
               (drmp3_seek_proc)&internal_generic::drmp3_reader_seek_proc, this->reader.get(),
               reinterpret_cast<const drmp3_config*>(&config), nullptr);
    fmt.channels   = f->channels;
    fmt.samplerate = f->sampleRate;
    fmt.length     = static_cast<imax>(drmp3_get_pcm_frame_count(f.get()));
    fmt.type       = audio_sample_type::i16;
}
template <typename T>
audio_reader_mp3<T>::~audio_reader_mp3()
{
}

template <typename T>
size_t audio_reader_mp3<T>::read(T* data, size_t size)
{
    if (fmt.type == audio_sample_type::unknown)
        return 0;
    if (audio_sample_traits<T>::type == audio_sample_type::i16)
    {
        const size_t sz =
            drmp3_read_pcm_frames_s16(f.get(), size / fmt.channels, reinterpret_cast<i16*>(data));
        position += sz;
        return sz * fmt.channels;
    }
    else
    {
        univector<i16> native(size * sizeof(i16));
        const size_t sz = drmp3_read_pcm_frames_s16(f.get(), size / fmt.channels, native.data());
        position += sz;
        convert(data, native.data(), sz * fmt.channels);
        return sz * fmt.channels;
    }
}

template <typename T>
bool audio_reader_mp3<T>::seek(imax offset, seek_origin origin)
{
    switch (origin)
    {
    case seek_origin::current:
        return drmp3_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(this->position + offset));
    case seek_origin::begin:
        return drmp3_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(offset));
    case seek_origin::end:
        return drmp3_seek_to_pcm_frame(f.get(), static_cast<drmp3_uint64>(fmt.length + offset));
    }
    return false;
}

template struct audio_reader_mp3<i16>;
template struct audio_reader_mp3<i24>;
template struct audio_reader_mp3<i32>;
template struct audio_reader_mp3<f32>;
template struct audio_reader_mp3<f64>;

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
