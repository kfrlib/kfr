# How to measure loudness according to EBU R 128

KFR implements EBU R128 compliant loudness metering through the `ebu_r128<T>` class in the DSP module. It performs momentary (M), short-term (S), and integrated (I) loudness evaluation along with loudness range (LRA) as defined by EBU Tech 3341.

---

### 1. Initialization

Create a loudness analyzer with sample rate and channel layout:

```cpp
ebu_r128<float> loudness(48000, arrangement_speakers(arrangement_for_channels(channel_number)), /* packet_size_factor */ 1);
```

The channel arrangement is automatically derived from the number of channels (1–8). Supported configurations include mono, stereo, and multichannel.

`packet_size_factor` sets the processing block size:
* __1__ = 100ms (default), 4800 samples at 48kHz
* __2__ = 50ms, 2400 samples at 48kHz
* __3__ = 33ms, 1600 samples at 48kHz
* __4__ = 25ms, 1200 samples at 48kHz

---

### 2. Feeding Data

Process audio in fixed-size packets determined by `loudness.packet_size()`:

```cpp
loudness.process_packet({ left, right }); // stereo example
```

Each channel is supplied as a `univector_ref<float>` or similar slice of contiguous samples. For file-based use, ensure data is deinterleaved before passing.

---

### 3. Reading Results

After each packet, retrieve metrics:

```cpp
float M, S, I, RL, RH;
loudness.get_values(M, S, I, RL, RH);
```

* **M** — Momentary loudness (400 ms window)
* **S** — Short-term loudness (3 s window)
* **I** — Integrated loudness (gated average)
* **RL**, **RH** — Lower and upper loudness range boundaries
* **LRA = RH − RL**

At least 1.5 s of silence must follow the signal before final measurement to ensure correct gating.

---

### 4. Example (Raw File Measurement)

`ebu_test.cpp` provides a full working example:

```bash
ebu_test input.raw 2
```

Reads 32-bit float interleaved audio, deinterleaves by channel, processes through `ebu_r128`, and prints:

```
M = -23.0
S = -23.1
I = -23.0
LRA = 2.1
```

---

### 5. Verification

`ebu.cpp` includes reference tests validating compliance with EBU R128 for stereo and multichannel inputs at 44.1 kHz and 48 kHz. These confirm numerical alignment within ±0.1 LU across configurations.
