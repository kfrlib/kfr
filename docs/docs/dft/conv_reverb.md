# Convolution reverb

Convolution reverb applies a recorded or synthesized impulse response (IR) to
audio. Each output sample is the [convolution](../advanced/dsp_glossary.md#convolution)
of the input and the IR, so an impulse response can describe a room, a device,
or any other linear time-invariant effect.

For long IRs, use [[`convolve_filter<T>`:nosig]]. It is a streaming
[[`filter<T>`:nosig]] that performs partitioned, FFT-based overlap-add
convolution. The IR is transformed when the filter is initialized; audio can
then be supplied as one buffer or as successive blocks without retaining the
complete input signal in memory.

For the shared [[`filter<T>`:nosig]] `apply` interface, reset behavior, and
general rules for preserving state across blocks, see [Applying Stateful
Filters](../dsp/filters.md).

## Mono reverb

The simplest use is in-place processing. The first output sample corresponds
to the first input sample, so the filter does not add a signal-alignment
[latency](../advanced/dsp_glossary.md#latency).

```c++
|||#include <kfr/base.hpp>
|||#include <kfr/dft.hpp>
|||#include <vector>
|||using namespace kfr;
|||TEST_CASE("dft/conv_reverb.md/mono in-place")
|||{
univector<float> audio{ 1.0f, 2.0f, 3.0f };
univector<float> impulse_response{ 0.5f, 1.0f };

convolve_filter<float> reverb(impulse_response);
reverb.apply(audio);
|||CHECK(rms(audio - univector<float>{ 0.5f, 2.0f, 3.5f }) < 1e-5f);
|||}
```

The in-place overload overwrites `audio`. To retain the dry signal, use a
separate output vector instead. An empty output vector is resized to the input
length by [[`filter<T>::apply(univector<T, Tag1> &, const univector<T, Tag2> &)`:nosig]].

```c++
|||TEST_CASE("dft/conv_reverb.md/separate output")
|||{
|||univector<float> audio{ 1.0f, 2.0f, 3.0f };
|||univector<float> impulse_response{ 0.5f, 1.0f };
|||convolve_filter<float> reverb(impulse_response);
univector<float> wet;
reverb.apply(wet, audio);
|||CHECK(rms(wet - univector<float>{ 0.5f, 2.0f, 3.5f }) < 1e-5f);
|||CHECK_THAT(audio, DeepMatcher(univector<float>{ 1.0f, 2.0f, 3.0f }));
|||}
```

[[`filter<T>::apply(univector<T, Tag1> &, const univector<T, Tag2> &)`:nosig]]
produces one output sample for every input sample. It does not append the part
of the response that continues after the input ends. To collect that tail, use
[[`filter<T>::apply_zeros`:nosig]] to process $M - 1$ zero-valued input samples
after an input whose IR has $M$ samples:

```c++
|||TEST_CASE("dft/conv_reverb.md/collect tail")
|||{
|||univector<float> audio{ 1.0f, 2.0f, 3.0f };
|||univector<float> impulse_response{ 0.5f, 1.0f };
convolve_filter<float> reverb(impulse_response);

univector<float> wet(audio.size() + impulse_response.size() - 1);
reverb.apply(wet.data(), audio.data(), audio.size());
reverb.apply_zeros(wet.data() + audio.size(), impulse_response.size() - 1);
|||CHECK(rms(wet - univector<float>{ 0.5f, 2.0f, 3.5f, 3.0f }) < 1e-5f);
|||}
```

`wet` now contains the full linear-convolution result. Do not call
[[`convolve_filter<T>::reset`:nosig]] before processing the
zero-valued input:
resetting clears the pending overlap and discards the tail.

## Streaming and block size

[[`convolve_filter<T>`:nosig]] retains its input history and overlap between
calls to [[`filter<T>::apply(univector<T, Tag1> &, const univector<T, Tag2> &)`:nosig]].
Keep the same filter object for consecutive blocks from one audio stream:

```c++
|||TEST_CASE("dft/conv_reverb.md/streaming blocks")
|||{
univector<float> impulse_response{ 0.5f, 1.0f };
std::vector<univector<float>> input_blocks{
    { 1.0f, 2.0f },
    { 3.0f, 4.0f },
};
std::vector<univector<float>> output_blocks;

convolve_filter<float> reverb(impulse_response, 512);

for (const univector<float>& input_block : input_blocks)
{
    univector<float> output_block;
    reverb.apply(output_block, input_block);
    output_blocks.push_back(output_block);
}

univector<float> tail(impulse_response.size() - 1);
reverb.apply_zeros(tail);
output_blocks.push_back(tail);
|||CHECK(reverb.input_block_size() == 512u);
|||REQUIRE(output_blocks.size() == 3u);
|||CHECK(rms(output_blocks[0] - univector<float>{ 0.5f, 2.0f }) < 1e-5f);
|||CHECK(rms(output_blocks[1] - univector<float>{ 3.5f, 5.0f }) < 1e-5f);
|||CHECK_THAT(output_blocks[2], DeepMatcher(univector<float>{ 4.0f }));
|||}
```

The optional block-size argument controls the FFT partition size. KFR rounds
it up to a power of two; query the actual value with
[[`convolve_filter<T>::input_block_size`:nosig]]. Processing blocks whose
length is a multiple of that value avoids repeated transforms of a partial
block and gives the best throughput. Other block lengths are valid and retain
the same sample order and filter state.

The final `apply_zeros` call drains the overlap remaining after the last input
block. Do this before resetting the filter or beginning an unrelated stream.

Call [[`convolve_filter<T>::reset`:nosig]] before starting an unrelated
stream. This clears input history and overlap but keeps the IR and the chosen
block size. To replace the IR, call
[[`convolve_filter<T>::set_data`:nosig]]; replacing it also resets the filter
state.

> [!note]
> Construct the filter from a non-empty IR, or call `set_data` before its
> first `apply` call. The overload that accepts only an IR length is useful
> when it is immediately followed by `set_data`.

## True stereo reverb

True stereo uses four impulse responses: one for every input-to-output route.
The subscripts below name **input first, output second**:

$$
\begin{aligned}
    out_L &= in_L * h_{LL} + in_R * h_{RL}\\
    out_R &= in_L * h_{LR} + in_R * h_{RR}
\end{aligned}
$$

Each route needs a separate filter because it keeps its own input history and
overlap. The following code processes one stereo block. `left` and `right`
must have equal lengths; repeat the same four calls for every later block.

```c++
|||TEST_CASE("dft/conv_reverb.md/true stereo")
|||{
// h_<input><output>
univector<float> ir_ll{ 1.0f };  // left  -> left
univector<float> ir_rl{ 0.5f };  // right -> left
univector<float> ir_lr{ 2.0f };  // left  -> right
univector<float> ir_rr{ -1.0f }; // right -> right

convolve_filter<float> ll(ir_ll);
convolve_filter<float> rl(ir_rl);
convolve_filter<float> lr(ir_lr);
convolve_filter<float> rr(ir_rr);

univector<float> left{ 1.0f, 2.0f };
univector<float> right{ 3.0f, 4.0f };

univector<float> left_from_left;
univector<float> left_from_right;
univector<float> right_from_left;
univector<float> right_from_right;

ll.apply(left_from_left, left);     // L -> L
rl.apply(left_from_right, right);   // R -> L
lr.apply(right_from_left, left);    // L -> R
rr.apply(right_from_right, right);  // R -> R

left  = left_from_left + left_from_right;
right = right_from_left + right_from_right;
|||CHECK_THAT(left, DeepMatcher(univector<float>{ 2.5f, 4.0f }));
|||CHECK_THAT(right, DeepMatcher(univector<float>{ -1.0f, 0.0f }));
|||}
```

For channel-oriented storage, [[`univector2d`:nosig]] holds a vector per
channel. With `channels[0]` as left and `channels[1]` as right, substitute
those vectors for `left` and `right` in the example.

## Choosing a convolution API

Use [[`convolve(const univector<T1, Tag1> &, const univector<T2, Tag2> &)`:nosig]]
when both finite signals are already available and the complete result is
needed at once. It returns $N + M - 1$ samples for input lengths $N$ and $M$.
Use [[`convolve_filter<T>`:nosig]] for a fixed IR and an incoming stream, or
for long IRs where partitioned convolution avoids holding the whole signal in
memory.

For more about the overlap-add algorithm and its efficiency criterion, see
[Convolution filter details](convolution.md).
