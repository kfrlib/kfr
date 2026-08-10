# Small containers and a lock-free queue

These are focused storage utilities rather than general-purpose containers.
They provide contiguous `data()` and `size()` access, so
[[`make_univector(Container &)`:nosig]] can create a non-owning KFR view when
needed. The view remains valid only while the original storage remains stable.

## `inline_vector`

[[`inline_vector<T, N>`:nosig]] is a small fixed-capacity sequence stored
directly in the object. `N` is a positive compile-time capacity; it never
allocates or grows.

```c++
|||#include <kfr/base.hpp>
|||using namespace kfr;
|||TEST_CASE("expressions/other_containers/inline_vector")
|||{
inline_vector<float, 4> gains{ 0.25f, 0.5f };
gains.push_back(0.25f);

auto view = make_univector(gains);
|||CHECK(view.size() == 3);
|||CHECK(view[2] == 0.25f);
|||}
```

It has contiguous iterators, `data()`, `size()`, checked
[[`inline_vector<T, N>::at`:nosig]] access, unchecked `operator[]`, and
[[`inline_vector<T, N>::push_back`:nosig]]. Pushing beyond `N` triggers a
logic check. There is no `resize`, `clear`, `reserve`, insertion, or heap
allocation. Its elements are inline array subobjects, so use simple,
default-constructible, trivially copyable/movable, trivially destructible
types. Copies and moves copy that inline storage.

Use it for a small bounded temporary list or fixed algorithm configuration;
prefer [[`univector<T, Size>`:nosig]] or another heap-backed container when
the required maximum is not known.

## `small_buffer`

[[`small_buffer<T, Capacity>`:nosig]] is an owning runtime-sized buffer with a
small-size optimization. It stores up to `Capacity` elements inline, then uses
KFR's aligned heap allocator for larger sizes:

```c++
|||TEST_CASE("expressions/other_containers/small_buffer")
|||{
small_buffer<float, 16> scratch(8); // inline storage
scratch.resize(1024);               // aligned heap storage
|||CHECK(scratch.size() == 1024);
|||}
```

It supports `resize`, `clear`, `push_back`, `pop_back`, element access, and
pointer iteration, but has no separate capacity, `reserve`, or emplacement
interface. A heap resize allocates exactly the requested size; crossing the
inline threshold also changes storage. Treat pointers, references, iterators,
and KFR views as invalid after such a resize. `clear()` releases heap storage.

Heap storage is cache-line aligned, while inline storage has only `T`'s normal
alignment. This is low-level numeric scratch storage, not a `std::vector`
replacement: use trivial or otherwise raw-storage-safe types, initialize newly
grown elements before reading them, and observe the unchecked access
preconditions. Copies own separate storage; moves transfer heap storage when
present.

Use `small_buffer` for short-lived buffers that are usually small but may
occasionally grow. Use an aligned [[`univector<T, Size>`:nosig]] when the
storage should itself be a KFR expression container.

## `spsc_ring_buffer`

[[`spsc_ring_buffer<T>`:nosig]] is an SPSC queue. It owns only atomic cursors;
the caller owns the backing mutable [[`univector<T, Size>`:nosig]].
[[`lockfree_ring_buffer`:nosig]] remains a compatibility alias:

```c++
|||TEST_CASE("expressions/other_containers/spsc_ring_buffer")
|||{
univector<float> storage(1024);
spsc_ring_buffer<float> queue;
float input[3]{ 1.0f, 2.0f, 3.0f };
float output[3]{};
const size_t input_size = 3;
const size_t output_size = 3;

const size_t pushed = queue.try_enqueue(input, input_size, storage);
const size_t pulled = queue.try_dequeue(output, output_size, storage, true);
|||CHECK(pushed == input_size);
|||CHECK(pulled == output_size);
|||CHECK(output[0] == input[0]);
|||CHECK(output[1] == input[1]);
|||CHECK(output[2] == input[2]);
|||}
```

[[`spsc_ring_buffer<T>::try_enqueue`:nosig]] and
[[`spsc_ring_buffer<T>::try_dequeue`:nosig]] return the count transferred.
By default a request is all-or-nothing and returns zero if it cannot complete;
`partial = true` transfers the available prefix. The entire backing vector is
usable capacity and copies wrap as necessary.

Exactly one thread may enqueue and exactly one thread may dequeue. The queue
uses separated, cache-line-aligned producer/consumer cursors with acquire/release
operations to publish copies; it does not support multiple producers or
consumers. Its [[`spsc_ring_buffer<T>::size`:nosig]] result is approximate, not
an atomic snapshot, and is not a synchronization primitive. Whether
`std::atomic<uint64_t>` is lock-free depends on the target.

The backing vector must be nonempty, dedicated to this queue, and remain alive,
unmoved, and unchanged in size throughout concurrent use. Transfers use byte
copies, so use byte-copy-safe, trivially-copyable element types and avoid
overlapping the transfer buffers with the backing storage. No thread may
otherwise access the backing elements during concurrent use. Unlike the two
containers above, this type manages neither allocation nor element lifetime.

The queue's cursors are `uint64_t` and eventually wrap after $2^{64}$ transferred
elements. Recreate the queue before that point when the backing capacity does
not divide $2^{64}$, because continuing after the wrap uses incorrect physical
indices.

For an unsynchronized local circular buffer, use `univector`'s `ringbuf_*`
helpers instead.

See [`univector`: one-dimensional containers](univector.md) for KFR's primary
1D container and views.
