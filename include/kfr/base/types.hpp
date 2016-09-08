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
constexpr inline T align_down(T x, identity<T> alignment)
{
    return (x) & ~(alignment - 1);
}
template <typename T>
constexpr inline T* align_down(T* x, size_t alignment)
{
    return reinterpret_cast<T*>(align_down(reinterpret_cast<size_t>(x), alignment));
}

template <typename T>
constexpr inline T align_up(T x, identity<T> alignment)
{
    return (x + alignment - 1) & ~(alignment - 1);
}
template <typename T>
constexpr inline T* align_up(T* x, size_t alignment)
{
    return reinterpret_cast<T*>(align_up(reinterpret_cast<size_t>(x), alignment));
}

template <typename T>
constexpr inline T* advance(T* x, ptrdiff_t offset)
{
    return x + offset;
}
constexpr inline void* advance(void* x, ptrdiff_t offset)
{
    return advance(static_cast<unsigned char*>(x), offset);
}

constexpr inline ptrdiff_t distance(const void* x, const void* y)
{
    return static_cast<const unsigned char*>(x) - static_cast<const unsigned char*>(y);
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
using pass_through = cometa::fn_pass_through;
using noop         = cometa::fn_noop;
using get_first    = cometa::fn_get_first;
using get_second   = cometa::fn_get_second;
using get_third    = cometa::fn_get_third;

template <typename T>
using returns = cometa::fn_returns<T>;
}

namespace internal
{
template <size_t bits>
struct float_type_impl;
template <size_t bits>
struct int_type_impl;
template <size_t bits>
struct unsigned_type_impl;

template <>
struct float_type_impl<32>
{
    using type = f32;
};
template <>
struct float_type_impl<64>
{
    using type = f64;
};

template <>
struct int_type_impl<8>
{
    using type = i8;
};
template <>
struct int_type_impl<16>
{
    using type = i16;
};
template <>
struct int_type_impl<32>
{
    using type = i32;
};
template <>
struct int_type_impl<64>
{
    using type = i64;
};

template <>
struct unsigned_type_impl<8>
{
    using type = u8;
};
template <>
struct unsigned_type_impl<16>
{
    using type = u16;
};
template <>
struct unsigned_type_impl<32>
{
    using type = u32;
};
template <>
struct unsigned_type_impl<64>
{
    using type = u64;
};
}

template <size_t bits>
using float_type = typename internal::float_type_impl<bits>::type;
template <size_t bits>
using int_type = typename internal::int_type_impl<bits>::type;
template <size_t bits>
using unsigned_type = typename internal::unsigned_type_impl<bits>::type;

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

/// @brief Base class for all vector classes
template <typename T, size_t N>
struct vec_t
{
    using value_type = T;
    constexpr static size_t size() noexcept { return N; }
    constexpr vec_t() noexcept = default;

    using scalar_type = subtype<T>;
    constexpr static size_t scalar_size() noexcept { return N * compound_type_traits<T>::width; }
};

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

#pragma clang diagnostic push
#if CMT_HAS_WARNING("-Wundefined-reinterpret-cast")
#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
#endif

template <typename T, typename U>
CMT_INLINE constexpr static T& ref_cast(U& ptr)
{
    return reinterpret_cast<T&>(ptr);
}

template <typename T, typename U>
CMT_INLINE constexpr static const T& ref_cast(const U& ptr)
{
    return reinterpret_cast<const T&>(ptr);
}

template <typename T, typename U>
CMT_INLINE constexpr static T* ptr_cast(U* ptr)
{
    return reinterpret_cast<T*>(ptr);
}

template <typename T, typename U>
CMT_INLINE constexpr static const T* ptr_cast(const U* ptr)
{
    return reinterpret_cast<const T*>(ptr);
}

template <typename T, typename U>
CMT_INLINE constexpr static T* ptr_cast(U* ptr, ptrdiff_t offset)
{
    return ptr_cast<T>(ptr_cast<u8>(ptr) + offset);
}

template <typename T, typename U>
CMT_INLINE constexpr static T* derived_cast(U* ptr)
{
    return static_cast<T*>(ptr);
}

template <typename T, typename U>
CMT_INLINE constexpr static const T* derived_cast(const U* ptr)
{
    return static_cast<const T*>(ptr);
}

template <typename T, typename U>
CMT_INLINE constexpr static T implicit_cast(U&& value)
{
    return std::forward<T>(value);
}

#pragma clang diagnostic pop

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

template <typename T>
constexpr size_t widthof(T)
{
    return compound_type_traits<T>::width;
}
template <typename T>
constexpr size_t widthof()
{
    return compound_type_traits<T>::width;
}

constexpr size_t infinite_size = static_cast<size_t>(-1);

constexpr inline size_t size_add(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : x + y;
}

constexpr inline size_t size_sub(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : x - y;
}

constexpr inline size_t size_min(size_t x) noexcept { return x; }

template <typename... Ts>
constexpr inline size_t size_min(size_t x, size_t y, Ts... rest) noexcept
{
    return size_min(x < y ? x : y, rest...);
}

/// @brief Base class of all input expressoins
struct input_expression
{
    constexpr static size_t size() noexcept { return infinite_size; }

    constexpr static bool is_incremental = false;

    CMT_INLINE void begin_block(size_t) const {}
    CMT_INLINE void end_block(size_t) const {}
};

/// @brief Base class of all output expressoins
struct output_expression
{
    constexpr static size_t size() noexcept { return infinite_size; }

    constexpr static bool is_incremental = false;

    CMT_INLINE void output_begin_block(size_t) const {}
    CMT_INLINE void output_end_block(size_t) const {}
};

/// @brief Check if the type argument is an input expression
template <typename E>
using is_input_expression = std::is_base_of<input_expression, decay<E>>;

/// @brief Check if the type arguments are an input expressions
template <typename... Es>
using is_input_expressions = or_t<std::is_base_of<input_expression, decay<Es>>...>;

/// @brief Check if the type argument is an output expression
template <typename E>
using is_output_expression = std::is_base_of<output_expression, decay<E>>;

/// @brief Check if the type arguments are an output expressions
template <typename... Es>
using is_output_expressions = or_t<std::is_base_of<output_expression, decay<Es>>...>;

/// @brief Check if the type argument is a number or a vector of numbers
template <typename T>
using is_numeric = is_number<deep_subtype<T>>;

/// @brief Check if the type arguments are a numbers or a vectors of numbers
template <typename... Ts>
using is_numeric_args = and_t<is_numeric<Ts>...>;

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
namespace cometa
{

template <typename T, size_t N>
struct compound_type_traits<kfr::vec_t<T, N>>
{
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;

    template <typename U>
    using rebind = kfr::vec_t<U, N>;
    template <typename U>
    using deep_rebind = kfr::vec_t<cometa::deep_rebind<subtype, U>, N>;
};
}

#pragma GCC diagnostic pop
