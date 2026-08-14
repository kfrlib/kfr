# Building KFR with Clang

Since KFR 7.1, KFR supports Clang, GCC, and MSVC. Clang is recommended when
maximum performance is required, particularly for DFT and other complex
algorithms; MSVC builds may provide lower performance for these workloads.
KFR requires a compiler with C++20 support; Clang 12 or newer is supported.

This page explains how to install Clang and make CMake select it reliably. For
complete KFR build, installation, and integration instructions, see
[Installation](installation.md).

## Select Clang when configuring CMake

CMake chooses a compiler when a build directory is first configured. Set the
compiler on that first configure command; changing `CMAKE_C_COMPILER` or
`CMAKE_CXX_COMPILER` later in the same build directory is not supported.
Delete the build directory, or configure a new one, when switching compilers.

For C++-only KFR builds, set `CMAKE_CXX_COMPILER`:

```shell
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build-release
```

Use an explicit executable name or absolute path when several Clang versions
are installed:

```shell
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++-21
```

When your project also compiles C sources, set both compilers:

```shell
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
```

Verify the selection in CMake's configure output. It should report a compiler
identification similar to `Clang` or `AppleClang`; `CMakeCache.txt` records the
selected compiler for subsequent builds.

> [!important]
> Do not set the compiler through `CXX` after CMake has configured a build
> directory. CMake will keep using the cached compiler. Start with a fresh build
> directory and pass `-DCMAKE_CXX_COMPILER=...` instead.

## Linux

### Install from the distribution package manager

For a quick setup on Debian or Ubuntu, install the packaged compiler and Ninja:

```shell
sudo apt-get update
sudo apt-get install clang ninja-build
```

Then configure KFR with `clang++` as shown above. The available package version
varies by distribution; use a versioned executable such as `clang++-19` when a
specific installed version is required.

### Install a current LLVM release on Ubuntu or Debian

KFR CI installs LLVM 21 using LLVM's apt repository script. The equivalent
pattern for a local system is:

```shell
wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s -- 21
sudo apt-get install clang-21 lld-21 llvm-21
```

Use that version explicitly:

```shell
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang-21 \
  -DCMAKE_CXX_COMPILER=clang++-21
```

For KFR cross-compilation toolchain files, Clang's companion LLVM tools must be
available on `PATH`: `llvm-ar`, `ld.lld`, `llvm-nm`, `llvm-objcopy`,
`llvm-objdump`, and `llvm-ranlib`. The CI setup registers version 21 of these
tools before configuring ARM, AArch64, and RISC-V builds.

### Linux dependencies and vcpkg

If your build uses optional audio support, install dependencies for the same
target and make their CMake packages visible. For example, a vcpkg installation
can be passed through `CMAKE_PREFIX_PATH`:

```shell
vcpkg install --triplet=x64-linux libflac minimp3
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_PREFIX_PATH="$VCPKG_INSTALLATION_ROOT/installed/x64-linux"
```

When vcpkg itself must build dependencies with Clang on Linux, it may need a
custom triplet and chainloaded toolchain. See
[Building with Clang on Linux via vcpkg](../advanced/kb.md#building-with-clang-on-linux-via-vcpkg).

## macOS

Apple's Xcode toolchain includes Apple Clang. Install Xcode and accept its
license, or install the Command Line Tools:

```shell
xcode-select --install
```

CMake normally selects Apple Clang automatically. Confirm the active toolchain:

```shell
clang++ --version
xcode-select -p
```

Configure with Ninja or the Xcode generator as appropriate:

```shell
cmake -S . -B build-release -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++
```

For a universal binary, compile both architectures in one CMake build. KFR uses
an x86-compatible baseline for the universal configuration:

```shell
cmake -S . -B build-universal -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13 \
  -DKFR_ARCH=sse41
```

See the [knowledge base](../advanced/kb.md#how-to-build-kfr-on-macos-with-universal-binaries)
for additional universal-build notes.

## Windows

On Windows, build KFR with `clang-cl.exe`, not the GNU-compatible `clang.exe`.
`clang-cl` accepts MSVC-style options and uses the Microsoft C++ ABI, which
matches Visual Studio, the Windows SDK, and common Windows dependencies.

> [!warning]
> Using `clang.exe` on Windows can produce unsupported GNU-style options or
> linker errors such as `could not open stdc++.lib`. Use `clang-cl.exe` and a
> Visual Studio developer environment instead.

### Prerequisites

Install:

1. Visual Studio 2022 or Build Tools with the **Desktop development with C++**
   workload and a Windows SDK.
2. LLVM for Windows, from the
   [LLVM releases page](https://github.com/llvm/llvm-project/releases) or a
   package manager such as Chocolatey.
3. Ninja, if using the Ninja generator.

Open **x64 Native Tools Command Prompt for VS 2022** (or the corresponding x86
or ARM64 prompt). This initializes paths for the MSVC headers, libraries, and
linker tools. Then configure with `clang-cl`:

```bat
cmake -S . -B build-release -GNinja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_LINKER="C:/Program Files/LLVM/bin/lld-link.exe" ^
  -DCMAKE_AR="C:/Program Files/LLVM/bin/llvm-lib.exe"
cmake --build build-release
```

The explicit linker and archive-tool settings mirror KFR's Windows CI builds.
They are especially useful when more than one toolchain is installed.

For a 32-bit build, start the x86 developer prompt before configuring. For an
ARM64 build on an x64 host, start the ARM64 developer prompt and add the target
triple:

```bat
cmake -S . -B build-arm64 -GNinja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER_TARGET=arm64-pc-windows-msvc ^
  -DCMAKE_LINKER="C:/Program Files/LLVM/bin/lld-link.exe" ^
  -DCMAKE_AR="C:/Program Files/LLVM/bin/llvm-lib.exe" ^
  -DKFR_ARCH=target
```

Use dependencies built for the same architecture and MSVC runtime. For example,
KFR's Windows CI pairs `clang-cl` with the vcpkg `x64-windows-static-md`,
`x86-windows-static-md`, or `arm64-windows-static-md` triplet.

### Visual Studio generator

CMake can also generate a Visual Studio solution while using the ClangCL
toolset. In a Visual Studio developer prompt:

```bat
cmake -S . -B build-vs -G "Visual Studio 17 2022" -T ClangCL -A x64
cmake --build build-vs --config Release
```

Choose either the Ninja/explicit-compiler approach or the Visual Studio
`ClangCL` toolset for a build directory; do not reuse one build directory for
both.

## Cross-compiling with Clang

KFR ships CMake toolchain files for Linux ARM (`cmake/arm.cmake`), AArch64
(`cmake/aarch64.cmake`), and RISC-V 64 (`cmake/riscv64.cmake`). They select
Clang, LLD, LLVM binutils, an appropriate `--target` triple, and a sysroot
location. Install the corresponding GNU cross-development packages to provide
the headers, libraries, and sysroot expected by the toolchain file.

> [!note]
> These toolchain files are internal KFR build configurations, primarily
> maintained for the project's CI builds. They make specific assumptions about
> the host system, target triple, CPU baseline, sysroot paths, and available
> tools, so they are not intended to be flexible general-purpose cross-toolchains.
> Copy and adapt one, or provide your own toolchain file, when your deployment
> has different requirements.

For example, on Ubuntu, an AArch64 build requires the cross compiler/sysroot
packages and can be configured with:

```shell
sudo apt-get install g++-aarch64-linux-gnu ninja-build
cmake -S . -B build-aarch64 -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/aarch64.cmake \
  -DKFR_ARCH=target
cmake --build build-aarch64
```

The resulting executable targets AArch64; it cannot run directly on an x86_64
host. Use the target device or a suitable emulator for tests. The same principle
applies to ARM and RISC-V. See [Installation](installation.md) for supported
release targets and general cross-build guidance.

## KFR-managed Clang download

KFR provides the experimental `KFR_WITH_CLANG` option for a self-contained
Windows or Linux build. When enabled **during the first CMake configure**, KFR
downloads a pinned LLVM distribution, extracts it under the build directory,
and sets CMake's C and C++ compiler cache entries to that copy.

```shell
cmake -S . -B build-release -GNinja \
  -DKFR_WITH_CLANG=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
```

This convenience option requires network access and downloads a fixed LLVM
version defined by KFR's top-level `CMakeLists.txt`. Prefer a system-installed
Clang for reproducible package-managed builds, offline builds, custom LLVM
versions, or cross-compilation. Do not combine `KFR_WITH_CLANG=ON` with explicit
`CMAKE_C_COMPILER` or `CMAKE_CXX_COMPILER` settings.

## Troubleshooting

### CMake still reports GCC, MSVC, or another compiler

The build directory was configured before the Clang compiler options were
provided. Remove it and configure again with a `-DCMAKE_CXX_COMPILER` value.

### Windows reports unsupported options or `stdc++.lib` errors

`clang.exe` was selected instead of `clang-cl.exe`, or the Visual Studio
developer environment was not initialized. Open the correct Native Tools prompt,
configure a clean build directory, and select `clang-cl` for both C and C++.

### The DFT module is disabled

DFT is supported with Clang, GCC, and MSVC since KFR 7.1. `KFR_ENABLE_DFT`
defaults to `ON` for Clang and GCC builds and `OFF` for MSVC builds. Configure
explicitly with `-DKFR_ENABLE_DFT=ON` when you want the DFT, FFT, and
convolution targets.

### A cross build cannot find headers or libraries

Confirm that the matching cross-development/sysroot packages are installed,
that the toolchain file matches the target, and that target dependencies use
the same architecture. Do not point a cross build at host libraries.

## Next steps

* [Installation](installation.md) explains KFR packages, installation prefixes,
  optional modules, and CMake integration.
* [Knowledge base](../advanced/kb.md) contains additional compiler and build
  troubleshooting notes.
* [C API](../advanced/capi.md) describes building the optional C API library.
