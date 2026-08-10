# DFT data layout

This page describes the packed spectra used by one-dimensional real DFT plans.
For a real input of length $N$, let $X[k]$ be the ordinary complex DFT bin at
index $k$. Hermitian symmetry makes the negative-frequency bins redundant:

$$
X[N-k] = \overline{X[k]}.
$$

KFR therefore stores only the independent non-negative-frequency bins. Select
the representation with [[`dft_pack_format`:nosig]] when constructing a
[[`dft_plan_real<T>`:nosig]]. The format is part of the spectrum's meaning:
the forward transform, inverse transform, and every frequency-domain operation
on the buffer must use the same format.

The names and scalar ordering follow the Intel IPP packed-format
terminology.
KFR does not provide Intel IPP's distinct **Pack** format.

## Element counts

Always use [[`dft_plan_real<T>::complex_size`:nosig]] to size the packed
buffer. The formulas below state what it returns:

| Format | Even $N$ | Odd $N$ |
| --- | ---: | ---: |
| `dft_pack_format::CCs` | $N/2 + 1$ | $\lfloor N/2 \rfloor + 1 = (N+1)/2$ |
| `dft_pack_format::Perm` | $N/2$ | $\lceil N/2 \rceil = (N+1)/2$ |

For even $N$, DC ($X[0]$) and Nyquist ($X[N/2]$) are purely real. `Perm`
combines them to save one `complex<T>` element. For odd $N$, there is no
Nyquist bin, so `CCs` and `Perm` contain the same data and have identical
storage.

## Even-length transforms

For even $N$, the independent bins are $X[0]$ through $X[N/2]$.

### CCs

`CCs` (complex-conjugate symmetric) stores each non-negative-frequency bin as
an ordinary `complex<T>` value. The imaginary components of DC and Nyquist are
zero:

| Complex index | Real component | Imaginary component |
| ---: | --- | --- |
| `0` | $\operatorname{Re}(X[0])$ (DC) | $0$ |
| `1` | $\operatorname{Re}(X[1])$ | $\operatorname{Im}(X[1])$ |
| $\vdots$ | $\vdots$ | $\vdots$ |
| `N/2 - 1` | $\operatorname{Re}(X[N/2-1])$ | $\operatorname{Im}(X[N/2-1])$ |
| `N/2` | $\operatorname{Re}(X[N/2])$ (Nyquist) | $0$ |

This is the default format and is best suited to code that indexes frequency
bins directly. [[`realdft(const univector<T, Tag> &)`:nosig]] returns CCs.

### Perm

`Perm` packs the two real endpoint bins into `spectrum[0]`. The remaining
elements are the ordinary positive-frequency complex bins:

| Complex index | Real component | Imaginary component |
| ---: | --- | --- |
| `0` | $\operatorname{Re}(X[0])$ (DC) | $\operatorname{Re}(X[N/2])$ (Nyquist) |
| `1` | $\operatorname{Re}(X[1])$ | $\operatorname{Im}(X[1])$ |
| $\vdots$ | $\vdots$ | $\vdots$ |
| `N/2 - 1` | $\operatorname{Re}(X[N/2-1])$ | $\operatorname{Im}(X[N/2-1])$ |

Viewed as $N$ scalar values, the layout is:

```text
Re(X[0]), Re(X[N/2]), Re(X[1]), Im(X[1]), ..., Re(X[N/2-1]), Im(X[N/2-1])
```

`spectrum[0]` is a packed pair, not a complex DFT bin. Frequency-domain
operations must therefore process its real (DC) and imaginary (Nyquist)
components independently. Pass `dft_pack_format::Perm` to
[[`fft_multiply`:nosig]] or [[`fft_multiply_accumulate`:nosig]] so KFR applies
that special handling.

## Odd-length transforms

For odd $N$, the independent bins are $X[0]$ through $X[(N-1)/2]$. Only DC is
purely real; $X[(N-1)/2]$ is an ordinary complex bin. Consequently `CCs` and
`Perm` use the same $(N+1)/2$ `complex<T>` elements:

| Complex index | Real component | Imaginary component |
| ---: | --- | --- |
| `0` | $\operatorname{Re}(X[0])$ (DC) | $0$ |
| `1` | $\operatorname{Re}(X[1])$ | $\operatorname{Im}(X[1])$ |
| $\vdots$ | $\vdots$ | $\vdots$ |
| `(N-1)/2` | $\operatorname{Re}(X[(N-1)/2])$ | $\operatorname{Im}(X[(N-1)/2])$ |

The `Perm` selector has no endpoint-packing effect for an odd-length plan.

## Converting an even-length Spectrum

The formats differ only in where they store Nyquist. To convert CCs to Perm:

```c++
||||||||||
// Both buffers describe an even-length real transform of length size.
perm[0] = { ccs[0].real(), ccs[size / 2].real() };
for (size_t k = 1; k < size / 2; ++k)
    perm[k] = ccs[k];
||||||||||
```

To convert in the opposite direction, copy bins `1` through `N/2 - 1`, then
write `{ dc, 0 }` at index `0` and `{ nyquist, 0 }` at index `N/2`. No
conversion is required for odd lengths because both formats have the same
layout.

See [Fast Fourier Transform with KFR](dft.md) for plan use, normalization,
scratch storage, and in-place buffer requirements.
