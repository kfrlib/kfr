/** @addtogroup math
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "types.hpp"
#include <limits>

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4309))
CMT_PRAGMA_MSVC(warning(disable : 4146))

namespace kfr
{

#if CMT_COMPILER_GNU
constexpr double infinity = __builtin_inf();
constexpr double qnan     = __builtin_nan("");
#else
constexpr double infinity = HUGE_VAL;
constexpr double qnan     = NAN;
#endif

template <typename T>
struct constants
{
public:
    using Tsub = subtype<T>;

    constexpr static Tsub pi_s(int m, int d = 1) { return pi * m / d; }
    constexpr static Tsub recip_pi_s(int m, int d = 1) { return recip_pi * m / d; }

    constexpr static Tsub pi           = static_cast<Tsub>(3.1415926535897932384626433832795);
    constexpr static Tsub sqr_pi       = static_cast<Tsub>(9.8696044010893586188344909998762);
    constexpr static Tsub recip_pi     = static_cast<Tsub>(0.31830988618379067153776752674503);
    constexpr static Tsub degtorad     = static_cast<Tsub>(pi / 180);
    constexpr static Tsub radtodeg     = static_cast<Tsub>(pi * 180);
    constexpr static Tsub e            = static_cast<Tsub>(2.718281828459045235360287471352662);
    constexpr static Tsub recip_log_2  = static_cast<Tsub>(1.442695040888963407359924681001892137426645954);
    constexpr static Tsub recip_log_10 = static_cast<Tsub>(0.43429448190325182765112891891661);
    constexpr static Tsub log_2        = static_cast<Tsub>(0.69314718055994530941723212145818);
    constexpr static Tsub log_10       = static_cast<Tsub>(2.3025850929940456840179914546844);
    constexpr static Tsub sqrt_2       = static_cast<Tsub>(1.4142135623730950488016887242097);

    constexpr static Tsub fold_constant_div = choose_const<Tsub>(
        CMT_FP(0x1.921fb6p-1f, 7.8539818525e-01f), CMT_FP(0x1.921fb54442d18p-1, 7.853981633974482790e-01));

    constexpr static Tsub fold_constant_hi = choose_const<Tsub>(
        CMT_FP(0x1.922000p-1f, 7.8540039062e-01f), CMT_FP(0x1.921fb40000000p-1, 7.853981256484985352e-01));
    constexpr static Tsub fold_constant_rem1 =
        choose_const<Tsub>(CMT_FP(-0x1.2ae000p-19f, -2.2267922759e-06f),
                           CMT_FP(0x1.4442d00000000p-25, 3.774894707930798177e-08));
    constexpr static Tsub fold_constant_rem2 =
        choose_const<Tsub>(CMT_FP(-0x1.de973ep-32f, -4.3527578764e-10f),
                           CMT_FP(0x1.8469898cc5170p-49, 2.695151429079059484e-15));

    constexpr static Tsub epsilon     = std::numeric_limits<Tsub>::epsilon();
    constexpr static Tsub infinity    = std::numeric_limits<Tsub>::infinity();
    constexpr static Tsub neginfinity = -std::numeric_limits<Tsub>::infinity();
    constexpr static Tsub qnan        = std::numeric_limits<Tsub>::quiet_NaN();

#if CMT_COMPILER_GNU

    CMT_PRAGMA_GNU(GCC diagnostic push)
    CMT_PRAGMA_GNU(GCC diagnostic ignored "-Woverflow")

    constexpr static Tsub allones()
    {
        if (is_same<Tsub, f32>::value)
        {
            return -__builtin_nanf("0xFFFFFFFF");
        }
        else if (is_same<Tsub, f64>::value)
        {
            return -__builtin_nan("0xFFFFFFFFFFFFFFFF");
        }
        else
        {
            return static_cast<Tsub>(-1ll);
        }
    }

    constexpr static Tsub allzeros() { return Tsub(0); }

    constexpr static Tsub highbitmask()
    {
        if (is_same<Tsub, f32>::value)
        {
            return -0.0f;
        }
        else if (is_same<Tsub, f64>::value)
        {
            return -0.0;
        }
        else
        {
            return static_cast<Tsub>(1ull << (sizeof(Tsub) * 8 - 1));
        }
    }

    constexpr static Tsub invhighbitmask()
    {
        if (is_same<Tsub, f32>::value)
        {
            return __builtin_nanf("0xFFFFFFFF");
        }
        else if (is_same<Tsub, f64>::value)
        {
            return __builtin_nan("0xFFFFFFFFFFFFFFFF");
        }
        else
        {
            return static_cast<Tsub>((1ull << (sizeof(Tsub) * 8 - 1)) - 1);
        }
    }
    CMT_PRAGMA_GNU(GCC diagnostic pop)
#else

    static Tsub allones()
    {
        if (is_same<Tsub, f32>::value)
        {
            return static_cast<Tsub>(bitcast<f32>(0xFFFFFFFFu));
        }
        else if (is_same<Tsub, f64>::value)
        {
            return static_cast<Tsub>(bitcast<f64>(0xFFFFFFFFFFFFFFFFull));
        }
        else
        {
            return static_cast<Tsub>(-1ll);
        }
    }

    constexpr static Tsub allzeros() { return Tsub(0); }

    static Tsub highbitmask()
    {
        if (is_same<Tsub, f32>::value)
        {
            return static_cast<Tsub>(-0.0f);
        }
        else if (is_same<Tsub, f64>::value)
        {
            return static_cast<Tsub>(-0.0);
        }
        else
        {
            return static_cast<Tsub>(1ull << (sizeof(Tsub) * 8 - 1));
        }
    }

    static Tsub invhighbitmask()
    {
        if (is_same<Tsub, f32>::value)
        {
            return static_cast<Tsub>(bitcast<f32>(0x7FFFFFFFu));
        }
        else if (is_same<Tsub, f64>::value)
        {
            return static_cast<Tsub>(bitcast<f64>(0x7FFFFFFFFFFFFFFFull));
        }
        else
        {
            return static_cast<Tsub>((1ull << (sizeof(Tsub) * 8 - 1)) - 1);
        }
    }
#endif
};

template <typename T>
constexpr subtype<T> constants<T>::pi;
template <typename T>
constexpr subtype<T> constants<T>::sqr_pi;
template <typename T>
constexpr subtype<T> constants<T>::recip_pi;
template <typename T>
constexpr subtype<T> constants<T>::degtorad;
template <typename T>
constexpr subtype<T> constants<T>::radtodeg;
template <typename T>
constexpr subtype<T> constants<T>::e;
template <typename T>
constexpr subtype<T> constants<T>::recip_log_2;
template <typename T>
constexpr subtype<T> constants<T>::recip_log_10;
template <typename T>
constexpr subtype<T> constants<T>::log_2;
template <typename T>
constexpr subtype<T> constants<T>::log_10;
template <typename T>
constexpr subtype<T> constants<T>::sqrt_2;
template <typename T>
constexpr subtype<T> constants<T>::fold_constant_div;
template <typename T>
constexpr subtype<T> constants<T>::fold_constant_hi;
template <typename T>
constexpr subtype<T> constants<T>::fold_constant_rem1;
template <typename T>
constexpr subtype<T> constants<T>::fold_constant_rem2;
template <typename T>
constexpr subtype<T> constants<T>::epsilon;
template <typename T>
constexpr subtype<T> constants<T>::infinity;
template <typename T>
constexpr subtype<T> constants<T>::neginfinity;
template <typename T>
constexpr subtype<T> constants<T>::qnan;

// π (pi)
// c_pi<f64, 4>      = 4pi
// c_pi<f64, 3, 4>   = 3/4pi
template <typename T, int m = 1, int d = 1>
constexpr subtype<T> c_pi = subtype<T>(3.1415926535897932384626433832795 * m / d);

// π² (pi²)
// c_sqr_pi<f64, 4>      = 4pi²
// c_sqr_pi<f64, 3, 4>   = 3/4pi²
template <typename T, int m = 1, int d = 1>
constexpr subtype<T> c_sqr_pi = subtype<T>(9.8696044010893586188344909998762 * m / d);

// 1/π (1/pi)
// c_recip_pi<f64>       1/pi
// c_recip_pi<f64, 4>    4/pi
template <typename T, int m = 1, int d = 1>
constexpr subtype<T> c_recip_pi = subtype<T>(0.31830988618379067153776752674503 * m / d);

// degree to radian conversion factor
template <typename T>
constexpr subtype<T> c_degtorad = c_pi<T, 1, 180>;

// radian to degree conversion factor
template <typename T>
constexpr subtype<T> c_radtodeg = c_recip_pi<T, 180>;

// e, Euler's number
template <typename T, int m = 1, int d = 1>
constexpr subtype<T> c_e = subtype<T>(2.718281828459045235360287471352662 * m / d);

template <typename T>
constexpr unsigned c_mantissa_bits = sizeof(subtype<T>) == 32 ? 23 : 52;

template <typename T>
constexpr subtype<T> c_mantissa_mask = (subtype<T>(1) << c_mantissa_bits<T>)-1;

template <typename T>
constexpr subtype<T> c_epsilon = (std::numeric_limits<subtype<T>>::epsilon());

template <typename T>
constexpr subtype<T> c_infinity = std::numeric_limits<subtype<T>>::infinity();

template <typename T>
constexpr subtype<T> c_neginfinity = -std::numeric_limits<subtype<T>>::infinity();

template <typename T>
constexpr subtype<T> c_qnan = std::numeric_limits<subtype<T>>::quiet_NaN();

template <typename T>
constexpr subtype<T> c_recip_log_2 = subtype<T>(1.442695040888963407359924681001892137426645954);

template <typename T>
constexpr subtype<T> c_recip_log_10 = subtype<T>(0.43429448190325182765112891891661);

template <typename T>
constexpr subtype<T> c_log_2 = subtype<T>(0.69314718055994530941723212145818);

template <typename T>
constexpr subtype<T> c_log_10 = subtype<T>(2.3025850929940456840179914546844);

template <typename T, int m = 1, int d = 1>
constexpr subtype<T> c_sqrt_2 = subtype<T>(1.4142135623730950488016887242097 * m / d);
}

CMT_PRAGMA_MSVC(warning(pop))
