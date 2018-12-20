/** @addtogroup io
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../base/basic_expressions.hpp"
#include "../base/conversion.hpp"
#include "../base/univector.hpp"
#include "../base/vec.hpp"
#include "../cometa/ctti.hpp"
#include "file.hpp"

#ifndef KFR_ENABLE_WAV
#define KFR_ENABLE_WAV 1
#endif
#ifndef KFR_ENABLE_FLAC
#define KFR_ENABLE_FLAC 0
#endif

#if KFR_ENABLE_WAV
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_CONVERSION_API
#include "dr/dr_wav.h"
#endif
#if KFR_ENABLE_FLAC
#define DR_FLAC_NO_STDIO
#define DR_FLAC_NO_CONVERSION_API
#include "dr/dr_flac.h"
#endif

namespace kfr
{

struct audio_format
{
    size_t channels        = 2;
    audio_sample_type type = audio_sample_type::i16;
    fmax samplerate        = 44100;
    bool use_w64           = false;
};

struct audio_format_and_length : audio_format
{
    using audio_format::audio_format;
    audio_format_and_length(const audio_format& fmt) : audio_format(fmt) {}

    imax length = 0; // in samples
};

template <typename T>
struct audio_reader : public abstract_reader<T>
{

    /// @brief Returns audio format description
    virtual const audio_format_and_length& format() const = 0;
};

template <typename T>
struct audio_writer : public abstract_writer<T>
{

    /// @brief Returns audio format description
    virtual const audio_format_and_length& format() const = 0;

    /// @brief Finishes writing and closes underlying writer
    virtual void close() = 0;
};

namespace internal
{
#if KFR_ENABLE_WAV
static size_t drwav_writer_write_proc(abstract_writer<void>* file, const void* pData, size_t bytesToWrite)
{
    return file->write(pData, bytesToWrite);
}
static drwav_bool32 drwav_writer_seek_proc(abstract_writer<void>* file, int offset, drwav_seek_origin origin)
{
    return file->seek(offset, origin == drwav_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
static size_t drwav_reader_read_proc(abstract_reader<void>* file, void* pBufferOut, size_t bytesToRead)
{
    return file->read(pBufferOut, bytesToRead);
}
static drwav_bool32 drwav_reader_seek_proc(abstract_reader<void>* file, int offset, drwav_seek_origin origin)
{
    return file->seek(offset, origin == drwav_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
#endif
#if KFR_ENABLE_FLAC
static size_t drflac_reader_read_proc(abstract_reader<void>* file, void* pBufferOut, size_t bytesToRead)
{
    return file->read(pBufferOut, bytesToRead);
}
static drflac_bool32 drflac_reader_seek_proc(abstract_reader<void>* file, int offset,
                                             drflac_seek_origin origin)
{
    return file->seek(offset, origin == drflac_seek_origin_start ? seek_origin::begin : seek_origin::current);
}
#endif

} // namespace internal

#if KFR_ENABLE_WAV
/// @brief WAV format writer
template <typename T>
struct audio_writer_wav : audio_writer<T>
{
    /// @brief Constructs WAV writer using target writer and format
    audio_writer_wav(std::shared_ptr<abstract_writer<>>&& writer, const audio_format& fmt)
        : writer(std::move(writer)), f(nullptr), fmt(fmt)
    {
        drwav_data_format wav_fmt;
        wav_fmt.channels   = fmt.channels;
        wav_fmt.sampleRate = fmt.samplerate;
        wav_fmt.format =
            fmt.type >= audio_sample_type::first_float ? DR_WAVE_FORMAT_IEEE_FLOAT : DR_WAVE_FORMAT_PCM;
        wav_fmt.bitsPerSample = audio_sample_bit_depth(fmt.type);
        wav_fmt.container     = fmt.use_w64 ? drwav_container_w64 : drwav_container_riff;
        f = drwav_open_write(&wav_fmt, (drwav_write_proc)&internal::drwav_writer_write_proc,
                             (drwav_seek_proc)&internal::drwav_writer_seek_proc, this->writer.get());
    }
    ~audio_writer_wav() { close(); }

    /// @brief Write data to underlying binary writer
    size_t write(const T* data, size_t size) override
    {
        if (!f)
            return 0;
        if (fmt.type == audio_sample_type::unknown)
            return 0;
        if (fmt.type == audio_sample_traits<T>::type)
        {
            const size_t sz = drwav_write(f, size, data);
            fmt.length += sz / fmt.channels;
            return sz;
        }
        else
        {
            univector<uint8_t> native(size * audio_sample_sizeof(fmt.type));
            convert(native.data(), fmt.type, data, size);
            const size_t sz = drwav_write(f, size, native.data());
            fmt.length += sz / fmt.channels;
            return sz;
        }
    }

    void close() override
    {
        drwav_close(f);
        f = nullptr;
        writer.reset();
    }

    const audio_format_and_length& format() const override { return fmt; }

    imax tell() const override { return fmt.length; }

    bool seek(imax position, seek_origin origin) override { return false; }

private:
    std::shared_ptr<abstract_writer<>> writer;
    drwav* f;
    audio_format_and_length fmt;
};

/// @brief WAV format reader
template <typename T>
struct audio_reader_wav : audio_reader<T>
{
    /// @brief Constructs WAV reader
    audio_reader_wav(std::shared_ptr<abstract_reader<>>&& reader) : reader(std::move(reader))
    {
        f              = drwav_open((drwav_read_proc)&internal::drwav_reader_read_proc,
                       (drwav_seek_proc)&internal::drwav_reader_seek_proc, this->reader.get());
        fmt.channels   = f->channels;
        fmt.samplerate = f->sampleRate;
        fmt.length     = f->totalSampleCount / fmt.channels;
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
    ~audio_reader_wav() { drwav_close(f); }

    /// @brief Returns audio format description
    const audio_format_and_length& format() const override { return fmt; }

    /// @brief Reads and decodes audio data
    size_t read(T* data, size_t size) override
    {
        if (fmt.type == audio_sample_type::unknown)
            return 0;
        if (fmt.type == audio_sample_traits<T>::type)
        {
            return drwav_read(f, size, data);
        }
        else
        {
            univector<uint8_t> native(size * audio_sample_sizeof(fmt.type));
            const size_t sz = drwav_read(f, size, native.data());
            convert(data, native.data(), fmt.type, sz);
            return sz;
        }
    }

    /// @brief Returns current position
    imax tell() const override { return position; }

    /// @brief Seeks to specific sample
    bool seek(imax offset, seek_origin origin) override
    {
        switch (origin)
        {
        case seek_origin::current:
            return drwav_seek_to_sample(f, this->position + offset);
        case seek_origin::begin:
            return drwav_seek_to_sample(f, offset);
        case seek_origin::end:
            return drwav_seek_to_sample(f, fmt.length + offset);
        default:
            return false;
        }
    }

private:
    std::shared_ptr<abstract_reader<>> reader;
    drwav* f;
    audio_format_and_length fmt;
    imax position = 0;
};
#endif

#if KFR_ENABLE_FLAC

/// @brief FLAC format reader
template <typename T>
struct audio_reader_flac : audio_reader<T>
{
    /// @brief Constructs FLAC reader
    audio_reader_flac(std::shared_ptr<abstract_reader<>>&& reader) : reader(std::move(reader))
    {
        f              = drflac_open((drflac_read_proc)&internal::drflac_reader_read_proc,
                        (drflac_seek_proc)&internal::drflac_reader_seek_proc, this->reader.get());
        fmt.channels   = f->channels;
        fmt.samplerate = f->sampleRate;
        fmt.length     = f->totalSampleCount / fmt.channels;
        fmt.type       = audio_sample_type::i32;
    }
    ~audio_reader_flac() { drflac_close(f); }

    /// @brief Returns audio format description
    const audio_format_and_length& format() const override { return fmt; }

    /// @brief Reads and decodes audio data
    size_t read(T* data, size_t size) override
    {
        if (fmt.type == audio_sample_type::unknown)
            return 0;
        if (audio_sample_traits<T>::type == audio_sample_type::i32)
        {
            return drflac_read_s32(f, size, reinterpret_cast<i32*>(data));
        }
        else
        {
            univector<i32> native(size * sizeof(i32));
            const size_t sz = drflac_read_s32(f, size, native.data());
            convert(data, native.data(), sz);
            return sz;
        }
    }

    /// @brief Returns current position
    imax tell() const override { return position; }

    /// @brief Seeks to specific sample
    bool seek(imax offset, seek_origin origin) override
    {
        switch (origin)
        {
        case seek_origin::current:
            return drflac_seek_to_sample(f, this->position + offset);
        case seek_origin::begin:
            return drflac_seek_to_sample(f, offset);
        case seek_origin::end:
            return drflac_seek_to_sample(f, fmt.length + offset);
        default:
            return false;
        }
    }

private:
    std::shared_ptr<abstract_reader<>> reader;
    drflac* f;
    audio_format_and_length fmt;
    imax position = 0;
};
#endif

} // namespace kfr
