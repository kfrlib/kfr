# Installation

## Support

KFR is tested and supported on the following systems and architectures:

**OS** • Windows • Linux • macOS • iOS • Android

**CPU** • x86 • x86_64 • ARM • ARM64 (AArch64)

**x86 extensions** • SSE2 • SSE3 • SSSE3 • SSE4.1 • SSE4.2 • AVX • AVX2 • AVX512 • FMA

**ARM extensions** • NEON

**Compiler** • GCC7+ • Clang 9+ • MSVC2019+ • Xcode 10.3+

## Prerequisites

* CMake 3.10 or newer for building tests and examples
* Python 3.6+ for running examples
* (recommended) Ninja (https://ninja-build.org/) for faster builds

For running examples and generating the frequency responses of the filters some python packages are required:

```bash
# in the kfr directory
pip install -r requirements.txt
```

### Clang

Clang is highly recommended and proven to provide the best performance for KFR. 

#### Installing Clang on Linux

Install clang using your package manager and add the following defines to the cmake command line:

```bash
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ...
```

#### Installing Clang on macOS

On macOS clang is the default compiler and already included in the official Xcode toolchain. No additional setup required.

#### Installing Clang on Windows

Download and install the latest `win64` build from the official LLVM GitHub page:

https://github.com/llvm/llvm-project/releases

## Getting KFR source code and binaries

### Git (recommended)

To obtain the full source code, including examples and tests, you can clone the git repository:

```
git clone https://github.com/kfrlib/kfr.git
```

The repository default branch `main` is stable and passes all tests. Latest features reside in `dev`.

#### Update

```bash
# in the kfr directory
git pull
```

### Tarball/zip

Download the latest release package from the GitHub releases:

https://github.com/kfrlib/kfr/releases

#### Update

Re-download tarball and unpack it to the same location.

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

### Including in CMake project (add_subdirectory)

`CMakeLists.txt` contains these libraries:
* `kfr` - header only interface library
* `kfr_dft` - static library for DFT and related algorithms
* `kfr_io` - static library for file IO and audio IO

```cmake
# Include KFR subdirectory
add_subdirectory(kfr)

# Add header-only KFR to your executable or library, this sets include directories etc
target_link_libraries(your_executable_or_library kfr)

# Add KFR DFT to your executable or library, (cpp file will be built for this)
target_link_libraries(your_executable_or_library kfr_dft)

# Add KFR IO to your executable or library, (cpp file will be built for this)
target_link_libraries(your_executable_or_library kfr_io)
```

### Makefile, command line etc (Unix-like systems)

```bash
# Add this to command line
-Ipath_to_kfr/include

# And this if needed
-lkfr_dft -lkfr_io

# C++17 mode must be enabled
-std=c++17
# or
-std=gnu++17

# linker options (requires kfr to be installed)
-lkfr_dft -lkfr_io
```

### Linux

#### Prerequisites

* GCC 7 or newer
* Clang 9.0 or newer (recommended)

#### Command line

```bash
cd <path_to_kfr>
mkdir build && cd build
cmake -DENABLE_TESTS=ON -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ..
make -- -j
```
Or using Ninja (better):
```bash
cd <path_to_kfr>
mkdir build && cd build
cmake -GNinja -DENABLE_TESTS=ON -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### macOS

#### Prerequisites

* Xcode 10.3 or later

#### Command line
Using Xcode project:
```bash
cd <path_to_kfr>
mkdir build && cd build
cmake -GXcode -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```
Using Unix Makefiles:
```bash
cd <path_to_kfr>
mkdir build && cd build
cmake -G"Unix Makefiles" -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
make -- -j
```
Or using Ninja (better):
```bash
cd <path_to_kfr>
mkdir build && cd build
cmake -GNinja -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### Visual Studio

#### Prerequisites
* Visual Studio 2019 or later
* Latest Clang (https://llvm.org/)
* Ninja is highly recommended because Visual Studio does not support parallel build with Clang at this moment.

#### Visual Studio IDE

To work with KFR in Visual Studio you must add the path to the `include` directory inside KFR directory to the list of the project's include directories.<br>
More details:
https://learn.microsoft.com/en-us/cpp/build/reference/vcpp-directories-property-page?view=msvc-160

Make sure that LLVM toolset is set for the project.

Download and install the official LLVM toolchain extension:
https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.llvm-toolchain

More details:
https://docs.microsoft.com/en-us/cpp/ide/general-property-page-project?view=vs-2017

LLVM/Clang has very good compatibility with MSVC ABI and is widely used for building large projects on Windows (including Chrome), so switching to LLVM/Clang should not cause any compatibility problems.

#### Command line
Using Ninja:
```
cd <path_to_kfr>
mkdir build && cd build
call "C:\<path to your Visual Studio installation>\VC\Auxiliary\Build\vcvars64.bat"
cmake -GNinja -DENABLE_TESTS=ON -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" -DCMAKE_BUILD_TYPE=Release ..
ninja
```
Or generate Visual Studio solution (building will be slower):
```
cd <path_to_kfr>
mkdir build && cd build
cmake -G"Visual Studio 16 2019 Win64" -DENABLE_TESTS=ON -Tllvm -DCMAKE_BUILD_TYPE=Release ..
```

### MinGW/MSYS

#### Prerequisites
* Latest MinGW or MSYS2
* Clang 9.0 or newer

Using Makefiles:
```
cd <path_to_kfr>
mkdir build && cd build
cmake -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
make -- -j
```
Using Ninja:
```
cd <path_to_kfr>
mkdir build && cd build
cmake -GNinja -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release ..
ninja
```

## Compile for multiple architectures

### Linux/macOS

There are two ways to use KFR. Use one architecture everywhere (but it must match in static library and your project) or use multiple architectures (selected at runtime).

#### 1. Single architecture (simpler setup)
Use -DKFR_ARCH=avx2 (or avx or sse41 or even sse2) when you run CMake to install KFR.
Example:
```
cmake -GNinja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DKFR_ARCH=avx2 ..
ninja
sudo ninja install # This installs libkfr_dft.a
```
The same architecture should be used when building source files:
Example:
```
g++ -mavx2 ... your_source.cpp -lkfr_dft
```

Then linking works well and KFR will use avx2 instructions for both DFT and other functions (built in your code).

Code from `examples/dft.cpp` will show:
```
KFR 5.1.0 avx2 64-bit (gcc-11.4.0/linux) +in
fft_specialization<double, 7>(avx2): 0, 128, 3072, 0, 1, 0, 0, 0, 1, 0, 0
```

#### 2. Multiple architectures (best performance)

Setting `KFR_ENABLE_MULTIARCH` to `ON` enables multiple architectures.
In this case instead of a single `libkfr_dft.a` multiple arch-specific libraries will be installed.
```
cmake -GNinja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DKFR_ENABLE_DFT_MULTIARCH=ON ..
ninja
sudo ninja install # This installs libkfr_dft_sse2.a libkfr_dft_sse41.a libkfr_dft_avx.a libkfr_dft_avx2.a libkfr_dft_avx512.a
```

Then you can compile your code using any architecture settings but should link all KFR DFT libraries:
Example (gcc will select sse2 for `your_source.cpp`):
```
g++ your_source.cpp -Wl,--push-state,--whole-archive -lkfr_dft_sse2 -Wl,--pop-state -lkfr_dft_sse41 -lkfr_dft_avx -lkfr_dft_avx2 -lkfr_dft_avx512
```
`whole-archive` flag is needed to link inline and template functions with correct architecture.

KFR code will detect cpu at runtime and select appropriate code path for DFT.

Code from `examples/dft.cpp` will show:
```
KFR 5.1.0 sse2 64-bit (gcc-11.4.0/linux) +in
fft_specialization<double, 7>(avx2): 0, 128, 3072, 0, 1, 0, 0, 0, 1, 0, 0
```

Notice that first mentioned architecture is sse2 (architecture used for `your_source.cpp`) while the second is now avx2 (dft source selected at runtime)

### Windows

For Windows build instructions are similar but below are exact commands.

#### 1. Single architecture (simpler setup)

```
:: Warning: VS Path and LLVM Path may be different on your machine
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cmake -GNinja -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" -DCMAKE_LINKER="C:/Program Files/LLVM/bin/lld-link.exe" -DCMAKE_BUILD_TYPE=Release -DKFR_ARCH=avx2 -DCMAKE_INSTALL_PREFIX=install ..
ninja
ninja install # This installs kfr_dft.lib to CMAKE_BINARY_DIR/install
```

Then, the following compile options must be added (through VS Project Properties or CMake target_compile_options)

```
/arch:AVX2 "PATH-TO-INSTALLED-KFR/lib/kfr_dft.lib"
```
As always `/arch` must match `KFR_ARCH` argument in CMake call.

#### 2. Multiple architectures (best performance)

```
:: Warning: VS Path and LLVM Path may be different on your machine
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
cmake -GNinja -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang-cl.exe" -DCMAKE_LINKER="C:/Program Files/LLVM/bin/lld-link.exe" -DCMAKE_BUILD_TYPE=Release -DKFR_ENABLE_DFT_MULTIARCH=ON -DCMAKE_INSTALL_PREFIX=install ..
ninja
ninja install # This installs kfr_dft_sse2.lib kfr_dft_sse41.lib kfr_dft_avx.lib kfr_dft_avx2.lib kfr_dft_avx512.lib to CMAKE_BINARY_DIR/install
```
`KFR_ENABLE_MULTIARCH=ON` is the key option here.
```
/WHOLEARCHIVE:"PATH-TO-INSTALLED-KFR/lib/kfr_dft_sse2.lib" "PATH-TO-INSTALLED-KFR/lib/kfr_dft_sse41.lib" "PATH-TO-INSTALLED-KFR/lib/kfr_dft_avx.lib" "PATH-TO-INSTALLED-KFR/lib/kfr_dft_avx2.lib" "PATH-TO-INSTALLED-KFR/lib/kfr_dft_avx512.lib"
```
`/WHOLEARCHIVE` for sse2 lib is required here for the same reason as linux code has `--whole-archive`: to force compiler to select correct inline/template functions from multiple similar libraries. Without this a runtime invalid instruction exception may occur.
