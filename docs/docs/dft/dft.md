# Fast Fourier Transform with KFR

This guide explains KFR's general-purpose [Discrete Fourier Transform
(DFT)](../advanced/dsp_glossary.md#dft-vs-fft) interface. It covers complex and
real transforms, packed spectra, reusable plans, multidimensional transforms,
and the FFT-based operations built on them. For an overview of the DFT module
and links to related algorithms, start with [DFT and FFT in KFR](dft_introduction.md).

[[`dft_plan<T>`:nosig]] accepts arbitrary positive lengths, selects a mixed-radix,
four-step, or Bluestein implementation as appropriate for the requested size.
For a fixed power-of-two transform with caller-owned twiddles and no execution 
scratch storage, see the [low-level ngFFT API](ngfft.md).

> [!note]
> The DFT module is enabled by default with Clang and GCC, but disabled by
> default with MSVC. Enable it explicitly in an MSVC build with
> `-DKFR_ENABLE_DFT=ON`. MSVC DFT performance may be lower than with Clang or
> GCC, particularly for complex algorithms.

## Transform convention and normalization

For a complex sequence $x[n]$ of length $N$, KFR's direct (forward) transform
is

$$
X[k] = \sum_{n=0}^{N-1} x[n] e^{-2\pi i nk/N}.
$$

The inverse transform uses the opposite sign:

$$
x'[n] = \sum_{k=0}^{N-1} X[k] e^{+2\pi i nk/N}.
$$

**Neither direction is scaled.** This is intentional and applies to the
complex, real, multidimensional, cached, and low-level APIs. A forward
transform followed by an inverse produces

$$
x'[n] = N x[n].
$$

Divide after the inverse when a normalized round trip is required. For an
$D$-dimensional transform with shape $(S_0, \ldots, S_{D-1})$, divide by the
total element count $\prod_a S_a$.

```c++
|||#include <kfr/dft.hpp>
|||using namespace kfr;
|||TEST_CASE("dft.md/complex in-place round trip")
|||{

constexpr size_t size = 1024;
univector<complex<float>, size> data;
data = make_complex(counter(0.f), zeros<float>());
univector<complex<float>, size> original = data;

dft_plan<float> plan(size);
univector<u8> temp(plan.temp_size);

plan.execute(data, data, temp);       // forward, in place
plan.execute(data, data, temp, true); // inverse, in place
data = data / size;                   // restore the original scale
|||CHECK(rms(cabs(data - original)) < 1e-4f);
|||}
```

The `inverse` argument is `false` for the direct transform and `true` for the
inverse.

## Complex 1D transforms

Use [[`dft_plan<T>`:nosig]] for complex-to-complex transforms. `T` is `float`
or `double`; each input and output buffer holds exactly $N$ `complex<T>`
elements. Constructing a plan precomputes the internal stage data, so construct
it once for a fixed size and reuse it rather than rebuilding it for every
block. Plans are movable but not copyable.

The same initialized plan executes both directions and does not modify its
internal state during execution. It can therefore be shared between concurrent
calls when every call has its own data and scratch storage.

```c++
|||TEST_CASE("dft.md/complex out-of-place round trip")
|||{
constexpr size_t size = 1000; // Arbitrary positive sizes are supported.

dft_plan<double> plan(size);
univector<complex<double>> input(size);
input = make_complex(counter(0.0), zeros<double>());
univector<complex<double>> spectrum(size);
univector<complex<double>> reconstructed(size);
univector<u8> temp(plan.temp_size);

plan.execute(spectrum, input, temp);               // direct, out of place
plan.execute(reconstructed, spectrum, temp, true); // inverse, out of place
reconstructed = reconstructed / size;
|||CHECK(rms(cabs(reconstructed - input)) < 1e-10);
|||}
```

Passing the same input and output buffer is supported for the ordinary plan;
passing separate buffers preserves the input. Do not use partially overlapping
input and output ranges. The public result is always in ordinary frequency-bin
order: element `k` is $X[k]$. Internal radix and digit-reversal arrangements
are not exposed.

### Scratch storage and `temp_size`

[[`dft_plan<T>::temp_size`:nosig]] is the number of bytes of scratch storage
needed by [[`dft_plan<T>::execute(complex<T> *, const complex<T> *, u8 *, bool)`:nosig]].
Allocate a `univector<u8>` of that size once and reuse it with the plan, as in
the examples. This avoids a per-call temporary allocation and is normally the
right choice in loops, audio callbacks, and multithreaded code.

Passing `nullptr` as the scratch pointer is valid. KFR then obtains temporary
storage for that call when `temp_size` is nonzero. This is convenient for
occasional transforms but makes allocation lifetime part of the call. A scratch
buffer belongs to one execution at a time; do not share it between concurrent
calls without synchronization.

## Real transforms and packed spectra

A real sequence has a Hermitian spectrum:

$$
X[N-k] = \overline{X[k]}.
$$

The negative-frequency half is consequently redundant. A
[[`dft_plan_real<T>`:nosig]] reads $N$ real samples for a direct transform and
writes only the non-redundant bins; its inverse reads the packed spectrum and
reconstructs $N$ real samples. Use its `execute` overloads with different
pointer types to make the direction explicit:

```c++
|||TEST_CASE("dft.md/real packed round trip")
|||{
constexpr size_t size = 1024;
dft_plan_real<float> plan(size, dft_pack_format::CCs);

univector<float, size> input = counter();
univector<complex<float>> spectrum(plan.complex_size());
univector<float, size> output;
univector<u8> temp(plan.temp_size);

plan.execute(spectrum, input, temp);  // real -> packed complex, direct
plan.execute(output, spectrum, temp); // packed complex -> real, inverse
output = output / size;
|||CHECK(rms(output - input) < 1e-4f);
|||}
```

[[`dft_plan_real<T>::complex_size`:nosig]] is the authoritative output/input
count. Do not guess it from a buffer's size: it depends on the selected
[[`dft_pack_format`:nosig]]. One-dimensional real plans support odd as well as
even lengths. Odd lengths 
can cost more than even lengths, but require no different calling convention.

### CCS and Perm formats

`dft_pack_format` chooses between two layouts. The selected format is part of
the spectrum's meaning: forward and inverse plans, frequency-domain operations,
and external code must agree on it. **CCs** is the default format and stores
the non-negative frequency bins as ordinary complex values; it is returned by
[[`realdft(const univector<T, Tag> &)`:nosig]]. **Perm** packs the real DC and
Nyquist values together for an even-length transform, so frequency-domain
products must use [[`fft_multiply(univector<complex<T>, Tag1> &, const univector<complex<T>, Tag2> &, const univector<complex<T>, Tag3> &, dft_pack_format)`:nosig]] or
[[`fft_multiply_accumulate`:nosig]] with `dft_pack_format::Perm`.

See [DFT data layout](dft_layout.md) for the precise CCs and Perm element
counts and even- and odd-length layouts. When exchanging a
buffer, record its scalar type, real length, transform direction, normalization
convention, and packing format.

### In-place real buffers

The pointer overloads allow input and output to refer to the same allocation,
but the allocation must be large enough for **both views**. For an even-sized
Perm transform, $N$ real scalars occupy exactly $N/2$ complex values. CCS
needs $N + 2$ scalar slots for its $N/2 + 1$ complex output. For an odd-sized
transform the packed output occupies $N + 1$ scalar slots. Allocate for the
larger representation and use pointer casts only when the alignment and object
representation requirements of the surrounding code are satisfied.

## Plan options and output order

[[`dft_order`:nosig]] appears in the `dft_plan` constructor. Its `normal`
value describes ordinary bin order, while `internal` is reserved for an
implementation-defined digit-reversed order. At present KFR ignores this
parameter and always returns normal order, so applications should pass
`dft_order::normal` and must not rely on internal ordering for an optimization.

## Convenience functions and the cache

For one-off vector transforms, the free functions
[[`dft(const univector<complex<T>, Tag> &)`:nosig]],
[[`idft(const univector<complex<T>, Tag> &)`:nosig]],
[[`realdft(const univector<T, Tag> &)`:nosig]], and
[[`irealdft(const univector<complex<T>, Tag> &)`:nosig]] are concise:

```c++
|||TEST_CASE("dft.md/cached convenience transforms")
|||{
univector<complex<float>> complex_signal(8);
complex_signal = make_complex(counter(0.f), zeros<float>());
univector<float> real_signal(8);
real_signal = counter(0.f);
univector<complex<float>> spectrum = dft(complex_signal);
univector<complex<float>> round_trip = idft(spectrum) / spectrum.size();

univector<complex<float>> real_spectrum = realdft(real_signal); // CCS
univector<float> real_round_trip = irealdft(real_spectrum) / real_signal.size();
|||CHECK(rms(cabs(round_trip - complex_signal)) < 1e-5f);
|||CHECK(rms(real_round_trip - real_signal) < 1e-5f);
|||}
```

They obtain plans from the process-wide [[`dft_cache`:nosig]], keyed by element
type and size, and allocate scratch storage for each call. Cache access is
mutex-protected unless KFR is built in single-thread mode. Cached plans remain
alive until `dft_cache::instance().clear()` is called or the process exits.

Use this convenience layer for occasional transforms and ordinary containers.
Use explicit plans and caller-managed `temp_size` storage for a bounded
real-time path, repeated same-size transforms, explicit scratch ownership, or
concurrent work where each worker needs independent scratch memory. The cache
uses CCS for its real convenience functions; `irealdft` infers an **even**
real length from the CCS buffer size, so use `dft_plan_real` directly when an
odd-length real transform or Perm layout is needed.

## Multidimensional complex transforms

[[`dft_plan_md<T, Dims>`:nosig]] performs a separable complex DFT over every
axis of a shape. It constructs one 1D plan per axis and applies transforms
axis by axis. KFR stores tensors in row-major order: the final axis is
contiguous and changes fastest. A shape `{ rows, columns }` therefore stores
each row consecutively, and flat element `(r, c)` is at `r * columns + c`.

```c++
|||TEST_CASE("dft.md/multidimensional complex round trip")
|||{

const shape<2> image_shape{ 480, 640 };
dft_plan_md<float, 2> plan(image_shape);

tensor<complex<float>, 2> image(image_shape);
image = make_complex(counter(0.f), zeros<float>());
tensor<complex<float>, 2> original = image.copy();
tensor<complex<float>, 2> spectrum(image_shape);
univector<u8> temp(plan.temp_size);

plan.execute(spectrum, image, temp.data());       // 2D forward transform
plan.execute(image, spectrum, temp.data(), true); // 2D inverse transform
image = image / image_shape.product();
|||CHECK(rms(cabs(image - original)) < 1e-4f);
|||}
```

The raw-pointer overload accepts contiguous row-major storage of
`size.product()` complex values. Tensor overloads require input and output
[[`tensor<T, NDims>`:nosig]] objects of exactly the plan shape and require both
tensors to be contiguous. A transposed tensor view or a strided slice must be
copied to a contiguous tensor before calling the plan; its visible shape alone
is not enough because its strides describe a different physical layout.

`temp_size` is the largest scratch requirement among the axis plans. As with
1D plans, passing `nullptr` permits a temporary allocation for the call, while
a retained `univector<u8>` provides predictable reuse. In-place complex MD
execution is supported by supplying the same contiguous buffer for input and
output.

## Multidimensional real transforms

[[`dft_plan_md_real<T, Dims>`:nosig]] is the real counterpart. It transforms
the final axis as real-to-complex, retaining that axis's non-redundant CCS
bins, then transforms every preceding axis as complex-to-complex. For a real
input shape

$$
(S_0, S_1, \ldots, S_{D-1}),
$$

the packed spectrum shape is

$$
(S_0, S_1, \ldots, S_{D-2}, S_{D-1}/2 + 1).
$$

Call [[`dft_plan_md_real<T, Dims>::complex_size`:nosig]] rather than
reproducing this calculation in application code. The MD real API always uses
CCS; it does not offer the 1D Perm layout. The final axis must be even. Other
axes may have arbitrary positive sizes.

```c++
|||TEST_CASE("dft.md/multidimensional real round trip")
|||{
const shape<2> input_shape{ 480, 640 };
dft_plan_md_real<float, 2> plan(input_shape);

tensor<float, 2> input(input_shape);
input = counter(0.f);
tensor<float, 2> original = input.copy();
tensor<complex<float>, 2> spectrum(plan.complex_size());
tensor<float, 2> output(input_shape);
univector<u8> temp(plan.temp_size);

plan.execute(spectrum, input, temp.data());  // real 2D forward transform
plan.execute(output, spectrum, temp.data()); // inverse
output = output / input_shape.product();
|||CHECK(rms(output - original) < 1e-4f);
|||}
```

The tensor requirements are the same as for complex MD transforms: exact
shapes and contiguous row-major storage. On inverse execution the plan may
need a temporary complex working region in addition to its per-axis scratch
space. This region is included in `temp_size` by default.

For an in-place inverse, provide a real output allocation large enough for
[[`dft_plan_md_real<T, Dims>::real_out_size`:nosig]] real elements, then
construct the plan with `real_out_is_enough = true`. That tells the plan it can
use the output allocation as its complex workspace. If the output only holds
`size.product()` real values, keep the default `false` and allocate the full
`temp_size` buffer instead. This distinction prevents a common error: a packed
complex spectrum can require more storage than the visible real output.

## Discrete cosine transforms

[[`dct_plan<T>`:nosig]] provides a DCT-II in the forward direction and the
corresponding DCT-III in the inverse direction. Like the DFT APIs, it has a
reusable plan and a `temp_size` scratch requirement:

```c++
|||TEST_CASE("dft.md/DCT round trip")
|||{
dct_plan<float> plan(256);
univector<float, 256> input = counter();
univector<float, 256> coefficients, reconstructed;
univector<u8> temp(plan.temp_size);

plan.execute(coefficients, input, temp);               // unscaled DCT-II
plan.execute(reconstructed, coefficients, temp, true); // unscaled DCT-III
reconstructed = reconstructed * (2.0f / input.size());
|||CHECK(rms(reconstructed - input) < 1e-4f);
|||}
```

The forward DCT-II is unscaled and the inverse DCT-III is also unscaled. Their
round trip therefore requires the conventional factor $2/N$. For orthonormal
coefficients, apply the usual per-bin normalization as well: $1/\sqrt{N}$ for
the DC coefficient and $\sqrt{2/N}$ for the remaining forward coefficients.

## Convolution, correlation, and frequency-domain products

[[`convolve(const univector<T1, Tag1> &, const univector<T2, Tag2> &)`:nosig]]
computes the full linear convolution of two equally typed real or complex
signals. The result length is `a.size() + b.size() - 1`; KFR pads internally,
uses an FFT, and applies the required inverse scaling.
[[`correlate(const univector<T1, Tag1> &, const univector<T2, Tag2> &)`:nosig]]
computes full linear correlation, and
[[`autocorrelate(const univector<T, Tag1> &)`:nosig]] returns its signal's
non-negative lags. These functions are useful for finite-sized signals; they
support `float`, `double`, `complex<float>`, and `complex<double>`.

When building a frequency-domain algorithm yourself,
[[`fft_multiply(univector<complex<T>, Tag1> &, const univector<complex<T>, Tag2> &, const univector<complex<T>, Tag3> &, dft_pack_format)`:nosig]]
computes `dest = left * right`.
[[`fft_multiply_accumulate(univector<complex<T>, Tag1> &, const univector<complex<T>, Tag2> &, const univector<complex<T>, Tag3> &, dft_pack_format)`:nosig]]
provides `dest += left * right`; its four-input overload provides
`dest = addend + left * right`. They are ordinary element-wise complex
operations for CCS and full complex spectra; pass `Perm` for a Perm real
spectrum so DC and Nyquist remain independent.

For a long impulse response or a continuous input stream, use
[[`convolve_filter<T>`:nosig]]. It partitions the kernel, transforms its blocks
once, and processes later input through FFT overlap-add convolution. It supports
the same real and complex scalar types as the finite helpers. The block size is
rounded up to a power of two and is available through
[[`convolve_filter<T>::input_block_size`:nosig]]. Larger blocks usually improve
throughput and reduce per-sample overhead, but consume more memory and increase
the amount of audio that must accumulate before efficient block processing;
smaller blocks reduce that buffering cost at the expense of more FFT work. Feed
blocks that are multiples of the selected block size for best throughput. See
[Convolution filter details](convolution.md) and [Convolution reverb](conv_reverb.md)
for streaming behavior, overlap state, and end-of-stream tail handling with
[[`filter<T>::apply_zeros`:nosig]].

## See also

- [DFT and FFT in KFR](dft_introduction.md) — module overview and API choices.
- [Low-level ngFFT API](ngfft.md) — caller-owned twiddles, power-of-two sizes,
  and no execution scratch buffer.
- [DFT data layout](dft_layout.md) — concise CCS and Perm reference tables.
- [FFT-based sample rate conversion](dft_resampler.md) — overlap-save
  frequency-domain resampling.
- [Convolution filter details](convolution.md) — partitioned overlap-add
  convolution.
