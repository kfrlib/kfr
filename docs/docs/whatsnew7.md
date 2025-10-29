# KFR 7 — Major Update

KFR 7 introduces a range of new capabilities, platform support, and performance improvements for modern audio and signal processing workflows.

## Reworked Audio I/O

Audio input and output have been completely reworked.
The new system supports WAV, AIFF, Apple CAF, FLAC, raw formats, and MP3 reading. It is more reliable, faster, and tolerant to format variations.

Highlights:

* Read and write directly from memory or custom streams
* Handle multiple bit depths and floating-point formats
* Work with planar or interleaved audio
* Built-in dithering
* Reading arbitrary chunks from RIFF-based formats
* Full UTF-8 file path support on Windows

## RISC-V Architecture Support

KFR now runs on RISC-V, with optimized implementations that take advantage of the new vector extensions for better performance on emerging hardware.

## Elliptic Filters

A new family of elliptic filters provides sharper frequency roll-off than Butterworth or Chebyshev designs, offering higher precision for demanding applications.

## Universal macOS Binaries

KFR can now build universal binaries that run natively on both Intel and Apple Silicon (ARM64) Macs.
These are built automatically and available for download.

## Zero-Phase IIR Filtering

A new zero-phase IIR filter applies forward-backward processing for phase-accurate results, useful in offline and analysis workflows.

## C++20 as the New Baseline

KFR now requires C++20, unlocking cleaner syntax and better use of modern language features.

## New High-Level Audio Module

A new `audio` module simplifies working with multi-channel data.
The `audio_data` class provides a unified interface for:

* Format conversion
* Interleaving and deinterleaving
* Zero-copy construction from user-provided pointers

## Additional Improvements

* Scoped control over denormal flushing for stable floating-point behavior
* Managed memory allocation turned off by default for better speed (can be re-enabled via `KFR_MANAGED_ALLOCATION`)
* All tests migrated to Catch2
* Expanded documentation with more examples and tutorials

## Summary of Progress Since Version 6

* Multidimensional DFT via the C API
* DFT performance improved by up to 80% on ARM and ARM64
* New Android x86/x64 and Linux ARM/AArch64 builds
* Matrix transpose up to 30% faster
* More stable Bluestein algorithm across all transform sizes
