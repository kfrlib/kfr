# Compile-Time Values and Metaprogramming Utilities

KFR uses a small set of compile-time value and type wrappers to express work
that must be specialized before the program runs. They appear throughout the
SIMD and expression implementations: choosing a vector width, making a shuffle
table, enumerating supported scalar types, or unrolling a fixed loop.

Most application code does not need these utilities. They are useful when
writing a custom SIMD primitive, an expression extension, or another template
library that needs a value to participate in overload resolution or template
instantiation. The utilities are header-only and are available from
`<kfr/meta.hpp>`.

## Compile-Time Values

[[`cval_t<T, val>`:nosig]] represents `value` as both a type and a lightweight
object. Its `value` member, conversion operator, and call operator all expose
the wrapped value. The aliases [[`cbool_t`:nosig]], [[`cint_t`:nosig]],
[[`cuint_t`:nosig]], and [[`csize_t`:nosig]] cover the common cases. KFR also
provides variable templates such as [[`csize`:nosig]], [[`cint`:nosig]], and
[[`cbool`:nosig]] so that a constant can be passed without spelling its type.

```c++
#include <kfr/meta.hpp>

using namespace kfr;

constexpr auto lanes = csize<8>;
static_assert(lanes.value == 8);
static_assert(lanes() == 8);
static_assert(val_of(lanes) == 8);
static_assert(KFR_CVAL(lanes) == 8);
```

[[`val_of`:nosig]] works for either a compile-time wrapper or an ordinary
runtime value, which is useful in code that accepts both. [[`is_constant_val`:nosig]]
and [[`is_val_t`:nosig]] distinguish a [[`cval_t<T, val>`:nosig]] from a
regular value when a template needs separate paths.

The important distinction is that `csize<4>` and `csize<8>` have different
types, so a callable receiving one can use its value directly as a template
argument (e.g. `[]<size_t N>(csize_t<N>) { float samples[N]{}; ... }`).

## Compile-Time Value Lists

[[`cvals_t`:nosig]] stores a homogeneous sequence in its type.
[[`cints_t`:nosig]], [[`cuints_t`:nosig]], [[`cbools_t`:nosig]],
[[`csizes_t`:nosig]], and [[`elements_t`:nosig]] are concise aliases. The
matching variable templates—[[`cints`:nosig]], [[`cuints`:nosig]],
[[`cbools`:nosig]], [[`csizes`:nosig]], and [[`elements`:nosig]]—construct
instances of those types.

Lists support indexed access through a [[`csize_t`:nosig]], slicing with a
second index list, and compile-time reductions. [[`csum`:nosig]] returns zero
for an empty list; [[`cprod`:nosig]] returns one. [[`cminof`:nosig]] and
[[`cmaxof`:nosig]] return the corresponding `std::numeric_limits<T>` extreme
for an empty list, so they are most useful when a list is known to be nonempty.

```c++
using namespace kfr;

constexpr auto strides = csizes<1, 2, 4, 8>;

static_assert(decltype(strides)::size() == 4);
static_assert(strides[csize<2>] == 4);
static_assert(csum(strides) == 15);
static_assert(cprod(strides) == 64);
static_assert(cfind(strides, size_t(4)) == 2);

// Pick positions 3 and 1, producing csizes_t<8, 2>.
constexpr auto reordered = strides[csizes<3, 1>];
```

[[`select(cvals_t<bool, flags...>, cvals_t<T, values1...>, cvals_t<T, values2...>)`:nosig]] chooses between two equal-length lists element by element,
and [[`cfilter`:nosig]] removes positions whose corresponding boolean flag is
false. [[`cconcat`:nosig]] and [[`concat_lists`:nosig]] concatenate value lists
or type lists. The arithmetic, comparison, logical, and bitwise operators are
also defined element-wise for compatible value lists and individual
[[`cval_t<T, val>`:nosig]] objects.

```c++
constexpr auto enabled = cbools<true, false, true, false>;
constexpr auto selected = cfilter(cints<10, 20, 30, 40>, enabled);
constexpr auto fallback = cints<1, 1, 1, 1>;
constexpr auto mixed = select(enabled, cints<5, 6, 7, 8>, fallback);

static_assert(csum(selected) == 40); // 10 + 30
static_assert(csum(mixed) == 14);    // 5 + 1 + 7 + 1
```

All operand packs in an element-wise operation must have matching lengths.
Mismatches are template errors, rather than runtime errors.

### Sequences and Expanded Indices

[[`cvalseq_t`:nosig]] creates an arithmetic sequence at compile time. Its
specializations [[`csizeseq_t`:nosig]] and [[`indicesfor_t`:nosig]] cover index
lists. The variable templates [[`cvalseq`:nosig]], [[`csizeseq`:nosig]],
[[`cintseq`:nosig]], [[`cuintseq`:nosig]], and [[`indicesfor`:nosig]] provide
instances. For example, `csizeseq<4, 3>` denotes `{ 3, 4, 5, 6 }`.

[[`scale`:nosig]] expands every input index into a contiguous group. It is
particularly handy for building the lane indices of interleaved data:

```c++
constexpr auto channels = csizes<1, 3>;
constexpr auto samples = scale<2>(channels);

// samples has type csizes_t<2, 3, 6, 7>.
static_assert(samples[csize<0>] == 2);
static_assert(samples[csize<3>] == 7);
```

## Compile-Time Type Lists

[[`ctype_t<T>`:nosig]] carries a type as an object. Recover the wrapped type
with [[`type_of`:nosig]]. [[`ctypes_t`:nosig]] is the equivalent list of
types; it exposes `size()`, its `nth<I>` type alias, and a `get` function. The
[[`ctype`:nosig]] and [[`ctypes`:nosig]] variable templates make these wrappers
convenient at call sites.

```c++
|||#include <kfr/base.hpp>
using namespace kfr;

using sample_types = ctypes_t<i16, f32, f64>;
static_assert(sample_types::size() == 3);
static_assert(std::same_as<sample_types::nth<1>, f32>);

|||TEST_CASE("metaprogramming.md/ctypes cforeach")
|||{
cforeach(ctypes<i16, f32, f64>, [](auto type_tag)
{
	using T = type_of<decltype(type_tag)>;
	static_assert(numeric<T>);
	// Instantiate or register code specialized for T here.
});
|||}
```

[[`function_arguments`:nosig]] and [[`function_result`:nosig]] extract a
non-overloaded functor's argument list and return type. They are intended for
simple concrete lambdas and function objects; a generic or overloaded
`operator()` cannot be uniquely inspected.

## Enumerating at Compile Time

[[`cforeach`:nosig]] invokes a callable once per compile-time value in a
[[`cvals_t`:nosig]] list, passing a [[`cval_t<T, val>`:nosig]].
It can also enumerate a [[`ctypes_t`:nosig]] list, ordinary iterable
ranges, and Cartesian products of two to four lists.

Use [[`cfor`:nosig]] for an explicitly bounded compile-time value range. Both
forms make the current value suitable for template arguments:

```c++
using namespace kfr;

template <size_t N>
constexpr size_t square()
{
	return N * N;
}

constexpr size_t sum_of_squares()
{
	size_t total = 0;
	cfor(csize<0>, csize<4>, [&](auto i)
	{
		total += square<val_of(i)>();
	});
	return total;
}

static_assert(sum_of_squares() == 14);
```

The [[`KFR_FOR`]] macro is a more compact spelling for the same loop
(`KFR_FOR(i, 0, 4) { total += i * i; };`). It introduces the loop variable as
a non-type template parameter, not an ordinary mutable variable, and the
trailing semicolon is required because the macro expands to an assignment
expression.

[[`KFR_FORC`]] is the conditional variant. Its body must return a value
convertible to `bool`; iteration stops after the first false result. Use either
form only for small, fixed bounds: unrolling a large loop increases compile
time and generated code size.

## Compile-Time Decisions from Runtime Values

[[`cswitch`:nosig]] bridges a finite runtime choice to a compile-time value.
It compares a runtime value against a constant list, calls the matching handler
with the matching [[`cval_t<T, val>`:nosig]], and invokes the fallback when
there is no match. The handler can then select a template specialization.

```c++
using namespace kfr;

template <int Channels>
void mix_channels(float* output, const float* inputs[], size_t length)
{ }

void mix(float* output, const float* inputs[], int channels, size_t length)
{
	cswitch(cints<1, 2, 6>, channels,
			[&](auto count)
			{
				mix_channels<val_of(count)>(output, inputs, length);
			},
			[]
			{
				// Handle an unsupported layout.
			});
}
```

The overload with a custom comparator supports non-default matching rules.
[[`cfind`:nosig]] instead returns the position of a runtime value in a constant
list, or `size_t(-1)` when it is absent.

[[`cmatch`:nosig]] dispatches an object to the first callable whose first
parameter type matches the object's decayed type; the final callable is the
fallback. This is a lightweight visitor-style helper for a known set of
concrete types. It is not a replacement for virtual dispatch when the type set
is open-ended.

## Building Index Tables

[[`map_indices_t`:nosig]] builds a [[`csizes_t`:nosig]] by applying one or more
`constexpr` index transforms to each input index in `[0, size)`. The functions
are composed right to left: the last listed transform runs first. The
[[`map_indices`:nosig]] function returns an instance when an object rather than
a type is required.

```c++
using namespace kfr;

constexpr size_t swap_adjacent(size_t index)
{
	return index ^ 1;
}

using swapped_pairs = map_indices_t<4, swap_adjacent>;

static_assert(swapped_pairs::get(csize<0>) == 1);
static_assert(swapped_pairs::get(csize<1>) == 0);
static_assert(swapped_pairs::get(csize<2>) == 3);
static_assert(swapped_pairs::get(csize<3>) == 2);
```

Transforms must be usable in constant evaluation and have the effective shape
`size_t(size_t)`. This facility is designed for tables such as SIMD shuffles and
permutations, where putting the complete index mapping in the type lets later
code select an optimized specialization.

## Further Building Blocks

Several smaller utilities support custom generic code:

- [[`cif`:nosig]] chooses one callable at compile time from a
  [[`cbool_t`:nosig]].
- [[`overload_priority`:nosig]], [[`overload_auto`:nosig]], and
  [[`overload_generic`:nosig]] form an inheritance-based priority ladder for
  SFINAE or constrained overload fallbacks.
- [[`typeindex`:nosig]] returns the zero-based position of a type in a type
  pack. Asking for a missing type is a compile-time error.
- [[`bind_func`:nosig]] creates a no-argument thunk and evaluates any bound
  argument that is itself callable. [[`pass_through`:nosig]], [[`noop`:nosig]],
  [[`get_first`:nosig]], [[`get_second`:nosig]], [[`get_third`:nosig]], and
  [[`returns`:nosig]] are small helpers for such composition.
- [[`is_even`:nosig]], [[`is_odd`:nosig]], [[`is_poweroftwo`:nosig]],
  [[`ilog2`:nosig]], [[`next_poweroftwo`:nosig]], and [[`prev_poweroftwo`:nosig]]
  provide common integer facts usable in constant expressions.

For routine application code, prefer ordinary C++ templates, `constexpr`
functions, standard algorithms, and KFR's high-level expressions. Use these
utilities when the value or type must deliberately become part of the
instantiated implementation.

## See Also

- [Numeric, Compound, and Complex Types](types.md)
- [SIMD vectors](../simd/vec.md)
- [Expressions](../../expressions/expressions.md)
