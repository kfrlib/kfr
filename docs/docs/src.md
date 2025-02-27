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

[See also a gallery with results of applying various SRC presets](src_gallery.md)
