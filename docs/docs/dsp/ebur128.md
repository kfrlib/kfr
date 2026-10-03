# Measuring Loudness with EBU R128

KFR implements [EBU R128](../advanced/dsp_glossary.md#ebu-r128) compliant loudness metering through the [[`ebu_r128<T>`]] class. It computes momentary (M), short-term (S), and integrated (I) loudness, along with loudness range (LRA), as defined by EBU Tech 3341 and EBU R128 s3.

---

### 1. Initialization

Create a loudness analyzer with sample rate and channel layout:

```c++
#include <kfr/dsp.hpp>

using namespace kfr;

|||TEST_CASE("dsp/ebur128.md/initialization")
|||{
constexpr size_t channel_count = 2;
ebu_r128<float> loudness(48000, arrangement_speakers(arrangement_for_channels(channel_count)), /* packet_size_factor */ 1);
|||    CHECK(loudness.count() == channel_count);
|||    CHECK(loudness.packet_size() == 4800);
|||}
```

The channel arrangement is automatically derived from the number of channels (1–8). Supported configurations include mono, stereo, and multichannel.

`packet_size_factor` sets the processing block size (valid range 1–6):

* __1__ = 100 ms (default, 10 Hz refresh), 4800 samples at 48 kHz
* __2__ = 50 ms (20 Hz refresh), 2400 samples at 48 kHz
* __3__ = 33 ms (30 Hz refresh), 1600 samples at 48 kHz
* __4__ = 25 ms (40 Hz refresh), 1200 samples at 48 kHz

---

### 2. Feeding Data

Process audio in fixed-size packets determined by [[`ebu_r128<T>::packet_size`:nosig:noscope]]:

```c++
|||TEST_CASE("dsp/ebur128.md/feeding-data")
|||{
const std::array channels{ speaker_type::Left, speaker_type::Right };
ebu_r128<float> loudness(48000, channels);
|||univector<float> left(loudness.packet_size(), 0.25f);
|||univector<float> right(loudness.packet_size(), 0.25f);
loudness.process_packet({ left, right }); // stereo example
|||    CHECK(loudness.count() == 2);
|||}
```

Each channel is supplied as a [[`univector_ref`]] or similar slice of contiguous samples. For file-based use, ensure data is deinterleaved before passing.

---

### 3. Reading Results

After each packet, retrieve metrics:

```c++
|||TEST_CASE("dsp/ebur128.md/reading-results")
|||{
const std::array channels{ speaker_type::Left, speaker_type::Right };
ebu_r128<float> loudness(48000, channels);
|||univector<float> left(loudness.packet_size(), 0.25f);
|||univector<float> right(loudness.packet_size(), 0.25f);
|||    for (int packet = 0; packet < 30; ++packet)
|||        loudness.process_packet({ left, right });
float M, S, I, RL, RH;
loudness.get_values(M, S, I, RL, RH);
|||    CHECK(std::isfinite(M));
|||    CHECK(std::isfinite(S));
|||    CHECK(std::isfinite(I));
|||}
```

* **M** — Momentary loudness (400 ms window)
* **S** — Short-term loudness (3 s window)
* **I** — Integrated loudness (gated average)
* **RL**, **RH** — 10th and 95th percentile loudness, the LRA boundaries
* **LRA = RH − RL**

EBU R128 measurements settle only after the signal ends; append at least 1.5 s of silence (roughly 15 packets at the default packet size) before reading the final `I` and LRA values.

---

### 4. Example (Command-Line Tool)

`tools/ebu_test.cpp` is a full working example: `ebu_test INPUT_FILE`. It opens the file with [[`create_decoder_for_file`:nosig]] (so it accepts WAV, RF64, BW64, W64, FLAC, MP3, AIFF, or CAF), builds an [[`ebu_r128`:nosig]] meter sized for the file's channel count via `arrangement_speakers(arrangement_for_channels(channels))`, feeds it packet-by-packet as the file is decoded, appends 1.5 s of silence at the end, and prints the final measurements together with the running peak M and S values:

```
M = -23.0
S = -23.1
I = -23.0
LRA = 2.1
maxM = -20.4
maxS = -21.2
```

---

### 5. Verification

`tests/unit/dsp/ebu.cpp` checks measured values against reference numbers for stereo and 5-channel inputs at 44.1 kHz and 48 kHz, within ±0.05 LU for stereo and ±0.1 LU for multichannel.
