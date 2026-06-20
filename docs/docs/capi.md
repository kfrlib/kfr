# KFR C API

## Overview

The KFR C API (`kfr_capi`) is a dynamic library that exposes the core DSP functionality of KFR —
DFT/FFT, DCT, and FIR/IIR/convolution filters — through a plain C99 interface.

The library is built as a shared object (`kfr_capi.dll` on Windows, `libkfr_capi.so` on Linux,
`libkfr_capi.dylib` on macOS) and can be linked directly from C/C++ or loaded through FFI from
any target language that supports dynamic loading (Python, Rust, Go, Node.js, C#, Julia, …).

All public symbols use the `kfr_` prefix and are exported with the default calling convention
of the host platform. The single header to include is `kfr/capi.h`.

## Linking and loading

### Linking from C/C++

Add `kfr/capi.h` to your include path and link against the `kfr_capi` shared library.

```c
#include <kfr/capi.h>

int main(void)
{
    printf("KFR %s\n", kfr_version_string());
    return 0;
}
```

### Loading through FFI

Because the C API is a plain dynamic library, it can be loaded at runtime from any language
that supports FFI. The example below uses Python with `ctypes`, but the same pattern applies
to `libload`/`cgo`/`dlopen`/`P/Invoke`/`@FFI`/… in other languages.

```python
import ctypes
import platform

name = "kfr_capi.dll" if platform.system() == "Windows" else "libkfr_capi.so"
kfr = ctypes.CDLL(name)

kfr.kfr_version_string.restype = ctypes.c_char_p
print("KFR", kfr.kfr_version_string().decode())

kfr.kfr_current_arch.restype = ctypes.c_int
print("Current arch:", kfr.kfr_current_arch())
```

!!! note
    On Windows the import library is `kfr_capi.lib`. Make sure `kfr_capi.dll` is on the
    `PATH` of the host process at runtime.

## Conventions

* All functions that operate on a plan take the plan as their first argument.
* Plans are created by `kfr_*_create_*` functions and must be released by the matching
  `kfr_*_delete_*` function. Passing `NULL` to a delete function is safe.
* Forward transforms perform **no scaling**. To obtain a properly scaled inverse, divide the
  result by the size of the transform.
* Functions that accept a `temp` scratch buffer may receive `NULL`; in that case KFR allocates
  the scratch buffer internally. Preallocating it (see `kfr_*_get_temp_size_*`) avoids this
  allocation and may improve performance.
* Output buffers may alias the input buffer for in-place execution, unless stated otherwise.
* Complex samples are represented as `kfr_c32` / `kfr_c64`. When the C99 complex type is
  available (`__STDC_IEC_559_COMPLEX__`), they map to `float _Complex` / `double _Complex`;
  otherwise they are plain `float` / `double` arrays of length `2 * N` (interleaved real/imag),
  and `KFR_COMPLEX_SIZE_MULTIPLIER` is `2`.

## Memory

`kfr_allocate` / `kfr_allocate_aligned` / `kfr_deallocate` provide memory with the same
alignment guarantees used internally by KFR (`KFR_DEFAULT_ALIGNMENT` is 64 bytes). Buffers
returned by these functions may be passed directly to the API functions.

```c
kfr_c32 *in  = kfr_allocate_aligned(256 * sizeof(kfr_c32), 64);
kfr_c32 *out = kfr_allocate_aligned(256 * sizeof(kfr_c32), 64);
/* ... use in/out ... */
kfr_deallocate(in);
kfr_deallocate(out);
```

## Versioning and introspection

```c
const char *ver  = kfr_version_string();   /* e.g. "7.0.0" */
uint32_t    num  = kfr_version();          /* e.g. 70000 */
const char *arch = kfr_enabled_archs();    /* e.g. "AVX2;SSE2;Generic" */
int         cur  = kfr_current_arch();     /* one of KFR_ARCH_* */
const char *err  = kfr_last_error();       /* last error message, or NULL */
```

## DFT (complex)

A complex DFT plan is created for a specific size and reused for every execution of that size.

```c
#include <kfr/capi.h>
#include <stdio.h>

int main(void)
{
    const size_t N = 256;
    KFR_DFT_PLAN_F32 *plan = kfr_dft_create_plan_f32(N);

    kfr_c32 *in   = kfr_allocate_aligned(N * sizeof(kfr_c32), 64);
    kfr_c32 *out  = kfr_allocate_aligned(N * sizeof(kfr_c32), 64);
    uint8_t *temp = kfr_allocate_aligned(kfr_dft_get_temp_size_f32(plan), 64);

    /* fill `in` with N complex samples ... */

    kfr_dft_execute_f32(plan, out, in, temp);          /* forward  */
    /* out now holds the forward DFT, no scaling applied */

    kfr_dft_execute_inverse_f32(plan, in, out, temp); /* inverse, in-place into `in` */

    kfr_deallocate(in);
    kfr_deallocate(out);
    kfr_deallocate(temp);
    kfr_dft_delete_plan_f32(plan);
    return 0;
}
```

Multi-dimensional complex DFT plans are available through `kfr_dft_create_2d_plan_f**`,
`kfr_dft_create_3d_plan_f**` and `kfr_dft_create_md_plan_f**`. The input/output buffers must
be laid out in row-major order with a total of `size1 * size2 * ...` complex samples.

## DFT (real)

Real-to-complex transforms pack the output into either `Perm` ($N/2$ complex values) or
`CCs` ($N/2 + 1$ complex values) format, selected at plan creation time. The size **must be
even**.

```c
const size_t N = 1024;
KFR_DFT_REAL_PLAN_F32 *plan = kfr_dft_real_create_plan_f32(N, CCs);

kfr_f32  *in   = kfr_allocate_aligned(N * sizeof(kfr_f32), 64);
kfr_c32  *out  = kfr_allocate_aligned((N / 2 + 1) * sizeof(kfr_c32), 64);
uint8_t  *temp = kfr_allocate_aligned(kfr_dft_real_get_temp_size_f32(plan), 64);

/* fill `in` with N real samples ... */
kfr_dft_real_execute_f32(plan, out, in, temp);

/* inverse: reads N/2+1 complex, writes N real */
kfr_dft_real_execute_inverse_f32(plan, in, out, temp);

kfr_dft_real_delete_plan_f32(plan);
```

For 2D / 3D / N-dimensional real DFTs use `kfr_dft_real_create_{2d,3d,md}_plan_f**`. The
`real_out_is_enough` flag selects whether the inverse transform only needs the real output
buffer (saves memory when the complex spectrum is not required afterwards).

## DCT-II

DCT-II and its inverse (DCT-III) are available for even sizes. As with the DFT, no scaling
is applied.

```c
KFR_DCT_PLAN_F64 *plan = kfr_dct_create_plan_f64(256);

kfr_f64 *in   = kfr_allocate_aligned(256 * sizeof(kfr_f64), 64);
kfr_f64 *out  = kfr_allocate_aligned(256 * sizeof(kfr_f64), 64);
kfr_dct_execute_f64(plan, out, in, NULL);          /* forward, internal temp */
kfr_dct_execute_inverse_f64(plan, in, out, NULL);  /* inverse, in-place */

kfr_dct_delete_plan_f64(plan);
```

## Filters

### FIR

```c
const kfr_f32 taps[5] = {0.1f, 0.2f, 0.4f, 0.2f, 0.1f};
KFR_FILTER_F32 *fir = kfr_filter_create_fir_plan_f32(taps, 5);

kfr_f32 in[1024], out[1024];
kfr_filter_process_f32(fir, out, in, 1024);

kfr_filter_reset_f32(fir);          /* clear the delay line */
kfr_filter_delete_plan_f32(fir);
```

### Convolution (FFT overlap-add)

For long impulse responses the convolution plan is significantly faster than a direct FIR.

```c
const kfr_f32 taps[4096] = { /* impulse response */ };
KFR_FILTER_F32 *conv =
    kfr_filter_create_convolution_plan_f32(taps, 4096, 8192);  /* block_size must be a power of two */

kfr_filter_process_f32(conv, out, in, 4096);
kfr_filter_delete_plan_f32(conv);
```

### IIR (second-order sections)

IIR filters are described by a flat array of second-order sections, each section using six
coefficients `[b0, b1, b2, a1, a2, gain]` (see the C++ IIR documentation for the exact layout).

```c
const kfr_f64 sos[6] = {1.0, 0.0, 0.0, 0.5, 0.2, 1.0};
KFR_FILTER_F64 *iir = kfr_filter_create_iir_plan_f64(sos, 1);

kfr_filter_process_f64(iir, out, in, N);
kfr_filter_delete_plan_f64(iir);
```

## Error handling

The C API does not return error codes. After any call that may fail (e.g. invalid size, invalid
plan pointer, allocation failure), call `kfr_last_error()` to retrieve a human-readable
message. A `NULL` return means no error is pending.

```c
KFR_DFT_REAL_PLAN_F32 *bad = kfr_dft_real_create_plan_f32(7 /* odd */, Perm);
if (bad == NULL) {
    fprintf(stderr, "kfr: %s\n", kfr_last_error());
}
```

## Thread safety

* Plan creation, deletion, `dump`, `get_size`, `get_temp_size` and `process`/`execute` calls
  on **distinct** plans are independent and may be called concurrently from multiple threads.
* A single plan must not be executed from two threads at the same time; create one plan per
  thread, or serialise access externally.
* `kfr_last_error()` is thread-local; each thread sees its own last error.

## Fetching C API binaries

KFR publishes C API binaries with every release and every commit. Prebuilt libraries for the
supported platforms are available on the
[GitHub releases page](https://github.com/kfrlib/kfr/releases).

## Building the C API

Building the C API requires Clang; see [Installation](installation.md) for details. The C API
is supported on non-x86 platforms as well, and `KFR_ENABLE_MULTIARCH` (enabled by default)
must remain on.

### Windows

The following commands must be run from the MSVC command prompt.

```bash
cd <path_to_kfr_repository>
cmake -B build -GNinja -DCMAKE_INSTALL_PREFIX=path/to/install/dir -DENABLE_CAPI_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="<PATH_TO_LLVM_DIR>/bin/clang-cl.exe" ..
ninja -C build install
```

### Linux, macOS, and other platforms

On Linux and macOS the `CMAKE_POSITION_INDEPENDENT_CODE` option is required when building the
C API:

```bash
cd <path_to_kfr_repository>
cmake -B build -GNinja -DCMAKE_INSTALL_PREFIX=path/to/install/dir -DENABLE_CAPI_BUILD=ON -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ ..
ninja -C build install
```

## C API Reference

See [C API Reference](auto/groups/capi.md).
