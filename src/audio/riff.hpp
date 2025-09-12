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
#pragma once

#include <kfr/audio/decoder.hpp>
#include <kfr/audio/encoder.hpp>
#include <kfr/meta.hpp>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <kfr/test/assert.hpp>

namespace kfr
{

#if defined _MSC_VER // MSVC
#define KFR_IO_SEEK_64 _fseeki64
#define KFR_IO_TELL_64 _ftelli64
#elif defined _WIN32 // MinGW
#define KFR_IO_SEEK_64 fseeko64
#define KFR_IO_TELL_64 ftello64
#else // macOS, Linux
#define KFR_IO_SEEK_64 fseeko
#define KFR_IO_TELL_64 ftello
#endif

template <typename T>
constexpr void cmemcpy(T* dest, const T* src, size_t n) noexcept
{
    for (size_t i = 0; i < n; ++i)
    {
        dest[i] = src[i];
    }
}

struct alignas(1) FourCC
{
    constexpr FourCC() noexcept = default;
    constexpr FourCC(const char (&str)[5]) noexcept { cmemcpy(data, str, 4); }
    constexpr FourCC(const std::string_view str) noexcept
    {
        if (str.size() == 4)
            cmemcpy(data, str.data(), 4);
        else
            stop_constexpr();
    }
    constexpr explicit FourCC(uint32_t val) noexcept { *this = std::bit_cast<FourCC>(val); }
    bool operator==(const FourCC& other) const noexcept { return std::memcmp(data, other.data, 4) == 0; }
    bool operator<(const FourCC& other) const noexcept { return std::memcmp(data, other.data, 4) < 0; }
    explicit operator std::string() const { return std::string(data, 4); }
    constexpr explicit operator uint32_t() const noexcept { return std::bit_cast<uint32_t>(*this); }

    char data[4]{};
};

/// @brief A parser for LIST chunk
struct RIFFMetadataParser
{
    static metadata_map fromBytes(const std::vector<uint8_t>& bytes)
    {
        metadata_map map;
        if (bytes.size() < 4)
            return map;
        size_t p = 4;
        while (bytes.size() - p >= 8)
        {
            std::string key(bytes.data() + p, bytes.data() + p + 4);
            uint32_t valsize;
            memcpy(&valsize, bytes.data() + p + 4, sizeof(uint32_t));
            std::string value(bytes.data() + p + 8, bytes.data() + p + 8 + valsize);
            // remove trailing \0
            value               = value.substr(0, value.find_first_of('\0'));
            map[std::move(key)] = std::move(value);
            p                   = align_up(p + 8 + valsize, 2);
        }
        return map;
    }

    static std::vector<uint8_t> toBytes(const metadata_map& map)
    {
        std::vector<uint8_t> bytes(4);
        *reinterpret_cast<FourCC*>(bytes.data()) = FourCC("INFO");
        for (auto&& kv : map)
        {
            if (kv.first.size() == 4)
            {
                size_t p         = bytes.size();
                uint32_t valsize = kv.second.size() + 1;
                bytes.resize(p + 8 + align_up(valsize, 2), 0);
                std::memcpy(bytes.data() + p, kv.first.data(), sizeof(uint32_t));
                std::memcpy(bytes.data() + p + 4, &valsize, sizeof(uint32_t));
                std::memcpy(bytes.data() + p + 8, kv.second.data(), valsize - 1);
            }
        }
        return bytes;
    }

    const static inline std::string softwareKey = "ISFT";
};

template <typename T>
inline void convertEndianess(T& value)
{
    union
    {
        T val{ 0 };
        uint8_t raw[sizeof(T)];
    } u;
    for (size_t i = 0; i < sizeof(T); i++)
    {
        if constexpr (is_poweroftwo(sizeof(T)))
            u.raw[i] = reinterpret_cast<const uint8_t*>(&value)[i ^ (sizeof(T) - 1)];
        else
            u.raw[i] = reinterpret_cast<const uint8_t*>(&value)[i / sizeof(T) + (sizeof(T) - 1 - i)];
    }
    value = u.val;
}

inline void convertEndianess(int16_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ushort(value);
#else
    value = __builtin_bswap16(value);
#endif
}

inline void convertEndianess(uint16_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ushort(value);
#else
    value = __builtin_bswap16(value);
#endif
}

inline void convertEndianess(int32_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ulong(value);
#else
    value = __builtin_bswap32(value);
#endif
}

inline void convertEndianess(uint32_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ulong(value);
#else
    value = __builtin_bswap32(value);
#endif
}

inline void convertEndianess(int64_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_uint64(value);
#else
    value = __builtin_bswap64(value);
#endif
}

inline void convertEndianess(uint64_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_uint64(value);
#else
    value = __builtin_bswap64(value);
#endif
}

template <typename T>
inline void fixByteOrder(T& val)
{
}

template <typename Traits>
class RIFFContainer;
template <typename ContainerType>
struct RIFFDecoder;
template <typename ContainerType>
struct RIFFEncoder;

struct RIFFTraits
{
    /// @brief File header
    struct MainHeader
    {
        FourCC riff;
        uint32_t size;
        FourCC type;
    };
    /// @brief Chunk ID
    using IDType = FourCC;
    /// @brief Chunk size type
    using SizeType = uint32_t;
    struct ChunkType
    {
        IDType id;
        SizeType size;
    };
    constexpr static int chunkAlignment                  = 2;
    constexpr static bool chunkSizeIsAligned             = false;
    constexpr static std::optional<IDType> metadataChunk = "LIST";

    /// @param position byte offset pointing at chunk header
    /// @returns byte size of chunk content
    [[nodiscard]] static expected<uint64_t, audiofile_error> chunkGetSize(const ChunkType& chunk,
                                                                          uint64_t position,
                                                                          RIFFContainer<RIFFTraits>*)
    {
        return chunk.size;
    }
    /// @param position byte offset pointing at chunk header
    /// @param byteSize byte size of chunk content
    [[nodiscard]] static expected<void, audiofile_error> chunkSetSize(ChunkType& chunk, uint64_t position,
                                                                      uint64_t byteSize,
                                                                      RIFFContainer<RIFFTraits>*)
    {
        chunk.size = byteSize;
        return {};
    }
};

template <typename Fn>
[[nodiscard]] bool forPCMCodec(Fn&& fn, audiofile_codec codec, int bitDepth)
{
    switch (codec)
    {
    case audiofile_codec::lpcm:
        if (bitDepth >= 25 && bitDepth <= 32)
        {
            fn(ctype<int32_t>);
            // 32
        }
        else if (bitDepth >= 17)
        {
            fn(ctype<i24>);
            // 24
        }
        else if (bitDepth >= 9)
        {
            fn(ctype<int16_t>);
            // 16
        }
        else
            return false;
        return true;
    case audiofile_codec::ieee_float:
        if (bitDepth == 64)
        {
            fn(ctype<double>);
            // 64
        }
        else if (bitDepth == 32)
        {
            fn(ctype<float>);
            // 32
        }
        else
            return false;
        return true;
    default:
        return false;
    }
}

template <typename Traits>
class RIFFContainer
{
public:
    friend Traits;

    friend struct RIFFDecoder<RIFFContainer<Traits>>;
    friend struct RIFFEncoder<RIFFContainer<Traits>>;

    using ThisTraits                                     = Traits;
    using MainHeader                                     = typename Traits::MainHeader;
    using IDType                                         = typename Traits::IDType;
    using SizeType                                       = typename Traits::SizeType;
    using MetadataParser                                 = typename Traits::MetadataParser;
    constexpr static int chunkAlignment                  = Traits::chunkAlignment;
    constexpr static bool chunkSizeIsAligned             = Traits::chunkSizeIsAligned;
    using ChunkType                                      = typename Traits::ChunkType;
    constexpr static std::optional<IDType> metadataChunk = Traits::metadataChunk;
    constexpr static IDType audioChunk                   = Traits::audioChunk;
    using EncodingOptions                                = typename Traits::EncodingOptions;
    using DecodingOptions                                = typename Traits::DecodingOptions;

    static_assert(chunkAlignment <= 16);
    static_assert(chunkAlignment > 0);

    struct ChunkInfo
    {
        ChunkType chunk;
        /// @brief offset to chunk data, right after header
        uint64_t fileOffset;
        /// @brief byte size of chunk data, excluding header
        uint64_t byteSize;
    };
    RIFFContainer() : m_stream(nullptr) {}

    void open(std::FILE* stream) { m_stream = stream; }

    /// @brief Returns full size of the file in bytes
    [[nodiscard]] expected<uint64_t, audiofile_error> readHeader()
    {
        if (KFR_IO_SEEK_64(m_stream, 0, SEEK_SET) != 0)
            return unexpected(audiofile_error::io_error);

        if (fread(&m_header, 1, sizeof(MainHeader), m_stream) != sizeof(MainHeader))
            return unexpected(audiofile_error::io_error);
        fixByteOrder(m_header);

        if (KFR_IO_SEEK_64(m_stream, 0, SEEK_END) != 0)
            return unexpected(audiofile_error::io_error);

        m_fileSize = KFR_IO_TELL_64(m_stream);
        return m_fileSize;
    }

    [[nodiscard]] expected<void, audiofile_error> writeHeader()
    {
        if (KFR_IO_SEEK_64(m_stream, 0, SEEK_SET) != 0)
            return unexpected(audiofile_error::io_error);

        MainHeader hdrFixed = m_header;
        fixByteOrder(hdrFixed);

        if (fwrite(&hdrFixed, 1, sizeof(MainHeader), m_stream) != sizeof(MainHeader))
            return unexpected(audiofile_error::io_error);

        return {};
    }

    /// @brief Returns number of chunks in file
    [[nodiscard]] expected<size_t, audiofile_error> readChunks()
    {
        uint64_t position = sizeof(MainHeader);
        for (;;)
        {
            if (KFR_IO_SEEK_64(m_stream, position, SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
            ChunkType chunk;
            if (fread(&chunk, 1, sizeof(ChunkType), m_stream) != sizeof(ChunkType))
                return unexpected(audiofile_error::io_error);
            fixByteOrder(chunk);
            expected<uint64_t, audiofile_error> chunkRealSize = Traits::chunkGetSize(chunk, position, this);
            if (!chunkRealSize)
                return unexpected(chunkRealSize.error());
            m_chunks.push_back(ChunkInfo{ chunk, position + sizeof(ChunkType), *chunkRealSize });
            position += *chunkRealSize + sizeof(ChunkType);
            position = align_up(position, chunkAlignment);
            if (position > m_fileSize)
                return unexpected(audiofile_error::format_error);
            if (position == m_fileSize)
                return m_chunks.size();
        }
    }

    /// @brief Returns number of bytes in chunk
    [[nodiscard]] expected<uint64_t, audiofile_error> readChunkStart(IDType id, uint64_t offsetInChunk = 0)
    {
        m_currentChunkToRead = findChunk(id);
        if (!m_currentChunkToRead)
            return unexpected(audiofile_error::io_error);
        m_offsetInChunk = offsetInChunk;
        return m_chunks[*m_currentChunkToRead].byteSize;
    }

    /// @brief Sets current chunk's read position (in bytes)
    [[nodiscard]] expected<void, audiofile_error> readChunkSeek(uint64_t offsetInChunk = 0)
    {
        if (!m_currentChunkToRead)
            return unexpected(audiofile_error::io_error);
        if (offsetInChunk > m_chunks[*m_currentChunkToRead].byteSize)
            return unexpected(audiofile_error::end_of_file);
        m_offsetInChunk = offsetInChunk;
        return {};
    }

    /// @brief Returns number of bytes read or EndOfFile if chunk is finished
    [[nodiscard]] expected<size_t, audiofile_error> readChunkContinue(size_t sizeToRead, void* buffer)
    {
        ASSERT(m_currentChunkToRead);
        if (m_offsetInChunk >= m_chunks[*m_currentChunkToRead].byteSize)
            return unexpected(audiofile_error::end_of_file);

        if (m_offsetInChunk + sizeToRead > m_chunks[*m_currentChunkToRead].byteSize)
            sizeToRead = m_chunks[*m_currentChunkToRead].byteSize - m_offsetInChunk;

        if (KFR_IO_SEEK_64(m_stream, m_chunks[*m_currentChunkToRead].fileOffset + m_offsetInChunk,
                           SEEK_SET) != 0)
            return unexpected(audiofile_error::io_error);

        size_t rd = fread(buffer, 1, sizeToRead, m_stream);
        if (rd == 0)
            return unexpected(audiofile_error::io_error);
        m_offsetInChunk += rd;
        return rd;
    }
    void readChunkFinish() { m_currentChunkToRead = std::nullopt; }

    /// @brief Returns number of bytes read or EndOfFile if chunk is finished
    template <typename T>
    [[nodiscard]] expected<void, audiofile_error> readChunkTo(IDType id, T& value)
    {
        if (auto e = readChunkStart(id); !e)
            return unexpected(e.error());
        if (auto e = readChunkContinue(sizeof(T), &value); !e)
            return unexpected(e.error());
        else if (*e == sizeof(T))
        {
            fixByteOrder(value);
            readChunkFinish();
            return {};
        }
        return unexpected(audiofile_error::format_error);
    }

    [[nodiscard]] expected<std::vector<uint8_t>, audiofile_error> readChunkBytes(IDType id)
    {
        std::vector<uint8_t> result;
        if (auto e = readChunkStart(id); !e)
            return unexpected(e.error());
        else
            result.resize(*e);

        if (auto e = readChunkContinue(result.size(), result.data()); !e)
            return unexpected(e.error());
        else if (*e == result.size())
        {
            readChunkFinish();
            return result;
        }
        return unexpected(audiofile_error::io_error);
    }

    [[nodiscard]] expected<void, audiofile_error> readChunkBuffered(
        IDType id, std::function<void(const std::vector<uint8_t>&)> callback, uint64_t offsetInChunk = 0)
    {
        constexpr size_t bufferSize = 65536;
        std::vector<uint8_t> buffer(bufferSize);
        if (auto e = readChunkStart(id, offsetInChunk); !e)
            return unexpected(e.error());

        for (;;)
        {
            if (auto e = readChunkContinue(buffer.size(), buffer.data()); e)
            {
                buffer.resize(*e);
                callback(buffer);
            }
            else
            {
                if (e.error() == audiofile_error::end_of_file)
                {
                    readChunkFinish();
                    return {};
                }
                else
                {
                    return unexpected(e.error());
                }
            }
        }
    }

    [[nodiscard]] expected<size_t, audiofile_error> readRawAudioContinue(size_t framesToRead, void* buffer,
                                                                         size_t bytesPerFrame)
    {
        expected<size_t, audiofile_error> e = readChunkContinue(framesToRead * bytesPerFrame, buffer);
        if (!e)
            return unexpected(e.error());
        if (*e % bytesPerFrame != 0)
            return unexpected(audiofile_error::io_error);
        return *e / bytesPerFrame;
    }

    [[nodiscard]] expected<audio_data, audiofile_error> readPCMAudio(size_t framesToRead,
                                                                     const audiofile_metadata* meta)
    {
        audio_data result(meta);
        univector<uint8_t> interleaved(framesToRead * meta->bytes_per_pcm_frame());
        auto e = readRawAudioContinue(framesToRead, interleaved.data(), meta->bytes_per_pcm_frame());
        if (!e)
            return unexpected(e.error());
        framesToRead = *e;

        result.resize(framesToRead);
        if (!forPCMCodec(
                [&]<typename T>(ctype_t<T>)
                {
                    T* interleavedAudio = reinterpret_cast<T*>(interleaved.data());
                    if (meta->endianness != audiofile_endianness::little)
                    {
                        for (size_t i = 0; i < framesToRead * meta->channels; i++)
                            convertEndianess(interleavedAudio[i]);
                    }
                    deinterleave_samples(result.pointers(), interleavedAudio, meta->channels, framesToRead);
                },
                meta->codec, meta->bit_depth))
            return unexpected(audiofile_error::format_error);
        return result;
    }

    [[nodiscard]] expected<void, audiofile_error> writePCMAudio(const audio_data& audio,
                                                                const audio_quantinization& quant)
    {
        const audiofile_metadata* meta = audio.typed_metadata<audiofile_metadata>();
        KFR_ASSERT(meta);
        size_t framesToWrite = audio.size;
        m_framesWritten += framesToWrite;
        univector<uint8_t> interleaved(framesToWrite * meta->bytes_per_pcm_frame());
        if (!forPCMCodec(
                [&]<typename T>(ctype_t<T>)
                {
                    T* interleavedAudio = reinterpret_cast<T*>(interleaved.data());
                    interleave_samples(interleavedAudio, audio.pointers(), audio.channel_count(),
                                       framesToWrite, quant);

                    if (meta->endianness != audiofile_endianness::little)
                    {
                        for (size_t i = 0; i < framesToWrite * meta->channels; i++)
                            convertEndianess(interleavedAudio[i]);
                    }
                },
                meta->codec, meta->bit_depth))
            return unexpected(audiofile_error::format_error);
        return writeRawAudio(interleaved.data(), framesToWrite, meta->bytes_per_pcm_frame());
    }

    /// @brief Writes chunk header to stream, adds chunk to list
    [[nodiscard]] expected<void, audiofile_error> writeChunkStart(IDType id, uint64_t dataSize = UINT64_MAX,
                                                                  bool overwrite              = false,
                                                                  std::optional<IDType> newID = std::nullopt)
    {
        if (m_currentChunkToWrite)
        {
            if (auto e = writeChunkFinish(); !e)
                return unexpected(e.error());
        }
        std::optional<size_t> chIndex = findChunk(id);
        ChunkType chunk;
        if (overwrite && chIndex)
        {
            uint64_t fileOffset = m_chunks[*chIndex].fileOffset;
            chunk.id            = newID.value_or(id);
            if (auto e = Traits::chunkSetSize(chunk, fileOffset - sizeof(ChunkType), dataSize, this); !e)
                return e;
            m_currentChunkToWrite         = *chIndex;
            m_chunks[*chIndex].chunk      = chunk;
            m_chunks[*chIndex].fileOffset = fileOffset;
            m_chunks[*chIndex].byteSize   = dataSize;
            if (KFR_IO_SEEK_64(m_stream, fileOffset, SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
        }
        else
        {
            uint64_t fileOffset = KFR_IO_TELL_64(m_stream) + sizeof(ChunkType);
            chunk.id            = id;
            if (auto e = Traits::chunkSetSize(chunk, fileOffset - sizeof(ChunkType), dataSize, this); !e)
                return e;
            m_currentChunkToWrite = m_chunks.size();
            m_chunks.push_back(ChunkInfo{ chunk, fileOffset, dataSize });
        }
        if (chunkSizeIsAligned)
            chunk.size = align_up(chunk.size, chunkAlignment);
        fixByteOrder(chunk);
        size_t wr = fwrite(&chunk, 1, sizeof(ChunkType), m_stream);
        if (wr != sizeof(ChunkType))
            return unexpected(audiofile_error::io_error);
        return {};
    }

    /// @brief write data portion to chunk
    [[nodiscard]] expected<void, audiofile_error> writeChunkContinue(const void* data, size_t size)
    {
        ASSERT(m_currentChunkToWrite);
        size_t wr = fwrite(data, 1, size, m_stream);
        if (wr != size)
            return unexpected(audiofile_error::io_error);
        return {};
    }
    /// @brief write audio portion to chunk
    [[nodiscard]] expected<void, audiofile_error> writeRawAudio(const void* buffer, size_t framesToWrite,
                                                                size_t bytesPerFrame)
    {
        return writeChunkContinue(buffer, framesToWrite * bytesPerFrame);
    }
    /// @brief Finishes chunk writing, fixes chunk header if needed
    [[nodiscard]] expected<void, audiofile_error> writeChunkFinish()
    {
        uint64_t fileOffset   = KFR_IO_TELL_64(m_stream);
        m_fileSize            = fileOffset;
        ChunkInfo& lastChunk  = m_chunks.back();
        uint64_t bytesWritten = fileOffset - lastChunk.fileOffset;
        m_currentChunkToWrite = std::nullopt;
        if (fileOffset - lastChunk.fileOffset != lastChunk.byteSize)
        {
            lastChunk.byteSize = bytesWritten;
            if (KFR_IO_SEEK_64(m_stream, lastChunk.fileOffset - sizeof(ChunkType), SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
            if (auto e =
                    Traits::chunkSetSize(lastChunk.chunk, fileOffset - sizeof(ChunkType), bytesWritten, this);
                !e)
                return e;
            ChunkType chunk = lastChunk.chunk;
            if (chunkSizeIsAligned)
                chunk.size = align_up(chunk.size, chunkAlignment);
            fixByteOrder(chunk);
            size_t wr = fwrite(&chunk, 1, sizeof(ChunkType), m_stream);
            if (wr != sizeof(ChunkType))
                return unexpected(audiofile_error::io_error);
            if (KFR_IO_SEEK_64(m_stream, fileOffset, SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
            if (chunkAlignment > 1)
                return writePadding(align_up(fileOffset, chunkAlignment) - fileOffset);
        }
        return {};
    }

    [[nodiscard]] expected<void, audiofile_error> writePadding(size_t bytes, uint8_t value = 0)
    {
        ASSERT(bytes <= 15);
        static const std::array<uint8_t, 16> padding = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        size_t wr = fwrite(&padding, 1, bytes, m_stream);
        if (wr != bytes)
            return unexpected(audiofile_error::io_error);
        return {};
    }

    template <typename T>
    [[nodiscard]] expected<void, audiofile_error> writeChunkFrom(IDType id, T value, bool overwrite = false,
                                                                 std::optional<IDType> newID = std::nullopt)
    {
        fixByteOrder(value);
        if (auto e = writeChunkStart(id, sizeof(T), overwrite, newID); !e)
            return unexpected(e.error());
        if (auto e = writeChunkContinue(&value, sizeof(T)); !e)
            return unexpected(e.error());
        if (auto e = writeChunkFinish(); !e)
            return unexpected(e.error());
        return {};
    }

    [[nodiscard]] expected<void, audiofile_error> writeChunkBytes(IDType id,
                                                                  const std::vector<uint8_t>& value,
                                                                  bool overwrite              = false,
                                                                  std::optional<IDType> newID = std::nullopt)
    {
        if (auto e = writeChunkStart(id, value.size(), overwrite, newID); !e)
            return unexpected(e.error());
        if (auto e = writeChunkContinue(value.data(), value.size()); !e)
            return unexpected(e.error());
        if (auto e = writeChunkFinish(); !e)
            return unexpected(e.error());
        return {};
    }
    [[nodiscard]] expected<void, audiofile_error> copyChunkFrom(RIFFContainer* sourceContainer, IDType id)
    {
        if (sourceContainer->findChunk(id))
        {
            if (auto content = sourceContainer->readChunkBytes(id))
                return writeChunkBytes(id, *content);
            else
                return unexpected(content.error());
        }
        return {};
    }

    void flush() { fflush(m_stream); }

    ~RIFFContainer() { ASSERT(!m_currentChunkToWrite); }

    const std::vector<ChunkInfo>& chunks() const { return m_chunks; }
    uint64_t fileSize() const { return m_fileSize; }

    [[nodiscard]] std::optional<size_t> findChunk(IDType id) const
    {
        for (size_t i = 0; i < m_chunks.size(); i++)
        {
            if (m_chunks[i].chunk.id == id)
                return i;
        }
        return std::nullopt;
    }

    [[nodiscard]] expected<void, audiofile_error> copyInitialChunksFrom(RIFFContainer* sourceContainer)
    {
        return {};
    }
    [[nodiscard]] expected<void, audiofile_error> copyFinalChunksFrom(RIFFContainer* sourceContainer)
    {
        return {};
    }

    DecodingOptions decodingOptions;
    EncodingOptions encodingOptions;

    void close()
    {
        m_framesWritten = 0;
        m_offsetInChunk = 0;
        m_currentChunkToWrite.reset();
        m_currentChunkToRead.reset();
        m_chunks.clear();
        m_fileSize = 0;
        m_header   = {};
        m_stream   = nullptr;
    }

protected:
    std::FILE* m_stream = nullptr;
    MainHeader m_header{};
    uint64_t m_fileSize = 0;
    std::vector<ChunkInfo> m_chunks;
    std::optional<size_t> m_currentChunkToRead;
    std::optional<size_t> m_currentChunkToWrite;
    uint64_t m_offsetInChunk = 0;
    uint64_t m_framesWritten = 0;
};

template <typename ContainerType>
struct RIFFDecoder : public audio_decoder
{
    [[nodiscard]] expected<audiofile_metadata, audiofile_error> open(const file_path& path) override
    {
        auto f = fopen_path(path, open_file_mode::read_existing);
        if (f)
            file.reset(*f);
        else
            return unexpected(audiofile_error::io_error);

        container.open(file.get());
        if (auto e = container.readHeader(); !e)
            return unexpected(e.error());
        if (auto e = container.readChunks(); !e)
            return unexpected(e.error());

        auto info = container.readFormat();
        if (!info)
            return unexpected(info.error());
        if (!info->valid())
            return unexpected(audiofile_error::format_error);
        m_metadata = *info;

        if (container.decodingOptions.read_metadata)
        {
            if constexpr (ContainerType::metadataChunk)
            {
                metadata_map map;
                using Parser = typename ContainerType::MetadataParser;
                if (container.findChunk(*ContainerType::metadataChunk))
                {
                    if (auto m = container.readChunkBytes(*ContainerType::metadataChunk))
                        map = Parser::fromBytes(*m);
                    else
                        return unexpected(m.error());
                }
                m_metadata->metadata = std::move(map);
            }
        }

        return *m_metadata;
    }
    [[nodiscard]] expected<audio_data, audiofile_error> read() override
    {
        if (!m_metadata)
            return unexpected(audiofile_error::internal_error);
        return container.readAudio(&*m_metadata);
    }

    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override
    {
        if (!m_metadata)
            return unexpected(audiofile_error::internal_error);
        if (position > m_metadata->total_frames)
            return unexpected(audiofile_error::end_of_file);
        return container.seek(&*m_metadata, position);
    }

    void close() override
    {
        container.close();
        file.reset();
        m_metadata.reset();
    }

    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
    ContainerType container;

    RIFFDecoder(typename ContainerType::DecodingOptions opts = {})
    {
        container.decodingOptions = std::move(opts);
    }
};

template <typename ContainerType>
struct RIFFEncoder : public audio_encoder
{
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path,
                                                       audio_decoder* copyMetadataFrom = nullptr) override
    {
        this->copyMetadataFrom = dynamic_cast<RIFFDecoder<ContainerType>*>(copyMetadataFrom);
        auto f                 = fopen_path(path, open_file_mode::write_new);
        if (f)
            file.reset(*f);
        else
            return unexpected(audiofile_error::io_error);

        container.open(file.get());

        if (auto e = container.writeHeader(); !e)
            return unexpected(e.error());

        return {};
    }

    [[nodiscard]] expected<void, audiofile_error> prepare(const audiofile_metadata& meta) override
    {
        m_metadata = meta;
        if (!meta.valid())
        {
            return unexpected(audiofile_error::format_error);
        }

        if (auto e = container.writeFormat(&meta); !e)
            return unexpected(e.error());
        if constexpr (ContainerType::metadataChunk)
        {
            metadata_map map;
            using Parser = typename ContainerType::MetadataParser;
            if (this->copyMetadataFrom &&
                this->copyMetadataFrom->container.findChunk(*ContainerType::metadataChunk))
            {
                if (auto m = this->copyMetadataFrom->container.readChunkBytes(*ContainerType::metadataChunk))
                    map = Parser::fromBytes(*m);
                else
                    return unexpected(m.error());
            }
            map[Parser::softwareKey] = audio_writing_software;
            if (auto e = container.writeChunkBytes(*ContainerType::metadataChunk, Parser::toBytes(map)); !e)
                return e;
        }
        if (this->copyMetadataFrom)
            return container.copyInitialChunksFrom(&this->copyMetadataFrom->container);
        return {};
    }
    [[nodiscard]] expected<void, audiofile_error> write(const audio_data& data) override
    {
        const audiofile_metadata* meta = data.typed_metadata<audiofile_metadata>();
        KFR_ASSERT(meta);
        if (!m_metadata)
        {
            auto e = prepare(*meta);
            if (!e)
                return e;
        }
        if (!meta->compatible(*m_metadata))
        {
            return unexpected(audiofile_error::format_error);
        }
        if (data.size > 0)
        {
            audio_data newData        = data;
            newData.metadata          = &*m_metadata;
            audio_dithering dithering = audio_dithering::none;
            if constexpr (std::derived_from<typename ContainerType::EncodingOptions, audio_encoding_options>)
            {
                dithering = container.encodingOptions.dithering;
            }
            return container.writeAudio(newData, audio_quantinization(meta->bit_depth, dithering));
        }
        else
        {
            return {};
        }
    }

    [[nodiscard]] expected<void, audiofile_error> close() override
    {
        if (this->copyMetadataFrom)
            if (auto e = container.copyFinalChunksFrom(&this->copyMetadataFrom->container); !e)
                return e;
        expected<void, audiofile_error> result = container.finalize();
        container.close();
        file.reset();
        return result;
    }

    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
    ContainerType container;
    RIFFDecoder<ContainerType>* copyMetadataFrom = nullptr;
    std::optional<audiofile_metadata> m_metadata;

    RIFFEncoder(typename ContainerType::EncodingOptions opts = {})
    {
        container.encodingOptions = std::move(opts);
    }
};

} // namespace kfr
