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
#include "riff.hpp"
#include "ieee.hpp"

namespace kfr
{
#pragma pack(push, 1)

struct AIFFHeader
{
    FourCC form{ "FORM" }; // "caff"
    uint32_t size;
    FourCC aiff{ "AIFF" }; //
};

struct AIFFFVER
{
    uint32_t fmtDate;
};

inline void fixByteOrder(AIFFFVER& val) { details::convert_endianness(val.fmtDate); }

struct AIFFCOMM
{
    uint16_t channels;
    uint32_t numSampleFrames;
    uint16_t sampleSize;
    float80_t sample_rate;
};

struct AIFFCOMM2 : public AIFFCOMM
{
    FourCC compressionID;
    uint8_t compressionName[2];
};

struct AIFFSSND
{
    uint32_t offset;
    uint32_t blockSize;
};

#pragma pack(pop)

static_assert(sizeof(AIFFCOMM) == 18);
static_assert(sizeof(AIFFCOMM2) == 24);

inline void fixByteOrder(AIFFCOMM& val)
{
    details::convert_endianness(val.channels);
    details::convert_endianness(val.numSampleFrames);
    details::convert_endianness(val.sampleSize);
    details::convert_endianness(val.sample_rate);
}

inline void fixByteOrder(AIFFCOMM2& val) { fixByteOrder(static_cast<AIFFCOMM&>(val)); }

inline void fixByteOrder(AIFFSSND& val)
{
    details::convert_endianness(val.offset);
    details::convert_endianness(val.blockSize);
}

struct AIFFTraits
{
    using MainHeader                                     = AIFFHeader;
    using IDType                                         = FourCC;
    using SizeType                                       = uint32_t;
    constexpr static int chunkAlignment                  = 2;
    constexpr static bool chunkSizeIsAligned             = false;
    constexpr static IDType audioChunk                   = "SSND";
    constexpr static std::optional<IDType> metadataChunk = std::nullopt; //"info";
    using MetadataParser                                 = void;
    using EncodingOptions                                = aiff_encoding_options;
    using DecodingOptions                                = aiff_decoding_options;

    constexpr static std::array<IDType, 0> initialChunksToCopy{};
    constexpr static std::array<IDType, 5> finalChunksToCopy{ "NAME", "AUTH", "(c) ", "ANNO", "COMT" };

#pragma pack(push, 1)
    struct ChunkType
    {
        IDType id;
        SizeType size;
    };
#pragma pack(pop)
};

inline void fixByteOrder(AIFFTraits::ChunkType& val) { details::convert_endianness(val.size); }

inline void fixByteOrder(AIFFHeader& val) { details::convert_endianness(val.size); }

struct AIFFDecoder : public RIFFDecoder<AIFFDecoder, AIFFTraits>
{
public:
    using RIFFDecoder<AIFFDecoder, AIFFTraits>::RIFFDecoder;

    expected<uint64_t, audiofile_error> chunkGetSize(const ChunkType& chunk, uint64_t position)
    {
        return chunk.size;
    }

    expected<audiofile_format, audiofile_error> readFormat()
    {
        if (!this->findChunk("SSND"))
            return unexpected(audiofile_error::format_error);
        AIFFCOMM2 comm;
        comm.compressionID = "NONE";
        if (m_header.aiff == "AIFC")
        {
            if (!this->readChunkTo("COMM", comm))
                return unexpected(audiofile_error::format_error);
            aifc = true;
        }
        else
        {
            if (!this->readChunkTo("COMM", static_cast<AIFFCOMM&>(comm)))
                return unexpected(audiofile_error::format_error);
        }

        audiofile_format audioInfo;
        audioInfo.channels     = comm.channels;
        audioInfo.sample_rate  = comm.sample_rate;
        audioInfo.codec        = audiofile_codec::lpcm;
        audioInfo.container    = audiofile_container::aiff;
        audioInfo.total_frames = comm.numSampleFrames;
        audioInfo.bit_depth    = comm.sampleSize;
        if (comm.compressionID == "NONE" || comm.compressionID == "twos" || comm.compressionID == "lpcm")
        {
            audioInfo.endianness = audiofile_endianness::big;
        }
        else if (comm.compressionID == "sowt")
        {
            audioInfo.endianness = audiofile_endianness::little;
        }
        else if (comm.compressionID == "fl32" || comm.compressionID == "FL32")
        {
            audioInfo.endianness = audiofile_endianness::big;
            audioInfo.codec      = audiofile_codec::ieee_float;
            audioInfo.bit_depth  = 32;
        }
        else if (comm.compressionID == "fl64" || comm.compressionID == "FL64")
        {
            audioInfo.endianness = audiofile_endianness::big;
            audioInfo.codec      = audiofile_codec::ieee_float;
            audioInfo.bit_depth  = 64;
        }
        else
        {
            return unexpected(audiofile_error::format_error);
        }

        if (!this->readChunkTo("SSND", ssnd))
            return unexpected(audiofile_error::format_error);
        m_format = audioInfo;
        return *m_format;
    }
    expected<size_t, audiofile_error> readTo(const audio_data_interleaved& data)
    {
        if (!m_currentChunkToRead)
            if (auto e = readChunkStart("SSND", sizeof(AIFFSSND) + ssnd.offset); !e)
                return unexpected(e.error());
        return this->readPCMAudio(data);
    }
    expected<void, audiofile_error> seekTo(uint64_t position)
    {
        if (!m_currentChunkToRead)
            if (auto e = readChunkStart("SSND", sizeof(AIFFSSND) + ssnd.offset); !e)
                return unexpected(e.error());
        return readChunkSeek(sizeof(AIFFSSND) + ssnd.offset + position * m_format->bytes_per_pcm_frame());
    }

private:
    bool aifc = false;
    AIFFSSND ssnd;
};

struct AIFFEncoder : public RIFFEncoder<AIFFEncoder, AIFFTraits>
{
    using RIFFEncoder<AIFFEncoder, AIFFTraits>::RIFFEncoder;

    expected<void, audiofile_error> chunkSetSize(ChunkType& chunk, uint64_t position, uint64_t byteSize)
    {
        chunk.size = byteSize;
        return {};
    }

    expected<void, audiofile_error> writeAudio(const audio_data_interleaved& data,
                                               const audio_quantization& quantization)
    {
        if (!m_currentChunkToWrite)
        {
            if (auto e = writeChunkStart("SSND"); !e)
                return e;
            AIFFSSND ssnd;
            ssnd.offset    = 0;
            ssnd.blockSize = 0;
            fixByteOrder(ssnd);
            if (auto e = writeChunkContinue(&ssnd, sizeof(ssnd)); !e)
                return e;
        }
        return this->writePCMAudio(data, quantization);
    }

    expected<void, audiofile_error> finalize()
    {
        if (m_currentChunkToWrite)
            if (auto e = writeChunkFinish(); !e)
                return e;

        m_header.form = "FORM";
        m_header.size = m_fileSize - 8;
        m_header.aiff = aifc ? "AIFC" : "AIFF";

        return writeHeader();
    }

    expected<void, audiofile_error> writeFormat()
    {
        KFR_ASSERT(m_format.has_value());
        AIFFCOMM2 comm{};

        comm.channels        = m_format->channels;
        comm.numSampleFrames = m_format->total_frames;
        comm.sample_rate     = m_format->sample_rate;
        comm.sampleSize      = m_format->bit_depth;
        if (m_format->codec == audiofile_codec::ieee_float ||
            m_format->endianness == audiofile_endianness::little)
        {
            aifc = true;
        }
        if (m_format->codec == audiofile_codec::ieee_float)
        {
            if (m_format->bit_depth == 32)
            {
                comm.compressionID = "fl32";
            }
            else if (m_format->bit_depth == 64)
            {
                comm.compressionID = "fl64";
            }
            else
                return unexpected(audiofile_error::format_error);
        }
        else if (m_format->codec == audiofile_codec::lpcm)
        {
            if (m_format->endianness == audiofile_endianness::little)
                comm.compressionID = "sowt";
            else
                comm.compressionID = "NONE";
        }
        else
            return unexpected(audiofile_error::format_error);

        if (aifc)
        {
            AIFFFVER fver;
            fver.fmtDate = 0xA2805140u;
            if (auto e = writeChunkFrom("FVER", fver); !e)
                return unexpected(e.error());
            if (auto e = writeChunkFrom("COMM", comm); !e)
                return unexpected(e.error());
        }
        else
        {
            if (auto e = writeChunkFrom("COMM", static_cast<AIFFCOMM&>(comm)); !e)
                return unexpected(e.error());
        }
        return {};
    }

private:
    bool aifc = false;
};

std::unique_ptr<audio_decoder> create_aiff_decoder(const aiff_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new AIFFDecoder(options));
}
std::unique_ptr<audio_encoder> create_aiff_encoder(const aiff_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new AIFFEncoder(options));
}

} // namespace kfr
