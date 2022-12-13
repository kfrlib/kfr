# KFR C API

## Building KFR C API

Clang is required. See [Installation](installation.md)

### Windows

These commands must be executed in MSVC2019 command prompt.

```bash
cd <path_to_kfr_repository>
mkdir build && cd build
cmake -GNinja -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="<PATH_TO_LLVM_DIR>/bin/clang-cl.exe" ..
ninja kfr_capi
```

### Linux, macOS, other

```bash
cd <path_to_kfr_repository>
mkdir build && cd build
cmake -GNinja -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
ninja kfr_capi
```

Optionally, you can install the binaries into your system using `ninja install`
