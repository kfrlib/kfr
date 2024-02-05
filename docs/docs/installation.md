# Installation

## Support

KFR is tested and supported on the following systems and architectures:

**OS** • Windows • Linux • macOS • iOS • Android

**CPU** • x86 • x86_64 • ARM • ARM64 (AArch64)

**x86 extensions** • SSE2 • SSE3 • SSSE3 • SSE4.1 • SSE4.2 • AVX • AVX2 • FMA • AVX512

**ARM extensions** • NEON

**Compiler** • GCC7+ • Clang 11+ • MSVC2019+ • Xcode 12+

## Prerequisites

* CMake 3.12 or newer for building tests and examples
* Python 3.6+ for running examples
* (recommended) Ninja (https://ninja-build.org/) for faster builds

For running examples and generating the frequency responses of the filters some python packages are required:

```bash
# in the kfr directory
pip install -r requirements.txt
```

### Clang

Clang is highly recommended and proven to provide the best performance for KFR.

See [how to install and setup Clang](clang.md)

## Getting KFR source code and binaries

### Git (recommended)

To obtain the full source code, including examples and tests, you can clone the git repository:

```
git clone https://github.com/kfrlib/kfr.git
```

The repository default branch `main` is stable and passes all tests. Latest features reside in `dev`.

### vcpkg

#### vcpkg on Linux/macOS
```bash
./vcpkg install kfr
```

#### vcpkg on Windows

```cmd
vcpkg install kfr
```

### ArchLinux Package
KFR is available on the [ArchLinux User Repository](https://wiki.archlinux.org/index.php/Arch_User_Repository) (AUR).
You can install it with an [AUR helper](https://wiki.archlinux.org/index.php/AUR_helpers), like [`yay`](https://aur.archlinux.org/packages/yay/), as follows:

```bash
yay -S kfr
```
To discuss any issues related to this AUR package refer to the comments section of
[`kfr`](https://aur.archlinux.org/packages/kfr/).

Prebuilt binaries will be available soon.

## Usage

### VCPKG

Refer to the vcpkg documentation for instructions on using libraries installed through vcpkg.

### Build and install KFR (recommended)

This way, KFR binaries are built in a separate step (Ninja is required).

```shell
cd path/to/kfr/repository
cmake -B build-release -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=path/to/kfr/install/dir -DCMAKE_CXX_COMPILER=path/to/clang
ninja -C build-release install
cmake -B build-debug -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=path/to/kfr/install/dir -DCMAKE_CXX_COMPILER=path/to/clang
ninja -C build-debug install
```

The following directory tree will be produced after successfull install:

```
include
  kfr
    all.hpp
    base.hpp
    dsp.hpp
    dft.hpp
    io.hpp
    ...
lib
  cmake
    kfr
      KFRConfig.cmake
      KFRConfig-release.cmake
      KFRConfig-debug.cmake
      KFRConfigVersion.cmake
  debug
    kfr-debug-libraries...
  kfr-libraries...
```

### CMake `find_package`

First, KFR binaries should be built as in [Build and install KFR](#build-and-install-kfr-recommended) section.

```cmake
# CMAKE_PREFIX_PATH must contain path-to-kfr-install-dir/lib/cmake
find_package(KFR CONFIG REQUIRED)
target_link_libraries(main PRIVATE kfr kfr_dsp)
```

### Including in your project directly (add_subdirectory)

This way, KFR binaries will be built in your project's build directory as part of building your project.

After inclusion of KFR, the following CMake targets will be available:
* `kfr` - header only interface library
* `kfr_dft` - static library for DFT and related algorithms
* `kfr_dsp` - static library for DSP algorithms
* `kfr_io` - static library for file IO and audio IO

```cmake
# Include KFR subdirectory
add_subdirectory(kfr kfr-build-dir)

# Add header-only KFR to your executable or library, this sets include directories etc
target_link_libraries(your_executable_or_library PUBLIC kfr)

# for <kfr/dsp.hpp>
target_link_libraries(your_executable_or_library PUBLIC kfr_dsp)
# for <kfr/dft.hpp>
target_link_libraries(your_executable_or_library PUBLIC kfr_dft)
# for <kfr/io.hpp>
target_link_libraries(your_executable_or_library PUBLIC kfr_io)
```

### Makefile, command line etc (Unix-like systems, not recommended)

First, KFR binaries should be built as in [Build and install KFR](#build-and-install-kfr-recommended) section.

Then see [using KFR without CMake](without_cmake.md)

### Visual Studio

First, KFR binaries should be built as in [Build and install KFR](#build-and-install-kfr-recommended) section.

Then see [using KFR without CMake](without_cmake.md)

### MinGW/MSYS

MinGW/MSYS environments are deprecated since KFR6 and no longer maintained. Use at your own risk.
