/** @addtogroup types
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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
#include "kfr.h"

#include "intrinsics.h"

#include <cmath>

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")

#ifdef KFR_TESTING
#include "../testo/testo.hpp"
#endif

#include "../cometa.hpp"

#define KFR_ENABLE_IF CMT_ENABLE_IF

/**
 *  @brief Internal macro for functions
 */
#define KFR_FN(FN)                                                                                           \
    namespace fn                                                                                             \
    {                                                                                                        \
    struct FN                                                                                                \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(::kfr::FN(std::declval<Args>()...)) operator()(Args&&... args) const      \
        {                                                                                                    \
            return ::kfr::FN(std::forward<Args>(args)...);                                                   \
        }                                                                                                    \
    };                                                                                                       \
    }

/**
 *  @brief Internal macro for functions
 */
#define KFR_I_FN(FN)                                                                                         \
    namespace fn                                                                                             \
    {                                                                                                        \
    struct FN                                                                                                \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(::kfr::intrinsics::FN(std::declval<Args>()...)) operator()(               \
            Args&&... args) const                                                                            \
        {                                                                                                    \
            return ::kfr::intrinsics::FN(std::forward<Args>(args)...);                                       \
        }                                                                                                    \
    };                                                                                                       \
    }

namespace kfr
{
// Include all from CoMeta library
using namespace cometa;

/// @brief Short names for common types
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

#if defined(KFR_BASETYPE_F32) || defined(KFR_NO_NATIVE_F64)
/// @brief Floating point type used by default
using fbase = f32;
#else
/// @brief Floating point type used by default
using fbase = f64;
#endif

constexpr ctype_t<f32> ctype_f32{};
constexpr ctype_t<f64> ctype_f64{};
constexpr ctype_t<i8> ctype_i8{};
constexpr ctype_t<i16> ctype_i16{};
constexpr ctype_t<i32> ctype_i32{};
constexpr ctype_t<i64> ctype_i64{};
constexpr ctype_t<u8> ctype_u8{};
constexpr ctype_t<u16> ctype_u16{};
constexpr ctype_t<u32> ctype_u32{};
constexpr ctype_t<u64> ctype_u64{};
constexpr ctype_t<umax> ctype_umax{};
constexpr ctype_t<imax> ctype_imax{};
constexpr ctype_t<fmax> ctype_fmax{};
constexpr ctype_t<f80> ctype_f80{};
constexpr ctype_t<fbase> ctype_base{};

struct u24
{
    u8 raw[3];
};

struct i24
{
    u8 raw[3];
};

struct f16
{
    u16 raw;
};

/// @brief An enumeration representing data type
template <typename T1>
struct range
{
    T1 min;
    T1 max;
    T1 distance() const { return max - min; }
};

/// @brief An enumeration representing data type
enum class datatype : int
{
    typebits_mask       = 0xFF,
    f                   = 0x100,
    i                   = 0x200,
    u                   = 0x300,
    c                   = 0x400,
    typeclass_mask      = 0xF00,
    x1                  = 0x1000,
    x2                  = 0x2000,
    x3                  = 0x3000,
    x4                  = 0x4000,
    typecomponents_mask = 0xF000,
    f16                 = static_cast<int>(f) | static_cast<int>(x1) | 16,
    f32                 = static_cast<int>(f) | static_cast<int>(x1) | 32,
    f64                 = static_cast<int>(f) | static_cast<int>(x1) | 64,
    f80                 = static_cast<int>(f) | static_cast<int>(x1) | 80,
    i8                  = static_cast<int>(i) | static_cast<int>(x1) | 8,
    i16                 = static_cast<int>(i) | static_cast<int>(x1) | 16,
    i24                 = static_cast<int>(i) | static_cast<int>(x1) | 24,
    i32                 = static_cast<int>(i) | static_cast<int>(x1) | 32,
    i64                 = static_cast<int>(i) | static_cast<int>(x1) | 64,
    u8                  = static_cast<int>(u) | static_cast<int>(x1) | 8,
    u16                 = static_cast<int>(u) | static_cast<int>(x1) | 16,
    u24                 = static_cast<int>(u) | static_cast<int>(x1) | 24,
    u32                 = static_cast<int>(u) | static_cast<int>(x1) | 32,
    u64                 = static_cast<int>(u) | static_cast<int>(x1) | 64,
    c32                 = static_cast<int>(c) | static_cast<int>(x2) | 32,
    c64                 = static_cast<int>(c) | static_cast<int>(x2) | 64
};

inline datatype operator|(datatype x, datatype y)
{
    using type = underlying_type<datatype>;
    return static_cast<datatype>(static_cast<type>(x) | static_cast<type>(y));
}

inline datatype operator&(datatype x, datatype y)
{
    using type = underlying_type<datatype>;
    return static_cast<datatype>(static_cast<type>(x) | static_cast<type>(y));
}

template <typename T>
constexpr datatype typeclass = std::is_floating_point<typename compound_type_traits<T>::subtype>::value
                                   ? datatype::f
                                   : std::is_integral<typename compound_type_traits<T>::subtype>::value
                                         ? (std::is_unsigned<typename compound_type_traits<T>::subtype>::value
                                                ? datatype::u
                                                : datatype::i)
                                         : datatype();

template <typename T>
using is_f_class = std::integral_constant<bool, typeclass<T> == datatype::f>;
template <typename T>
using is_u_class = std::integral_constant<bool, typeclass<T> == datatype::u>;
template <typename T>
using is_i_class = std::integral_constant<bool, typeclass<T> == datatype::i>;

template <typename T>
struct typebits
{
    static_assert(is_number<deep_subtype<T>>::value, "");
    constexpr static size_t bits  = sizeof(typename compound_type_traits<T>::subtype) * 8;
    constexpr static size_t width = compound_type_traits<T>::is_scalar ? 0 : compound_type_traits<T>::width;
    using subtype                 = typename compound_type_traits<T>::subtype;
};

namespace fn
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
}

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
using fsubtype = ftype<subtype<T>>;
template <typename T>
using isubtype = itype<subtype<T>>;
template <typename T>
using usubtype = utype<subtype<T>>;

namespace internal
{
template <typename T>
struct flt_type_impl
{
    using type = fbase;
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
}

template <typename T>
using flt_type = typename internal::flt_type_impl<T>::type;

namespace internal
{
#ifdef CMT_COMPILER_CLANG
#define builtin_addressof(x) __builtin_addressof(x)
#else
template <class T>
inline T* builtin_addressof(T& arg)
{
    return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg)));
}
#endif

#ifdef CMT_COMPILER_GNU
CMT_INLINE f32 builtin_sqrt(f32 x) { return __builtin_sqrtf(x); }
CMT_INLINE f64 builtin_sqrt(f64 x) { return __builtin_sqrt(x); }
CMT_INLINE f80 builtin_sqrt(f80 x) { return __builtin_sqrtl(x); }
CMT_INLINE void builtin_memcpy(void* dest, const void* src, size_t size)
{
    __builtin_memcpy(dest, src, size);
}
CMT_INLINE void builtin_memset(void* dest, int val, size_t size) { __builtin_memset(dest, val, size); }
#else

CMT_INLINE f32 builtin_sqrt(f32 x) { return ::sqrtf(x); }
CMT_INLINE f64 builtin_sqrt(f64 x) { return ::sqrt(x); }
CMT_INLINE f80 builtin_sqrt(f80 x) { return ::sqrtl(x); }
CMT_INLINE void builtin_memcpy(void* dest, const void* src, size_t size) { ::memcpy(dest, src, size); }
CMT_INLINE void builtin_memset(void* dest, int val, size_t size) { ::memset(dest, val, size); }

#endif

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wattributes")

template <typename T, bool A>
struct struct_with_alignment
{
    T value;
    KFR_INTRIN void operator=(T value) { this->value = value; }
};

template <typename T>
struct struct_with_alignment<T, false>
{
    T value;
    KFR_INTRIN void operator=(T value) { this->value = value; }
}
#ifdef CMT_GNU_ATTRIBUTES
__attribute__((__packed__, __may_alias__)) //
#endif
;

CMT_PRAGMA_GNU(GCC diagnostic pop)
}

/// @brief Fills a value with zeros
template <typename T1>
CMT_INLINE void zeroize(T1& value)
{
    internal::builtin_memset(static_cast<void*>(builtin_addressof(value)), 0, sizeof(T1));
}

/// @brief Used to determine the initial value for reduce functions
template <typename T>
struct initialvalue
{
};

namespace internal
{
template <size_t width, typename Fn>
CMT_INLINE void block_process_impl(size_t& i, size_t size, Fn&& fn)
{
    CMT_LOOP_NOUNROLL
    for (; i < size / width * width; i += width)
        fn(i, csize_t<width>());
}
}

template <size_t... widths, typename Fn>
CMT_INLINE void block_process(size_t size, csizes_t<widths...>, Fn&& fn)
{
    size_t i = 0;
    swallow{ (internal::block_process_impl<widths>(i, size, std::forward<Fn>(fn)), 0)... };
}

template <typename T>
struct is_simd_type
    : std::integral_constant<
          bool, std::is_same<T, float>::value || std::is_same<T, double>::value ||
                    std::is_same<T, signed char>::value || std::is_same<T, unsigned char>::value ||
                    std::is_same<T, short>::value || std::is_same<T, unsigned short>::value ||
                    std::is_same<T, int>::value || std::is_same<T, unsigned int>::value ||
                    std::is_same<T, long>::value || std::is_same<T, unsigned long>::value ||
                    std::is_same<T, long long>::value || std::is_same<T, unsigned long long>::value>
{
};

template <typename T, size_t N>
struct vec_t
{
    static_assert(N > 0 && N <= 256, "Invalid vector size");

    static_assert(is_simd_type<T>::value || !compound_type_traits<T>::is_scalar, "Invalid vector type");

    using value_type = T;
    constexpr static size_t size() noexcept { return N; }
    constexpr vec_t() noexcept = default;

    using scalar_type = subtype<T>;
    constexpr static size_t scalar_size() noexcept { return N * compound_type_traits<T>::width; }
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
}

CMT_PRAGMA_GNU(GCC diagnostic pop)
