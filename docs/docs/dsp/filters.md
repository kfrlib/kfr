# Applying Stateful Filters with KFR

KFR's FIR, IIR, biquad, and convolution filters share a common model: a filter processes one sequential sample stream and may retain history between calls. This article describes the [[`filter<T>`:nosig]] runtime interface, lazy filter expressions, state ownership, reset behavior, initial history, and multichannel processing. Use the FIR, IIR, and biquad guides to choose and design a particular filter.

## The `filter<T>` interface

[[`filter<T>`:nosig]] is the abstract base class for a one-dimensional stream of samples of type `T`. Concrete filters implement buffer and expression processing internally; applications use its [[`filter<T>::apply`:nosig]] and [[`filter<T>::apply_zeros`:nosig]] overloads.

The interface supports fixed-size C arrays, `univector` objects, raw pointers, and one-dimensional expressions.

| Input and output | Example | Notes |
| --- | --- | --- |
| In-place C array | `filter.apply(samples);` | Processes all elements of a fixed-size array. |
| Separate C arrays | `filter.apply(output, input);` | Arrays must have the same compile-time size. |
| In-place `univector` | `filter.apply(samples);` | Processes the vector's current size. |
| Separate `univector` objects | `filter.apply(output, input);` | An empty output is resized to the input size; otherwise the shorter size is processed. |
| In-place raw buffer | `filter.apply(data, count);` | `count` is the number of samples. |
| Separate raw buffers | `filter.apply(output, input, count);` | Processes exactly `count` samples. |
| Expression into a buffer | `filter.apply(output, source);` | The source must be one-dimensional; the shorter source/destination length is processed. |
| Zero-valued input / tail | `filter.apply_zeros(output);` | Writes results for implicit zero input; the output must already have its final size. |

```c++
|||#include <kfr/dsp.hpp>
|||using namespace kfr;
|||TEST_CASE("filters.md/apply overloads")
|||{
univector<float> taps{ 0.25f, 0.5f, 0.25f };
filter_fir<float> filter{ taps };

float audio_block[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
filter.apply(audio_block);              // in place

univector<float> input_block{ 1, 2, 3, 4 };
univector<float> output_block;
filter.apply(output_block, input_block); // separate univectors

univector<float> input{ 1, 2, 3, 4 };
univector<float> output(input.size());
filter.apply(output.data(), input.data(), input.size());

size_t tail_samples = 4;
univector<float> tail(tail_samples);
filter.apply_zeros(tail);
|||}
```

For expression input, size the destination before calling `apply`; expression overloads do not resize an empty destination. Raw pointers must point to valid storage of the requested length. The interface supports in-place processing, but do not assume arbitrary partially overlapping source and destination ranges are safe.

## Streaming state and evaluation order

A filter's delay line, registers, overlap, or other history advances every time samples are processed. Keep the same runtime filter object for consecutive blocks of one signal:

```c++
|||TEST_CASE("filters.md/streaming state")
|||{
|||filter_fir<float> filter{ univector<float>{ 0.25f, 0.5f, 0.25f } };
|||univector<float> block1{ 1, 2, 3, 4 };
|||univector<float> block2{ 5, 6, 7, 8 };
filter.apply(block1);
filter.apply(block2); // uses the history accumulated from block1
|||}
```

To process zero-valued input after the final block without allocating a zero source buffer, pre-size an output buffer and call [[`filter<T>::apply_zeros`:nosig]]. It advances the same state as `apply` and does not reset the filter. FIR and convolution filters have a finite tail; IIR filters usually decay asymptotically, so choose a practical output duration or amplitude threshold.

The same rule applies to stateful lazy expressions. Evaluate them once, in increasing signal order. Do not use one mutable state object simultaneously for different signals or from independent processing threads.

A newly constructed filter or expression normally starts with zero history. This produces the expected startup transient for a causal filter. Call [[`filter<T>::reset`:nosig]] before processing an unrelated signal only when the concrete filter documents a reset override; the base implementation is a no-op.

| Filter type | `reset()` behavior |
| --- | --- |
| [[`fir_filter<T, U>`:nosig]] | Clears the FIR delay line and rewinds its ring-buffer cursor. |
| [[`iir_filter<T>`:nosig]] | Clears the wrapped IIR delay state while preserving its coefficients. |
| [[`expression_filter<T>`:nosig]] | Propagates reset to its embedded expression state. |
| [[`convolve_filter<T>`:nosig]] | Clears its overlap-add history and pending input. |

## Stateful expressions and `state_holder`

FIR and fixed-size IIR expressions use [[`state_holder<T, Stateless>`:nosig]] internally. When `Stateless` is `false`, the expression owns a copy of its state. When it is `true`, the expression stores a non-owning pointer to caller-owned state supplied through `std::ref`.

Passing filter parameters by value therefore creates a finite, zero-history operation:

```c++
|||TEST_CASE("filters.md/stateless fir expression")
|||{
|||univector<float> taps{ 0.25f, 0.5f, 0.25f };
|||univector<float> input{ 1, 2, 3, 4 };
univector<float> output = fir(input, fir_params{ taps });
|||CHECK(output.size() == input.size());
|||}
```

For block streaming, hold the state externally and pass a reference wrapper. The state must outlive expression evaluation:

```c++
|||TEST_CASE("filters.md/stateful fir expression")
|||{
|||univector<float> taps{ 0.25f, 0.5f, 0.25f };
|||univector<float> input_block1{ 1, 2, 3, 4 };
|||univector<float> input_block2{ 5, 6, 7, 8 };
fir_state<float> state{ taps };

univector<float> output1 = fir(input_block1, std::ref(state));
univector<float> output2 = fir(input_block2, std::ref(state));
|||}
```

Copying an expression with owned state copies the state. Copying an expression built with `std::ref(state)` copies the reference, so both expressions mutate the same underlying state. The term `Stateless` in the implementation means *non-owning*; it does not mean that filtering has no state.

## Reusable expression filters

[[`expression_filter<T>`:nosig]] adapts an expression containing a [[`placeholder(csize_t<Key>)`:nosig]] into a runtime [[`filter<T>`:nosig]]. [[`to_filter(E &&)`:nosig]] stores the expression graph, substitutes the current input for its placeholder on each `apply` call, and retains any mutable state embedded in that graph.

```c++
|||TEST_CASE("filters.md/reusable expression filter")
|||{
|||univector<float> block1{ 1, 2, 3, 4 };
|||univector<float> block2{ 5, 6, 7, 8 };
auto section = biquad_lowpass<float>(1000.0f / 48000.0f, 0.707f);
auto filter = to_filter(iir(placeholder<float>(), iir_params{ section }));

filter.apply(block1);
filter.apply(block2); // the IIR state embedded in the expression continues
|||}
```

Substitution changes the input binding, not the filter history. A reusable expression filter must use a compatible one-dimensional placeholder; call [[`filter<T>::reset`:nosig]] to reset any embedded state that supports reset.

## Supplying initial history

Most applications should begin with zero history or continue from a preceding block. If the first block must begin from known previous samples, seed a supported state object before processing.

[[`fir_state<T, U>::push_delayline`:nosig]] appends samples to an FIR ring buffer. Supply past samples in chronological order, oldest first and the sample immediately before the next block last:

```c++
|||TEST_CASE("filters.md/push_delayline before fir expression")
|||{
|||univector<float> taps{ 0.25f, 0.5f, 0.25f };
|||float x_minus_3 = 1.0f, x_minus_2 = 2.0f, x_minus_1 = 3.0f;
|||univector<float> next_block{ 4, 5, 6, 7 };
fir_state<float> state{ taps };
state.push_delayline(univector<float>{ x_minus_3, x_minus_2, x_minus_1 });

univector<float> output = fir(next_block, std::ref(state));
|||CHECK(output.size() == next_block.size());
|||}
```

A runtime FIR filter can receive this prepared state:

```c++
|||TEST_CASE("filters.md/push_delayline before runtime filter")
|||{
|||univector<float> taps{ 0.25f, 0.5f, 0.25f };
|||univector<float> previous_samples{ 1, 2, 3 };
fir_state<float> state{ taps };
state.push_delayline(previous_samples);
filter_fir<float> filter{ std::move(state) };
|||}
```

IIR states expose internal delay registers, but KFR does not provide a public “previous input/output samples” or conventional initial-condition API. Manually setting an [[`iir_state<T, filters>`:nosig]] is advanced: the values are transposed-direct-form registers and, for cascades, include inter-section and look-ahead bookkeeping. Prefer a zero-state start or preserve the existing state from the preceding block.

## Multichannel signals

[[`filter<T>`:nosig]] processes one time stream. It has no multichannel, frame-stride, or `univector2d` overload. An interleaved stereo buffer passed to one scalar filter is interpreted as

```text
L0, R0, L1, R1, ...
```

which mixes channel histories and is incorrect for independent channel filtering. Use one independent filter or state object per channel. Process planar channel buffers, then interleave them again if required:

```c++
|||TEST_CASE("filters.md/multichannel filtering")
|||{
|||univector<float> taps{ 0.25f, 0.5f, 0.25f };
|||size_t frames = 4;
|||univector<float> interleaved{ 1, 10, 2, 20, 3, 30, 4, 40 };
|||univector<float> left(frames), right(frames);
float* channels[] = { left.data(), right.data() };
deinterleave(channels, interleaved.data(), 2, frames);

filter_fir<float> left_filter{ taps };
filter_fir<float> right_filter{ taps };
left_filter.apply(left);
right_filter.apply(right);

const float* processed[] = { left.data(), right.data() };
interleave(interleaved.data(), processed, 2, frames);
|||}
```

Retain one filter instance per channel across streaming blocks. This preserves each channel's history without coupling it to the other channels. See [Audio utilities](misc.md) for planar/interleaved conversion details.

## See also

- [FIR filters](fir.md)
- [IIR filters](iir.md)
- [Biquad filters](bq.md)
- [Convolution filter details](../dft/convolution.md)
