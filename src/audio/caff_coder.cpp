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

#ifdef KFR_AUDIO_ALAC

#include <alac/ALACBitUtilities.h>
#include <alac/ALACDecoder.h>
#include <alac/ALACEncoder.h>

#endif

#define kMaxBERSize 5

namespace kfr
{

#pragma pack(push, 1)

struct CAFFHeader
{
    FourCC caff{ "caff" }; // "caff"
    uint16_t caffVersion{ 1 };
    uint16_t fileVersion{ 0 };
};

#pragma pack(pop)

struct CAFFTraits
{
    using MainHeader                                     = CAFFHeader;
    using IDType                                         = FourCC;
    using SizeType                                       = uint64_t;
    constexpr static int chunkAlignment                  = 1;
    constexpr static bool chunkSizeIsAligned             = true;
    constexpr static std::optional<IDType> metadataChunk = std::nullopt; //"info";
    constexpr static IDType audioChunk                   = "data";
    using MetadataParser                                 = void;
    using EncodingOptions                                = caff_encoding_options;
    using DecodingOptions                                = caff_decoding_options;

#pragma pack(push, 1)
    struct ChunkType
    {
        IDType id;
        SizeType size;
    };
#pragma pack(pop)

    [[nodiscard]] static expected<uint64_t, audiofile_error> chunkGetSize(const ChunkType& chunk,
                                                                          uint64_t position,
                                                                          RIFFContainer<CAFFTraits>* self)
    {
        if (chunk.size == uint64_t(-1))
        {
            return self->fileSize() - position - sizeof(ChunkType);
        }
        return chunk.size;
    }
    [[nodiscard]] static expected<void, audiofile_error> chunkSetSize(ChunkType& chunk, uint64_t position,
                                                                      uint64_t byteSize,
                                                                      RIFFContainer<CAFFTraits>*)
    {
        chunk.size = byteSize;
        return {};
    }
};

inline void fixByteOrder(CAFFTraits::ChunkType& val) { convertEndianess(val.size); }

enum
{
    kCAFLinearPCMFormatFlagIsFloat        = 1,
    kCAFLinearPCMFormatFlagIsLittleEndian = 2,
};

inline void fixByteOrder(CAFFHeader& val)
{
    convertEndianess(val.caffVersion);
    convertEndianess(val.fileVersion);
}

#pragma pack(push, 1)

struct CAFFDesc
{
    double mSampleRate;
    FourCC mFormatID;
    uint32_t mFormatFlags;
    uint32_t mBytesPerPacket;
    uint32_t mFramesPerPacket;
    uint32_t mChannelsPerFrame;
    uint32_t mBitsPerChannel;
};

#pragma pack(pop)

inline void fixByteOrder(CAFFDesc& val)
{
    convertEndianess(val.mSampleRate);
    convertEndianess(val.mFormatFlags);
    convertEndianess(val.mBytesPerPacket);
    convertEndianess(val.mFramesPerPacket);
    convertEndianess(val.mChannelsPerFrame);
    convertEndianess(val.mBitsPerChannel);
}

#ifdef KFR_AUDIO_ALAC
using CAFFChan = ALACAudioChannelLayout;

inline void fixByteOrder(CAFFChan& val)
{
    convertEndianess(val.mChannelLayoutTag);
    convertEndianess(val.mChannelBitmap);
    convertEndianess(val.mNumberChannelDescriptions);
}

#pragma pack(push, 1)
struct CAFFPakt
{
    int64_t mNumberPackets;
    int64_t mNumberValidFrames;
    int32_t mPrimingFrames;
    int32_t mRemainderFrames;
};
#pragma pack(pop)

inline void fixByteOrder(CAFFPakt& val)
{
    convertEndianess(val.mNumberPackets);
    convertEndianess(val.mNumberValidFrames);
    convertEndianess(val.mPrimingFrames);
    convertEndianess(val.mRemainderFrames);
}
#endif

#pragma pack(push, 1)
struct CAFFDataPrefix
{
    uint32_t editCount;
};
#pragma pack(pop)

class CAFFContainer : public RIFFContainer<CAFFTraits>
{
public:
    using Super = RIFFContainer<CAFFTraits>;

    [[nodiscard]] expected<audiofile_metadata, audiofile_error> readFormat();

    [[nodiscard]] expected<audio_data, audiofile_error> readAudio(const audiofile_metadata* audioInfo);

    [[nodiscard]] expected<void, audiofile_error> seek(const audiofile_metadata* audioInfo,
                                                       uint64_t position);

    [[nodiscard]] expected<void, audiofile_error> writeFormat(const audiofile_metadata* info);

    [[nodiscard]] expected<void, audiofile_error> writeAudio(const audio_data& data,
                                                             const audio_quantinization& quantinization);

#ifdef KFR_AUDIO_ALAC
    [[nodiscard]] expected<void, audiofile_error> writeBuffered();
    void writePacketSize(int32_t pktSize);
    [[nodiscard]] expected<void, audiofile_error> writeAudioPacket(size_t packetFrames);
#endif
    [[nodiscard]] expected<void, audiofile_error> finalize();

    ~CAFFContainer() {}

#ifdef KFR_AUDIO_ALAC
    std::unique_ptr<ALACDecoder> theDecoder;
    std::vector<uint8_t> theReadBuffer;
    std::vector<uint8_t> theWriteBuffer;
    BitBuffer theInputBuffer;
    std::vector<uint64_t> packetOffsets; // offset of past-the-end byte for each packet
    size_t packetIndex              = 0;
    size_t framesToSkipInNextPacket = 0; // used for seek
#endif
    CAFFDesc desc;
#ifdef KFR_AUDIO_ALAC
    AudioFormatDescription afdesc, inafdesc;
    std::unique_ptr<ALACEncoder> theEncoder;
#endif
    bool finalized = false;
    std::vector<uint8_t> buffered;
    uint64_t framesWritten  = 0;
    uint64_t packetsWritten = 0;
};

struct CAFFDecoder : public RIFFDecoder<CAFFContainer>
{
public:
    using RIFFDecoder<CAFFContainer>::RIFFDecoder;
};

struct CAFFEncoder : public RIFFEncoder<CAFFContainer>
{
    using RIFFEncoder<CAFFContainer>::RIFFEncoder;
};

std::unique_ptr<audio_decoder> create_caff_decoder(const caff_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new CAFFDecoder(options));
}
std::unique_ptr<audio_encoder> create_caff_encoder(const caff_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new CAFFEncoder(options));
}

#ifdef KFR_AUDIO_ALAC
static uint32_t ReadBERInteger(uint8_t* theInputBuffer, int32_t* ioNumBytes)
{
    uint32_t theAnswer = 0;
    uint8_t theData;
    int32_t size = 0;
    do
    {
        theData   = theInputBuffer[size];
        theAnswer = (theAnswer << 7) | (theData & 0x7F);
        if (++size > 5)
        {
            size = 0xFFFFFFFF;
            return 0;
        }
    } while (((theData & 0x80) != 0) && (size <= *ioNumBytes));

    *ioNumBytes = size;

    return theAnswer;
}

static void GetBERInteger(int32_t theOriginalValue, uint8_t* theBuffer, int32_t* theBERSize)
{
    if ((theOriginalValue & 0x7f) == theOriginalValue)
    {
        *theBERSize  = 1;
        theBuffer[0] = theOriginalValue;
    }
    else if ((theOriginalValue & 0x3fff) == theOriginalValue)
    {
        *theBERSize  = 2;
        theBuffer[0] = theOriginalValue >> 7;
        theBuffer[0] |= 0x80;
        theBuffer[1] = theOriginalValue & 0x7f;
    }
    else if ((theOriginalValue & 0x1fffff) == theOriginalValue)
    {
        *theBERSize  = 3;
        theBuffer[0] = theOriginalValue >> 14;
        theBuffer[0] |= 0x80;
        theBuffer[1] = (theOriginalValue >> 7) & 0x7f;
        theBuffer[1] |= 0x80;
        theBuffer[2] = theOriginalValue & 0x7f;
    }
    else if ((theOriginalValue & 0x0fffffff) == theOriginalValue)
    {
        *theBERSize  = 4;
        theBuffer[0] = theOriginalValue >> 21;
        theBuffer[0] |= 0x80;
        theBuffer[1] = (theOriginalValue >> 14) & 0x7f;
        theBuffer[1] |= 0x80;
        theBuffer[2] = (theOriginalValue >> 7) & 0x7f;
        theBuffer[2] |= 0x80;
        theBuffer[3] = theOriginalValue & 0x7f;
    }
    else
    {
        *theBERSize  = 5;
        theBuffer[0] = theOriginalValue >> 28;
        theBuffer[0] |= 0x80;
        theBuffer[1] = (theOriginalValue >> 21) & 0x7f;
        theBuffer[1] |= 0x80;
        theBuffer[2] = (theOriginalValue >> 14) & 0x7f;
        theBuffer[2] |= 0x80;
        theBuffer[3] = (theOriginalValue >> 7) & 0x7f;
        theBuffer[3] |= 0x80;
        theBuffer[4] = theOriginalValue & 0x7f;
    }
}
#endif

expected<audiofile_metadata, audiofile_error> CAFFContainer::readFormat()
{
    if (!this->findChunk("data"))
        return unexpected(audiofile_error::format_error);

    if (auto e = this->readChunkTo("desc", desc); !e)
        return unexpected(e.error());

    audiofile_metadata audioInfo;
    audioInfo.channels    = desc.mChannelsPerFrame;
    audioInfo.bit_depth   = desc.mBitsPerChannel;
    audioInfo.container   = audiofile_container::caf;
    audioInfo.sample_rate = desc.mSampleRate;

    if (desc.mFormatID == "lpcm")
    {
        audioInfo.endianness = (desc.mFormatFlags & kCAFLinearPCMFormatFlagIsLittleEndian)
                                   ? audiofile_endianness::little
                                   : audiofile_endianness::big;
        audioInfo.codec = (desc.mFormatFlags & kCAFLinearPCMFormatFlagIsFloat) ? audiofile_codec::ieee_float
                                                                               : audiofile_codec::lpcm;
        audioInfo.total_frames = (m_chunks[*findChunk("data")].byteSize - sizeof(CAFFDataPrefix)) /
                                 audioInfo.bytes_per_pcm_frame();
    }
#ifdef KFR_AUDIO_ALAC
    else if (desc.mFormatID == "alac")
    {
        audioInfo.codec = audiofile_codec::alac;
        auto kuki       = readChunkBytes("kuki");
        if (!kuki)
            return unexpected(kuki.error());
        theDecoder.reset(new ALACDecoder());
        if (theDecoder->Init(kuki->data(), kuki->size()) != ALAC_noErr)
            return unexpected(audiofile_error::format_error);

        theReadBuffer.resize(desc.mChannelsPerFrame * sizeof(int32_t) * desc.mFramesPerPacket +
                             kALACMaxEscapeHeaderBytes);
        theWriteBuffer.resize(theReadBuffer.size() - kALACMaxEscapeHeaderBytes);
        BitBufferInit(&theInputBuffer, theReadBuffer.data(), theReadBuffer.size());

        CAFFPakt pakt;
        if (auto e = readChunkTo("pakt", pakt); !e)
            return unexpected(e.error());

        std::vector<uint8_t> packetTable = *readChunkBytes("pakt");
        packetTable.erase(packetTable.begin(), packetTable.begin() + sizeof(CAFFPakt));

        // uncompress packet table:
        size_t byteOffset     = 0;
        uint64_t packetOffset = 0;
        for (;;)
        {
            int32_t numBytes            = kMaxBERSize;
            int32_t theInputPacketBytes = ReadBERInteger(packetTable.data() + byteOffset, &numBytes);
            if (theInputPacketBytes == 0)
                break;
            packetOffset += theInputPacketBytes;
            byteOffset += numBytes;
            packetOffsets.push_back(packetOffset);
        }

        audioInfo.total_frames = pakt.mNumberValidFrames;
        // fmt::print(stderr, fg(fmt::color::light_pink), "totalSamples = {}\n", audioInfo.totalSamples);
        audioInfo.bit_depth = theDecoder->mConfig.bitDepth;
    }
#endif
    else
    {
        return unexpected(audiofile_error::format_error);
    }

    return audioInfo;
}

expected<audio_data, audiofile_error> CAFFContainer::readAudio(const audiofile_metadata* metadata)
{
    if (!m_currentChunkToRead)
        if (auto e = readChunkStart("data", sizeof(CAFFDataPrefix)); !e)
            return unexpected(e.error());
    if (metadata->codec == audiofile_codec::ieee_float || metadata->codec == audiofile_codec::lpcm)
        return Super::readPCMAudio(default_audio_frames_to_read, metadata);

#ifdef KFR_AUDIO_ALAC
    BitBufferReset(&theInputBuffer);

    if (packetIndex >= packetOffsets.size())
        return unexpected(audiofile_error::end_of_file);

    int32_t theInputPacketBytes =
        packetIndex == 0 ? packetOffsets[0] : packetOffsets[packetIndex] - packetOffsets[packetIndex - 1];
    ++packetIndex;

    if (auto e = readChunkContinue(theInputPacketBytes, theReadBuffer.data()); !e)
        return unexpected(e.error());
    else if (*e != theInputPacketBytes)
        return unexpected(audiofile_error::io_error);
    uint32_t framesToRead = 0;

    if (theDecoder->Decode(&theInputBuffer, theWriteBuffer.data(), desc.mFramesPerPacket,
                           desc.mChannelsPerFrame, &framesToRead) != ALAC_noErr)
    {
        return unexpected(audiofile_error::format_error);
    }

    audio_data result;
    result.metadata = metadata;

    result.resize(framesToRead);
    if (!forPCMCodec(
            [&]<typename T>(ctype_t<T>)
            {
                T* audio = reinterpret_cast<T*>(theWriteBuffer.data());
                deinterleave_samples(result.pointers(), audio, metadata->channels, framesToRead);
            },
            audiofile_codec::lpcm, metadata->bit_depth))
        return unexpected(audiofile_error::format_error);

    if (framesToSkipInNextPacket > 0)
    {
        result                   = result.slice(framesToSkipInNextPacket);
        framesToSkipInNextPacket = 0;
    }
    return result;
#else
    return unexpected(audiofile_error::format_error);
#endif
}

[[nodiscard]] expected<void, audiofile_error> CAFFContainer::seek(const audiofile_metadata* audioInfo,
                                                                  uint64_t position)
{
    if (!m_currentChunkToRead)
        if (auto e = readChunkStart("data", sizeof(CAFFDataPrefix)); !e)
            return unexpected(e.error());
    if (audioInfo->codec == audiofile_codec::ieee_float || audioInfo->codec == audiofile_codec::lpcm)
        return Super::readChunkSeek(sizeof(CAFFDataPrefix) + position * audioInfo->bytes_per_pcm_frame());

#ifdef KFR_AUDIO_ALAC
    packetIndex                  = position / desc.mFramesPerPacket;
    framesToSkipInNextPacket     = position % desc.mFramesPerPacket;
    uint64_t packetOffsetInBytes = packetIndex == 0 ? 0 : packetOffsets[packetIndex - 1];
    return readChunkSeek(sizeof(CAFFDataPrefix) + packetOffsetInBytes);
#else
    return unexpected(audiofile_error::format_error);
#endif
}

expected<void, audiofile_error> CAFFContainer::writeFormat(const audiofile_metadata* info)
{
    if (info->codec == audiofile_codec::lpcm)
    {
        desc.mFormatID        = "lpcm";
        desc.mFormatFlags     = kCAFLinearPCMFormatFlagIsLittleEndian;
        desc.mBitsPerChannel  = info->bit_depth;
        desc.mBytesPerPacket  = info->bytes_per_pcm_frame();
        desc.mFramesPerPacket = 1;
    }
    else if (info->codec == audiofile_codec::ieee_float)
    {
        desc.mFormatID        = "lpcm";
        desc.mFormatFlags     = kCAFLinearPCMFormatFlagIsFloat | kCAFLinearPCMFormatFlagIsLittleEndian;
        desc.mBitsPerChannel  = info->bit_depth;
        desc.mBytesPerPacket  = info->bytes_per_pcm_frame();
        desc.mFramesPerPacket = 1;
    }
#ifdef KFR_AUDIO_ALAC
    else
    {
        desc.mFormatID        = "alac";
        desc.mFormatFlags     = 0;
        desc.mBitsPerChannel  = 0;
        desc.mBytesPerPacket  = 0;
        desc.mFramesPerPacket = kALACDefaultFramesPerPacket;
    }
#else
    else
    {
        return unexpected(audiofile_error::format_error);
    }
#endif
    desc.mChannelsPerFrame = info->channels;
    desc.mSampleRate       = info->sample_rate;

#ifdef KFR_AUDIO_ALAC
    afdesc.mSampleRate       = desc.mSampleRate;
    afdesc.mFormatID         = uint32_t(desc.mFormatID);
    afdesc.mFormatFlags      = desc.mFormatFlags;
    afdesc.mBytesPerPacket   = desc.mBytesPerPacket;
    afdesc.mFramesPerPacket  = desc.mFramesPerPacket;
    afdesc.mBytesPerFrame    = info->bytes_per_pcm_frame();
    afdesc.mChannelsPerFrame = desc.mChannelsPerFrame;
    afdesc.mBitsPerChannel   = desc.mBitsPerChannel;
    afdesc.mReserved         = 0;
#endif

    if (auto e = writeHeader(); !e)
        return unexpected(e.error());

    if (auto e = writeChunkFrom("desc", desc); !e)
        return unexpected(e.error());

#ifdef KFR_AUDIO_ALAC
    if (info->codec == audiofile_codec::alac)
    {
        afdesc.mBytesPerFrame  = 0;
        afdesc.mBytesPerPacket = 0;
        switch (info->bit_depth)
        {
        case 16:
            afdesc.mFormatFlags = 1;
            break;
        case 20:
            afdesc.mFormatFlags = 2;
            break;
        case 24:
            afdesc.mFormatFlags = 3;
            break;
        case 32:
            afdesc.mFormatFlags = 4;
            break;
        default:
            return unexpected(audiofile_error::format_error);
        }

        theEncoder.reset(new ALACEncoder());
        theEncoder->SetFrameSize(desc.mFramesPerPacket);
        theEncoder->InitializeEncoder(afdesc);
        uint32_t cookieSize = theEncoder->GetMagicCookieSize(desc.mChannelsPerFrame);
        std::vector<uint8_t> cookie(cookieSize, 0);
        theEncoder->GetMagicCookie(cookie.data(), &cookieSize);
        theWriteBuffer.resize(desc.mFramesPerPacket * info->bytes_per_pcm_frame() +
                              kALACMaxEscapeHeaderBytes);

        if (auto e = writeChunkBytes("kuki", cookie); !e)
            return unexpected(e.error());
        cookie.clear();
    }

    CAFFChan chan;
    chan.mChannelLayoutTag          = ALACChannelLayoutTags[info->channels];
    chan.mChannelBitmap             = 0;
    chan.mNumberChannelDescriptions = 0;

    inafdesc = afdesc;

    inafdesc.mBytesPerFrame   = info->bytes_per_pcm_frame();
    inafdesc.mBytesPerPacket  = info->bytes_per_pcm_frame();
    inafdesc.mBitsPerChannel  = info->bit_depth;
    inafdesc.mFramesPerPacket = 1;
    inafdesc.mFormatID        = uint32_t(FourCC("lpcm"));
    inafdesc.mFormatFlags     = kCAFLinearPCMFormatFlagIsLittleEndian;

    if (auto e = writeChunkFrom("chan", chan); !e)
        return unexpected(e.error());
#endif
    flush();
    return {};
}

#ifdef KFR_AUDIO_ALAC
void CAFFContainer::writePacketSize(int32_t pktSize)
{
    if (packetIndex == 0)
        packetOffsets.push_back(pktSize);
    else
        packetOffsets.push_back(packetOffsets.back() + pktSize);
    packetIndex++;

    ++packetsWritten;
}

expected<void, audiofile_error> CAFFContainer::writeAudioPacket(size_t packetFrames)
{
    int32_t packetSize = packetFrames * inafdesc.mBytesPerFrame;

    if (theEncoder->Encode(inafdesc, afdesc, buffered.data(), theWriteBuffer.data(), &packetSize) != 0)
        return unexpected(audiofile_error::internal_error);

    writePacketSize(packetSize);
    return writeChunkContinue(theWriteBuffer.data(), packetSize);
}
#endif

expected<void, audiofile_error> CAFFContainer::writeAudio(const audio_data& audio,
                                                          const audio_quantinization& quantinization)
{
    const audiofile_metadata* metadata = audio.typed_metadata<audiofile_metadata>();
    if (!m_currentChunkToWrite)
    {
        if (auto e = writeChunkStart("data"); !e)
            return unexpected(e.error());
        CAFFDataPrefix prefix{ 0 };
        if (auto e = writeChunkContinue(&prefix, sizeof(prefix)); !e)
            return e;
    }
    framesWritten += audio.size;
    if (metadata->codec == audiofile_codec::lpcm || metadata->codec == audiofile_codec::ieee_float)
        return Super::writePCMAudio(audio, quantinization);

#ifdef KFR_AUDIO_ALAC
    size_t framesToWrite = audio.size;
    kfr::univector<uint8_t> interleaved(framesToWrite * inafdesc.mBytesPerFrame);
    if (!forPCMCodec(
            [&]<typename T>(ctype_t<T>)
            {
                interleave_samples(reinterpret_cast<T*>(interleaved.data()), audio.pointers(),
                                   audio.metadata->channels, framesToWrite, quantinization);
            },
            audiofile_codec::lpcm, metadata->bit_depth))
        return unexpected(audiofile_error::format_error);

    buffered.insert(buffered.end(), interleaved.begin(), interleaved.end());
    return writeBuffered();
#else
    return unexpected(audiofile_error::format_error);
#endif
}

#ifdef KFR_AUDIO_ALAC
expected<void, audiofile_error> CAFFContainer::writeBuffered()
{
    while (buffered.size() >= afdesc.mFramesPerPacket * inafdesc.mBytesPerFrame)
    {
        if (auto e = writeAudioPacket(afdesc.mFramesPerPacket); !e)
            return e;
        buffered.erase(buffered.begin(),
                       buffered.begin() + afdesc.mFramesPerPacket * inafdesc.mBytesPerFrame);
    }
    return {};
}
#endif

expected<void, audiofile_error> CAFFContainer::finalize()
{
#ifdef KFR_AUDIO_ALAC
    if (m_currentChunkToWrite)
    {
        flush();
        // write last (partial) packet
        size_t lastPacketFrames = buffered.size() / inafdesc.mBytesPerFrame;
        buffered.resize(afdesc.mFramesPerPacket * inafdesc.mBytesPerFrame, 0);
        if (auto e = writeAudioPacket(afdesc.mFramesPerPacket); !e)
            return e;
        buffered.clear();

        if (auto e = writeChunkFinish(); !e)
            return e;
        // finish data chunk
        if (!packetOffsets.empty())
        {
            CAFFPakt pakt;
            pakt.mNumberValidFrames = framesWritten;
            pakt.mNumberPackets     = packetsWritten;
            pakt.mPrimingFrames     = 0;
            pakt.mRemainderFrames   = afdesc.mFramesPerPacket - lastPacketFrames;
            fixByteOrder(pakt);

            std::vector<uint8_t> packetTable;
            for (size_t i = 0; i < packetOffsets.size(); i++)
            {
                uint64_t pktSize = i > 0 ? packetOffsets[i] - packetOffsets[i - 1] : packetOffsets[0];

                int32_t berSize = kMaxBERSize;
                size_t pos      = packetTable.size();
                packetTable.resize(pos + kMaxBERSize);
                GetBERInteger(pktSize, packetTable.data() + pos, &berSize);
                packetTable.resize(pos + berSize);
            }
            packetTable.push_back(0);
            packetTable.insert(packetTable.begin(), reinterpret_cast<const uint8_t*>(&pakt),
                               reinterpret_cast<const uint8_t*>(&pakt) + sizeof(pakt));
            return writeChunkBytes("pakt", packetTable);
        }
    }
#endif
    finalized = true;
    return {};
}
} // namespace kfr
