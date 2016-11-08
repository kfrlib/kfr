#pragma once

#include <cmath>

struct double_double
{
    double hi, lo;

    constexpr double_double(double x) noexcept : hi(x), lo(0.0) {}
    constexpr double_double(float x) noexcept : hi(x), lo(0.0) {}
    constexpr double_double(double hi, double lo) noexcept : hi(hi + lo), lo((hi - (hi + lo)) + lo) {}
    constexpr operator double() const noexcept { return hi + lo; }
    constexpr operator float() const noexcept { return hi + lo; }

    constexpr friend double_double operator-(const double_double& x) noexcept { return { -x.hi, -x.lo }; }
    constexpr friend double_double operator+(const double_double& x, const double_double& y) noexcept
    {
        const double sum = x.hi + y.hi;
        return { sum, std::abs(x.hi) > std::abs(y.hi) ? (((x.hi - sum) + y.hi) + y.lo) + x.lo
                                                      : (((y.hi - sum) + x.hi) + x.lo) + y.lo };
    }
    constexpr friend double_double operator-(const double_double& x, const double_double& y) noexcept
    {
        const double diff = x.hi - y.hi;
        return { diff, std::abs(x.hi) > std::abs(y.hi) ? (((x.hi - diff) - y.hi) - y.lo) + x.lo
                                                       : (((-y.hi - diff) + x.hi) + x.lo) - y.lo };
    }
    constexpr friend double_double operator*(const double_double& x, const double_double& y) noexcept
    {
        const double_double c = mul(x.hi, y.hi);
        const double cc       = (x.hi * y.lo + x.lo * y.hi) + c.lo;
        return { c.hi, cc };
    }
    constexpr friend double_double operator/(const double_double& x, const double_double& y) noexcept
    {
        const double c        = x.hi / y.hi;
        const double_double u = mul(c, y.hi);
        const double cc       = ((((x.hi - u.hi) - u.lo) + x.lo) - c * y.lo) / y.hi;
        return { c, cc };
    }
    constexpr bool isinf() const noexcept { return std::isinf(hi); }
    constexpr bool isnan() const noexcept { return std::isnan(hi) || std::isnan(lo); }

    constexpr double ulp(float value) const noexcept
    {
        if (std::isnan(value) && isnan())
            return 0.0;
        if (std::isinf(value) && isinf() && (std::copysign(1.0f, value) == std::copysign(1.0, hi)))
            return 0.0;
        if (std::nexttoward(value, 0.0) == 0.0)
            return 1.0;
        return (double_double(value) - *this) / double_double(std::nexttoward(value, 0.0));
    }
    constexpr double ulp(double value) const noexcept
    {
        if (std::isnan(value) && isnan())
            return 0.0;
        if (std::isinf(value) && isinf() && (std::copysign(1.0, value) == std::copysign(1.0, hi)))
            return 0.0;
        if (std::nexttoward(value, 0.0) == 0.0)
            return 1.0;
        return (double_double(value) - *this) / double_double(std::nexttoward(value, 0.0));
    }

private:
    constexpr static double_double splitprec(double x) noexcept
    {
        const double p = x * 1.34217729e8;
        const double h = (x - p) + p;
        return { h, x - h };
    }
    constexpr static double_double mul(double x, double y) noexcept
    {
        const double_double xx = splitprec(x);
        const double_double yy = splitprec(y);
        const double z         = x * y;
        return { z, ((xx.hi * yy.hi - z) + xx.hi * yy.lo + xx.lo * yy.hi) + xx.lo * yy.lo };
    }
};
