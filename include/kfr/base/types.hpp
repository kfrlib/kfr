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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

#include "../cometa.hpp"

#define KFR_ENABLE_IF CMT_ENABLE_IF

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
using namespace cometa;

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
using fbase = f32;
#else
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

enum class outputinput_t
{
    output,
    input
};
template <outputinput_t p>
using coutputinput_t = cval_t<outputinput_t, p>;

template <outputinput_t p>
constexpr coutputinput_t<p> coutputinput{};

using coutput_t = coutputinput_t<outputinput_t::output>;
using cinput_t  = coutputinput_t<outputinput_t::input>;

constexpr coutput_t coutput{};
constexpr cinput_t cinput{};

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
using pass_through = cometa::fn_pass_through;
using noop         = cometa::fn_noop;
using get_first    = cometa::fn_get_first;
using get_second   = cometa::fn_get_second;
using get_third    = cometa::fn_get_third;

template <typename T>
using returns = cometa::fn_returns<T>;
}

template <typename T>
using ftype = deep_rebind<T, float_type<typebits<deep_subtype<T>>::bits>>;
template <typename T>
using itype = deep_rebind<T, int_type<typebits<deep_subtype<T>>::bits>>;
template <typename T>
using utype = deep_rebind<T, unsigned_type<typebits<deep_subtype<T>>::bits>>;

template <typename T>
using fsubtype = ftype<subtype<T>>;
template <typename T>
using isubtype = itype<subtype<T>>;
template <typename T>
using usubtype = utype<subtype<T>>;

template <typename T, typename R = T>
using enable_if_vec = enable_if<(typebits<T>::width > 0), R>;
template <typename T, typename R = T>
using enable_if_not_vec = enable_if<(typebits<T>::width == 0), R>;

template <typename T, typename R = T>
using enable_if_i = enable_if<typeclass<T> == datatype::i, R>;
template <typename T, typename R = T>
using enable_if_u = enable_if<typeclass<T> == datatype::u, R>;
template <typename T, typename R = T>
using enable_if_f = enable_if<typeclass<T> == datatype::f, R>;

template <typename T, typename R = T>
using enable_if_not_i = enable_if<typeclass<T> != datatype::i, R>;
template <typename T, typename R = T>
using enable_if_not_u = enable_if<typeclass<T> != datatype::u, R>;
template <typename T, typename R = T>
using enable_if_not_f = enable_if<typeclass<T> != datatype::f, R>;

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

CMT_INLINE f32 builtin_sqrt(f32 x) { return __builtin_sqrtf(x); }
CMT_INLINE f64 builtin_sqrt(f64 x) { return __builtin_sqrt(x); }
CMT_INLINE f80 builtin_sqrt(f80 x) { return __builtin_sqrtl(x); }
CMT_INLINE void builtin_memcpy(void* dest, const void* src, size_t size)
{
    __builtin_memcpy(dest, src, size);
}
CMT_INLINE void builtin_memset(void* dest, int val, size_t size) { __builtin_memset(dest, val, size); }
template <typename T1>
CMT_INLINE void zeroize(T1& value)
{
    builtin_memset(static_cast<void*>(builtin_addressof(value)), 0, sizeof(T1));
}
}

template <typename T>
struct initialvalue
{
};

constexpr double infinity = __builtin_inf();
constexpr double qnan     = __builtin_nan("");

namespace internal
{
constexpr f32 allones_f32 = -__builtin_nanf("0xFFFFFFFF");
constexpr f64 allones_f64 = -__builtin_nan("0xFFFFFFFFFFFFFFFF");

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub allones = choose_const<Tsub>(allones_f32, allones_f64, static_cast<Tsub>(-1));

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub allzeros = Tsub();

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub highbitmask = choose_const<Tsub>(-0.f, -0.0, 1ull << (typebits<T>::bits - 1));

template <typename T, typename Tsub = subtype<T>>
constexpr Tsub invhighbitmask = choose_const<Tsub>(__builtin_nanf("0xFFFFFFFF"),
                                                   __builtin_nan("0xFFFFFFFFFFFFFFFF"),
                                                   ~(1ull << (typebits<T>::bits - 1)));

template <typename T>
constexpr inline T maskbits(bool value)
{
    return value ? internal::allones<T> : T();
}
}

namespace internal
{
template <size_t width, typename Fn>
CMT_INLINE void block_process_impl(size_t& i, size_t size, Fn&& fn)
{
    CMT_LOOP_NOUNROLL
    for (; i < size / width * width; i += width)
        fn(i, csize<width>);
}
}

template <size_t... widths, typename Fn>
CMT_INLINE void block_process(size_t size, csizes_t<widths...>, Fn&& fn)
{
    size_t i = 0;
    swallow{ (internal::block_process_impl<widths>(i, size, std::forward<Fn>(fn)), 0)... };
}
}

#pragma GCC diagnostic pop
