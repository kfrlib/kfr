# Window Functions

A window is a finite sequence of weights applied to a block of samples. It reduces discontinuities at block boundaries before spectrum analysis, or controls the transition-width and stopband-attenuation trade-off in a windowed-sinc FIR filter. KFR provides every [[`window_type`:nosig]] family as a finite one-dimensional expression.

Include the DSP umbrella header or the focused header:

```c++
|||#include <kfr/dsp.hpp>
|||using namespace kfr;
// or: #include <kfr/dsp/window.hpp>
```

## Creating and rendering a window

Named constructors return concrete lazy expressions. No coefficient array is allocated or calculated until the expression is assigned to a destination or passed to [[`render(Expr &&)`:nosig]]. A window can therefore be multiplied with a frame in one expression pipeline, or rendered once and reused.

```c++
|||TEST_CASE("dsp/window_functions.md/create and render")
|||{
univector<float, 1024> frame = counter<float>();

// Apply a symmetric Hann window without a separate coefficient array.
frame = frame * window_hann<float>(frame.size());

// Materialize coefficients when they will be reused.
univector<float> weights = render(window_kaiser<float>(1024, 8.0f));
|||CHECK(frame.size() == 1024);
|||CHECK(weights.size() == 1024);
|||}
```

The returned window has the requested length and no evolving processing state, so it can be rendered repeatedly. As with all composed expressions, retain any lvalue operands until evaluation; see [Expression fundamentals](../expressions/fundamentals.md#lifetime-of-composed-expressions).

## Symmetric and periodic sampling

[[`window_symmetry`:nosig]] selects how KFR samples the continuous window:

* `symmetric` includes both endpoints and is symmetric about the centre. Use it for FIR coefficients and other finite, non-repeating sequences.
* `periodic` samples one period without duplicating its final endpoint. Use it for a DFT or other spectral-analysis frame.

For the ordinary window grids, a periodic window of length $N$ is the first $N$ values of the corresponding symmetric window of length $N + 1$. For example, symmetric seven-sample Hann coefficients are `{ 0, 0.25, 0.75, 1, 0.75, 0.25, 0 }`; the periodic eight-sample form is `{ 0, 0.14644661, 0.5, 0.85355339, 1, 0.85355339, 0.5, 0.14644661 }`. Thus an even symmetric centred window commonly has two equal peak-adjacent values below one, while an even periodic one can contain the single centre value one.

Only the named Blackman, Blackman-Harris, Planck-taper, and Tukey functions expose a symmetry argument. The remaining named constructors create symmetric windows. Use the compile-time generic [[`window(size_t, cval_t<window_type, type>, std::type_identity_t<T>, window_symmetry, ctype_t<T>)`:nosig]] API for a periodic form of any other family:

```c++
|||TEST_CASE("dsp/window_functions.md/select periodic generic window")
|||{
univector<float> symmetric_hann = render(window_hann<float>(9));

auto analysis_window = window<float>(
    8,
    cwindow_type<window_type::kaiser>,
    8.0f,
    window_symmetry::periodic);
univector<float> periodic_hann = render(analysis_window);
|||CHECK_THAT(symmetric_hann[0], Catch::Matchers::WithinAbs(symmetric_hann[8], 1e-6f));
|||CHECK_THAT(symmetric_hann[1], Catch::Matchers::WithinAbs(symmetric_hann[7], 1e-6f));
|||CHECK_THAT(symmetric_hann[2], Catch::Matchers::WithinAbs(symmetric_hann[6], 1e-6f));
|||CHECK_THAT(symmetric_hann[4], Catch::Matchers::WithinAbs(1.0f, 1e-6f));
|||CHECK_THAT(periodic_hann[0], !Catch::Matchers::WithinAbs(periodic_hann[7], 1e-3f));
|||CHECK_THAT(periodic_hann[1], !Catch::Matchers::WithinAbs(periodic_hann[6], 1e-3f));
|||CHECK_THAT(periodic_hann[2], !Catch::Matchers::WithinAbs(periodic_hann[5], 1e-3f));
|||CHECK_THAT(periodic_hann[4], Catch::Matchers::WithinAbs(1.0f, 1e-6f));
|||}
```

## Named window constructors

The following constructors select a concrete window type at compile time. The scalar type defaults to KFR's default floating-point type; specify it explicitly, such as `window_hann<float>(size)`, when the destination does not establish the desired type.

| Family | Constructor | Shape and parameter |
|---|---|---|
| Rectangular | [[`window_rectangular`:nosig]] | All coefficients are one. |
| Triangular | [[`window_triangular`:nosig]] | Linear peak with nonzero end samples. Its sampling differs from Bartlett. |
| Bartlett | [[`window_bartlett`:nosig]] | Linear peak that reaches zero at symmetric endpoints. |
| Cosine | [[`window_cosine`:nosig]] | A single sine lobe. |
| Cosine, NumPy-compatible | [[`window_cosine_np`:nosig]] | Cosine-family sampling compatible with NumPy's cosine window. |
| Hann | [[`window_hann`:nosig]] | Raised cosine, $0.5(1 - \cos(2\pi x))$. |
| Bartlett-Hann | [[`window_bartlett_hann`:nosig]] | Combination of linear and cosine tapers. |
| Hamming | [[`window_hamming`:nosig]] | Raised cosine with `alpha`, default `0.54`. |
| Bohman | [[`window_bohman`:nosig]] | Cosine-and-linear taper. |
| Blackman | [[`window_blackman`:nosig]] | Three-term cosine with `alpha`, default `0.16`; accepts symmetry. |
| Blackman-Harris | [[`window_blackman_harris`:nosig]] | Four-term minimum-sidelobe window; accepts symmetry. |
| Kaiser | [[`window_kaiser`:nosig]] | Modified-Bessel window with `beta`, default `0.5`. Larger `beta` increases tapering and sidelobe attenuation. |
| Flat-top | [[`window_flattop`:nosig]] | Five-term window for accurate amplitude estimates. Small negative values near the ends are part of its definition. |
| Gaussian | [[`window_gaussian`:nosig]] | Gaussian with `alpha`, default `2.5`. Larger `alpha` produces a narrower taper. |
| Lanczos | [[`window_lanczos`:nosig]] | Sinc-based taper. |
| Planck-taper | [[`window_planck_taper`:nosig]] | Smooth flat-top taper with `epsilon`, where $0 < \epsilon < 1$; accepts symmetry. |
| Tukey | [[`window_tukey`:nosig]] | Cosine-tapered flat-top window with `alpha`, where $0 < \alpha < 1$; accepts symmetry. |

`epsilon` is the fraction at each end occupied by the Planck-taper transition. Tukey's `alpha` controls the total fraction in its cosine-tapered region: smaller values leave a wider flat portion, while values nearer one approach a fully tapered cosine shape. KFR does not validate either range, so supply values strictly between zero and one.

The named constructors provide useful defaults for parameterized families:

```c++
|||TEST_CASE("dsp/window_functions.md/named constructors")
|||{
auto hamming  = window_hamming<float>(256);         // alpha = 0.54
auto blackman = window_blackman<float>(256);        // alpha = 0.16
auto kaiser   = window_kaiser<float>(256, 7.5f);    // choose beta explicitly
auto gaussian = window_gaussian<float>(256, 3.0f);  // choose alpha explicitly

auto tapered = window_tukey<float>(
    256, 0.25f, window_symmetry::periodic);
|||CHECK(render(hamming).size() == 256);
|||CHECK(render(blackman).size() == 256);
|||CHECK(render(kaiser).size() == 256);
|||CHECK(render(gaussian).size() == 256);
|||CHECK(render(tapered).size() == 256);
|||}
```

## Selecting a family with `window_type`

Use [[`cwindow_type`:nosig]] with the compile-time overload of [[`window(size_t, cval_t<window_type, type>, std::type_identity_t<T>, window_symmetry, ctype_t<T>)`:nosig]] when the family is known while compiling. It returns the same concrete expression type as the matching named constructor, which makes it suitable for templates:

```c++
|||TEST_CASE("dsp/window_functions.md/compile-time family selection")
|||{
auto periodic_blackman = window<float>(
    1024,
    cwindow_type<window_type::blackman>,
    0.16f,
    window_symmetry::periodic);

univector<float> weights = render(periodic_blackman);
|||CHECK(weights.size() == 1024);
|||}
```

Pass the window-specific value as `win_param`: it is ignored for parameterless families, is `alpha` for Hamming, Blackman, Gaussian, and Tukey, is `beta` for Kaiser, and is `epsilon` for Planck-taper. Although this overload has a default-constructed `win_param`, it does **not** substitute the named constructors' defaults. In particular, omitting it selects zero for Hamming, Blackman, Kaiser, and Gaussian rather than `0.54`, `0.16`, `0.5`, or `2.5`. Always pass a meaningful value for a parameterized family; for Planck-taper and Tukey zero is outside the valid range.

When the family is chosen at run time, use [[`window(size_t, window_type, std::type_identity_t<T>, window_symmetry, ctype_t<T>)`:nosig]]. It returns an owning [[`expression_handle<T, Dims>`:nosig]], hiding the concrete family while preserving its scalar type and one-dimensional shape:

```c++
|||TEST_CASE("dsp/window_functions.md/runtime family selection")
|||{
window_type selected = window_type::kaiser;

expression_handle<float> weights = window<float>(
    1024, selected, 8.0f, window_symmetry::periodic);

univector<float> values = render(weights);
|||CHECK(values.size() == 1024);
|||}
```

Both generic overloads support every [[`window_type`:nosig]] value: `rectangular`, `triangular`, `bartlett`, `cosine`, `hann`, `bartlett_hann`, `hamming`, `bohman`, `blackman`, `blackman_harris`, `kaiser`, `flattop`, `gaussian`, `lanczos`, `cosine_np`, `planck_taper`, and `tukey`.

The runtime overload owns the temporary expression it creates, so the handle above remains valid after the call returns. It is useful for menus, configuration files, and interfaces that must store several possible window families. Named and compile-time-selected expressions retain a concrete type and avoid type-erasure dispatch; prefer those APIs when the choice is static. See [Expression handles](../expressions/handle.md) for ownership and performance details.

## Using a window for FIR design

The FIR design functions accept an [[`expression_handle<T, Dims>`:nosig]] for the window. Convert a temporary named window with the owning overload of [[`to_handle(E &&)`:nosig]]:

```c++
|||TEST_CASE("dsp/window_functions.md/FIR design")
|||{
univector<float, 127> taps;

fir_lowpass(
    taps,
    0.2f,
    to_handle(window_kaiser<float>(taps.size(), 5.0f)),
    true);
|||CHECK(taps.size() == 127);
|||}
```

This is a symmetric Kaiser window, appropriate for a finite FIR tap sequence. The `0.2f` cutoff is normalized to the sample rate, so it represents $0.2 f_s$; Nyquist is $0.5 f_s$. The final `true` asks [[`fir_lowpass(univector<T, Tag> &, std::type_identity_t<T>, const expression_handle<T> &, bool)`:nosig]] to normalize DC gain. See [FIR filters](fir.md) for the complete design and application workflow.

## Practical selection notes

* Start with Hann for general spectral work and use Kaiser for FIR designs that need an adjustable trade-off.
* Use flat-top when amplitude accuracy is more important than narrow spectral peaks; do not clamp its small negative edge coefficients.
* Do not interchange triangular and Bartlett merely because both have a linear profile. Their endpoint sampling differs, especially for even lengths.
* Match the window length to the frame or tap count. A window expression is finite; assigning it to a differently sized destination is a shape error.
* Use `periodic` for a DFT frame and `symmetric` for FIR coefficients to avoid an unintended duplicate endpoint in a periodic analysis frame.
