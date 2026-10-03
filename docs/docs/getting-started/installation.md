# Installation

This guide describes the four supported ways to acquire and integrate KFR:

1. download a prebuilt package from [GitHub Releases](https://github.com/kfrlib/kfr/releases),
2. build and install KFR from source, or
3. add the KFR source tree directly to an existing CMake project with `add_subdirectory`, or
4. download and configure KFR as part of the build with CMake `FetchContent`.

Downloaded packages and source-built installations are consumed with CMake
`find_package`. Use `add_subdirectory` or `FetchContent` when KFR should be
configured and built as part of your own project.

## Support

KFR is tested and supported on the following systems and architectures:

**OS** • Windows • Linux • macOS • iOS • Android • WebAssembly (Emscripten)

**CPU** • x86 • x86_64 • ARM • ARM64 (AArch64) • RISC-V (RV64)

**x86 extensions** • SSE2 • SSE3 • SSSE3 • SSE4.1 • SSE4.2 • AVX • AVX2 • FMA • AVX512

**ARM extensions** • NEON (a.k.a. AdvSIMD)

**RISC-V extensions** • RVV (Vector)

**Compiler** • GCC 11+ • Clang 16+ • MSVC 2022 (19.30+)+ • Xcode 13+

Since KFR 7.1, KFR supports Clang, GCC, and MSVC. MSVC builds may provide
lower performance, especially for DFT and other complex algorithms; use Clang
or GCC when maximum performance is required.

KFR itself and applications using it require C++20. Other operating systems,
compilers, and CPUs may work but are not part of the regular test matrix.

Emscripten builds support WebAssembly (`wasm` and `wasm64`), but do not use
multiarchitecture runtime dispatch.

## Prerequisites

* CMake 3.16 or newer
* A C++20-capable compiler
* Ninja is recommended when building KFR from source
* Python 3.6+ and the packages in `requirements.txt` only when running the
  plotting examples or generating filter-response graphs

Install the optional Python packages from the repository root:

```shell
pip install -r requirements.txt
```

Clang is recommended for the best KFR performance. See [Clang](clang.md) for
setup guidance.

## Choose an acquisition route

| Route | Choose it when | How the application consumes KFR |
| --- | --- | --- |
| [GitHub Releases package](#download-a-github-releases-package) | You need a tested binary for a supported platform and architecture. | `find_package(KFR CONFIG REQUIRED)` |
| [Build from source](#build-and-install-from-source) | You need a different compiler, architecture, feature set, build type, or local modifications. | `find_package(KFR CONFIG REQUIRED)` |
| [Include the source tree](#include-kfr-with-add_subdirectory) | KFR is part of the same CMake build and should use that build's configuration. | `add_subdirectory`, then link KFR targets |
| [Fetch KFR with FetchContent](#fetch-kfr-with-fetchcontent) | KFR is not checked out locally and should be downloaded and configured as part of the build. | `FetchContent_MakeAvailable`, then link KFR targets |

Prebuilt release packages are produced for the architectures exercised by the
release workflow, including Windows x86/x86_64/ARM64, Linux x86_64/ARM/ARM64/
RISC-V, macOS x86_64/ARM64/universal, and Android ABIs. Choose the package that
matches both the target operating system and architecture. A package built for
one platform, ABI, compiler runtime, or CPU architecture cannot be reused for
another.

## Download a GitHub Releases package

1. Open [GitHub Releases](https://github.com/kfrlib/kfr/releases) and download
   the archive for the required platform and architecture.
2. Extract it to a stable directory, referred to below as `<kfr-prefix>`.
3. Configure your application with `CMAKE_PREFIX_PATH` set to that directory.

For example, given this layout:

```text
<kfr-prefix>
├── include/kfr/
├── lib/
│   ├── cmake/kfr/KFRConfig.cmake
│   ├── kfr_dsp...
│   ├── kfr_dft...
│   ├── kfr_io...
│   └── kfr_audio...
└── lib/debug/                 # present when debug libraries are packaged
```

configure an application as follows:

```shell
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/kfr-prefix
cmake --build build
```

On Windows, pass a quoted native path when it contains spaces, for example
`-DCMAKE_PREFIX_PATH="C:/Libraries/kfr"`.

> [!note]
> `CMAKE_PREFIX_PATH` is the **installation prefix**, not the `lib/cmake/kfr`
> subdirectory. If setting it is inconvenient, set `KFR_DIR` directly to
> `<kfr-prefix>/lib/cmake/kfr`, the directory containing `KFRConfig.cmake`.

Release packages contain headers, CMake package metadata, and the libraries
that were enabled for that release build. They are intended for use with a
matching target platform; they are not a substitute for a cross-compilation
toolchain.

## Build and install from source

Building from source is the right choice for custom CPU targets, cross builds,
different compilers, optional modules, or changes to KFR itself. Clone the
repository, then configure separate Release and Debug build directories into a
single installation prefix:

```shell
git clone https://github.com/kfrlib/kfr.git
cd kfr

cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/kfr-prefix \
  -DCMAKE_CXX_COMPILER=path/to/clang++
cmake --build build-release
cmake --install build-release

cmake -S . -B build-debug -GNinja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/path/to/kfr-prefix \
  -DCMAKE_CXX_COMPILER=path/to/clang++
cmake --build build-debug
cmake --install build-debug
```

On multi-configuration generators such as Visual Studio or Xcode, configure
once and select the configuration while building and installing:

```shell
cmake -S . -B build -G "Visual Studio 17 2022" \
  -DCMAKE_INSTALL_PREFIX=C:/Libraries/kfr
cmake --build build --config Release
cmake --install build --config Release
cmake --build build --config Debug
cmake --install build --config Debug
```

Installing both configurations places Release libraries in `lib` and Debug
libraries in `lib/debug`. This lets CMake select the appropriate library when
your project is built in either configuration.

### Configure modules and dependencies

KFR's header-only core is always available. The following CMake options control
the optional compiled modules:

| Option | Default | Target when enabled | Purpose |
| --- | --- | --- | --- |
| `KFR_ENABLE_DSP` | `ON` | `kfr_dsp` | Filters, resampling, and other DSP algorithms. |
| `KFR_ENABLE_DFT` | compiler-dependent | `kfr_dft` | DFT, FFT, convolution, and related algorithms. Supported with Clang, GCC, and MSVC since KFR 7.1; it defaults to `ON` with Clang or GCC and `OFF` with MSVC. |
| `KFR_ENABLE_IO` | `ON` | `kfr_io` | File and data I/O facilities. |
| `KFR_ENABLE_AUDIO` | `ON` | `kfr_audio` | Audio file and audio-processing support. Requires both DSP and I/O. |
| `KFR_ENABLE_CAPI_BUILD` | `OFF` | `kfr_capi` | Build the C API shared library; this also requires DFT. |

Pass options while configuring, for example:

```shell
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/kfr-prefix \
  -DKFR_ENABLE_DFT=ON \
  -DKFR_ENABLE_AUDIO=OFF
```

`kfr_audio` enables FLAC and ALAC support when the corresponding libraries are
discoverable at configure time. The release workflows install FLAC through
vcpkg and pass its installed directory through `CMAKE_PREFIX_PATH`; use the
same pattern for source builds that need FLAC. The installed package records
enabled audio-codec support and links the bundled codec libraries from its
prefix where applicable.

The DSP module enables elliptic filter support through standalone Boost.Math by
default and obtains it with CMake `FetchContent`. For an offline or restricted
build, disable it with `-DKFR_USE_BOOST_MATH=OFF`, or provide Boost yourself and
enable `-DKFR_USE_BOOST=ON`.

KFR selects a CPU architecture through `KFR_ARCH`. `target` uses the target
machine's native/default settings; values such as `sse2`, `avx2`, and `avx512`
are useful for controlled x86 deployments. Cross-compilation additionally
requires the appropriate CMake toolchain file. The project CI contains working
examples for ARM, AArch64, RISC-V, Android, iOS, Windows ARM64, and macOS
universal builds.

## Consume an installed KFR package with `find_package`

Both downloaded packages and source-built prefixes export a CMake config
package. Its configuration file is:

```text
<kfr-prefix>/lib/cmake/kfr/KFRConfig.cmake
```

In your application's `CMakeLists.txt`, locate KFR in config mode and link only
the targets that provide the headers and functionality you use:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES CXX)

find_package(KFR CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE kfr kfr_dsp kfr_dft)
```

`find_package` imports the following targets when the corresponding module was
enabled in the installation:

* `kfr` — the header-only core target. It supplies KFR's public include path
  (`<kfr-prefix>/include`), requests C++20, and carries required platform
  compile/link settings.
* `kfr_dsp` — DSP algorithms. Link it for `<kfr/dsp.hpp>`.
* `kfr_dft` — DFT, FFT, and convolution algorithms. Link it for
  `<kfr/dft.hpp>`.
* `kfr_io` — file/data I/O. Link it for `<kfr/io.hpp>`.
* `kfr_audio` — audio support. Link it for `<kfr/audio.hpp>`; its dependency on
  `kfr_io` and `kfr_dsp` is transitive.
* `kfr_capi` — the optional shared C API library, when it was built and
  installed.

Do not add KFR's include directory manually and do not write library file paths
by hand. Linking the imported targets supplies the correct include directories,
compiler requirements, transitive libraries, and Debug/Release library choice.
For example, an application that only needs container and math facilities can
link `kfr` alone:

```cmake
target_link_libraries(my_app PRIVATE kfr)
```

> [!important]
> The optional modules are selected when **KFR is built**. They are not CMake
> `find_package` components, so `find_package(KFR COMPONENTS dft)` does not
> enable or download them. Ensure the required target exists in the package you
> installed, then link that target. Rebuild KFR with the relevant
> `KFR_ENABLE_*` option when it does not.

To verify integration, configure a small project with
`-DCMAKE_PREFIX_PATH=<kfr-prefix>`. The KFR repository also includes complete
consumer examples in `tests/usage-config` and `tests/usage-manual`.

## Include KFR with `add_subdirectory`

Use this route when the KFR checkout is present inside your source tree (for
example as a Git submodule) or next to it. KFR is configured in the same CMake
run as your project, so the targets are immediately available and no install
step or `find_package` call is needed.

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_app LANGUAGES CXX)

# Configure KFR before adding it when non-default modules are needed.
set(KFR_ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_ENABLE_AUDIO OFF CACHE BOOL "" FORCE)

add_subdirectory(external/kfr)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE kfr kfr_dsp kfr_dft)
```

The target names, module relationships, and header conventions are the same as
for an installed package. Link `kfr` for the core headers and the optional
targets required by the public headers used by your application. CMake
propagates KFR's source-tree include path and C++20 requirement automatically.

Set KFR options before `add_subdirectory`; cache entries created afterward may
be too late because KFR has already configured its targets. Usually KFR tests
are disabled in a parent build. If you enable them, examples are enabled by
default; set `-DENABLE_EXAMPLES=OFF` when they are not wanted.

## Fetch KFR with `FetchContent`

Use CMake's `FetchContent` module when KFR is not already present in your
source tree. CMake downloads KFR while configuring your project, then adds it
to the same build in the same way as `add_subdirectory`.

```cmake
cmake_minimum_required(VERSION 3.16)
project(your_app LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
  kfr
  GIT_REPOSITORY https://github.com/kfrlib/kfr.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(kfr)

add_executable(your_app main.cpp)
target_compile_features(your_app PRIVATE cxx_std_20)
target_link_libraries(your_app PRIVATE kfr)
```

The example tracks the latest development version on `main`. For reproducible
builds, use a released version tag instead, such as `GIT_TAG 7.0.1`.

`FetchContent_MakeAvailable` makes the same KFR targets available as
`add_subdirectory`. Link optional targets such as `kfr_dsp` or `kfr_dft` when
your application uses their corresponding public headers. Set any KFR options
before `FetchContent_MakeAvailable`, since it configures KFR immediately.

## Next steps

* [Introduction](../index.md) provides an overview and first examples.
* [Basics](basics.md) explains `univector`, `vec`, and the core
  API.
* [How to apply Fast Fourier Transform](../dft/dft.md) covers the DFT module.
* [FIR filters](../dsp/fir.md) and
  [Biquad filters](../dsp/bq.md) introduce common DSP workflows.
