/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

#ifdef KFR_OS_APPLE

// This file implements an audio decoder using the Apple Core Audio API.

#include <AudioToolbox/AudioToolbox.h>

#include <algorithm>
#include <limits>

namespace kfr
{
namespace
{
audiofile_error map_osstatus_to_error(OSStatus status)
{
    switch (status)
    {
    case kAudioFileEndOfFileError:
        return audiofile_error::end_of_file;
    case kAudioFileFileNotFoundError:
        return audiofile_error::not_found;
    case kAudioFilePermissionsError:
        return audiofile_error::access_denied;
    case kAudioFileInvalidFileError:
    case kAudioFileUnsupportedFileTypeError:
    case kAudioFileUnsupportedDataFormatError:
        return audiofile_error::format_error;
    default:
        return audiofile_error::io_error;
    }
}

struct coreaudio_decoder final : audio_decoder
{
    explicit coreaudio_decoder(coreaudio_decoding_options options) : m_options(std::move(options)) {}
    ~coreaudio_decoder() override { close(); }

    [[nodiscard]] expected<audiofile_format, audiofile_error> open(
        std::shared_ptr<binary_reader> reader) override;
    [[nodiscard]] expected<size_t, audiofile_error> read_to(const audio_data_interleaved& output) override;
    [[nodiscard]] expected<void, audiofile_error> seek(uint64_t position) override;
    void close() override;

private:
    static OSStatus read_callback(void* client_data, SInt64 position, UInt32 request_count, void* buffer,
                                  UInt32* actual_count);
    static SInt64 size_callback(void* client_data);

    [[maybe_unused]] coreaudio_decoding_options m_options;
    AudioFileID m_audio_file        = nullptr;
    ExtAudioFileRef m_extended_file = nullptr;
};

OSStatus coreaudio_decoder::read_callback(void* client_data, SInt64 position, UInt32 request_count,
                                          void* buffer, UInt32* actual_count)
{
    auto* decoder = static_cast<coreaudio_decoder*>(client_data);
    if (!decoder || !decoder->m_reader || position < 0 || !actual_count)
        return kAudioFileInvalidFileError;

    if (!decoder->m_reader->seek(position, seek_origin::begin))
        return kAudioFileInvalidFileError;

    *actual_count = static_cast<UInt32>(decoder->m_reader->read(buffer, request_count));
    return noErr;
}

SInt64 coreaudio_decoder::size_callback(void* client_data)
{
    auto* decoder = static_cast<coreaudio_decoder*>(client_data);
    if (!decoder || !decoder->m_reader)
        return 0;
    if (auto size = decoder->m_reader->size())
        return static_cast<SInt64>(*size);
    return 0;
}

expected<audiofile_format, audiofile_error> coreaudio_decoder::open(std::shared_ptr<binary_reader> reader)
{
    close();
    if (!reader)
        return unexpected(audiofile_error::invalid_argument);
    if (!reader->size())
        return unexpected(audiofile_error::io_error);

    m_reader = std::move(reader);

    OSStatus status = AudioFileOpenWithCallbacks(this, &read_callback, nullptr, &size_callback, nullptr, 0,
                                                  &m_audio_file);
    if (status != noErr)
    {
        close();
        return unexpected(map_osstatus_to_error(status));
    }

    status = ExtAudioFileWrapAudioFileID(m_audio_file, false, &m_extended_file);
    if (status != noErr)
    {
        close();
        return unexpected(map_osstatus_to_error(status));
    }

    AudioStreamBasicDescription source_format{};
    UInt32 property_size = sizeof(source_format);
    status = ExtAudioFileGetProperty(m_extended_file, kExtAudioFileProperty_FileDataFormat, &property_size,
                                     &source_format);
    if (status != noErr || source_format.mChannelsPerFrame == 0 || source_format.mSampleRate <= 0 ||
        source_format.mChannelsPerFrame > max_audio_channels)
    {
        close();
        return unexpected(status == noErr ? audiofile_error::format_error : map_osstatus_to_error(status));
    }

    AudioStreamBasicDescription client_format{};
    client_format.mSampleRate       = source_format.mSampleRate;
    client_format.mFormatID         = kAudioFormatLinearPCM;
    client_format.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    client_format.mBytesPerPacket   = sizeof(fbase) * source_format.mChannelsPerFrame;
    client_format.mFramesPerPacket  = 1;
    client_format.mBytesPerFrame    = sizeof(fbase) * source_format.mChannelsPerFrame;
    client_format.mChannelsPerFrame = source_format.mChannelsPerFrame;
    client_format.mBitsPerChannel   = sizeof(fbase) * 8;

    property_size = sizeof(client_format);
    status        = ExtAudioFileSetProperty(m_extended_file, kExtAudioFileProperty_ClientDataFormat,
                                     property_size, &client_format);
    if (status != noErr)
    {
        close();
        return unexpected(map_osstatus_to_error(status));
    }

    SInt64 total_frames = 0;
    property_size       = sizeof(total_frames);
    status = ExtAudioFileGetProperty(m_extended_file, kExtAudioFileProperty_FileLengthFrames, &property_size,
                                     &total_frames);
    if (status != noErr)
    {
        close();
        return unexpected(map_osstatus_to_error(status));
    }

    audiofile_format format;
    format.container    = audiofile_container::unknown;
    format.codec        = audiofile_codec::unknown;
    format.endianness   = audiofile_endianness::little;
    format.bit_depth    = source_format.mBitsPerChannel;
    format.channels     = source_format.mChannelsPerFrame;
    format.sample_rate  = static_cast<uint32_t>(source_format.mSampleRate);
    format.total_frames = std::max<SInt64>(total_frames, 0);
    if (!format.valid())
    {
        close();
        return unexpected(audiofile_error::format_error);
    }

    m_format = format;
    return format;
}

expected<size_t, audiofile_error> coreaudio_decoder::read_to(const audio_data_interleaved& output)
{
    if (!m_extended_file || !m_format)
        return unexpected(audiofile_error::closed);
    if (output.size == 0 || output.channels != m_format->channels)
        return unexpected(audiofile_error::invalid_argument);

    const size_t frames_to_read = std::min(output.size, static_cast<size_t>(std::numeric_limits<UInt32>::max()));
    UInt32 frames_read          = static_cast<UInt32>(frames_to_read);
    AudioBufferList buffer_list{};
    buffer_list.mNumberBuffers              = 1;
    buffer_list.mBuffers[0].mNumberChannels = m_format->channels;
    buffer_list.mBuffers[0].mDataByteSize = static_cast<UInt32>(frames_to_read * m_format->channels * sizeof(fbase));
    buffer_list.mBuffers[0].mData         = output.data;

    const OSStatus status = ExtAudioFileRead(m_extended_file, &frames_read, &buffer_list);
    if (status != noErr)
        return unexpected(map_osstatus_to_error(status));
    if (frames_read == 0)
        return unexpected(audiofile_error::end_of_file);
    return frames_read;
}

expected<void, audiofile_error> coreaudio_decoder::seek(uint64_t position)
{
    if (!m_extended_file || !m_format)
        return unexpected(audiofile_error::closed);
    if (position > static_cast<uint64_t>(std::numeric_limits<SInt64>::max()))
        return unexpected(audiofile_error::invalid_argument);

    const OSStatus status = ExtAudioFileSeek(m_extended_file, static_cast<SInt64>(position));
    if (status != noErr)
        return unexpected(map_osstatus_to_error(status));
    return {};
}

void coreaudio_decoder::close()
{
    if (m_extended_file)
    {
        ExtAudioFileDispose(m_extended_file);
        m_extended_file = nullptr;
    }
    if (m_audio_file)
    {
        AudioFileClose(m_audio_file);
        m_audio_file = nullptr;
    }
    m_reader.reset();
    m_format.reset();
}
} // namespace

std::unique_ptr<audio_decoder> create_coreaudio_decoder(const coreaudio_decoding_options& options)
{
    return std::make_unique<coreaudio_decoder>(options);
}
} // namespace kfr

#endif
