/** @addtogroup base
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

#include "../cometa/string.hpp"
#include "../simd/types.hpp"

namespace kfr
{

struct fraction
{
    fraction(i64 num = 0, i64 den = 1) : numerator(num), denominator(den) { normalize(); }
    void normalize()
    {
        if (CMT_UNLIKELY(denominator < 0))
        {
            denominator = -denominator;
            numerator   = -numerator;
        }
        const i64 z = gcd(std::abs(numerator), std::abs(denominator));
        numerator /= z;
        denominator /= z;
    }

    i64 numerator;
    i64 denominator;

    fraction operator+() const { return *this; }
    fraction operator-() const { return fraction(-numerator, denominator); }

    explicit operator bool() const { return numerator != 0; }
    explicit operator double() const { return static_cast<double>(numerator) / denominator; }
    explicit operator float() const { return static_cast<float>(numerator) / denominator; }
    explicit operator i64() const { return static_cast<i64>(numerator) / denominator; }

    friend fraction operator+(const fraction& x, const fraction& y)
    {
        return fraction(x.numerator * y.denominator + y.numerator * x.denominator,
                        x.denominator * y.denominator);
    }
    friend fraction operator-(const fraction& x, const fraction& y)
    {
        return fraction(x.numerator * y.denominator - y.numerator * x.denominator,
                        x.denominator * y.denominator);
    }
    friend fraction operator*(const fraction& x, const fraction& y)
    {
        return fraction(x.numerator * y.numerator, x.denominator * y.denominator);
    }
    friend fraction operator/(const fraction& x, const fraction& y)
    {
        return fraction(x.numerator * y.denominator, x.denominator * y.numerator);
    }

    friend bool operator==(const fraction& x, const fraction& y)
    {
        return x.numerator == y.numerator && x.denominator == y.denominator;
    }
    friend bool operator!=(const fraction& x, const fraction& y) { return !(operator==(x, y)); }
    friend bool operator<(const fraction& x, const fraction& y)
    {
        return x.numerator * y.denominator < y.numerator * x.denominator;
    }
    friend bool operator<=(const fraction& x, const fraction& y)
    {
        return x.numerator * y.denominator <= y.numerator * x.denominator;
    }
    friend bool operator>(const fraction& x, const fraction& y)
    {
        return x.numerator * y.denominator > y.numerator * x.denominator;
    }
    friend bool operator>=(const fraction& x, const fraction& y)
    {
        return x.numerator * y.denominator >= y.numerator * x.denominator;
    }

    fraction& operator+=(const fraction& y)
    {
        *this = *this + y;
        return *this;
    }
    fraction& operator-=(const fraction& y)
    {
        *this = *this - y;
        return *this;
    }
    fraction& operator*=(const fraction& y)
    {
        *this = *this * y;
        return *this;
    }
    fraction& operator/=(const fraction& y)
    {
        *this = *this / y;
        return *this;
    }

private:
    static i64 gcd(i64 a, i64 b)
    {
        i64 r;
        while (b > 0)
        {
            r = a % b;
            a = b;
            b = r;
        }
        return a;
    }
    static i64 lcm(i64 a, i64 b) { return std::abs(a * b) / gcd(a, b); }
};
} // namespace kfr

namespace cometa
{
template <>
struct representation<kfr::fraction>
{
    using type = std::string;
    static std::string get(const kfr::fraction& value)
    {
        if (value.denominator == 1)
            return as_string(value.numerator);
        else
            return as_string(value.numerator, "/", value.denominator);
    }
};
} // namespace cometa
