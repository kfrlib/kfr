/**
 * KFR (https://www.kfrlib.com)
 * Copyright (C) 2016-2023 Dan Cazarin
 * See LICENSE.txt for details
 */

#include <thread>

#include <kfr/dsp/oscillators.hpp>
#include <kfr/dsp/units.hpp>
#include <kfr/test/test.hpp>
#include <kfr/audio/decoder.hpp>
#include <kfr/audio/encoder.hpp>

namespace Catch
{
// Convert expected<T, E> to string for logging
template <typename T, typename E>
struct StringMaker<tl::expected<T, E>>
{
    static std::string convert(const tl::expected<T, E>& exp)
    {
        if (exp)
            return StringMaker<T>::convert(exp.value());
        else
            return StringMaker<E>::convert(exp.error());
    }
};
template <typename E>
struct StringMaker<tl::expected<void, E>>
{
    static std::string convert(const tl::expected<void, E>& exp)
    {
        if (exp)
            return "(void)";
        else
            return StringMaker<E>::convert(exp.error());
    }
};
// Convert audiofile_error to string for logging
template <>
struct StringMaker<kfr::audiofile_error>
{
    static std::string convert(const kfr::audiofile_error& err) { return kfr::to_string(err); }
};
template <>
struct StringMaker<kfr::audio_sample_type>
{
    static std::string convert(const kfr::audio_sample_type& s)
    {
        if (kfr::audio_sample_is_float(s))
            return "f" + std::to_string(kfr::audio_sample_bit_depth(s));
        else
            return "i" + std::to_string(kfr::audio_sample_bit_depth(s));
    }
};
template <>
struct StringMaker<kfr::audiofile_endianness>
{
    static std::string convert(const kfr::audiofile_endianness& e)
    {
        switch (e)
        {
        case kfr::audiofile_endianness::little:
            return "little";
        case kfr::audiofile_endianness::big:
            return "big";
        default:
            return "unknown";
        }
    }
};
} // namespace Catch

using namespace kfr;

struct ErrorDesc
{
    std::string msg;
    operator bool() const { return false; }
    friend std::ostream& operator<<(std::ostream& os, const ErrorDesc& v)
    {
        os << v.msg;
        return os;
    }
};

static void testAudioEquality(const audio_data_interleaved& test, const audio_data_interleaved& reference);

[[maybe_unused]] static void testRandomReads(const std::unique_ptr<audio_decoder>& decoder,
                                             const audio_data_interleaved& reference)
{
    std::mt19937_64 rnd(12345);
    std::uniform_int_distribution<int64_t> dist(0, reference.size);
    for (size_t i = 0; i < 20; i++)
    {
        int64_t start;
        int64_t end;
        do
        {
            start = dist(rnd);
            end   = dist(rnd);
        } while (std::abs(start - end) < 10 || std::abs(start - end) > 44100);
        if (start > end)
            std::swap(start, end);
        CAPTURE(start);
        CAPTURE(end);
        // fmt::print(stderr, "{}..{}\n", start, end);
        if (auto e = decoder->seek(start); !e)
        {
            CHECK(ErrorDesc{ "Cannot seek due to " + to_string(e.error()) });
            continue;
        }
        audio_data_interleaved data(reference.channels, end - start);
        auto sizeRead = decoder->read_to(data);
        if (!sizeRead)
        {
            CHECK(ErrorDesc{ "Cannot read fragment due to " + to_string(sizeRead.error()) });
            continue;
        }
        data.resize(*sizeRead);
        testAudioEquality(data, reference.slice(start, end - start));
    }
}

static fbase rmsThresholdDefault = kfr::dB_to_amp(-84.0);
static fbase rmsThreshold        = rmsThresholdDefault; // 2.0 / 32768.0;
static fbase rmsThresholdHigh    = kfr::dB_to_amp(-30.0);

fbase fastrmsdiff(const fbase* x, const fbase* y, size_t sz)
{
    fbase sum = 0;
#pragma clang loop vectorize(enable)
    for (size_t i = 0; i < sz; ++i)
    {
        fbase diff = x[i] - y[i];
        sum += diff * diff;
    }
    return std::sqrt(sum / sz);
}

static void testAudioEquality(const audio_data_interleaved& test, const audio_data_interleaved& reference)
{
    CHECK(test.channels == reference.channels);
    CHECK(test.size == reference.size);

    double errRMS =
        fastrmsdiff(test.data, reference.data, std::min(test.total_samples(), reference.total_samples()));
    CHECK(errRMS < rmsThreshold);

    if (errRMS >= rmsThreshold)
    {
        audiofile_format fmt;
        fmt.container   = audiofile_container::wave;
        fmt.codec       = audiofile_codec::lpcm;
        fmt.endianness  = audiofile_endianness::little;
        fmt.bit_depth   = 24;
        fmt.sample_rate = 44100;
        fmt.channels    = test.channels;
        auto enc        = create_wave_encoder();
        std::ignore     = enc->open(std::to_string(std::random_device{}()) + ".wav", fmt);
        std::ignore     = enc->write(test);
        std::ignore     = enc->close();
    }
}

[[maybe_unused]] static void testAudioEqualityCompressed(const audio_data_interleaved& test,
                                                         const audio_data_interleaved& reference)
{
    ptrdiff_t sizeDiff = static_cast<ptrdiff_t>(test.size) - static_cast<ptrdiff_t>(reference.size);
    if (sizeDiff == 0)
    {
        // same size, just compare
        double errRMS =
            fastrmsdiff(test.data, reference.data, std::min(test.total_samples(), reference.total_samples()));
        if (errRMS < rmsThreshold)
        {
            return; // all good
        }
    }
    else if (sizeDiff > 0)
    {
        // test is longer than reference
        double errRMS = fastrmsdiff(test.slice(sizeDiff).data, reference.data, reference.total_samples());
        if (errRMS < rmsThreshold)
        {
            return; // all good
        }
        errRMS = fastrmsdiff(test.truncate(reference.size).data, reference.data, reference.total_samples());
        if (errRMS < rmsThreshold)
        {
            return; // all good
        }
    }
    else
    {
        // reference is longer than test, truncate
        double errRMS = fastrmsdiff(test.data, reference.slice(-sizeDiff).data, test.total_samples());
        if (errRMS < rmsThreshold)
        {
            return; // all good
        }
        errRMS = fastrmsdiff(test.data, reference.truncate(test.size).data, test.total_samples());
        if (errRMS < rmsThreshold)
        {
            return; // all good
        }
    }

    size_t peakTest      = test.find_peak();
    size_t peakReference = reference.find_peak();
    ptrdiff_t peakDiff   = static_cast<ptrdiff_t>(peakTest) - static_cast<ptrdiff_t>(peakReference);
    CHECK(peakDiff >= -2000);
    CHECK(peakDiff <= 2000);

    // Align peaks
    audio_data_interleaved testAligned = test;
    audio_data_interleaved refAligned  = reference;
    if (peakDiff > 0)
    {
        testAligned = testAligned.slice(peakDiff);
    }
    else if (peakDiff < 0)
    {
        refAligned = refAligned.slice(-peakDiff);
    }
    // truncate to the shortest length
    testAligned = testAligned.truncate(std::min(testAligned.size, refAligned.size));
    refAligned  = refAligned.truncate(std::min(testAligned.size, refAligned.size));

    testAudioEquality(testAligned, refAligned);
}

#ifdef KFR_USE_STD_FILESYSTEM

static bool disable_random_reads = false;

static void testFormat(const std::filesystem::path& dir, bool expectedToFail, bool useOSDecoder = false)
{
    for (auto f : std::filesystem::directory_iterator(dir))
    {
        if (!f.is_regular_file())
            continue;
        std::string ext = f.path().extension().string();
        if (ext == ".txt" || ext == ".raw")
            continue;
        println(f.path().string());
        fflush(stdout);
        INFO(f.path().string());

        std::unique_ptr<audio_decoder> decoder;
#ifdef KFR_OS_WIN
        if (useOSDecoder)
            decoder = create_mediafoundation_decoder();
        else
#endif
            decoder = create_decoder_for_file(f.path());
        if (!decoder)
        {
            if (!expectedToFail)
                CHECK(ErrorDesc{ "Cannot detect format" });
            continue;
        }
        auto info = decoder->open(f.path());
        if (!info)
        {
            if (!expectedToFail)
                CHECK(ErrorDesc{ "Cannot read format due to " + to_string(info.error()) });
            continue;
        }
        if (auto e = decoder->seek(0); !e)
        {
            CHECK(ErrorDesc{ "Cannot seek due to " + to_string(e.error()) });
            continue;
        }
        auto audio = decoder->read_all();
        if (!audio)
        {
            CHECK(ErrorDesc{ "Cannot read audio due to " + to_string(audio.error()) });
            continue;
        }

        raw_decoding_options rawOptions;
        rawOptions.raw.channels               = info->channels;
        rawOptions.raw.bit_depth              = 16;
        rawOptions.raw.codec                  = audiofile_codec::lpcm;
        rawOptions.raw.endianness             = audiofile_endianness::little;
        std::unique_ptr<audio_decoder> rawDec = create_raw_decoder(rawOptions);
        auto rawInfo                          = rawDec->open(f.path().string() + ".raw");
        if (!rawInfo)
        {
            CHECK(ErrorDesc{ "Cannot read raw audio due to " + to_string(rawInfo.error()) });
            continue;
        }
        auto rawAudio = rawDec->read_all();
        if (!rawAudio)
        {
            CHECK(ErrorDesc{ "Cannot read raw audio due to " + to_string(rawAudio.error()) });
            continue;
        }

        if (useOSDecoder || decoder->format()->codec == audiofile_codec::mp3 ||
            decoder->format()->codec == audiofile_codec::alac)
        {
            testAudioEqualityCompressed(*audio, *rawAudio);
        }
        else
        {
            testAudioEquality(*audio, *rawAudio);
        }

        if (!disable_random_reads)
            testRandomReads(decoder, *rawAudio);
    }
}

namespace fs = std::filesystem;

TEST_CASE("audio_format_wav")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "wav", false);
}
#if 0
TEST_CASE("audio_format_wav_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "wav" / "unsupported", true);
}
#endif

TEST_CASE("audio_format_flac")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "flac", false);
}
#if 0
TEST_CASE("audio_format_flac_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "flac" / "unsupported", true);
}
#endif

TEST_CASE("audio_format_aiff")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "aiff", false);
}
#if 0
TEST_CASE("audio_format_aiff_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "aiff" / "unsupported", true);
}
#endif

TEST_CASE("audio_format_caf")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "caf", false);
}
#if 0
TEST_CASE("audio_format_caf_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "caf" / "unsupported", true);
}
#endif

TEST_CASE("audio_format_alac")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "alac", false);
}
#if 0
TEST_CASE("audio_format_alac_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "alac" / "unsupported", true);
}
#endif

TEST_CASE("audio_format_mp3")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    // disable_random_reads          = true;
    rmsThreshold = kfr::dB_to_amp(-35.0);
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "mp3", false);
    // disable_random_reads          = false;
    rmsThreshold = rmsThresholdDefault;
}
#if 0
TEST_CASE("audio_format_mp3_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "mp3" / "unsupported", true);
}
#endif

#ifdef KFR_OS_WIN
TEST_CASE("audio_format_mediafoundation")
{
    if (!std::getenv("TEST_IN_DIR"))
        return;
    disable_random_reads = true;
    rmsThreshold         = kfr::dB_to_amp(-35.0);
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "os", false, true);
    disable_random_reads = false;
    rmsThreshold         = rmsThresholdDefault;
}
#if 0
TEST_CASE("audio_format_os_unsupported")
{
    if (!std::getenv("TEST_IN_DIR")) return;
    disable_random_reads = true;
    rmsThreshold         = kfr::dB_to_amp(-35.0);
    testFormat(fs::path(std::getenv("TEST_IN_DIR")) / "format" / "os" / "unsupported", true, true);
    disable_random_reads = false;
    rmsThreshold         = rmsThresholdDefault;
}
#endif
#endif

#endif

static auto data_generator(size_t size, uint32_t ch, double scale)
{
    return scale * truncate(sinenorm(counter() * ((ch + 4) / 300.f)), size);
}

static audio_data_planar generate_test_audio(size_t size, uint32_t channels, double scale)
{
    audio_data_planar data(channels, size);
    for (uint32_t ch = 0; ch < channels; ++ch)
    {
        data.channel(ch) = data_generator(size, ch, scale);
    }
    return data;
}

static void test_audiodata(audio_decoder& decoder, bool allowLengthMismatch = false,
                           double threshold = 0.0001, double scale = dB_to_amp(-3))
{
    REQUIRE(decoder.format().has_value());
    const audiofile_format& r = *decoder.format();
    CHECK(r.channels == 2);
    CHECK(r.sample_rate == 44100);
    if (allowLengthMismatch)
        CHECK(r.total_frames >= 44100);
    else
        CHECK(r.total_frames == 44100);

    auto data = decoder.read_all();
    REQUIRE(data.has_value());
    CHECK(data->size == 44100);

    for (size_t ch = 0; ch < r.channels; ++ch)
    {
        double err = absmaxof(data->channel(ch) - data_generator(44100, ch, scale));
        CHECK(err < threshold);
    }
}

TEST_CASE("wave_decoder")
{
    auto decoder = create_wave_decoder({ { .read_metadata = true } });
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_pcm_s24le.wav");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::wave);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 24);

    test_audiodata(*decoder);

    REQUIRE(r->metadata.find("ISFT") != r->metadata.end());
    CHECK(r->metadata.at("ISFT") == "KFR 7.0.0 debug avx2 64-bit (clang-msvc-20.1.3/windows) +in +ve");
}

TEST_CASE("w64_decoder")
{
    auto decoder = create_w64_decoder({ { .read_metadata = true } });
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.w64");
    REQUIRE(r);
    CHECK(r->container == audiofile_container::w64);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 16);

    test_audiodata(*decoder);
}

TEST_CASE("aiff_decoder")
{
    auto decoder = create_aiff_decoder({ { .read_metadata = true } });
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.aiff");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::aiff);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::big);
    CHECK(r->bit_depth == 16);

    test_audiodata(*decoder);
}

TEST_CASE("raw_decoder: le")
{
    raw_decoding_options info{};
    info.raw.codec       = audiofile_codec::ieee_float;
    info.raw.endianness  = audiofile_endianness::little;
    info.raw.bit_depth   = 32;
    info.raw.channels    = 2;
    info.raw.sample_rate = 44100;

    auto decoder = create_raw_decoder(info);
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c.f32le");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::unknown);
    CHECK(r->codec == audiofile_codec::ieee_float);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 32);

    test_audiodata(*decoder);
}

TEST_CASE("raw_decoder: be24")
{
    raw_decoding_options info{};
    info.raw.codec       = audiofile_codec::lpcm;
    info.raw.endianness  = audiofile_endianness::big;
    info.raw.bit_depth   = 24;
    info.raw.channels    = 2;
    info.raw.sample_rate = 44100;

    auto decoder = create_raw_decoder(info);
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c.s24be");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::unknown);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::big);
    CHECK(r->bit_depth == 24);

    test_audiodata(*decoder);
}

TEST_CASE("raw_encoder: s32")
{
    std::string name = "temp" + std::to_string(std::random_device{}()) + ".raw";

    raw_encoding_options info{};
    info.raw.codec       = audiofile_codec::lpcm;
    info.raw.endianness  = audiofile_endianness::little;
    info.raw.bit_depth   = 32;
    info.raw.channels    = 2;
    info.raw.sample_rate = 44100;

    auto encoder = create_raw_encoder(info);
    REQUIRE(encoder != nullptr);
    auto r = encoder->open(name, {});
    REQUIRE(r);
    // No need to call prepare for raw files

    audiofile_format info2 = info.raw.to_format();
    audio_data data(info2.channels, 44100);
    data.channel(0) = data_generator(data.size, 0, dB_to_amp(-3));
    data.channel(1) = data_generator(data.size, 1, dB_to_amp(-3));

    auto e = encoder->write(data);
    REQUIRE(e);
    auto closed = encoder->close();
    REQUIRE(closed);

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for file to be written

    raw_decoding_options info3{};
    info3.raw    = info.raw;
    auto decoder = create_raw_decoder(info3);
    REQUIRE(decoder != nullptr);
    auto r2 = decoder->open(name);
    REQUIRE(r2);
    CHECK(r2->container == audiofile_container::unknown);
    CHECK(r2->codec == audiofile_codec::lpcm);
    CHECK(r2->endianness == audiofile_endianness::little);
    CHECK(r2->bit_depth == 32);

    test_audiodata(*decoder);
}

#ifdef KFR_AUDIO_FLAC
TEST_CASE("flac_decoder")
{
    auto decoder = create_flac_decoder({ { .read_metadata = true } });
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.flac");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::flac);
    CHECK(r->codec == audiofile_codec::flac);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 16);

    test_audiodata(*decoder);

    REQUIRE(r->metadata.find("encoder") != r->metadata.end());
    CHECK(r->metadata.at("encoder") == "Lavf60.4.101");
}
#endif

#ifdef KFR_AUDIO_MP3
TEST_CASE("mp3_decoder")
{
    auto decoder = create_mp3_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c.mp3");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::mp3);
    CHECK(r->codec == audiofile_codec::mp3);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 0);

    test_audiodata(*decoder, false, 0.01, dB_to_amp(-3.445));
}
#endif

TEST_CASE("caff_decoder")
{
    auto decoder = create_caff_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_f32be.caf");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::caf);
    CHECK(r->codec == audiofile_codec::ieee_float);
    CHECK(r->endianness == audiofile_endianness::big);
    CHECK(r->bit_depth == 32);

    test_audiodata(*decoder);
}

TEST_CASE("caff_decoder 16bit")
{
    auto decoder = create_caff_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.caf");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::caf);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 16);

    test_audiodata(*decoder);
}

#ifdef KFR_AUDIO_ALAC
TEST_CASE("caff_decoder alac s24")
{
    auto decoder = create_caff_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_alac_s24.caf");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::caf);
    CHECK(r->codec == audiofile_codec::alac);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 24);

    test_audiodata(*decoder, true);
}

TEST_CASE("caff_decoder alac s16")
{
    auto decoder = create_caff_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_alac_s16.caf");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::caf);
    CHECK(r->codec == audiofile_codec::alac);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 16);

    test_audiodata(*decoder, true);
}
#endif

#ifdef KFR_OS_WIN
TEST_CASE("mediafoundation_decoder")
{
    auto decoder = create_mediafoundation_decoder({ { .read_metadata = true } });
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_pcm_s24le.wav");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::unknown);
    CHECK(r->codec == audiofile_codec::unknown);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 24);

    test_audiodata(*decoder);
}

TEST_CASE("os_decoder_no_file")
{
    auto decoder = create_mediafoundation_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open("this_file_does_not_exist.wav");
    REQUIRE(!r.has_value());
    CHECK(r.error() == audiofile_error::not_found);
}

TEST_CASE("os_decoder_unsupported")
{
    auto decoder = create_mediafoundation_decoder();
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/not_audio");
    REQUIRE(!r.has_value());
    CHECK(r.error() == audiofile_error::format_error);
}
#endif

TEST_CASE("decoder_for_file")
{
    auto decoder = create_decoder_for_file(KFR_SRC_DIR "/tests/test-audio/testdata_2c_pcm_s24le.wav");
    REQUIRE(decoder != nullptr);
    auto r = decoder->open(KFR_SRC_DIR "/tests/test-audio/testdata_2c_pcm_s24le.wav");
    REQUIRE(r.has_value());
    CHECK(r->container == audiofile_container::wave);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 24);

    test_audiodata(*decoder);
}

TEST_CASE("decoder_for_file_unsupported")
{
    auto decoder = create_decoder_for_file(KFR_SRC_DIR "/tests/test-audio/not_audio");
    REQUIRE(decoder == nullptr);
}

TEST_CASE("read_audiofile_header")
{
    using details::header_is;

    auto h = read_audiofile_header(KFR_SRC_DIR "/tests/test-audio/testdata_2c_pcm_s24le.wav");
    REQUIRE(h.has_value());
    CHECK(header_is(*h, "RIFF....WAVEJUNK"));
    h = read_audiofile_header(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.aiff");
    REQUIRE(h.has_value());
    CHECK(header_is(*h, "FORM....AIFFCOMM"));
    h = read_audiofile_header(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.caf");
    REQUIRE(h.has_value());
    CHECK(header_is(*h, "caff............"));
    h = read_audiofile_header(KFR_SRC_DIR "/tests/test-audio/testdata_2c_s16.flac");
    REQUIRE(h.has_value());
    CHECK(header_is(*h, "fLaC............"));
    h = read_audiofile_header("this_file_does_not_exist.wav");
    REQUIRE(!h.has_value());
    CHECK(h.error() == std::errc::no_such_file_or_directory);
}

TEST_CASE("audiofile_container_from_extension")
{
    CHECK(audiofile_container_from_extension(".wav") == audiofile_container::wave);
    CHECK(audiofile_container_from_extension(".WAV") == audiofile_container::wave);
    CHECK(audiofile_container_from_extension(".wave") == audiofile_container::wave);
    CHECK(audiofile_container_from_extension(".WAVE") == audiofile_container::wave);
    CHECK(audiofile_container_from_extension(".aif") == audiofile_container::aiff);
    CHECK(audiofile_container_from_extension(".AIF") == audiofile_container::aiff);
    CHECK(audiofile_container_from_extension(".aiff") == audiofile_container::aiff);
    CHECK(audiofile_container_from_extension(".AIFF") == audiofile_container::aiff);
    CHECK(audiofile_container_from_extension(".aifc") == audiofile_container::aiff);
    CHECK(audiofile_container_from_extension(".AIFC") == audiofile_container::aiff);
    CHECK(audiofile_container_from_extension(".caf") == audiofile_container::caf);
    CHECK(audiofile_container_from_extension(".CAF") == audiofile_container::caf);
    CHECK(audiofile_container_from_extension(".flac") == audiofile_container::flac);
    CHECK(audiofile_container_from_extension(".FLAC") == audiofile_container::flac);
    CHECK(audiofile_container_from_extension(".mp3") == audiofile_container::mp3);
    CHECK(audiofile_container_from_extension(".MP3") == audiofile_container::mp3);
    CHECK(audiofile_container_from_extension(".unknown") == audiofile_container::unknown);
    CHECK(audiofile_container_from_extension("") == audiofile_container::unknown);
}

TEST_CASE("create_decoder_for_container")
{
    CHECK(create_decoder_for_container(audiofile_container::wave) != nullptr);
    CHECK(create_decoder_for_container(audiofile_container::aiff) != nullptr);
    CHECK(create_decoder_for_container(audiofile_container::caf) != nullptr);
#ifdef KFR_AUDIO_FLAC
    CHECK(create_decoder_for_container(audiofile_container::flac) != nullptr);
#endif
#ifdef KFR_AUDIO_MP3
    CHECK(create_decoder_for_container(audiofile_container::mp3) != nullptr);
#endif
    CHECK(create_decoder_for_container(audiofile_container::unknown) == nullptr);
}

TEST_CASE("every decoder must report io_error for non-existing file")
{
    for (auto c : {
             audiofile_container::wave,
             audiofile_container::aiff,
             audiofile_container::caf,
#ifdef KFR_AUDIO_FLAC
             audiofile_container::flac,
#endif
#ifdef KFR_AUDIO_MP3
             audiofile_container::mp3,
#endif
         })
    {
        auto decoder = create_decoder_for_container(c);
        REQUIRE(decoder != nullptr);
        auto r = decoder->open("this_file_does_not_exist.wav");
        REQUIRE(!r.has_value());
        CHECK(r.error() == audiofile_error::not_found);
    }
}

TEST_CASE("wave encoder")
{
    std::string name = "temp" + std::to_string(std::random_device{}()) + ".wav";

    auto enc = create_wave_encoder({ {}, /* .switch_to_rf64_if_over_4gb = */ false });
    REQUIRE(enc != nullptr);
    audiofile_format info{};
    info.container    = audiofile_container::wave;
    info.codec        = audiofile_codec::lpcm;
    info.endianness   = audiofile_endianness::little;
    info.bit_depth    = 16;
    info.channels     = 2;
    info.sample_rate  = 44100;
    info.total_frames = 44100;
    auto e            = enc->open(name, info);
    REQUIRE(e);
    audio_data data(info.channels, 44100);
    for (size_t ch = 0; ch < info.channels; ++ch)
        data.channel(ch) = data_generator(data.size, ch, dB_to_amp(-3));
    e = enc->write(data);
    REQUIRE(e);
    auto closed = enc->close();
    REQUIRE(closed);

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for file to be closed

    auto dec = create_wave_decoder();
    REQUIRE(dec != nullptr);
    auto r = dec->open(name);
    REQUIRE(r);
    CHECK(r->container == audiofile_container::wave);
    CHECK(r->codec == audiofile_codec::lpcm);
    CHECK(r->endianness == audiofile_endianness::little);
    CHECK(r->bit_depth == 16);
    test_audiodata(*dec);
}

TEST_CASE("Each channel is cache-aligned")
{
    audio_data data(16, 1);
    for (size_t ch = 0; ch < data.channels; ++ch)
    {
        CHECK((reinterpret_cast<uintptr_t>(data.channel(ch).data()) % 64) == 0);
    }
}

TEST_CASE("encoders")
{
    CHECK(create_wave_encoder() != nullptr);
    CHECK(create_w64_encoder() != nullptr);
    CHECK(create_raw_encoder({}) != nullptr);
#ifdef KFR_AUDIO_FLAC
    CHECK(create_flac_encoder() != nullptr);
#endif
    CHECK(create_caff_encoder() != nullptr);
}

static void test_encode_and_decode(const audio_data_interleaved& audio, const audiofile_format& fmt,
                                   audio_encoder& encoder, audio_decoder& decoder, const std::string& ext)
{
    std::string name = "temp" + std::to_string(std::random_device{}()) + ext;

    auto e = encoder.open(name, fmt);
    CHECK(e);
    if (!e)
        return;

    e = encoder.write(audio);
    CHECK(e);
    if (!e)
        return;

    auto closed = encoder.close();
    CHECK(closed);
    if (!closed)
        return;

    auto r = decoder.open(name);
    CHECK(r);
    if (!r)
        return;

    auto decoded = decoder.read_all();
    CHECK(decoded);
    if (!decoded)
        return;
    if (fmt.codec == audiofile_codec::mp3 || fmt.codec == audiofile_codec::alac)
        testAudioEqualityCompressed(*decoded, audio);
    else
        testAudioEquality(*decoded, audio);
}

TEST_CASE("encode and decode raw")
{
    raw_stream_options rawOptions{};
    rawOptions.endianness  = audiofile_endianness::little;
    rawOptions.sample_rate = 44100;
    for (uint32_t channels : { 1, 2, 6, (int)max_audio_channels })
    {
        if (channels > max_audio_channels)
            continue;
        CAPTURE(channels);
        rawOptions.channels = channels;

        auto audio = generate_test_audio(44100, channels, dB_to_amp(-3));

        for (audio_sample_type smp_type :
             { audio_sample_type::f32, audio_sample_type::f64, audio_sample_type::i16, audio_sample_type::i24,
               audio_sample_type::i32 })
        {
            CAPTURE(smp_type);
            rawOptions.bit_depth = audio_sample_bit_depth(smp_type);
            rawOptions.codec =
                audio_sample_is_float(smp_type) ? audiofile_codec::ieee_float : audiofile_codec::lpcm;

            for (audiofile_endianness endianness :
                 { audiofile_endianness::little, audiofile_endianness::big })
            {
                CAPTURE(endianness);
                rawOptions.endianness = endianness;
                test_encode_and_decode(audio, rawOptions.to_format(), *create_raw_encoder({ {}, rawOptions }),
                                       *create_raw_decoder({ {}, rawOptions }), ".raw");
            }
        }
    }
}

TEST_CASE("encode and decode")
{
    audiofile_format format{};
    format.sample_rate = 44100;

    for (uint32_t channels : { 1, 2, 6, (int)max_audio_channels })
    {
        if (channels > max_audio_channels)
            continue;
        CAPTURE(channels);
        format.channels = channels;

        auto audio = generate_test_audio(44100, channels, dB_to_amp(-3));

        for (audio_sample_type smp_type :
             { audio_sample_type::f32, audio_sample_type::f64, audio_sample_type::i16, audio_sample_type::i24,
               audio_sample_type::i32 })
        {
            CAPTURE(smp_type);
            format.bit_depth = audio_sample_bit_depth(smp_type);
            format.codec =
                audio_sample_is_float(smp_type) ? audiofile_codec::ieee_float : audiofile_codec::lpcm;
            format.endianness = audiofile_endianness::little;

            {
                INFO("wave");
                test_encode_and_decode(audio, format, *create_wave_encoder(), *create_wave_decoder(), ".wav");
            }
            {
                INFO("w64");
                test_encode_and_decode(audio, format, *create_w64_encoder(), *create_w64_decoder(), ".w64");
            }

            if (format.codec == audiofile_codec::lpcm)
            {
#ifdef KFR_AUDIO_ALAC
                if (format.channels <= 8)
                {
                    INFO("alac");
                    audiofile_format formatAlac = format;
                    formatAlac.codec            = audiofile_codec::alac;
                    test_encode_and_decode(audio, formatAlac, *create_caff_encoder(), *create_caff_decoder(),
                                           ".alac.caf");
                }
#endif
#ifdef KFR_AUDIO_FLAC
                if (format.channels <= 8)
                {
                    INFO("flac");
                    audiofile_format formatFlac = format;
                    formatFlac.codec            = audiofile_codec::flac;
                    test_encode_and_decode(audio, formatFlac, *create_flac_encoder(), *create_flac_decoder(),
                                           ".flac");
                }
#endif
            }
            for (audiofile_endianness endianness :
                 { audiofile_endianness::little, audiofile_endianness::big })
            {
                CAPTURE(endianness);
                format.endianness = endianness;
                {
                    INFO("caff");
                    test_encode_and_decode(audio, format, *create_caff_encoder(), *create_caff_decoder(),
                                           ".caf");
                }
                if (format.codec == audiofile_codec::lpcm || format.endianness == audiofile_endianness::big)
                {
                    INFO("aiff");
                    test_encode_and_decode(audio, format, *create_aiff_encoder(), *create_aiff_decoder(),
                                           ".aiff");
                }
            }
        }
    }
}

#ifndef KFR_NO_MAIN
int main(int argc, char* argv[])
{
    println(library_version(), " running on ", cpu_runtime());

    int result = Catch::Session().run(argc, argv);

    return result;
}
#endif
