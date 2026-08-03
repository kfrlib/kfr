/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  KFR is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with KFR.

  If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
  Buying a commercial license is mandatory as soon as you develop commercial activities without
  disclosing the source code of your own applications.
  See https://www.kfrlib.com for details.
 */
#pragma once

#include "../kfr.h"

#include "impl/intrinsics.h"
#include "impl/specialconstants.hpp"

#include <climits>

#include <cmath>
#include <limits>
#include <random>

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wignored-qualifiers")

#ifdef KFR_TESTING
#include "../meta/function.hpp"
#include "../test/test.hpp"
#endif

#include "../meta.hpp"
#include "../meta/numeric.hpp"

namespace kfr
{

/**
 * @brief Decays to the common type of the supplied arguments after decay.
 * @tparam T... Argument types.
 */
template <typename... T>
using decay_common = std::decay_t<std::common_type_t<T...>>;

/**
 * @brief Helper that maps a common-type result onto a single-argument template.
 *
 * If the common type @p CT exposes a nested `type` member, the specialization
 * exposes `type` as `Tpl<typename CT::type>`.
 * @tparam CT Common type carrier.
 * @tparam Tpl Unary template to wrap the resolved type with.
 */
template <typename CT, template <typename T> typename Tpl>
struct construct_common_type
{
};
template <typename CT, template <typename T> typename Tpl>
    requires requires { typename CT::type; }
struct construct_common_type<CT, Tpl>
{
    using type = Tpl<typename CT::type>;
};

/// @brief Type list of all signed integer element types.
constexpr ctypes_t<i8, i16, i32, i64> signed_types{};
/// @brief Type list of all unsigned integer element types.
constexpr ctypes_t<u8, u16, u32, u64> unsigned_types{};
/// @brief Type list of all integer element types (signed and unsigned).
constexpr ctypes_t<i8, i16, i32, i64, u8, u16, u32, u64> integer_types{};
/// @brief Type list of all floating-point element types supported by the target.
constexpr ctypes_t<f32
#ifdef KFR_NATIVE_F64
                   ,
                   f64
#endif
                   >
    float_types{};
/// @brief Type list of all numeric element types (integer and floating-point).
constexpr ctypes_t<i8, i16, i32, i64, u8, u16, u32, u64, f32
#ifdef KFR_NATIVE_F64
                   ,
                   f64
#endif
                   >
    numeric_types{};

/// @brief Set of vector sizes used by the test harness.
constexpr csizes_t<1, 2, 3, 4, 8, 16, 32, 64> test_vector_sizes{};

#ifdef KFR_ARCH_AVX512
/// @brief Maximum vector size (in elements) exercised by the test suite for the current architecture.
constexpr size_t max_test_size = 128;
#elif defined KFR_ARCH_AVX
/// @brief Maximum vector size (in elements) exercised by the test suite for the current architecture.
constexpr size_t max_test_size = 64;
#else
/// @brief Maximum vector size (in elements) exercised by the test suite for the current architecture.
constexpr size_t max_test_size = 32;
#endif

/**
 * @brief Builds a type list of vector instantiations of @p vec_tpl for element
 *        type @p T over the test vector sizes.
 * @tparam vec_tpl Template taking an element type and a size.
 * @tparam T       Element type.
 * @tparam sizes    Compile-time list of vector sizes to instantiate.
 */
template <template <typename, size_t> class vec_tpl, typename T,
          typename sizes =
#ifdef KFR_EXTENDED_TESTS
              cfilter_t<decltype(test_vector_sizes),
                        decltype(test_vector_sizes <= csize<max_test_size / sizeof(T)>)>
#else
              csizes_t<1, 2>
#endif
          >
struct vector_types_for_size_t_impl;

/**
 * @brief Specialization that expands the size list into a `ctypes_t` of vector types.
 */
template <template <typename, size_t> class vec_tpl, typename T, size_t... sizes>
struct vector_types_for_size_t_impl<vec_tpl, T, csizes_t<sizes...>>
{
    using type = ctypes_t<vec_tpl<T, sizes>...>;
};

/// @brief Convenience alias for the type list produced by vector_types_for_size_t_impl.
template <template <typename, size_t> class vec_tpl, typename T>
using vector_types_for_size_t = typename vector_types_for_size_t_impl<vec_tpl, T>::type;

/// @brief Type list of signed-integer vector instantiations for the test sizes.
template <template <typename, size_t> class vec_tpl>
using signed_vector_types_t =
    concat_lists<vector_types_for_size_t<vec_tpl, i8>, vector_types_for_size_t<vec_tpl, i16>,
                 vector_types_for_size_t<vec_tpl, i32>, vector_types_for_size_t<vec_tpl, i64>>;

/// @brief Instance of signed_vector_types_t for use as a value parameter pack.
template <template <typename, size_t> class vec_tpl>
constexpr signed_vector_types_t<vec_tpl> signed_vector_types{};

/// @brief Type list of unsigned-integer vector instantiations for the test sizes.
template <template <typename, size_t> class vec_tpl>
using unsigned_vector_types_t =
    concat_lists<vector_types_for_size_t<vec_tpl, u8>, vector_types_for_size_t<vec_tpl, u16>,
                 vector_types_for_size_t<vec_tpl, u32>, vector_types_for_size_t<vec_tpl, u64>>;

/// @brief Instance of unsigned_vector_types_t for use as a value parameter pack.
template <template <typename, size_t> class vec_tpl>
constexpr unsigned_vector_types_t<vec_tpl> unsigned_vector_types{};

/// @brief Type list of all integer vector instantiations for the test sizes.
template <template <typename, size_t> class vec_tpl>
using integer_vector_types_t = concat_lists<signed_vector_types_t<vec_tpl>, unsigned_vector_types_t<vec_tpl>>;

/// @brief Instance of integer_vector_types_t for use as a value parameter pack.
template <template <typename, size_t> class vec_tpl>
constexpr integer_vector_types_t<vec_tpl> integer_vector_types{};

/// @brief Type list of floating-point vector instantiations for the test sizes.
template <template <typename, size_t> class vec_tpl>
using float_vector_types_t = concat_lists<vector_types_for_size_t<vec_tpl, f32>
#ifdef KFR_NATIVE_F64
                                          ,
                                          vector_types_for_size_t<vec_tpl, f64>
#endif
                                          >;

/// @brief Instance of float_vector_types_t for use as a value parameter pack.
template <template <typename, size_t> class vec_tpl>
constexpr float_vector_types_t<vec_tpl> float_vector_types{};

/// @brief Instance combining integer and floating-point vector types for use as a value parameter pack.
template <template <typename, size_t> class vec_tpl>
constexpr concat_lists<integer_vector_types_t<vec_tpl>, float_vector_types_t<vec_tpl>> numeric_vector_types{};

/**
 * @brief Unsigned 24-bit integer stored in three bytes (little-endian).
 */
struct u24
{
    /// @brief Raw byte storage.
    u8 raw[3];
};

/**
 * @brief Signed 24-bit integer stored in three bytes (little-endian).
 */
struct i24
{
    /// @brief Raw byte storage.
    u8 raw[3];

    /// @brief Default constructor leaving the value uninitialized.
    i24() noexcept {}

    /**
     * @brief Construct from a 32-bit signed integer, truncating to 24 bits.
     */
    i24(i32 x) noexcept
    {
        raw[0] = x & 0xFF;
        raw[1] = (x >> 8) & 0xFF;
        raw[2] = (x >> 16) & 0xFF;
    }

    /**
     * @brief Reconstructs the signed 32-bit value, sign-extending from bit 23.
     * @return The value as a signed 32-bit integer.
     */
    i32 as_int() const noexcept
    {
        return static_cast<i32>(raw[0]) | static_cast<i32>(raw[1] << 8) |
               (static_cast<i32>(raw[2] << 24) >> 8);
    }

    /// @brief Implicit conversion to `int` via as_int().
    operator int() const noexcept { return as_int(); }
};

/**
 * @brief 16-bit half-precision floating-point value stored as raw bits.
 */
struct f16
{
    /// @brief Raw bit representation of the half-precision value.
    u16 raw;
};

/**
 * @brief Holds an unsigned integer wide enough to store @p bits bits.
 * @tparam bits Number of bits required.
 */
template <size_t bits>
struct bitmask
{
    /// @brief The unsigned integer type selected for the requested width.
    using type = std::conditional_t<
        (bits > 32), uint64_t,
        std::conditional_t<(bits > 16), uint32_t, std::conditional_t<(bits > 8), uint16_t, uint8_t>>>;

    /// @brief Construct from a value of the underlying type.
    bitmask(type val) : value(val) {}

    /// @brief The stored bit value.
    type value;
};

/**
 * @brief Returns all-ones or all-zeros of type @p T depending on @p value.
 * @tparam T Target element type.
 * @param value When true, all bits are set; otherwise all bits are cleared.
 * @return The mask value.
 */
template <typename T>
constexpr inline T maskbits(bool value)
{
    return value ? special_constants<T>::allones() : special_constants<T>::allzeros();
}
/**
 * @brief Interprets @p value as a signed integer and tests the sign bit.
 * @tparam T Source element type.
 * @param value Mask value to test.
 * @return True when the most significant bit is set.
 */
template <typename T>
constexpr inline bool from_maskbits(T value)
{
    return bitcast_anything<itype<T>>(value) < 0;
}

/**
 * @brief Boolean value stored as a mask of type @p T (all-ones for true, all-zeros for false).
 *
 * The stored mask is interpreted by examining the sign bit of the corresponding
 * signed integer type, so any value with the high bit set reads as true.
 * @tparam T Underlying element type used to hold the mask bits.
 */
template <typename T>
struct bit
{
    /// @brief The raw mask value.
    T value;

    /// @brief Default constructor leaving the value uninitialized.
    bit() noexcept = default;

    /// @brief Construct from a bool, storing all-ones or all-zeros.
    constexpr bit(bool value) noexcept : value(maskbits<T>(value)) {}

    /**
     * @brief Convert a bit of another element type to this one.
     */
    template <typename U>
    constexpr bit(const bit<U>& value) noexcept : value(value.operator bool())
    {
    }

    /// @brief Converts to bool by testing the sign bit of the stored mask.
    constexpr operator bool() const noexcept { return bitcast_anything<itype<T>>(value) < 0; }

    constexpr bit(T value) noexcept       = delete;
    constexpr operator T() const noexcept = delete;

    /// @brief Equality comparison between two bit values.
    constexpr bool operator==(const bit& other) const noexcept
    {
        return operator bool() == other.operator bool();
    }
    /// @brief Inequality comparison between two bit values.
    constexpr bool operator!=(const bit& other) const noexcept { return !operator==(other); }
    /// @brief Equality comparison against a plain bool.
    constexpr bool operator==(bool other) const noexcept { return operator bool() == other; }
    /// @brief Inequality comparison against a plain bool.
    constexpr bool operator!=(bool other) const noexcept { return !operator==(other); }
};

/**
 * @brief Special scalar constants for `bit<T>`.
 */
template <typename T>
struct special_scalar_constants<bit<T>>
{
    /// @brief Mask with only the most significant bit set, expressed as a bit.
    constexpr static bit<T> highbitmask() { return true; }
    /// @brief All bits set (true).
    constexpr static bit<T> allones() noexcept { return true; }
    /// @brief All bits cleared (false).
    constexpr static bit<T> allzeros() { return false; }
    /// @brief Inverse of highbitmask, i.e. all bits except the high bit set.
    constexpr static bit<T> invhighbitmask() { return false; }
};

namespace internal_generic
{
template <typename T>
struct unwrap_bit_impl
{
    using type = T;
};
template <typename T>
struct unwrap_bit_impl<bit<T>>
{
    using type = T;
};

} // namespace internal_generic

template <typename T>
using unwrap_bit = typename internal_generic::unwrap_bit_impl<T>::type;

/// @brief True when @p T is a `bit<U>` specialization.
template <typename T>
constexpr inline bool is_bit = false;
/// @brief Specialization marking `bit<T>` as a bit type.
template <typename T>
constexpr inline bool is_bit<bit<T>> = true;

/**
 * @brief Returns the underlying mask value of a bit, or the value itself for non-bit types.
 * @tparam T Argument type.
 * @param value Value to unwrap.
 * @return For `bit<T>` the stored mask; otherwise @p value unchanged.
 */
template <typename T>
KFR_INTRINSIC T unwrap_bit_value(const T& value)
{
    return value;
}
/**
 * @brief Overload unwrapping a `bit<T>` to its underlying mask value.
 */
template <typename T>
KFR_INTRINSIC T unwrap_bit_value(const bit<T>& value)
{
    return value.value;
}

/**
 * @brief Wraps a raw mask value into a `bit<T>`.
 * @tparam T Must be a `bit<U>` type.
 * @param value Raw mask value to wrap.
 * @return A `bit` whose stored mask is @p value.
 */
template <typename T>
    requires(is_bit<T>)
KFR_INTRINSIC T wrap_bit_value(const unwrap_bit<T>& value)
{
    T result;
    result.value = value;
    return result;
}

/**
 * @brief Pass-through overload of wrap_bit_value for non-bit types.
 */
template <typename T>
    requires(!is_bit<T>)
KFR_INTRINSIC T wrap_bit_value(const T& value)
{
    return value;
}

namespace fn_generic
{
///@copybrief kfr::pass_through
using pass_through = kfr::fn_pass_through;

///@copybrief kfr::noop
using noop = kfr::fn_noop;

///@copybrief kfr::get_first
using get_first = kfr::fn_get_first;

///@copybrief kfr::get_second
using get_second = kfr::fn_get_second;

///@copybrief kfr::get_third
using get_third = kfr::fn_get_third;

///@copybrief kfr::returns
template <typename T>
using returns = kfr::fn_returns<T>;
} // namespace fn_generic

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wattributes")

/**
 * @brief Wrapper exposing a member of type @p T with the requested alignment.
 *
 * The aligned specialization is used to obtain a pointer with guaranteed alignment
 * for SIMD load/store operations; the unaligned specialization is packed.
 * @tparam T Member type.
 * @tparam A When true the member is over-aligned; when false the struct is packed.
 */
template <typename T, bool A>
struct struct_with_alignment
{
    /// @brief Pointer to this wrapper type.
    using pointer = struct_with_alignment*;
    /// @brief Const pointer to this wrapper type.
    using const_pointer = const struct_with_alignment*;
    /// @brief The wrapped value.
    T value;
    /// @brief Assigns a new value to the wrapped member.
    KFR_MEM_INTRINSIC void operator=(T value) { this->value = value; }
};

/**
 * @brief Packed, unaligned specialization of struct_with_alignment.
 */
template <typename T>
struct struct_with_alignment<T, false>
{
    /// @brief Pointer to this wrapper type.
    using pointer = struct_with_alignment*;
    /// @brief Const pointer to this wrapper type.
    using const_pointer = const struct_with_alignment*;
    /// @brief The wrapped value.
    T value;
    /// @brief Assigns a new value to the wrapped member.
    KFR_MEM_INTRINSIC void operator=(T value) { this->value = value; }
}
#ifdef KFR_GNU_ATTRIBUTES
__attribute__((__packed__, __may_alias__)) //
#endif
;

KFR_PRAGMA_GNU(GCC diagnostic pop)

/// @brief Fills a value with zeros
template <typename T1>
KFR_INTRINSIC void zeroize(T1& value)
{
    builtin_memset(static_cast<void*>(builtin_addressof(value)), 0, sizeof(T1));
}

/**
 * @brief Used to determine the initial value for reduce functions.
 *
 * Specializations provide the identity element for a given type @p T.
 * @tparam T Element type.
 */
template <typename T>
struct initialvalue
{
};

/// @brief True when @p T is one of the scalar types usable as a SIMD element.
template <typename T>
constexpr inline bool is_simd_type =
    std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, signed char> ||
    std::is_same_v<T, unsigned char> || std::is_same_v<T, short> || std::is_same_v<T, unsigned short> ||
    std::is_same_v<T, int> || std::is_same_v<T, unsigned int> || std::is_same_v<T, long> ||
    std::is_same_v<T, unsigned long> || std::is_same_v<T, long long> || std::is_same_v<T, unsigned long long>;

/// @brief True when @p T is a floating-point SIMD element type.
template <typename T>
constexpr inline bool is_simd_float_type = std::is_same_v<T, float> || std::is_same_v<T, double>;

/// @brief True when @p T is an integer SIMD element type.
template <typename T>
constexpr inline bool is_simd_int_type =
    std::is_same_v<T, signed char> || std::is_same_v<T, unsigned char> || std::is_same_v<T, short> ||
    std::is_same_v<T, unsigned short> || std::is_same_v<T, int> || std::is_same_v<T, unsigned int> ||
    std::is_same_v<T, long> || std::is_same_v<T, unsigned long> || std::is_same_v<T, long long> ||
    std::is_same_v<T, unsigned long long>;

/// @brief Propagates is_simd_type through `bit<T>`.
template <typename T>
constexpr inline bool is_simd_type<bit<T>> = is_simd_type<T>;
/// @brief Propagates is_simd_float_type through `bit<T>`.
template <typename T>
constexpr inline bool is_simd_float_type<bit<T>> = is_simd_float_type<T>;
/// @brief Propagates is_simd_int_type through `bit<T>`.
template <typename T>
constexpr inline bool is_simd_int_type<bit<T>> = is_simd_int_type<T>;

/// @brief Concept satisfied by scalar types usable as SIMD elements.
template <typename T>
concept simd_compat = is_simd_type<T>;

/**
 * @brief Describes the shape of a SIMD vector of @p N elements of type @p T.
 *
 * Provides the element type, the element count and the total scalar count,
 * accounting for compound element types.
 * @tparam T Element type.
 * @tparam N Number of elements.
 */
template <typename T, size_t N>
struct vec_shape
{
    /// @brief The element type stored in the vector.
    using value_type = T;
    /// @brief Number of elements in the vector.
    constexpr static size_t size() noexcept { return N; }
    /// @brief Default constructor.
    constexpr vec_shape() noexcept = default;

    /// @brief Scalar component type of the element.
    using scalar_type = subtype<T>;
    /// @brief Total number of scalar components, including compound element widths.
    constexpr static size_t scalar_size() noexcept { return N * compound_type_traits<T>::width; }
};

/// @brief Sentinel index value meaning "no index".
constexpr size_t index_undefined = static_cast<size_t>(-1);

/// @brief Tag type selecting zero-initialization.
struct czeros_t
{
};
/// @brief Tag type selecting all-ones initialization.
struct cones_t
{
};
/// @brief Instance of czeros_t.
constexpr czeros_t czeros{};
/// @brief Instance of cones_t.
constexpr cones_t cones{};

/// @brief Compile-time boolean tag selecting aligned access.
using caligned_t = cbool_t<true>;
/// @brief Compile-time boolean tag selecting unaligned access.
using cunaligned_t = cbool_t<false>;

/// @brief Instance of caligned_t.
constexpr caligned_t caligned{};
/// @brief Instance of cunaligned_t.
constexpr cunaligned_t cunaligned{};

} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)
