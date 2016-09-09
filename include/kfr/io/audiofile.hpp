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
#include "../base/univector.hpp"
#include "../base/vec.hpp"
#include "../cometa/ctti.hpp"
#include "file.hpp"

namespace kfr
{

template <typename Tout, typename Tin, size_t Tag1, size_t Tag2, typename E1>
void write_interleaved(E1&& dest, const univector2d<Tin, Tag1, Tag2>& src)
{
    const size_t channels = src.size();
    if (channels == 1)
    {
        process<Tout>(std::forward<E1>(dest), src[0]);
    }
    else if (channels == 2)
    {
        process<Tout>(std::forward<E1>(dest), pack(src[0], src[1]), 0, infinite_size, csize<2>);
    }
    else
    {
        const size_t size = src[0].size();
        internal::expression_writer<Tout, E1> wr = writer<Tout>(std::forward<E1>(dest));
        for (size_t i = 0; i < size; i++)
            for (size_t ch = 0; ch < channels; ch++)
                wr.write(src[ch][i]);
    }
}

enum class audiodatatype
{
    unknown,
    i16,
    i24,
    i24a32,
    i32,
    f32,
    f64
};

namespace internal
{
template <typename T>
constexpr range<fmax> audio_range()
{
    return { -std::numeric_limits<T>::max(), std::numeric_limits<T>::max() };
}

template <>
constexpr range<fmax> audio_range<f32>()
{
    return { -1.0, +1.0 };
}

template <>
constexpr range<fmax> audio_range<f64>()
{
    return { -1.0, +1.0 };
}

inline size_t get_audiobitdepth(audiodatatype type)
{
    return (size_t[]){ 0, 16, 24, 24, 32, 32, 64 }[static_cast<int>(type)];
}

template <typename T>
inline audiodatatype get_audiodatatype()
{
    if (ctypeid<T>() == ctypeid<i16>())
        return audiodatatype::i16;
    else if (ctypeid<T>() == ctypeid<i32>())
        return audiodatatype::i32;
    else if (ctypeid<T>() == ctypeid<f32>())
        return audiodatatype::f32;
    else if (ctypeid<T>() == ctypeid<f64>())
        return audiodatatype::f64;
    else
        return audiodatatype::unknown;
}
}

struct audioformat
{
    size_t channels;
    size_t samples;
    audiodatatype type;
    fmax samplerate;

    template <typename T, size_t Tag1, size_t Tag2>
    constexpr audioformat(const univector2d<T, Tag1, Tag2>& data, fmax sample_rate)
        : channels(data.size()), samples(data[0].size()), type(internal::get_audiodatatype<T>()),
          samplerate(sample_rate)
    {
    }
};

namespace internal
{
static constexpr u32 FourCC(const char (&ch)[5])
{
    return u32(u8(ch[0])) | u32(u8(ch[1])) << 8 | u32(u8(ch[2])) << 16 | u32(u8(ch[3])) << 24;
}

struct WAV_FMT
{
    i32 fId; // 'fmt '
    i32 pcmHeaderLength;
    i16 wFormatTag;
    i16 numChannels;
    i32 nSamplesPerSec;
    i32 nAvgBytesPerSec;
    i16 numBlockAlingn;
    i16 numBitsPerSample;
} __attribute__((packed));

struct WAV_DATA
{
    i32 dId; // 'data' or 'fact'
    i32 dLen;
    u8 data[1];
} __attribute__((packed));

struct WAV_DATA_HDR
{
    i32 dId; // 'data' or 'fact'
    i32 dLen;
} __attribute__((packed));

struct AIFF_FMT
{
    i32 chunkID;
    i32 chunkLen;
    i16 channels;
    u32 frames;
    i16 bitsPerSample;
    f80 sampleRate;
    i32 compression;
} __attribute__((packed));

struct AIFF_DATA
{
    i32 chunkID;
    i32 chunkLen;
    u32 offset;
} __attribute__((packed));

constexpr u32 cWAVE_FORMAT_PCM  = 1;
constexpr u32 cWAVE_FORMAT_IEEE = 3;

constexpr u32 ccRIFF = FourCC("RIFF");
constexpr u32 ccWAVE = FourCC("WAVE");
constexpr u32 ccfmt  = FourCC("fmt ");
constexpr u32 ccdata = FourCC("data");

constexpr u32 ccFORM = FourCC("FORM");
constexpr u32 ccAIFF = FourCC("AIFF");
constexpr u32 ccAIFC = FourCC("AIFC");
constexpr u32 ccCOMM = FourCC("COMM");
constexpr u32 ccSSND = FourCC("SSND");
constexpr u32 ccNONE = FourCC("NONE");
constexpr u32 ccsowt = FourCC("sowt");

struct RIFF_HDR
{
    i32 riffID; // 'RIFF' or 'COMM'
    i32 fileLen;
    i32 formatID; // 'WAVE' or 'AIFF'
} __attribute__((packed));

struct WAV_HEADER
{
    RIFF_HDR riff;
    WAV_FMT fmt;
    WAV_DATA_HDR data;

} __attribute__((packed));

struct CHUNK_HDR
{
    i32 chunkID;
    i32 chunkLen;
} __attribute__((packed));

static bool audio_test_wav(const array_ref<u8>& rawbytes)
{
    if (rawbytes.size() < sizeof(RIFF_HDR))
    {
        return false;
    }
    const RIFF_HDR* hdr = reinterpret_cast<const RIFF_HDR*>(rawbytes.data());
    if (hdr->riffID != ccRIFF)
    {
        return false;
    }
    if (hdr->formatID != ccWAVE)
    {
        return false;
    }
    return true;
}

static bool audio_test_aiff(const array_ref<u8>& rawbytes)
{
    if (rawbytes.size() < sizeof(RIFF_HDR))
    {
        return false;
    }
    const RIFF_HDR* hdr = reinterpret_cast<const RIFF_HDR*>(rawbytes.data());
    if (hdr->riffID != ccFORM)
    {
        return false;
    }
    if (hdr->formatID != ccAIFF && hdr->formatID != ccAIFC)
    {
        return false;
    }
    return true;
}

enum class file_status
{
    ok,
    unknown_format,
    bad_format,
    unsupported_compression,
    unsupported_bit_format
};

static file_status audio_info_wav(audioformat& info, const array_ref<u8>& rawbytes)
{
    const CHUNK_HDR* chunk  = ptr_cast<CHUNK_HDR>(rawbytes.data() + 12);
    const void* end         = ptr_cast<char>(rawbytes.end());
    const WAV_FMT* fmt      = nullptr;
    const WAV_DATA* rawdata = nullptr;
    while (chunk < end)
    {
        switch (chunk->chunkID)
        {
        case ccfmt:
            fmt = ptr_cast<WAV_FMT>(chunk);
            break;
        case ccdata:
            rawdata = ptr_cast<WAV_DATA>(chunk);
            break;
        }
        chunk = ptr_cast<CHUNK_HDR>(ptr_cast<u8>(chunk) + chunk->chunkLen + 8);
    }
    if (!fmt || !rawdata)
    {
        return file_status::bad_format;
    }

    if (fmt->wFormatTag != cWAVE_FORMAT_PCM && fmt->wFormatTag != cWAVE_FORMAT_IEEE)
    {
        return file_status::unsupported_compression;
    }

    int storedbits = fmt->numBlockAlingn * 8 / fmt->numChannels;
    if (fmt->wFormatTag == cWAVE_FORMAT_PCM && fmt->numBitsPerSample == 16 && storedbits == 16)
    {
        info.type = audiodatatype::i16;
    }
    else if (fmt->wFormatTag == cWAVE_FORMAT_PCM && fmt->numBitsPerSample == 24 && storedbits == 24)
    {
        info.type = audiodatatype::i24;
    }
    else if (fmt->wFormatTag == cWAVE_FORMAT_PCM && fmt->numBitsPerSample == 24 && storedbits == 32)
    {
        info.type = audiodatatype::i24a32;
    }
    else if (fmt->wFormatTag == cWAVE_FORMAT_PCM && fmt->numBitsPerSample == 32 && storedbits == 32)
    {
        info.type = audiodatatype::i32;
    }
    else if (fmt->wFormatTag == cWAVE_FORMAT_IEEE && fmt->numBitsPerSample == 32 && storedbits == 32)
    {
        info.type = audiodatatype::f32;
    }
    else if (fmt->wFormatTag == cWAVE_FORMAT_IEEE && fmt->numBitsPerSample == 64 && storedbits == 64)
    {
        info.type = audiodatatype::f64;
    }
    else
    {
        return file_status::unsupported_bit_format;
    }

    if (fmt->numChannels < 1 || fmt->numChannels > 16)
        return file_status::unsupported_bit_format;

    info.channels   = size_t(fmt->numChannels);
    info.samplerate = size_t(fmt->nSamplesPerSec);
    info.samples    = size_t(rawdata->dLen) / info.channels / (get_audiobitdepth(info.type) / 8);

    return file_status::ok;
}

static file_status audio_info(audioformat& info, const array_ref<u8>& file_bytes)
{
    if (audio_test_wav(file_bytes))
        return audio_info_wav(info, file_bytes);
    else
        return file_status::unknown_format;
}
}

template <size_t = 0>
void audio_encode_header(internal::expression_sequential_file_writer& dest, const audioformat& info)
{
    using namespace internal;
    WAV_HEADER hdr;
    zeroize(hdr);
    const size_t framesize   = info.channels * get_audiobitdepth(info.type) / 8;
    hdr.riff.riffID          = ccRIFF;
    hdr.riff.formatID        = ccWAVE;
    hdr.riff.fileLen         = autocast(info.samples * framesize + sizeof(hdr) - 8);
    hdr.fmt.fId              = ccfmt;
    hdr.fmt.pcmHeaderLength  = autocast(sizeof(hdr.fmt) - sizeof(CHUNK_HDR));
    hdr.fmt.numBlockAlingn   = autocast(framesize);
    hdr.fmt.nAvgBytesPerSec  = autocast(info.samplerate * framesize);
    hdr.fmt.nSamplesPerSec   = autocast(info.samplerate);
    hdr.fmt.numChannels      = autocast(info.channels);
    hdr.fmt.wFormatTag       = info.type >= audiodatatype::f32 ? cWAVE_FORMAT_IEEE : cWAVE_FORMAT_PCM;
    hdr.fmt.numBitsPerSample = autocast(get_audiobitdepth(info.type));
    hdr.data.dId             = ccdata;
    hdr.data.dLen            = autocast(info.samples * framesize);

    dest.write(hdr);
}

template <typename T, size_t Tag1, size_t Tag2>
void audio_encode_audio(internal::expression_sequential_file_writer& dest,
                        const univector2d<T, Tag1, Tag2>& audio)
{
    write_interleaved<T>(dest, audio);
}

template <typename T, size_t Tag1, size_t Tag2>
void audio_encode(internal::expression_sequential_file_writer& dest, const univector2d<T, Tag1, Tag2>& audio,
                  const audioformat& info)
{
    audio_encode_header(dest, info);
    audio_encode_audio(dest, audio);
}
}
