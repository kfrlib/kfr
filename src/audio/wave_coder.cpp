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

struct WAVEHeader
{
    FourCC riff{ "RIFF" };
    uint32_t riffSize{ 0 };
    FourCC wave{ "WAVE" };
};

struct WAVEDS64
{
    uint64_t bw64Size;
    uint64_t dataSize;
    uint64_t dummy;
    uint32_t tableLength;
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

struct WAVETraits
{
    using MainHeader                                     = WAVEHeader;
    using IDType                                         = FourCC;
    using SizeType                                       = uint32_t;
    constexpr static int chunkAlignment                  = 2;
    constexpr static bool chunkSizeIsAligned             = false;
    constexpr static std::optional<IDType> metadataChunk = "LIST";
    constexpr static IDType audioChunk                   = "data";
    using MetadataParser                                 = RIFFMetadataParser;
    using EncodingOptions                                = wave_encoding_options;
    using DecodingOptions                                = wave_decoding_options;

    constexpr static std::array<IDType, 3> initialChunksToCopy{ "chna", "axml", "bext" };
    constexpr static std::array<IDType, 0> finalChunksToCopy{};

#pragma pack(push, 1)
    struct ChunkType
    {
        IDType id;
        SizeType size;
    };
#pragma pack(pop)
};

enum : uint16_t
{
    WAVE_FORMAT_PCM        = 0x1,
    WAVE_FORMAT_IEEE_FLOAT = 0x3,
    WAVE_FORMAT_EXTENSIBLE = 0xFFFE,
};

struct WaveDecoder : public RIFFDecoder<WaveDecoder, WAVETraits>
{
    using RIFFDecoder<WaveDecoder, WAVETraits>::RIFFDecoder;

    [[nodiscard]] expected<uint64_t, audiofile_error> chunkGetSize(const ChunkType& chunk, uint64_t position)
    {
        if (chunk.size == SizeType(-1))
        {
            if (chunk.id != "data")
                return unexpected(audiofile_error::format_error);
            if (!this->findChunk("ds64"))
                return unexpected(audiofile_error::format_error);
            WAVEDS64 ds64;
            if (auto e = this->readChunkTo("ds64", ds64); !e)
                return unexpected(e.error());
            return ds64.dataSize;
        }
        return chunk.size;
    }

    expected<audiofile_format, audiofile_error> readFormat()
    {
        audiofile_format meta;
        if (m_header.riff == "RIFF" && m_header.wave == "WAVE")
        {
            meta.container = audiofile_container::wave;
        }
        else if (m_header.riff == "RF64" && m_header.wave == "WAVE")
        {
            meta.container = audiofile_container::rf64;
        }
        else if (m_header.riff == "BW64" && m_header.wave == "WAVE")
        {
            meta.container = audiofile_container::bw64;
        }
        else
        {
            return unexpected(audiofile_error::format_error);
        }

        WaveFmtEx fmt;
        fmt.extendedSize = 0;
        if (!this->findChunk("data"))
            return unexpected(audiofile_error::format_error);
        if (!this->readChunkTo("fmt ", fmt) && !this->readChunkTo("fmt ", fmt.fmt))
            return unexpected(audiofile_error::format_error);

        if (fmt.extendedSize == 0 && fmt.fmt.formatTag == WAVE_FORMAT_EXTENSIBLE)
            return unexpected(audiofile_error::format_error);
        uint16_t formatTag =
            fmt.fmt.formatTag == WAVE_FORMAT_EXTENSIBLE ? fmt.formatTagEx : fmt.fmt.formatTag;

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
        meta.total_frames = m_chunks[*this->findChunk("data")].byteSize / meta.bytes_per_pcm_frame();
        if (!meta.valid())
        {
            return unexpected(audiofile_error::format_error);
        }

        return meta;
    }

    expected<size_t, audiofile_error> readTo(const audio_data_interleaved& data)
    {
        if (!m_currentChunkToRead)
            if (auto e = readChunkStart("data"); !e)
                return unexpected(e.error());
        return this->readPCMAudio(data);
    }

    expected<void, audiofile_error> seekTo(uint64_t position)
    {
        if (!m_currentChunkToRead)
            if (auto e = readChunkStart("data"); !e)
                return unexpected(e.error());
        return readChunkSeek(position * m_format->bytes_per_pcm_frame());
    }
};

struct WaveEncoder : public RIFFEncoder<WaveEncoder, WAVETraits>
{
    using RIFFEncoder<WaveEncoder, WAVETraits>::RIFFEncoder;

    expected<void, audiofile_error> chunkSetSize(ChunkType& chunk, uint64_t position, uint64_t byteSize)
    {
        chunk.size = byteSize > UINT32_MAX ? UINT32_MAX : byteSize;
        return {};
    }

    expected<void, audiofile_error> writeFormat()
    {
        WaveFmt fmt;
        fmt.bitsPerSample  = m_format->bit_depth;
        fmt.sample_rate    = m_format->sample_rate;
        fmt.channels       = m_format->channels;
        fmt.formatTag      = m_format->codec == audiofile_codec::lpcm         ? WAVE_FORMAT_PCM
                             : m_format->codec == audiofile_codec::ieee_float ? WAVE_FORMAT_IEEE_FLOAT
                                                                              : 0;
        fmt.blockAlign     = m_format->bytes_per_pcm_frame();
        fmt.avgBytesPerSec = fmt.sample_rate * fmt.blockAlign;

        if (m_format->container == audiofile_container::unknown)
            m_format->container = audiofile_container::wave;

        if (m_format->container != audiofile_container::wave || m_options.switch_to_rf64_if_over_4gb)
        {
            WAVEDS64 ds64{};
            if (auto e = writeChunkFrom("JUNK", ds64); !e)
                return unexpected(e.error());
        }

        if (auto e = writeChunkFrom("fmt ", fmt); !e)
            return unexpected(e.error());

        flush();
        return {};
    }

    expected<void, audiofile_error> writeAudio(const audio_data_interleaved& data,
                                               const audio_quantization& quantization)
    {
        if (!m_currentChunkToWrite)
            if (auto e = writeChunkStart("data"); !e)
                return unexpected(e.error());
        return this->writePCMAudio(data, quantization);
    }

    expected<void, audiofile_error> finalize()
    {
        if (m_currentChunkToWrite)
            if (auto e = writeChunkFinish(); !e)
                return e;

        if (m_fileSize >= 0x1'00000000ull || m_format->container != audiofile_container::wave)
        {
            // >= 4GB
            if (m_format->container == audiofile_container::wave && !m_options.switch_to_rf64_if_over_4gb)
                return unexpected(audiofile_error::too_large);

            auto data_idx = findChunk("data");
            if (!data_idx)
                return unexpected(audiofile_error::format_error);

            // rewrite JUNK with ds64
            WAVEDS64 ds64{};
            ds64.bw64Size    = m_fileSize;
            ds64.dataSize    = m_chunks[*data_idx].byteSize;
            ds64.dummy       = 0;
            ds64.tableLength = 0;
            if (auto e = writeChunkFrom("JUNK", ds64, true, "ds64"); !e)
                return unexpected(e.error());

            // rewrite header
            if ((m_format->container == audiofile_container::wave && m_options.switch_to_rf64_if_over_4gb) ||
                m_format->container == audiofile_container::rf64)
                m_header.riff = "RF64";
            else if (m_format->container == audiofile_container::bw64)
                m_header.riff = "BW64";
            m_header.riffSize = UINT32_MAX;
            m_header.wave     = "WAVE";
        }
        else
        {
            // < 4GB, regular WAV file
            m_header.riff     = "RIFF";
            m_header.riffSize = m_fileSize - 8;
            m_header.wave     = "WAVE";
        }
        return writeHeader();
    }
};

std::unique_ptr<audio_decoder> create_wave_decoder(const wave_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new WaveDecoder(options));
}
std::unique_ptr<audio_encoder> create_wave_encoder(const wave_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new WaveEncoder(options));
}

} // namespace kfr
