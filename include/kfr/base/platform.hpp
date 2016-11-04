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

#include "types.hpp"

namespace kfr
{

/// @brief An enumeration representing cpu instruction set
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
    cvals_t<cpu_t, cpu_t::avx2, cpu_t::avx1, cpu_t::sse41, cpu_t::ssse3, cpu_t::sse3, cpu_t::sse2>();
#else
constexpr auto cpu_list = cvals<cpu_t, cpu_t::neon>;
#endif
}

template <cpu_t cpu>
using cpuval_t = cval_t<cpu_t, cpu>;
template <cpu_t cpu>
constexpr auto cpuval = cpuval_t<cpu>{};

constexpr auto cpu_all = cfilter(internal::cpu_list, internal::cpu_list >= cpuval_t<cpu_t::native>());

/// @brief Returns name of the cpu instruction set
CMT_UNUSED static const char* cpu_name(cpu_t set)
{
    static const char* names[] = { "common", "sse2", "sse3", "ssse3", "sse41", "sse42", "avx1", "avx2" };
    if (set >= cpu_t::lowest && set <= cpu_t::highest)
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
template <int           = 0>
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

template <typename T = i32, cpu_t c = cpu_t::native>
struct platform
{
    constexpr static size_t native_cache_alignment        = 64;
    constexpr static size_t native_cache_alignment_mask   = native_cache_alignment - 1;
    constexpr static size_t maximum_vector_alignment      = 32;
    constexpr static size_t maximum_vector_alignment_mask = maximum_vector_alignment - 1;
#ifdef CMT_ARCH_X86
    constexpr static size_t simd_register_count = bitness_const(8, 16);
#endif
#ifdef CMT_ARCH_ARM
    constexpr static size_t simd_register_count = 16;
#endif

    constexpr static size_t common_float_vector_size = 16;
    constexpr static size_t common_int_vector_size   = 16;

#ifdef CMT_ARCH_X86
    constexpr static size_t native_float_vector_size =
        c >= cpu_t::avx1 ? 32 : c >= cpu_t::sse2 ? 16 : common_float_vector_size;
#endif
#ifdef CMT_ARCH_ARM
    constexpr static size_t native_float_vector_size = c == cpu_t::neon ? 16 : common_float_vector_size;
#endif
#ifdef CMT_ARCH_X86
    constexpr static size_t native_int_vector_size =
        c >= cpu_t::avx2 ? 32 : c >= cpu_t::sse2 ? 16 : common_int_vector_size;
#endif
#ifdef CMT_ARCH_ARM
    constexpr static size_t native_int_vector_size = c == cpu_t::neon ? 16 : common_int_vector_size;
#endif

    /// @brief SIMD vector width for the given cpu instruction set
    constexpr static size_t vector_width =
        (const_max(size_t(1), typeclass<T> == datatype::f ? native_float_vector_size / sizeof(T)
                                                          : native_int_vector_size / sizeof(T)));

    constexpr static size_t vector_capacity = simd_register_count * vector_width;

    constexpr static size_t maximum_vector_size = const_min(static_cast<size_t>(32), vector_capacity / 4);

    constexpr static size_t native_vector_alignment =
        const_max(native_float_vector_size, native_int_vector_size);

    constexpr static bool fast_unaligned =
#ifdef CMT_ARCH_X86
        c >= cpu_t::avx1;
#else
        false;
#endif

    constexpr static size_t native_vector_alignment_mask = native_vector_alignment - 1;
};

template <typename T, size_t N = platform<T>::vector_width>
struct vec;
template <typename T, size_t N = platform<T>::vector_width>
struct mask;
}
