/** @addtogroup base
 *  @{
 */
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

#include "../meta/string.hpp"
#include "../simd/types.hpp"

namespace kfr
{

/// @brief Exact rational number represented as a numerator/denominator pair.
///
/// Always stored in normalized form: denominator > 0, and numerator and
/// denominator are coprime.
struct fraction
{
    /// @brief Construct a fraction from numerator and denominator.
    /// @param num Numerator (default 0).
    /// @param den Denominator (default 1); the fraction is normalized.
    fraction(i64 num = 0, i64 den = 1) noexcept : numerator(num), denominator(den) { normalize(); }

    /// @brief Reduce the fraction to lowest terms and enforce a positive denominator.
    void normalize() noexcept
    {
        if (KFR_UNLIKELY(denominator < 0))
        {
            denominator = -denominator;
            numerator   = -numerator;
        }
        const i64 z = std::gcd(std::abs(numerator), std::abs(denominator));
        numerator /= z;
        denominator /= z;
    }

    /// @brief Numerator of the fraction.
    i64 numerator;
    /// @brief Denominator of the fraction (always positive after normalization).
    i64 denominator;

    /// @brief Unary plus.
    fraction operator+() const noexcept { return *this; }
    /// @brief Unary minus (negates the numerator).
    fraction operator-() const noexcept { return fraction(-numerator, denominator); }

    /// @brief True if the fraction is non-zero.
    explicit operator bool() const noexcept { return numerator != 0; }
    /// @brief Convert to double (exact value).
    explicit operator double() const noexcept { return static_cast<double>(numerator) / denominator; }
    /// @brief Convert to float (exact value).
    explicit operator float() const noexcept { return static_cast<float>(numerator) / denominator; }
    /// @brief Convert to integer (truncates toward zero).
    explicit operator i64() const noexcept { return static_cast<i64>(numerator) / denominator; }

    /// @brief Add two fractions.
    friend fraction operator+(const fraction& x, const fraction& y) noexcept
    {
        return fraction(x.numerator * y.denominator + y.numerator * x.denominator,
                        x.denominator * y.denominator);
    }
    /// @brief Subtract two fractions.
    friend fraction operator-(const fraction& x, const fraction& y) noexcept
    {
        return fraction(x.numerator * y.denominator - y.numerator * x.denominator,
                        x.denominator * y.denominator);
    }
    /// @brief Multiply two fractions.
    friend fraction operator*(const fraction& x, const fraction& y) noexcept
    {
        return fraction(x.numerator * y.numerator, x.denominator * y.denominator);
    }
    /// @brief Divide two fractions.
    friend fraction operator/(const fraction& x, const fraction& y) noexcept
    {
        return fraction(x.numerator * y.denominator, x.denominator * y.numerator);
    }

    /// @brief Equality comparison.
    friend bool operator==(const fraction& x, const fraction& y) noexcept
    {
        return x.numerator == y.numerator && x.denominator == y.denominator;
    }
    /// @brief Inequality comparison.
    friend bool operator!=(const fraction& x, const fraction& y) noexcept { return !(x == y); }
    /// @brief Less-than comparison.
    friend bool operator<(const fraction& x, const fraction& y) noexcept
    {
        return x.numerator * y.denominator < y.numerator * x.denominator;
    }
    /// @brief Less-than-or-equal comparison.
    friend bool operator<=(const fraction& x, const fraction& y) noexcept
    {
        return x.numerator * y.denominator <= y.numerator * x.denominator;
    }
    /// @brief Greater-than comparison.
    friend bool operator>(const fraction& x, const fraction& y) noexcept
    {
        return x.numerator * y.denominator > y.numerator * x.denominator;
    }
    /// @brief Greater-than-or-equal comparison.
    friend bool operator>=(const fraction& x, const fraction& y) noexcept
    {
        return x.numerator * y.denominator >= y.numerator * x.denominator;
    }

    /// @brief Add-assign.
    fraction& operator+=(const fraction& y) noexcept
    {
        *this = *this + y;
        return *this;
    }
    /// @brief Subtract-assign.
    fraction& operator-=(const fraction& y) noexcept
    {
        *this = *this - y;
        return *this;
    }
    /// @brief Multiply-assign.
    fraction& operator*=(const fraction& y) noexcept
    {
        *this = *this * y;
        return *this;
    }
    /// @brief Divide-assign.
    fraction& operator/=(const fraction& y) noexcept
    {
        *this = *this / y;
        return *this;
    }

private:
};
} // namespace kfr

namespace kfr
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
} // namespace kfr
