# How to apply Convolution Reverb

## Mono version

Input/Output data: [See how to pass data to KFR](types.md)
```c++
univector<float> audio;
univector<float> impulse_response;
```
Code
```c++
convolve_filter<float> reverb(impulse_response);
reverb.apply(audio);
```

!!! note
    `convolve_filter` uses [Filter API](filters.md) and preserves its internal state between calls to `apply`.
    Audio can be processed in chunks.
    Use `reset` function to reset its internal state.

    `convolve_filter` has zero latency.

    Internally, [FFT](dft.md) is used for performing convolution

## True stereo version

Formula

$$
\begin{aligned}
    out_{L} &= in_{L} * ir_{LL} + in_{R} * ir_{RL}\\
    out_{R} &= in_{L} * ir_{LR} + in_{R} * ir_{RR}
\end{aligned}
$$

where $*$ is convolution operator.

Input/Output data:
```c++
univector<float, 2> stereo_audio;
univector<float, 4> impulse_response;
// impulse_response[0] is left to left
// impulse_response[1] is right to left
// impulse_response[2] is left to right
// impulse_response[3] is right to right
```
Code
```c++
// Prepare filters
convolve_filter<float> reverb_LL(impulse_response[0]);
convolve_filter<float> reverb_LR(impulse_response[1]);
convolve_filter<float> reverb_RL(impulse_response[2]);
convolve_filter<float> reverb_RR(impulse_response[3]);

// Allocate temp data
univector<float> tmp1(stereo_audio[0].size());
univector<float> tmp2(stereo_audio[0].size());
univector<float> tmp3(stereo_audio[0].size());
univector<float> tmp4(stereo_audio[0].size());

// Apply convolution
reverb_LL.apply(tmp1, audio[0]);
reverb_RL.apply(tmp2, audio[1]);
reverb_LR.apply(tmp3, audio[0]);
reverb_RR.apply(tmp4, audio[1]);

// final downmix
audio[0] = tmp1 + tmp2;
audio[1] = tmp3 + tmp4;
```

# Implementation Details

The convolution filter efficiently computes the convolution of two signals.
The efficiency is achieved by employing the FFT and the circular convolution
theorem.  The algorithm is a variant of the [overlap-add
method](https://en.wikipedia.org/wiki/Overlap%E2%80%93add_method).  It works on
a fixed block size \(B\) for arbitrarily long input signals.  Thus, the
convolution of a streaming input signal with a long FIR filter \(h[n]\) (where
the length of \(h[n]\) may exceed the block size \(B\)) is computed with a
fixed complexity \(O(B \log B)\).

More formally, the convolution filter computes \(y[n] = (x * h)[n]\) by
partitioning the input \(x\) and filter \(h\) into blocks and applies the
overlap-add method. Let \(x[n]\) be an input signal of arbitrary length. Often,
\(x[n]\) is a streaming input with unknown length.  Let \(h[n]\) be an FIR
filter with \(M\) taps.  The convolution filter works on a fixed block size
\(B=2^b\).

First, the input and filter are windowed and shifted to the origin to give the
\(k\)-th block input \(x_k[n] = x[n + kB] , n=\{0,1,\ldots,B-1\},\forall
k\in\mathbb{Z}\) and \(j\)-th block filter \(h_j[n] = h[n + jB] ,
n=\{0,1,\ldots,B-1\},j=\{0,1,\ldots,\lfloor M/B \rfloor\}\).  The convolution
\(y_{k,j}[n] = (x_k * h_j)[n]\) is efficiently computed with length \(2B\) FFTs
as
\[
y_{k,j}[n] = \mathrm{IFFT}(\mathrm{FFT}(x_k[n])\cdot\mathrm{FFT}(h_j[n]))
.
\]

The overlap-add method sums the "overlap" from the previous block with the current block.
To complete the \(k\)-th block, the contribution of all blocks of the filter
are summed together to give
\[ y_{k}[n] = \sum_j y_{k-j,j}[n] . \]
The final convolution is then the sum of the shifted blocks
\[ y[n] = \sum_k y_{k}[n - kB] . \]
Note that \(y_k[n]\) is of length \(2B\) so its second half overlaps and adds
into the first half of the \(y_{k+1}[n]\) block.

## Maximum efficiency criterion

To avoid excess computation or maximize throughput, the convolution filter
should be given input samples in multiples of the block size \(B\).  Otherwise,
the FFT of a block is computed twice as many times as would be necessary and
hence throughput is reduced.
