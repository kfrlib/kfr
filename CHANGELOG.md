# Changelog

## 3.0.3

2019-01-08

#### Added

#### Changed

- KFR_READCYCLECOUNTER may be redefined to point to any function returning (pseudo-)random value

#### Fixed

## 3.0.2

2018-12-19

#### Added

- More documentation
- dspplot: freqticks parameter
- dspplot: freq_lim parameter
- dspplot: freq_dB_lim parameter
- dspplot: phasearg parameter
- More tests for Sample Rate Converter

#### Changed

- Sample Rate Converter: now computes the transition width of the filter. This makes cutoff frequency more precise
- Sample Rate Converter: now uses Kaiser window with different &beta; parameter for different quality. This leads to a better balance between transition width and sidelobes attenuation.
- Sample Rate Converter: `quality` parameter is now passed as runtime parameter rather than compile time parameter
- `fir.cpp` example has been extended to include examples of applying FIR filters

#### Fixed

- amp_to_dB function
- 24-bit audio reading

## 3.0.1

2018-11-28

#### Added

- WAV file reading/writing
- FLAC file reading
- Audio sample conversion
- Interleaving/deinterleaving
- sample_rate_converter example
- Sample Rate Converter: process() function

#### Changed

- Assignment to an empty vector resizes it
- New documentation
- KFR IO is now built separately (only needed for audio file support)

#### Fixed

- Resampler bug: sequential calls to resampler::operator() may fail

### 3.0.0

#### Added

- Optimized non-power of two DFT implementation
- Full AVX-512 support
- EBU R128
- Ability to include KFR as a subdirectory in cmake project
- Number of automatic tests has been increased
- C API for DFT
- Partial GCC 8.x support
- Ability to make sized linspace
- `fraction` type

#### Changed

- GPL version changed from 3 to 2+
- Refactoring of DFT code
- KFR DFT is now built separately
- FIR filter now supports different tap and sample types (thanks to @fbbdev)

#### Fixed

- Various fixes for KFR functions
- FFT with size=128 on architectures with SSE only
- Small kfr::complex type fixes
