# Convolution filter details

## Implementation Details

The convolution filter efficiently computes the convolution of two signals.
The efficiency is achieved by employing the FFT and the circular convolution
theorem.  The algorithm is a variant of the [overlap-add
method](https://en.wikipedia.org/wiki/Overlap%E2%80%93add_method).  It works on
a fixed block size $B$ for arbitrarily long input signals.  Thus, the
convolution of a streaming input signal with a long FIR filter $h[n]$ (where
the length of $h[n]$ may exceed the block size $B$) is computed with a
fixed complexity $O(B \log B)$.

More formally, the convolution filter computes $y[n] = (x * h)[n]$ by
partitioning the input $x$ and filter $h$ into blocks and applies the
overlap-add method. Let $x[n]$ be an input signal of arbitrary length. Often,
$x[n]$ is a streaming input with unknown length.  Let $h[n]$ be an FIR
filter with $M$ taps.  The convolution filter works on a fixed block size
$B=2^b$.

First, the input and filter are windowed and shifted to the origin to give the
$k$-th block input $x_k[n] = x[n + kB] , n=\{0,1,\ldots,B-1\},\forall k\in\mathbb{Z}$ and $j$-th block filter $h_j[n] = h[n + jB] ,n=\{0,1,\ldots,B-1\},j=\{0,1,\ldots,\lfloor M/B \rfloor\}$.  The convolution
$y_{k,j}[n] = (x_k * h_j)[n]$ is efficiently computed with length $2B$ FFTs
as

$$
y_{k,j}[n] = \mathrm{IFFT}(\mathrm{FFT}(x_k[n])\cdot\mathrm{FFT}(h_j[n]))
.
$$

The overlap-add method sums the "overlap" from the previous block with the current block.
To complete the $k$-th block, the contribution of all blocks of the filter
are summed together to give

$$ y_{k}[n] = \sum_j y_{k-j,j}[n] . $$

The final convolution is then the sum of the shifted blocks

$$ y[n] = \sum_k y_{k}[n - kB] . $$

Note that $y_k[n]$ is of length $2B$ so its second half overlaps and adds
into the first half of the $y_{k+1}[n]$ block.

## Maximum efficiency criterion

To avoid excess computation or maximize throughput, the convolution filter
should be given input samples in multiples of the block size $B$.  Otherwise,
the FFT of a block is computed twice as many times as would be necessary and
hence throughput is reduced.

