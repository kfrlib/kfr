# Mathematical Functions and Complex Processing

KFR provides element-wise arithmetic, elementary functions, and complex-number
operations that use the same vocabulary for individual values and SIMD data.
Include `<kfr/base.hpp>` for the complete expression-capable interface, or
`<kfr/math.hpp>` when only immediate scalar and vector calculations are needed.

```c++
#include <kfr/base.hpp>

using namespace kfr;
```

This article describes elementary functions that take scalar, complex, or
vector arguments. The exact domain differs by function: ordinary trigonometric
and logarithmic functions operate on real scalar or vector values, while the
`c`-prefixed functions are the complex equivalents. Vector arguments are
processed lane by lane, and scalar operands broadcast naturally when mixed with
vectors.

Almost every function listed here also has a lazy expression overload. Pass an
expression, such as a [[`univector<T, Size>`:nosig]], [[`counter(T)`:nosig]], or arithmetic
expression, and it builds a template expression instead of immediately
traversing a buffer, so multi-stage pipelines over large data buffers don't
need intermediate arrays. See [Expressions](../../expressions/expressions.md)
for expression lifetime and evaluation rules.

> [!note]
> Integer inputs to the real transcendental functions are promoted to a
> floating-point shape. For example, a `vec<i32, 4>` passed to [[`sin(const T1 &)`:nosig]] yields
> a floating-point vector. Mixed arithmetic and binary math functions use the
> common value type while preserving scalar, vector, and complex shape where
> applicable.

## Immediate values, vectors, and expressions

The following three forms use the same operations at different evaluation
scales:

```c++
|||TEST_CASE("mathematical_functions.md/immediate values")
|||{
// A scalar result
const f32 gain = exp(-0.5f);

// Four independent SIMD lanes
const f32x4 phases{ 0.f, c_pi<f32, 1, 2>, c_pi<f32>, c_pi<f32, 3, 2> };
const f32x4 waveform = sin(phases);

// A lazy expression, evaluated when it initializes output
const univector<f32> input = truncate(counter(), 1024) * 0.01f;
const univector<f32> output = tanh(input * 2.f);
|||CHECK_THAT(gain, DeepMatcher(0.6065306597f));
|||CHECK_THAT(waveform, DeepMatcher(f32x4{ 0.f, 1.f, 0.f, -1.f }));
|||CHECK(input.size() == 1024);
|||CHECK(output.size() == 1024);
|||}
```

Use the immediate form for a single value or an explicit [[`vec<T, N>`:nosig]]. Use the
expression form for a pipeline over a container. Both calculate the same
per-element mathematical result.

## Elementary arithmetic

For arithmetic operators, polynomial evaluation, interpolation, rounding,
remainders, saturation, bitwise operations, and numeric conversion, see
[Arithmetic, Bitwise Operations, and Type Conversion](../simd/arithmetic.md).

## Trigonometric functions

[[`sin(const T1 &)`:nosig]], [[`cos(const T1 &)`:nosig]], and [[`tan(const T1 &)`:nosig]] take angles in radians. Their degree-based
counterparts are [[`sindeg(const T1 &)`:nosig]], [[`cosdeg(const T1 &)`:nosig]], and [[`tandeg(const T1 &)`:nosig]]; `sindeg(90.f)`
is the same value as `sin(c_pi<f32, 1, 2>)`.

### Inverse trigonometric functions and phase

[[`asin(const T1 &)`:nosig]] and [[`acos(const T1 &)`:nosig]] accept real inputs in $[-1,1]$. Their outputs are in
$[-\pi/2,\pi/2]$ and $[0,\pi]$, respectively. Inputs outside that domain
produce invalid floating-point results.

[[`atan(const T1 &)`:nosig]] returns an angle in $[-\pi/2,\pi/2]$; [[`atandeg(const T1 &)`:nosig]] returns the
same angle in degrees. [[`atan2(const T1 &, const T2 &)`:nosig]] and [[`atan2deg(const T1 &, const T2 &)`:nosig]] use the conventional
argument order `(y, x)` and select the quadrant from both signs:

```c++
|||TEST_CASE("mathematical_functions.md/atan2")
|||{
const f32 phase_radians = atan2(1.f, 0.f); // pi / 2
const f32 phase_degrees = atan2deg(-1.f, 0.f); // -90
|||CHECK_THAT(phase_radians, DeepMatcher(c_pi<f32> / 2));
|||CHECK_THAT(phase_degrees, DeepMatcher(-90.f));
|||}
```

Use [[`atan2(const T1 &, const T2 &)`:nosig]] rather than [[`atan(const T1 &)`:nosig]] when a two-dimensional direction or a
complex phase is required, because division before [[`atan(const T1 &)`:nosig]] loses quadrant
information.

### Sinc and paired lane operations

[[`sinc(const T1 &)`:nosig]] uses the unnormalized definition

$$
\operatorname{sinc}(x) = \begin{cases}
\sin(x)/x, & |x| > \epsilon, \\
1, & |x| \leq \epsilon.
\end{cases}
$$

Its argument is in radians; the DSP-normalized $\sin(\pi x)/(\pi x)$
definition uses different scaling. The zero case is defined as the limiting value 1.

[[`sincos(const T1 &)`:nosig]] and [[`cossin(const T1 &)`:nosig]] produce interleaved vector results rather than scalar
pairs. [[`sincos(const T1 &)`:nosig]] places `sin(x)` in even lanes and
`cos(x)` in odd lanes; [[`cossin(const T1 &)`:nosig]] reverses that order. [[`sincosdeg(const T1 &)`:nosig]] and
[[`cossindeg(const T1 &)`:nosig]] apply the same rule to degree inputs.

```c++
|||TEST_CASE("mathematical_functions.md/sincos")
|||{
const f32x4 paired_angles{ 0.f, 0.f, c_pi<f32, 1, 2>, c_pi<f32, 1, 2> };
const auto paired_values = sincos(paired_angles);
// { sin(0), cos(0), sin(pi/2), cos(pi/2) } = { 0, 1, 1, 0 }
|||CHECK_THAT(paired_values, DeepMatcher(f32x4{ 0.f, 1.f, 1.f, 0.f }));
|||}
```

When `sin(x)` and `cos(x)` are already available, [[`sin2x(const T &, const T &)`:nosig]], [[`sin3x(const T &, const T &)`:nosig]],
[[`cos2x(const T &, const T &)`:nosig]], and [[`cos3x(const T &, const T &)`:nosig]] derive multiple-angle values without evaluating
trigonometric functions again.

### Fast trigonometric approximations

[[`fastsin(const T1 &)`:nosig]], [[`fastcos(const T1 &)`:nosig]], [[`fastsindeg(const T1 &)`:nosig]], and [[`fastcosdeg(const T1 &)`:nosig]] trade
accuracy for speed. Use the regular functions unless reduced precision is
appropriate for the algorithm. Measured over $[0, 2\pi]$, the errors are
approximately the same for `float` and `double`:

| Function  | Maximum absolute error |    Mean absolute error |
|-----------|-----------------------:|-----------------------:|
| `fastsin` | $1.216 \times 10^{-6}$ | $4.738 \times 10^{-7}$ |
| `fastcos` | $1.222 \times 10^{-6}$ | $4.550 \times 10^{-7}$ |

Outside $[0, 2\pi]$, these approximations are not numerically stable and their
error increases rapidly.

## Exponential, logarithmic, and root functions

[[`exp(const T1 &)`:nosig]], [[`exp2(const T1 &)`:nosig]], and [[`exp10(const T1 &)`:nosig]] calculate $e^x$, $2^x$, and $10^x$.
[[`exp(const T1 &)`:nosig]] maps negative infinity to zero and positive infinity to positive
infinity. [[`exp_fmadd(const T1 &, const T2 &, const T3 &)`:nosig]] evaluates $\exp(xm+a)$ directly.

[[`log(const T1 &)`:nosig]], [[`log2(const T1 &)`:nosig]], and [[`log10(const T1 &)`:nosig]] calculate real logarithms. Use a
strictly positive argument: zero produces negative infinity, and a negative
argument produces NaN. [[`logb(const T1 &)`:nosig]] returns the unbiased binary exponent and
returns negative infinity for zero. [[`logn(const T1 &, const T2 &)`:nosig]] calculates a logarithm with a
specified base, [[`logm(const T1 &, const T2 &)`:nosig]] multiplies a natural logarithm by a scale, and
[[`log_fmadd(const T1 &, const T2 &, const T3 &)`:nosig]] evaluates $\log(x)m+a$.

```c++
|||TEST_CASE("mathematical_functions.md/log10 level")
|||{
const f32x4 amplitude{ 1.f, 0.5f, 0.f, -0.5f };
const auto safe_amplitude = max(amplitude, 1.0e-20f);
const auto level = log10(safe_amplitude) * 20.f;
|||CHECK_THAT(level[0], DeepMatcher(0.f));
|||CHECK(level[1] < 0.f);
|||}
```

[[`pow(const T1 &, const T2 &)`:nosig]] calculates a real power. A negative base is valid only with an
integer exponent; a negative base combined with a non-integer exponent yields
NaN. [[`root(const T1 &, const T2 &)`:nosig]] calculates a real $n$th root. It accepts negative values for
odd integer degrees and returns a negative root; even or non-integer roots of
negative values yield NaN. [[`sqrt(const T1 &)`:nosig]] is the positive square root and produces
NaN for negative real inputs. [[`cbrt(const T1 &)`:nosig]] supports negative inputs.

```c++
|||TEST_CASE("mathematical_functions.md/pow root cbrt")
|||{
const f32 sixth_power = pow(2.f, 6.f); // 64
const f32 negative_root = root(-32.f, 5.f); // -2
const f32 cube_root = cbrt(-8.f); // -2
|||CHECK_THAT(sixth_power, DeepMatcher(64.f));
|||CHECK_THAT(negative_root, DeepMatcher(-2.f));
|||CHECK_THAT(cube_root, DeepMatcher(-2.f));
|||}
```

## Hyperbolic and special functions

The hyperbolic family comprises [[`sinh(const T1 &)`:nosig]], [[`cosh(const T1 &)`:nosig]], [[`tanh(const T1 &)`:nosig]], and
[[`coth(const T1 &)`:nosig]]. [[`coth(const T1 &)`:nosig]] is undefined at zero. The interleaved vector functions
[[`sinhcosh(const T1 &)`:nosig]] and [[`coshsinh(const T1 &)`:nosig]] place the first named result in even lanes
and the second in odd lanes, following the same convention as [[`sincos(const T1 &)`:nosig]].

[[`gamma(const T1 &)`:nosig]] is an approximation of the gamma function for positive real
arguments. [[`factorial_approx(const T1 &)`:nosig]] evaluates an approximate factorial as
$\Gamma(x+1)$ and is useful for floating-point calculation. [[`factorial(int)`:nosig]]
is the integer lookup form, returning a `uint64_t` result for its supported
range, including `factorial(0) == 1`, and a maximum `uint64_t` sentinel
outside it. Values larger than $20!$ do not fit in `uint64_t`.

[[`modzerobessel(const T1 &)`:nosig]] calculates the modified zeroth-order Bessel function of
the first kind, $I_0(x)$. It is commonly useful in coefficient calculations
such as Kaiser windows.

> [!note]
> [[`gamma(const T1 &)`:nosig]], [[`factorial_approx(const T1 &)`:nosig]], and [[`modzerobessel(const T1 &)`:nosig]] are
> approximate implementations. Exact ulp error bounds and special-value behavior depend on the target
> version and input range.

## Complex numbers

[[`complex`:nosig]] is KFR's complex value type, normally an alias of
`std::complex`. [[`c32`:nosig]] and [[`c64`:nosig]] are convenient single- and
double-precision aliases. Construct a scalar complex value or a vector of
complex values with [[`make_complex(T1, T2)`:nosig]]. [[`real(const complex<T> &)`:nosig]] and [[`imag(const complex<T> &)`:nosig]] extract its
components, and [[`cconj(const T1 &)`:nosig]] returns its conjugate.

```c++
|||TEST_CASE("mathematical_functions.md/complex real imag conj")
|||{
const c32 z = make_complex(3.f, 4.f);
const f32 re = kfr::real(z); // 3
const f32 im = kfr::imag(z); // 4
const c32 conjugate = cconj(z); // 3 - 4i
|||CHECK_THAT(re, DeepMatcher(3.f));
|||CHECK_THAT(im, DeepMatcher(4.f));
|||CHECK_THAT(conjugate, DeepMatcher(c32{ 3.f, -4.f }));
|||}
```

For a `vec<complex<f32>, N>`, each vector lane is one complex number. Its
flattened scalar layout is interleaved as `re0, im0, re1, im1, ...`.
[[`cdecom(const vec<complex<T>, N> &)`:nosig]] exposes that flattened real vector and [[`ccomp(const vec<T, N> &)`:nosig]] rebuilds the
complex vector. [[`cdupreal(const vec<complex<T>, N> &)`:nosig]], [[`cdupimag(const vec<complex<T>, N> &)`:nosig]], [[`cswapreim(const vec<complex<T>, N> &)`:nosig]],
[[`cnegreal(const vec<complex<T>, N> &)`:nosig]], and [[`cnegimag(const vec<complex<T>, N> &)`:nosig]] are SIMD layout transforms for specialized
complex algorithms.

### Complex magnitude, phase, and coordinates

[[`cabssqr(const T1 &)`:nosig]] returns $\operatorname{re}(z)^2 + \operatorname{im}(z)^2$;
[[`cabs(const T1 &)`:nosig]] returns its square root. [[`carg(const T1 &)`:nosig]] returns the phase using
`atan2(imag(z), real(z))`. [[`polar(const T1 &)`:nosig]] represents a complex value as another
complex value whose real part is magnitude and imaginary part is phase.
[[`cartesian(const T1 &)`:nosig]] reverses that conversion.

```c++
|||TEST_CASE("mathematical_functions.md/complex magnitude phase")
|||{
const c32 z = make_complex(3.f, 4.f);
const f32 magnitude = cabs(z); // 5
const f32 phase = carg(z);
const c32 magnitude_phase = polar(z);
const c32 restored = cartesian(magnitude_phase);
|||CHECK_THAT(magnitude, DeepMatcher(5.f));
|||epsilon_scope<void> eps(5);
|||CHECK_THAT(restored, DeepMatcher(z));
|||(void)phase;
|||}
```

### Complex elementary functions

Use the complex-prefixed functions when the input or desired result is complex:

| Operation                    | Functions                                                                                                                                                     |
|------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Trigonometric and hyperbolic | [[`csin(const T1 &)`:nosig]], [[`ccos(const T1 &)`:nosig]], [[`csinh(const T1 &)`:nosig]], [[`ccosh(const T1 &)`:nosig]]                                      |
| Exponential                  | [[`cexp(const T1 &)`:nosig]], [[`cexp2(const T1 &)`:nosig]], [[`cexp10(const T1 &)`:nosig]]                                                                   |
| Logarithm                    | [[`clog(const T1 &)`:nosig]], [[`clog2(const T1 &)`:nosig]], [[`clog10(const T1 &)`:nosig]]                                                                   |
| Roots and powers             | [[`csqrt(const T1 &)`:nosig]], [[`csqr(const T1 &)`:nosig]]                                                                                                   |
| Magnitude and phase          | [[`cabssqr(const T1 &)`:nosig]], [[`cabs(const T1 &)`:nosig]], [[`carg(const T1 &)`:nosig]], [[`polar(const T1 &)`:nosig]], [[`cartesian(const T1 &)`:nosig]] |

For example, the complex exponential maps a logarithmic magnitude and phase
back to Cartesian form:

```c++
|||TEST_CASE("mathematical_functions.md/cexp")
|||{
const c32 exponent = make_complex(log(2.f), c_pi<f32, 1, 2>);
const c32 value = cexp(exponent); // approximately 0 + 2i
|||CHECK_THAT(value, DeepMatcher(c32{ 0.f, 2.f }));
|||}
```

[[`clog(const T1 &)`:nosig]] uses magnitude and principal phase, so complex logarithms and roots
follow their standard branch-dependent interpretation. If a signal-processing
algorithm needs phase continuity across a sequence, unwrap or otherwise manage
phase at the algorithm level rather than assuming a scalar complex function
will do so.

## Choosing the right operation

- Use [[`pow2(const T &)`:nosig]] through [[`pow5(const T &)`:nosig]], [[`sqr(const T1 &)`:nosig]], or [[`cub(const T1 &)`:nosig]] when the exponent
  is fixed and small; use [[`ipow(const T &, int)`:nosig]] for a non-negative integer exponent and
  [[`pow(const T1 &, const T2 &)`:nosig]] for a real exponent.
- Use [[`atan2(const T1 &, const T2 &)`:nosig]] for directions and phases, and remember its `(y, x)` order.
- Protect real logarithm inputs with [[`max(const T1 &, const T2 &)`:nosig]] or another domain policy when
  input data can reach zero or become negative.
- Use [[`cabs(const T1 &)`:nosig]] and [[`carg(const T1 &)`:nosig]] for complex magnitude and phase rather than
  manually dividing components.
- Prefer the lazy expression overloads for multi-stage processing of large
  buffers; materialize only at an intended output or evaluation boundary.

## See Also

- [Arithmetic, Bitwise Operations, and Type Conversion](../simd/arithmetic.md)
- [The `vec` Type](../simd/vec.md)
- [Numeric, Compound, and Complex Types](../core/types.md)
- [Expressions](../../expressions/expressions.md)
- [Window Functions](../../dsp/examples/window_gallery.md)
