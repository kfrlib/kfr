# Audio Buffers, PCM Samples, and File Formats

KFR keeps the representation used for DSP separate from the representation used
on disk. [[`audio_data<IsInterleaved>`:nosig]] holds normalized floating-point
samples for processing. [[`audiofile_format`:nosig]] describes an encoded audio
stream: its container, codec, sample representation, rate, layout, and
metadata. The decoder and encoder APIs described in [Reading and Writing Audio
Files](read_audio.md) connect the two.

Include `<kfr/audio.hpp>` for the complete audio I/O API.

## Frames, channels, and layouts

A **frame** is one sample from every channel at one instant. A stereo buffer
with 48,000 frames therefore contains 48,000 left samples and 48,000 right
samples, or 96,000 scalar samples in total. In an [[`audio_data<IsInterleaved>`:nosig]]
object, `size` and `capacity` are measured in frames, while
[[`audio_data<IsInterleaved>::total_samples`:noscope]] returns the scalar-sample
count.

KFR provides two aliases for the two common layouts:

| Type                               | Storage                                                                           | Best fit                                                                   |
|------------------------------------|-----------------------------------------------------------------------------------|----------------------------------------------------------------------------|
| [[`audio_data_planar`:nosig]]      | One contiguous buffer per channel: $L_0, L_1, \ldots$ and $R_0, R_1, \ldots$. | Per-channel DSP and independent channel processing.                        |
| [[`audio_data_interleaved`:nosig]] | One contiguous buffer ordered by frame: $L_0, R_0, L_1, R_1, \ldots$.           | File I/O, device callbacks, and APIs that already use interleaved buffers. |

Both layouts store [[`fbase`:nosig]] values, which are normalized floating-point
samples (`float` or `double`, depending on the KFR build). Integer PCM is only
used at an I/O boundary and is converted with [[`samples_load(fbase *, const Tin *, size_t, bool)`:nosig]]
and [[`samples_store(Tout *, const fbase *, size_t, const audio_quantization &, bool)`:nosig]].

The maximum channel count is [[`max_audio_channels`:nosig]] (16 by default).
Defining `KFR_MAX_AUDIO_CHANNELS` changes it at build time; the supported range
is 2 through 64.

### Example: planar processing, interleaved output

Planar storage lets each channel participate directly in KFR expressions. This
example creates one second of silent stereo audio, applies a gain to each
channel, and makes an interleaved copy for an encoder or audio device.

```c++
|||#include <array>
|||#include <stdexcept>
|||#include <vector>
|||#include <kfr/audio.hpp>
|||#include <iostream>
#include <kfr/audio.hpp>|||
using namespace kfr;
|||TEST_CASE("audio_data.md/planar processing and interleaved output")
|||{

constexpr size_t sample_rate = 48000;

audio_data_planar channels(2, sample_rate, 0.0f);

// Generate or load samples into channels.channel(0) and channels.channel(1).
for (size_t ch = 0; ch < channels.channel_count(); ++ch)
    channels.channel(ch) *= 0.5f;

audio_data_interleaved output = channels; // Copies and interleaves samples
|||CHECK(output.channel_count() == 2);
|||CHECK(output.size == sample_rate);
|||CHECK(output.is_silent());
|||}
```

The conversion constructor is a data conversion, not a different view of the
same storage. Constructing [[`audio_data_planar`:nosig]] from an interleaved
buffer deinterleaves it; constructing [[`audio_data_interleaved`:nosig]] from a
planar buffer interleaves it.

## Constructing and owning audio buffers

The allocating constructors take `(channels, frames)`. Their memory is
SIMD-friendly aligned; planar buffers also align each channel independently.
The two-argument form deliberately leaves samples uninitialized. Pass a value
or call [[`audio_data<IsInterleaved>::fill`:noscope]] when silence or another known
initial value is required.

```c++
|||TEST_CASE("audio_data.md/constructing owned buffers")
|||{
audio_data_planar scratch(2, 4096);            // Samples are uninitialized
audio_data_planar silence(2, 4096, 0.0f);      // Every sample is zero
audio_data_interleaved signal(2, 4096, 0.25f); // Every sample is 0.25

signal.multiply(0.5f); // Scales every channel in place
|||CHECK(silence.is_silent());
|||CHECK(signal.stat().peak == 0.125f);
|||}
```

The default constructor creates an empty buffer. [[`audio_data<IsInterleaved>::clear`:noscope]]
sets its frame count to zero while retaining the allocation; [[`audio_data<IsInterleaved>::reset`:noscope]]
returns it to the default empty state and releases owned storage when no other
view refers to it. [[`audio_data<IsInterleaved>::resize(size_t, fbase)`:noscope]]
initializes a newly appended region, whereas [[`audio_data<IsInterleaved>::resize(size_t)`:noscope]]
does not.

### Wrapping external storage

The view constructors avoid a copy. For interleaved audio, pass one pointer,
the channel count, and the frame count. For planar audio, pass a span of one
pointer per channel. These constructors are non-owning: the caller must keep
the wrapped memory alive.

```c++
|||TEST_CASE("audio_data.md/wrapping external storage")
|||{
#include <array>|||
#include <vector>|||

std::vector<fbase> device_buffer(2 * 1024);
audio_data_interleaved device_audio(device_buffer.data(), 2, 1024);

std::vector<fbase> left(1024);
std::vector<fbase> right(1024);
std::array<fbase*, 2> planes = { left.data(), right.data() };
audio_data_planar planar_audio(planes, left.size());
|||CHECK(device_audio.channel_count() == 2);
|||CHECK(planar_audio.pointers()[0] == left.data());
|||}
```

An overload with a callable finalizer makes the buffer own external storage.
The finalizer runs after the last buffer or view sharing it is destroyed.

```c++
|||TEST_CASE("audio_data.md/owning external storage")
|||{
fbase* samples = new fbase[2 * 1024];
audio_data_interleaved audio(samples, 2, 1024,
                             [samples] { delete[] samples; });
|||CHECK(audio.total_samples() == 2 * 1024);
|||}
```

Copies of the same layout are shallow: they copy the pointers and shared
ownership state, so modifying samples through either object modifies the same
storage. The same is true for slices. Make an explicitly allocated buffer and
copy its samples when independent storage is needed.

## Accessing samples and channels

[[`audio_data<IsInterleaved>::channel`:noscope]] returns a one-dimensional KFR
expression for a channel. For [[`audio_data_planar`:nosig]], it is a contiguous
[[`univector_ref`:nosig]]; for [[`audio_data_interleaved`:nosig]], it is a
[[`strided_channel<T>`:nosig]] that visits every `channels`-th element. The
strided form is still suitable for ordinary expression assignments and DSP
operations, but it is not contiguous.

```c++
|||TEST_CASE("audio_data.md/accessing interleaved channels")
|||{
audio_data_interleaved stereo(2, 4, 0.0f);
stereo.interlaved() = univector<fbase>{ 1, 10, 2, 20, 3, 30, 4, 40 };

// Left and right are strided views: { 1, 2, 3, 4 } and { 10, 20, 30, 40 }.
stereo.channel(0) *= 0.5f;
stereo.channel(1) *= 0.25f;
|||CHECK(stereo.interlaved()[0] == 0.5f);
|||CHECK(stereo.interlaved()[1] == 2.5f);
|||CHECK(stereo.interlaved()[6] == 2.0f);
|||CHECK(stereo.interlaved()[7] == 10.0f);
|||}
```

[[`audio_data<IsInterleaved>::interlaved`:noscope]] is available only for the
interleaved layout and returns a contiguous view containing `size * channels`
scalars. [[`audio_data<IsInterleaved>::pointers`:noscope]] is available only for
the planar layout and returns the per-channel pointer array for APIs that need
it.

The two overloads of [[`audio_data<IsInterleaved>::for_channel`:noscope]] are
primarily storage traversal helpers. For planar data they call the callback for
each logical channel. For interleaved data they call it **once** for the whole
interleaved buffer, not once per channel. Use [[`audio_data<IsInterleaved>::channel`:noscope]]
when an operation must address individual interleaved channels.

## Views, capacity, and stream assembly

[[`audio_data<IsInterleaved>::slice(size_t, size_t)`:noscope]] returns a shallow
frame-range view. Its requested length is clamped to the available frames, and
the returned view's `position` advances by `start`. [[`audio_data<IsInterleaved>::truncate(size_t)`:noscope]]
is the convenient `slice(0, length)` form.

```c++
|||TEST_CASE("audio_data.md/slicing frame ranges")
|||{
audio_data_planar recording(2, 48000, 0.0f);
recording.position = 96000; // This block starts at frame 96000 in its stream.

audio_data_planar first_100_ms = recording.truncate(4800);
audio_data_planar next_100_ms  = recording.slice(4800, 4800);
// first_100_ms.position == 96000; next_100_ms.position == 100800
|||CHECK(first_100_ms.position == 96000);
|||CHECK(next_100_ms.position == 100800);
|||CHECK(first_100_ms.size == 4800);
|||CHECK(next_100_ms.size == 4800);
|||}
```

Growing [[`audio_data<IsInterleaved>::resize(size_t)`:noscope]] or
[[`audio_data<IsInterleaved>::reserve`:noscope]] can reallocate the backing store,
invalidating pointers and views into it. Reserve capacity before creating
long-lived views. [[`audio_data<IsInterleaved>::slice_past_end`:noscope]] is an
advanced helper for a producer that wants a writable view after the current
end: it reserves enough storage and returns the view, but it does **not** grow
the parent buffer's `size`. After filling the returned view, grow the parent to
commit those frames.

[[`audio_data<IsInterleaved>::append`:noscope]] and
[[`audio_data<IsInterleaved>::prepend`:noscope]] copy frames and can convert the
source layout. The buffers must represent the same number of channels;
matching channel counts are a caller precondition. `prepend` also decreases
the destination's `position` by the number of inserted frames, while `append`
leaves it unchanged.

## Measuring a buffer

[[`audio_data<IsInterleaved>::stat`:noscope]] returns an [[`audio_stat`:nosig]]
with two aggregate measurements:

- `peak` is the largest absolute scalar sample across every channel.
- `rms` is the root-mean-square value across every scalar sample and channel.

[[`audio_data<IsInterleaved>::is_silent(fbase)`:noscope]] tests whether every
sample lies in the inclusive range $[-threshold, threshold]$. The default
threshold is $10^{-5}$; a sample exactly equal to the threshold is silent.
[[`audio_data<IsInterleaved>::find_peak`:noscope]] returns a **frame** index. It
chooses the frame with the largest sum of absolute channel values, which is
useful for locating a multichannel transient rather than a single-channel
maximum.

```c++
|||TEST_CASE("audio_data.md/measuring a buffer")
|||{
audio_data_planar channels(2, 4, 0.0f);
channels.channel(1)[2] = 0.5f;

const audio_stat level = channels.stat();
if (!channels.is_silent())
    std::cout << "peak frame: " << channels.find_peak()
              << ", peak: " << level.peak << '\n';
|||CHECK(level.peak == 0.5f);
|||CHECK(channels.find_peak() == 2);
|||}
```

## Describing an encoded stream with `audiofile_format`

[[`audiofile_format`:nosig]] describes audio outside the floating-point DSP
buffer. A decoder returns the detected format when it opens a file; an encoder
uses a format supplied by the caller. Its main fields are:

| Field                        | Meaning                                                                                     |
|------------------------------|---------------------------------------------------------------------------------------------|
| `container`                  | [[`audiofile_container`:nosig]], such as WAVE, W64, RF64, FLAC, CAF, AIFF, or MP3.          |
| `codec`                      | [[`audiofile_codec`:nosig]], such as linear PCM, IEEE floating point, FLAC, ALAC, or MP3.   |
| `endianness` and `bit_depth` | [[`audiofile_endianness`:nosig]] and the encoded sample representation.                     |
| `channels` and `sample_rate` | Channel count and rate in hertz.                                                            |
| `speakers`                   | A [[`speaker_arrangement`:nosig]] that records a known channel order when one is available. |
| `total_frames`               | Total frame count when it is known.                                                         |
| `metadata`                   | A [[`metadata_map`:nosig]] of string key/value pairs.                                       |

For example, this describes 16-bit, little-endian stereo PCM in a WAVE
container:

```c++
|||TEST_CASE("audio_data.md/describing an encoded stream")
|||{
audiofile_format format;
format.container   = audiofile_container::wave;
format.codec       = audiofile_codec::lpcm;
format.endianness  = audiofile_endianness::little;
format.bit_depth   = 16;
format.channels    = 2;
format.sample_rate = 48000;
format.speakers    = speaker_arrangement::Stereo;

if (!format.valid())
    throw std::runtime_error("invalid audio format");
|||CHECK(format.bytes_per_pcm_frame() == 4);
|||CHECK(format.sample_type() == audio_sample_type::i16);
|||}
```

[[`audiofile_format::valid`:noscope]] checks the basic channel, rate, codec, and
bit-depth constraints. It does not establish that every container accepts every
codec, so select a supported combination from [Audio Format Support](file_support.md).
[[`audiofile_format::bytes_per_pcm_frame`:noscope]] returns
$channels \times \lceil bit\_depth / 8 \rceil$ for encoded PCM storage.

[[`audiofile_format::sample_type`:noscope]] maps supported linear PCM and IEEE
float formats to [[`audio_sample_type`:nosig]] (`i16`, `i24`, `i32`, `f32`, or
`f64`), or `unknown` where there is no direct memory sample type.
[[`audiofile_format::sample_type_lpcm`:noscope]] performs the integer-depth part
of that mapping without considering the codec. [[`audio_sample_bit_depth(audio_sample_type)`:nosig]]
and [[`audio_sample_is_float(audio_sample_type)`:nosig]] are useful when code
needs to inspect such a runtime tag.

Use [[`arrangement_speakers(speaker_arrangement)`:nosig]] to obtain the ordered
speaker list for a declared arrangement, or [[`arrangement_for_channels(size_t)`]]
to choose KFR's default predefined arrangement for a channel count.

## Converting PCM data

[[`samples_load(fbase *, const Tin *, size_t, bool)`:nosig]] converts a
contiguous encoded sample sequence to normalized [[`fbase`:nosig]]. Its planar
overload deinterleaves an encoded buffer while it converts. The corresponding
[[`samples_store(Tout *, const fbase *const *, size_t, size_t, const audio_quantization &, bool)`:nosig]]
overloads interleave planar floating-point channels into an encoded buffer.
Runtime overloads accept an [[`audio_sample_type`:nosig]] and `std::byte*` when
the sample representation is known only after parsing a format.

For integer output, conversion clamps the floating-point value to $[-1, 1]$
before scaling and rounding. The positive endpoint maps to the largest positive
integer value. Pass `true` for `swap_bytes` when the encoded byte order differs
from the host representation.

[[`audio_quantization`:nosig]] adds dither before integer quantization. Construct
it with the target bit depth and an [[`audio_dithering`:nosig]] method:
`none`, `rectangular`, or `triangular`. The dither scale is one quantization
step ($1 / 2^{bit\_depth}$). Use this overload when manually reducing integer
bit depth; ordinary file encoding selects its dither behavior through
[[`audio_encoding_options`:nosig]].

```c++
|||TEST_CASE("audio_data.md/converting PCM data")
|||{
audio_data_planar source(2, 256, 0.0f);
std::vector<i16> pcm(source.total_samples());

audio_quantization quantization(16, audio_dithering::triangular);
samples_store(pcm.data(), source.pointers(), source.channels, source.size,
              quantization);
|||CHECK(pcm.size() == source.total_samples());
|||}
```

## Modern and legacy format APIs

For new code, use [[`audiofile_format`:nosig]] with [[`audio_decoder`:nosig]]
and [[`audio_encoder`:nosig]]. The older [[`audio_format`:nosig]] and
[[`audio_format_and_length`:nosig]] types belong to the previous reader/writer
API. They expose only a channel count, sample type, sample rate, W64 flag, and
sample length, and cannot represent modern container, codec, metadata, or
speaker information. Migrate old code to the current decoder and encoder API
rather than introducing the legacy types into a new pipeline.

## See also

- [Reading and Writing Audio Files](read_audio.md) — decoder/encoder lifetime,
  chunked I/O, raw streams, and one-call convenience functions.
- [Audio Format Support](file_support.md) — supported container, codec, and
  bit-depth combinations.
- [Sample-rate conversion](../dsp/src.md#processing-streams-and-files-in-chunks)
  — processing file data in bounded-size blocks.
