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

#include "../platform.hpp"

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

#if defined CMT_COMPILER_GNU
constexpr f32 allones_f32() CMT_NOEXCEPT { return -__builtin_nanf("0xFFFFFFFF"); }
constexpr f64 allones_f64() CMT_NOEXCEPT { return -__builtin_nan("0xFFFFFFFFFFFFFFFF"); }
constexpr f32 invhighbit_f32() CMT_NOEXCEPT { return __builtin_nanf("0x7FFFFFFF"); }
constexpr f64 invhighbit_f64() CMT_NOEXCEPT { return __builtin_nan("0x7FFFFFFFFFFFFFFF"); }
#elif defined CMT_COMPILER_MSVC
constexpr f32 allones_f32() CMT_NOEXCEPT { return -__builtin_nanf("-1"); }
constexpr f64 allones_f64() CMT_NOEXCEPT { return -__builtin_nan("-1"); }
constexpr f32 invhighbit_f32() CMT_NOEXCEPT { return __builtin_nanf("-1"); }
constexpr f64 invhighbit_f64() CMT_NOEXCEPT { return __builtin_nan("-1"); }
#else
inline f32 allones_f32() CMT_NOEXCEPT
{
    return _mm_cvtss_f32(_mm_castsi128_ps(_mm_cvtsi32_si128(0xFFFFFFFFu)));
}
inline f64 allones_f64() CMT_NOEXCEPT
{
    return _mm_cvtsd_f64(_mm_castsi128_pd(_mm_cvtsi64x_si128(0xFFFFFFFFFFFFFFFFull)));
}
inline f32 invhighbit_f32() CMT_NOEXCEPT
{
    return _mm_cvtss_f32(_mm_castsi128_ps(_mm_cvtsi32_si128(0x7FFFFFFFu)));
}
inline f64 invhighbit_f64() CMT_NOEXCEPT
{
    return _mm_cvtsd_f64(_mm_castsi128_pd(_mm_cvtsi64x_si128(0x7FFFFFFFFFFFFFFFull)));
}
#endif

template <typename T>
struct special_scalar_constants
{
    constexpr static T highbitmask() { return static_cast<T>(1ull << (sizeof(T) * 8 - 1)); }
    constexpr static T allones() { return static_cast<T>(-1ll); }
    constexpr static T allzeros() { return T(0); }
    constexpr static T invhighbitmask() { return static_cast<T>((1ull << (sizeof(T) * 8 - 1)) - 1); }
};

#ifndef CMT_COMPILER_INTEL
#define KFR_CONSTEXPR_NON_INTEL constexpr
#else
#define KFR_CONSTEXPR_NON_INTEL
#endif

template <>
struct special_scalar_constants<float>
{
    constexpr static float highbitmask() { return -0.f; }
    KFR_CONSTEXPR_NON_INTEL static float allones() noexcept { return allones_f32(); };
    constexpr static float allzeros() { return 0.f; }
    KFR_CONSTEXPR_NON_INTEL static float invhighbitmask() { return invhighbit_f32(); }
};

template <>
struct special_scalar_constants<double>
{
    constexpr static double highbitmask() { return -0.; }
    KFR_CONSTEXPR_NON_INTEL static double allones() noexcept { return allones_f64(); };
    constexpr static double allzeros() { return 0.; }
    KFR_CONSTEXPR_NON_INTEL static double invhighbitmask() { return invhighbit_f64(); }
};

template <typename T>
struct special_constants : public special_scalar_constants<subtype<T>>
{
public:
    using Tsub = subtype<T>;
};

namespace intrinsics
{

template <typename T, size_t N>
struct simd_t
{
    using value_type = T;

    constexpr static size_t size() { return N; }
};

template <typename T, size_t N1, size_t N2>
struct simd2_t
{
    using value_type = T;

    constexpr static size_t size1() { return N1; }

    constexpr static size_t size2() { return N2; }
};

template <typename Tout, typename Tin, size_t N>
struct simd_cvt_t
{
    using value_type_out = Tout;
    using value_type_in  = Tin;

    constexpr static size_t size() { return N; }
};

template <typename T, size_t N>
constexpr size_t alignment()
{
    return const_min(size_t(platform<>::native_vector_alignment), next_poweroftwo(sizeof(T) * N));
}

template <typename T, size_t N>
struct alignas(alignment<T, N>()) simd_array
{
    T val[next_poweroftwo(N)];
};

template <typename T, size_t N>
struct simd_type;

template <typename T>
struct simd_type<T, 0>
{
    // SFINAE
};

template <typename T, size_t N>
struct simd_halves
{
    using subtype = typename simd_type<T, prev_poweroftwo(N - 1)>::type;

    subtype low;
    subtype high;
#if KFR_DEFINE_CTORS_FOR_HALVES
    simd_halves() CMT_NOEXCEPT {}
    simd_halves(const subtype& l, const subtype& h) CMT_NOEXCEPT : low(l), high(h) {}
    simd_halves(const simd_halves& v) CMT_NOEXCEPT : low(v.low), high(v.high) {}
    simd_halves(simd_halves&& v) CMT_NOEXCEPT : low(v.low), high(v.high) {}

    simd_halves& operator=(const simd_halves& v) CMT_NOEXCEPT
    {
        low  = v.low;
        high = v.high;
        return *this;
    }
    simd_halves& operator=(simd_halves&& v) CMT_NOEXCEPT
    {
        low  = v.low;
        high = v.high;
        return *this;
    }
#endif
};

} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr
