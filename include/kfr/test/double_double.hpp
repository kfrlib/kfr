#pragma once

#include <algorithm>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstring>

struct precise_fp
{
    int sign; // 1 means '+', -1 means '-', can't be 0
    int exponent; // unbiased, INT_MIN means 0/denormal, INT_MAX means inf/nan
    uint64_t mantissa; // with explicit first bit set, 63 significant bits

    bool is_zero() const { return exponent == INT_MIN && mantissa == 0; }
    bool is_denormal() const { return exponent == INT_MIN && mantissa != 0; }
    bool is_inf() const { return exponent == INT_MAX && mantissa == 0; }
    bool is_nan() const { return exponent == INT_MAX && mantissa != 0; }

    double to_double() const { return sign * std::ldexp(static_cast<double>(mantissa), exponent); }
    float to_float() const { return sign * std::ldexp(static_cast<float>(mantissa), exponent); }

    precise_fp(int sign, int exponent, uint64_t mantissa) : sign(sign), exponent(exponent), mantissa(mantissa)
    {
    }

    template <typename T>
    explicit precise_fp(T value)
    {
        sign = static_cast<int>(std::copysign(T(1), value));
        if (value == 0)
        {
            mantissa = 0;
            exponent = INT_MIN;
        }
        else if (std::isinf(value))
        {
            mantissa = 0;
            exponent = INT_MAX;
        }
        else if (std::isnan(value))
        {
            mantissa = 1;
            exponent = INT_MAX;
        }
        else
        {
            mantissa = 0x80000000'00000000ull * std::frexp(value, &exponent);
        }
    }

    friend double precise_ulps(const precise_fp& x, const float& y)
    {
        return precise_ulps(x, precise_fp(y), -126, 24);
    }
    friend double precise_ulps(const precise_fp& x, const double& y)
    {
        return precise_ulps(x, precise_fp(y), -1022, 53);
    }

    friend double precise_ulps(const precise_fp& x, const precise_fp& y, int minexponent, int mantissabits)
    {
        if (x.is_zero() && y.is_zero())
            return 0;
        if (x.is_nan() && y.is_nan())
            return 0;
        if (x.is_inf() && y.is_inf())
            return x.sign == y.sign ? 0 : HUGE_VAL;
        if (x.is_zero() && y.is_zero())
            return 0;

        if (x.sign != y.sign)
            return HUGE_VAL;
        uint64_t xx      = x.mantissa;
        uint64_t yy      = y.mantissa;
        const int minexp = std::min(x.exponent, y.exponent);
        if (x.exponent - minexp <= 1 && y.exponent - minexp <= 1)
        {
            xx >>= y.exponent - minexp;
            yy >>= x.exponent - minexp;
            return static_cast<double>(xx > yy ? xx - yy : yy - xx) / (1 << (63 - mantissabits));
        }
        return HUGE_VAL;
    }
};

struct double_double
{
    double hi, lo;

    static_assert(sizeof(double) == 8, "");

    constexpr double_double(double x) noexcept : hi(x), lo(0.0) {}
    constexpr double_double(float x) noexcept : hi(x), lo(0.0) {}
    constexpr double_double(double hi, double lo) noexcept : hi(hi + lo), lo((hi - (hi + lo)) + lo) {}
    constexpr operator double() const noexcept { return hi + lo; }
    constexpr operator float() const noexcept { return hi + lo; }

    constexpr static double abs(double x) noexcept { return x >= 0 ? x : -x; }

    constexpr friend double_double operator-(const double_double& x) noexcept { return { -x.hi, -x.lo }; }
    constexpr friend double_double operator+(const double_double& x, const double_double& y) noexcept
    {
        const double sum = x.hi + y.hi;
        return { sum, abs(x.hi) > abs(y.hi) ? (((x.hi - sum) + y.hi) + y.lo) + x.lo
                                            : (((y.hi - sum) + x.hi) + x.lo) + y.lo };
    }
    constexpr friend double_double operator-(const double_double& x, const double_double& y) noexcept
    {
        const double diff = x.hi - y.hi;
        return { diff, abs(x.hi) > abs(y.hi) ? (((x.hi - diff) - y.hi) - y.lo) + x.lo
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

    bool isinf() const noexcept { return std::isinf(hi); }
    bool isnan() const noexcept { return std::isnan(hi) || std::isnan(lo); }
    bool iszero() const noexcept { return hi == 0 && lo == 0; }

    double ulp(float value) const noexcept
    {
        if (std::isnan(value) && isnan())
            return 0.0;
        if (std::isinf(value) && isinf() && (std::copysign(1.0f, value) == std::copysign(1.0, hi)))
            return 0.0;
        if (value == 0 && iszero())
            return 0.0;
        if (std::nexttoward(value, 0.0) == 0.0 && iszero())
            return 1.0;
        return (double_double(value) - *this) / double_double(std::nexttoward(value, 0.0));
    }
    double ulp(double value) const noexcept
    {
        if (std::isnan(value) && isnan())
            return 0.0;
        if (std::isinf(value) && isinf() && (std::copysign(1.0, value) == std::copysign(1.0, hi)))
            return 0.0;
        if (value == 0 && iszero())
            return 0.0;
        if (std::nexttoward(value, 0.0) == 0.0 && iszero())
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
