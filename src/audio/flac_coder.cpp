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
    [[nodiscard]] expected<audiofile_format, audiofile_error> open(const file_path& path) override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

protected:
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
    std::unique_ptr<FLAC__StreamDecoder, libflac_deleter> decoder;
    audio_data_interleaved packet;
    audio_data_interleaved buffer;
    bool flacError          = false;
    FLAC__uint64 dataOffset = 0;
    flac_decoding_options options;
    expected<audio_data_interleaved, audiofile_error> read_packet();
    [[nodiscard]] expected<size_t, audiofile_error> read_to(const audio_data_interleaved& output) override
    {
        return read_buffered(output, [this]() { return read_packet(); }, buffer);
    }

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
    [[nodiscard]] expected<void, audiofile_error> open(const file_path& path, const audiofile_format& format,
                                                       audio_decoder* copyMetadataFrom = nullptr) override;
    [[nodiscard]] expected<void, audiofile_error> write(const audio_data_interleaved& data) override;
    ~FLACEncoder();

    [[nodiscard]] expected<uint64_t, audiofile_error> close() override;

protected:
    std::unique_ptr<std::FILE, details::stdFILE_deleter> file;
    std::unique_ptr<FLAC__StreamEncoder, libflac_deleter> encoder;
    bool flacError = false;
    flac_encoding_options options;
};

namespace details
{
// sample ranges for bit depths 1..32
static constexpr fbase sample_ranges[] = {
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

inline void cvt_sample(int32_t& sample, fbase value, const audio_quantization& quant, int bitDepth)
{
    sample =
        std::llround(std::clamp(value + quant.dither(), fbase(-1.0), fbase(+1.0)) * sample_ranges[bitDepth]);
}
inline void cvt_sample(fbase& value, int32_t sample, int bitDepth)
{
    value = sample / sample_ranges[bitDepth];
}
} // namespace details

inline void samples_store_flac(FLAC__int32* out, const fbase* in, size_t size,
                               const audio_quantization& quantization, int bitDepth)
{
    for (size_t i = 0; i < size; ++i)
    {
        details::cvt_sample(out[i], in[i], quantization, bitDepth);
    }
}

inline void samples_load_flac(fbase* out, const FLAC__int32* const in[], size_t channels, size_t size,
                              int bitDepth)
{
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t ch = 0; ch < channels; ++ch)
            details::cvt_sample(out[i * channels + ch], in[ch][i], bitDepth);
    }
}

// inline bool supportsOggFlac = FLAC_API_SUPPORTS_OGG_FLAC;

FLAC__StreamDecoderWriteStatus libflac_write_callback(const FLAC__StreamDecoder* decoder,
                                                      const FLAC__Frame* frame,
                                                      const FLAC__int32* const buffer[], void* client_data)
{
    FLACDecoder* instance = reinterpret_cast<FLACDecoder*>(client_data);
    if (!instance->m_format)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.channels != instance->m_format->channels)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.sample_rate != instance->m_format->sample_rate)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.bits_per_sample <= 0 || frame->header.bits_per_sample > 32)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    const size_t chanNum      = frame->header.channels;
    const size_t sampleNum    = frame->header.blocksize;
    const size_t writePos     = instance->packet.size;
    const int bits_per_sample = frame->header.bits_per_sample;

    if (instance->packet.empty())
    {
        instance->packet = audio_data_interleaved(chanNum);
    }

    // allocate audio data
    instance->packet.resize(writePos + sampleNum);

    samples_load_flac(instance->packet.data + writePos * chanNum, buffer, chanNum, sampleNum,
                      bits_per_sample);

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void libflac_metadata_callback(const FLAC__StreamDecoder* decoder, const FLAC__StreamMetadata* metadata,
                               void* client_data)
{
    FLACDecoder* instance = reinterpret_cast<FLACDecoder*>(client_data);
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
    {
        audiofile_format info;
        info.container     = audiofile_container::flac;
        info.channels      = metadata->data.stream_info.channels;
        info.sample_rate   = metadata->data.stream_info.sample_rate;
        info.total_frames  = metadata->data.stream_info.total_samples;
        info.bit_depth     = metadata->data.stream_info.bits_per_sample;
        info.codec         = audiofile_codec::flac;
        instance->m_format = info;
    }
    if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
    {
        KFR_ASSERT(instance->m_format);
        for (uint32_t i = 0; i < metadata->data.vorbis_comment.num_comments; i++)
        {
            const FLAC__StreamMetadata_VorbisComment_Entry& entry = metadata->data.vorbis_comment.comments[i];
            std::string_view comment{ reinterpret_cast<const char*>(entry.entry), entry.length };
            size_t eq = comment.find('=');
            if (eq != std::string_view::npos)
            {
                std::string key{ comment.substr(0, eq) };
                std::string value{ comment.substr(eq + 1) };
                instance->m_format->metadata.emplace(std::move(key), std::move(value));
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

expected<audiofile_format, audiofile_error> FLACDecoder::open(const file_path& path)
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

    if (!m_format)
    {
        // can't parse STREAM INFO
        return unexpected(audiofile_error::format_error);
    }
    if (flacError)
        return unexpected(audiofile_error::format_error);
    if (!m_format->valid())
    {
        return unexpected(audiofile_error::format_error);
    }

    return *m_format;
}
expected<audio_data_interleaved, audiofile_error> FLACDecoder::read_packet()
{
    if (!m_format)
        return unexpected(audiofile_error::closed);
    if (!decoder)
        return unexpected(audiofile_error::internal_error);

    if (FLAC__stream_decoder_get_state(decoder.get()) == FLAC__STREAM_DECODER_END_OF_STREAM)
        return unexpected(audiofile_error::end_of_file);
    FLAC__bool success = FLAC__stream_decoder_process_single(decoder.get());
    if (!success || flacError)
        return unexpected(audiofile_error::format_error);

    audio_data_interleaved result(m_format->channels);
    result.swap(packet);
    if (result.empty())
    {
        if (FLAC__stream_decoder_get_state(decoder.get()) == FLAC__STREAM_DECODER_END_OF_STREAM)
            return unexpected(audiofile_error::end_of_file);
    }
    return result;
}

expected<void, audiofile_error> FLACDecoder::seek(uint64_t position)
{
    if (!decoder)
        return unexpected(audiofile_error::internal_error);
    if (!m_format)
        return unexpected(audiofile_error::closed);
    if (position > m_format->total_frames)
        return unexpected(audiofile_error::end_of_file);

    buffer.reset();
    packet.clear();

    FLAC__bool success = FLAC__stream_decoder_seek_absolute(decoder.get(), position);
    if (!success)
        return unexpected(audiofile_error::format_error);
    return {};
}

void FLACDecoder::close()
{
    packet.reset();
    decoder.reset();
    file.reset();
    flacError  = false;
    dataOffset = 0;
    m_format.reset();
}

expected<void, audiofile_error> FLACEncoder::open(const file_path& path, const audiofile_format& format,
                                                  audio_decoder* copyMetadataFrom)
{
    auto f = fopen_path(path, open_file_mode::write_new);
    if (f)
        file.reset(*f);
    else
        return unexpected(audiofile_error::io_error);

    encoder.reset(FLAC__stream_encoder_new());
    if (!encoder)
        return unexpected(audiofile_error::internal_error);

    m_format = format;

    if (format.codec != audiofile_codec::flac || !format.valid())
    {
        return unexpected(audiofile_error::format_error);
    }

    if (!FLAC__stream_encoder_set_sample_rate(encoder.get(), format.sample_rate))
    {
        return unexpected(audiofile_error::format_error);
    }
    if (!FLAC__stream_encoder_set_bits_per_sample(encoder.get(), format.bit_depth))
    {
        return unexpected(audiofile_error::format_error);
    }
    if (!FLAC__stream_encoder_set_channels(encoder.get(), format.channels))
    {
        return unexpected(audiofile_error::format_error);
    }

    FLAC__StreamEncoderInitStatus init_status =
        FLAC__stream_encoder_init_FILE(encoder.get(), file.get(), nullptr, this);
    if (init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
        return unexpected(audiofile_error::internal_error);

    return {};
}

expected<void, audiofile_error> FLACEncoder::write(const audio_data_interleaved& data)
{
    if (!m_format)
        return unexpected(audiofile_error::closed);
    if (data.channels != m_format->channels)
        return unexpected(audiofile_error::invalid_argument);

    const size_t length = data.size;
    if (length > 0)
    {
        kfr::univector<int32_t> interleaved(length * data.channels);

        samples_store_flac(interleaved.data(), data.data, data.total_samples(),
                           audio_quantization(m_format->bit_depth, options.dithering), m_format->bit_depth);

        m_format->total_frames += length;

        if (!FLAC__stream_encoder_process_interleaved(encoder.get(), interleaved.data(), length))
        {
            return unexpected(audiofile_error::io_error);
        }
    }

    return {};
}
expected<uint64_t, audiofile_error> FLACEncoder::close()
{
    bool ok = true;
    if (encoder)
    {
        ok = FLAC__stream_encoder_finish(encoder.get());
        encoder.reset();
    }
    file.reset();
    flacError             = false;
    uint64_t totalWritten = m_format->total_frames;
    m_format.reset();
    return totalWritten;
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
