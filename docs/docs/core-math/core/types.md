# Numeric, Compound, and Complex Types

KFR algorithms work with ordinary C++ arithmetic types, SIMD vectors, complex
values, and nested combinations of those types. This article describes the type
vocabulary and compile-time traits that make those values work together.

## Scalar Type Names

KFR provides short, fixed-width names for the standard scalar types. Prefer
them in public DSP code when the width matters, rather than relying on the
platform-dependent sizes of `long` and `int`.

| KFR name                  | C++ type                                   |
|---------------------------|--------------------------------------------|
| `b8`                      | `bool`                                     |
| `f32`, `f64`, `f80`       | `float`, `double`, `long double`           |
| `i8`, `i16`, `i32`, `i64` | Signed 8-, 16-, 32-, and 64-bit integers   |
| `u8`, `u16`, `u32`, `u64` | Unsigned 8-, 16-, 32-, and 64-bit integers |
| `imax`, `umax`            | `i64`, `u64`                               |
| `fmax`                    | Currently `f64`                            |

`fbase` is KFR's default floating-point calculation type: `double` normally,
or `float` when `KFR_BASETYPE_F32` or `KFR_NO_NATIVE_F64` is defined (KFR sets
the latter on 32-bit ARM targets without 64-bit float vector registers). Use
`fbase` for code that should follow the library's selected precision, and
`f32`/`f64` when an interface or data format needs a specific width.

`fix_type<T>` normalizes a few implementation-dependent integer spellings,
such as mapping `char` to `i8` and `long` to `i32` or `i64` by width. It's
mainly useful in generic code that needs KFR's canonical integer aliases.

### Packed Sample Types

`i24` and `u24` are three-byte, little-endian storage types for signed and
unsigned 24-bit data. `i24` can be constructed from an `i32`, which retains the
low 24 bits, and converts back to `int` with sign extension. Reach for them
when reading or writing packed audio samples and file formats, but convert to
a regular integer or floating type before doing any arithmetic — don't do DSP
work directly on the packed representation.

`f16` is a 16-bit storage representation for a half-precision floating-point
value. Its `raw` member stores the bit pattern, and it supports conversion to
and from `f32`. It does not provide half-precision arithmetic; use it for
interchange or storage, and convert it to a regular floating-point type before
performing calculations.

## Compound Values

KFR calls a value *compound* when it consists of one or more recursively
described components. This model lets generic algorithms preserve a value's
shape while operating on its scalar elements. A scalar has one component;
compound values expose an immediate component type and can themselves be
nested.

[[`compound_type_traits<T>`:nosig]] is the customization point behind this
model. Its default definition treats `T` as a scalar. Specializations describe
the component count, component types, nesting depth, rebinding rules, and—when
the value can be read element-wise—an `at` accessor.

The following standard types participate automatically:

| Type                       | Immediate components | Notes                                                                                       |
|----------------------------|----------------------|---------------------------------------------------------------------------------------------|
| Scalar `T`                 | One `T`              | The default trait behavior.                                                                 |
| `std::pair<T, T>`          | Two `T` values       | `first`, then `second`. Only homogeneous pairs have this built-in specialization.           |
| [[`complex`:nosig]]     | Two `T` values       | Real part, then imaginary part. `complex<T>` is normally KFR's alias for `std::complex<T>`. |
| [[`vec<T, N>`:nosig]]      | `N` values of `T`    | A fixed-size SIMD-capable vector; `T` may itself be compound.                               |
| [[`mat`:nosig]] | `N2` rows            | An alias for `vec<vec<T, N1>, N2>`: `N1` columns per row and `N2` rows.                     |

For example, `vec<complex<f32>, 4>` has four immediate complex components and
eight deepest scalar components. A `mat<f32, 3, 2>` is a two-row, three-column
nested vector, with six deepest scalar components.

### Inspecting and Transforming Shapes

The trait and its convenience aliases are useful when writing generic numeric
code:

- [[`widthof()`:nosig]] returns the number of immediate components.
- [[`is_compound_type`:nosig]] is `true` for a non-scalar compound value.
- [[`subtype`:nosig]] is the immediate component type.
- [[`deep_subtype`:nosig]] is the recursively deepest scalar type.
- `compound_type_traits<T>::deep_width` is the total number of deepest scalar
	components, and `depth` is the nesting depth.
- `rebind<U>` replaces the immediate component type with `U`; `deep_rebind<U>`
	preserves the complete shape while replacing its deepest scalar type.

These rules are also why a vector of complex values, a matrix, or a nested
vector can be accepted by many of the same APIs as a scalar value.

## Numeric Types and the `numeric` Concept

[[`numeric`:nosig]] accepts an arithmetic scalar other than `bool`, or a
supported compound value whose [[`deep_subtype`:nosig]] is such a scalar. It
is the constraint used by many SIMD and math functions. Thus `f32`,
`vec<i16, 8>`, `complex<f64>`, and `vec<complex<f32>, 4>` are numeric, while
`bool` is not.

The related variable templates [[`is_number`:nosig]],
[[`is_numeric`:nosig]], and [[`is_numeric_args`:nosig]] are useful in
code that needs the same test outside a `requires` clause. `is_number` tests a
single scalar type; `is_numeric` follows compound types down to their deepest
subtype; `is_numeric_args` requires every supplied type to be numeric.

> [!note]
> `i24`, `u24`, and `f16` are storage representations rather than ordinary
> integral or floating-point C++ types. They do not satisfy [[`numeric`:nosig]].

## Numeric Promotion

KFR relies on `std::common_type_t` for the result type of mixed arithmetic. In
addition to the standard scalar rules, KFR supplies `std::common_type`
specializations for its vectors and complex values. This keeps the shape of a
value while promoting its element type.

```c++
|||#include <kfr/base.hpp>
using namespace kfr;

|||TEST_CASE("types.md/common_type")
|||{
static_assert(std::same_as<std::common_type_t<i32x4, u32x4>, u32x4>);
static_assert(std::same_as<std::common_type_t<i32x4, u32x4, f64x4>, f64x4>);
static_assert(std::same_as<std::common_type_t<complex<int>, double>, complex<double>>);
|||}
```

A scalar combined with `vec<T, N>` produces a vector of the scalar common
type. Equal-width vectors likewise produce `vec<common_type, N>`. Complex
values promote their underlying real type and retain the complex shape. This is
why mixed scalar/vector and scalar/complex calls generally have the result type
you would expect without needing casts at every call site.

`decay_common<Ts...>` is KFR's convenience alias for the decayed
`std::common_type_t<Ts...>` result.

Some mathematical operations require floating-point calculation even when their
arguments are integers. The following aliases preserve the compound shape while
selecting an element type:

- [[`ftype`:nosig]], [[`itype`:nosig]], and [[`utype`:nosig]] replace
	the deepest subtype with a floating, signed integer, or unsigned integer type
	of the corresponding bit width.
- [[`flt_type`:nosig]] selects `float` for a deepest subtype no wider than
	16 bits; otherwise it selects `fbase`.
- [[`fsubtype`:nosig]], [[`isubtype`:nosig]], and
	[[`usubtype`:nosig]] apply the corresponding conversion to an immediate
	subtype.

For example, `ftype<vec<complex<i32>, 4>>` is
`vec<complex<f32>, 4>`, preserving both the vector and complex structure.

## `datatype` and Type Categories

[[`datatype`:nosig]] is a compact enumeration for describing sample/data
formats. It combines a type class with a bit count. The classes are floating
point (`f`), signed integer (`i`), unsigned integer (`u`), complex (`c`), and
boolean (`b`); `typebits_mask` and `typeclass_mask` select the corresponding
fields. Named values include `datatype::f32`, `datatype::i24`,
`datatype::u16`, `datatype::c64`, and `datatype::b8`. Bitwise `|` and `&` can
be used to compose or inspect the encoded fields.

`datatype` describes a format, not a general `T`-to-`datatype` mapping — treat
it as a runtime tag rather than a C++ type trait.

For template dispatch, [[`typeclass`:nosig]] reports whether a type's
immediate subtype is a standard floating, signed integral, or unsigned integral
type. The boolean helpers [[`is_f_class`:nosig]], [[`is_i_class`:nosig]],
and [[`is_u_class`:nosig]], plus the [[`f_class`:nosig]],
[[`i_class`:nosig]], and [[`u_class`:nosig]] concepts, expose the same
classification. [[`typebits`:nosig]] supplies the scalar bit count, compound
width, and immediate subtype.

Because `typeclass<T>` considers the *immediate* subtype, it is most useful for
scalars and vectors of scalars. Use [[`deep_subtype`:nosig]] first when a
classification must apply recursively to matrices or other nested compound
values.

## See Also

- [Basics](../../getting-started/basics.md) for `vec` and other everyday KFR
	values.
- [Expressions](../../expressions/expressions.md) for lazy composition and
	evaluation.
