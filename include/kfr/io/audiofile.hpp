/** @addtogroup audio_io
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

#include "../base/basic_expressions.hpp"
#include "../base/conversion.hpp"
#include "../base/univector.hpp"
#include "../cometa/ctti.hpp"
#include "../simd/vec.hpp"
#include "file.hpp"

namespace kfr
{

struct audio_format
{
    size_t channels        = 2;
    audio_sample_type type = audio_sample_type::i16;
    fmax samplerate        = 44100;
    bool use_w64           = false;
};

struct audio_format_and_length : audio_format
{
    using audio_format::audio_format;
    constexpr audio_format_and_length() CMT_NOEXCEPT {}
    constexpr audio_format_and_length(const audio_format& fmt) : audio_format(fmt) {}

    imax length = 0; // in samples
};

template <typename T>
struct audio_reader : public abstract_reader<T>
{
    /// @brief Reads interleaved audio
    using abstract_reader<T>::read;

    univector2d<T> read_channels() { return read_channels(format().length); }

    univector2d<T> read_channels(size_t size)
    {
        univector<T> interleaved = read(size * format().channels);
        univector2d<T> input_channels(format().channels,
                                      univector<T>(interleaved.size() / format().channels));
        deinterleave(input_channels, interleaved);
        return input_channels;
    }

    /// @brief Returns audio format description
    virtual const audio_format_and_length& format() const = 0;
};

template <typename T>
struct audio_writer : public abstract_writer<T>
{
    /// @brief Writes interleaved audio
    using abstract_writer<T>::write;

    template <univector_tag Tag1, univector_tag Tag2>
    size_t write_channels(const univector2d<T, Tag1, Tag2>& data)
    {
        const univector<T> interleaved = interleave(data);
        return write(interleaved) / format().channels;
    }

    /// @brief Returns audio format description
    virtual const audio_format_and_length& format() const = 0;

    /// @brief Finishes writing and closes underlying writer
    virtual void close() = 0;
};

#ifndef KFR_DISABLE_WAV

namespace internal_generic
{
struct wav_file;
struct wav_file_deleter
{
    void operator()(wav_file*);
};
} // namespace internal_generic

/// @brief WAV format writer
template <typename T>
struct audio_writer_wav : audio_writer<T>
{
    /// @brief Constructs WAV writer using target writer and format
    audio_writer_wav(std::shared_ptr<abstract_writer<>>&& writer, const audio_format& fmt);
    ~audio_writer_wav() override;

    using audio_writer<T>::write;

    /// @brief Write data to underlying binary writer
    /// data is PCM samples in interleaved format
    /// size is the number of samples (PCM frames * channels)
    size_t write(const T* data, size_t size) override;

    void close() override;

    const audio_format_and_length& format() const override { return fmt; }

    imax tell() const override { return fmt.length; }

    bool seek(imax, seek_origin) override { return false; }

private:
    std::shared_ptr<abstract_writer<>> writer;
    std::unique_ptr<internal_generic::wav_file, internal_generic::wav_file_deleter> f;
    audio_format_and_length fmt;
};

extern template struct audio_writer_wav<i16>;
extern template struct audio_writer_wav<i24>;
extern template struct audio_writer_wav<i32>;
extern template struct audio_writer_wav<f32>;
extern template struct audio_writer_wav<f64>;

/// @brief WAV format reader
template <typename T>
struct audio_reader_wav : audio_reader<T>
{
    using audio_reader<T>::read;

    /// @brief Constructs WAV reader
    audio_reader_wav(std::shared_ptr<abstract_reader<>>&& reader);
    ~audio_reader_wav() override;

    /// @brief Reads and decodes audio data
    size_t read(T* data, size_t size) override;

    /// @brief Seeks to specific sample
    bool seek(imax offset, seek_origin origin) override;

    /// @brief Returns audio format description
    const audio_format_and_length& format() const override { return fmt; }

    /// @brief Returns current position
    imax tell() const override { return position; }

private:
    std::shared_ptr<abstract_reader<>> reader;
    std::unique_ptr<internal_generic::wav_file> f;
    audio_format_and_length fmt;
    imax position = 0;
};

extern template struct audio_reader_wav<i16>;
extern template struct audio_reader_wav<i24>;
extern template struct audio_reader_wav<i32>;
extern template struct audio_reader_wav<f32>;
extern template struct audio_reader_wav<f64>;
#endif

#ifndef KFR_DISABLE_FLAC

namespace internal_generic
{
struct flac_file;
struct flac_file_deleter
{
    void operator()(flac_file*);
};
} // namespace internal_generic

/// @brief FLAC format reader
template <typename T>
struct audio_reader_flac : audio_reader<T>
{
    /// @brief Constructs FLAC reader
    audio_reader_flac(std::shared_ptr<abstract_reader<>>&& reader);
    ~audio_reader_flac() override;

    /// @brief Reads and decodes audio data
    size_t read(T* data, size_t size) override;

    /// @brief Seeks to specific sample
    bool seek(imax offset, seek_origin origin) override;

    /// @brief Returns audio format description
    const audio_format_and_length& format() const override { return fmt; }

    /// @brief Returns current position
    imax tell() const override { return position; }

private:
    std::shared_ptr<abstract_reader<>> reader;
    std::unique_ptr<internal_generic::flac_file, internal_generic::flac_file_deleter> f;
    audio_format_and_length fmt;
    imax position = 0;
};

extern template struct audio_reader_flac<i16>;
extern template struct audio_reader_flac<i24>;
extern template struct audio_reader_flac<i32>;
extern template struct audio_reader_flac<f32>;
extern template struct audio_reader_flac<f64>;
#endif

#ifndef KFR_DISABLE_MP3

struct mp3_config
{
    uint32_t outputChannels;
    uint32_t outputSampleRate;
};

namespace internal_generic
{
struct mp3_file;
struct mp3_file_deleter
{
    void operator()(mp3_file*);
};
} // namespace internal_generic

/// @brief MP3 format reader
template <typename T>
struct audio_reader_mp3 : audio_reader<T>
{
    /// @brief Constructs MP3 reader
    audio_reader_mp3(std::shared_ptr<abstract_reader<>>&& reader);
    ~audio_reader_mp3() override;

    /// @brief Reads and decodes audio data
    size_t read(T* data, size_t size) override;

    /// @brief Seeks to specific sample
    bool seek(imax offset, seek_origin origin) override;

    mp3_config config{ 0, 0 };

    /// @brief Returns audio format description
    const audio_format_and_length& format() const override { return fmt; }

    /// @brief Returns current position
    imax tell() const override { return position; }

private:
    std::shared_ptr<abstract_reader<>> reader;
    std::unique_ptr<internal_generic::mp3_file, internal_generic::mp3_file_deleter> f;
    audio_format_and_length fmt;
    imax position = 0;
};

extern template struct audio_reader_mp3<i16>;
extern template struct audio_reader_mp3<i24>;
extern template struct audio_reader_mp3<i32>;
extern template struct audio_reader_mp3<f32>;
extern template struct audio_reader_mp3<f64>;
#endif

} // namespace kfr
