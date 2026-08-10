# KFR Knowledge Base

## Upgrading to KFR 7

See the [KFR 7 Upgrade Guide](../getting-started/upgrade7.md) for instructions on updating a codebase for KFR 7.

## Building with Clang on Linux via vcpkg

Create this custom triplet in the vcpkg checkout or a fork of it:

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

Build with `--triplet x64-linux-clang`, or set `VCPKG_DEFAULT_TRIPLET=x64-linux-clang`.

You can instead place the triplet in an overlay directory and pass
`--overlay-triplets=/path/to/overlay`. In that case, adjust
`VCPKG_CHAINLOAD_TOOLCHAIN_FILE` so that it resolves to the toolchain file in
the vcpkg checkout.

## Applying two FIR filters in parallel

For two FIR filters operating on the same, time-aligned input at the same sample
rate, sum their coefficient vectors and create one filter from the result.

The resulting filter produces the sum of the two filter outputs. To combine FIR
filters in series, convolve their coefficient vectors. This equivalence assumes
compatible delay/state handling; keep separate streaming filters when their
states must remain independent.

## How to Build KFR on macOS with Universal Binaries

### Using CMake

Set the following CMake variables when configuring the build:
```bash
-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DKFR_ARCH=sse41
```

All linked third-party dependencies must also contain both architectures.
Prebuilt universal binaries are available from GitHub releases.

## Note on AVX10

KFR does not provide a separate AVX10 target. AVX10-512 processors expose the
AVX-512 feature flags that KFR's `avx512` path checks, so KFR detects them as
`avx512` and uses that path automatically.

AVX10 also has a 256-bit vector-width variant. KFR does not support that
variant yet.

## Potential Issues

### Performance is slow

Ensure that you're building KFR with optimizations enabled.
CMake should be configured with `-DCMAKE_BUILD_TYPE=Release` or `-DCMAKE_BUILD_TYPE=RelWithDebInfo` for best performance.

If you're using your own build system, make sure to define the `NDEBUG` macro and enable optimizations in your compiler flags (e.g., `-O3` for GCC/Clang or `/O2` for MSVC).

The following line prints KFR's version and build-mode markers:
```c++
||||||||||
println(library_version());
||||||||||
```

`optimized` indicates that `NDEBUG` or `KFR_NDEBUG` was defined, while `debug`
indicates that `DEBUG` or `KFR_DEBUG` was defined. These are build-mode markers,
not proof that compiler optimization was enabled.

### Undefined symbol: kfr::XXX

This usually indicates that KFR libraries are not being linked correctly.

Link the targets for the modules that your application uses and that the KFR
package was built with. For example:

```cmake
find_package(KFR CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE kfr kfr_dsp)
```

Add `kfr_dft`, `kfr_io`, or `kfr_audio` only when required and available.

If you're not using CMake, make sure to include the appropriate library files in your linker settings.

### Undefined symbol: FLAC__XXX

This indicates that the FLAC library is not being linked correctly.

CMake propagates the required FLAC linkage through `kfr_audio` when KFR was
built with FLAC support.

For a manual build, link the matching FLAC library and its transitive
dependencies, such as Ogg when required by that FLAC build.

FLAC support is optional and is selected when KFR is configured.

### relocation XXX against symbol `YYY' cannot be used when making a shared object; recompile with -fPIC

This ELF linker error means that position-independent code (PIC) is required
when static objects are linked into a shared library.

If you're building KFR from source, ensure that the `-fPIC` flag is added to your compiler flags.

When using CMake, `CMAKE_POSITION_INDEPENDENT_CODE` should be set to `ON`.

Configure PIC for source builds whenever the resulting static libraries will be
linked into a shared library.

### Building KFR DFT fails with vcpkg on Linux

KFR's DFT module is supported with Clang. GCC and MSVC have compiler limitations
that affect this code path.

#### Solution

[See Building with Clang on Linux via vcpkg](#building-with-clang-on-linux-via-vcpkg) for a workaround.

### Build fails on Windows with Unknown Compile Options or "could not open stdc++.lib"

You're likely building KFR with clang.exe, which is GNU-compatible, but the MSVC-compatible clang-cl.exe is required.

#### Solution

When using the MSVC ABI/toolchain, change both `CMAKE_CXX_COMPILER` and
`CMAKE_C_COMPILER` to `C:/Program Files/LLVM/bin/clang-cl.exe`, or simply
`clang-cl` if `C:/Program Files/LLVM/bin/` is already in your `PATH`. Recreate
the CMake build directory after changing compilers.

