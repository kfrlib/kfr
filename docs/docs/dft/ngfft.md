# The low-level ngFFT API

KFR 7.1 introduces ngFFT: a compact FFT API for code that needs direct control
over transform storage and execution. Unlike [[`dft_plan<T>`:nosig]], an
[[`ngfft_plan<T>`:nosig]] neither allocates memory nor requires a scratch buffer
when a transform runs. The caller supplies the precomputed twiddle-factor
storage, input, and output buffers.

ngFFT is deliberately narrower than the regular DFT API. It supports `float`
and `double` transforms whose lengths are powers of two, but provides complex
and real transforms in both directions, either in-place or out-of-place. The
smaller execution path and absence of scratch-buffer traffic can make it the
better choice in a tight, fixed-size processing loop. Use [[`dft_plan<T>`:nosig]]
instead when arbitrary transform sizes, the higher-level real-data layouts, or
the convenience of an owning plan are more important.

The API is available from:

```c++
#include <kfr/dft.hpp>
```

## How a plan is prepared

An ngFFT plan is a small, non-owning structure. For a complex transform of
length $N = 2^L$, construct [[`ngfft_plan<T>`:nosig]] with `L`, the base-2
logarithm of the transform length. For example, `ngfft_plan<float>{ 10 }`
selects a 1024-point transform; the constructor argument is not the sample
count.

The plan owns no twiddles. The setup sequence is always:

1. Construct the plan with the desired logarithmic size.
2. Call [[`ngfft_twiddle_count(ngfft_plan<T> &, dft_algorithm)`:nosig]] to get
   the required number of `complex<T>` elements.
3. Allocate an appropriately aligned buffer, assign it to `plan.twiddles`, and
   keep it alive for every execution using that plan.
4. Call [[`ngfft_initialize(ngfft_plan<T> &, dft_algorithm)`:nosig]] once to
   fill that buffer.

KFR's owning [[`univector<T, Size>`:nosig]] uses aligned storage and is a
convenient owner for the twiddle table. The transform API itself does not
allocate: using a `univector` here is a caller choice. A fixed buffer or an
application-specific allocator works as well, provided that the table is
cache-line aligned. Assign the buffer through
[[`ngfft_plan<T>::twiddles`:nosig]].

`ngfft_twiddle_count` returns `SIZE_MAX` for an unsupported size. In that case,
do not allocate or initialize the plan. `ngfft_initialize` returns `false` if
the size is unsupported or a required `twiddles` buffer was not supplied;
check that result before executing a transform.

### Complex-plan example

This example creates a reusable 1024-point complex plan and transforms a
buffer in place. The table is computed once and reused for every call.

```c++
using namespace kfr;

|||TEST_CASE("dft/ngfft complex transforms")
|||{
constexpr uint8_t l2size = 10;
constexpr size_t size = size_t(1) << l2size;

ngfft_plan<float> plan{ l2size };
const size_t twiddle_count = ngfft_twiddle_count(plan);
if (twiddle_count == SIZE_MAX)
	throw std::runtime_error("unsupported FFT size");

univector<complex<float>> twiddles(twiddle_count);
plan.twiddles = twiddles.data();
if (!ngfft_initialize(plan))
	throw std::runtime_error("could not initialize FFT plan");

univector<complex<float>, size> data;
data = counter(); // Fill data...
ngfft_execute(plan, false, data.data()); // forward transform, in place
```

The plan only contains the logarithmic size and the pointer. Do not move or
destroy `twiddles` while `plan` is still used. After initialization, execution
only reads the table, so a fully prepared plan and its immutable twiddles can
be shared by concurrent calls as long as each call uses independent data
buffers.

## Complex transforms

[[`ngfft_execute(const ngfft_plan<T> &, bool, complex<T> *, dft_algorithm)`]]
performs an in-place complex transform. Pass `false` for a forward transform or
`true` for an inverse transform. Its buffer contains exactly $N$ `complex<T>`
elements and is overwritten by the result.

[[`ngfft_execute(const ngfft_plan<T> &, bool, complex<T> *, const complex<T> *, dft_algorithm)`]]
accepts separate output and input pointers, each with $N$ `complex<T>`
elements. This allows the source signal to be retained. Passing the same
pointer for both arguments is the in-place case; use distinct, non-overlapping
buffers otherwise.

```c++
univector<complex<float>, size> input;
univector<complex<float>, size> spectrum;
input = counter();
|||const auto original_input = input;

// Forward, out of place: input is preserved.
ngfft_execute(plan, false, spectrum.data(), input.data());
|||CHECK_THAT(input, DeepMatcher(original_input));

// Inverse, in place: spectrum is replaced by the time-domain result.
ngfft_execute(plan, true, spectrum.data());
```

Neither direction applies normalization. A forward transform followed by an
inverse transform produces the original samples multiplied by $N$. Divide the
inverse result by the transform length when a normalized round trip is needed:

```c++
|||data = counter();
|||const auto round_trip_input = data;
ngfft_execute(plan, false, data.data());
ngfft_execute(plan, true, data.data());
data = data / size;
|||CHECK(rms(cabs(data - round_trip_input)) < 1e-4f);
```

The `inverse` argument has overloads for both a runtime `bool` and a
compile-time `cbool_t`. When the direction is fixed in the calling code,
[[`cfalse`:nosig]] and [[`ctrue`:nosig]] select the compile-time form:

```c++
ngfft_execute(plan, cfalse, data.data()); // forward
ngfft_execute(plan, ctrue, data.data());  // inverse
|||}
```

## Real transforms

Use [[`ngfft_plan_real<T>`:nosig]] for a real transform. Its constructor still
takes $L = \log_2 N$, where `N` is the number of real time-domain samples; it
adjusts the underlying complex-plan size internally. Consequently a real ngFFT
requires $N \geq 2$ and uses a power-of-two length.

[[`ngfft_real_execute(const ngfft_plan<T> &, complex<T> *, const T *, dft_algorithm)`:nosig]]
is the forward real-to-complex transform. It reads $N$ real samples and writes
$N/2$ `complex<T>` values. The reverse overload,
[[`ngfft_real_execute(const ngfft_plan<T> &, T *, const complex<T> *, dft_algorithm)`:nosig]],
reads those $N/2$ complex values and writes $N$ real samples. Neither overload
normalizes the result.

The real API uses the compact **Perm** representation. The DC and Nyquist bins
share the first complex element:

| Element | Contents |
| --- | --- |
| `spectrum[0].real()` | DC bin, $X[0]$ |
| `spectrum[0].imag()` | Nyquist bin, $X[N/2]$ |
| `spectrum[k]`, $1 \leq k < N/2$ | Ordinary positive-frequency bin $X[k]$ |

This differs from the CCS layout used by the ordinary real DFT. See
[DFT data layout](dft_layout.md) for a comparison. Keep the Nyquist value in
`spectrum[0].imag()` when passing a spectrum to the inverse function.

```c++
using namespace kfr;

|||TEST_CASE("dft/ngfft real transforms")
|||{
constexpr uint8_t l2size = 10;
constexpr size_t size = size_t(1) << l2size;

ngfft_plan_real<float> plan{ l2size };
const size_t twiddle_count = ngfft_twiddle_count(plan);
if (twiddle_count == SIZE_MAX)
	throw std::runtime_error("unsupported FFT size");

univector<complex<float>> twiddles(twiddle_count);
plan.twiddles = twiddles.data();
if (!ngfft_initialize(plan))
	throw std::runtime_error("could not initialize FFT plan");

univector<float, size> samples;
univector<complex<float>, size / 2> spectrum;
univector<float, size> reconstructed;
samples = counter();
|||const auto original_samples = samples;

ngfft_real_execute(plan, spectrum.data(), samples.data());
ngfft_real_execute(plan, reconstructed.data(), spectrum.data());
reconstructed = reconstructed / size;
|||CHECK(rms(reconstructed - original_samples) < 1e-4f);
|||}
```

The forward real transform may reuse the real input's storage as its
`N/2`-element complex output, and the inverse may reuse the spectrum storage
as its `N`-element real output. In both cases, treat the input as overwritten;
the same allocation must hold $N$ real scalar values. Separate buffers are
usually clearer and preserve the input, as in the preceding example.

## Algorithm selection and limits

All public functions accept an optional [[`dft_algorithm`:nosig]] argument.
The current default, [[`default_dft_algorithm`:nosig]], is the four-step FFT;
there is normally no reason to pass it explicitly. Query and initialize a plan
with the same algorithm value that will be used for execution.

ngFFT accepts only sizes representable as powers of two and supported by the
active architecture. It has no fallback for arbitrary lengths. Validate sizes
with `ngfft_twiddle_count` during setup, rather than assuming that every
power-of-two exponent is available on every target.

## Choosing between ngFFT and `dft_plan`

Choose ngFFT when transform size is fixed and known, memory ownership must stay
with the application, and the processing path benefits from no scratch buffer
or plan allocation. It is particularly suitable for real-time callbacks,
embedded processing, and applications that keep long-lived FFT state in a
caller-managed arena.

Choose [[`dft_plan<T>`:nosig]] or [[`dft_plan_real<T>`:nosig]] for arbitrary
sizes, higher-level packed real layouts, or a more convenient owning plan. The
regular API remains the general-purpose FFT interface; ngFFT trades that
generality for a smaller, more explicit execution path.
