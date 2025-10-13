# How to do Sample Rate Conversion

## How to apply a Sample Rate Conversion to a contiguous signal?

For a continuous signal, the same instance of the `samplerate_converter` class should be used across all subsequent calls, rather than creating a new instance for each fragment. In the case of stereo audio, two instances (one per channel) are required.

The `samplerate_converter` class supports both `push` and `pull` methods for handling data flow.  

- **`push`**: Input data of a fixed size is provided, and all available output data is received.  
  **Example**: Processing audio from a microphone, where the sound device sends data in fixed-size chunks.  

- **`pull`**: An output buffer of a fixed size is provided, and the necessary amount of input data is processed to generate the required output.  
  **Example**: Streaming audio at a different sample rate to a sound device, where a specific number of output samples must be generated to fill the device buffer.

Let’s consider the case of resampling 44.1 kHz to 96 kHz with an output buffer of 512 samples (`pull`).  
The corresponding input size should be 235.2, which is not an integer.

The `samplerate_converter` class processes signals split into buffers of different sizes by maintaining an internal state.

To determine the required input buffer size for the next call to `process`, `input_size_for_output` can be used by passing the desired output buffer length. This will return either 236 or 235 samples in the 44.1khz to 96khz scenario.

The `process` function accepts two parameters:  
- `output`: Output buffer, provided as a univector with the desired size (512).  
- `input`: Input buffer, provided as a univector of at least the size returned by `input_size_for_output`. The resampler consumes the necessary number of samples to generate 512 output samples and returns the number of input samples read. The input should be adjusted accordingly to skip these samples.

For the `push` method, call `output_size_for_input` with the size of your input buffer. This function returns the corresponding output buffer size required to receive all pending output.

### Example (pull)

```c++
// Initialization
auto src = samplerate_converter<double>(sample_rate_conversion_quality::high, output_samplerate, input_samplerate);

void process_chunk(univector_ref<double> output) {
    univector<double> input(src.input_size_for_output(output.size()));
    // fill `input` with input samples
    src.process(output, input);
    // `output` now contains resampled version of input
}
```

### Example (push)

```c++
// Initialization
auto src = samplerate_converter<double>(resample_quality::high, output_sr, input_sr);

void process_chunk(univector_ref<const double> input) {
    univector<double> output(src.output_size_for_input(input.size()));
    src.process(output, input);
    // `output` now contains resampled version of input
}
```

## Processing audio streams or files in chunks

For large audio files or real-time streams, the signal should be processed in sequential chunks rather than loading the entire content into memory.
Each channel should maintain its own `samplerate_converter` instance across all iterations.

1. **Initialize converters** once before the loop.
2. **Determine chunk sizes** based on target output rate:

   * Use `resampler.input_size_for_output(chunk_size)` to compute how many input frames to read for a given number of desired output frames.
   * For the first chunk, include the delay compensation from `resampler.get_delay()`.
3. **Read a chunk of input samples**, deinterleave if multichannel.
4. **Call `process` for each channel**:

   * Optionally skip the initial delay samples (`r.skip(r.get_delay(), input)` on the first chunk).
   * Call `r.process(output, input)` to resample.
5. **Write the resulting output chunk** to the encoder or output stream.
6. **Repeat until the decoder signals end of file or stream.**

### Example (simplified loop)

```c++
std::vector<samplerate_converter<float>> resamplers(channels);
for (size_t ch = 0; ch < channels; ++ch)
    resamplers[ch] = resampler<float>(resample_quality::high, output_sr, input_sr);

constexpr size_t output_chunk_size = 16384;
audio_data output_chunk(channels, output_chunk_size);
audio_data input_chunk(channels, resamplers[0].input_size_for_output(output_chunk_size));

bool first_chunk = true;

for (;;) {
    auto frames_read = decoder->read_to(input_chunk_interleaved.truncate(input_chunk.size()));
    if (!frames_read || frames_read.error() == audiofile_error::end_of_file) break;

    input_chunk = input_chunk_interleaved.truncate(*frames_read);

    for (size_t ch = 0; ch < channels; ++ch) {
        auto& r = resamplers[ch];
        if (first_chunk)
            r.skip(r.get_delay(), input_chunk.channel(ch));
        r.process(output_chunk.channel(ch), input_chunk.channel(ch));
    }

    encoder->write(output_chunk);
    first_chunk = false;
}
encoder->close();
```

This approach minimizes memory use, supports long recordings or live input, and maintains sample-rate accuracy by preserving converter state across chunks.

[See also a gallery with results of applying various SRC presets](src_gallery.md)
