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

#include "../constants.hpp"
#include "../platform.hpp"

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

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
struct alignas(force_compiletime_size_t<alignment<T, N>()>) simd_array
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
#if defined KFR_DEFINE_CTORS_FOR_HALVES && KFR_DEFINE_CTORS_FOR_HALVES
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

#define KFR_COMPONENTWISE_RET(code)                                                                          \
    vec<T, N> result;                                                                                        \
    for (size_t i = 0; i < N; i++)                                                                           \
        code;                                                                                                \
    return result;

#define KFR_COMPONENTWISE_RET_I(Tvec, code)                                                                  \
    Tvec result;                                                                                             \
    for (size_t i = 0; i < result.size(); i++)                                                               \
        code;                                                                                                \
    return result;

#define KFR_COMPONENTWISE(code)                                                                              \
    for (size_t i = 0; i < N; i++)                                                                           \
        code;

} // namespace kfr
