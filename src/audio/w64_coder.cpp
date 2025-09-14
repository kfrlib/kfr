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
#include "riff.hpp"

namespace kfr
{

#pragma pack(push, 1)

struct W64GUID
{
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];

    bool operator==(const W64GUID& other) const = default;
};

// 66666972-912E-11CF-A5D6-28DB04C10000
constexpr inline W64GUID guidRIFF = {
    0x66666972, 0x912E, 0x11CF, { 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00 }
};
// 65766177-ACF3-11D3-8CD1-00C04F8EDB8A
constexpr inline W64GUID guidWAVE = {
    0x65766177, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A }
};
// 20746D66-ACF3-11D3-8CD1-00C04F8EDB8A
constexpr inline W64GUID guidFMT = {
    0x20746D66, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A }
};
// 61746164-ACF3-11D3-8CD1-00C04F8EDB8A
constexpr inline W64GUID guidDATA = {
    0x61746164, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A }
};
// 7473696C-912F-11CF-A5D6-28DB04C10000
constexpr inline W64GUID guidLIST = {
    0x7473696C, 0x912F, 0x11CF, { 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00 }
};
//  74786562-ACF3-11D3-8CD1-00C04F8EDB8A
constexpr inline W64GUID guidBEXT = {
    0x74786562, 0xACF3, 0x11D3, { 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, 0xDB, 0x8A }
};

struct W64Header
{
    W64GUID riff = guidRIFF;
    uint64_t riffSize{ 0 };
    W64GUID wave = guidWAVE;
};

struct WaveFmt
{
    uint16_t formatTag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t avgBytesPerSec;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
};

struct WaveFmtEx
{
    WaveFmt fmt;

    uint16_t extendedSize;
    uint16_t validBitsPerSample;
    uint32_t channelMask;
    uint16_t formatTagEx;
    uint8_t subFormatContinue[14];
};

#pragma pack(pop)

struct W64Traits
{
    using MainHeader                                     = W64Header;
    using IDType                                         = W64GUID;
    using SizeType                                       = uint64_t;
    constexpr static int chunkAlignment                  = 8;
    constexpr static bool chunkSizeIsAligned             = false;
    constexpr static std::optional<IDType> metadataChunk = guidLIST;
    constexpr static IDType audioChunk                   = guidDATA;
    using MetadataParser                                 = RIFFMetadataParser;
    using EncodingOptions                                = w64_encoding_options;
    using DecodingOptions                                = w64_decoding_options;

#pragma pack(push, 1)
    struct ChunkType
    {
        IDType id;
        SizeType size;
    };
#pragma pack(pop)

    [[nodiscard]] static expected<uint64_t, audiofile_error> chunkGetSize(const ChunkType& chunk,
                                                                          uint64_t position,
                                                                          RIFFContainer<W64Traits>*)
    {
        return chunk.size - sizeof(ChunkType);
    }
    [[nodiscard]] static expected<void, audiofile_error> chunkSetSize(ChunkType& chunk, uint64_t position,
                                                                      uint64_t byteSize,
                                                                      RIFFContainer<W64Traits>*)
    {
        chunk.size = byteSize + sizeof(ChunkType);
        return {};
    }
};

class W64Container : public RIFFContainer<W64Traits>
{
public:
    using Super = RIFFContainer<W64Traits>;

    enum : uint16_t
    {
        WAVE_FORMAT_PCM        = 0x1,
        WAVE_FORMAT_IEEE_FLOAT = 0x3,
        WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
    };

    [[nodiscard]] expected<audiofile_metadata, audiofile_error> readFormat();

    [[nodiscard]] expected<audio_data, audiofile_error> readAudio(const audiofile_metadata* meta);

    [[nodiscard]] expected<void, audiofile_error> seek(const audiofile_metadata* meta, uint64_t position);

    [[nodiscard]] expected<void, audiofile_error> writeFormat(const audiofile_metadata* meta);
    [[nodiscard]] expected<void, audiofile_error> writeAudio(const audio_data& data,
                                                             const audio_quantinization& quantinization);

    [[nodiscard]] expected<void, audiofile_error> finalize();

    [[nodiscard]] expected<void, audiofile_error> copyInitialChunksFrom(RIFFContainer* sourceContainer);

    ~W64Container() {}
    bool finalized                = false;
    audiofile_container container = audiofile_container::unknown;
};

struct W64Decoder : public RIFFDecoder<W64Container>
{
    using RIFFDecoder<W64Container>::RIFFDecoder;
};

expected<audiofile_metadata, audiofile_error> W64Container::readFormat()
{
    audiofile_metadata meta;
    if (m_header.riff == guidRIFF && m_header.wave == guidWAVE)
    {
        meta.container = audiofile_container::w64;
    }
    else
    {
        return unexpected(audiofile_error::format_error);
    }

    WaveFmtEx fmt;
    fmt.extendedSize = 0;
    if (!this->findChunk(guidDATA))
        return unexpected(audiofile_error::format_error);
    if (!this->readChunkTo(guidFMT, fmt) && !this->readChunkTo(guidFMT, fmt.fmt))
        return unexpected(audiofile_error::format_error);

    if (fmt.extendedSize == 0 && fmt.fmt.formatTag == WAVE_FORMAT_EXTENSIBLE)
        return unexpected(audiofile_error::format_error);
    uint16_t formatTag = fmt.fmt.formatTag == WAVE_FORMAT_EXTENSIBLE ? fmt.formatTagEx : fmt.fmt.formatTag;

    meta.channels = fmt.fmt.channels;
    meta.codec    = formatTag == WAVE_FORMAT_PCM          ? audiofile_codec::lpcm
                    : formatTag == WAVE_FORMAT_IEEE_FLOAT ? audiofile_codec::ieee_float
                                                          : audiofile_codec::unknown;
    if (meta.codec == audiofile_codec::unknown)
    {
        return unexpected(audiofile_error::format_error);
    }
    meta.sample_rate = fmt.fmt.sample_rate;
    meta.bit_depth   = fmt.fmt.bitsPerSample;
    if (meta.bit_depth == 0)
    {
        return unexpected(audiofile_error::format_error);
    }
    meta.total_frames = m_chunks[*this->findChunk(guidDATA)].byteSize / meta.bytes_per_pcm_frame();
    if (!meta.valid())
    {
        return unexpected(audiofile_error::format_error);
    }

    return meta;
}

expected<audio_data, audiofile_error> W64Container::readAudio(const audiofile_metadata* meta)
{
    if (!m_currentChunkToRead)
        if (auto e = readChunkStart(guidDATA); !e)
            return unexpected(e.error());
    return Super::readPCMAudio(default_audio_frames_to_read, meta);
}

[[nodiscard]] expected<void, audiofile_error> W64Container::seek(const audiofile_metadata* meta,
                                                                 uint64_t position)
{
    if (!m_currentChunkToRead)
        if (auto e = readChunkStart(guidDATA); !e)
            return unexpected(e.error());
    return readChunkSeek(position * meta->bytes_per_pcm_frame());
}

expected<void, audiofile_error> W64Container::copyInitialChunksFrom(RIFFContainer* sourceContainer)
{
    if (auto e = copyChunkFrom(sourceContainer, guidBEXT); !e)
        return unexpected(e.error());
    return {};
}

expected<void, audiofile_error> W64Container::writeFormat(const audiofile_metadata* meta)
{
    WaveFmt fmt;
    fmt.bitsPerSample  = meta->bit_depth;
    fmt.sample_rate    = meta->sample_rate;
    fmt.channels       = meta->channels;
    fmt.formatTag      = meta->codec == audiofile_codec::lpcm         ? WAVE_FORMAT_PCM
                         : meta->codec == audiofile_codec::ieee_float ? WAVE_FORMAT_IEEE_FLOAT
                                                                      : 0;
    fmt.blockAlign     = meta->bytes_per_pcm_frame();
    fmt.avgBytesPerSec = fmt.sample_rate * fmt.blockAlign;
    container          = meta->container;
    if (container == audiofile_container::unknown)
        container = audiofile_container::w64;

    if (auto e = writeChunkFrom(guidFMT, fmt); !e)
        return unexpected(e.error());

    flush();
    return {};
}
expected<void, audiofile_error> W64Container::writeAudio(const audio_data& data,
                                                         const audio_quantinization& quantinization)
{
    if (!m_currentChunkToWrite)
        if (auto e = writeChunkStart(guidDATA); !e)
            return unexpected(e.error());
    return Super::writePCMAudio(data, quantinization);
}

expected<void, audiofile_error> W64Container::finalize()
{
    if (m_currentChunkToWrite)
        if (auto e = writeChunkFinish(); !e)
            return e;

    m_header.riff     = guidRIFF;
    m_header.riffSize = m_fileSize;
    m_header.wave     = guidWAVE;
    finalized         = true;
    return writeHeader();
}

struct W64Encoder : public RIFFEncoder<W64Container>
{
    using RIFFEncoder<W64Container>::RIFFEncoder;
};

std::unique_ptr<audio_decoder> create_w64_decoder(const w64_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new W64Decoder(options));
}
std::unique_ptr<audio_encoder> create_w64_encoder(const w64_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new W64Encoder(options));
}

} // namespace kfr
