# Loading and calling KFR from C

This article covers the mechanics of reaching `kfr_capi` from a program: link
options, the startup compatibility check, explicit loading, and FFI bindings.
Build the package first as described in
[Building a KFR C API package](building_kfr_c_api.md); the
[C API overview](../capi.md) describes the transforms, filters, and data
layouts themselves.

## Link and load

The installed `kfr_capi` CMake target supplies the header path, import library,
and configuration-specific artifact:

```cmake
find_package(KFR CONFIG REQUIRED)
add_executable(analyzer main.c)
target_link_libraries(analyzer PRIVATE kfr_capi)
```

At runtime the shared library must also be discoverable by the loader: next to
a Windows executable or on `PATH`, or through the platform search path, an
rpath, or a packaged library directory elsewhere. A Windows import library
alone does not make the DLL loadable.

## Check compatibility at startup

The header publishes the minimum library version it requires. Compare the two
before using anything else, which matters most when the library is deployed
independently of the program:

```c
#include <kfr/capi.h>
#include <stdio.h>

static int kfr_is_compatible(void)
{
    if (kfr_version() < KFR_HEADERS_VERSION)
    {
        fprintf(stderr, "KFR library is older than its C header\n");
        return 0;
    }
    printf("%s\n", kfr_version_string());
    return 1;
}
```

[[`::kfr_version`:nosig]] is encoded as
$\text{major} \times 10000 + \text{minor} \times 100 + \text{patch}$.
[[`::kfr_version_string`:nosig]] is diagnostic text with build and platform
details, and [[`::kfr_enabled_archs`:nosig]] and
[[`::kfr_current_arch`:nosig]] report the compiled and selected CPU variants of
a multiarchitecture build. A version check is a compatibility guard, not a
substitute for shipping a matching binary.

## Resolve symbols dynamically

Plugin hosts and most FFI systems load the library explicitly. Load exactly one
architecture-matched `kfr_capi` and resolve the C names as declared — never
mangled C++ names or non-`kfr_` exports:

```c
#ifdef _WIN32
#include <windows.h>
#include <kfr/capi.h>

typedef uint32_t(KFR_CDECL* version_fn)(void);
typedef KFR_DFT_PLAN_F32*(KFR_CDECL* create_dft_f32_fn)(size_t);
typedef void(KFR_CDECL* delete_dft_f32_fn)(KFR_DFT_PLAN_F32*);

int open_kfr(HMODULE* out_library, version_fn* out_version,
             create_dft_f32_fn* out_create, delete_dft_f32_fn* out_delete)
{
    HMODULE library = LoadLibraryW(L"kfr_capi.dll");
    if (library == NULL)
        return 0;

    version_fn version = (version_fn)GetProcAddress(library, "kfr_version");
    create_dft_f32_fn create =
        (create_dft_f32_fn)GetProcAddress(library, "kfr_dft_create_plan_f32");
    delete_dft_f32_fn destroy =
        (delete_dft_f32_fn)GetProcAddress(library, "kfr_dft_delete_plan_f32");

    if (version == NULL || create == NULL || destroy == NULL ||
        version() < KFR_HEADERS_VERSION)
    {
        FreeLibrary(library);
        return 0;
    }

    *out_library = library;
    *out_version = version;
    *out_create   = create;
    *out_delete   = destroy;
    return 1;
}
#endif
```

On Linux and macOS, load the corresponding shared-object name with `dlopen`.
Use `-ldl` when linking on platforms where the dynamic-loader functions are not
provided by the C library:

```c
#if defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#include <kfr/capi.h>

typedef uint32_t(KFR_CDECL* version_fn)(void);
typedef KFR_DFT_PLAN_F32*(KFR_CDECL* create_dft_f32_fn)(size_t);
typedef void(KFR_CDECL* delete_dft_f32_fn)(KFR_DFT_PLAN_F32*);

int open_kfr(void** out_library, version_fn* out_version,
       create_dft_f32_fn* out_create, delete_dft_f32_fn* out_delete)
{
#if defined(__APPLE__)
  const char* library_name = "libkfr_capi.dylib";
#else
  const char* library_name = "libkfr_capi.so";
#endif
  void* library = dlopen(library_name, RTLD_NOW | RTLD_LOCAL);
  if (library == NULL)
    return 0;

  version_fn version;
  create_dft_f32_fn create;
  delete_dft_f32_fn destroy;
  *(void**)(&version) = dlsym(library, "kfr_version");
  *(void**)(&create) = dlsym(library, "kfr_dft_create_plan_f32");
  *(void**)(&destroy) = dlsym(library, "kfr_dft_delete_plan_f32");

  if (version == NULL || create == NULL || destroy == NULL ||
    version() < KFR_HEADERS_VERSION)
  {
    dlclose(library);
    return 0;
  }

  *out_library = library;
  *out_version = version;
  *out_create  = create;
  *out_delete  = destroy;
  return 1;
}
#endif
```

Keep the module loaded until every plan has been deleted, all calls have
returned, and no finalizer can still reach the library.

## Call from foreign languages

A binding follows the C prototypes rather than KFR's C++ wrappers:

* Map `kfr_f32` and `kfr_f64` to native 32- and 64-bit IEEE floating-point
  types, and plan and filter handles to opaque pointers.
* Declare argument and result types explicitly. In Python `ctypes`, set both
  `argtypes` and `restype`; the default result type is wrong for pointers and
  `size_t`. Rust `libloading`, Go `cgo`, and .NET P/Invoke have the same
  requirement.
* Retain the loaded module while any handle or finalizer exists, and bind each
  handle finalizer to the matching `kfr_*_delete_*` function.
* Pass contiguous host-owned arrays for input and output. KFR does not retain
  them after a call returns.
* Represent complex DFT data as interleaved real/imaginary scalars unless the
  host's complex ABI is known to match C99 `_Complex`. In C, define
  `KFR_NO_C_COMPLEX_TYPES` to force that layout.
* Check every creation and allocation result for `NULL`, then read
  [[`::kfr_last_error`:nosig]] immediately and copy the message into a
  host-language error object.

The C boundary cannot make invalid pointers, use-after-delete, buffer overruns,
or wrong handle types safe. Validate sizes and lifetimes in the binding layer;
see [Memory management and ownership](c_api_memory_management.md).
