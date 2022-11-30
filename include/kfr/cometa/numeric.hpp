/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

namespace cometa
{

/// @brief Short names for common types
using b8   = bool;
using f32  = float;
using f64  = double;
using i8   = int8_t;
using i16  = int16_t;
using i32  = int32_t;
using i64  = int64_t;
using u8   = uint8_t;
using u16  = uint16_t;
using u32  = uint32_t;
using u64  = uint64_t;
using umax = uint64_t;
using imax = int64_t;
using fmax = double;
using f80  = long double;

#if defined(CMT_BASETYPE_F32) || defined(CMT_NO_NATIVE_F64)
using fbase = float;
#else
using fbase = double;
#endif

namespace details
{
template <typename T>
struct fix_type_impl
{
    using type = T;
};

template <>
struct fix_type_impl<char>
{
    using type = i8;
};

template <>
struct fix_type_impl<unsigned long>
{
#if ULONG_MAX == ULLONG_MAX
    using type = u64;
#else
    using type = u32;
#endif
};

template <>
struct fix_type_impl<signed long>
{
#if LONG_MAX == LLONG_MAX
    using type = i64;
#else
    using type = i32;
#endif
};

template <>
struct fix_type_impl<unsigned long long>
{
    using type = u64;
};

template <>
struct fix_type_impl<signed long long>
{
    using type = i64;
};

} // namespace details

template <typename T>
using fix_type = typename details::fix_type_impl<T>::type;

/// @brief An enumeration representing data type
enum class datatype : int
{
    typebits_mask  = 0xFF,
    f              = 0x100, // floating point
    i              = 0x200, // signed integer
    u              = 0x300, // unsigned integer
    c              = 0x400, // complex floating point
    b              = 0x500, // boolean
    typeclass_mask = 0xF00,
    f16            = static_cast<int>(f) | 16,
    f32            = static_cast<int>(f) | 32,
    f64            = static_cast<int>(f) | 64,
    f80            = static_cast<int>(f) | 80,
    i8             = static_cast<int>(i) | 8,
    i16            = static_cast<int>(i) | 16,
    i24            = static_cast<int>(i) | 24,
    i32            = static_cast<int>(i) | 32,
    i64            = static_cast<int>(i) | 64,
    u8             = static_cast<int>(u) | 8,
    u16            = static_cast<int>(u) | 16,
    u24            = static_cast<int>(u) | 24,
    u32            = static_cast<int>(u) | 32,
    u64            = static_cast<int>(u) | 64,
    c32            = static_cast<int>(c) | 32,
    c64            = static_cast<int>(c) | 64,
    b8             = static_cast<int>(b) | 8
};

constexpr inline datatype operator|(datatype x, datatype y)
{
    using type = std::underlying_type_t<datatype>;
    return static_cast<datatype>(static_cast<type>(x) | static_cast<type>(y));
}

constexpr inline datatype operator&(datatype x, datatype y)
{
    using type = std::underlying_type_t<datatype>;
    return static_cast<datatype>(static_cast<type>(x) & static_cast<type>(y));
}

template <typename T>
constexpr inline datatype typeclass =
    std::is_floating_point_v<typename compound_type_traits<T>::subtype> ? datatype::f
    : std::is_integral_v<typename compound_type_traits<T>::subtype>
        ? (std::is_unsigned_v<typename compound_type_traits<T>::subtype> ? datatype::u : datatype::i)
        : datatype();

template <typename T>
constexpr inline bool is_f_class = typeclass<T> == datatype::f;
template <typename T>
constexpr inline bool is_u_class = typeclass<T> == datatype::u;
template <typename T>
constexpr inline bool is_i_class = typeclass<T> == datatype::i;

template <typename T>
struct typebits
{
    constexpr static size_t bits  = sizeof(typename compound_type_traits<T>::subtype) * 8;
    constexpr static size_t width = compound_type_traits<T>::is_scalar ? 0 : compound_type_traits<T>::width;
    using subtype                 = typename compound_type_traits<T>::subtype;
};

template <typename T>
using ftype =
    typename compound_type_traits<T>::template deep_rebind<float_type<typebits<deep_subtype<T>>::bits>>;
template <typename T>
using itype =
    typename compound_type_traits<T>::template deep_rebind<int_type<typebits<deep_subtype<T>>::bits>>;
template <typename T>
using utype =
    typename compound_type_traits<T>::template deep_rebind<unsigned_type<typebits<deep_subtype<T>>::bits>>;

template <typename T>
using uitype = std::conditional_t<is_i_class<deep_subtype<T>>, T, utype<T>>;

template <typename T>
using fsubtype = ftype<subtype<T>>;
template <typename T>
using isubtype = itype<subtype<T>>;
template <typename T>
using usubtype = utype<subtype<T>>;
namespace details
{
template <typename T>
struct flt_type_impl
{
    using type = std::conditional_t<sizeof(T) <= 2, float, fbase>;
};

template <>
struct flt_type_impl<float>
{
    using type = float;
};
template <>
struct flt_type_impl<double>
{
    using type = double;
};
} // namespace details

template <typename T>
using flt_type = typename cometa::compound_type_traits<T>::template deep_rebind<
    typename details::flt_type_impl<deep_subtype<T>>::type>;

} // namespace cometa
