/** @addtogroup shuffle
 *  @{
 */
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
#include "../simd/operators.hpp"
#include "../simd/read_write.hpp"
#include "expression.hpp"

namespace kfr
{

namespace details
{

template <typename T>
inline void convert_endianness(T& value)
{
    union
    {
        T val{ 0 };
        uint8_t raw[sizeof(T)];
    } u;
    for (size_t i = 0; i < sizeof(T); i++)
    {
        if constexpr (is_poweroftwo(sizeof(T)))
            u.raw[i] = reinterpret_cast<const uint8_t*>(&value)[i ^ (sizeof(T) - 1)];
        else
            u.raw[i] = reinterpret_cast<const uint8_t*>(&value)[i / sizeof(T) + (sizeof(T) - 1 - i)];
    }
    value = u.val;
}

KFR_INTRINSIC void convert_endianness(int16_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ushort(value);
#else
    value = __builtin_bswap16(value);
#endif
}

KFR_INTRINSIC void convert_endianness(uint16_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ushort(value);
#else
    value = __builtin_bswap16(value);
#endif
}

KFR_INTRINSIC void convert_endianness(int32_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ulong(value);
#else
    value = __builtin_bswap32(value);
#endif
}

KFR_INTRINSIC void convert_endianness(uint32_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_ulong(value);
#else
    value = __builtin_bswap32(value);
#endif
}

KFR_INTRINSIC void convert_endianness(int64_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_uint64(value);
#else
    value = __builtin_bswap64(value);
#endif
}

KFR_INTRINSIC void convert_endianness(uint64_t& value)
{
#ifdef KFR_COMPILER_IS_MSVC
    value = _byteswap_uint64(value);
#else
    value = __builtin_bswap64(value);
#endif
}

KFR_INTRINSIC void convert_endianness(float& value)
{
    uint32_t tmp = std::bit_cast<uint32_t>(value);
    convert_endianness(tmp);
    value = std::bit_cast<float>(tmp);
}

KFR_INTRINSIC void convert_endianness(double& value)
{
    uint64_t tmp = std::bit_cast<uint64_t>(value);
    convert_endianness(tmp);
    value = std::bit_cast<double>(tmp);
}

} // namespace details

template <typename T>
void convert_endianness(T* data, size_t size)
{
    block_process(size, csizes<2 * vector_width<T>, 1>,
                  [&](size_t i, auto w)
                  {
                      constexpr size_t width = KFR_CVAL(w);
                      vec<T, width> value    = read<width>(data + i);
                      value                  = swapbyteorder(value);
                      write(data + i, value);
                  });
}
} // namespace kfr
