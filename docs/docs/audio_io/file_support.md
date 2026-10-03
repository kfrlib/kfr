# Audio Format Support in KFR

## Overview

KFR provides support for both raw and encoded audio formats. It handles a range of PCM bit depths, endianness, and container types.

---

## Uncompressed Audio

### Integer PCM

* [Bit depth](../advanced/dsp_glossary.md#bit-depth): 16–32 bits
* Signed only
* Endianness: Little and Big endian
* Channels: Any number (concrete formats may impose limits)

### Floating-Point PCM

* 32-bit and 64-bit supported
* Endianness: Little and Big endian
* Channels: Any number

---

## Compressed Audio

### FLAC

* Decoding and encoding supported
* Implementation uses **libFLAC**

### ALAC (Apple Lossless)

* Decoding and encoding supported
* Bit depths: 16, 20, 24, and 32-bit
* Up to 8 channels supported

### MP3

* Decoding only

### System Codecs

Proprietary or otherwise platform-specific formats, including AAC where supported by the operating system, can be decoded with the platform system decoder:

* On Windows, use [[`create_mediafoundation_decoder`]] (Media Foundation).
* On Apple platforms, use [[`create_coreaudio_decoder`]] (Core Audio).

These decoders are created explicitly rather than selected by [[`create_decoder_for_file`]], and the exact supported formats depend on the installed operating-system codecs.

---

## Raw Audio

All PCM formats are supported for raw data streams.

* Decoding and encoding supported

---

## Container Formats

| Container        | Supported Content                      |
| ---------------- | -------------------------------------- |
| **WAVE (.wav)**  | All PCM formats                        |
| **AIFF (.aiff)** | All PCM formats                        |
| **W64 (.w64)**   | All PCM formats                        |
| **RF64 (.rf64)** | All PCM formats                        |
| **BW64 (.bw64)** | All PCM formats                        |
| **CAFF (.caf)**  | All PCM formats and ALAC encoded audio |
| **MP3 (.mp3)**   | MP3 encoded audio                      |
| **FLAC (.flac)** | FLAC encoded audio                     |

---

## RIFF-Like Containers

KFR supports:

* Reading of arbitrary RIFF chunks
* Copying chunks directly from decoder to encoder
