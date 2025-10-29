# KFR Knowledge Base

## Upgrading to KFR 7

[See the KFR 7 Upgrade Guide](upgrade7.md) for detailed instructions on updating your codebase to be compatible with KFR 7.

## Building with Clang on Linux via vcpkg

Place these two files into your vcpkg directory or fork:

**triplets/x64-linux-clang.cmake**
```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../scripts/toolchains/linux-clang.cmake)
```

**scripts/toolchains/linux-clang.cmake**
```cmake
include(${CMAKE_CURRENT_LIST_DIR}/linux.cmake)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
```

You can also place these in a separate folder and point vcpkg to that directory with `--overlay-triplets`, but this would require path fixes.

## Applying two FIR filters in parallel

To apply two FIR filters in parallel, you can sum their coefficients and create a new filter from the result.

Applying a filter that is the sum of coefficients of two filters is the same as applying two filters in parallel (equaliser-like). For filters in series you would need a convolution of two coefficient vectors.

## How to Build KFR on macOS with Universal Binaries

### Using CMake

Set the following CMake variables when configuring the build:
```bash
-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DKFR_ARCH=sse41
```

Note that prebuilt universal binaries are available for download on GitHub.

## Note on AVX10

AVX10 is a marketing term used by Intel to refer to a set of new instructions introduced with the Intel Xeon Scalable "Sapphire Rapids" processors.
Technically, AVX10 is based on the AVX-512 instruction set architecture (ISA) but with some modifications and optimizations.

KFR supports AVX10 through its existing AVX-512 implementation.

## Potential Issues

### Performance is slow

Ensure that you're building KFR with optimizations enabled.
CMake should be configured with `-DCMAKE_BUILD_TYPE=Release` or `-DCMAKE_BUILD_TYPE=RelWithDebInfo` for best performance.

If you're using your own build system, make sure to define the `NDEBUG` macro and enable optimizations in your compiler flags (e.g., `-O3` for GCC/Clang or `/O2` for MSVC).

The following line should print the library version with flags:
```c++
println(library_version());
```

In correctly optimized builds, it should include `optimized` in the version string, e.g., `KFR 7.x.x optimized` and should not include `debug`.

### Undefined symbol: kfr::XXX

This usually indicates that KFR libraries are not being linked correctly.

Ensure that all modules you are using are linked in your build system.

```cmake
find_package(KFR CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kfr kfr_dsp kfr_dft kfr_io kfr_audio)
```

If you're not using CMake, make sure to include the appropriate library files in your linker settings.

### Undefined symbol: FLAC__XXX

This indicates that the FLAC library is not being linked correctly.

CMake automatically links the FLAC library when building with the `kfr_audio` module.

If you're not using CMake, make sure to add `FLAC.lib` (or `FLAC.a` on Linux/macOS) to your linker settings.

Prebuilt KFR binaries already include FLAC library.

### relocation XXX against symbol `YYY' can not be used when making a shared object; recompile with -fPIC

Linux and macOS require PIC (Position Independent Code) to be enabled for shared libraries.

If you're building KFR from source, ensure that the `-fPIC` flag is added to your compiler flags.

When using CMake, `CMAKE_POSITION_INDEPENDENT_CODE` should be set to `ON`.

Prebuilt KFR binaries are already built with position-independent code (PIC) enabled on Linux and macOS.

### Building KFR DFT fails with Vcpkg on Linux

By default Vcpkg configures builds to use GCC on Linux, which may lead to build failures because DFT relies on extensive function inlining for performance. Clang handles this efficiently, requiring a reasonable amount of memory.

#### Solution

[See Building with Clang on Linux via vcpkg](#building-with-clang-on-linux-via-vcpkg) for a workaround.

### Build fails on Windows with Unknown Compile Options or "could not open stdc++.lib"

You're likely building KFR with clang.exe, which is GNU-compatible, but the MSVC-compatible clang-cl.exe is required.

#### Solution

To fix this just change both `CMAKE_CXX_COMPILER` and `CMAKE_C_COMPILER` to `C:/Program Files/LLVM/bin/clang-cl.exe` or simply `clang-cl` if `C:/Program Files/LLVM/bin/` is already in your `PATH`.

