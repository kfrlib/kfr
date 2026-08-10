# Reading and Writing Audio Files with KFR

KFR provides decoder and encoder classes for reading and writing audio files across several common containers and codecs. This lets you load audio into memory for processing (filtering, resampling, analysis) and save the result back to disk. Include `<kfr/audio.hpp>` to use these facilities.

### Supported Formats

KFR reads WAV (including RF64 and BW64 for files over 4 GB), W64, AIFF, CAF, FLAC, and MP3 (decode-only), and writes WAV (with automatic RF64 fallback for large files), W64, AIFF, CAF, and FLAC. CAF additionally supports the ALAC codec when built with the ALAC library. On Windows, an optional Media Foundation decoder extends support to additional formats. FLAC and ALAC support depend on external libraries enabled at build time via CMake. See [Audio Format Support](file_support.md) for the full breakdown of codecs, bit depths, and container capabilities.

### Key Concepts

- **Audio data storage**: Samples are stored in [[`audio_data<IsInterleaved>`:nosig]], a template over the sample layout: [[`audio_data_interleaved`:nosig]] (channels interleaved in a single buffer) or [[`audio_data_planar`:nosig]] (one buffer per channel). The sample type is always [[`fbase`:nosig]] (`float` or `double`, depending on how KFR was built).
- **Error handling**: I/O functions return `expected<T, audiofile_error>`. Check the result before using it — either test it directly (`if (!result) ...`) or call `.error()` to get the failure reason. Unchecked errors typically surface later as an empty buffer or a crash when dereferencing `*result`.
- **File paths**: The [[`file_path`:nosig]] type depends on the build configuration:
    - `std::filesystem::path`, if the `KFR_USE_STD_FILESYSTEM` CMake option is enabled.
  - Otherwise, `std::wstring` on Windows or `std::string` on Linux/macOS.
  - On Windows without `std::filesystem`, overloads that accept a UTF-8 encoded `std::string` are also provided; they convert to `std::wstring` internally.
- **Working with large files**: Read and write in chunks rather than loading an entire file into memory (see [Sample rate conversion](../dsp/src.md#processing-streams-and-files-in-chunks) for a chunked-processing example). [[`audio_data<IsInterleaved>`:nosig]] also has constructors that wrap caller-supplied buffers instead of allocating, which avoids a copy when integrating with existing audio pipelines.

## Reading Audio into [[`audio_data<IsInterleaved>`:nosig]]

To read an audio file, create an [[`audio_decoder`:nosig]], open the file to retrieve its format, then read the samples into an [[`audio_data<IsInterleaved>`:nosig]] buffer.

### Step-by-Step Process

1. Create a decoder with [[`create_decoder_for_file`]]. It's selected from the file extension, or the header when the extension is missing or ambiguous; it returns `nullptr` if no matching decoder is available.
2. Call [[`audio_decoder::open(const std::string &)`:noscope]] to retrieve the [[`audiofile_format`:nosig]] (sample rate, channel count, bit depth, and so on).
3. Read samples with [[`audio_decoder::read_all()`:noscope]] (whole file into a newly allocated buffer), [[`audio_decoder::read(size_t)`:noscope]] (up to a fixed number of frames into a newly allocated buffer), or [[`audio_decoder::read_to(const audio_data_interleaved &)`:noscope]] (into a buffer you already own). The first two return [[`audio_data_interleaved`:nosig]], while `read_to` returns the number of frames read; use [[`audio_decoder::read_all_planar()`:noscope]] if you want a planar buffer directly.
4. Convert to planar afterward, if needed, by constructing an [[`audio_data_planar`:nosig]] from the interleaved result.

Check the result of every step — [[`create_decoder_for_file`:noscope]] can return `nullptr`, and [[`audio_decoder::open(const std::string &)`]]/[[`audio_decoder::read_all()`:noscope]] return `expected<T, audiofile_error>`.

### Example: Reading a Stereo WAV File

```c++
#include <kfr/audio.hpp>
#include <iostream>
|||#include <array>
|||#include <cstdint>
|||#include <vector>

|||#define SNIPPET_AUDIO_PATH(name) KFR_FILEPATH(KFR_SRC_DIR "/tests/test-audio/" name)

int main() {|||TEST_CASE("read_audio.md/reading a stereo WAV file") {
    kfr::file_path path = KFR_FILEPATH("input.wav");|||const kfr::file_path path = SNIPPET_AUDIO_PATH("testdata_2c_pcm_s24le.wav");

    auto decoder = kfr::create_decoder_for_file(path);
    if (!decoder) {
        std::cerr << "Error: no decoder available for this file" << std::endl;
        return 1;|||FAIL("no decoder available for this file"); return;
    }

    auto format = decoder->open(path);
    if (!format) {
        std::cerr << "Error opening file: " << kfr::to_string(format.error()) << std::endl;
        return 1;|||FAIL(kfr::to_string(format.error())); return;
    }

    std::cout << "Sample rate: " << format->sample_rate << ", Channels: " << format->channels << std::endl;

    auto data = decoder->read_all();  // Reads the entire file into audio_data_interleaved
    if (!data) {
        std::cerr << "Error reading data: " << kfr::to_string(data.error()) << std::endl;
        return 1;|||FAIL(kfr::to_string(data.error())); return;
    }

    // data->size is the frame count, data->channels is the channel count
    // Convert to planar if needed: kfr::audio_data_planar planar = *data;

    return 0;|||CHECK(data->channel_count() == 2);
}|||}
```

[[`file_path`:nosig]] and the [[`KFR_FILEPATH`:nosig]] macro adapt to the active build configuration (see [Key Concepts](#key-concepts) above), so this example compiles unchanged whether [[`file_path`:nosig]] is `std::string`, `std::wstring`, or `std::filesystem::path`. On Windows without `std::filesystem`, you can instead pass a `std::string` directly; matching overloads convert it internally.

### Reading Directly into User-Supplied Buffers (No Copy)

[[`audio_data_interleaved`:nosig]] and [[`audio_data_planar`:nosig]] can wrap existing pointers instead of allocating, which avoids a copy when the destination buffer already exists (for example, a buffer owned by another library).

#### Example: Reading Interleaved Stereo Audio into a `std::vector`

```c++
|||TEST_CASE("read_audio.md/reading interleaved audio into a vector")
|||{
std::vector<kfr::fbase> buffer(44100 * 2 * 60);  // 1 minute of 44.1 kHz stereo

kfr::audio_data_interleaved stereo_data(buffer.data(), 2, buffer.size() / 2);  // Wrap the buffer, no copy

auto decoder = kfr::create_decoder_for_file(KFR_FILEPATH("stereo.wav"));|||const kfr::file_path path = SNIPPET_AUDIO_PATH("testdata_2c_pcm_s24le.wav");
|||auto decoder = kfr::create_decoder_for_file(path);
if (!decoder) {
    std::cerr << "Error: no decoder available for this file" << std::endl;
    return;|||FAIL("no decoder available for this file"); return;
}
auto format = decoder->open(KFR_FILEPATH("stereo.wav"));|||auto format = decoder->open(path);
if (format) {
    auto read = decoder->read_to(stereo_data);  // Read directly into the buffer
    if (read) {
        std::cout << "Read " << *read << " frames" << std::endl;
|||        CHECK(*read > 0);
    }
}
|||REQUIRE(format);
|||}
```

For planar data, wrap an array of per-channel pointers instead:

```c++
|||TEST_CASE("read_audio.md/wrapping planar audio buffers")
|||{
constexpr size_t frames = 44100;
std::vector<kfr::fbase> left(frames);
std::vector<kfr::fbase> right(frames);
std::array<kfr::fbase*, 2> pointers = { left.data(), right.data() };
kfr::audio_data_planar planar(pointers, left.size());  // Wrap two buffers, no copy
|||CHECK(planar.channel_count() == 2);
|||CHECK(planar.size == frames);
|||}
```

### Advanced Options

- Pass `audio_decoding_options{ .read_metadata = true }` to [[`create_decoder_for_file(const file_path &, const audio_decoding_options &)`:nosig]] to load tags (artist, title, and so on) into `format->metadata`.
- To seek, call [[`audio_decoder::seek(uint64_t)`:noscope]] and pass the position in frames. Seeking isn't always sample-accurate for compressed codecs; call [[`audio_decoder::seek_is_precise()`:noscope]] to check before relying on exact positioning.

## Writing Audio from [[`audio_data<IsInterleaved>`:nosig]]

Writing means creating a [[`audio_encoder`:nosig]] for the target container, opening the destination file with the desired [[`audiofile_format`:nosig]], writing one or more chunks of data, and closing the encoder to finalize the file.

### Step-by-Step Process
1. Create an encoder for the target container — either a container-specific factory such as [[`create_wave_encoder`]], or [[`create_encoder_for_container(audiofile_container, const audio_encoding_options &)`:nosig]] when the container is only known at runtime.
2. Fill in an [[`audiofile_format`:nosig]] (container, codec, bit depth, sample rate, channel count).
3. Call [[`audio_encoder::open(const std::string &, const audiofile_format &, audio_decoder *)`:noscope]] to start writing.
4. Write one or more chunks of interleaved data with [[`audio_encoder::write(const audio_data_interleaved &)`:noscope]].
5. Call [[`audio_encoder::close()`:noscope]] to finalize the file and get the total number of frames written.

### Example: Writing Processed Audio to WAV

```c++
|||TEST_CASE("read_audio.md/writing processed audio to WAV")
|||{
kfr::audio_data_interleaved data(2, 44100);  // Stereo, 1 second at 44.1 kHz
// Fill data with samples...

kfr::audiofile_format format;
format.container  = kfr::audiofile_container::wave;
format.codec       = kfr::audiofile_codec::lpcm;
format.bit_depth   = 16;  // 16-bit PCM
format.sample_rate = 44100;
format.channels    = 2;

auto encoder = kfr::create_wave_encoder();
auto opened = encoder->open(KFR_FILEPATH("output.wav"), format);|||const kfr::file_path path = KFR_FILEPATH("snippet_output.wav");
|||auto opened = encoder->open(path, format);
if (!opened) {
    std::cerr << "Error opening file: " << kfr::to_string(opened.error()) << std::endl;
    return 1;|||FAIL(kfr::to_string(opened.error())); return;
}

auto written = encoder->write(data);
if (!written) {
    std::cerr << "Error writing: " << kfr::to_string(written.error()) << std::endl;
    return 1;|||FAIL(kfr::to_string(written.error())); return;
}

auto closed = encoder->close();
if (closed) {
    std::cout << "Wrote " << *closed << " frames" << std::endl;
|||    CHECK(*closed == data.size);
}
|||REQUIRE(closed);
|||}
```

Pass `audio_encoding_options{ .dithering = kfr::audio_dithering::triangular }` to the encoder factory for better-sounding quantization noise when reducing bit depth (options: `none`, `rectangular`, `triangular`).

[[`audio_encoder::write(const audio_data_interleaved &)`:noscope]] only accepts [[`audio_data_interleaved`]]; if your data is planar, convert it first (`kfr::audio_data_interleaved interleaved = planar_data;`) or use the planar overload of [[`encode_audio_file`]] described below.

## Working with Raw Audio

Raw audio (a sample stream with no container header or metadata) is supported through dedicated decoder and encoder factories that take the format as an explicit parameter, since there's no header to read it from.

### Reading Raw Audio

Use [[`create_raw_decoder`]] with format that specifies sample rate, channel count, bit depth, and codec.

### Writing Raw Audio

Use [[`create_raw_encoder`]], then proceed as with any other encoder (open with an [[`audiofile_format`:nosig]], write, close).

### Example: Reading Raw 16-bit PCM Stereo

```c++
|||TEST_CASE("read_audio.md/reading raw PCM stereo")
|||{
kfr::raw_decoding_options opts;
opts.format.sample_rate = 48000;
opts.format.channels    = 2;
opts.format.bit_depth   = 32;
opts.format.codec       = kfr::audiofile_codec::ieee_float;

auto decoder = kfr::create_raw_decoder(opts);
auto format  = decoder->open(KFR_FILEPATH("raw.pcm"));|||auto format = decoder->open(SNIPPET_AUDIO_PATH("testdata_2c.f32le"));
auto data    = decoder->read_all();
if (!format || !data) {
    std::cerr << "Error reading raw PCM data" << std::endl;
}
|||REQUIRE(format);
|||REQUIRE(data);
|||CHECK(data->size > 0);
|||}
```

Raw I/O assumes little-endian samples by default; set `opts.format.endianness` explicitly if the stream is big-endian.

## Decoding and Encoding in One Call

For simple cases that don't need explicit control over the decoder or encoder lifetime, use the convenience functions below.

### Decoding

[[`decode_audio_file`]] opens the file, reads it in full, and returns [[`audio_data_interleaved`:nosig]]. Pass a pointer to an [[`audiofile_format`:nosig]] if you want the detected format back; pass `nullptr` (the default) to discard it.

```c++
|||TEST_CASE("read_audio.md/decoding an audio file")
|||{
kfr::audiofile_format detected_format;
auto data = kfr::decode_audio_file(KFR_FILEPATH("input.flac"), &detected_format);|||auto data = kfr::decode_audio_file(SNIPPET_AUDIO_PATH("testdata_2c_pcm_s24le.wav"), &detected_format);
if (data) {
    // Process *data
} else {
    std::cerr << "Error: " << kfr::to_string(data.error()) << std::endl;
}
|||REQUIRE(data);
|||CHECK(detected_format.channels == 2);
|||}
```

### Encoding

[[`encode_audio_file`:nosig]] creates the appropriate encoder, opens the file, writes `data` in one call, and closes it. Overloads accept either [[`audio_data_interleaved`:nosig]] or [[`audio_data_planar`:nosig]].

```c++
|||TEST_CASE("read_audio.md/encoding an audio file")
|||{
kfr::audio_data_interleaved data(2, 44100, 0.0f);
kfr::audiofile_format format;
format.container  = kfr::audiofile_container::wave;
format.codec      = kfr::audiofile_codec::lpcm;
format.bit_depth  = 16;
format.sample_rate = 44100;
format.channels    = 2;
auto result = kfr::encode_audio_file(KFR_FILEPATH("output.wav"), data, format);|||const auto result = kfr::encode_audio_file(KFR_FILEPATH("snippet_encode.wav"), data, format);
if (!result) {
    std::cerr << "Error: " << kfr::to_string(result.error()) << std::endl;
}
|||REQUIRE(result);
|||}
```

To copy metadata (such as tags) from an existing file while re-encoding, pass the source decoder as the fourth argument: [[`encode_audio_file`:nosig]].

## Reading a RIFF Chunk

RIFF-based containers (WAV, W64, RF64, BW64) store auxiliary data in named chunks, such as `LIST`/`INFO` for metadata. [[`audio_decoder::has_chunk(std::span<const std::byte>)`:nosig:noscope]], [[`audio_decoder::read_chunk(std::span<const std::byte>, const std::function<bool (std::span<const std::byte>)> &, size_t)`:nosig:noscope]], and [[`audio_decoder::read_chunk_bytes(std::span<const std::byte>)`:nosig:noscope]] give direct access to these chunks; they're implemented for RIFF-family decoders and return [[`audiofile_error::not_implemented`]] (or an empty result) on non-RIFF containers such as FLAC or MP3.

### Example: Reading a Custom Chunk

```c++
|||TEST_CASE("read_audio.md/reading a custom RIFF chunk")
|||{
auto decoder = kfr::create_decoder_for_file(KFR_FILEPATH("file.wav"));|||const kfr::file_path path = SNIPPET_AUDIO_PATH("testdata_2c_pcm_s24le.wav");
|||auto decoder = kfr::create_decoder_for_file(path);
if (!decoder) {
    std::cerr << "Error: no decoder available for this file" << std::endl;
    return;|||FAIL("no decoder available for this file"); return;
}
auto format = decoder->open(KFR_FILEPATH("file.wav")); // Handle errors as usual|||auto format = decoder->open(path);
if (!format) {
    std::cerr << "Error opening file: " << kfr::to_string(format.error()) << std::endl;
    return;|||FAIL(kfr::to_string(format.error())); return;
}

constexpr char id[4] = { 'L', 'I', 'S', 'T' };
std::span<const std::byte> chunk_id(reinterpret_cast<const std::byte*>(id), 4);

if (decoder->has_chunk(chunk_id)) {
    auto read = decoder->read_chunk_bytes(chunk_id);
    if (read) {
        std::vector<uint8_t> chunk_data = std::move(*read);
        // Process chunk_data
    }
}
|||REQUIRE(decoder->has_chunk(chunk_id));
|||}
```

For large chunks, use [[`audio_decoder::read_chunk(std::span<const std::byte>, const std::function<bool (std::span<const std::byte>)> &, size_t)`:nosig:noscope]] instead: it invokes `handler` repeatedly with successive buffers rather than accumulating the whole chunk in memory.

This is useful for extracting metadata or format-specific extensions without decoding the audio samples themselves.
