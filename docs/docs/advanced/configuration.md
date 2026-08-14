# Advanced Configuration

KFR is designed to work with its default CMake configuration, but a source
build can be tailored for a particular CPU, deployment target, feature set, or
integration constraint. This guide describes the build switches and
compile-time macros that affect KFR's public interface and implementation.

Most applications should start with the standard instructions in
[Installation](../getting-started/installation.md). Change the configuration
only when a concrete requirement calls for it: for example, to make a portable
x86 binary, omit optional modules, enable the C API, or select a different
memory or path interface.

> [!important]
> Build options and public configuration macros are part of the effective KFR
> ABI. Use one configuration consistently for KFR and every translation unit
> that includes its headers. When changing a configuration that affects public
> types or inline functions, use a fresh build directory and rebuild KFR and
> its consumers.

## Configuration workflow

Configure KFR by passing cache values when CMake first generates a build
directory. Ninja is recommended for single-configuration builds:

```shell
cmake -S . -B build-release -GNinja \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/path/to/kfr-prefix \
	-DKFR_ARCH=sse2 \
	-DKFR_ENABLE_MULTIARCH=ON \
	-DKFR_ENABLE_CAPI_BUILD=ON
cmake --build build-release
cmake --install build-release
```

For a Debug installation, configure a separate `build-debug` directory with
`-DCMAKE_BUILD_TYPE=Debug` and the same installation prefix. This is the layout
used for the release packages: Release libraries are installed in `lib` and
Debug libraries in `lib/debug`.

With Visual Studio or Xcode, configure once, then select `Release` or `Debug`
while building and installing. See [Installation](../getting-started/installation.md)
for complete commands and for consuming the resulting package with
`find_package(KFR CONFIG REQUIRED)`.

### Changing an existing configuration

CMake retains cache entries in the build directory. Pass a new `-D` value to
reconfigure an ordinary option, but remove and recreate the build directory
when changing any of the following:

* the compiler, target platform, toolchain file, or `KFR_ARCH`;
* the enabled module set (`KFR_ENABLE_DFT`, `KFR_ENABLE_DSP`,
	`KFR_ENABLE_IO`, or `KFR_ENABLE_AUDIO`);
* allocation, default-precision, filesystem, vector-backend, or FFT-selection
	macros; or
* the architecture set of a multiarchitecture build.

Do not combine libraries from different KFR prefixes or build configurations.
A package for a different OS, ABI, target architecture, or compiler runtime
isn't a cross-compilation shortcut — configure the target toolchain and build
KFR for that target instead.

## Select a CPU architecture

`KFR_ARCH` selects the baseline architecture compiled into KFR. It defaults to
`target`: KFR detects the native CPU for a native x86 build, and chooses the
appropriate NEON target on ARM. For a controlled deployment, set the lowest
instruction-set level that every target machine supports:

```shell
cmake -S . -B build -GNinja -DKFR_ARCH=sse2
```

The supported values depend on the target:

| Target family | `KFR_ARCH` values |
| --- | --- |
| x86 / x86_64 | `generic`, `sse`, `sse2`, `sse3`, `ssse3`, `sse41`, `sse42`, `avx`, `avx2`, `avx512`, `target` |
| ARM / AArch64 | `generic`, `neon`, `neon64`, `target` |
| RISC-V | `generic`, `rvv`, `target` |

`detect` is accepted as an alias for `target`. On non-x86 cross builds,
automatic CPU detection is unavailable; select an explicit architecture and
pass the matching CMake toolchain file. KFR requires the RISC-V Vector
extension for its RISC-V implementation.

### x86 runtime dispatch

On x86, `KFR_ENABLE_MULTIARCH=ON` builds several ISA variants and dispatches
to the best one at runtime for the components that support multiarchitecture
dispatch. It is enabled by default on x86 and forced off on non-x86, Android,
and Emscripten builds. `KFR_ARCHS` is a semicolon-separated list of the variants
to build:

```shell
cmake -S . -B build -GNinja \
	-DKFR_ARCH=sse2 \
	-DKFR_ENABLE_MULTIARCH=ON \
	-DKFR_ARCHS="sse2;sse41;avx;avx2;avx512"
```

For a small binary or a known fixed target, disable dispatch and select exactly
one architecture:

```shell
cmake -S . -B build -GNinja \
	-DKFR_ARCH=avx2 \
	-DKFR_ENABLE_MULTIARCH=OFF
```

The selected instructions must be available on every CPU that runs the binary.
Use a conservative baseline such as `sse2` for broadly compatible x86/x86_64
distribution. The CI test matrix validates multiple x86 ISAs with Intel SDE;
this does not make an `avx2`-only binary safe on an SSE2-only machine.

## CMake options

The following are the project-level CMake options. Set Boolean values to `ON`
or `OFF` during configuration, for example
`-DKFR_ENABLE_AUDIO=OFF`.

### Modules, examples, and installation

| Option | Default | Meaning |
| --- | --- | --- |
| `KFR_ENABLE_DSP` | `ON` | Build and export `kfr_dsp`, which provides filters, resampling, and other DSP algorithms. |
| `KFR_ENABLE_DFT` | `ON` with Clang or GCC; `OFF` with MSVC | Build and export `kfr_dft`, including FFT, DFT, convolution, and related algorithms. |
| `KFR_ENABLE_IO` | `ON` | Build and export `kfr_io`. |
| `KFR_ENABLE_AUDIO` | `ON` | Build and export `kfr_audio`. It requires both DSP and I/O to be enabled. |
| `KFR_ENABLE_CAPI_BUILD` | `OFF` | Build and install the `kfr_capi` shared C API library. It requires `KFR_ENABLE_DFT=ON`. |
| `ENABLE_TESTS` | `OFF` | Add the KFR test suite to the build. |
| `ENABLE_EXAMPLES` | `ON` when tests are enabled; otherwise `OFF` | Build the examples and tools. This is a dependent option controlled by `ENABLE_TESTS`. |
| `KFR_INSTALL_HEADERS` | `ON` | Install public headers and the generated `kfr/config.h`. |
| `KFR_INSTALL_LIBRARIES` | `ON` | Install the enabled compiled module libraries. |
| `KFR_DEBUG_INSTALL_SUFFIX` | `/debug` | Installation subdirectory suffix for Debug libraries. |
| `KFR_INSTALL_CMAKEDIR` | `lib/cmake/kfr` | Installation directory for `KFRConfig.cmake` and exported targets. |

The header-only `kfr` core target is always available. Disabling a module
removes its target and public API from that build; a consumer cannot restore it
with `find_package` components. Link only to targets that exist in the package,
as described in [Installation](../getting-started/installation.md).

### Performance and code generation

| Option | Default | Meaning |
| --- | --- | --- |
| `KFR_ARCH` | `target` | Baseline CPU architecture. See [Select a CPU architecture](#select-a-cpu-architecture). |
| `KFR_ENABLE_MULTIARCH` | `ON` on x86; forced `OFF` elsewhere | Build x86 architecture variants with runtime dispatch. |
| `KFR_ARCHS` | platform-dependent | Semicolon-separated architecture list for a multiarchitecture x86 build. The non-Apple default is `sse2;sse41;avx;avx2;avx512`; the Apple default omits `sse2`. |
| `KFR_DISABLE_CLANG_EXTENSIONS` | `OFF` | Disable KFR's automatic Clang vector-extension backend and use the generic SIMD backend instead. This is an advanced compatibility/code-generation switch. |
| `KFR_CLASSIC_FFT` | `OFF` | Select the classic, pre-7.1 FFT implementation. Enable only for compatibility, comparison, or regression investigation. |
| `KFR_BASETYPE_F32` | `OFF` | Use `float` instead of `double` as KFR's default scalar type. See [`KFR_BASETYPE_F32`](#kfr_basetype_f32). |
| `KFR_MANAGED_ALLOCATION` | `OFF` | Use KFR managed aligned allocation with refcounting and copy-free aligned reallocation. See [Allocation configuration](#allocation-configuration). |
| `KFR_USE_STD_FILESYSTEM` | `OFF` | Request `std::filesystem::path` for I/O and audio path APIs. See [Filesystem path interface](#filesystem-path-interface). |

### Dependencies and optional algorithms

| Option | Default | Meaning |
| --- | --- | --- |
| `KFR_USE_BOOST_MATH` | `ON` | Fetch standalone Boost.Math with CMake `FetchContent` and enable elliptic IIR filter support. |
| `KFR_USE_BOOST` | `OFF` | Find and use an existing Boost.Math installation instead. Use this when the parent project already provides Boost. |

Either Boost option enables `KFR_HAVE_ELLIPTIC` in `kfr_dsp`. Set both to `OFF`
for an offline or restricted build that does not need elliptic filters. The
resulting build does not expose the elliptic IIR design API.

### Tests and diagnostics

| Option | Default | Meaning |
| --- | --- | --- |
| `KFR_EXTENDED_TESTS` | `OFF` | Instantiate a substantially larger SIMD test matrix. It can take up to an hour. |
| `KFR_SKIP_TESTS` | `OFF` | Build test targets but do not register/run them. Useful for targets that cannot execute on the build host. |
| `KFR_ENABLE_COVERAGE` | `OFF` | When configuring the test subdirectory with Clang, add LLVM source-based coverage instrumentation. |
| `KFR_NO_PERF_TESTS` | `OFF` | Omit performance tests from the test build. |
| `KFR_ARCH_TESTS` | unset | Comma-separated architecture list used by the test build to choose ISA test variants. |
| `KFR_USE_SDE` | unset | Run selected x86 ISA tests through Intel SDE. Intended for KFR's CI infrastructure. |
| `KFR_ENABLE_ASMTEST` | `OFF` | Generate disassembly-oriented tests; an advanced option. |
| `KFR_REGENERATE_TESTS` | `OFF` | Regenerate automatic test data; an advanced option. |

KFR's CI enables extended tests in its normal test drivers. It cross-builds ARM,
AArch64, RISC-V, Android, iOS, and Windows ARM64 configurations; some targets
set `KFR_SKIP_TESTS=ON` because they cannot run on the hosted build machine.
Treat a successful cross-build as compile validation, not as target runtime
validation.

## Public configuration macros

KFR headers contain inline code and templates, so a preprocessor macro that
changes their behaviour must be supplied to **every** translation unit using
that configuration. Prefer the corresponding KFR CMake option when one is
available. These options propagate their definitions to all KFR static-library
sources and to source-tree consumers:

```cmake
set(KFR_BASETYPE_F32 ON CACHE BOOL "" FORCE)
add_subdirectory(external/kfr)

target_link_libraries(my_app PRIVATE kfr kfr_dsp kfr_audio)
```

For a source-tree integration, set CMake options before `add_subdirectory`:

```cmake
set(KFR_ENABLE_DFT ON CACHE BOOL "" FORCE)
set(KFR_MANAGED_ALLOCATION ON CACHE BOOL "" FORCE)
add_subdirectory(external/kfr)

target_link_libraries(my_app PRIVATE kfr kfr_dsp kfr_dft)
```

For an installed KFR package, the installed generated `kfr/config.h` records
the definitions selected by supported KFR CMake options. Do not add a
conflicting macro only to an application that consumes a prebuilt or separately
installed KFR package.

### Allocation configuration

#### `KFR_MANAGED_ALLOCATION`

`KFR_MANAGED_ALLOCATION` changes KFR's aligned allocator from its normal
platform allocator to a managed allocation block. A managed block records its
size and alignment, maintains a reference count, and supports aligned
reallocation. This enables the C API and internal allocation helpers to retain
or resize memory without copying where possible.

Enable it with:

```shell
cmake -S . -B build -DKFR_MANAGED_ALLOCATION=ON
```

It is `OFF` by default, including in release packages. Because allocation
ownership and exported C API operations differ, do not allocate with one mode
and deallocate, retain, or reallocate with another. Rebuild all of KFR and its
consumers after changing it.

#### `KFR_USE_STD_ALLOCATION`

`KFR_USE_STD_ALLOCATION` is a direct C++ preprocessor macro, not a KFR CMake
option. When defined, `kfr::data_allocator<T>` is `std::allocator<T>` instead
of KFR's 64-byte-aligned allocator. In practice this makes `univector`
allocation behavior compatible with `std::vector` at the cost of KFR's
SIMD-oriented container alignment.

```cmake
target_compile_definitions(my_app PRIVATE KFR_USE_STD_ALLOCATION=1)
```

This macro is independent of `KFR_MANAGED_ALLOCATION`: the former selects the
allocator type used by containers, while the latter changes the implementation
of KFR aligned allocation. Apply it consistently wherever KFR container types
cross a translation-unit or binary boundary.

### Precision and SIMD capability

#### `KFR_NO_NATIVE_F64`

`KFR_NO_NATIVE_F64` means that the selected target lacks native SIMD `double`
support. KFR automatically defines it for 32-bit ARM NEON; it normally should
not be set manually. When it is defined, `KFR_NATIVE_F64` is absent and
`kfr::fbase` becomes `float`.

It's a target-capability setting, so use `KFR_BASETYPE_F32` instead when you
want `float` as the default scalar type on an otherwise double-capable
target.

#### `KFR_BASETYPE_F32`

The `KFR_BASETYPE_F32` CMake option makes `kfr::fbase` equal to `float`;
otherwise `fbase` is `double` on targets with native f64 support. It defines
the `KFR_BASETYPE_F32` preprocessor macro for all KFR library sources and
consumers, and influences APIs and containers that use `fbase` as their default
sample or scalar type, including audio conversion paths.

Configure it when generating the KFR build:

```shell
cmake -S . -B build -DKFR_BASETYPE_F32=ON
```

Use it when a single-precision default reduces memory usage or matches an
existing float audio pipeline. It does not remove explicit `double` APIs. Keep
the choice identical for every KFR source and consumer translation unit. The
CMake option ensures a source-tree build applies the setting to `kfr_audio` and
all other static libraries. An inconsistent manual definition between a
consumer and a compiled KFR library changes symbol types and can result in
linker errors. Rebuild KFR and all consumers after changing this option.

#### `KFR_VEC_EXT`

`KFR_VEC_EXT` selects KFR's vector-extension SIMD backend. KFR defines it
automatically for Clang unless `KFR_DISABLE_CLANG_EXT` is defined. In a CMake
build, use the supported switch below rather than defining `KFR_VEC_EXT`
yourself:

```shell
cmake -S . -B build -DKFR_DISABLE_CLANG_EXTENSIONS=ON
```

That option defines `KFR_DISABLE_CLANG_EXT` and forces KFR's generic SIMD
backend. This may be useful when investigating compiler-specific code
generation or a compiler compatibility problem, but it can change performance
and should be tested on the actual target. Do not force `KFR_VEC_EXT` for a
compiler that KFR does not automatically support.

#### `KFR_DISABLE_OPTIMIZED_SHUFFLE`

`KFR_DISABLE_OPTIMIZED_SHUFFLE` is a direct preprocessor macro. It disables a
specialized optimized shuffle path for full power-of-two `float` and `double`
SIMD vectors, leaving the generic shuffle implementation available:

```cmake
target_compile_definitions(my_app PRIVATE KFR_DISABLE_OPTIMIZED_SHUFFLE=1)
```

It is primarily a compiler/code-generation troubleshooting switch. It changes
performance rather than the public algorithmic result; benchmark and test the
affected target before retaining it.

### Algorithms and test scope

#### `KFR_CLASSIC_FFT`

`KFR_CLASSIC_FFT` selects the classic FFT code path used before KFR 7.1,
including its progressive FFT interface. The supported source-build control is:

```shell
cmake -S . -B build -DKFR_ENABLE_DFT=ON -DKFR_CLASSIC_FFT=ON
```

It has no effect when the DFT module is disabled. Use it for compatibility or
comparison rather than as a default performance setting. Build KFR and every
consumer with the same FFT selection; do not mix a classic-FFT header
configuration with a library compiled for the current implementation.

The current source build adds this definition while compiling KFR but does not
record it in the installed generated configuration header. Therefore, after
installing a classic-FFT build, add `KFR_CLASSIC_FFT=1` explicitly to consumer
targets as well. Verify an installed-package consumer after enabling it.

#### `KFR_HAVE_ELLIPTIC`

`KFR_HAVE_ELLIPTIC` is generated by the DSP build when either
`KFR_USE_BOOST_MATH` or `KFR_USE_BOOST` is enabled. It exposes elliptic (Cauer)
IIR filter design, which needs Boost.Math elliptic and Jacobi functions.
Applications don't normally define this macro themselves.

* Keep the default `KFR_USE_BOOST_MATH=ON` to obtain standalone Boost.Math
	automatically during configuration.
* Set `KFR_USE_BOOST=ON` when the build already supplies Boost.Math.
* Set both options to `OFF` to remove that dependency and the elliptic filter
	API.

This setting only applies when `KFR_ENABLE_DSP=ON`. It does not affect the
other IIR design families described in [IIR filters](../dsp/iir.md).

#### `KFR_EXTENDED_TESTS`

`KFR_EXTENDED_TESTS` expands the SIMD test-instantiation matrix beyond the
quick default sizes. It is controlled by the CMake option of the same name:

```shell
cmake -S . -B build-tests -DENABLE_TESTS=ON -DKFR_EXTENDED_TESTS=ON
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

The expanded suite can take up to an hour. It is for validating a KFR build;
it is not an application feature and should normally remain disabled in
production package builds.

### Filesystem path interface

#### `KFR_USE_STD_FILESYSTEM`

`KFR_USE_STD_FILESYSTEM` selects `std::filesystem::path` as the path type used
by KFR I/O and audio APIs. Without it, KFR uses `std::string` on non-Windows
platforms and `std::wstring` on Windows; Windows builds also provide UTF-8
`std::string` convenience overloads in that default mode.

The project exposes `KFR_USE_STD_FILESYSTEM` as a CMake cache option, but the
current build scripts do not propagate that option to a C++
`KFR_USE_STD_FILESYSTEM` definition. Until that wiring is added, supply the
definition while compiling KFR and every consumer. For a source-tree build,
set it before KFR targets are created:

```cmake
set(KFR_USE_STD_FILESYSTEM ON CACHE BOOL "" FORCE)
add_compile_definitions(KFR_USE_STD_FILESYSTEM=1)
add_subdirectory(external/kfr)
```

For an installed package, rebuild KFR with the same definition rather than
changing only the consumer, then add the definition to the consumer target.
Path type changes are source and ABI compatibility changes for functions that
accept file paths.

## Practical build profiles

### Small core-only build

For numeric, container, and expression facilities without compiled modules:

```shell
cmake -S . -B build -GNinja \
	-DKFR_ENABLE_DSP=OFF \
	-DKFR_ENABLE_DFT=OFF \
	-DKFR_ENABLE_IO=OFF \
	-DKFR_ENABLE_AUDIO=OFF \
	-DKFR_ENABLE_MULTIARCH=OFF \
	-DKFR_ARCH=sse2
```

Consumers link only the header-only `kfr` target.

### Audio and C API package

The release workflow builds a C API-enabled package with a conservative x86
baseline and position-independent code. A similar native Linux configuration
is:

```shell
cmake -S . -B build-release -GNinja \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/path/to/kfr-prefix \
	-DKFR_ENABLE_CAPI_BUILD=ON \
	-DKFR_ENABLE_DFT=ON \
	-DKFR_ARCH=sse2 \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
```

Audio codec support is discovered at configure time. The release workflows use
vcpkg for FLAC and Minimp3 and pass the selected vcpkg installation directory
through `CMAKE_PREFIX_PATH`. Use the same pattern when a source build needs
codec support.

### Cross-compilation

Cross-compilation requires a target toolchain and target dependencies, not
just a different `KFR_ARCH`. For example, KFR's CI invokes the ARM, AArch64,
and RISC-V toolchain files under `cmake/`, and Android builds use the Android
NDK CMake toolchain with an `ANDROID_ABI` value. A typical pattern is:

```shell
cmake -S . -B build-arm -GNinja \
	-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake \
	-DKFR_ARCH=target \
	-DENABLE_EXAMPLES=OFF
```

Pass the target dependency prefix through `CMAKE_PREFIX_PATH` when optional
I/O or audio dependencies are needed. Build-only validation is appropriate
when the target executable cannot run on the host; run the resulting tests on
the device or an emulator for runtime verification.

## Configuration checklist

Before distributing a custom KFR build:

1. Choose a baseline `KFR_ARCH` that every deployment CPU supports.
2. Enable `KFR_ENABLE_MULTIARCH` only for x86 binaries that need runtime ISA
	 dispatch, and explicitly review `KFR_ARCHS`.
3. Enable exactly the module targets that consumers need.
4. Decide whether Boost.Math/elliptic filters and external audio codecs are
	 required, especially for offline builds.
5. Treat allocation, `fbase`, filesystem, and FFT choices as uniform
	 build-wide settings.
6. Install both Release and Debug variants when consumers build both
	 configurations.
7. Validate with a clean consumer using `find_package`, as the release
	 workflow does through `tests/usage-config` and `tests/usage-manual`.
