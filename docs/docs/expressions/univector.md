# `univector`: one-dimensional containers

[[`univector<T, Size>`:nosig]] is KFR's one-dimensional container and a 1D
input/output expression. It is suitable for sample buffers, coefficient lists,
and the result of a finite expression. Its storage form is selected by the
second template argument.

## Owning vectors

`univector<T>` is the usual dynamically sized, owning form. It uses KFR's
aligned allocator, so its storage is suitable for SIMD processing. Its size is
known at run time:

```c++
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("expressions/univector.md/dynamic-owning")
|||{
univector<float> samples(1024);
samples = truncate(counter<float>(), samples.size());
|||    CHECK(samples.size() == 1024);
|||    CHECK(samples[0] == 0.0f);
|||    CHECK(samples[1023] == 1023.0f);
|||}
```

`univector<T, N>` owns exactly `N` elements and stores them inline, like an
aligned `std::array`. Use it for small buffers and fixed algorithm state:

```c++
|||TEST_CASE("expressions/univector.md/fixed-owning")
|||{
univector<float, 4> coefficients{ 0.25f, 0.5f, 0.25f, 0.0f };
univector<float, 4> squares = sqr(counter<float>());
|||    CHECK_THAT(coefficients, DeepMatcher(univector<float, 4>{ 0.25f, 0.5f, 0.25f, 0.0f }));
|||    CHECK_THAT(squares, DeepMatcher(univector<float, 4>{ 0.0f, 1.0f, 4.0f, 9.0f }));
|||}
```

The dynamic form derives from `std::vector<T, data_allocator<T>>`, and the
fixed-size form derives from `std::array<T, N>`. All members provided by those
corresponding standard containers are therefore also available on `univector`.

Both forms accept 0D or 1D expressions. A fixed vector evaluates into its
existing `N` elements. Constructing a dynamic vector from an expression
requires a finite source and resizes it to the source extent:

```c++
|||TEST_CASE("expressions/univector.md/finite-expression-resize")
|||{
univector<int> values = truncate(counter(), 10);

univector<int> another;
another = truncate(counter(), 10); // resizes to 10
|||    CHECK_THAT(values, DeepMatcher(univector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));
|||    CHECK_THAT(another, DeepMatcher(values));
|||}
```

Assignment from an infinite 1D expression does not resize a dynamic vector; it
uses the vector's current length as the bound. This is useful when filling a
preallocated processing buffer:

```c++
|||TEST_CASE("expressions/univector.md/infinite-expression-assignment")
|||{
univector<float> block(256);
block = counter<float>();
|||    CHECK(block.size() == 256);
|||    CHECK(block[255] == 255.0f);
|||}
```

## Non-owning views

[[`univector_ref`:nosig]] is an alias for a non-owning `univector` view. It
contains a pointer and length, does not allocate or copy, and can use a
const-qualified element type for a read-only view. Create it with
[[`make_univector(T (&)[N])`:nosig]]:

```c++
|||TEST_CASE("expressions/univector.md/non-owning-views")
|||{
float raw[] = { 10, 20, 30 };
univector_ref<float> view = make_univector(raw);

view[1] = 99; // raw is now { 10, 99, 30 }

const std::vector<float> input = { 1, 2, 3 };
auto read_only = make_univector(input); // univector_ref<const float>
|||    CHECK(raw[1] == 99.0f);
|||    CHECK(read_only.size() == input.size());
|||    CHECK(read_only[2] == 3.0f);
|||}
```

[[`array_ref<T>`:nosig]] is the more general pointer-and-size range used by
KFR. It has ordinary range access (`data`, `size`, iterators, `operator[]`) but
is not itself an expression container. It is similar to C++20's `std::span`.
`univector_ref` derives from `array_ref` and adds the 1D expression interface.

Neither view owns its elements or extends their lifetime. Do not keep a view
of a temporary container, an initializer list, or storage that may be
reallocated.

## Slicing and references

[[`univector_base<T, Class, true>::slice(size_t, size_t)`:nosig:noscope]] and 
[[`univector_base<T, Class, true>::truncate(size_t)`:nosig:noscope]]
member functions on an expression-capable `univector`
return non-owning views of its storage. They clamp the requested range rather
than extending past the end:

```c++
|||TEST_CASE("expressions/univector.md/slicing-and-references")
|||{
univector<float> samples = { 0, 1, 2, 3, 4, 5 };

auto middle = samples.slice(2, 3); // aliases { 2, 3, 4 }
middle = -middle;                  // samples becomes { 0, 1, -2, -3, -4, 5 }

auto first = samples.truncate(2);  // aliases { 0, 1 }
auto all   = samples.ref();        // array_ref<float> over all elements
|||    CHECK_THAT(samples, DeepMatcher(univector<float>{ 0, 1, -2, -3, -4, 5 }));
|||    CHECK(first.size() == 2);
|||    CHECK(all.data() == samples.data());
|||}
```

A const vector returns `univector_ref<const T>` from `slice` and `truncate`.

There is also a free [[`slice(Arg &&, std::type_identity_t<shape<Dims>>, std::type_identity_t<shape<Dims>>)`:nosig]]
for lazy expression slicing. It works for expressions of every rank. The
member form above is the simpler choice when an already-materialized vector
needs a direct storage view.

## Ring-buffer helpers

An owning or mutable reference `univector` has small circular-buffer helpers.
They use a cursor supplied by the caller and wrap at the vector boundary:

* [[`univector_base<T, Class, true>::ringbuf_write(size_t &, const T &)`:nosig]] writes one value and advances `cursor`.
* [[`univector_base<T, Class, true>::ringbuf_write(size_t &, const T *, size_t)`:nosig]] copies a block, wrapping when
  necessary.
* [[`univector_base<T, Class, true>::ringbuf_read(size_t &, vec<T, N> &)`:nosig]] provides matching one-value, SIMD-vector, and block reads.
* [[`univector_base<T, Class, true>::ringbuf_step(size_t &, size_t)`:nosig]] advances a cursor without copying.

```c++
|||TEST_CASE("expressions/univector.md/ring-buffer-helpers")
|||{
univector<float, 4> delayline{};
size_t cursor = 3;

delayline.ringbuf_write(cursor, 1.0f); // stores at index 3; cursor becomes 0

delayline.ringbuf_write(cursor, 2.0f); // stores at index 0; cursor becomes 1

float value;
delayline.ringbuf_read(cursor, value); // reads index 1 and advances
|||    CHECK_THAT(delayline, DeepMatcher(univector<float, 4>{ 2.0f, 0.0f, 0.0f, 1.0f }));
|||    CHECK(value == 0.0f);
|||    CHECK(cursor == 2);
|||}
```

Keep the cursor in `[0, size())` and use a nonempty buffer. These helpers are
for local circular storage; they do not provide synchronization.

For a thread-shared single-producer/single-consumer queue, see
[[`spsc_ring_buffer<T>`:nosig]] in [Small containers and an SPSC queue](other_containers.md).

## Choosing a form

| Requirement | Recommended type |
| --- | --- |
| Size is chosen at run time and the vector owns data | `univector<T>` |
| Size is part of the algorithm's compile-time contract | `univector<T, N>` |
| Existing contiguous storage should be processed without a copy | `univector_ref<T>` / `make_univector` |
| A generic, non-expression pointer-and-size range is enough | `array_ref<T>` |

Use [[`render(Expr &&)`:nosig]] or direct expression construction when a
finite 1D expression should become an owning vector. Use a reference view when
an algorithm should operate on caller-owned data in place.
