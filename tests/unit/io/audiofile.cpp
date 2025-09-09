/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <thread>

#include <kfr/io/audiofile.hpp>
#include <kfr/test/test.hpp>
#include <kfr/base.hpp>

namespace kfr
{

#ifndef KFR_DISABLE_WAV
TEST_CASE("readwrite_wav_file")
{
    {
        audio_writer_wav<float> writer(open_file_for_writing(KFR_FILEPATH("temp_audio_file.wav")),
                                       audio_format{});
        univector<float> data(44100 * 2);
        data      = sin(counter() * 0.01f);
        size_t wr = writer.write(data.data(), data.size());
        CHECK(wr == data.size());
        CHECK(umax(writer.format().length) == data.size() / 2);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for file to be closed

    {
        audio_reader_wav<float> reader(open_file_for_reading(KFR_FILEPATH("temp_audio_file.wav")));
        CHECK(reader.format().channels == 2u);
        CHECK(reader.format().type == audio_sample_type::i16);
        CHECK(reader.format().samplerate == 44100);
        univector<float> data(44100 * 2);
        CHECK(umax(reader.format().length) == data.size() / 2);
        size_t rd = reader.read(data.data(), data.size());
        CHECK(rd == data.size());
        CHECK(absmaxof(data - render(sin(counter() * 0.01f), data.size())) < 0.0001f);
    }
}
#endif

#ifndef KFR_DISABLE_FLAC
TEST_CASE("read_flac_file")
{
    audio_reader_flac<float> reader(
        open_file_for_reading(KFR_FILEPATH(KFR_SRC_DIR "/tests/test-audio/sine.flac")));
    CHECK(reader.format().channels == 2u);
    CHECK(reader.format().type == audio_sample_type::i32);
    CHECK(reader.format().samplerate == 44100);
    univector<float> data(44100 * 2);
    CHECK(reader.format().length == data.size() / 2);
    size_t rd = reader.read(data.data(), data.size());
    CHECK(rd == data.size());
    CHECK(absmaxof(data - render(sin(counter() * 0.01f), data.size())) < 0.0001f);
}
#endif

#ifndef KFR_DISABLE_MP3
TEST_CASE("read_mp3_file")
{
    audio_reader_mp3<float> reader(
        open_file_for_reading(KFR_FILEPATH(KFR_SRC_DIR "/tests/test-audio/sine.mp3")));
    CHECK(reader.format().channels == 2u);
    CHECK(reader.format().type == audio_sample_type::i16);
    CHECK(reader.format().samplerate == 44100);
    univector<float> data(44100 * 2);
    CHECK(reader.format().length >= data.size() / 2);
    size_t rd = reader.read(data.data(), data.size());
    CHECK(rd == data.size());
    data = data.slice(2402, 2 * 44100); // MP3 format delay
    CHECK(absmaxof(data - render(sin(counter() * 0.01f), data.size())) < 0.005f);
}
#endif
} // namespace kfr
