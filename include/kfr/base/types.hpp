/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once
#include "kfr.h"

#include "intrinsics.h"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <type_traits>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

#include "../cometa.hpp"

#define KFR_ENABLE_IF CMT_ENABLE_IF

#define KFR_FN(fn)                                                                                           \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(fn(std::declval<Args>()...)) operator()(Args&&... args) const             \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

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

#define KFR_FNR(fn, in, out)                                                                                 \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        using ratio = ioratio<in, out>;                                                                      \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(fn(std::declval<Args>()...)) operator()(Args&&... args) const             \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

#define KFR_SPEC_FN(tpl, fn)                                                                                 \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        constexpr fn_##fn() noexcept = default;                                                              \
        template <typename... Args>                                                                          \
        CMT_INLINE decltype(fn(std::declval<Args>()...)) operator()(Args&&... args) const                    \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

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

template <size_t in, size_t out>
struct ioratio
{
    constexpr static size_t input  = in;
    constexpr static size_t output = out;
};

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

struct generic
{
    template <typename T>
    CMT_INLINE constexpr operator T() const noexcept
    {
        return T();
    }
};

struct infinite
{
    template <typename T>
    CMT_INLINE constexpr operator T() const noexcept
    {
        return T();
    }
    constexpr friend bool operator<(infinite, size_t) noexcept { return false; }
    constexpr friend bool operator<(size_t, infinite) noexcept { return true; }
    constexpr friend bool operator<(infinite, infinite) noexcept { return false; }
};

enum class accuracy : int
{
    accuracy      = 1,
    speed         = 2,
    _accuracy_min = static_cast<int>(accuracy),
    _accuracy_max = static_cast<int>(speed)
};

enum class archendianness : int
{
    littleendian        = 1,
    bigendian           = 2,
    _archendianness_min = static_cast<int>(littleendian),
    _archendianness_max = static_cast<int>(bigendian)
};

typedef void*(CMT_CDECL* func_allocate)(size_t);

typedef void(CMT_CDECL* func_deallocate)(void*);

struct mem_allocator
{
    func_allocate allocate;
    func_deallocate deallocate;
    size_t granularity;
    size_t alignment;
};

struct mem_header
{
    size_t size;
    mem_allocator* allocator;
    uintptr_t refcount;
    uintptr_t reserved;
};

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

namespace internal
{
template <typename Fn, typename enable = void_t<>>
struct func_ratio_impl
{
    using type = ioratio<1, 1>;
};
template <typename Fn>
struct func_ratio_impl<Fn, void_t<typename Fn::ratio>>
{
    using type = typename Fn::ratio;
};
}

template <typename Fn>
using func_ratio = typename internal::func_ratio_impl<remove_reference<Fn>>::type;

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

enum class cpu_t : int
{
    common = 0,
#ifdef CMT_ARCH_X86
    sse2    = 1,
    sse3    = 2,
    ssse3   = 3,
    sse41   = 4,
    sse42   = 5,
    avx1    = 6,
    avx2    = 7,
    avx     = static_cast<int>(avx1),
    lowest  = static_cast<int>(sse2),
    highest = static_cast<int>(avx2),
#endif
#ifdef CMT_ARCH_ARM
    neon    = 1,
    neon64  = 2,
    lowest  = static_cast<int>(neon),
    highest = static_cast<int>(neon64),
#endif
    native  = static_cast<int>(CMT_ARCH_NAME),
    runtime = -1,
};

#define KFR_ARCH_DEP cpu_t cpu = cpu_t::native

template <cpu_t cpu>
using ccpu_t = cval_t<cpu_t, cpu>;

template <cpu_t cpu>
constexpr ccpu_t<cpu> ccpu{};

namespace internal
{
constexpr cpu_t older(cpu_t x) { return static_cast<cpu_t>(static_cast<int>(x) - 1); }
constexpr cpu_t newer(cpu_t x) { return static_cast<cpu_t>(static_cast<int>(x) + 1); }

#ifdef CMT_ARCH_X86
constexpr auto cpu_list =
    cvals<cpu_t, cpu_t::avx2, cpu_t::avx1, cpu_t::sse41, cpu_t::ssse3, cpu_t::sse3, cpu_t::sse2>;
#else
constexpr auto cpu_list = cvals<cpu_t, cpu_t::neon>;
#endif
}

template <cpu_t cpu>
using cpuval_t = cval_t<cpu_t, cpu>;
template <cpu_t cpu>
constexpr auto cpuval = cpuval_t<cpu>{};

constexpr auto cpu_all = cfilter(internal::cpu_list, internal::cpu_list >= cpuval<cpu_t::native>);

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
    builtin_memset(static_cast<void*>(std::addressof(value)), 0, sizeof(T1));
}
}

#pragma clang diagnostic push
#if CMT_HAS_WARNING("-Wundefined-reinterpret-cast")
#pragma clang diagnostic ignored "-Wundefined-reinterpret-cast"
#endif

template <typename T, typename U>
constexpr inline static T& ref_cast(U& ptr)
{
    return reinterpret_cast<T&>(ptr);
}

template <typename T, typename U>
constexpr inline static const T& ref_cast(const U& ptr)
{
    return reinterpret_cast<const T&>(ptr);
}

template <typename T, typename U>
constexpr inline static T* ptr_cast(U* ptr)
{
    return reinterpret_cast<T*>(ptr);
}

template <typename T, typename U>
constexpr inline static const T* ptr_cast(const U* ptr)
{
    return reinterpret_cast<const T*>(ptr);
}

template <typename T, typename U>
constexpr inline static T* ptr_cast(U* ptr, ptrdiff_t offset)
{
    return ptr_cast<T>(ptr_cast<u8>(ptr) + offset);
}

template <typename T, typename U>
constexpr inline static T* derived_cast(U* ptr)
{
    return static_cast<T*>(ptr);
}

template <typename T, typename U>
constexpr inline static const T* derived_cast(const U* ptr)
{
    return static_cast<const T*>(ptr);
}

template <typename T, typename U>
constexpr inline static T implicit_cast(U&& value)
{
    return std::forward<T>(value);
}

#pragma clang diagnostic pop

__attribute__((unused)) static const char* cpu_name(cpu_t set)
{
    static const char* names[] = { "sse2", "sse3", "ssse3", "sse41", "sse42", "avx1", "avx2" };
    if (set >= cpu_t::lowest && set <= cpu_t::highest)
        return names[static_cast<size_t>(set)];
    return "-";
}

#define KFR_FN_S(fn)                                                                                         \
    template <typename Arg, typename... Args>                                                                \
    CMT_INLINE enable_if_not_vec<Arg> fn(Arg arg, Args... args)                                              \
    {                                                                                                        \
        return fn(make_vector(arg), make_vector(args)...)[0];                                                \
    }
#define KFR_FN_S_S(fn)                                                                                       \
    template <typename Arg, typename... Args, KFR_ENABLE_IF(is_number<Arg>::value)>                          \
    KFR_SINTRIN enable_if_not_vec<Arg> fn(Arg arg, Args... args)                                             \
    {                                                                                                        \
        return fn(make_vector(arg), make_vector(args)...)[0];                                                \
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

template <typename T>
constexpr inline const T& bitness_const(const T& x32, const T& x64)
{
#ifdef CMT_ARCH_X64
    (void)x32;
    return x64;
#else
    (void)x64;
    return x32;
#endif
}

constexpr inline const char* bitness_const(const char* x32, const char* x64)
{
#ifdef CMT_ARCH_X64
    (void)x32;
    return x64;
#else
    (void)x64;
    return x32;
#endif
}

constexpr size_t native_cache_alignment        = 64;
constexpr size_t native_cache_alignment_mask   = native_cache_alignment - 1;
constexpr size_t maximum_vector_alignment      = 32;
constexpr size_t maximum_vector_alignment_mask = maximum_vector_alignment - 1;
constexpr size_t native_register_count         = bitness_const(8, 16);

constexpr size_t common_float_vector_size = 16;
constexpr size_t common_int_vector_size   = 16;

template <cpu_t c>
constexpr size_t native_float_vector_size =
#ifdef CMT_ARCH_X86
    c >= cpu_t::avx1 ? 32 : c >= cpu_t::sse2 ? 16 : common_float_vector_size;
#endif
#ifdef CMT_ARCH_ARM
c == cpu_t::neon ? 16 : common_float_vector_size;
#endif
template <cpu_t c>
constexpr size_t native_int_vector_size =
#ifdef CMT_ARCH_X86
    c >= cpu_t::avx2 ? 32 : c >= cpu_t::sse2 ? 16 : common_int_vector_size;
#endif
#ifdef CMT_ARCH_ARM
c == cpu_t::neon ? 16 : common_int_vector_size;
#endif

struct input_expression
{
    using value_type = generic;
    using size_type  = infinite;
    constexpr size_type size() const noexcept { return {}; }

    CMT_INLINE void begin_block(size_t) const {}
    CMT_INLINE void end_block(size_t) const {}
};

struct output_expression
{
    using value_type = generic;
    using size_type  = infinite;
    constexpr size_type size() const noexcept { return {}; }

    CMT_INLINE void output_begin_block(size_t) const {}
    CMT_INLINE void output_end_block(size_t) const {}
};

template <typename E>
using is_input_expression = std::is_base_of<input_expression, decay<E>>;

template <typename... Es>
using is_input_expressions = or_t<std::is_base_of<input_expression, decay<Es>>...>;

template <typename E>
using is_output_expression = std::is_base_of<output_expression, decay<E>>;

template <typename T>
using is_numeric = is_number<deep_subtype<T>>;

template <typename... Ts>
using is_numeric_args = and_t<is_numeric<Ts>...>;

template <typename T, cpu_t c = cpu_t::native>
constexpr size_t vector_width = const_max(size_t(1), typeclass<T> == datatype::f
                                                         ? native_float_vector_size<c> / sizeof(T)
                                                         : native_int_vector_size<c> / sizeof(T));

template <cpu_t c>
constexpr size_t vector_width<void, c> = 0;

namespace internal
{

template <cpu_t c>
constexpr size_t native_vector_alignment = const_max(native_float_vector_size<c>, native_int_vector_size<c>);

template <cpu_t c>
constexpr bool fast_unaligned =
#ifdef CMT_ARCH_X86
    c >= cpu_t::avx1;
#else
    false;
#endif

template <cpu_t c>
constexpr size_t native_vector_alignment_mask = native_vector_alignment<c> - 1;

template <typename T, cpu_t c>
constexpr inline size_t get_vector_width(size_t scale = 1)
{
    return scale * vector_width<T, c>;
}
template <typename T, cpu_t c>
constexpr inline size_t get_vector_width(size_t x32scale, size_t x64scale)
{
    return bitness_const(x32scale, x64scale) * vector_width<T, c>;
}

template <typename T, cpu_t c>
constexpr auto vector_width_range = csize<1> << csizeseq<ilog2(vector_width<T, c>) + 1>;

template <typename T, cpu_t c>
constexpr size_t vector_capacity = native_register_count* vector_width<T, c>;

template <typename T, cpu_t c>
constexpr size_t maximum_vector_size = const_min(static_cast<size_t>(32), vector_capacity<T, c> / 4);
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

#pragma clang diagnostic pop
