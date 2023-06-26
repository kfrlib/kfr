/** @addtogroup complex
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

#include "../simd/abs.hpp"
#include "../simd/complex.hpp"
#include "../simd/min_max.hpp"
#include "../simd/select.hpp"
#include "atan.hpp"
#include "hyperbolic.hpp"
#include "log_exp.hpp"
#include "sin_cos.hpp"
#include "sqrt.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> csin(const vec<complex<T>, N>& x)
{
    return ccomp(sincos(cdecom(cdupreal(x))) * coshsinh(cdecom(cdupimag(x))));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> csinh(const vec<complex<T>, N>& x)
{
    return ccomp(sinhcosh(cdecom(cdupreal(x))) * cossin(cdecom(cdupimag(x))));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> ccos(const vec<complex<T>, N>& x)
{
    return ccomp(negodd(cossin(cdecom(cdupreal(x))) * coshsinh(cdecom(cdupimag(x)))));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> ccosh(const vec<complex<T>, N>& x)
{
    return ccomp(coshsinh(cdecom(cdupreal(x))) * cossin(cdecom(cdupimag(x))));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> cabssqr(const vec<complex<T>, N>& x)
{
    const vec<T, N* 2> xx = sqr(cdecom(x));
    return even(xx) + odd(xx);
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> cabs(const vec<complex<T>, N>& x)
{
    const vec<T, N* 2> xx = sqr(cdecom(x));
    return sqrt(even(xx) + odd(xx));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> carg(const vec<complex<T>, N>& x)
{
    const vec<T, N* 2> xx = cdecom(x);
    return atan2(odd(xx), even(xx));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> clog(const vec<complex<T>, N>& x)
{
    return make_complex(log(cabs(x)), carg(x));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> clog2(const vec<complex<T>, N>& x)
{
    return clog(x) * c_recip_log_2<T>;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> clog10(const vec<complex<T>, N>& x)
{
    return clog(x) * c_recip_log_10<T>;
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cexp(const vec<complex<T>, N>& x)
{
    return ccomp(exp(cdecom(cdupreal(x))) * cossin(cdecom(cdupimag(x))));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cexp2(const vec<complex<T>, N>& x)
{
    return cexp(x * c_log_2<T>);
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cexp10(const vec<complex<T>, N>& x)
{
    return cexp(x * c_log_10<T>);
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> polar(const vec<complex<T>, N>& x)
{
    return make_complex(cabs(x), carg(x));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cartesian(const vec<complex<T>, N>& x)
{
    return cdupreal(x) * ccomp(cossin(cdecom(cdupimag(x))));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> cabsdup(const vec<T, N>& x)
{
    vec<T, N> xx = sqr(x);
    return sqrt(xx + swap<2>(xx));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> csqrt(const vec<complex<T>, N>& x)
{
    const vec<T, N> s        = sqrt((abs(real(x)) + cabs(x)) * 0.5);
    const vec<T, N> d        = abs(imag(x)) * 0.5 / s;
    const mask<T, N> posreal = real(x) >= T(0);
    const vec<T, N> imagsign = imag(x) & special_constants<T>::highbitmask();
    return make_complex(select(posreal, s, d), select(posreal, d ^ imagsign, s ^ imagsign));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> csqr(const vec<complex<T>, N>& x)
{
    return x * x;
}

KFR_HANDLE_SCALAR(csin)
KFR_HANDLE_SCALAR(csinh)
KFR_HANDLE_SCALAR(ccos)
KFR_HANDLE_SCALAR(ccosh)
KFR_HANDLE_SCALAR(clog)
KFR_HANDLE_SCALAR(clog2)
KFR_HANDLE_SCALAR(clog10)
KFR_HANDLE_SCALAR(cexp)
KFR_HANDLE_SCALAR(cexp2)
KFR_HANDLE_SCALAR(cexp10)
KFR_HANDLE_SCALAR(polar)
KFR_HANDLE_SCALAR(cartesian)
KFR_HANDLE_SCALAR(csqrt)
KFR_HANDLE_SCALAR(csqr)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> cabssqr(const vec<T, N>& a)
{
    return to_scalar(intrinsics::cabssqr(static_cast<vec<complex<T>, N>>(a)));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> cabs(const vec<T, N>& a)
{
    return to_scalar(intrinsics::cabs(static_cast<vec<complex<T>, N>>(a)));
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> carg(const vec<T, N>& a)
{
    return to_scalar(intrinsics::carg(static_cast<vec<complex<T>, N>>(a)));
}
template <typename T1>
KFR_INTRINSIC realtype<T1> cabssqr(const T1& a)
{
    using vecout = vec1<T1>;
    return to_scalar(intrinsics::cabssqr(vecout(a)));
}
template <typename T1>
KFR_INTRINSIC realtype<T1> cabs(const T1& a)
{
    using vecout = vec1<T1>;
    return to_scalar(intrinsics::cabs(vecout(a)));
}
template <typename T1>
KFR_INTRINSIC realtype<T1> carg(const T1& a)
{
    using vecout = vec1<T1>;
    return to_scalar(intrinsics::carg(vecout(a)));
}
} // namespace intrinsics

KFR_I_FN(csin)
KFR_I_FN(csinh)
KFR_I_FN(ccos)
KFR_I_FN(ccosh)
KFR_I_FN(cabssqr)
KFR_I_FN(cabs)
KFR_I_FN(carg)
KFR_I_FN(clog)
KFR_I_FN(clog2)
KFR_I_FN(clog10)
KFR_I_FN(cexp)
KFR_I_FN(cexp2)
KFR_I_FN(cexp10)
KFR_I_FN(polar)
KFR_I_FN(cartesian)
KFR_I_FN(csqrt)
KFR_I_FN(csqr)

/// @brief Returns the sine of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 csin(const T1& x)
{
    return intrinsics::csin(x);
}

/// @brief Returns the hyperbolic sine of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 csinh(const T1& x)
{
    return intrinsics::csinh(x);
}

/// @brief Returns the cosine of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 ccos(const T1& x)
{
    return intrinsics::ccos(x);
}

/// @brief Returns the hyperbolic cosine of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 ccosh(const T1& x)
{
    return intrinsics::ccosh(x);
}

/// @brief Returns the squared absolute value (magnitude squared) of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION realtype<T1> cabssqr(const T1& x)
{
    return intrinsics::cabssqr(x);
}

/// @brief Returns the absolute value (magnitude) of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION realtype<T1> cabs(const T1& x)
{
    return intrinsics::cabs(x);
}

/// @brief Returns the phase angle (argument) of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION realtype<T1> carg(const T1& x)
{
    return intrinsics::carg(x);
}

/// @brief Returns the natural logarithm of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 clog(const T1& x)
{
    return intrinsics::clog(x);
}

/// @brief Returns the binary (base-2) logarithm of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 clog2(const T1& x)
{
    return intrinsics::clog2(x);
}

/// @brief Returns the common (base-10) logarithm of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 clog10(const T1& x)
{
    return intrinsics::clog10(x);
}

/// @brief Returns \f$e\f$ raised to the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 cexp(const T1& x)
{
    return intrinsics::cexp(x);
}

/// @brief Returns 2 raised to the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 cexp2(const T1& x)
{
    return intrinsics::cexp2(x);
}

/// @brief Returns 10 raised to the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 cexp10(const T1& x)
{
    return intrinsics::cexp10(x);
}

/// @brief Converts complex number to polar
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 polar(const T1& x)
{
    return intrinsics::polar(x);
}

/// @brief Converts complex number to cartesian
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 cartesian(const T1& x)
{
    return intrinsics::cartesian(x);
}

/// @brief Returns square root of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 csqrt(const T1& x)
{
    return intrinsics::csqrt(x);
}

/// @brief Returns square of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION T1 csqr(const T1& x)
{
    return intrinsics::csqr(x);
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
