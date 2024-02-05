# Installing Clang

## Installing Clang on Linux

Install the latest clang available in your package manager:

`apt` (Ubuntu/Debian):
```bash
sudo apt install clang
```

or install any version from llvm apt:
```bash
wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s - 17
```

### CMake

The following defines are required to instruct CMake on which compiler to use:

```bash
cmake -DCMAKE_CXX_COMPILER=clang++ ...
```
or with specific version:
```bash
cmake -DCMAKE_CXX_COMPILER=clang++-17 ...
```

### make

Set `CXX=clang++` or `CXX=clang++-17` environment variables before running make.

## Installing Clang on macOS

On macOS clang is the default compiler and already included in the official Xcode toolchain. No additional setup required.

## Installing Clang on Windows

### CMake + Ninja

Download and install the latest `win64` build from the official LLVM GitHub page:

https://github.com/llvm/llvm-project/releases

### Visual Studio

In MSVC 2019 and 2022, there is built-in support for the Clang toolchain:

https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170
https://learn.microsoft.com/en-us/cpp/build/clang-support-cmake?view=msvc-170

LLVM/Clang has very good compatibility with the MSVC ABI and is widely used for building large projects on Windows (including Chrome). Therefore, switching to LLVM/Clang should not cause any compatibility problems.

## Transparent Clang use (experimental)

Setting `KFR_WITH_CLANG` to `ON` instructs KFR to download Clang from the official GitHub, extract it to the build directory, and set all required CMake variables to enable its usage.
Works on Windows and Linux.

```bash
cmake -B build -GNinja -DKFR_WITH_CLANG=ON -DCMAKE_BUILD_TYPE=Release
```
