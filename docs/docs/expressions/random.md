# Random expressions

KFR provides a compact SIMD-oriented pseudorandom generator for direct draws
and for infinite generator expressions. Use an explicitly seeded
[[`random_state`:nosig]] when a sequence must be repeatable. These APIs are for
simulation, test data, dithering, and signal generation; they are not intended
for cryptographic use.

## State and seeding

A [[`random_state`:nosig]] contains the generator's 128-bit state. Initialize
it from four 32-bit values with
[[`random_init(u32, u32, u32, u32)`:nosig]], or from two 64-bit values with
[[`random_init(u64, u64)`:nosig]]:

```c++
#include <kfr/base.hpp>

using namespace kfr;

|||TEST_CASE("random.md/explicit seeding")
|||{
random_state rng = random_init(1u, 2u, 3u, 4u);
|||CHECK(rng.v != portable_vec<u32, 4>{ 1u, 2u, 3u, 4u });
|||}
```

The initialization functions advance the supplied seed state once, so the
first output is generated state rather than the literal seed words.

On supported x86 builds, the no-argument [[`random_init()`:nosig]] overload
seeds from the CPU cycle counter:

```c++
|||TEST_CASE("random.md/cycle counter seeding")
|||{
random_state rng = random_init(); // variable seed on supported x86 builds
|||CHECK(rng.v != portable_vec<u32, 4>{});
|||}
```

This overload, and the generator-expression overloads that use it, are absent
when cycle-counter access is disabled. In particular, KFR disables it on
non-x86 targets. Cycle-counter seeding is convenient for nonrepeatable local
noise, but it is not a substitute for explicit deterministic seeds when a
result must be reproduced.

## Raw bits and direct distributions

[[`random_bits`:nosig]] produces a `vec<u8, N>` and advances the supplied state:

```c++
|||TEST_CASE("random.md/raw bits")
|||{
random_state rng = random_init(1u, 2u, 3u, 4u);
auto tag   = random_bits<8>(rng);
auto block = random_bits<64>(rng);
|||CHECK(tag.size() == 8);
|||CHECK(block.size() == 64);
|||}
```

The generator has no leftover-byte cache. Every request of 16 bytes or fewer
advances the whole 128-bit state once, even if only one returned byte is used.
Larger requests are assembled from 128-bit chunks. Changing request sizes or
splitting one request into several requests therefore changes the sequence:
`random_bits<16>(rng)` is not equivalent to four `random_bits<4>(rng)` calls.

> [!warning]
> The PRNG is deterministic **only** when both the seeds and the complete
> pattern of draws are the same. A draw consumes one or more 128-bit state
> chunks rather than a continuous stream of individual bytes. Changing a
> requested vector width, splitting a request, changing distributions, or
> changing generator-expression traversal changes subsequent output.
>
> ```c++
> |||TEST_CASE("random.md/draw granularity")
> |||{
> random_state one_block = random_init(1u, 2u, 3u, 4u);
> random_state two_parts = random_init(1u, 2u, 3u, 4u);
>
> auto a  = random_bits<16>(one_block); // advances one 128-bit state block
> auto b0 = random_bits<8>(two_parts);  // advances one block
> auto b1 = random_bits<8>(two_parts);  // advances another block
>
> // concat(b0, b1) is not equal to a, and the two states now differ.
> |||CHECK(concat(b0, b1) != a);
> |||CHECK(two_parts.v != one_block.v);
> |||}
> ```
>
> For reproducible output, retain the same seed, distribution, request sizes,
> and expression traversal.

The direct distribution functions write no storage; they return SIMD vectors
and mutate the `random_state` passed by reference:

| Function                                                | Distribution                             |
|---------------------------------------------------------|------------------------------------------|
| [[`random_uniform`:nosig]] with an integral result type | Raw uniformly distributed bit patterns   |
| [[`random_uniform`:nosig]] with `float` or `double`     | Uniform values in $[0, 1)$               |
| [[`random_range`:nosig]]                                | Values in $[\mathit{min}, \mathit{max})$ |
| [[`random_normal`:nosig]]                               | Gaussian values from $N(\mu, \sigma^2)$  |

```c++
|||TEST_CASE("random.md/direct distributions")
|||{
random_state rng = random_init(1u, 2u, 3u, 4u);
auto bits   = random_uniform<uint32_t, 4>(rng);
auto unit   = random_uniform<float, 8>(rng);
auto signed_noise = random_range<8>(rng, -1.0f, 1.0f);
auto gaussian = random_normal<8>(rng, 0.0f, 0.25f); // mu, sigma
|||CHECK(all((unit >= 0.0f) & (unit < 1.0f)));
|||CHECK(all((signed_noise >= -1.0f) & (signed_noise < 1.0f)));
|||CHECK(bits.size() == 4);
|||CHECK(gaussian.size() == 8);
|||}
```

The integer range mapping uses scaling rather than rejection sampling. Treat
it as KFR's fast range generator rather than relying on exact statistical
unbiasedness for every possible integer interval.

> [!note]
> [[`random_normal`:nosig]] takes `mu, sigma`, whereas the generator expression
> described below takes `sigma, mu` where `mu` defaults to 0. Name the arguments in a wrapper when that
> distinction would otherwise be unclear.

Normal samples use the Box-Muller transform. Requesting an odd vector width
internally draws an even count and discards one result, so it can consume a
different number of raw chunks from an even-width request.

## Generator expressions

[[`gen_random_uniform(const random_state &)`:nosig]],
[[`gen_random_range(const random_state &, T, T)`:nosig]], and
[[`gen_random_normal(const random_state &, T, T)`:nosig]] are the
generator-expression factories. They generate data only when KFR evaluates
them, so bound them before assigning to a new dynamic container:

```c++
|||#include <algorithm>
|||TEST_CASE("random.md/referenced range expression")
|||{
random_state rng = random_init(10u, 20u, 30u, 40u);

auto noise = gen_random_range<float>(std::ref(rng), -1.0f, 1.0f);
univector<float> samples = truncate(noise, 4096);
|||CHECK(samples.size() == 4096);
|||CHECK(std::all_of(samples.begin(), samples.end(), [](float value) { return value >= -1.0f && value < 1.0f; }));
|||}
```

The range factory accepts the inclusive lower and exclusive upper bound. The
normal factory's arguments are `sigma` followed by `mu` (defaulted to 0):

```c++
|||TEST_CASE("random.md/referenced normal expression")
|||{
random_state rng = random_init(10u, 20u, 30u, 40u);
auto distribution = gen_random_normal<float>(std::ref(rng),
                                             0.15f, // sigma
                                             0.50f  // mu
);
univector<float> samples = truncate(distribution, 4096);
|||CHECK(samples.size() == 4096);
|||}
```

Cycle-counter-seeded forms omit the state argument where supported:

```c++
|||TEST_CASE("random.md/cycle counter range expression")
|||{
// Available only when KFR supports reading the cycle counter.
univector<float> noise = truncate(gen_random_range<float>(-1.0f, 1.0f), 512);
|||CHECK(noise.size() == 512);
|||CHECK(std::all_of(noise.begin(), noise.end(), [](float value) { return value >= -1.0f && value < 1.0f; }));
|||}
```

As with other infinite sources, use [[`truncate`:nosig]], a sized
[[`render`:nosig]] overload, or [[`sink`:nosig]] to select how many values to
consume. A random expression is a stateful stream, not an index-stable array:
reading the same logical index twice produces new values. Different traversal,
vector widths, partitioning, or surrounding expression structure can change
how the state is consumed.

## Owning versus referenced state

The factories distinguish a normal `random_state` argument from
`std::ref(state)`:

```c++
|||TEST_CASE("random.md/owned and referenced state")
|||{
random_state rng = random_init(1u, 2u, 3u, 4u);

auto snapshot = gen_random_range<float>(rng, -1.0f, 1.0f); // owns a state copy
auto shared = gen_random_range<float>(std::ref(rng), -1.0f, 1.0f); // refers to rng
|||const random_state before_snapshot = rng;
|||const univector<float> snapshot_values = truncate(snapshot, 8);
|||CHECK(rng.v == before_snapshot.v);
|||const univector<float> shared_values = truncate(shared, 8);
|||CHECK(rng.v != before_snapshot.v);
|||CHECK(snapshot_values.size() == shared_values.size());
|||}
```

Passing `rng` copies its current state into the expression. Evaluating
`snapshot` does not alter `rng`; two independently constructed expressions
from the same state begin with the same stream. Copying such an expression
copies its current state and branches that stream. Pass `std::ref(rng)` to the
[[`gen_random_range(std::reference_wrapper<random_state>, T, T)`:nosig]]
overload for shared state.

Passing `std::ref(rng)` stores a non-owning reference. Evaluating `shared`
advances `rng`, and copies of `shared` refer to the same state and interleave
one stream. Keep the referenced `random_state` alive until every expression
that references it has been evaluated. Do not return or store a generator
expression that refers to a local state object.

## See also

- [Expression fundamentals](fundamentals.md), for infinite expressions and
  evaluation points
- [Expression handles](handle.md), when an expression's concrete type must be
  hidden or stored
