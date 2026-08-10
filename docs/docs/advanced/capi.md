# KFR C API

`kfr_capi` is KFR's shared-library interface for code that cannot use the C++
API directly. It exposes complex and real DFTs, DCT-II/DCT-III, and stateful
FIR, FFT-convolution, and IIR filters through C declarations in
`<kfr/capi.h>`, so it can be used from C and from any language with a C FFI.

The surface is intentionally small: fixed-width scalar aliases, opaque plan
handles, caller-provided buffers, and global `kfr_`-prefixed symbols. No C++
containers, templates, exceptions, callbacks, or object layouts cross the ABI
boundary.

## What the C API provides

| Family | Contents |
| --- | --- |
| Library information | [[`::kfr_version`:nosig]], [[`::kfr_version_string`:nosig]], [[`::kfr_enabled_archs`:nosig]], [[`::kfr_current_arch`:nosig]], [[`::kfr_last_error`:nosig]] |
| Allocation | [[`::kfr_allocate`:nosig]], [[`::kfr_allocate_aligned`:nosig]], [[`::kfr_deallocate`:nosig]] |
| Complex DFT | Reusable 1D, 2D, 3D, and N-dimensional plans in `f32` and `f64` |
| Real DFT | Real-to-complex and complex-to-real plans, with `Perm` or `CCs` packing for 1D |
| DCT | DCT-II plans and their DCT-III inverse |
| Filters | Stateful real FIR, FFT convolution, and IIR (biquad cascade) filters |

Every documented entry point has C linkage and a `kfr_` name. Other symbols
visible in a binary are C++ implementation details and are not part of this ABI.

`KFR_FILTER_C32` and `KFR_FILTER_C64` are declared, but this release has no
creation or processing functions for complex filters; filtering operates on
real `f32` or `f64` buffers.

## Header and library

```c
#include <kfr/capi.h>|||
```

The header is self-contained: it includes the generated `<kfr/config.h>` and
standard C headers only, compiles as C89, C99, or C++, and declares everything
`extern "C"` when included from C++. Do not redeclare imported functions by
hand — the header's `KFR_API_SPEC` macro supplies the import/export annotation
and the calling convention (`__cdecl` on 32-bit x86, the platform default
elsewhere).

| Platform | Library used at runtime | Link-time file |
| --- | --- | --- |
| Windows | `kfr_capi.dll` | `kfr_capi.lib` import library |
| Linux and other ELF systems | `libkfr_capi.so` | `libkfr_capi.so` |
| macOS | `libkfr_capi.dylib` | `libkfr_capi.dylib` |

The header and the library must come from the **same installed prefix**.
`config.h` records the options compiled into that installation, including
whether the managed-allocation functions are declared, so mixing a header and a
binary from different builds silently changes the declared ABI.

## Data types and buffer conventions

Real samples are [[`::kfr_f32`:nosig]] (`float`) and `kfr_f64` (`double`).
These are ABI aliases and are not affected by
[`KFR_BASETYPE_F32`](configuration.md#kfr_basetype_f32).

Complex DFT data uses [[`::kfr_c32`:nosig]] and `kfr_c64`. When the C compiler
defines `__STDC_IEC_559_COMPLEX__`, they are C99 `float _Complex` and
`double _Complex`, and [[`KFR_COMPLEX_SIZE_MULTIPLIER`:nosig]] is `1`.
Otherwise they are the underlying scalar type with interleaved real and
imaginary values, and the multiplier is `2`. Define `KFR_NO_C_COMPLEX_TYPES`
before including the header to force the interleaved representation:

```c
#define KFR_NO_C_COMPLEX_TYPES 1
|||#include <kfr/test/mini_catch.h>
#include <kfr/capi.h>

|||TEST_CASE("advanced/capi.md/interleaved complex storage")
|||{
enum { N = 256 };
kfr_f32 spectrum[N * KFR_COMPLEX_SIZE_MULTIPLIER];
/* spectrum[2*i] is real and spectrum[2*i + 1] is imaginary */
|||CHECK(KFR_COMPLEX_SIZE_MULTIPLIER == 2);
|||}
```

Interleaved storage is normally the right choice for a binding, because few
language runtimes promise the same layout as C99 `_Complex`. Pass such a buffer
as `(kfr_c32*)spectrum`.

All sizes in the API are counts of samples, except the `*_get_temp_size_*`
queries, which return **bytes**.

## Plans, transforms, and filter handles

Plan pointers such as `KFR_DFT_PLAN_F32*` and `KFR_FILTER_F32*` are opaque,
library-owned objects; their visible struct fields have no client meaning.
Obtain a handle from a `kfr_*_create_*` function, use it only with the matching
family and precision, and destroy it with the matching `kfr_*_delete_*`
function. Creation returns `NULL` on failure, and delete functions accept
`NULL`.

Filter creation copies its taps or SOS coefficients, so those source arrays may
be released afterwards. Input, output, and scratch buffers are caller-owned and
must stay valid for the duration of a call; KFR never retains or frees them.
[Memory management and ownership](capi/c_api_memory_management.md) covers the
lifetime, allocator, and thread-safety rules in full.

### Transforms

Forward and inverse transforms apply no scaling, so a complex round trip
produces $N$ times the original data. `*_get_temp_size_*` returns the byte
capacity that an execute call needs for its `temp` argument; passing `NULL`
instead lets KFR obtain that storage internally for the call, which a reusable
caller buffer avoids in realtime paths.

```c
#define KFR_NO_C_COMPLEX_TYPES 1
#include <kfr/capi.h>

|||TEST_CASE("advanced/capi.md/complex DFT round trip")
|||{
const size_t n = 256;
KFR_DFT_PLAN_F32* plan = kfr_dft_create_plan_f32(n);
|||REQUIRE(plan != NULL);
kfr_c32* time = kfr_allocate(n * KFR_COMPLEX_SIZE_MULTIPLIER * sizeof(kfr_f32));
kfr_c32* freq = kfr_allocate(n * KFR_COMPLEX_SIZE_MULTIPLIER * sizeof(kfr_f32));
|||REQUIRE(time != NULL);
|||REQUIRE(freq != NULL);
size_t temp_size = kfr_dft_get_temp_size_f32(plan);
uint8_t* temp = temp_size == 0 ? NULL : kfr_allocate(temp_size);
|||REQUIRE(temp_size == 0 || temp != NULL);

for (size_t i = 0; i < n * KFR_COMPLEX_SIZE_MULTIPLIER; ++i)
  time[i] = 0.0f;
time[0] = 1.0f; /* An impulse in the real component of the first sample. */

kfr_dft_execute_f32(plan, freq, time, temp);
kfr_dft_execute_inverse_f32(plan, time, freq, temp);
for (size_t i = 0; i < n * KFR_COMPLEX_SIZE_MULTIPLIER; ++i)
  time[i] /= (kfr_f32)n; /* Divide by n for a conventional round trip. */
|||CHECK(time[0] > 0.999f && time[0] < 1.001f);

kfr_deallocate(temp);
kfr_deallocate(freq);
kfr_deallocate(time);
kfr_dft_delete_plan_f32(plan);
|||}
```

The 2D, 3D, and N-dimensional creators take the individual dimensions and
operate on the product of them; arrange values in row-major order.
[[`::kfr_dft_real_create_plan_f32`:nosig]] needs an even $N$ and selects `Perm`
($N/2$ complex values) or `CCs` ($N/2 + 1$ complex values) output, as described
in [DFT data layout](../dft/dft_layout.md).
[[`::kfr_dct_create_plan_f32`:nosig]] also requires an even size.

### Filters

Three filter kinds share one handle type and one processing function:

* [[`::kfr_filter_create_fir_plan_f32`:nosig]] — direct FIR from taps in
  natural order $h[0], h[1], \ldots$. Simplest choice for short filters.
* [[`::kfr_filter_create_convolution_plan_f32`:nosig]] — same response computed
  with FFT overlap-add, preferable for a long fixed impulse response.
  `block_size` must be a power of two; zero selects 1024.
* [[`::kfr_filter_create_iir_plan_f32`:nosig]] — cascade of second-order
  sections, each six consecutive scalars `(a0, a1, a2, b0, b1, b2)` matching
  KFR's `biquad_section` layout.

```c
#include <kfr/capi.h>
#include <stdio.h>

int main(void)|||TEST_CASE("advanced/capi.md/FIR impulse response")
{
    const kfr_f32 taps[]  = { 0.25f, 0.5f, 0.25f };
    const kfr_f32 input[] = { 1, 0, 0, 0, 0, 0, 0, 0 };
    kfr_f32 output[sizeof(input) / sizeof(input[0])];

    KFR_FILTER_F32* filter =
        kfr_filter_create_fir_plan_f32(taps, sizeof(taps) / sizeof(taps[0]));
    if (filter == NULL)
    {
        fprintf(stderr, "KFR: %s\n", kfr_last_error());
|||REQUIRE(0);
      return 1;|||return;
    }

    kfr_filter_process_f32(filter, output, input, sizeof(input) / sizeof(input[0]));
|||CHECK(output[0] == 0.25f);
|||CHECK(output[1] == 0.5f);
|||CHECK(output[2] == 0.25f);
|||CHECK(output[3] == 0.0f);
    kfr_filter_delete_plan_f32(filter);
    return 0;|||
}
```

A filter handle owns its coefficients and its running state, so processing
consecutive blocks through one handle yields the same result as processing their
concatenation. [[`::kfr_filter_reset_f32`:nosig]] clears that state without
releasing the handle. `output == input` is supported for in-place processing,
but partial overlap is not.

Every function has an `_f64` counterpart. Benchmark a realistic workload before
fixing a crossover point between direct FIR and convolution.

## Next steps

* [Building a C API package](capi/building_kfr_c_api.md) — CMake options,
  install layout, ABI boundary, and package verification.
* [Loading and calling from C](capi/loading_kfr_from_c.md) — linking, version
  checks, dynamic loading, and FFI bindings.
* [Memory management and ownership](capi/c_api_memory_management.md) — buffers,
  handle lifetime, allocators, errors, and thread safety.
* [C API Reference](../_gen/capi/index.md) — every declaration, precision, and
  dimensionality.
