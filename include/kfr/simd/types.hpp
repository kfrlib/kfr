/** @addtogroup types
 *  @{
 */
/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
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

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wignored-qualifiers")

#ifdef KFR_TESTING
#include "../cometa/function.hpp"
#include "../testo/testo.hpp"
#endif

#include "../cometa.hpp"
#include "../cometa/numeric.hpp"

namespace kfr
{

// Include all from CoMeta library
using namespace cometa;

using cometa::identity;

using cometa::fbase;
using cometa::fmax;

template <typename... T>
using decay_common = std::decay_t<std::common_type_t<T...>>;

template <typename CT, template <typename T> typename Tpl, typename = void>
struct construct_common_type
{
};
template <typename CT, template <typename T> typename Tpl>
struct construct_common_type<CT, Tpl, std::void_t<typename CT::type>>
{
    using type = Tpl<typename CT::type>;
};

constexpr ctypes_t<i8, i16, i32, i64> signed_types{};
constexpr ctypes_t<u8, u16, u32, u64> unsigned_types{};
constexpr ctypes_t<i8, i16, i32, i64, u8, u16, u32, u64> integer_types{};
constexpr ctypes_t<f32
#ifdef CMT_NATIVE_F64
                   ,
                   f64
#endif
                   >
    float_types{};
constexpr ctypes_t<i8, i16, i32, i64, u8, u16, u32, u64, f32
#ifdef CMT_NATIVE_F64
                   ,
                   f64
#endif
                   >
    numeric_types{};

constexpr csizes_t<1, 2, 3, 4, 8, 16, 32, 64> test_vector_sizes{};

#ifdef CMT_ARCH_AVX512
constexpr size_t max_test_size = 128;
#elif defined CMT_ARCH_AVX
constexpr size_t max_test_size = 64;
#else
constexpr size_t max_test_size = 32;
#endif

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

template <template <typename, size_t> class vec_tpl, typename T, size_t... sizes>
struct vector_types_for_size_t_impl<vec_tpl, T, csizes_t<sizes...>>
{
    using type = ctypes_t<vec_tpl<T, sizes>...>;
};

template <template <typename, size_t> class vec_tpl, typename T>
using vector_types_for_size_t = typename vector_types_for_size_t_impl<vec_tpl, T>::type;

template <template <typename, size_t> class vec_tpl>
using signed_vector_types_t =
    concat_lists<vector_types_for_size_t<vec_tpl, i8>, vector_types_for_size_t<vec_tpl, i16>,
                 vector_types_for_size_t<vec_tpl, i32>, vector_types_for_size_t<vec_tpl, i64>>;

template <template <typename, size_t> class vec_tpl>
constexpr signed_vector_types_t<vec_tpl> signed_vector_types{};

template <template <typename, size_t> class vec_tpl>
using unsigned_vector_types_t =
    concat_lists<vector_types_for_size_t<vec_tpl, u8>, vector_types_for_size_t<vec_tpl, u16>,
                 vector_types_for_size_t<vec_tpl, u32>, vector_types_for_size_t<vec_tpl, u64>>;

template <template <typename, size_t> class vec_tpl>
constexpr unsigned_vector_types_t<vec_tpl> unsigned_vector_types{};

template <template <typename, size_t> class vec_tpl>
using integer_vector_types_t = concat_lists<signed_vector_types_t<vec_tpl>, unsigned_vector_types_t<vec_tpl>>;

template <template <typename, size_t> class vec_tpl>
constexpr integer_vector_types_t<vec_tpl> integer_vector_types{};

template <template <typename, size_t> class vec_tpl>
using float_vector_types_t = concat_lists<vector_types_for_size_t<vec_tpl, f32>
#ifdef CMT_NATIVE_F64
                                          ,
                                          vector_types_for_size_t<vec_tpl, f64>
#endif
                                          >;

template <template <typename, size_t> class vec_tpl>
constexpr float_vector_types_t<vec_tpl> float_vector_types{};

template <template <typename, size_t> class vec_tpl>
constexpr concat_lists<integer_vector_types_t<vec_tpl>, float_vector_types_t<vec_tpl>> numeric_vector_types{};

struct u24
{
    u8 raw[3];
};

struct i24
{
    u8 raw[3];

    constexpr i24(i32 x) CMT_NOEXCEPT : raw{}
    {
        raw[0] = x & 0xFF;
        raw[1] = (x >> 8) & 0xFF;
        raw[2] = (x >> 16) & 0xFF;
    }

    constexpr i32 as_int() const CMT_NOEXCEPT
    {
        return static_cast<i32>(raw[0]) | static_cast<i32>(raw[1] << 8) |
               (static_cast<i32>(raw[2] << 24) >> 8);
    }

    operator int() const CMT_NOEXCEPT { return as_int(); }
};

struct f16
{
    u16 raw;
};

template <size_t bits>
struct bitmask
{
    using type = std::conditional_t<
        (bits > 32), uint64_t,
        std::conditional_t<(bits > 16), uint32_t, std::conditional_t<(bits > 8), uint16_t, uint8_t>>>;

    bitmask(type val) : value(val) {}

    type value;
};

template <typename T>
constexpr inline T maskbits(bool value)
{
    return value ? special_constants<T>::allones() : special_constants<T>::allzeros();
}
template <typename T>
constexpr inline bool from_maskbits(T value)
{
    return bitcast_anything<itype<T>>(value) < 0;
}

template <typename T>
struct bit
{
    T value;
    bit() CMT_NOEXCEPT = default;

    constexpr bit(bool value) CMT_NOEXCEPT : value(maskbits<T>(value)) {}

    template <typename U>
    constexpr bit(const bit<U>& value) CMT_NOEXCEPT : value(value.operator bool())
    {
    }

    constexpr operator bool() const CMT_NOEXCEPT { return bitcast_anything<itype<T>>(value) < 0; }

    constexpr bit(T value) CMT_NOEXCEPT       = delete;
    constexpr operator T() const CMT_NOEXCEPT = delete;

    constexpr bool operator==(const bit& other) const CMT_NOEXCEPT
    {
        return operator bool() == other.operator bool();
    }
    constexpr bool operator!=(const bit& other) const CMT_NOEXCEPT { return !operator==(other); }
    constexpr bool operator==(bool other) const CMT_NOEXCEPT { return operator bool() == other; }
    constexpr bool operator!=(bool other) const CMT_NOEXCEPT { return !operator==(other); }
};

template <typename T>
struct special_scalar_constants<bit<T>>
{
    constexpr static bit<T> highbitmask() { return true; }
    constexpr static bit<T> allones() noexcept { return true; }
    constexpr static bit<T> allzeros() { return false; }
    constexpr static bit<T> invhighbitmask() { return false; }
};

namespace internal_generic
{
template <typename T>
struct unwrap_bit
{
    using type = T;
};
template <typename T>
struct unwrap_bit<bit<T>>
{
    using type = T;
};

} // namespace internal_generic

template <typename T>
using unwrap_bit = typename internal_generic::unwrap_bit<T>::type;

template <typename T>
constexpr inline bool is_bit = false;
template <typename T>
constexpr inline bool is_bit<bit<T>> = true;

template <typename T>
CMT_INTRINSIC T unwrap_bit_value(const T& value)
{
    return value;
}
template <typename T>
CMT_INTRINSIC T unwrap_bit_value(const bit<T>& value)
{
    return value.value;
}

template <typename T, KFR_ENABLE_IF(is_bit<T>)>
CMT_INTRINSIC T wrap_bit_value(const unwrap_bit<T>& value)
{
    T result;
    result.value = value;
    return result;
}

template <typename T, KFR_ENABLE_IF(!is_bit<T>)>
CMT_INTRINSIC T wrap_bit_value(const T& value)
{
    return value;
}

namespace fn_generic
{
///@copybrief cometa::pass_through
using pass_through = cometa::fn_pass_through;

///@copybrief cometa::noop
using noop = cometa::fn_noop;

///@copybrief cometa::get_first
using get_first = cometa::fn_get_first;

///@copybrief cometa::get_second
using get_second = cometa::fn_get_second;

///@copybrief cometa::get_third
using get_third = cometa::fn_get_third;

///@copybrief cometa::returns
template <typename T>
using returns = cometa::fn_returns<T>;
} // namespace fn_generic

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wattributes")

template <typename T, bool A>
struct struct_with_alignment
{
    using pointer       = struct_with_alignment*;
    using const_pointer = const struct_with_alignment*;
    T value;
    KFR_MEM_INTRINSIC void operator=(T value) { this->value = value; }
};

template <typename T>
struct struct_with_alignment<T, false>
{
    using pointer       = struct_with_alignment*;
    using const_pointer = const struct_with_alignment*;
    T value;
    KFR_MEM_INTRINSIC void operator=(T value) { this->value = value; }
}
#ifdef CMT_GNU_ATTRIBUTES
__attribute__((__packed__, __may_alias__)) //
#endif
;

CMT_PRAGMA_GNU(GCC diagnostic pop)

/// @brief Fills a value with zeros
template <typename T1>
KFR_INTRINSIC void zeroize(T1& value)
{
    builtin_memset(static_cast<void*>(builtin_addressof(value)), 0, sizeof(T1));
}

/// @brief Used to determine the initial value for reduce functions
template <typename T>
struct initialvalue
{
};

template <typename T>
constexpr inline bool is_simd_type =
    std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, signed char> ||
    std::is_same_v<T, unsigned char> || std::is_same_v<T, short> || std::is_same_v<T, unsigned short> ||
    std::is_same_v<T, int> || std::is_same_v<T, unsigned int> || std::is_same_v<T, long> ||
    std::is_same_v<T, unsigned long> || std::is_same_v<T, long long> || std::is_same_v<T, unsigned long long>;

template <typename T>
constexpr inline bool is_simd_float_type = std::is_same_v<T, float> || std::is_same_v<T, double>;

template <typename T>
constexpr inline bool is_simd_int_type =
    std::is_same_v<T, signed char> || std::is_same_v<T, unsigned char> || std::is_same_v<T, short> ||
    std::is_same_v<T, unsigned short> || std::is_same_v<T, int> || std::is_same_v<T, unsigned int> ||
    std::is_same_v<T, long> || std::is_same_v<T, unsigned long> || std::is_same_v<T, long long> ||
    std::is_same_v<T, unsigned long long>;

template <typename T>
constexpr inline bool is_simd_type<bit<T>> = is_simd_type<T>;
template <typename T>
constexpr inline bool is_simd_float_type<bit<T>> = is_simd_float_type<T>;
template <typename T>
constexpr inline bool is_simd_int_type<bit<T>> = is_simd_int_type<T>;

template <typename T, size_t N>
struct vec_shape
{
    using value_type = T;
    constexpr static size_t size() CMT_NOEXCEPT { return N; }
    constexpr vec_shape() CMT_NOEXCEPT = default;

    using scalar_type = subtype<T>;
    constexpr static size_t scalar_size() CMT_NOEXCEPT { return N * compound_type_traits<T>::width; }
};

constexpr size_t index_undefined = static_cast<size_t>(-1);

struct czeros_t
{
};
struct cones_t
{
};
constexpr czeros_t czeros{};
constexpr cones_t cones{};

using caligned_t   = cbool_t<true>;
using cunaligned_t = cbool_t<false>;

constexpr caligned_t caligned{};
constexpr cunaligned_t cunaligned{};

#ifdef CMT_INTRINSICS_IS_CONSTEXPR
#define KFR_I_CE constexpr
#else
#define KFR_I_CE
#endif

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
