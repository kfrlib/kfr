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

inline void fixByteOrder(AIFFFVER& val) { convertEndianess(val.fmtDate); }

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
    convertEndianess(val.channels);
    convertEndianess(val.numSampleFrames);
    convertEndianess(val.sampleSize);
    convertEndianess(val.sample_rate);
}

inline void fixByteOrder(AIFFCOMM2& val) { fixByteOrder(static_cast<AIFFCOMM&>(val)); }

inline void fixByteOrder(AIFFSSND& val)
{
    convertEndianess(val.offset);
    convertEndianess(val.blockSize);
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

#pragma pack(push, 1)
    struct ChunkType
    {
        IDType id;
        SizeType size;
    };
#pragma pack(pop)

    [[nodiscard]] static expected<uint64_t, audiofile_error> chunkGetSize(const ChunkType& chunk,
                                                                          uint64_t position,
                                                                          RIFFContainer<AIFFTraits>*)
    {
        return chunk.size;
    }
    [[nodiscard]] static expected<void, audiofile_error> chunkSetSize(ChunkType& chunk, uint64_t position,
                                                                      uint64_t byteSize,
                                                                      RIFFContainer<AIFFTraits>*)
    {
        chunk.size = byteSize;
        return {};
    }
};

inline void fixByteOrder(AIFFTraits::ChunkType& val) { convertEndianess(val.size); }

inline void fixByteOrder(AIFFHeader& val) { convertEndianess(val.size); }

class AIFFContainer : public RIFFContainer<AIFFTraits>
{
public:
    using Super = RIFFContainer<AIFFTraits>;

    [[nodiscard]] expected<audiofile_metadata, audiofile_error> readFormat();

    [[nodiscard]] expected<audio_data, audiofile_error> readAudio(const audiofile_metadata* meta);

    [[nodiscard]] expected<void, audiofile_error> seek(const audiofile_metadata* meta, uint64_t position);

    [[nodiscard]] expected<void, audiofile_error> writeFormat(const audiofile_metadata* meta);

    [[nodiscard]] expected<void, audiofile_error> writeAudio(const audio_data& data,
                                                             const audio_quantinization& quantinization);

    [[nodiscard]] expected<void, audiofile_error> finalize();

    [[nodiscard]] expected<void, audiofile_error> copyFinalChunksFrom(RIFFContainer* sourceContainer);

    ~AIFFContainer() {}
    AIFFSSND ssnd;
    bool aifc      = false;
    bool finalized = false;
};

struct AIFFDecoder : public RIFFDecoder<AIFFContainer>
{
public:
    using RIFFDecoder<AIFFContainer>::RIFFDecoder;
};

struct AIFFEncoder : public RIFFEncoder<AIFFContainer>
{
    using RIFFEncoder<AIFFContainer>::RIFFEncoder;
};

std::unique_ptr<audio_decoder> create_aiff_decoder(const aiff_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new AIFFDecoder(options));
}
std::unique_ptr<audio_encoder> create_aiff_encoder(const aiff_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new AIFFEncoder(options));
}

[[nodiscard]] expected<audiofile_metadata, audiofile_error> AIFFContainer::readFormat()
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

    audiofile_metadata audioInfo;
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
    return audioInfo;
}

[[nodiscard]] expected<audio_data, audiofile_error> AIFFContainer::readAudio(const audiofile_metadata* meta)
{
    if (!m_currentChunkToRead)
        if (auto e = readChunkStart("SSND", sizeof(AIFFSSND) + ssnd.offset); !e)
            return unexpected(e.error());
    return Super::readPCMAudio(default_audio_frames_to_read, meta);
}

[[nodiscard]] expected<void, audiofile_error> AIFFContainer::seek(const audiofile_metadata* meta,
                                                                  uint64_t position)
{
    if (!m_currentChunkToRead)
        if (auto e = readChunkStart("SSND", sizeof(AIFFSSND) + ssnd.offset); !e)
            return unexpected(e.error());
    return readChunkSeek(sizeof(AIFFSSND) + ssnd.offset + position * meta->bytes_per_pcm_frame());
}

[[nodiscard]] expected<void, audiofile_error> AIFFContainer::writeFormat(const audiofile_metadata* meta)
{
    AIFFCOMM2 comm{};

    comm.channels        = meta->channels;
    comm.numSampleFrames = meta->total_frames;
    comm.sample_rate     = meta->sample_rate;
    comm.sampleSize      = meta->bit_depth;
    if (meta->codec == audiofile_codec::ieee_float || meta->endianness == audiofile_endianness::little)
    {
        aifc = true;
    }
    if (meta->codec == audiofile_codec::ieee_float)
    {
        if (meta->bit_depth == 32)
        {
            comm.compressionID = "fl32";
        }
        else if (meta->bit_depth == 64)
        {
            comm.compressionID = "fl64";
        }
        else
            return unexpected(audiofile_error::format_error);
    }
    else if (meta->codec == audiofile_codec::lpcm)
    {
        if (meta->endianness == audiofile_endianness::little)
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
    flush();
    return {};
}

[[nodiscard]] expected<void, audiofile_error> AIFFContainer::writeAudio(
    const audio_data& data, const audio_quantinization& quantinization)
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
    return Super::writePCMAudio(data, quantinization);
}

[[nodiscard]] expected<void, audiofile_error> AIFFContainer::finalize()
{
    if (m_currentChunkToWrite)
        if (auto e = writeChunkFinish(); !e)
            return e;

    m_header.form = "FORM";
    m_header.size = m_fileSize - 8;
    m_header.aiff = aifc ? "AIFC" : "AIFF";

    finalized = true;
    return writeHeader();
}

expected<void, audiofile_error> AIFFContainer::copyFinalChunksFrom(RIFFContainer* sourceContainer)
{
    if (auto e = copyChunkFrom(sourceContainer, "NAME"); !e)
        return unexpected(e.error());
    if (auto e = copyChunkFrom(sourceContainer, "AUTH"); !e)
        return unexpected(e.error());
    if (auto e = copyChunkFrom(sourceContainer, "(c) "); !e)
        return unexpected(e.error());
    if (auto e = copyChunkFrom(sourceContainer, "ANNO"); !e)
        return unexpected(e.error());
    return {};
}

} // namespace kfr
