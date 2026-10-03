# FFT-Based Sample Rate Conversion

[[`dft_resampler`:nosig]] resamples real-valued signals by applying a low-pass filter in the frequency domain with an [FFT](../advanced/dsp_glossary.md#dft-vs-fft) overlap-save algorithm. It is designed for power-of-two conversion ratios: use a positive shift to upsample and a negative shift to downsample.

For long filters, this block-based approach can be faster than the sample-by-sample polyphase implementation in [[`samplerate_converter`:nosig]]. Use [[`samplerate_converter`:nosig]] when the ratio is not a power of two or when sample-based processing is required; see [Sample Rate Conversion](../dsp/src.md).

| Feature          | [[`samplerate_converter`:nosig]] | [[`dft_resampler`:nosig]] |
|:-----------------|:---------------------------------|:--------------------------|
| Algorithm        | Polyphase FIR                    | FFT-based overlap-save    |
| Resampling ratio | Any rational ratio               | Power of two ($2^n$)      |
| Processing       | Sample-based                     | Block-based               |

## Creating a Resampler

Configure the conversion with [[`dft_resampler_params`:nosig]]. Its `shift` argument specifies the ratio: `1` is 2× upsampling, `-1` is 2× downsampling, and `0` applies only the filter. The remaining parameters control the filter cutoff, stopband attenuation, and transition width.

```c++
|||#include <kfr/dft.hpp>
|||using namespace kfr;
|||TEST_CASE("dft/dft_resampler.md/upsample one frame")
|||{
// 2x upsampling (shift = 1)
dft_resampler_params params(1);
dft_resampler<float> resampler(params);
```

## Processing Blocks and Streams

[[`dft_resampler<T>::process_frame`:noscope]] processes one explicit overlap-save frame. The input buffer must contain [[`dft_resampler<T>::input_block_size()`:noscope]] samples, and the output buffer must hold [[`dft_resampler<T>::output_hop()`:noscope]] samples.

```c++
univector<float> input(resampler.input_block_size(), 1.0f);
univector<float> output(resampler.output_hop());

// Process one input frame.
resampler.process_frame(output.data(), input.data());
|||CHECK(output.size() == resampler.output_hop());
|||CHECK_THAT(output[0], Catch::Matchers::WithinAbs(1.0f, 1e-4f));
|||}
```

For input that does not already arrive in complete frames, use [[`dft_resampler<T>::process`:noscope]]. It accumulates samples into [[`dft_resampler<T>::input_hop()`:noscope]]-sized hops and calls [[`dft_resampler<T>::process_frame`:noscope]] for each complete frame. Each complete input hop produces one output hop; ensure the output span has room for all produced samples.

See [Fast Fourier Transform with KFR](dft.md) for the underlying DFT API.
