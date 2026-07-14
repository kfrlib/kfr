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

#include "types.hpp"

namespace kfr
{

/**
 * @brief Enumeration representing a CPU instruction set.
 *
 * Values are ordered from least capable to most capable within each architecture.
 * The special value @ref runtime indicates that the target CPU is selected at
 * run time. @ref native resolves to the instruction set KFR was compiled for,
 * and @ref secondary selects the secondary platform used by the multi-platform
 * build (sse42 when the primary is AVX, otherwise equal to @ref native).
 */
enum class cpu_t : int
{
    generic = 0,
#ifdef KFR_ARCH_X86
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
#ifdef KFR_ARCH_ARM
    neon    = 1,
    neon64  = 2,
    lowest  = static_cast<int>(neon),
    highest = static_cast<int>(neon64),
#endif
#ifdef KFR_ARCH_RISCV
    rvv     = 1,
    lowest  = static_cast<int>(rvv),
    highest = static_cast<int>(rvv),
#endif
    native = static_cast<int>(KFR_ARCH_NAME),

#ifdef KFR_ARCH_AVX
#define KFR_HAS_SECONDARY_PLATFORM
    secondary = static_cast<int>(sse42),
#else
    secondary = static_cast<int>(native),
#endif

    common  = generic, // For compatibility
    runtime = -1,
};

/**
 * @brief Compile-time wrapper around a @ref cpu_t value.
 * @tparam cpu The CPU instruction set to wrap.
 */
template <cpu_t cpu>
using ccpu_t = cval_t<cpu_t, cpu>;

/**
 * @brief Compile-time constant instance of @ref ccpu_t.
 * @tparam cpu The CPU instruction set to wrap.
 */
template <cpu_t cpu>
constexpr ccpu_t<cpu> ccpu{};

namespace internal_generic
{
constexpr cpu_t older(cpu_t x) { return static_cast<cpu_t>(static_cast<int>(x) - 1); }
constexpr cpu_t newer(cpu_t x) { return static_cast<cpu_t>(static_cast<int>(x) + 1); }

#ifdef KFR_ARCH_X86
constexpr auto cpu_list = cvals_t<cpu_t, cpu_t::avx512, cpu_t::avx2, cpu_t::avx1, cpu_t::sse41, cpu_t::ssse3,
                                  cpu_t::sse3, cpu_t::sse2>();
#endif
#ifdef KFR_ARCH_ARM
constexpr auto cpu_list = cvals<cpu_t, cpu_t::neon>;
#endif
#ifdef KFR_ARCH_RISCV
constexpr auto cpu_list = cvals<cpu_t, cpu_t::rvv>;
#endif
} // namespace internal_generic

/**
 * @brief Compile-time wrapper around a @ref cpu_t value (alias).
 * @tparam cpu The CPU instruction set to wrap.
 */
template <cpu_t cpu>
using cpuval_t = cval_t<cpu_t, cpu>;
/**
 * @brief Compile-time constant instance of @ref cpuval_t.
 * @tparam cpu The CPU instruction set to wrap.
 */
template <cpu_t cpu>
constexpr auto cpuval = cpuval_t<cpu>{};

/**
 * @brief Compile-time list of all CPU instruction sets supported by the current
 *        architecture that are not newer than @ref cpu_t::native.
 */
constexpr auto cpu_all =
    cfilter(internal_generic::cpu_list, internal_generic::cpu_list >= cpuval_t<cpu_t::native>());

/**
 * @brief Returns the name of the given CPU instruction set.
 * @param set The CPU instruction set whose name is requested.
 * @return Pointer to a static string with the instruction set name, or "-" if
 *         @p set is out of the supported range.
 */
KFR_UNUSED static const char* cpu_name(cpu_t set)
{
#ifdef KFR_ARCH_X86
    static const char* names[] = { "generic", "sse2", "sse3", "ssse3", "sse41",
                                   "sse42",   "avx",  "avx2", "avx512" };
#endif
#ifdef KFR_ARCH_ARM
    static const char* names[] = { "generic", "neon", "neon64" };
#endif
#ifdef KFR_ARCH_RISCV
    static const char* names[] = { "generic", "rvv" };
#endif
    if (KFR_LIKELY(set >= cpu_t::lowest && set <= cpu_t::highest))
        return names[static_cast<size_t>(set)];
    return "-";
}

#ifdef KFR_ARCH_X64
/**
 * @brief Selects between 32-bit and 64-bit compile-time constants.
 *
 * This overload, selected when the dummy template argument defaults to `int`, is
 * used for string literals.
 * @param x32 Value to use on a 32-bit build (ignored on x64 builds).
 * @param x64 Value to use on a 64-bit build.
 * @return @p x64 on x64 builds.
 */
template <int = 0>
constexpr inline const char* bitness_const(const char*, const char* x64)
{
    return x64;
}
/**
 * @brief Selects between 32-bit and 64-bit compile-time constants.
 * @tparam T The type of the values.
 * @param x32 Value to use on a 32-bit build (ignored on x64 builds).
 * @param x64 Value to use on a 64-bit build.
 * @return @p x64 on x64 builds.
 */
template <typename T>
constexpr inline const T& bitness_const(const T&, const T& x64)
{
    return x64;
}
#else
/**
 * @brief Selects between 32-bit and 64-bit compile-time constants.
 *
 * This overload, selected when the dummy template argument defaults to `int`, is
 * used for string literals.
 * @param x32 Value to use on a 32-bit build.
 * @param x64 Value to use on a 64-bit build (ignored on 32-bit builds).
 * @return @p x32 on non-x64 builds.
 */
template <int = 0>
constexpr inline const char* bitness_const(const char* x32, const char*)
{
    return x32;
}
/**
 * @brief Selects between 32-bit and 64-bit compile-time constants.
 * @tparam T The type of the values.
 * @param x32 Value to use on a 32-bit build.
 * @param x64 Value to use on a 64-bit build (ignored on 32-bit builds).
 * @return @p x32 on non-x64 builds.
 */
template <typename T>
constexpr inline const T& bitness_const(const T& x32, const T&)
{
    return x32;
}
#endif

/**
 * @brief Trait structure describing the SIMD capabilities of a CPU instruction
 *        set.
 *
 * Specializations expose a set of compile-time constants such as vector sizes,
 * alignment requirements, register counts and feature flags used throughout
 * KFR to select the appropriate code paths.
 * @tparam c The CPU instruction set to describe (defaults to @ref cpu_t::native).
 */
template <cpu_t c = cpu_t::native>
struct platform;

#ifdef KFR_ARCH_X86
/**
 * @brief Platform traits for the common (baseline) x86 instruction set.
 *
 * Defines the minimum SIMD capabilities shared by all x86 targets.
 */
template <>
struct platform<cpu_t::common>
{
    constexpr static size_t native_cache_alignment        = KFR_CACHE_LINE_SIZE;
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
/**
 * @brief Platform traits for SSE2. Inherits the common baseline and reports
 *        the SSE2 register count.
 */
template <>
struct platform<cpu_t::sse2> : platform<cpu_t::common>
{
    constexpr static size_t simd_register_count = bitness_const(8, 16);
};
/**
 * @brief Platform traits for SSE3. Equivalent to SSE2.
 */
template <>
struct platform<cpu_t::sse3> : platform<cpu_t::sse2>
{
};
/**
 * @brief Platform traits for SSSE3. Equivalent to SSE3.
 */
template <>
struct platform<cpu_t::ssse3> : platform<cpu_t::sse3>
{
};
/**
 * @brief Platform traits for SSE4.1. Equivalent to SSSE3.
 */
template <>
struct platform<cpu_t::sse41> : platform<cpu_t::ssse3>
{
};
/**
 * @brief Platform traits for SSE4.2. Equivalent to SSE4.1.
 */
template <>
struct platform<cpu_t::sse42> : platform<cpu_t::sse41>
{
};
/**
 * @brief Platform traits for AVX. Doubles the native float vector size and
 *        alignment to 32 bytes and enables fast unaligned access.
 */
template <>
struct platform<cpu_t::avx> : platform<cpu_t::sse42>
{
    constexpr static size_t native_float_vector_size = 32;

    constexpr static size_t native_vector_alignment      = 32;
    constexpr static size_t native_vector_alignment_mask = native_vector_alignment - 1;

    constexpr static bool fast_unaligned = true;
};
/**
 * @brief Platform traits for AVX2. Extends AVX with 256-bit integer vectors.
 */
template <>
struct platform<cpu_t::avx2> : platform<cpu_t::avx>
{
    constexpr static size_t native_int_vector_size = 32;
};
/**
 * @brief Platform traits for AVX-512. Provides 512-bit vectors, 64-byte
 *        alignment, mask register support and an increased register count.
 */
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
#ifdef KFR_ARCH_ARM
/**
 * @brief Platform traits for the common (baseline) ARM instruction set.
 */
template <>
struct platform<cpu_t::common>
{
    constexpr static size_t native_cache_alignment        = KFR_CACHE_LINE_SIZE;
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
/**
 * @brief Platform traits for NEON (AArch32). Reports 32 SIMD registers.
 */
template <>
struct platform<cpu_t::neon> : platform<cpu_t::common>
{
    constexpr static size_t simd_register_count = 32;
};
/**
 * @brief Platform traits for NEON for AArch64. Equivalent to NEON.
 */
template <>
struct platform<cpu_t::neon64> : platform<cpu_t::neon>
{
};
#endif

#ifdef KFR_ARCH_RISCV
/**
 * @brief Platform traits for the common (baseline) RISC-V instruction set.
 */
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
/**
 * @brief Platform traits for the RISC-V Vector extension (RVV). Reports 16
 *        vector registers.
 */
template <>
struct platform<cpu_t::rvv> : platform<cpu_t::common>
{
    constexpr static size_t simd_register_count = 16;
};
#endif

inline namespace KFR_ARCH_NAME
{

/**
 * @brief SIMD vector width (in elements) for type @p T on the native platform.
 *
 * Computed from the native float or integer vector size divided by
 * `sizeof(T)`, clamped to a minimum of 1.
 * @tparam T The element type.
 */
template <typename T>
constexpr static size_t vector_width =
    (std::max(size_t(1), typeclass<T> == datatype::f ? platform<>::native_float_vector_size / sizeof(T)
                                                     : platform<>::native_int_vector_size / sizeof(T)));

/**
 * @brief SIMD vector width (in elements) for type @p T on the given CPU.
 * @tparam T The element type.
 * @tparam cpu The CPU instruction set to query.
 */
template <typename T, cpu_t cpu>
constexpr static size_t vector_width_for =
    (std::max(size_t(1), typeclass<T> == datatype::f ? platform<cpu>::native_float_vector_size / sizeof(T)
                                                     : platform<cpu>::native_int_vector_size / sizeof(T)));

/**
 * @brief Minimum SIMD vector width (in elements) for type @p T on the native
 *        platform.
 * @tparam T The element type.
 */
template <typename T>
constexpr static size_t minimum_vector_width =
    (std::max(size_t(1), typeclass<T> == datatype::f ? platform<>::minimum_float_vector_size / sizeof(T)
                                                     : platform<>::minimum_int_vector_size / sizeof(T)));

/**
 * @brief Total SIMD capacity (in elements) for type @p T on the native platform.
 *
 * Equals the number of SIMD registers multiplied by @ref vector_width.
 * @tparam T The element type.
 */
template <typename T>
constexpr static size_t vector_capacity = platform<>::simd_register_count * vector_width<T>;

/**
 * @brief Maximum SIMD vector size (in elements) used by KFR for type @p T.
 *
 * On MSVC this is `vector_width<T> * 2` capped at 32; on other compilers it is
 * derived from the register count.
 * @tparam T The element type.
 */
#ifdef KFR_COMPILER_IS_MSVC
template <typename T>
constexpr static size_t maximum_vector_size = std::min(static_cast<size_t>(32), vector_width<T> * 2);
#else
template <typename T>
constexpr static size_t maximum_vector_size = std::min(
    static_cast<size_t>(32), std::max(size_t(1), platform<>::simd_register_count / 4) * vector_width<T>);
#endif

/**
 * @brief Checks whether @p size is a valid SIMD vector size for type @p T.
 *
 * A size is valid when it is a power of two and lies within the range
 * [`minimum_vector_width<T>`, `vector_width<T>`].
 * @tparam T The element type.
 * @param size The candidate vector size (in elements).
 * @return `true` if @p size is a supported SIMD vector size.
 */
template <typename T>
constexpr static bool is_simd_size(size_t size)
{
    return is_poweroftwo(size) && size >= minimum_vector_width<T> && size <= vector_width<T>;
}

/**
 * @brief SIMD vector type of @p T elements with default width @ref vector_width.
 * @tparam T The element type.
 * @tparam N The vector width in elements (defaults to @ref vector_width<T>).
 */
template <typename T, size_t N = vector_width<T>>
struct vec;

/**
 * @brief SIMD mask type of @p T elements with default width @ref vector_width.
 * @tparam T The element type.
 * @tparam N The vector width in elements (defaults to @ref vector_width<T>).
 */
template <typename T, size_t N = vector_width<T>>
using mask = vec<bit<T>, N>;

#ifdef KFR_ARCH_AVX512
/**
 * @brief Maps an element type @p T to its native AVX-512 SIMD register type.
 * @tparam T The element type.
 */
template <typename T>
struct native_vector_type
{
    using type = __m512i;
};
template <>
struct native_vector_type<float>
{
    using type = __m512;
};
template <>
struct native_vector_type<double>
{
    using type = __m512d;
};
#elif defined KFR_ARCH_AVX
/**
 * @brief Maps an element type @p T to its native AVX SIMD register type.
 * @tparam T The element type.
 */
template <typename T>
struct native_vector_type
{
    using type = __m256i;
};
template <>
struct native_vector_type<float>
{
    using type = __m256;
};
template <>
struct native_vector_type<double>
{
    using type = __m256d;
};
#elif defined KFR_ARCH_SSE2

/**
 * @brief Maps an element type @p T to its native SSE2 SIMD register type.
 * @tparam T The element type.
 */
template <typename T>
struct native_vector_type
{
    using type = __m128i;
};
template <>
struct native_vector_type<float>
{
    using type = __m128;
};
template <>
struct native_vector_type<double>
{
    using type = __m128d;
};
#elif defined KFR_ARCH_NEON
/**
 * @brief Maps an element type @p T to its native NEON SIMD register type.
 * @tparam T The element type.
 */
template <typename T>
struct native_vector_type
{
};
template <>
struct native_vector_type<float>
{
    using type = float32x4_t;
};
#ifdef __aarch64__
template <>
struct native_vector_type<double>
{
    using type = float64x2_t;
};
#endif
template <>
struct native_vector_type<int64_t>
{
    using type = int64x2_t;
};
template <>
struct native_vector_type<uint64_t>
{
    using type = uint64x2_t;
};
template <>
struct native_vector_type<int32_t>
{
    using type = int32x4_t;
};
template <>
struct native_vector_type<uint32_t>
{
    using type = uint32x4_t;
};
template <>
struct native_vector_type<int16_t>
{
    using type = int16x8_t;
};
template <>
struct native_vector_type<uint16_t>
{
    using type = uint16x8_t;
};
template <>
struct native_vector_type<int8_t>
{
    using type = int8x16_t;
};
template <>
struct native_vector_type<uint8_t>
{
    using type = uint8x16_t;
};

#elif defined KFR_ARCH_RVV
/**
 * @brief Maps an element type @p T to its native RISC-V Vector (RVV) register
 *        type.
 * @tparam T The element type.
 */
template <typename T>
struct native_vector_type
{
};
template <>
struct native_vector_type<float>
{
    using type = vfloat32m1_t;
};
template <>
struct native_vector_type<double>
{
    using type = vfloat64m1_t;
};
template <>
struct native_vector_type<int64_t>
{
    using type = vint64m1_t;
};
template <>
struct native_vector_type<uint64_t>
{
    using type = vuint64m1_t;
};
template <>
struct native_vector_type<int32_t>
{
    using type = vint32m1_t;
};
template <>
struct native_vector_type<uint32_t>
{
    using type = vuint32m1_t;
};
template <>
struct native_vector_type<int16_t>
{
    using type = vint16m1_t;
};
template <>
struct native_vector_type<uint16_t>
{
    using type = vuint16m1_t;
};
template <>
struct native_vector_type<int8_t>
{
    using type = vint8m1_t;
};
template <>
struct native_vector_type<uint8_t>
{
    using type = vuint8m1_t;
};

#endif

} // namespace KFR_ARCH_NAME
} // namespace kfr
