# Multi-channel Audio Filtering

[[`audio_filter`:nosig]] applies a stateful FIR, IIR, or convolution filter independently to every channel of planar [[`audio_data_planar`:nosig]]. It owns one [[`filter<fbase>`:nosig]] instance per channel, so channel histories remain isolated while streaming audio blocks are processed.

Include `<kfr/audio.hpp>` for the audio filtering API.

## Creating a filter

Use the `fir`, `iir`, or `convolution` factory and pass the number of channels to process. Every factory initializes an equivalent independent filter for each channel. The channel count must be greater than zero and no greater than [[`max_audio_channels`:nosig]].

```c++
|||#include <kfr/audio.hpp>
|||using namespace kfr;
|||TEST_CASE("audio_filter.md/FIR filtering")
|||{
|||univector<fbase> taps{ 0.25, 0.5, 0.25 };
|||audio_data_planar audio(2, 4, 0.0);
|||audio.channel(0)[0] = 1.0;
|||audio.channel(1)[0] = 10.0;

audio_filter filter = audio_filter::fir(audio.channels, taps);
filter.apply(audio);

|||CHECK(audio.channel(0)[0] == 0.25);
|||CHECK(audio.channel(1)[0] == 2.5);
|||}
```

The available factories are:

| Factory | Per-channel filter |
| --- | --- |
| [[`audio_filter::fir`:nosig]] | FIR filter configured from [[`fir_params<T>`:nosig]] taps. |
| [[`audio_filter::iir`:nosig]] | IIR filter configured from dynamic IIR parameters or one [[`biquad_section<T>`:nosig]]. |
| [[`audio_filter::convolution`:nosig]] | Streaming overlap-add FFT convolution configured with an impulse response and optional block size. |

## Applying and resetting

[[`audio_filter::apply(audio_data_planar &)`:noscope]] processes planar data in place. [[`audio_filter::apply(audio_data_planar &, const audio_data_planar &)`:noscope]] writes to a distinct planar destination; source and destination must have the same frame count. In either case, both buffers must have exactly the channel count used to construct the filter.

Like a scalar runtime filter, `audio_filter` retains state between calls. Keep the same object for successive blocks of the same stream. Call [[`audio_filter::reset`:noscope]] before starting an unrelated stream to reset the state of every channel filter.

```c++
|||TEST_CASE("audio_filter.md/streaming and destination")
|||{
|||auto section = biquad_lowpass<fbase>(1000.0 / 48000.0, 0.707);
|||audio_filter filter = audio_filter::iir(2, section);

|||audio_data_planar block1(2, 256, 1.0);
|||audio_data_planar block2(2, 256, 1.0);
|||audio_data_planar output(2, block2.size);
filter.apply(block1);
filter.apply(output, block2); // continues each channel's state from block1

filter.reset();
|||CHECK(output.channel_count() == 2);
|||}
```

`audio_filter` requires planar audio. Convert interleaved input before filtering and convert it back afterward when required by an encoder or audio device:

```c++
|||TEST_CASE("audio_filter.md/converting interleaved audio")
|||{
|||audio_data_interleaved interleaved(2, 4, 1.0);
audio_data_planar planar = interleaved;
audio_filter filter = audio_filter::fir(2, univector<fbase>{ 1.0 });
filter.apply(planar);
interleaved = planar;
|||CHECK(interleaved.channel_count() == 2);
|||}
```

## See also

- [Audio buffers and formats](audio_data.md)
- [Stateful filtering](../dsp/filters.md)
- [FIR filters](../dsp/fir.md)
- [IIR filters](../dsp/iir.md)
- [Convolution filter details](../dft/convolution.md)
