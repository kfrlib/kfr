# KFR C API

## Building KFR C API

Clang is required. See [Installation](installation.md)

C API is supported on non-x86 platforms.
C API requires that `KFR_ENABLE_MULTIARCH` be enabled (enabled by default).

### Windows

These commands must be executed in the MSVC command prompt.

```bash
cd <path_to_kfr_repository>
cmake -B build -GNinja -DCMAKE_INSTALL_PREFIX=path/to/install/dir -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="<PATH_TO_LLVM_DIR>/bin/clang-cl.exe" ..
ninja -C build install
```

### Linux, macOS, other

On Linux and macOS, the `CMAKE_POSITION_INDEPENDENT_CODE` CMake option is required for building the C API:

```bash
cd <path_to_kfr_repository>
cmake -B build -GNinja -DCMAKE_INSTALL_PREFIX=path/to/install/dir -DENABLE_CAPI_BUILD=ON -DDCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
ninja -C build install
```
