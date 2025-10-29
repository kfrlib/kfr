# KFR 7 Upgrade Guide

## Audio reading/writing

KFR 7 introduces a new audio I/O module that replaces the previous `audio_reader` and `audio_writer` interface. The new module provides broader support for audio formats and improved workflows.

[See the Audio I/O documentation](read_audio.md) for details on how to read and write audio files using the new module.

## CMake 3.16 required

Now KFR requires CMake version 3.16 or higher to build. Please ensure your build environment meets this requirement.

## Cometa and Testo integration

Previously, KFR included Cometa and Testo sub-libraries for metaprogramming and testing functionalities. It was developed by the same team and tightly integrated with KFR.
In KFR 7, these libraries have been merged into the main KFR codebase.

See required changes below:

### `cometa` and `testo` namespaces have been merged into `kfr`

All references to the `cometa` or `testo` namespaces should be replaced with `kfr`.

```diff
- cometa::some_function();
+ kfr::some_function();

- testo::some_test_function();
+ kfr::some_test_function();
```

```diff
-using namespace cometa;
-using namespace testo;
```

### Macros

All Cometa macros renamed from `CMT_` to `KFR_`. All Testo macros renamed from `TESTO_` to `KFR_`.

```diff
- CMT_ARCH_NAME
+ KFR_ARCH_NAME

- TESTO_ASSERT
+ KFR_ASSERT
```

### File locations

`cometa.hpp` renamed to `meta.hpp`; `kfr/cometa/*.hpp` renamed to `kfr/meta/*.hpp`. Update your includes accordingly.

```diff
- #include <kfr/cometa.hpp>
+ #include <kfr/meta.hpp>

- #include <kfr/cometa/some_header.hpp>
+ #include <kfr/meta/some_header.hpp>
```

`kfr/testo/*.hpp` renamed to `kfr/test/*.hpp`.

```diff
-#include <kfr/testo/testo.hpp>
+#include <kfr/test/test.hpp>
```

## IIR filter design

Some classes and functions no longer accept a template parameter and now default to `double` for maximum precision. Related changes:
  * `zpk<float_type>` replaced with `zpk`
  * `bessel<float_type>` and `butterworth<float_type>` replaced with `bessel` and `butterworth`
  * `bilinear<float_type>`, `lp2lp_zpk<float_type>`, `lp2bs_zpk<float_type>`, `lp2bp_zpk<float_type>`, `lp2hp_zpk<float_type>`, and `warp_freq<float_type>` replaced with non-template equivalents
  * `iir_lowpass<float_type>`, `iir_highpass<float_type>`, `iir_bandpass<float_type>`, `iir_bandstop<float_type>` replaced with non-template equivalents
  * `iir_params<float_type>` still accepts a template parameter to control precision
  * `to_sos` now accepts `float_type` (default `double`) to return `iir_params<float_type>`
  * New `elliptic` function follows the same rule and always produces double-precision `zpk`

```diff
 // Create an 8th-order Chebyshev Type II lowpass filter with a cutoff frequency of 0.09 (normalized
 // frequency) and 80 dB attenuation in the stopband
-zpk<fbase> filt = iir_lowpass(chebyshev2<fbase>(8, 80), 0.09);
+zpk filt = iir_lowpass(chebyshev2(8, 80), 0.09);
 // Convert the filter to second-order sections (SOS).
 // This is an expensive operation, so keep 'iir_params' if it is reused later
-iir_params<fbase> bqs = to_sos(filt);
+iir_params<fbase> bqs = to_sos<fbase>(filt);
 // Apply the filter to a unit impulse signal to get the filter's impulse response
 output = iir(unitimpulse(), filt);
```

## C++20 related changes

Because of migration to C++20, the way to define custom expressions has changed.

```diff
-template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
+template <expression_argument E1>
KFR_INTRINSIC expression_some<E1> some_expr(E1&& arg)
```

```diff
-template <typename E1, typename E2, KFR_ACCEPT_EXPRESSIONS(E1, E2)>
+template <typename E1, typename E2>
+    requires expression_arguments<E1, E2>
KFR_INTRINSIC expression_some2<E1, E2> some_expr2(E1&& arg, E2&& arg2)
```

### Other changes

* `numeric` concept replaces `is_numeric<T1>` predicate
* `output_expression` concept replaces `enable_if_output_expression` trait.
* `CMT_ENABLE_IF` mostly replaced with concepts and `requires`; `KFR_ENABLE_IF` still available

## Memory handling

KFR 7 defaults to use standard C++ aligned memory allocation functions with POSIX fallback for performance and compatibility.
If your project relies on previous KFR-managed memory allocation behavior with reference counting and statistics, 
you can turn `KFR_MANAGED_ALLOCATION` on in your CMake configuration. Clean rebuild is required.

Prebuilt binaries are built with standard allocation (`KFR_MANAGED_ALLOCATION` is off).

Note that `aligned_size`, `aligned_force_free`, `aligned_release`, and `aligned_reallocate` are only defined if `KFR_MANAGED_ALLOCATION` is on.

## Renames

`Speaker` and `SpeakerArrangement` enums have been renamed to `speaker_type` and `speaker_arrangement`, respectively.

```diff
- Speaker spk = Speaker::Left;
+ speaker_type spk = speaker_type::Left;

- SpeakerArrangement arr = SpeakerArrangement::Stereo;
+ speaker_arrangement arr = speaker_arrangement::Stereo;
```

Fixed typo: `convert_endianess` → `convert_endianness`

## Manual linking

In multiarch builds, the base architecture is now named exactly as the module itself (was: `kfr_dsp_sse2`, `kfr_dsp_avx2`; now: `kfr_dsp`, `kfr_dsp_avx2`).
If you're manually linking against KFR libraries, please use the new names.

