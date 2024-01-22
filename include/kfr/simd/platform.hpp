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

#include "types.hpp"

namespace kfr
{

/// @brief An enumeration representing cpu instruction set
enum class cpu_t : int
{
    generic = 0,
#ifdef CMT_ARCH_X86
    sse2    = 1,
    sse3    = 2,
    ssse3   = 3,
    sse41   = 4,
    sse42   = 5,
    avx1    = 6,
    avx2    = 7,
    avx512  = 8, // F, CD, VL, DQ and BW
    avx     = static_cast<int>(avx1),
    lowest  = static_cast<int>(sse2),
    highest = static_cast<int>(avx512),
#endif
#ifdef CMT_ARCH_ARM
    neon    = 1,
    neon64  = 2,
    lowest  = static_cast<int>(neon),
    highest = static_cast<int>(neon64),
#endif
    native = static_cast<int>(CMT_ARCH_NAME),

#ifdef CMT_ARCH_AVX
#define KFR_HAS_SECONDARY_PLATFORM
    secondary = static_cast<int>(sse42),
#else
    secondary = static_cast<int>(native),
#endif

    common  = generic, // For compatibility
    runtime = -1,
};

template <cpu_t cpu>
using ccpu_t = cval_t<cpu_t, cpu>;

template <cpu_t cpu>
constexpr ccpu_t<cpu> ccpu{};

namespace internal_generic
{
constexpr cpu_t older(cpu_t x) { return static_cast<cpu_t>(static_cast<int>(x) - 1); }
constexpr cpu_t newer(cpu_t x) { return static_cast<cpu_t>(static_cast<int>(x) + 1); }

#ifdef CMT_ARCH_X86
constexpr auto cpu_list = cvals_t<cpu_t, cpu_t::avx512, cpu_t::avx2, cpu_t::avx1, cpu_t::sse41, cpu_t::ssse3,
                                  cpu_t::sse3, cpu_t::sse2>();
#else
constexpr auto cpu_list = cvals<cpu_t, cpu_t::neon>;
#endif
} // namespace internal_generic

template <cpu_t cpu>
using cpuval_t = cval_t<cpu_t, cpu>;
template <cpu_t cpu>
constexpr auto cpuval = cpuval_t<cpu>{};

constexpr auto cpu_all =
    cfilter(internal_generic::cpu_list, internal_generic::cpu_list >= cpuval_t<cpu_t::native>());

/// @brief Returns name of the cpu instruction set
CMT_UNUSED static const char* cpu_name(cpu_t set)
{
#ifdef CMT_ARCH_X86
    static const char* names[] = { "generic", "sse2", "sse3", "ssse3", "sse41",
                                   "sse42",   "avx",  "avx2", "avx512" };
#endif
#ifdef CMT_ARCH_ARM
    static const char* names[] = { "generic", "neon", "neon64" };
#endif
    if (CMT_LIKELY(set >= cpu_t::lowest && set <= cpu_t::highest))
        return names[static_cast<size_t>(set)];
    return "-";
}

#ifdef CMT_ARCH_X64
template <int = 0>
constexpr inline const char* bitness_const(const char*, const char* x64)
{
    return x64;
}
template <typename T>
constexpr inline const T& bitness_const(const T&, const T& x64)
{
    return x64;
}
#else
template <int = 0>
constexpr inline const char* bitness_const(const char* x32, const char*)
{
    return x32;
}
template <typename T>
constexpr inline const T& bitness_const(const T& x32, const T&)
{
    return x32;
}
#endif

template <cpu_t c = cpu_t::native>
struct platform;

#ifdef CMT_ARCH_X86
template <>
struct platform<cpu_t::common>
{
    constexpr static size_t native_cache_alignment        = 64;
    constexpr static size_t native_cache_alignment_mask   = native_cache_alignment - 1;
    constexpr static size_t maximum_vector_alignment      = 64;
    constexpr static size_t maximum_vector_alignment_mask = maximum_vector_alignment - 1;

    constexpr static size_t simd_register_count = 1;

    constexpr static size_t common_float_vector_size = 16;
    constexpr static size_t common_int_vector_size   = 16;

    constexpr static size_t minimum_float_vector_size = 16;
    constexpr static size_t minimum_int_vector_size   = 16;

    constexpr static size_t native_float_vector_size = 16;
    constexpr static size_t native_int_vector_size   = 16;

    constexpr static size_t native_vector_alignment      = 16;
    constexpr static size_t native_vector_alignment_mask = native_vector_alignment - 1;

    constexpr static bool fast_unaligned = false;

    constexpr static bool mask_registers = false;
};
template <>
struct platform<cpu_t::sse2> : platform<cpu_t::common>
{
    constexpr static size_t simd_register_count = bitness_const(8, 16);
};
template <>
struct platform<cpu_t::sse3> : platform<cpu_t::sse2>
{
};
template <>
struct platform<cpu_t::ssse3> : platform<cpu_t::sse3>
{
};
template <>
struct platform<cpu_t::sse41> : platform<cpu_t::ssse3>
{
};
template <>
struct platform<cpu_t::sse42> : platform<cpu_t::sse41>
{
};
template <>
struct platform<cpu_t::avx> : platform<cpu_t::sse42>
{
    constexpr static size_t native_float_vector_size = 32;

    constexpr static size_t native_vector_alignment      = 32;
    constexpr static size_t native_vector_alignment_mask = native_vector_alignment - 1;

    constexpr static bool fast_unaligned = true;
};
template <>
struct platform<cpu_t::avx2> : platform<cpu_t::avx>
{
    constexpr static size_t native_int_vector_size = 32;
};
template <>
struct platform<cpu_t::avx512> : platform<cpu_t::avx2>
{
    constexpr static size_t native_float_vector_size = 64;
    constexpr static size_t native_int_vector_size   = 64;

    constexpr static size_t native_vector_alignment      = 64;
    constexpr static size_t native_vector_alignment_mask = native_vector_alignment - 1;

    constexpr static size_t simd_register_count = bitness_const(8, 32);

    constexpr static bool mask_registers = true;
};
#endif
#ifdef CMT_ARCH_ARM
template <>
struct platform<cpu_t::common>
{
    constexpr static size_t native_cache_alignment        = 64;
    constexpr static size_t native_cache_alignment_mask   = native_cache_alignment - 1;
    constexpr static size_t maximum_vector_alignment      = 16;
    constexpr static size_t maximum_vector_alignment_mask = maximum_vector_alignment - 1;

    constexpr static size_t simd_register_count = 1;

    constexpr static size_t common_float_vector_size = 16;
    constexpr static size_t common_int_vector_size   = 16;

    constexpr static size_t minimum_float_vector_size = 16;
    constexpr static size_t minimum_int_vector_size   = 16;

    constexpr static size_t native_float_vector_size = 16;
    constexpr static size_t native_int_vector_size   = 16;

    constexpr static size_t native_vector_alignment      = 16;
    constexpr static size_t native_vector_alignment_mask = native_vector_alignment - 1;

    constexpr static bool fast_unaligned = false;

    constexpr static bool mask_registers = false;
};
template <>
struct platform<cpu_t::neon> : platform<cpu_t::common>
{
    constexpr static size_t simd_register_count = 32;
};
template <>
struct platform<cpu_t::neon64> : platform<cpu_t::neon>
{
};
#endif

inline namespace CMT_ARCH_NAME
{

/// @brief SIMD vector width for the given cpu instruction set
template <typename T>
constexpr static size_t vector_width =
    (const_max(size_t(1), typeclass<T> == datatype::f ? platform<>::native_float_vector_size / sizeof(T)
                                                      : platform<>::native_int_vector_size / sizeof(T)));

template <typename T, cpu_t cpu>
constexpr static size_t vector_width_for =
    (const_max(size_t(1), typeclass<T> == datatype::f ? platform<cpu>::native_float_vector_size / sizeof(T)
                                                      : platform<cpu>::native_int_vector_size / sizeof(T)));

template <typename T>
constexpr static size_t minimum_vector_width =
    (const_max(size_t(1), typeclass<T> == datatype::f ? platform<>::minimum_float_vector_size / sizeof(T)
                                                      : platform<>::minimum_int_vector_size / sizeof(T)));

template <typename T>
constexpr static size_t vector_capacity = platform<>::simd_register_count * vector_width<T>;

#ifdef CMT_COMPILER_IS_MSVC
template <typename T>
constexpr static size_t maximum_vector_size = const_min(static_cast<size_t>(32), vector_width<T> * 2);
#else
template <typename T>
constexpr static size_t maximum_vector_size = const_min(
    static_cast<size_t>(32), const_max(size_t(1), platform<>::simd_register_count / 4) * vector_width<T>);
#endif

template <typename T>
constexpr static bool is_simd_size(size_t size)
{
    return is_poweroftwo(size) && size >= minimum_vector_width<T> && size <= vector_width<T>;
}

template <typename T, size_t N = vector_width<T>>
struct vec;

template <typename T, size_t N = vector_width<T>>
using mask = vec<bit<T>, N>;

} // namespace CMT_ARCH_NAME
} // namespace kfr
