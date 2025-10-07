/** @addtogroup audio
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

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>

namespace kfr
{

using ieee80 = std::array<std::byte, 10>;

ieee80 ieee754_write_extended(double in)
{
    ieee80 out{};
    int sgn = 0, exp = 0, shift = 0;
    double fraction, t;
    unsigned int lexp, hexp;
    uint32_t low = 0, high = 0;

    if (in == 0.0)
    {
        return out;
    }

    if (in < 0.0)
    {
        in  = std::fabs(in);
        sgn = 1;
    }

    fraction = std::frexp(in, &exp);

    if (exp == 0 || exp > 16384)
    {
        if (exp > 16384)
        {
            low = high = 0;
        }
        else
        {
            low  = 0x80000000;
            high = 0;
        }
        exp = 32767;
    }
    else
    {
        fraction = std::ldexp(fraction, 32);
        t        = std::floor(fraction);
        low      = static_cast<uint32_t>(t);
        fraction -= t;
        t    = std::floor(std::ldexp(fraction, 32));
        high = static_cast<uint32_t>(t);

        if (exp < -16382)
        {
            shift = -exp - 16382;
            high >>= shift;
            high |= (low << (32 - shift));
            low >>= shift;
            exp = -16382;
        }
        exp += 16383 - 1;
    }

    lexp = static_cast<unsigned int>(exp) >> 8;
    hexp = static_cast<unsigned int>(exp) & 0xFF;

    out[9 - 0] = static_cast<std::byte>((sgn << 7) | (lexp & 0x7F));
    out[9 - 1] = static_cast<std::byte>(hexp);
    out[9 - 2] = static_cast<std::byte>(low >> 24);
    out[9 - 3] = static_cast<std::byte>((low >> 16) & 0xFF);
    out[9 - 4] = static_cast<std::byte>((low >> 8) & 0xFF);
    out[9 - 5] = static_cast<std::byte>(low & 0xFF);
    out[9 - 6] = static_cast<std::byte>(high >> 24);
    out[9 - 7] = static_cast<std::byte>((high >> 16) & 0xFF);
    out[9 - 8] = static_cast<std::byte>((high >> 8) & 0xFF);
    out[9 - 9] = static_cast<std::byte>(high & 0xFF);

    return out;
}

double ieee754_read_extended(const ieee80& in)
{
    int sgn, exp;
    uint32_t low, high;
    double out;

    sgn = static_cast<int>(std::to_integer<uint8_t>(in[9 - 0]) >> 7);
    exp = ((static_cast<int>(std::to_integer<uint8_t>(in[9 - 0]) & 0x7F)) << 8) |
          static_cast<int>(std::to_integer<uint8_t>(in[9 - 1]));

    low = (static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 2])) << 24) |
          (static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 3])) << 16) |
          (static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 4])) << 8) |
          static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 5]));

    high = (static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 6])) << 24) |
           (static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 7])) << 16) |
           (static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 8])) << 8) |
           static_cast<uint32_t>(std::to_integer<uint8_t>(in[9 - 9]));

    if (exp == 0 && low == 0 && high == 0)
    {
        return sgn ? -0.0 : 0.0;
    }

    if (exp == 32767)
    {
        if (low == 0 && high == 0)
        {
            return sgn ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
        }
        else
        {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    exp -= 16383;

    out = std::ldexp(static_cast<double>(low), -31 + exp);
    out += std::ldexp(static_cast<double>(high), -63 + exp);

    return sgn ? -out : out;
}

struct float80_t
{
    float80_t()                 = default;
    float80_t(const float80_t&) = default;
    float80_t(double val) { raw = ieee754_write_extended(val); }

    operator double() const { return ieee754_read_extended(raw); }
    ieee80 raw;
};

} // namespace kfr
