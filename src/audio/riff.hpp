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

    operator std::span<const std::byte>() const noexcept
    {
        return std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), 4);
    }

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
inline void fixByteOrder(T& val)
{
}

// template <typename Traits>
// class RIFFContainer;
template <typename Decoder, typename Traits>
struct RIFFDecoder;
template <typename Encoder, typename Traits>
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

#if 0
template <typename Traits>
class RIFFContainer
{
public:
    friend Traits;

    friend struct RIFFEncoder<RIFFContainer<Traits>>;

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

    EncodingOptions encodingOptions;

    uint64_t framesWritten() const { return m_framesWritten; }

    void close()
    {
        m_framesWritten = 0;
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
    std::optional<size_t> m_currentChunkToWrite;
    uint64_t m_framesWritten = 0;
};
#endif

template <typename Decoder, typename Traits>
struct RIFFDecoder : public audio_decoder
{
protected:
    using MainHeader                                     = typename Traits::MainHeader;
    using IDType                                         = typename Traits::IDType;
    using SizeType                                       = typename Traits::SizeType;
    using ChunkType                                      = typename Traits::ChunkType;
    using DecodingOptions                                = typename Traits::DecodingOptions;
    constexpr static int chunkAlignment                  = Traits::chunkAlignment;
    constexpr static bool chunkSizeIsAligned             = Traits::chunkSizeIsAligned;
    constexpr static std::optional<IDType> metadataChunk = Traits::metadataChunk;
    using MetadataParser                                 = typename Traits::MetadataParser;

    DecodingOptions m_options;
    std::unique_ptr<std::FILE, details::stdFILE_deleter> m_file;
    uint64_t m_fileSize = 0;
    MainHeader m_header{};

    struct ChunkInfo
    {
        ChunkType chunk;
        /// @brief offset to chunk data, right after header
        uint64_t fileOffset;
        /// @brief byte size of chunk data, excluding header
        uint64_t byteSize;
    };

    std::vector<ChunkInfo> m_chunks;
    std::optional<size_t> m_currentChunkToRead;
    uint64_t m_offsetInChunk = 0;

protected:
public:
    [[nodiscard]] std::optional<size_t> findChunk(IDType id) const
    {
        for (size_t i = 0; i < m_chunks.size(); i++)
        {
            if (m_chunks[i].chunk.id == id)
                return i;
        }
        return std::nullopt;
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

        if (KFR_IO_SEEK_64(m_file.get(), m_chunks[*m_currentChunkToRead].fileOffset + m_offsetInChunk,
                           SEEK_SET) != 0)
            return unexpected(audiofile_error::io_error);

        size_t rd = fread(buffer, 1, sizeToRead, m_file.get());
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

    [[nodiscard]] expected<size_t, audiofile_error> readPCMAudio(const audio_data_interleaved& audio)
    {
        size_t framesToRead  = audio.size;
        size_t bytesPerFrame = m_format->bytes_per_pcm_frame();
        univector<std::byte> interleaved(framesToRead * bytesPerFrame);

        expected<size_t, audiofile_error> e =
            readChunkContinue(framesToRead * bytesPerFrame, interleaved.data());
        if (!e)
            return unexpected(e.error());
        if (*e % bytesPerFrame != 0)
            return unexpected(audiofile_error::io_error);
        size_t framesRead = *e / bytesPerFrame;

        sample_t typ = m_format->sample_type();
        if (typ == sample_t::unknown)
            return unexpected(audiofile_error::format_error);

        samples_load(typ, audio.data, interleaved.data(), m_format->channels * framesToRead,
                     m_format->endianness != audiofile_endianness::little);
        return framesRead;
    }

    /// @brief Returns full size of the file in bytes
    [[nodiscard]] expected<uint64_t, audiofile_error> readHeader() // Decoder
    {
        if (KFR_IO_SEEK_64(m_file.get(), 0, SEEK_SET) != 0)
            return unexpected(audiofile_error::io_error);

        if (fread(&m_header, 1, sizeof(MainHeader), m_file.get()) != sizeof(MainHeader))
            return unexpected(audiofile_error::io_error);
        fixByteOrder(m_header);

        if (KFR_IO_SEEK_64(m_file.get(), 0, SEEK_END) != 0)
            return unexpected(audiofile_error::io_error);

        m_fileSize = KFR_IO_TELL_64(m_file.get());
        return m_fileSize;
    }

    /// @brief Returns number of chunks in file
    [[nodiscard]] expected<size_t, audiofile_error> readChunks() // Decoder
    {
        uint64_t position = sizeof(MainHeader);
        for (;;)
        {
            if (KFR_IO_SEEK_64(m_file.get(), position, SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
            ChunkType chunk;
            if (fread(&chunk, 1, sizeof(ChunkType), m_file.get()) != sizeof(ChunkType))
                return unexpected(audiofile_error::io_error);
            fixByteOrder(chunk);
            expected<uint64_t, audiofile_error> chunkRealSize =
                static_cast<Decoder*>(this)->chunkGetSize(chunk, position);
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

    [[nodiscard]] expected<audiofile_format, audiofile_error> open(const file_path& path) override
    {
        auto f = fopen_path(path, open_file_mode::read_existing);
        if (f)
            m_file.reset(*f);
        else
            return unexpected(audiofile_error::io_error);

        if (auto e = this->readHeader(); !e)
            return unexpected(e.error());
        if (auto e = this->readChunks(); !e)
            return unexpected(e.error());

        auto info = static_cast<Decoder*>(this)->readFormat();
        if (!info)
            return unexpected(info.error());
        if (!info->valid())
            return unexpected(audiofile_error::format_error);
        m_format = *info;

        if (m_options.read_metadata)
        {
            if constexpr (metadataChunk)
            {
                metadata_map map;
                if (findChunk(*metadataChunk))
                {
                    if (auto m = readChunkBytes(*metadataChunk))
                        map = MetadataParser::fromBytes(*m);
                    else
                        return unexpected(m.error());
                }
                m_format->metadata = std::move(map);
            }
        }

        return *m_format;
    }
    [[nodiscard]] expected<size_t, audiofile_error> read_to(const audio_data_interleaved& audio) override
    {
        if (!m_format)
            return unexpected(audiofile_error::closed);
        return static_cast<Decoder*>(this)->readTo(audio);
    }

    std::optional<uint64_t> has_chunk(std::span<const std::byte> chunk_id) const override
    {
        if (chunk_id.size() != sizeof(IDType))
            return std::nullopt;
        IDType id;
        std::memcpy(&id, chunk_id.data(), sizeof(IDType));
        if (auto idx = findChunk(id))
            return m_chunks[*idx].byteSize;
        return std::nullopt;
    }

    expected<void, audiofile_error> read_chunk(std::span<const std::byte> chunk_id,
                                               const std::function<bool(std::span<const std::byte>)>& handler,
                                               size_t buffer_size) override
    {
        IDType chunkId;
        if (chunk_id.size() != sizeof(chunkId))
            return unexpected(audiofile_error::not_implemented);
        std::memcpy(&chunkId, chunk_id.data(), sizeof(chunkId));

        auto chunkSize = readChunkStart(chunkId);
        if (!chunkSize)
            return unexpected(chunkSize.error());

        size_t sizeToRead = std::min(buffer_size, size_t(*chunkSize));
        std::unique_ptr<std::byte[]> buffer(new std::byte[sizeToRead]);

        for (;;)
        {
            auto rd = readChunkContinue(buffer_size, buffer.get());
            if (rd)
            {
                if (!handler(std::span(buffer.get(), *rd)))
                {
                    readChunkFinish();
                    break;
                }
            }
            else
            {
                readChunkFinish();
                if (rd.error() == audiofile_error::end_of_file)
                    break;

                return unexpected(rd.error());
            }
        }
        return {};
    }

    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override
    {
        if (!m_format)
            return unexpected(audiofile_error::closed);
        if (position >= m_format->total_frames)
            return unexpected(audiofile_error::end_of_file);
        return static_cast<Decoder*>(this)->seekTo(position);
    }

    void close() override
    {
        m_file.reset();
        m_format.reset();
    }

    RIFFDecoder(DecodingOptions opts = {}) : m_options(std::move(opts)) {}
};

template <typename Encoder, typename Traits>
struct RIFFEncoder : public audio_encoder
{
protected:
    using MainHeader                                     = typename Traits::MainHeader;
    using IDType                                         = typename Traits::IDType;
    using SizeType                                       = typename Traits::SizeType;
    using ChunkType                                      = typename Traits::ChunkType;
    using EncodingOptions                                = typename Traits::EncodingOptions;
    constexpr static int chunkAlignment                  = Traits::chunkAlignment;
    constexpr static bool chunkSizeIsAligned             = Traits::chunkSizeIsAligned;
    constexpr static std::optional<IDType> metadataChunk = Traits::metadataChunk;
    using MetadataParser                                 = typename Traits::MetadataParser;

    EncodingOptions m_options;
    std::unique_ptr<std::FILE, details::stdFILE_deleter> m_file;
    uint64_t m_fileSize = 0;
    MainHeader m_header{};

    struct ChunkInfo
    {
        ChunkType chunk;
        /// @brief offset to chunk data, right after header
        uint64_t fileOffset;
        /// @brief byte size of chunk data, excluding header
        uint64_t byteSize;
    };

    std::vector<ChunkInfo> m_chunks;
    std::optional<size_t> m_currentChunkToWrite;

public:
    [[nodiscard]] std::optional<size_t> findChunk(IDType id) const
    {
        for (size_t i = 0; i < m_chunks.size(); i++)
        {
            if (m_chunks[i].chunk.id == id)
                return i;
        }
        return std::nullopt;
    }

    [[nodiscard]] expected<void, audiofile_error> writeHeader()
    {
        if (KFR_IO_SEEK_64(m_file.get(), 0, SEEK_SET) != 0)
            return unexpected(audiofile_error::io_error);

        MainHeader hdrFixed = m_header;
        fixByteOrder(hdrFixed);

        if (fwrite(&hdrFixed, 1, sizeof(MainHeader), m_file.get()) != sizeof(MainHeader))
            return unexpected(audiofile_error::io_error);

        return {};
    }

    [[nodiscard]] expected<void, audiofile_error> writePCMAudio(const audio_data_interleaved& audio,
                                                                const audio_quantization& quant)
    {
        size_t framesToWrite = audio.size;
        m_format->total_frames += framesToWrite;
        univector<std::byte> interleaved(framesToWrite * m_format->bytes_per_pcm_frame());

        sample_t typ = m_format->sample_type();
        if (typ == sample_t::unknown)
            return unexpected(audiofile_error::format_error);
        samples_store(typ, interleaved.data(), audio.data, m_format->channels * framesToWrite, quant,
                      m_format->endianness != audiofile_endianness::little);

        return writeRawAudio(interleaved.data(), framesToWrite, m_format->bytes_per_pcm_frame());
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
            if (auto e = static_cast<Encoder*>(this)->chunkSetSize(chunk, fileOffset - sizeof(ChunkType),
                                                                   dataSize);
                !e)
                return e;
            m_currentChunkToWrite         = *chIndex;
            m_chunks[*chIndex].chunk      = chunk;
            m_chunks[*chIndex].fileOffset = fileOffset;
            m_chunks[*chIndex].byteSize   = dataSize;
            if (KFR_IO_SEEK_64(m_file.get(), fileOffset, SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
        }
        else
        {
            uint64_t fileOffset = KFR_IO_TELL_64(m_file.get()) + sizeof(ChunkType);
            chunk.id            = id;
            if (auto e = static_cast<Encoder*>(this)->chunkSetSize(chunk, fileOffset - sizeof(ChunkType),
                                                                   dataSize);
                !e)
                return e;
            m_currentChunkToWrite = m_chunks.size();
            m_chunks.push_back(ChunkInfo{ chunk, fileOffset, dataSize });
        }
        if (chunkSizeIsAligned)
            chunk.size = align_up(chunk.size, chunkAlignment);
        fixByteOrder(chunk);
        size_t wr = fwrite(&chunk, 1, sizeof(ChunkType), m_file.get());
        if (wr != sizeof(ChunkType))
            return unexpected(audiofile_error::io_error);
        return {};
    }

    /// @brief write data portion to chunk
    [[nodiscard]] expected<void, audiofile_error> writeChunkContinue(const void* data, size_t size)
    {
        ASSERT(m_currentChunkToWrite);
        size_t wr = fwrite(data, 1, size, m_file.get());
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
        uint64_t fileOffset   = KFR_IO_TELL_64(m_file.get());
        m_fileSize            = fileOffset;
        ChunkInfo& lastChunk  = m_chunks.back();
        uint64_t bytesWritten = fileOffset - lastChunk.fileOffset;
        m_currentChunkToWrite = std::nullopt;
        if (fileOffset - lastChunk.fileOffset != lastChunk.byteSize)
        {
            lastChunk.byteSize = bytesWritten;
            if (KFR_IO_SEEK_64(m_file.get(), lastChunk.fileOffset - sizeof(ChunkType), SEEK_SET) != 0)
                return unexpected(audiofile_error::io_error);
            if (auto e = static_cast<Encoder*>(this)->chunkSetSize(
                    lastChunk.chunk, fileOffset - sizeof(ChunkType), bytesWritten);
                !e)
                return e;
            ChunkType chunk = lastChunk.chunk;
            if (chunkSizeIsAligned)
                chunk.size = align_up(chunk.size, chunkAlignment);
            fixByteOrder(chunk);
            size_t wr = fwrite(&chunk, 1, sizeof(ChunkType), m_file.get());
            if (wr != sizeof(ChunkType))
                return unexpected(audiofile_error::io_error);
            if (KFR_IO_SEEK_64(m_file.get(), fileOffset, SEEK_SET) != 0)
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
        size_t wr = fwrite(&padding, 1, bytes, m_file.get());
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

    void flush() { fflush(m_file.get()); }

    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path, const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom = nullptr) override
    {
        this->copyMetadataFrom = copyMetadataFrom;
        auto f                 = fopen_path(path, open_file_mode::write_new);
        if (f)
            m_file.reset(*f);
        else
            return unexpected(audiofile_error::io_error);

        if (auto e = writeHeader(); !e)
            return unexpected(e.error());

        m_format = format;
        if (!m_format->valid())
        {
            return unexpected(audiofile_error::invalid_argument);
        }

        if (auto e = static_cast<Encoder*>(this)->writeFormat(); !e)
            return unexpected(e.error());

        if constexpr (metadataChunk)
        {
            metadata_map map;
            if (this->copyMetadataFrom && this->copyMetadataFrom->has_chunk(*metadataChunk))
            {
                if (auto m = this->copyMetadataFrom->read_chunk_bytes(*metadataChunk))
                    map = MetadataParser::fromBytes(*m);
                else
                    return unexpected(m.error());
            }
            map[MetadataParser::softwareKey] = audio_writing_software;
            if (auto e = writeChunkBytes(*metadataChunk, MetadataParser::toBytes(map)); !e)
                return e;
        }

        if (this->copyMetadataFrom)
        {
            for (IDType id : Traits::initialChunksToCopy)
            {
                if (auto e = copyChunk(id); !e)
                    return unexpected(e.error());
            }
        }
        return {};
    }
    [[nodiscard]] expected<void, audiofile_error> write(const audio_data_interleaved& data) override
    {
        if (!m_format)
            return unexpected(audiofile_error::closed);
        if (data.channels != m_format->channels)
            return unexpected(audiofile_error::invalid_argument);

        if (!data.empty())
        {
            audio_dithering dithering = audio_dithering::none;
            if constexpr (std::derived_from<EncodingOptions, audio_encoding_options>)
            {
                dithering = m_options.dithering;
            }
            return static_cast<Encoder*>(this)->writeAudio(
                data, audio_quantization(m_format->bit_depth, dithering));
        }
        else
        {
            return {};
        }
    }

    expected<void, audiofile_error> copyChunk(IDType id)
    {
        auto sourceChunk =
            this->copyMetadataFrom->has_chunk(std::span(reinterpret_cast<const std::byte*>(&id), sizeof(id)));

        if (!sourceChunk)
        {
            return {};
        }

        if (auto e = writeChunkStart(id, *sourceChunk, true); !e)
            return unexpected(e.error());

        std::optional<audiofile_error> err;
        auto e = this->copyMetadataFrom->read_chunk(
            std::span(reinterpret_cast<const std::byte*>(&id), sizeof(id)),
            [this, &err](std::span<const std::byte> data)
            {
                if (auto e = writeChunkContinue(data.data(), data.size_bytes()); !e)
                {
                    err = e.error();
                    return false;
                }
                return true;
            });
        if (!e)
            return unexpected(e.error());
        if (err)
            return unexpected(*err);
        return {};
    }

    [[nodiscard]] expected<uint64_t, audiofile_error> close() override
    {
        if (this->copyMetadataFrom)
        {
            for (IDType id : Traits::finalChunksToCopy)
            {
                if (auto e = copyChunk(id); !e)
                    return unexpected(e.error());
            }
        }

        expected<void, audiofile_error> result = static_cast<Encoder*>(this)->finalize();

        m_currentChunkToWrite.reset();
        m_chunks.clear();
        m_fileSize = 0;
        m_header   = {};

        m_file.reset();
        if (!result)
            return unexpected(result.error());
        uint64_t totalLength = m_format->total_frames;
        m_format.reset();
        return totalLength;
    }

    audio_decoder* copyMetadataFrom = nullptr;

    RIFFEncoder(EncodingOptions opts = {}) : m_options(std::move(opts)) {}
};

} // namespace kfr
