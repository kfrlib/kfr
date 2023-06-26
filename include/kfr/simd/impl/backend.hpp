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

#include "simd.hpp"
#ifdef CMT_CLANG_EXT
#include "backend_clang.hpp"
#else
#include "backend_generic.hpp"
#endif

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

#ifdef KFR_AUTOTESTS
template <typename T>
struct check_sizes
{
    static_assert(sizeof(simd<T, 1>) == sizeof(T), "");
    static_assert(sizeof(simd<T, 2>) == sizeof(T) * 2, "");
    static_assert(sizeof(simd<T, 3>) == sizeof(T) * 4, "");
    static_assert(sizeof(simd<T, 4>) == sizeof(T) * 4, "");
    static_assert(sizeof(simd<T, 5>) == sizeof(T) * 8, "");
    static_assert(sizeof(simd<T, 6>) == sizeof(T) * 8, "");
    static_assert(sizeof(simd<T, 7>) == sizeof(T) * 8, "");
    static_assert(sizeof(simd<T, 8>) == sizeof(T) * 8, "");
    static_assert(sizeof(simd<T, 16>) == sizeof(T) * 16, "");
    static_assert(sizeof(simd<T, 32>) == sizeof(T) * 32, "");
    static_assert(sizeof(simd<T, 64>) == sizeof(T) * 64, "");
    static_assert(sizeof(simd<T, 128>) == sizeof(T) * 128, "");
    static_assert(sizeof(simd<T, 256>) == sizeof(T) * 256, "");
    static_assert(sizeof(simd<T, 512>) == sizeof(T) * 512, "");
    static_assert(sizeof(simd<T, 513>) == sizeof(T) * 1024, "");
    static_assert(sizeof(simd<T, 1023>) == sizeof(T) * 1024, "");
    static_assert(sizeof(simd<T, 1024>) == sizeof(T) * 1024, "");
};

template struct check_sizes<float>;
template struct check_sizes<double>;
template struct check_sizes<uint8_t>;
template struct check_sizes<uint16_t>;
template struct check_sizes<uint32_t>;
template struct check_sizes<uint64_t>;
template struct check_sizes<int8_t>;
template struct check_sizes<int16_t>;
template struct check_sizes<int32_t>;
template struct check_sizes<int64_t>;

#endif
} // namespace intrinsics
} // namespace CMT_ARCH_NAME

using CMT_ARCH_NAME::intrinsics::simd;
} // namespace kfr
