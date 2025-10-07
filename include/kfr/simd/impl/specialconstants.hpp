/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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

#include <bit>
#include "../../meta/numeric.hpp"
#include "intrinsics.h"

namespace kfr
{
using namespace kfr;

#if defined KFR_COMPILER_GNU
constexpr f32 allones_f32() noexcept { return -__builtin_nanf("0xFFFFFFFF"); }
constexpr f64 allones_f64() noexcept { return -__builtin_nan("0xFFFFFFFFFFFFFFFF"); }
constexpr f32 invhighbit_f32() noexcept { return __builtin_nanf("0x7FFFFFFF"); }
constexpr f64 invhighbit_f64() noexcept { return __builtin_nan("0x7FFFFFFFFFFFFFFF"); }
#elif defined KFR_COMPILER_MSVC
constexpr f32 allones_f32() noexcept { return -__builtin_nanf("-1"); }
constexpr f64 allones_f64() noexcept { return -__builtin_nan("-1"); }
constexpr f32 invhighbit_f32() noexcept { return __builtin_nanf("-1"); }
constexpr f64 invhighbit_f64() noexcept { return __builtin_nan("-1"); }
#else
constexpr f32 allones_f32() noexcept { return std::bit_cast<f32>(0xffffffffu); }
constexpr f64 allones_f64() noexcept { return std::bit_cast<f64>(0xffffffffffffffffull); }
constexpr f32 invhighbit_f32() noexcept { return std::bit_cast<f32>(0x7fffffffu); }
constexpr f64 invhighbit_f64() noexcept { return std::bit_cast<f64>(0x7fffffffffffffffull); }
#endif

template <typename T>
struct special_scalar_constants
{
    constexpr static T highbitmask() { return static_cast<T>(1ull << (sizeof(T) * 8 - 1)); }
    constexpr static T allones() { return static_cast<T>(-1ll); }
    constexpr static T allzeros() { return T(0); }
    constexpr static T invhighbitmask() { return static_cast<T>((1ull << (sizeof(T) * 8 - 1)) - 1); }
};

template <>
struct special_scalar_constants<float>
{
    constexpr static float highbitmask() { return -0.f; }
    constexpr static float allones() noexcept { return allones_f32(); }
    constexpr static float allzeros() { return 0.f; }
    constexpr static float invhighbitmask() { return invhighbit_f32(); }
};

template <>
struct special_scalar_constants<double>
{
    constexpr static double highbitmask() { return -0.; }
    constexpr static double allones() noexcept { return allones_f64(); }
    constexpr static double allzeros() { return 0.; }
    constexpr static double invhighbitmask() { return invhighbit_f64(); }
};

template <typename T>
struct special_constants : public special_scalar_constants<subtype<T>>
{
public:
    using Tsub = subtype<T>;
};

} // namespace kfr
