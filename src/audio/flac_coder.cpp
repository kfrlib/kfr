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

#include <kfr/audio/decoder.hpp>
#include <kfr/audio/encoder.hpp>

#ifdef KFR_AUDIO_FLAC

#include <FLAC/all.h>

namespace kfr
{

struct libflac_deleter
{
    void operator()(FLAC__StreamDecoder* f) { FLAC__stream_decoder_delete(f); }
    void operator()(FLAC__StreamEncoder* f) { FLAC__stream_encoder_delete(f); }
};

struct FLACDecoder : public audio_decoder
{
public:
    FLACDecoder(flac_decoding_options options) : options(std::move(options)) {}
    ~FLACDecoder() { close(); }
    [[nodiscard]] expected<audiofile_metadata, audiofile_error> open(const file_path& path) override;
    [[nodiscard]] expected<audio_data, audiofile_error> read() override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

protected:
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
    std::unique_ptr<FLAC__StreamDecoder, libflac_deleter> decoder;
    audio_data audio;
    bool flacError          = false;
    FLAC__uint64 dataOffset = 0;
    flac_decoding_options options;
    friend FLAC__StreamDecoderWriteStatus libflac_write_callback(const FLAC__StreamDecoder* decoder,
                                                                 const FLAC__Frame* frame,
                                                                 const FLAC__int32* const buffer[],
                                                                 void* client_data);
    friend void libflac_metadata_callback(const FLAC__StreamDecoder* decoder,
                                          const FLAC__StreamMetadata* metadata, void* client_data);
    friend void libflac_error_callback(const FLAC__StreamDecoder* decoder,
                                       FLAC__StreamDecoderErrorStatus status, void* client_data);
};

struct FLACEncoder : public audio_encoder
{
public:
    FLACEncoder(flac_encoding_options options) : options(std::move(options)) {}
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path,
                                                       audio_decoder* copyMetadataFrom = nullptr) override;
    [[nodiscard]] expected<void, audiofile_error> prepare(const audiofile_metadata& info) override;
    [[nodiscard]] expected<void, audiofile_error> write(const audio_data& data) override;
    ~FLACEncoder();

    [[nodiscard]] expected<void, audiofile_error> close() override;

protected:
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
    std::unique_ptr<FLAC__StreamEncoder, libflac_deleter> encoder;
    bool flacError       = false;
    bool prepared        = false;
    intmax_t totalLength = 0;
    flac_encoding_options options;
};

namespace details
{
// sample ranges for bit depths 1..32
static const fbase sample_ranges[] = {
    NAN,
    NAN,
    1,
    3,
    7,
    15,
    31,
    63,
    127,
    255,
    511,
    1023,
    2047,
    4095,
    8191,
    16383,
    32767,
    65535,
    131071,
    262143,
    524287,
    1048575,
    2097151,
    4194303,
    8388607,
    16777215,
    fbase(33554431),
    fbase(67108863),
    fbase(134217727),
    fbase(268435455),
    fbase(536870911),
    fbase(1073741823),
    fbase(2147483647),
};

inline void cvt_sample(int32_t& sample, fbase value, const audio_quantinization& quant, int bitDepth)
{
    sample =
        std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * sample_ranges[bitDepth]);
}
inline void cvt_sample(fbase& value, int32_t sample, int bitDepth)
{
    value = sample / sample_ranges[bitDepth];
}
} // namespace details

/// @brief Interleaves and converts audio samples
inline void interleaveSamplesVar(int32_t* out, const fbase* const in[], size_t channels, size_t size,
                                 const audio_quantinization& quantinization, int bitDepth)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            details::cvt_sample(out[i * channels + ch], in[ch][i], quantinization, bitDepth);
    }
}

inline void deinterleaveSamplesVar(fbase* const out[], const int32_t* in, size_t channels, size_t size,
                                   int bitDepth)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            details::cvt_sample(out[ch][i], in[i * channels + ch], bitDepth);
    }
}

// inline bool supportsOggFlac = FLAC_API_SUPPORTS_OGG_FLAC;

FLAC__StreamDecoderWriteStatus libflac_write_callback(const FLAC__StreamDecoder* decoder,
                                                      const FLAC__Frame* frame,
                                                      const FLAC__int32* const buffer[], void* client_data)
{
    FLACDecoder* instance = reinterpret_cast<FLACDecoder*>(client_data);
    if (!instance->m_metadata)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.channels != instance->m_metadata->channels)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.sample_rate != instance->m_metadata->sample_rate)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.bits_per_sample <= 0 || frame->header.bits_per_sample > 32)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    const size_t chanNum      = frame->header.channels;
    const size_t sampleNum    = frame->header.blocksize;
    const size_t writePos     = instance->audio.size;
    const int bits_per_sample = frame->header.bits_per_sample;

    // allocate audio data
    instance->audio.resize(writePos + sampleNum);

    for (size_t c = 0; c < chanNum; c++)
    {
        deinterleaveSamplesVar(instance->audio.pointers() + c, buffer[c], 1, sampleNum, bits_per_sample);
    }

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void libflac_metadata_callback(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata,
                               void* client_data)
{
    FLACDecoder* instance = reinterpret_cast<FLACDecoder*>(client_data);
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        audiofile_metadata info;
        info.container           = audiofile_container::flac;
        info.channels            = metadata->data.stream_info.channels;
        info.sample_rate         = metadata->data.stream_info.sample_rate;
        info.total_frames        = metadata->data.stream_info.total_samples;
        info.bit_depth           = metadata->data.stream_info.bits_per_sample;
        info.codec               = audiofile_codec::flac;
        instance->m_metadata     = info;
        instance->audio.metadata = &*instance->m_metadata;
    }
    if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
    {
        for (uint32_t i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
        {
            const FLAC__StreamMetadata_VorbisComment_Entry& entry = metadata->data.vorbis_comment.comments[i];
            std::string_view comment{ reinterpret_cast<const char*>(entry.entry), entry.length };
            size_t eq = comment.find('=');
            if (eq != std::string_view::npos)
            {
                std::string key{ comment.substr(0, eq) };
                std::string value{ comment.substr(eq + 1) };
                instance->m_metadata->metadata.emplace(std::move(key), std::move(value));
            }
        }
    }
}
void libflac_error_callback(const FLAC__StreamDecoder* decoder, FLAC__StreamDecoderErrorStatus status,
                            void* client_data)
{
    FLACDecoder* instance = reinterpret_cast<FLACDecoder*>(client_data);
    instance->flacError   = true;
}

expected<audiofile_metadata, audiofile_error> FLACDecoder::open(const file_path& path)
{
    decoder.reset(FLAC__stream_decoder_new());
    if (!decoder)
        return unexpected(audiofile_error::internal_error);

    if (options.read_metadata)
    {
        FLAC__stream_decoder_set_metadata_respond(decoder.get(), FLAC__METADATA_TYPE_VORBIS_COMMENT);
    }

    auto f = fopen_path(path, open_file_mode::read_existing);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);

    FLAC__StreamDecoderInitStatus init_status =
        FLAC__stream_decoder_init_FILE(decoder.get(), file.get(), &libflac_write_callback,
                                       &libflac_metadata_callback, &libflac_error_callback, this);
    if (init_status == FLAC__STREAM_DECODER_INIT_STATUS_UNSUPPORTED_CONTAINER)
        return unexpected(audiofile_error::format_error);
    else if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
        return unexpected(audiofile_error::internal_error);

    FLAC__bool success = FLAC__stream_decoder_process_until_end_of_metadata(decoder.get());
    if (!success)
        return unexpected(audiofile_error::format_error);

    FLAC__stream_decoder_get_decode_position(decoder.get(), &dataOffset);

    if (!m_metadata)
    {
        // can't parse STREAM INFO
        return unexpected(audiofile_error::format_error);
    }
    if (flacError)
        return unexpected(audiofile_error::format_error);
    if (!m_metadata->valid())
    {
        return unexpected(audiofile_error::format_error);
    }

    return *m_metadata;
}
expected<audio_data, audiofile_error> FLACDecoder::read()
{
    if (!decoder)
        return unexpected(audiofile_error::internal_error);
    if (!m_metadata)
        return unexpected(audiofile_error::internal_error);
    audio.metadata = &*m_metadata;

    if (!audio.size)
    {
        if (FLAC__stream_decoder_get_state(decoder.get()) == FLAC__STREAM_DECODER_END_OF_STREAM)
            return unexpected(audiofile_error::end_of_file);
        FLAC__bool success = FLAC__stream_decoder_process_single(decoder.get());
        if (!success || flacError)
            return unexpected(audiofile_error::format_error);
    }

    // return data in buffer if exists
    if (audio.size)
    {
        audio_data result;
        result.metadata = &*m_metadata;
        result.swap(audio);
        return result;
    }
    return unexpected(audiofile_error::end_of_file);
}
#if 0
expected<SHA256Hash, AudioError> FLACDecoder::dataChunkHash()
{
    if (!decoder)
        return unexpected(AudioError::InternalError);
    if (!m_info)
        return unexpected(AudioError::InternalError);

    if (dataOffset == 0)
    {
        return {};
    }
    // Save file seek position
    long long savedPos = IO_TELL_64(file.get());

    // Seek to data start
    IO_SEEK_64(file.get(), dataOffset, SEEK_SET);

    SHA256Hasher hash;
    uint8_t buffer[4096];

    // read FLAC till end
    for (;;)
    {
        size_t sz = fread(buffer, 1, sizeof(buffer), file.get());
        if (sz == 0)
            break;
        // Update hash
        hash.write(buffer, sz);
    }

    SHA256Hash digest;
    hash.finish(digest);

    // Restore file seek position
    IO_SEEK_64(file.get(), savedPos, SEEK_SET);
    return digest;
}
#endif

expected<void, audiofile_error> FLACDecoder::seek(uint64_t position)
{
    if (!decoder)
        return unexpected(audiofile_error::internal_error);
    if (!m_metadata)
        return unexpected(audiofile_error::internal_error);
    if (position > m_metadata->total_frames)
        return unexpected(audiofile_error::end_of_file);

    audio.clear();

    FLAC__bool success = FLAC__stream_decoder_seek_absolute(decoder.get(), position);
    if (!success)
        return unexpected(audiofile_error::format_error);
    return {};
}

void FLACDecoder::close()
{
    audio.reset();
    decoder.reset();
    file.reset();
    flacError  = false;
    dataOffset = 0;
    m_metadata.reset();
}

expected<void, audiofile_error> FLACEncoder::open(const file_path& path, audio_decoder* copyMetadataFrom)
{
    auto f = fopen_path(path, open_file_mode::write_new);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);

    encoder.reset(FLAC__stream_encoder_new());
    if (!encoder)
        return unexpected(audiofile_error::internal_error);

    return {};
}

expected<void, audiofile_error> FLACEncoder::prepare(const audiofile_metadata& metadata)
{
    prepared = true;
    if (metadata.codec != audiofile_codec::flac || !metadata.valid())
    {
        return unexpected(audiofile_error::format_error);
    }

    if (!FLAC__stream_encoder_set_sample_rate(encoder.get(), metadata.sample_rate))
    {
        return unexpected(audiofile_error::format_error);
    }
    if (!FLAC__stream_encoder_set_bits_per_sample(encoder.get(), metadata.bit_depth))
    {
        return unexpected(audiofile_error::format_error);
    }
    if (!FLAC__stream_encoder_set_channels(encoder.get(), metadata.channels))
    {
        return unexpected(audiofile_error::format_error);
    }

    FLAC__StreamEncoderInitStatus init_status =
        FLAC__stream_encoder_init_FILE(encoder.get(), file.get(), nullptr, this);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
        return unexpected(audiofile_error::internal_error);

    return {};
}
expected<void, audiofile_error> FLACEncoder::write(const audio_data& data)
{
    const audiofile_metadata* meta = data.typed_metadata<audiofile_metadata>();
    KFR_ASSERT(meta);
    if (!prepared)
    {
        auto e = prepare(*meta);
        if (!e)
            return e;
    }
    const size_t length = data.size;
    if (length > 0)
    {
        kfr::univector<int32_t> interleaved(length * meta->channels);
        interleaveSamplesVar(interleaved.data(), data.pointers(), meta->channels, length,
                             audio_quantinization(meta->bit_depth, options.dithering), meta->bit_depth);

        totalLength += length;
        // LOG_INFO(format, "FLAC length = {} totalLength = {}", length, totalLength);

        if (!FLAC__stream_encoder_process_interleaved(encoder.get(), interleaved.data(), length))
        {
            return unexpected(audiofile_error::io_error);
        }
    }

    return {};
}
expected<void, audiofile_error> FLACEncoder::close()
{
    if (encoder)
    {
        FLAC__stream_encoder_finish(encoder.get());
        encoder.reset();
    }
    file.reset();
    flacError   = false;
    prepared    = false;
    totalLength = 0;
    return {};
}

FLACEncoder::~FLACEncoder() { std::ignore = close(); }

std::unique_ptr<audio_decoder> create_flac_decoder(const flac_decoding_options& options)
{
    return std::unique_ptr<audio_decoder>(new FLACDecoder(options));
}
std::unique_ptr<audio_encoder> create_flac_encoder(const flac_encoding_options& options)
{
    return std::unique_ptr<audio_encoder>(new FLACEncoder(options));
}

} // namespace kfr
#endif
