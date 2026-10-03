# Sample Rate Conversion with KFR

KFR's [[`samplerate_converter`:nosig]] performs [polyphase sample rate conversion](../advanced/dsp_glossary.md#polyphase-sample-rate-conversion): the resampling filter is a [window-sinc](../advanced/dsp_glossary.md#window-sinc-method) FIR design (Kaiser window), split into per-phase sub-filters so each output sample is produced from a single sub-filter convolution rather than the full kernel. The Kaiser beta is derived from the requested quality level, so higher quality levels use a longer filter and a more aggressive window for better stopband attenuation.

## Processing a Continuous Signal

For a continuous signal, reuse the same [[`samplerate_converter`:nosig]] instance across all calls instead of creating a new one for each fragment. The instance owns both the FIR delay line and the exact input/output positions, which makes consecutive chunks resample as one uninterrupted signal. Each channel needs its own instance, so a stereo stream requires two.

The converter has a linear-phase FIR delay. The normal, non-compensated workflow retains this delay and is the simplest way to process a stream. [Delay compensation](#delay-compensation) is an optional second workflow for finite signals or buffered streams that need their output aligned with the original timeline.

The [[`samplerate_converter`:nosig]] class supports both `push` and `pull` data flow:

- **`push`**: You supply a fixed amount of input and receive however much output is available.
  Example: processing audio from a microphone, which delivers data in fixed-size chunks.
- **`pull`**: You request a fixed amount of output, and the converter determines how much input it needs to produce it.
  Example: streaming audio to a sound device, which needs a specific number of output samples to fill its buffer.

Consider resampling 44.1 kHz to 96 kHz with a 512-sample output buffer (`pull`). The exact input size would be 235.2 samples, which isn't an integer — the required input size varies slightly from call to call as the fractional remainder accumulates. The [[`samplerate_converter`:nosig]] class handles this by tracking its internal position, so buffers of varying size can be processed back-to-back without losing sync.

To find the required input size for the next call to [[`samplerate_converter<T>::process`:nosig:noscope]], call [[`samplerate_converter<T>::input_size_for_output`:nosig:noscope]] with the desired output length; in this example it returns 235 or 236 samples, depending on the converter's current internal position. [[`samplerate_converter<T>::process`:nosig:noscope]] then takes two arguments:

- `output`: the output buffer, sized to the desired number of samples (512 in this example).
- `input`: the next contiguous input samples. For a pull workflow, supply exactly the value returned by [[`samplerate_converter<T>::input_size_for_output`:nosig:noscope]]. [[`samplerate_converter<T>::process`:nosig:noscope]] consumes that count and returns it; advance the source position by the returned count before the next call.

For the `push` method, call [[`samplerate_converter<T>::output_size_for_input`:nosig:noscope]] with the number of new input samples to find out how much output space to allocate. The sizing functions use the converter's current state; call them immediately before the matching `process` or `skip` operation, rather than calculating sizes from a nominal rate ratio.

### Example (pull)

```c++
// Initialization
|||#include <kfr/dsp.hpp>
|||using namespace kfr;
|||TEST_CASE("dsp/src.md/pull example")
|||{
const size_t input_sr = 44100;
const size_t output_sr = 96000;
auto src = samplerate_converter<double>(resample_quality::high, output_sr, input_sr);

void process_chunk(univector_ref<double> output)|||const auto process_chunk = [&](univector_ref<double> output)
{
    univector<double> input(src.input_size_for_output(output.size()));
    input = scalar(1.0);
    src.process(output, input);
    // `output` now contains the resampled input
}||| };
|||univector<double> output_buffer(512);
|||process_chunk(output_buffer);
|||CHECK(src.input_size_for_output(output_buffer.size()) == 235);
|||}
```

### Example (push)

```c++
// Initialization
|||TEST_CASE("dsp/src.md/push example")
|||{
const size_t input_sr = 44100;
const size_t output_sr = 96000;
auto src = samplerate_converter<double>(resample_quality::high, output_sr, input_sr);

void process_chunk(univector_ref<const double> input)|||const auto process_chunk = [&](univector_ref<const double> input)
{
    univector<double> output(src.output_size_for_input(input.size()));
    src.process(output, input);
    // `output` now contains the resampled input
}||| };
|||univector<double> input(256, 1.0);
|||process_chunk(input);
|||CHECK(src.output_size_for_input(1) == 2);
|||}
```

## Processing Streams and Files in Chunks

For large audio files or real-time streams, process sequential chunks instead of loading the full signal into memory. Keep one [[`samplerate_converter`:nosig]] per channel for the entire stream. Never reset, replace, or share a converter between independent channels while processing a continuous signal.

### Non-compensated workflow

This is the default and simplest workflow. Call [[`samplerate_converter<T>::process`:nosig:noscope]] for every input chunk and write every output sample it produces. The initial output includes the FIR's normal startup delay, just as it would in a real-time signal path. No input samples are discarded and no special first-chunk logic is needed.

For a finite signal, retaining the startup delay makes the result slightly longer: it contains the leading delayed samples in addition to the resampled signal. This is usually desirable for real-time playback, monitoring, or any pipeline in which latency is represented explicitly rather than removed.

The following pull-based loop requests 16,384 output frames at a time. The converter determines how many input frames are needed for each request. The final, shorter read uses `output_size_for_input` to determine its exact output length.

```c++
|||TEST_CASE("dsp/src.md/non-compensated chunks")
|||{
constexpr size_t output_chunk_size = 16384;
const size_t input_sr = 44100;
const size_t output_sr = 96000;
auto src = samplerate_converter<fbase>(resample_quality::high, output_sr, input_sr);

univector<fbase> output(output_chunk_size);
|||univector<fbase> source(2 * output_chunk_size, fbase(1));
|||size_t source_position = 0;
|||size_t written = 0;
|||const auto read_next_input = [&](univector_ref<fbase> input) {
|||    const size_t count = std::min(input.size(), source.size() - source_position);
|||    input.truncate(count) = source.slice(source_position, count);
|||    source_position += count;
|||    return count;
|||};
|||const auto write_output = [&](univector_ref<const fbase> samples) {
|||    written += samples.size();
|||};

for (;;) {
    const size_t requested_input = src.input_size_for_output(output_chunk_size);
    univector<fbase> input(requested_input);
    const size_t frames_read = read_next_input(input);
    if (frames_read == 0)
        break;

    const size_t frames_to_write = frames_read == requested_input
        ? output_chunk_size
        : src.output_size_for_input(frames_read);
    src.process(output.truncate(frames_to_write).ref(), input.truncate(frames_read));
    write_output(output.truncate(frames_to_write));

    if (frames_read < requested_input)
        break;
}
|||CHECK(written > 0);
|||}
```

The same rule applies to multichannel file I/O: request the input frame count from one converter, read and deinterleave that many frames, then call `process` once for each channel with the corresponding channel range. Each channel's converter has the same positions, so all channels use the same input and output frame counts.

### Delay compensation

[[`samplerate_converter<T>::get_delay`:nosig:noscope]] reports the FIR delay in **output samples**. Delay compensation discards those initial output positions so that the first written output sample is aligned with the original signal timeline.

Compensation is only possible when the required future input is already available. To omit $D$ delayed output samples, the converter must first consume the input samples that correspond to those $D$ output positions. This is natural when converting an in-memory signal or a file, and when a buffered stream has enough look-ahead. It may be inappropriate for a low-latency live stream: retaining the delay preserves immediate output, while compensating it requires waiting for the needed input and drops the initial output interval.

Use [[`samplerate_converter<T>::skip`:nosig:noscope]] once, before the first `process` call. Its argument is an **output** count; its return value is the number of **input** samples consumed. They are not generally equal. The first call to `process` must receive only the unconsumed suffix:

```c++
|||TEST_CASE("dsp/src.md/delay compensation")
|||{
const size_t input_sr = 44100;
const size_t output_sr = 96000;
const size_t output_size = 512;
auto src = samplerate_converter<fbase>(resample_quality::high, output_sr, input_sr);
const size_t delay = src.get_delay();              // Output samples to discard
const size_t input_for_delay = src.input_size_for_output(delay);

univector<fbase> input(input_for_delay + src.input_size_for_output(output_size));
input = scalar(fbase(1)); // Must include the future samples needed after the delay
univector<fbase> output(output_size);

const size_t consumed_input = src.skip(delay, input);
src.process(output, input.slice(consumed_input));
|||CHECK(consumed_input == input_for_delay);
|||CHECK(output.size() == output_size);
|||}
```

Calling `process(output, input)` after `skip(delay, input)` is incorrect. It supplies samples already consumed by `skip`, while the converter's state has advanced beyond them. The first output block is then corrupted and may cause a click or discontinuity at a later chunk boundary. Pass `input.slice(consumed_input)` directly to `process`; do not assign the slice to an owning container or a value-like view whose assignment may copy data instead of selecting the range.

The following chunked pull loop adds compensation to the non-compensated workflow. It reads enough input for the first output chunk **and** the delayed output positions that will be discarded. Later chunks use the ordinary workflow.

```c++
|||TEST_CASE("dsp/src.md/compensated chunks")
|||{
constexpr size_t output_chunk_size = 16384;
const size_t input_sr = 44100;
const size_t output_sr = 96000;
auto src = samplerate_converter<fbase>(resample_quality::high, output_sr, input_sr);
const size_t delay = src.get_delay();
const size_t input_delay = src.input_size_for_output(delay);

univector<fbase> output(output_chunk_size);
bool first_chunk = true;
|||univector<fbase> source(2 * output_chunk_size, fbase(1));
|||size_t source_position = 0;
|||size_t written = 0;
|||const auto read_next_input = [&](univector_ref<fbase> input) {
|||    const size_t count = std::min(input.size(), source.size() - source_position);
|||    input.truncate(count) = source.slice(source_position, count);
|||    source_position += count;
|||    return count;
|||};
|||const auto write_output = [&](univector_ref<const fbase> samples) {
|||    written += samples.size();
|||};

for (;;) {
    const size_t requested_input = src.input_size_for_output(
        output_chunk_size + (first_chunk ? delay : 0));
    univector<fbase> input(requested_input);
    const size_t frames_read = read_next_input(input);
    if (frames_read == 0)
        break;

    if (first_chunk) {
        if (frames_read <= input_delay)
            break; // No aligned output sample can be produced.
        const size_t consumed = src.skip(delay, input.truncate(frames_read));
        const size_t available_input = frames_read - consumed;
        const size_t frames_to_write = frames_read == requested_input
            ? output_chunk_size
            : src.output_size_for_input(available_input);
        src.process(output.truncate(frames_to_write).ref(), input.slice(consumed));
        write_output(output.truncate(frames_to_write));
    } else {
        const size_t frames_to_write = frames_read == requested_input
            ? output_chunk_size
            : src.output_size_for_input(frames_read);
        src.process(output.truncate(frames_to_write).ref(), input.truncate(frames_read));
        write_output(output.truncate(frames_to_write));
    }

    if (frames_read < requested_input)
        break;
    first_chunk = false;
}
|||CHECK(written > 0);
|||CHECK(!first_chunk);
|||}
```

Processing in chunks keeps memory use bounded regardless of file length and, because the converter state persists across iterations, keeps the resampled output continuous at chunk boundaries.

## Troubleshooting

### A click or discontinuity occurs at the first chunk boundary

Verify that startup delay is handled exactly once. After `skip`, advance the input range by the **returned** consumed-input count before calling `process`. Do not use `get_delay()` itself as the input offset, and do not pass the complete first input chunk to both calls.

### The output slowly drifts, has missing samples, or has duplicate samples

Do not derive every chunk size from `input_rate / output_rate`. Fractional positions accumulate across calls. Query `input_size_for_output` or `output_size_for_input` from the same converter immediately before each operation, then advance the external source by the count returned from `process`.

### Each block sounds like a separate signal

Create one converter per channel and keep each converter alive for the entire stream. Constructing a new converter, calling `reset()`, or sharing one converter between channels loses the independent history each continuous channel requires.

### The final block is too long or contains an unexpected transient

At end of input, calculate the final output size from the available, unconsumed input with `output_size_for_input`. The polyphase converter has no end-of-stream flush operation; it cannot synthesize future input samples needed to complete the FIR tail. Decide explicitly whether to stop at the last fully supported output sample or to pad the source signal before conversion.

## Complete Working Example

For a complete, ready-to-run example that reads an input file, resamples it chunk-by-chunk, and writes the result to an output file, see `tools/sample_rate_converter.cpp`. It demonstrates the full workflow above, including file I/O, multichannel handling, and chunked processing with [[`samplerate_converter`:nosig]].

## See Also

- [Gallery with results of applying various sample rate conversion presets](examples/src_gallery.md)
- [FFT-based sample rate conversion](../dft/dft_resampler.md)

