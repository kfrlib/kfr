# Using KFR without CMake

The following methods provide a much simpler way to build and use KFR but involves CMake:

* `add_subdirectory`.
* Installing KFR and using `find_package(KFR CONFIG)`

Both methods are described in the [installation guide](installation.md).

It is recommended to use these methods for a hassle-free experience.

If you use different method please follow the instructions below.

## Header path

### Visual Studio

The `include` subdirectory of the KFR directory must be added to the `Include directories` section in Project's properties.

### Command line

```shell
-Ipath-to-kfr-repository/include
```

## Compiler flags

### Visual Studio

These compiler flags are required for KFR to work in Visual Studio.
* `/Zc:lambda /bigobj`

These flags must be added to C/C++ → Command line → Additional options.

### Command line


## Linking KFR in Multiarchitecture mode (`KFR_ENABLE_MULTIARCH=ON`, default)

Multiarchitecture mode enables building algorithms multiple times, each for a different architecture.
It adds runtime dispatch, allowing KFR to detect the CPU of the target machine and select the appropriate code path for algorithms.

It is important to link KFR libraries correctly when using the multiarchitecture feature.

In this example, we assume that `KFR_ARCHS` is set to `sse2;avx;avx2` with SSE2 being the base architecture.
DFT and DSP modules are multiarchitecture-enabled while IO modules is not.

KFR building produces the following static libraries:
```
# DFT module, <kfr/dft.hpp>
kfr_dft_sse2
kfr_dft_avx
kfr_dft_avx2
# DSP module, <kfr/dsp.hpp>
kfr_dsp_sse2
kfr_dsp_avx
kfr_dsp_avx2
# IO module, <kfr/io.hpp>
kfr_io
```

!!! note
    Exact name on Linux/macOS is `libkfr_dft_sse2.a` on Windows is `kfr_dft_sse2.lib` for `kfr_dft_sse2`

The linking command line must contain the foolowing commands _before_ any KFR libraries:

Linux:
```shell
--push-state --whole-archive kfr_dft_sse2 kfr_dsp_sse2 --pop-state
```
macOS:
```shell
-force_load path/to/libkfr_dft_sse2.a -force_load path/to/libkfr_dsp_sse2.a
```
Windows:
```batch
/WHOLEARCHIVE:path\to\kfr_dft_sse2.lib /WHOLEARCHIVE:path\to\kfr_dsp_sse2.lib
```

Without this, the linker may select incorrect functions which results in invalid instruction exception on older systems.

Your project's target architecture should match the lowest architecture selected during KFR build. In the example above, it is SSE2.

Add the `/arch` or `-m` flag to select the appropriate architecture for your code, or adjust `KFR_ARCHS` so that KFR matches your project settings.

## Linking KFR in single architecture mode (`KFR_ENABLE_MULTIARCH=OFF`)

KFR building produces the following static libraries:

```
# DFT module, <kfr/dft.hpp>
kfr_dft
# DSP module, <kfr/dsp.hpp>
kfr_dsp
# IO module, <kfr/io.hpp>
kfr_io
```

Add these libraries to the command line or the link libraries list of your build system.

Your project's target architecture should match the architecture selected during KFR build.

Add the `/arch` or `-m` flag to select the appropriate architecture for your code, or adjust `KFR_ARCH` so that KFR matches your project settings.
