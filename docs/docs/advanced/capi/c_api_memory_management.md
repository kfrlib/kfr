# C API memory management and ownership

The KFR C API separates caller buffers from library objects. Respecting that
boundary is essential for C bindings and for runtimes with their own allocators,
such as Rust, Python, .NET, Java, or JavaScript.

For transform and filter behavior, see the [C API overview](../capi.md); for
package-level ABI choices, see
[Building a KFR C API package](building_kfr_c_api.md).

## Ownership at a glance

| Resource | Owner | How it is released |
| --- | --- | --- |
| Input, output, and caller scratch buffers | Caller | Caller allocator or [[`::kfr_deallocate`:nosig]] if KFR allocated it |
| DFT, real-DFT, DCT, and filter plan handles | KFR library | The matching `kfr_*_delete_*` function |
| Filter taps and IIR SOS arrays passed at creation | Caller during creation only | Caller may release them after a successful creation call |
| String from [[`::kfr_last_error`:nosig]] | KFR, thread-local | Never release; copy before the next KFR call on that thread if it must persist |
| Version and architecture strings | KFR, static | Never release; copy only if application ownership is needed |

KFR does not retain caller input, output, or temporary buffers after an execute
or process function returns. They can live on the stack, in an audio pool, in an
FFI runtime's array, or in KFR-allocated storage. They must remain valid and
correctly sized for the whole call.

## Caller buffers and alignment

[[`::kfr_allocate`:nosig]] returns storage aligned to
[[`KFR_DEFAULT_ALIGNMENT`:nosig]] (64 bytes). Use
[[`::kfr_allocate_aligned`:nosig]] for a stricter power-of-two alignment, and
release either result only through [[`::kfr_deallocate`:nosig]], which accepts
`NULL`. Allocation failure is reported as `NULL`, not through
`kfr_last_error()`, so check the returned pointer directly.

That 64-byte alignment is convenient for reusable SIMD-oriented storage, not a
requirement: an ordinary correctly aligned `float[]` or `double[]` is accepted.
Use KFR allocation when the host language cannot conveniently provide the
alignment, or for a long-lived scratch buffer.

A `temp` buffer supplied to a DFT or DCT execute call must be at least the byte
count reported by the plan's `*_get_temp_size_*` query:

```c
#define KFR_NO_C_COMPLEX_TYPES 1
#include <kfr/capi.h>
#include <stdio.h>
|||#include <kfr/test/mini_catch.h>

int main(void)|||TEST_CASE("advanced/capi/c_api_memory_management.md/reusable DFT scratch buffer")
{
  enum { N = 8 };
  kfr_f32 input[N * KFR_COMPLEX_SIZE_MULTIPLIER] = { 1.0f };
  kfr_f32 output[N * KFR_COMPLEX_SIZE_MULTIPLIER];
  KFR_DFT_PLAN_F32* plan = kfr_dft_create_plan_f32(N);
  if (plan == NULL)
  {
    fprintf(stderr, "KFR: %s\n", kfr_last_error());
    return 1;|||REQUIRE(plan != NULL); return;
  }

  size_t bytes = kfr_dft_get_temp_size_f32(plan);
  uint8_t* temp = bytes == 0 ? NULL : kfr_allocate(bytes);
  if (bytes != 0 && temp == NULL)
  {
    kfr_dft_delete_plan_f32(plan);
    return 1;|||REQUIRE(temp != NULL); return;
  }
  kfr_dft_execute_f32(plan, output, input, temp);
|||CHECK(output[0] == 1.0f);
|||CHECK(output[2] == 1.0f);

  kfr_deallocate(temp);
  kfr_dft_delete_plan_f32(plan);
  return 0;|||
}
```

Passing `NULL` for `temp` asks KFR to obtain that storage itself, which is
convenient but allocates on every call. Do not share one scratch buffer between
simultaneous executions.

When allocating interleaved complex storage, remember that a complex count is
not a scalar count — multiply by [[`KFR_COMPLEX_SIZE_MULTIPLIER`:nosig]], as
described in [Data types and buffer conventions](../capi.md#data-types-and-buffer-conventions).

## Opaque handles and lifetime

`KFR_DFT_PLAN_F32`, `KFR_DFT_REAL_PLAN_F32`, `KFR_DCT_PLAN_F32`, and
`KFR_FILTER_F32`, with their `F64` forms, are deliberately opaque. The visible
struct definition does **not** describe the implementation object: do not
inspect it, copy it by value, allocate it on the stack, or free it with a host
allocator.

The lifecycle is:

1. Create the handle with the matching `kfr_*_create_*` function.
2. On `NULL`, read the diagnostic from [[`::kfr_last_error`:nosig]] and do not
   use the handle.
3. Use it only with functions of the same family and precision.
4. Stop all concurrent use.
5. Call its matching `kfr_*_delete_*` function exactly once; delete functions
   accept `NULL`.

A deleted handle is invalid. Do not cache one in another runtime past its
finalizer, and do not unload `kfr_capi` while a handle or a deferred finalizer
can still reach it.

Filter handles additionally own a delay line and overlap state.
[[`::kfr_filter_reset_f32`:nosig]] clears that state but does not release the
handle.

## Avoid allocator mismatches

Never cross allocator families. In particular:

* Do not pass `malloc`, `calloc`, `realloc`, `new`, Rust `alloc`, Python
  memory, .NET marshaling memory, or JavaScript-runtime memory to
  `kfr_deallocate`.
* Do not release a pointer from [[`::kfr_allocate`:nosig]] with `free`,
  `delete`, or any foreign-runtime release function.
* Do not free a KFR plan with anything other than its matching delete function.
* Do not pass a pointer returned by KFR into a host runtime that assumes it owns
  or may resize the storage.

Keep ownership clear in a binding: either allocate the numeric arrays in the
host runtime and retain them during each call, or allocate them through KFR and
wrap them in a finalizer that calls `kfr_deallocate`. Do not expose a KFR
allocation as a general host-owned buffer unless the binding can guarantee the
correct finalizer.

## Managed allocation mode

`KFR_MANAGED_ALLOCATION=ON` changes the library allocator. Based on the
generated `config.h` it includes, `<kfr/capi.h>` additionally declares
`kfr_reallocate`, `kfr_reallocate_aligned`, `kfr_add_ref`, `kfr_release`, and
`kfr_allocated_size`. These functions work only with blocks created by the
matching KFR managed allocation mode:

* `kfr_reallocate*` may move a block; always replace the old pointer with the
  returned pointer after checking for failure.
* `kfr_add_ref` increments a KFR-managed block's reference count.
* `kfr_release` drops that reference count and frees the block at zero.
* `kfr_allocated_size` reports the managed block's usable byte size.

Normal release packages have this option `OFF`. It is an ABI choice, not a
consumer-side feature macro: configure KFR with it, install the generated header
and library together, and rebuild bindings compiled against the header. Never
call a managed allocation function on a normal allocation or on memory owned by
another runtime. See
[`KFR_MANAGED_ALLOCATION`](../configuration.md#kfr_managed_allocation) for the
underlying allocator change.

## Error and string ownership

Instead of status codes, wrapped C entry points store a caught exception message
in a thread-local buffer that [[`::kfr_last_error`:nosig]] returns. A successful
wrapped call clears the buffer, so an empty string means no pending error.

```c
#include <kfr/capi.h>
#include <stdio.h>

int main(void)|||TEST_CASE("advanced/capi/c_api_memory_management.md/plan creation error")
{
  /* Real DFT plans accept odd and even sizes. */
  KFR_DFT_REAL_PLAN_F32* plan = kfr_dft_real_create_plan_f32(7, Perm);
  if (plan == NULL)
  {
    const char* message = kfr_last_error();
    fprintf(stderr, "KFR: %s\n", message[0] != '\0' ? message : "plan creation failed");
    return 1;|||REQUIRE(message[0] != '\0'); return;
  }

|||CHECK(kfr_dft_real_get_size_f32(plan) == 7);
  kfr_dft_real_delete_plan_f32(plan);
  return 0;|||
}
```

The returned `const char*` belongs to KFR and stays valid only until the next
KFR call on that thread. Read or copy it immediately; do not free it, keep it
for deferred use without copying, or move it to another thread.
[[`::kfr_version_string`:nosig]] and [[`::kfr_enabled_archs`:nosig]] return
library-owned static strings, which must also never be released.

## Thread-safety boundary

Handles can be created and used on different threads and the error buffer is
thread-local, but a single mutable handle is not concurrently safe:

* A filter handle carries mutable delay, overlap, and IIR state. Never process
  or reset one from two threads at once.
* Treat every operation on a plan handle — execution, queries, reset, deletion
  — as externally serialized. Create one handle per worker for parallel work.
* A caller buffer, including a `temp` buffer, must have a single active writer.
* Before deleting a handle or unloading the library, join or otherwise exclude
  every thread and finalizer that could still call into it.
