# Using KFR Without CMake

KFR is built and packaged with CMake. The supported and recommended integration
routes are therefore:

* install KFR and use `find_package(KFR CONFIG REQUIRED)`, or
* include its source tree with `add_subdirectory`.

Both routes propagate include directories, C++20 requirements, platform flags,
transitive dependencies, Debug/Release variants, and the special linker rules
needed for multiarchitecture libraries. See [Installation](installation.md) for
the recommended workflows.

This page is for projects that cannot use CMake: IDE projects, Makefiles,
custom build generators, or other build systems. Manual integration is possible,
but your build must reproduce the compiler and linker requirements that CMake
would normally supply. Build and install KFR first; do not link directly to
libraries from KFR's build directory.

> [!warning]
> The manual instructions are an advanced fallback. CMake package targets are
> the authoritative integration interface. They are substantially less error
> prone than copying include paths, libraries, flags, and codec dependencies by
> hand.

## Prerequisites and installation layout

Build KFR from source or extract a compatible GitHub Releases package as
described in [Installation](installation.md). This guide calls the installation
root `<kfr-prefix>`.

A typical complete installation contains:

```text
<kfr-prefix>
├── include/kfr/                 # public headers
│   └── config.h                 # generated build configuration
├── lib/                         # Release static libraries
│   ├── kfr_dsp...
│   ├── kfr_dft...
│   ├── kfr_io...
│   └── kfr_audio...
├── lib/debug/                   # Debug static libraries, if installed
└── bin/                         # shared libraries such as kfr_capi, if built
```

The exact files depend on the KFR options used while building the installation.
For example, `kfr_dft` exists only when `KFR_ENABLE_DFT=ON`; `kfr_audio` exists
only when `KFR_ENABLE_AUDIO=ON`.

Use an installation whose operating system, CPU architecture, compiler ABI,
C++ runtime, build mode, and CPU baseline match your application. Do not mix
32-bit and 64-bit libraries, Debug and Release libraries, or MSVC-compatible
and GNU-compatible Windows builds.

## Headers and language requirements

Add the installation's `include` directory—not `include/kfr`—to your compiler's
header-search path:

```text
<kfr-prefix>/include
```

This permits the public include form used throughout KFR:

```c++
||||||||||
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/dft.hpp>
||||||||||
```

Compile application code as C++20 or newer. For GCC and Clang, this normally
means:

```shell
-std=c++20 -I/path/to/kfr-prefix/include
```

For MSVC and `clang-cl`, use `/std:c++20` and add
`<kfr-prefix>\include` to the project's additional include directories.

### Generated configuration header

The installed `include/kfr/config.h` records configuration definitions from the
KFR build, such as enabled modules and optional codec support. Always include
headers from the same installation prefix as the libraries. Do not replace the
installed `config.h` with the source-tree version, and do not define KFR feature
macros differently in the application.

## Select the libraries for the headers you use

The `kfr` core is header-only, so it has no corresponding static library. Link
the compiled KFR libraries required by your public headers:

| Header or feature                                    | Required library                 | Additional requirements                                                            |
|------------------------------------------------------|----------------------------------|------------------------------------------------------------------------------------|
| `<kfr/base.hpp>`, `<kfr/math.hpp>`, `<kfr/simd.hpp>` | none                             | Compile as C++20.                                                                  |
| `<kfr/dsp.hpp>`                                      | `kfr_dsp`                        | May require Boost.Math when elliptic-filter support was enabled.                   |
| `<kfr/dft.hpp>`                                      | `kfr_dft`                        | Link its multiarchitecture variants correctly when enabled.                        |
| `<kfr/io.hpp>`                                       | `kfr_io`                         | —                                                                                  |
| `<kfr/audio.hpp>`                                    | `kfr_audio`, `kfr_io`, `kfr_dsp` | May require the codec and platform libraries described below.                      |
| C API                                                | `kfr_capi`                       | Build with `KFR_ENABLE_CAPI_BUILD=ON`; its runtime dependencies must be available. |

Add a library only when your program uses the corresponding module. For example,
a basic program using only `univector`, expressions, and math functions needs
only the headers; a program using FFT and FIR filters needs `kfr_dft` and
`kfr_dsp`.

> [!important]
> Link order matters for static libraries on Unix-like linkers. Put object files
> first, then KFR libraries, then libraries required by KFR. When in doubt,
> preserve the order shown in the platform examples below.

## Link a single-architecture KFR build

A single-architecture build uses `KFR_ENABLE_MULTIARCH=OFF`. Its compiled
libraries have the simple names `kfr_dsp`, `kfr_dft`, `kfr_io`, and `kfr_audio`.
Their filenames include the conventional platform prefix and suffix:

| Platform | Example `kfr_dft` file |
|----------|------------------------|
| Linux    | `libkfr_dft.a`         |
| macOS    | `libkfr_dft.a`         |
| Windows  | `kfr_dft.lib`          |

Link the libraries required by your application, plus their dependencies. For a
program using DFT and DSP, a GNU/Clang command line on Linux commonly resembles:

```shell
clang++ -std=c++20 -O3 main.cpp \
  -I/path/to/kfr-prefix/include \
  -L/path/to/kfr-prefix/lib \
  -lkfr_dft -lkfr_dsp -lm -lpthread
```

The correct CPU flags for your application must match KFR's `KFR_ARCH` setting.
For example, if KFR was built with `-DKFR_ARCH=avx2`, compile all code that uses
that installation for AVX2 as well. In controlled x86 deployments, GCC and
Clang use options such as `-msse2`, `-mavx2`, or `-mavx512f`; MSVC uses the
corresponding `/arch:` option. Prefer building KFR with a conservative baseline
when the application must run on older processors.

> [!note]
> A single-architecture build cannot safely run on CPUs lacking the instruction
> set selected at KFR build time. Use multiarchitecture mode for x86 binaries
> distributed across a range of CPU generations.

## Link a multiarchitecture KFR build

On x86, `KFR_ENABLE_MULTIARCH` is enabled by default. KFR compiles selected
modules for several instruction sets and dispatches at runtime to the best
implementation supported by the current CPU. The DSP and DFT modules are
multiarchitecture-enabled; IO is not.

For example, a build configured with:

```text
KFR_ARCHS=sse2;sse41;avx;avx2
```

produces libraries conceptually named:

```text
kfr_dft,       kfr_dft_sse41,  kfr_dft_avx,  kfr_dft_avx2
kfr_dsp,       kfr_dsp_sse41,  kfr_dsp_avx,  kfr_dsp_avx2
kfr_io
```

The first architecture in `KFR_ARCHS` is the **base architecture**. It is
special: its archive is installed without an architecture suffix and must be
force-loaded so that the runtime dispatch entries are retained. The remaining
architecture archives are linked normally. The project-level CPU flags must be
compatible with the base architecture—in this example, SSE2—not with the
highest optimized variant.

> [!important]
> Force-load the base DFT/DSP archives **before** the normal KFR archives. If
> they are not force-loaded, a static linker can discard needed dispatch code;
> the resulting program may select an invalid instruction sequence on older
> CPUs.

### Linux and other ELF linkers

Use `--whole-archive` for the base variants, then restore ordinary archive
handling. Pass the remaining variants and other KFR libraries normally:

```shell
clang++ -std=c++20 -O3 main.cpp \
  -I/path/to/kfr-prefix/include \
  -L/path/to/kfr-prefix/lib \
  -Wl,--push-state,--whole-archive -lkfr_dft -lkfr_dsp \
  -Wl,--pop-state \
  -lkfr_dft_sse41 -lkfr_dft_avx -lkfr_dft_avx2 \
  -lkfr_dsp_sse41 -lkfr_dsp_avx -lkfr_dsp_avx2 \
  -lkfr_io -lm -lpthread
```

The GNU linker spellings `-Wl,--whole-archive ... -Wl,--no-whole-archive` are
also acceptable. Keep whole-archive scope as narrow as possible: it should
cover only the base KFR module archives.

### macOS

Use `-force_load` with the full path to each base archive:

```shell
clang++ -std=c++20 -O3 main.cpp \
  -I/path/to/kfr-prefix/include \
  -Wl,-force_load,/path/to/kfr-prefix/lib/libkfr_dft.a \
  -Wl,-force_load,/path/to/kfr-prefix/lib/libkfr_dsp.a \
  -L/path/to/kfr-prefix/lib \
  -lkfr_dft_avx -lkfr_dft_avx2 -lkfr_dft_avx512 \
  -lkfr_dsp_avx -lkfr_dsp_avx2 -lkfr_dsp_avx512 \
  -lkfr_io -lm -lpthread
```

Use the actual first architecture and actual archive names from your
installation. macOS multiarchitecture defaults may start at `sse41`, unlike
other x86 builds that commonly start at `sse2`.

### Windows

Use `/WHOLEARCHIVE:` with the complete path to each base archive. With
`clang-cl`, invoke it from a Visual Studio developer prompt:

```bat
clang-cl /std:c++20 /O2 main.cpp ^
  /I C:\path\to\kfr-prefix\include ^
  /link /LIBPATH:C:\path\to\kfr-prefix\lib ^
  /WHOLEARCHIVE:C:\path\to\kfr-prefix\lib\kfr_dft.lib ^
  /WHOLEARCHIVE:C:\path\to\kfr-prefix\lib\kfr_dsp.lib ^
  kfr_dft_sse41.lib kfr_dft_avx.lib kfr_dft_avx2.lib ^
  kfr_dsp_sse41.lib kfr_dsp_avx.lib kfr_dsp_avx2.lib ^
  kfr_io.lib
```

Set the compiler architecture to the base architecture where necessary; for
example, use `/arch:SSE2` for a 32-bit SSE2 baseline. For Windows Clang setup,
see [Building KFR with Clang](clang.md).

## Audio, codecs, and system dependencies

`kfr_audio` depends on `kfr_io` and `kfr_dsp`; link all three when using
`<kfr/audio.hpp>`. The audio build may also enable FLAC and ALAC based on the
dependencies found when KFR was configured. Manual consumers must link any
libraries required by their particular KFR installation.

* When FLAC support is enabled, link `FLAC` and `ogg` from the matching KFR
  installation's `lib` directory. Use the Debug copies from `lib/debug` for a
  Debug application when they are present.
* When ALAC support is enabled, link the installed `alac`/`libalac` library.
* On Windows, `kfr_audio` also needs the Media Foundation libraries: `mf`,
  `mfplat`, `mfreadwrite`, `mfuuid`, and `propsys`.
* On Linux and macOS, most KFR builds require the system math and thread
  libraries: `-lm -lpthread`.

The installed KFR CMake package determines these dependencies automatically;
manually linked projects must inspect the installation's contents and add them
explicitly. If a codec symbol is unresolved, verify that the matching codec
library was built and is linked after `kfr_audio`.

## Debug and Release configurations

A dual configuration installation stores Release libraries in `lib` and Debug
libraries in `lib/debug`. Link one matching set at a time:

```text
Release application → <kfr-prefix>/lib
Debug application   → <kfr-prefix>/lib/debug
```

On Windows, choose libraries built with the same MSVC runtime model as your
application. In particular, do not mix static and dynamic runtime variants
without intentionally rebuilding the full dependency set.

## Visual Studio project settings

For a manually maintained Visual Studio project:

1. Set **C++ Language Standard** to C++20 or later.
2. Add `<kfr-prefix>\include` under **C/C++ → General → Additional Include
   Directories**.
3. Add either `<kfr-prefix>\lib` or `<kfr-prefix>\lib\debug` under
   **Linker → General → Additional Library Directories**.
4. Add the required `.lib` files under **Linker → Input → Additional
   Dependencies**.
5. Add `/WHOLEARCHIVE:<full path to base archive>` under **Linker → Command
   Line** when multiarchitecture mode is enabled.
6. When using Clang, use `clang-cl` within a Visual Studio developer environment
   rather than GNU-compatible `clang.exe`.

KFR's CMake target applies `/bigobj` for MSVC builds and selects suitable
architecture settings. If your manual project encounters object-section limits
or architecture-flag mismatches, add `/bigobj` and make the project CPU baseline
match the KFR installation.

## Diagnose manual-link problems

### Undefined `kfr::` symbols

Link the library corresponding to the header that declares the missing symbol.
For example, DFT symbols need `kfr_dft`, and DSP filters need `kfr_dsp`. On
Unix-like systems, ensure KFR libraries appear after the objects that reference
them.

### Missing FLAC, Ogg, ALAC, or Media Foundation symbols

The selected `kfr_audio` library was built with support for that dependency.
Link its matching codec or Windows system libraries, using the same architecture
and configuration as KFR.

### Illegal instruction on an older x86 CPU

Check that the application is compiled for the multiarchitecture base ISA and
that the base DFT/DSP archives are force-loaded. Rebuild KFR with a lower
`KFR_ARCHS` baseline if the deployed CPU does not support the first entry.

### Debug libraries or symbols do not match the application

Use `lib/debug` for a Debug application and `lib` for a Release application.
Rebuild KFR with the same compiler, ABI, runtime, architecture, and build mode
as the application when a compatible set is not available.

## Next steps

* [Installation](installation.md) documents the recommended CMake integration
  routes, package layout, and build options.
* [Building KFR with Clang](clang.md) covers Clang setup on Linux, macOS,
  Windows, and cross-compilation targets.
* [C API](../advanced/capi.md) describes the optional C interface.
