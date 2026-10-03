# Expressions and containers

KFR uses **expressions** for both data and computation. An expression behaves
like a virtual multidimensional array: it can supply values, receive values, or
do both. This lets the same APIs work with stored sample buffers, matrices, and
signal generators.

[[`univector<T, Size>`:nosig]] and [[`tensor<T, NDims>`:nosig]] are containers:
they own element storage and are expressions. In contrast, generators,
adaptors, and operations normally describe a calculation without allocating an
element buffer.

## Build a pipeline, then evaluate it

Combining expressions usually creates a lazy pipeline, not an intermediate
container:

```c++
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("expressions.md/build a pipeline, then evaluate it")
|||{
univector<float, 5> input{ 1, 2, 3, 4, 5 };
auto transformed = sqrt(input * 0.5f + 1.0f);

univector<float, 5> output = transformed; // evaluates the pipeline
|||univector<float, 5> expected{ 1, 2, 3, 4, 5 };
|||expected = sqrt(expected * 0.5f + 1.0f);
|||CHECK_THAT(output, DeepMatcher(expected));
|||}
```

KFR can therefore combine the multiply, addition, and square root into
vectorized processing. Evaluation happens when a result is assigned to storage
or passed to an operation such as [[`process`:nosig]], [[`render`:nosig]], or
[[`trender`:nosig]]. Bound an infinite source before materializing it into a
finite container.

The next article, [Expression fundamentals](fundamentals.md), explains the
properties that make this composition work: ranks and shapes, broadcasting,
operand lifetimes, and expression evaluation.

## Topics

Choose the page that matches the kind of expression or container you are
working with:

- [Expression fundamentals](fundamentals.md) — shapes, broadcasting, lazy
  composition, evaluation, lifetime rules, and implementing custom
  expressions.
- [Writing custom expressions](custom.md) — the expression traits and SIMD
  block-access protocol, with read-only and writable examples.
- [Sources and adaptors](sources.md) — constants, counters, lambda sources,
  slicing, reshaping, concatenation, and stateful adaptors.
- [Statistics and histograms](statistics.md) — reductions such as `sum`,
  `rms`, and `dotproduct`, plus one-pass histogram collection.
- [Random expressions](random.md) — seeded random state, direct random draws,
  and lazy random generators.
- [`univector`: one-dimensional containers](univector.md) — owning vectors,
  non-owning views, slices, and local ring-buffer helpers.
- [Tensors and multidimensional expressions](tensor.md) — multidimensional
  storage, strided views, tensor ranges, transpose, and tensor evaluation.
- [Small containers and a lock-free queue](other_containers.md) —
  `inline_vector`, `small_buffer`, and the single-producer/single-consumer
  `spsc_ring_buffer`.
- [Expression handles](handle.md) — type erasure for storing an expression,
  crossing an ABI boundary, or selecting an expression at run time.

## Choosing storage or a handle

Keep an expression lazy while it remains part of a calculation. Materialize it
when its values need storage, must outlive referenced inputs, or are required by
an API that consumes ordinary data. Use a [[`univector<T, Size>`:nosig]] for a
one-dimensional result and a [[`tensor<T, NDims>`:nosig]] for a
multidimensional result. [Expression fundamentals](fundamentals.md#materializing-or-discarding-results)
details `render`, `trender`, `process`, and `sink`.

If a program must store or hide the *expression type* rather than its values,
an [expression handle](handle.md) preserves the expression interface while
erasing the concrete type.

Internally, KFR uses [expression templates](../advanced/dsp_glossary.md#expression-templates)
and explicitly [vectorized](../advanced/dsp_glossary.md#simd-single-instruction-multiple-data)
processing to implement this model.
