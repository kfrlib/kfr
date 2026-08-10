# Memory, Alignment, and Ownership

KFR's vector and container APIs are designed to work efficiently with SIMD
loads and stores. Alignment is one part of that: an address aligned for the
selected access width can let the compiler and hardware use a simpler or faster
load/store path. KFR containers arrange suitable allocation automatically, but
low-level code sometimes needs an aligned scratch buffer, an allocator for an
STL container, or a class-specific allocation policy.

The allocation tools described here are in `<kfr/meta/memory.hpp>`. Prefer
[[`univector`:nosig]], tensors, and KFR read/write APIs for normal signal
storage; use the low-level facilities when an interface specifically needs raw
storage or a temporary buffer.

## What Alignment Means

A pointer is aligned to $A$ bytes when its address is evenly divisible by $A$.
KFR's [[`default_memory_alignment`:nosig]] is
[[`KFR_CACHE_LINE_SIZE`]]—normally 64 bytes, with platform-specific exceptions.
This is large enough for common SIMD register widths and also aligns the start
of an allocation to a cache-line boundary.

Alignment applies to the start address, not every possible view into a buffer.
For example, a 64-byte-aligned `float*` becomes offset by four bytes after
`data + 1`, so that derived pointer is no longer 64-byte aligned. The data is
still valid and KFR operations remain correct; an operation may simply need an
unaligned access path.

> [!note]
> Alignment is a performance and interface contract, not permission to read
> beyond the actual object range. A correctly aligned pointer still requires
> enough contiguous, valid elements for the access being performed.

## Allocating Aligned Storage

[[`aligned_allocate`:nosig]] allocates raw storage aligned to either
[[`default_memory_alignment`:nosig]] or an explicit alignment. Pass the element
count, rather than the byte count, when a type parameter is supplied. Release
successful allocations only with [[`aligned_deallocate`:nosig]].

```c++
#include <kfr/meta/memory.hpp>

using namespace kfr;

|||#include <kfr/base.hpp>
|||TEST_CASE("memory.md/aligned_allocate")
|||{
float* samples = aligned_allocate<float>(1024);
if (samples == nullptr)
{
    // Handle allocation failure.
}
else
{
    for (size_t i = 0; i < 1024; ++i)
        samples[i] = 0.0f;

    aligned_deallocate(samples);
}
|||}
```

Pass an explicit second argument, e.g. `aligned_allocate<float>(1024, 32)`,
when an external API requires a particular supported alignment. The requested
value must be nonzero, a power of two, and no greater than
32768 bytes. KFR rejects zero and excessively large values; callers should
supply a valid power of two. KFR allocates at least one alignment-sized block,
so very small allocations can reserve more storage than their element count
suggests.

[[`aligned_allocate`:nosig]] hands back raw storage — it won't run constructors
for non-trivial objects, so stick to sample values, scratch buffers, or memory
whose lifetime you start yourself. Never mix allocation families: passing a
KFR allocation to `delete[]`, `std::free`, or a platform aligned-free function
corrupts the heap. Only call [[`aligned_deallocate`:nosig]] on a successful,
non-null allocation.

## RAII with `autofree`

[[`autofree<T>`:nosig]] owns a KFR-aligned allocation and releases it when the
object leaves scope. It is move-only, exposes indexed access, and returns its
underlying pointer from [[`autofree<T>::data()`:nosig]]. It does not remember an
element count, so retain that information separately when needed.

```c++
#include <kfr/meta/memory.hpp>

using namespace kfr;

void clear_block(size_t count)
{
    autofree<float> block(count);
    float* data = block.data();

    if (data == nullptr)
        return;

    for (size_t i = 0; i < count; ++i)
        data[i] = 0.0f;
} // block releases the allocation here
```

This is a convenient owner for a local buffer that must cross several return
paths. For a normal dynamically sized signal container, [[`univector`:nosig]]
is usually clearer because it records the size and supports expressions.

## STL Allocation and `aligned_new`

[[`data_allocator<T>`:nosig]] is KFR's STL-compatible aligned allocator. It is
used by KFR's dynamic [[`univector`:nosig]] storage and can also be chosen for a
standard container:

```c++
#include <kfr/meta/memory.hpp>
#include <vector>

using namespace kfr;

|||TEST_CASE("memory.md/data_allocator")
|||{
std::vector<float, data_allocator<float>> samples(4096);
|||CHECK(samples.size() == 4096);
|||}
```

Define `KFR_USE_STD_ALLOCATION` when ordinary `std::allocator<T>` behavior is
required instead. In that configuration [[`data_allocator<T>`:nosig]] becomes
an alias for `std::allocator<T>`, which can help when an API requires the exact
allocator type of a conventional `std::vector`.

Derive a class from [[`aligned_new`:nosig]] when individual instances need
KFR-aligned scalar `new` and `delete`:

```c++
#include <kfr/meta/memory.hpp>

using namespace kfr;

struct processor : aligned_new
{
    float coefficients[16]{};
};

|||TEST_CASE("memory.md/aligned_new")
|||{
processor* state = new processor;
// ...
delete state;
|||}
```

[[`aligned_new`:nosig]] provides allocation operators for single objects. It
should not be treated as a general replacement for array allocation or for
containers that already manage their own storage.

## Temporary Scratch Buffers

[[`call_with_temp`:nosig]] supplies a temporary aligned buffer to a callback.
It chooses stack storage when the requested element count is at most its
compile-time `stack_size` threshold, otherwise it uses an [[`autofree<T>`:nosig]]
heap allocation. The default threshold is 4096 `u8` elements.

```c++
#include <kfr/meta/memory.hpp>

using namespace kfr;

void normalize_with_scratch(const float* input, size_t count)
{
    call_with_temp<1024, float>(count, [=](float* scratch)
    {
        if (scratch == nullptr)
            return;

        for (size_t i = 0; i < count; ++i)
            scratch[i] = input[i];

        // Process scratch before the callback returns.
    });
}
```

The callback owns neither the stack buffer nor the heap buffer and must not
save the pointer. The stack path reserves an array sized by `stack_size`, not
by the smaller runtime request, so choose a conservative threshold for deeply
nested or thread-stack-sensitive code. Heap allocation can fail; code that
needs a guaranteed buffer must provide its own failure policy.

## Sharing Intrusive Objects

[[`KFR_CLASS_REFCOUNT`]] inserts atomic `addref()` and `release()` members into
a class; `release()` deletes the object once the count reaches zero. Reach for
`std::shared_ptr` in ordinary application code — this macro is for classes
already built around an intrusive refcount protocol, such as interop with a
COM-style or plugin boundary.

```c++
struct shared_state
{
    KFR_CLASS_REFCOUNT(shared_state)

    // State fields...
};

|||TEST_CASE("memory.md/refcount")
|||{
shared_state* state = new shared_state;
state->addref();
// Transfer or use the owned reference.
state->release();
|||}
```

The injected count begins at zero. The creator must establish an ownership
reference with `addref()` before the matching `release()`; calling `release()`
on a newly created object without doing so underflows the count instead of
safely deleting it. Keep the ownership convention local and explicit.

## Practical Guidance for SIMD Buffers

1. **Prefer KFR-managed containers**, such as [[`univector`:nosig]], over manual
   allocation and size tracking.
2. **Don't assume alignment survives a subview.** A slice, offset pointer, or
   external buffer can be unaligned even when the original allocation was not.
3. **Reuse buffers in real-time code.** Allocate scratch storage ahead of time
   rather than inside an audio callback or other latency-sensitive loop.
4. **State who releases a buffer, and with which function** — especially across
   DLL, language-runtime, or C API boundaries.

## See Also

- [Basics](../../getting-started/basics.md) for aligned `univector` storage
- [SIMD vectors](../simd/vec.md)
- [Expressions](../../expressions/expressions.md)
