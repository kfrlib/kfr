# Vector Rearrangement and Matrix-Like Lane Operations

KFR's lane operations rearrange the values already held in a [[`vec`:nosig]]. They
are the building blocks for packing data, deinterleaving channels, constructing
small matrix transforms, and FFT-style index layouts. The control information
for most operations is known at compile time, allowing KFR to select an
appropriate shuffle sequence for the target.

KFR indexes lanes in increasing memory order: lane 0 is the first stored value.
Thus the examples below describe `vec{ 1, 2, 3, 4 }` as lanes 0 through 3 in
that same order.

Include `<kfr/simd.hpp>` for the operations in this article.

## Compile-time shuffles

[[`vec<T, N>::shuffle(csizes_t<indices...>)`:nosig]] creates a new vector by selecting lanes with a compile-time
index sequence. The one-vector form selects from its source; the two-vector
form selects from the concatenation of its two equal-width sources.

```c++
#include <kfr/simd.hpp>

using namespace kfr;

|||TEST_CASE("lane_operations.md/compile-time-shuffle")
|||{
const i32x4 x{ 10, 20, 30, 40 };
const i32x4 y{ 50, 60, 70, 80 };

const auto reversed = x.shuffle(csizes<3, 2, 1, 0>);
// { 40, 30, 20, 10 }

const auto alternating = x.shuffle(y, csizes<0, 5, 2, 7>);
// { 10, 60, 30, 80 }
|||CHECK_THAT(reversed, DeepMatcher(i32x4{ 40, 30, 20, 10 }));
|||CHECK_THAT(alternating, DeepMatcher(i32x4{ 10, 60, 30, 80 }));
|||}
```

For the two-vector form, indices `0` through `N - 1` select from the first
vector, and `N` through `2N - 1` select from the second. The indices are part
of the call's type, not a run-time index array. This makes [[`shuffle(const vec<T, N> &, const vec<T, N> &, elements_t<Indices...>)`:nosig]] suitable
for fixed data layouts; use gather operations when the indices are only known
at run time.

Out-of-range compile-time shuffle indices are used internally by zero-padding
helpers. Prefer [[`padlow`:nosig]], [[`padhigh`:nosig]], [[`widen`:nosig]], or [[`narrow`:nosig]] when
that is the intended operation rather than relying on an invalid index.

[[`shufflevector`:nosig]] and [[`shufflevectors`:nosig]] provide corresponding free
functions. [[`permute`:nosig]] applies a repeating compile-time pattern to one
vector, while [[`shuffle`:nosig]] applies a repeating pattern to two sources.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/permute")
|||{
const i32x8 x{ 0, 1, 2, 3, 4, 5, 6, 7 };
const auto swap_adjacent = permute(x, elements<1, 0>);
// { 1, 0, 3, 2, 5, 4, 7, 6 }
|||CHECK_THAT(swap_adjacent, DeepMatcher(i32x8{ 1, 0, 3, 2, 5, 4, 7, 6 }));
|||}
```

[[`shufflegroups`:nosig]] and [[`permutegroups`:nosig]] apply the same idea to adjacent
groups of lanes. Use them when a group represents one compound item, such as a
complex sample stored as real/imaginary lanes.

## Joining, extracting, and splitting vectors

[[`concat`:nosig]] joins vectors in argument order. [[`concat2`:nosig]] and [[`concat4`:nosig]]
are convenience forms for two and four inputs. [[`slice(const vec<T, N> &)`:nosig]] extracts a
compile-time contiguous range; its two-vector form extracts from two
equal-width vectors treated as one concatenated sequence.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/concat-slice")
|||{
const i32x2 left{ 1, 2 };
const i32x2 right{ 3, 4 };
const auto joined = concat(left, right); // { 1, 2, 3, 4 }

const auto tail = slice<2, 2>(joined); // { 3, 4 }
const auto middle = slice<1, 2>(left, right); // { 2, 3 }
|||CHECK_THAT(joined, DeepMatcher(i32x4{ 1, 2, 3, 4 }));
|||CHECK_THAT(tail, DeepMatcher(i32x2{ 3, 4 }));
|||CHECK_THAT(middle, DeepMatcher(i32x2{ 2, 3 }));
|||}
```

[[`split`:nosig]] writes consecutive ranges into output vector references. The
output widths describe the partition, so their total width should cover the
input width you intend to split.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/split")
|||{
const i32x4 input{ 10, 20, 30, 40 };
i32x2 low;
i32x2 high;

split(input, low, high);
// low = { 10, 20 }, high = { 30, 40 }
|||CHECK_THAT(low, DeepMatcher(i32x2{ 10, 20 }));
|||CHECK_THAT(high, DeepMatcher(i32x2{ 30, 40 }));
|||}
```

The two-output convenience form is naturally used with an even lane count; the
four-output form is naturally used with a lane count divisible by four.
[[`low`:nosig]], [[`high`:nosig]], [[`lowhalf`:nosig]], and [[`highhalf`:nosig]] cover common
extractions without declaring output variables.

## Repetition, resizing, and padding

The width-changing helpers have deliberately different behavior. Choose the
one that describes the fill policy you need.

| Operation | Result when widening | Result when narrowing |
|-----------|----------------------|-----------------------|
| [[`repeat`:nosig]] | Repeats the whole input pattern a requested number of times. | Not applicable. |
| [[`resize`:nosig]] | Repeats the input pattern cyclically. | Keeps the leading lanes. |
| [[`extend`:nosig]] | Keeps source lanes and fills remaining lanes with zero (except a one-lane input, whose value repeats). | Keeps the leading lanes. |
| [[`padhigh`:nosig]] / [[`padlow`:nosig]] | Appends or prepends zeros, or a supplied value. | Not applicable. |
| [[`widen`:nosig]] | Appends zeros or a supplied value. | Not applicable. |
| [[`narrow`:nosig]] | Not applicable. | Keeps the leading lanes. |

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/resize-extend-pad")
|||{
const vec<i32, 3> rgb{ 1, 2, 3 };

const auto repeated = resize<8>(rgb);
// { 1, 2, 3, 1, 2, 3, 1, 2 }

const auto extended = extend<8>(rgb);
// { 1, 2, 3, 0, 0, 0, 0, 0 }

const auto padded = padhigh<2>(rgb, -1);
// { 1, 2, 3, -1, -1 }
|||CHECK_THAT(repeated, DeepMatcher(vec<i32, 8>{ 1, 2, 3, 1, 2, 3, 1, 2 }));
|||CHECK_THAT(extended, DeepMatcher(vec<i32, 8>{ 1, 2, 3, 0, 0, 0, 0, 0 }));
|||CHECK_THAT(padded, DeepMatcher(vec<i32, 5>{ 1, 2, 3, -1, -1 }));
|||}
```

Use [[`resize(const vec<T, N> &)`:nosig]] or [[`repeat(const vec<T, N> &)`:nosig]] instead of [[`extend(const vec<T, N> &)`:nosig]] when you need cyclic repetition
of a multi-lane pattern.

## Alternating lanes, duplication, swapping, and rotation

[[`even`:nosig]] and [[`odd`:nosig]] extract alternating lanes from an even-width vector.
Their optional group argument treats adjacent lanes as one unit. For example,
`odd<2>` selects the second two-lane group from each four-lane group.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/even-odd-dupeven")
|||{
const i32x8 x{ 0, 1, 2, 3, 4, 5, 6, 7 };

const auto evens = even(x);  // { 0, 2, 4, 6 }
const auto odds = odd<2>(x); // { 2, 3, 6, 7 }
const auto paired_even = dupeven(x);
// { 0, 0, 2, 2, 4, 4, 6, 6 }
|||CHECK_THAT(evens, DeepMatcher(vec<i32, 4>{ 0, 2, 4, 6 }));
|||CHECK_THAT(odds, DeepMatcher(vec<i32, 4>{ 2, 3, 6, 7 }));
|||CHECK_THAT(paired_even, DeepMatcher(i32x8{ 0, 0, 2, 2, 4, 4, 6, 6 }));
|||}
```

[[`dupeven`:nosig]] copies the first lane of every pair into the second;
[[`dupodd`:nosig]] copies the second lane into the first. [[`dup`:nosig]] duplicates every
input lane into two adjacent result lanes, while [[`duphalves`:nosig]] repeats the
whole vector as two consecutive halves. [[`duplow`:nosig]] and [[`duphigh`:nosig]] repeat
one half of an existing vector to fill its width.

[[`swap`:nosig]] reverses lanes within adjacent power-of-two-sized blocks. Its default
block size is two lanes, so it swaps each adjacent pair. Use a power-of-two
block size that divides the vector into complete blocks.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/swap")
|||{
const i32x8 x{ 0, 1, 2, 3, 4, 5, 6, 7 };
const auto swapped_pairs = swap(x);
// { 1, 0, 3, 2, 5, 4, 7, 6 }

const auto reversed_blocks = swap<4>(x);
// { 3, 2, 1, 0, 7, 6, 5, 4 }
|||CHECK_THAT(swapped_pairs, DeepMatcher(i32x8{ 1, 0, 3, 2, 5, 4, 7, 6 }));
|||CHECK_THAT(reversed_blocks, DeepMatcher(i32x8{ 3, 2, 1, 0, 7, 6, 5, 4 }));
|||}
```

[[`rotateleft`:nosig]] and [[`rotateright`:nosig]] circularly move lanes by a compile-time
amount. [[`rotatetwo`:nosig]] instead creates a shifted width-sized window across two
equal-width vectors. [[`insertlow`:nosig]] and [[`inserthigh`:nosig]] insert one scalar at
an end while discarding the lane at the other end.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/rotate-insert")
|||{
const i32x4 x{ 10, 20, 30, 40 };
const auto left = rotateleft<1>(x);  // { 20, 30, 40, 10 }
const auto right = rotateright<1>(x); // { 40, 10, 20, 30 }
const auto shifted = inserthigh(50, x); // { 20, 30, 40, 50 }
|||CHECK_THAT(left, DeepMatcher(i32x4{ 20, 30, 40, 10 }));
|||CHECK_THAT(right, DeepMatcher(i32x4{ 40, 10, 20, 30 }));
|||CHECK_THAT(shifted, DeepMatcher(i32x4{ 20, 30, 40, 50 }));
|||}
```

Rotation amounts must be compile-time values in the valid lane range. Use
[[`reverse(const vec<T, N> &)`:nosig]] to reverse lane order. With a group argument, it reverses groups
while preserving the order inside each group.

## Interleaving, zipping, and matrix columns

[[`interleave(const vec<T, N> &, const vec<T, N> &)`:nosig]] alternates lanes from two equal-width vectors. It is a direct
way to create an interleaved stereo or complex layout. Its group argument
interleaves groups rather than single lanes.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/interleave")
|||{
const i32x4 left{ 1, 2, 3, 4 };
const i32x4 right{ 10, 20, 30, 40 };

const auto interleaved = interleave(left, right);
// { 1, 10, 2, 20, 3, 30, 4, 40 }
|||CHECK_THAT(interleaved, DeepMatcher(vec<i32, 8>{ 1, 10, 2, 20, 3, 30, 4, 40 }));
|||}
```

[[`interleavehalves`:nosig]] interleaves the low and high halves of one vector, and
[[`splitpairs`:nosig]] performs the corresponding pair separation. [[`zip`:nosig]] turns
multiple vectors into a vector of small row vectors; the number of inputs must
be a power of two. For a normal row-wise matrix construction, use equal-width
input vectors.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/zip-column")
|||{
const i32x4 left{ 1, 2, 3, 4 };
const i32x4 right{ 10, 20, 30, 40 };

const auto rows = zip(left, right);
// { { 1, 10 }, { 2, 20 }, { 3, 30 }, { 4, 40 } }

const auto second_column = column<1>(rows);
// { 10, 20, 30, 40 }
|||CHECK_THAT(rows, DeepMatcher(vec<vec<i32, 2>, 4>{ i32x2{ 1, 10 }, i32x2{ 2, 20 }, i32x2{ 3, 30 }, i32x2{ 4, 40 } }));
|||CHECK_THAT(second_column, DeepMatcher(i32x4{ 10, 20, 30, 40 }));
|||}
```

[[`column`:nosig]] extracts one compile-time column from a nested vector. KFR does
not provide a plural `columns` function; obtain each required column with its
own `column<Index>` call.

## Transposition

[[`transpose`:nosig]] treats a flat vector as a row-major matrix and produces its
transposed layout. Give the number of rows as the first template argument;
the number of columns follows from the vector size. Both dimensions must divide
the represented lane count exactly.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/transpose")
|||{
const auto matrix = enumerate<i32, 16>();
const auto transposed = transpose<4>(matrix);
// { 0, 4, 8, 12,
//   1, 5, 9, 13,
//   2, 6, 10, 14,
//   3, 7, 11, 15 }
|||CHECK_THAT(transposed, DeepMatcher(vec<i32, 16>{ 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 }));
|||}
```

The optional group argument transposes groups of adjacent lanes as units.
[[`transposeinverse`:nosig]] reverses a previous transposition with the corresponding
dimension. The nested-vector overload of [[`transpose(const vec<T, N> &)`:nosig]] handles square
`vec<vec<T, N>, N>` matrices. [[`ctranspose`:nosig]] and [[`ctransposeinverse`:nosig]] use
adjacent real/imaginary pairs as complex elements.

## Bit-index permutations and digit reversal

[[`shuffleindexbits`:nosig]] permutes a vector by permuting the bits of its lane
index. It is useful for fixed FFT-style layouts. The vector width and group
size must be powers of two, and the compile-time bit-position list must contain
exactly $\log_2(N / \mathrm{group})$ entries.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/shuffleindexbits")
|||{
const i32x4 x{ 0, 1, 2, 3 };
const auto swapped_index_bits = shuffleindexbits(x, elements<1, 0>);
// { 0, 2, 1, 3 }
|||CHECK_THAT(swapped_index_bits, DeepMatcher(i32x4{ 0, 2, 1, 3 }));
|||}
```

In the example, the two bits of each lane index are exchanged. The position at
the start of the `elements` list becomes the least significant output index
bit.

[[`bitreverse`:nosig]] reverses binary lane-index bits; [[`digitreverse4`:nosig]] reverses
base-4 index digits; [[`digitreverse`:nosig]] is the underlying radix-2 or radix-4
operation. Their optional group argument keeps each adjacent group together.

```c++
using namespace kfr;

|||TEST_CASE("lane_operations.md/digitreverse4")
|||{
const auto input = enumerate<i32, 16>();
const auto radix4_order = digitreverse4(input);
// { 0, 4, 8, 12,
//   1, 5, 9, 13,
//   2, 6, 10, 14,
//   3, 7, 11, 15 }
|||CHECK_THAT(radix4_order, DeepMatcher(vec<i32, 16>{ 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15 }));
|||}
```

Use radix 2 or radix 4 and a vector/group configuration with a complete index
space for that radix. Scalar [[`bitreverse`:nosig]] overloads are also available for
reversing the low bits of a [[`u32`:nosig]] index.

[[`bitpermute`:nosig]] is a specialized, compile-time lane-index bit-permutation
facility used by advanced transform code. Prefer [[`shuffle(const vec<T, N> &, const vec<T, N> &, elements_t<Indices...>)`:nosig]], [[`permute(const vec<T, N> &, elements_t<Indices...>)`:nosig]], or
[[`shuffleindexbits(const vec<T, N> &, elements_t<A...>)`:nosig]] unless a power-of-two FFT-style permutation is exactly what
you need.

## Choosing an operation

1. Use [[`shuffle`:nosig]] for one fixed arbitrary arrangement; use the two-vector
	form when lanes come from two sources.
2. Use [[`concat(const vec<T, Ns> &...)`:nosig]], [[`slice(const vec<T, N> &)`:nosig]], and [[`split(const vec<T, N> &, vec<T, Nout> &, Args &&...)`:nosig]] for contiguous composition
	or partitioning.
3. Use [[`resize`:nosig]], [[`repeat`:nosig]], or padding helpers only after choosing the
	correct widening policy.
4. Use [[`interleave(const vec<T, N> &, const vec<T, N> &)`:nosig]], [[`zip(const vec<T, N1> &, const vec<T, Ns> &...)`:nosig]], [[`column(const vec<vec<T, N1>, N2> &)`:nosig]], and [[`transpose(const vec<T, N> &)`:nosig]] when
	the lanes represent structured rows, columns, channels, or complex pairs.
5. Use bit-index permutations only when the layout is explicitly defined by
	index bits, such as an FFT stage.

## See Also

- [The `vec` Type](vec.md)
- [Masks, Comparisons, and Branchless Selection](masks.md)
- [Arithmetic, Bitwise Operations, and Type Conversion](arithmetic.md)
