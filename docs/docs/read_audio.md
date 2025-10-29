# How to Read or Write Audio Files in KFR

KFR provides a comprehensive set of tools for reading and writing audio files, supporting a variety of formats through its decoder and encoder classes. This allows you to load audio data into memory for processing (e.g., DSP operations) and save processed results back to disk. The library handles common audio containers and codecs, making it suitable for tasks like sample rate conversion, filtering, or analysis.

### Key Features and Supported Formats

- **Input Formats (Decoding)**: WAV (including RF64 and BW64 extensions for large files), W64, FLAC, MP3, AIFF, CAF. Metadata reading is optional and controlled via decoding options.
- **Output Formats (Encoding)**: WAV (with optional RF64 fallback for files over 4GB), W64, AIFF, CAF, FLAC. Supports PCM (16/24/32-bit) and IEEE floating-point (32/64-bit) codecs. Dithering options (none, rectangular, triangular) are available for quantization during encoding.
- **Audio Data Storage**: Audio samples are stored in `audio_data` templates, which can be planar (separate arrays per channel) or interleaved (channels mixed in a single array). The sample type is `fbase` (typically `double`).
- **Error Handling**: All I/O functions return `expected<T, audiofile_error>` types. You must check for errors (e.g., file not found, invalid format) using `.has_value()` or `.error()`. Ignoring errors can lead to runtime crashes or silent failures—always handle them explicitly.
- **File Path Handling**: The `file_path` type depends on your build configuration:
  - If `KFR_USE_STD_FILESYSTEM` is enabled (CMake option), `file_path` is `std::filesystem::path`.
  - Otherwise, `file_path` is `std::string` on Linux/macOS and `std::wstring` on Windows.
  - On Windows (without std::filesystem), overloads accept `std::string` (UTF-8 encoded) and convert internally to `std::wstring`.
- **Performance Tips**: For large files, read/write in chunks to avoid memory exhaustion. Use `audio_data` constructors that take user-supplied pointers to avoid unnecessary copies. The library is optimized for SIMD, so align your data where possible.
- **Dependencies**: FLAC support require external library (enabled via CMake). On Windows, Media Foundation decoders are available for additional formats.

Include `<kfr/audio.hpp>` for audio I/O functionality.

## Reading Audio into `audio_data`

To read audio files, use an `audio_decoder` instance. You can create one automatically based on the file extension or header, then open the file and read data into an `audio_data` structure.

### Step-by-Step Process

1. Create a decoder using `create_decoder_for_file(path)`.
2. Call `decoder->open(path)` to retrieve the `audiofile_format` (sample rate, channels, bit depth, etc.).
3. Read data using `decoder->read_all()` (for the entire file) or `decoder->read(max_frames)` (for chunks). Data is returned as `audio_data_interleaved` by default.
4. Optionally convert to planar format if needed.

Handle errors at each step, as file access or format issues can occur.

### Example: Reading a Stereo WAV File

```cpp
#include <kfr/audio.hpp>
#include <iostream>

int main() {
    std::string path = "input.wav";  // Use std::string on Windows/Linux/macOS

    auto decoder = kfr::create_decoder_for_file(path);
    if (!decoder) {
        std::cerr << "Error creating decoder: " << kfr::to_string(decoder.error()) << std::endl;
        return 1;
    }

    auto format = decoder->open(path);
    if (!format) {
        std::cerr << "Error opening file: " << kfr::to_string(format.error()) << std::endl;
        return 1;
    }

    std::cout << "Sample rate: " << format->sample_rate << ", Channels: " << format->channels << std::endl;

    auto data = decoder->read_all();  // Reads entire file into audio_data_interleaved
    if (!data) {
        std::cerr << "Error reading data: " << kfr::to_string(data.error()) << std::endl;
        return 1;
    }

    // Access data: data->size is frames, data->channels is number of channels
    // Convert to planar if needed: kfr::audio_data_planar planar = *data;

    return 0;
}
```

### Reading Directly into User-Supplied Buffers (No Copy)

`audio_data_interleaved` and `audio_data_planar` can wrap existing pointers without copying data, which is efficient for large buffers or integrating with external libraries.

#### Example: Reading Interleaved Stereo Audio into `std::vector`

```cpp
std::vector<kfr::fbase> buffer(44100 * 2 * 60);  // Pre-allocate for 1 minute at 44.1kHz stereo

kfr::audio_data_interleaved stereo_data(buffer.data(), 2, buffer.size() / 2);  // Wrap pointer, no copy

auto decoder = kfr::create_decoder_for_file("stereo.wav");
auto format = decoder->open("stereo.wav");
if (format) {
    auto read = decoder->read_to(stereo_data);  // Read directly into buffer
    if (read) {
        std::cout << "Read " << *read << " frames" << std::endl;
    }
}
```

For planar data, use an array of pointers: `std::array<kfr::fbase*, 2> pointers = { left.data(), right.data() }; kfr::audio_data_planar planar(pointers, 2, size);`.

### Advanced Options

- Use `audio_decoding_options{ .read_metadata = true }` to load tags (e.g., artist, title) into `format->metadata`.
- For seeking: `decoder->seek(position_in_frames)` (not always precise; check `decoder->seek_is_precise()`).

## Writing Audio from `audio_data`

Writing involves creating an `audio_encoder`, specifying the format, and encoding data.

### Step-by-Step Process
1. Create an encoder using `create_encoder_for_container(container)`.
2. Set the `audiofile_format` (e.g., sample rate, channels, bit depth).
3. Call `encoder->open(path, format)` to start writing.
4. Write audio chunks with `encoder->write(data)`.
5. Call `encoder->close()` to finalize.

### Example: Writing Processed Audio to WAV

```cpp
kfr::audio_data_interleaved data(2, 44100);  // Stereo, 1 second at 44.1kHz
// Fill data with samples...

auto encoder = kfr::create_wave_encoder();
kfr::audiofile_format format;
format.container = kfr::audiofile_container::wave;
format.codec = kfr::audiofile_codec::lpcm;
format.bit_depth = 16;  // 16-bit PCM
format.sample_rate = 44100;
format.channels = 2;

auto opened = encoder->open("output.wav", format);
if (!opened) {
    std::cerr << "Error opening file: " << kfr::to_string(opened.error()) << std::endl;
    return 1;
}

auto written = encoder->write(data);
if (!written) {
    std::cerr << "Error writing: " << kfr::to_string(written.error()) << std::endl;
}

auto closed = encoder->close();
if (closed) {
    std::cout << "Wrote " << *closed << " frames" << std::endl;
}
```

Use `audio_encoding_options{ .dithering = kfr::audio_dithering::triangular }` for better quantization when reducing bit depth.

For planar data, convert to interleaved first or use overloads that accept `audio_data_planar`.

## Working with Raw Audio

Raw audio (no headers/metadata) is supported via specialized decoders/encoders. Specify the format manually.

### Reading Raw Audio

Use `create_raw_decoder(raw_decoding_options{ .format = your_format })`.

### Writing Raw Audio

Use `create_raw_encoder()` and set format details.

### Example: Reading Raw 16-bit PCM Stereo

```cpp
kfr::raw_decoding_options opts;
opts.format.sample_rate = 48000;
opts.format.channels = 2;
opts.format.bit_depth = 16;
opts.format.codec = kfr::audiofile_codec::lpcm;

auto decoder = kfr::create_raw_decoder(opts);
auto format = decoder->open("raw.pcm");
auto data = decoder->read_all();
```

Raw I/O assumes little-endian by default; adjust `format.endianness` if needed.

## Encoding/Decoding Audio in One Call

For simple cases, use convenience functions to handle everything in one step.

### Decoding

`decode_audio_file(path, &out_format)` returns `audio_data_interleaved` directly.

```cpp
auto data = kfr::decode_audio_file("input.flac");
if (data) {
    // Process data.value()
} else {
    std::cerr << "Error: " << kfr::to_string(data.error()) << std::endl;
}
```

### Encoding

`encode_audio_file(path, data, format)` writes directly.

```cpp
kfr::audiofile_format format;  // Set as above
auto result = kfr::encode_audio_file("output.wav", data, format);
if (!result) {
    std::cerr << "Error: " << kfr::to_string(result.error()) << std::endl;
}
```

Copy metadata from a decoder: `encode_audio_file(path, data, format, decoder.get())`.

## How to Read a RIFF Chunk

RIFF-based formats (e.g., WAV) contain chunks like 'data' or 'LIST'. Use the decoder to access them.

### Example: Reading a Custom Chunk

```cpp
auto decoder = kfr::create_decoder_for_file("file.wav");
decoder->open("file.wav"); // Handle errors as usual

std::span<const std::byte> chunk_id = { reinterpret_cast<const std::byte*>("INFO"), 4 };
auto has_chunk = decoder->has_chunk(chunk_id);
if (has_chunk) {
    std::vector<uint8_t> chunk_data;
    auto read = decoder->read_chunk_bytes(chunk_id);
    if (read) {
        chunk_data = std::move(*read);
        // Process chunk_data
    }
}
```

For streaming: Use `decoder->read_chunk(chunk_id, handler)` with a callback that processes data in buffers.

This is useful for extracting metadata or custom extensions without decoding the full audio.
