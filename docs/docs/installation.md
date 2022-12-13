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

#### Linux

Install clang using your package manager and add the following defines to the cmake command line:

```bash
cmake -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang ...
```

#### macOS

On macOS clang is the default compiler and already included in the official Xcode toolchain. No additional setup required.

#### Windows

Download and install the latest `win64` build from the official LLVM GitHub page:

https://github.com/llvm/llvm-project/releases

## Getting the source code and binaries

### Git (recommended)

To obtain the full source code, including examples and tests, you can clone the git repository:

```
git clone https://github.com/kfrlib/kfr.git
```

The repository default branch `master` is stable and passes all tests. Latest features reside in `dev`.

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

#### Linux/macOS
```bash
./vcpkg install kfr
```

#### Windows

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

### Including in CMake project

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
