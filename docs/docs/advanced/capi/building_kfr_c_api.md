# Building a KFR C API package

`kfr_capi` is the shared library that implements the C ABI described in the
[C API overview](../capi.md). It is built from the KFR source tree and contains
the complex and real DFT, DCT, FIR, FFT convolution, and IIR implementations;
C callers link `kfr_capi` alone and never the static C++ module libraries.

## Configure and install

The C API is optional and requires the DFT module:

```shell
cmake -S . -B build-capi -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/kfr-prefix \
  -DKFR_ENABLE_CAPI_BUILD=ON \
  -DKFR_ENABLE_DFT=ON \
  -DKFR_ENABLE_MULTIARCH=ON \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build-capi --target kfr_capi
cmake --install build-capi
```

`KFR_ENABLE_CAPI_BUILD=ON` is rejected when `KFR_ENABLE_DFT=OFF`. `kfr_capi`
also links `kfr_dsp` and `kfr_audio`, so `KFR_ENABLE_DSP`, `KFR_ENABLE_IO`, and
`KFR_ENABLE_AUDIO` must stay enabled. The implementation is C++ even though its
declarations are C-compatible, so a supported C++ compiler is required: use
`clang-cl` or MSVC on Windows, and add
`-DCMAKE_POSITION_INDEPENDENT_CODE=ON` where a shared library needs
position-independent objects.

`KFR_MANAGED_ALLOCATION` is the only other option that changes the exported C
API; see [Managed allocation mode](c_api_memory_management.md#managed-allocation-mode).
All of these options are described in
[Configuration](../configuration.md#cmake-options).

### CPU baseline

`KFR_ENABLE_MULTIARCH=ON` builds the `KFR_ARCHS` variants and selects the best
supported ISA at runtime, which is normally what a redistributable package
wants. A single-architecture build is smaller, but its `KFR_ARCH` must be
available on every machine that loads the library — an `avx2` build cannot run
on an SSE2-only CPU. See
[Select a CPU architecture](../configuration.md#select-a-cpu-architecture) for
the supported values on x86, ARM, and RISC-V, and use the cross-compilation
toolchain of the intended target rather than the build host's defaults.

## Install layout and CMake target

With default GNU install directories, installation produces:

```text
<kfr-prefix>/
├── include/kfr/capi.h       # C ABI declarations
├── include/kfr/config.h     # generated build configuration
├── lib/                     # libkfr_capi.so or libkfr_capi.dylib
│   └── cmake/kfr/           # KFRConfig.cmake and exported targets
└── bin/                     # kfr_capi.dll on Windows
```

Debug artifacts are installed under the `KFR_DEBUG_INSTALL_SUFFIX`
subdirectory (`lib/debug` by default), so one prefix can hold both
configurations.

An installed package exports `kfr_capi` alongside the usual KFR targets, which
supplies the include directory and the configuration-appropriate link artifact:

```cmake
cmake_minimum_required(VERSION 3.16)
project(capi_smoke C)

find_package(KFR CONFIG REQUIRED)
add_executable(capi_smoke main.c)
target_link_libraries(capi_smoke PRIVATE kfr_capi)
```

When integrating the source tree with `add_subdirectory`, set all KFR options
before adding it, then link the same target.

## ABI boundary

A C ABI is not a promise that arbitrary KFR builds are interchangeable. The
library must match the process operating system, CPU architecture, pointer
width, calling convention, and available ISA baseline, and any C++ runtime it
depends on must be present. Do not load a 64-bit library into a 32-bit process
or ship a library whose baseline ISA is missing on a target machine.

Only `kfr_`-prefixed entry points declared in `<kfr/capi.h>` are part of the
ABI. Plan and filter pointers are opaque and may be released only by their
matching `kfr_*_delete_*` function. Nothing else — C++ containers, templates,
exceptions, callbacks, object layouts — crosses the boundary.

## Verify a package before shipping

Run these checks on a clean, deployment-like environment. A successful KFR
build only proves that the library compiled; a C smoke test proves that the
installed header, exported target, link artifact, loader search path, and ABI
work together.

1. Confirm that `capi.h`, the generated `config.h`, and the shared library are
   present in the installed prefix. On Windows the DLL is installed in `bin`,
   elsewhere in `lib`.
2. Configure, build, and run a small **C** executable with
   `find_package(KFR CONFIG REQUIRED)` and `kfr_capi`, with the shared library
   discoverable by the loader.
3. At startup, check [[`::kfr_version`:nosig]] against `KFR_HEADERS_VERSION` and
   log [[`::kfr_version_string`:nosig]]. For a dispatch build, also log
   [[`::kfr_enabled_archs`:nosig]] and [[`::kfr_current_arch`:nosig]].
4. Create, execute, and delete a DFT or filter handle, and check one allocation
   for `NULL`.
5. Inspect runtime dependencies with `dumpbin /dependents`, `ldd`, or
   `otool -L`, then confirm the application can locate all of them.
