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

#include <bit>
#include "../../meta/numeric.hpp"
#include "intrinsics.h"

namespace kfr
{

namespace internal_generic
{

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

} // namespace internal_generic

} // namespace kfr
