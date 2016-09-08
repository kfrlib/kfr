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
#include "abs.hpp"
#include "atan.hpp"
#include "constants.hpp"
#include "function.hpp"
#include "hyperbolic.hpp"
#include "log_exp.hpp"
#include "min_max.hpp"
#include "operators.hpp"
#include "select.hpp"
#include "sin_cos.hpp"
#include "sqrt.hpp"

#ifdef KFR_STD_COMPLEX
#include <complex>
#endif

namespace kfr
{
#ifdef KFR_STD_COMPLEX

template <typename T>
using complex = std::complex<T>;

#else
#ifndef KFR_CUSTOM_COMPLEX

/**
 * @brief Represents the complex numbers. If KFR_STD_COMPLEX is defined, then kfr::complex is an alias for
 * std::complex.
 */
template <typename T>
struct complex
{
    constexpr static bool is_pod = true;
    constexpr complex() noexcept = default;
    constexpr complex(T re) noexcept : re(re), im(0) {}
    constexpr complex(T re, T im) noexcept : re(re), im(im) {}
    constexpr complex(const complex&) noexcept = default;
    constexpr complex(complex&&) noexcept      = default;
    template <typename U>
    constexpr complex(const complex<U>& other) noexcept : re(static_cast<T>(other.re)),
                                                          im(static_cast<T>(other.im))
    {
    }
    template <typename U>
    constexpr complex(complex<U>&& other) noexcept : re(std::move(other.re)), im(std::move(other.im))
    {
    }
    constexpr complex& operator=(const complex&) noexcept = default;
    constexpr complex& operator=(complex&&) noexcept = default;
    constexpr const T& real() const noexcept { return re; }
    constexpr const T& imag() const noexcept { return im; }
    constexpr void real(T value) noexcept { re = value; }
    constexpr void imag(T value) noexcept { im = value; }
    T re;
    T im;

    KFR_INTRIN friend complex operator+(const complex& x, const complex& y)
    {
        return (make_vector(x) + make_vector(y))[0];
    }
    KFR_INTRIN friend complex operator-(const complex& x, const complex& y)
    {
        return (make_vector(x) - make_vector(y))[0];
    }
    KFR_INTRIN friend complex operator*(const complex& x, const complex& y)
    {
        return (make_vector(x) * make_vector(y))[0];
    }
    KFR_INTRIN friend complex operator/(const complex& x, const complex& y)
    {
        return (make_vector(x) / make_vector(y))[0];
    }
    KFR_INTRIN friend complex operator-(const complex& x) { return (-make_vector(x))[0]; }
};
#endif
#endif
}
namespace cometa
{
template <typename T>
struct compound_type_traits<kfr::complex<T>>
{
    constexpr static size_t width      = 2;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;
    template <typename U>
    using rebind = kfr::complex<U>;
    template <typename U>
    using deep_rebind = kfr::complex<cometa::deep_rebind<subtype, U>>;

    static constexpr subtype at(const kfr::complex<T>& value, size_t index)
    {
        return index == 0 ? value.real() : value.imag();
    }
};
}
namespace kfr
{

using c32   = complex<f32>;
using c64   = complex<f64>;
using cbase = complex<fbase>;

template <typename T, size_t N>
struct vec_op<complex<T>, N> : private vec_op<T, N * 2>
{
    using scalar_type = T;
    using vec_op<scalar_type, N * 2>::add;
    using vec_op<scalar_type, N * 2>::sub;
    using vec_op<scalar_type, N * 2>::eq;
    using vec_op<scalar_type, N * 2>::ne;
    using vec_op<scalar_type, N * 2>::band;
    using vec_op<scalar_type, N * 2>::bor;
    using vec_op<scalar_type, N * 2>::bxor;
    using vec_op<scalar_type, N * 2>::bnot;
    using vec_op<scalar_type, N * 2>::neg;

    constexpr static size_t w = N * 2;

    CMT_INLINE constexpr static simd<scalar_type, w> mul(const simd<scalar_type, w>& x,
                                                         const simd<scalar_type, w>& y) noexcept
    {
        const vec<scalar_type, w> xx = x;
        const vec<scalar_type, w> yy = y;
        return *subadd(xx * dupeven(yy), swap<2>(xx) * dupodd(yy));
    }
    CMT_INLINE constexpr static simd<scalar_type, w> div(const simd<scalar_type, w>& x,
                                                         const simd<scalar_type, w>& y) noexcept
    {
        const vec<scalar_type, w> xx = x;
        const vec<scalar_type, w> yy = y;
        const vec<scalar_type, w> m  = (sqr(dupeven(yy)) + sqr(dupodd(yy)));
        return *swap<2>(subadd(swap<2>(xx) * dupeven(yy), xx * dupodd(yy)) / m);
    }
};

template <typename T, size_t N>
CMT_INLINE vec<complex<T>, N> cdupreal(const vec<complex<T>, N>& x)
{
    return compcast<complex<T>>(dupeven(compcast<T>(x)));
}
KFR_FN(cdupreal)

template <typename T, size_t N>
CMT_INLINE vec<complex<T>, N> cdupimag(const vec<complex<T>, N>& x)
{
    return compcast<complex<T>>(dupodd(compcast<T>(x)));
}
KFR_FN(cdupimag)

template <typename T, size_t N>
CMT_INLINE vec<complex<T>, N> cswapreim(const vec<complex<T>, N>& x)
{
    return compcast<complex<T>>(swap<2>(compcast<T>(x)));
}
KFR_FN(cswapreim)

template <typename T, size_t N>
CMT_INLINE vec<complex<T>, N> cnegreal(const vec<complex<T>, N>& x)
{
    return x ^ complex<T>(-T(), T());
}
KFR_FN(cnegreal)
template <typename T, size_t N>
CMT_INLINE vec<complex<T>, N> cnegimag(const vec<complex<T>, N>& x)
{
    return x ^ complex<T>(T(), -T());
}
KFR_FN(cnegimag)

template <typename T, size_t N>
CMT_INLINE vec<complex<T>, N> cconj(const vec<complex<T>, N>& x)
{
    return cnegimag(x);
}
KFR_FN(cconj)

namespace internal
{
template <typename T>
struct is_complex_impl : std::false_type
{
};
template <typename T>
struct is_complex_impl<complex<T>> : std::true_type
{
};

// vector<complex> to vector<complex>
template <typename To, typename From, size_t N>
struct conversion<vec<complex<To>, N>, vec<complex<From>, N>>
{
    static_assert(!is_compound<To>::value, "");
    static_assert(!is_compound<From>::value, "");
    static vec<complex<To>, N> cast(const vec<complex<From>, N>& value)
    {
        return builtin_convertvector<complex<To>>(value);
    }
};

// vector to vector<complex>
template <typename To, typename From, size_t N>
struct conversion<vec<complex<To>, N>, vec<From, N>>
{
    static_assert(!is_compound<To>::value, "");
    static_assert(!is_compound<From>::value, "");
    static vec<complex<To>, N> cast(const vec<From, N>& value)
    {
        const vec<To, N> casted = static_cast<vec<To, N>>(value);
        return *interleave(casted, zerovector(casted));
    }
};
}

template <typename T, size_t N>
constexpr CMT_INLINE vec<complex<T>, N / 2> ccomp(const vec<T, N>& x)
{
    return compcast<complex<T>>(x);
}

template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N * 2> cdecom(const vec<complex<T>, N>& x)
{
    return compcast<T>(x);
}

template <typename T>
constexpr CMT_INLINE T real(const complex<T>& value)
{
    return value.real();
}
template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> real(const vec<complex<T>, N>& value)
{
    return even(compcast<T>(value));
}

template <typename T>
using realtype = decltype(kfr::real(std::declval<T>()));
template <typename T>
using realftype = ftype<decltype(kfr::real(std::declval<T>()))>;

KFR_FN(real)
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
CMT_INLINE internal::expression_function<fn::real, E1> real(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T>
constexpr CMT_INLINE T imag(const complex<T>& value)
{
    return value.imag();
}
template <typename T, size_t N>
constexpr CMT_INLINE vec<T, N> imag(const vec<complex<T>, N>& value)
{
    return odd(compcast<T>(value));
}
KFR_FN(imag)
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
CMT_INLINE internal::expression_function<fn::imag, E1> imag(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, typename T2 = T1, size_t N, typename T = common_type<T1, T2>>
constexpr CMT_INLINE vec<complex<T>, N> make_complex(const vec<T1, N>& real, const vec<T2, N>& imag = T2(0))
{
    return compcast<complex<T>>(interleave(cast<T>(real), cast<T>(imag)));
}

template <typename T1, typename T2 = T1, typename T = common_type<T1, T2>>
constexpr CMT_INLINE complex<T> make_complex(T1 real, T2 imag = T2(0))
{
    return complex<T>(cast<T>(real), cast<T>(imag));
}

namespace intrinsics
{

template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> csin(const vec<complex<T>, N>& x)
{
    return ccomp(sincos(cdecom(cdupreal(x))) * coshsinh(cdecom(cdupimag(x))));
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> csinh(const vec<complex<T>, N>& x)
{
    return ccomp(sinhcosh(cdecom(cdupreal(x))) * cossin(cdecom(cdupimag(x))));
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> ccos(const vec<complex<T>, N>& x)
{
    return ccomp(negodd(cossin(cdecom(cdupreal(x))) * coshsinh(cdecom(cdupimag(x)))));
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> ccosh(const vec<complex<T>, N>& x)
{
    return ccomp(coshsinh(cdecom(cdupreal(x))) * cossin(cdecom(cdupimag(x))));
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> cabs(const vec<complex<T>, N>& x)
{
    const vec<T, N* 2> xx = sqr(cdecom(x));
    return sqrt(even(xx) + odd(xx));
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> carg(const vec<complex<T>, N>& x)
{
    const vec<T, N* 2> xx = cdecom(x);
    return atan2(even(xx), odd(xx));
}

template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> clog(const vec<complex<T>, N>& x)
{
    return make_complex(log(cabs(x)), carg(x));
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> clog2(const vec<complex<T>, N>& x)
{
    return clog(x) * c_recip_log_2<T>;
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> clog10(const vec<complex<T>, N>& x)
{
    return clog(x) * c_recip_log_10<T>;
}

template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> cexp(const vec<complex<T>, N>& x)
{
    return ccomp(exp(cdecom(cdupreal(x))) * cossin(cdecom(cdupimag(x))));
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> cexp2(const vec<complex<T>, N>& x)
{
    return cexp(x * c_log_2<T>);
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> cexp10(const vec<complex<T>, N>& x)
{
    return cexp(x * c_log_10<T>);
}

template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> polar(const vec<complex<T>, N>& x)
{
    return make_complex(cabs(x), carg(x));
}
template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> cartesian(const vec<complex<T>, N>& x)
{
    return cdupreal(x) * ccomp(cossin(cdecom(cdupimag(x))));
}

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> cabsdup(const vec<T, N>& x)
{
    x = sqr(x);
    return sqrt(x + swap<2>(x));
}

template <typename T, size_t N>
KFR_SINTRIN vec<complex<T>, N> csqrt(const vec<complex<T>, N>& x)
{
    const vec<T, N> t = (cabsdup(cdecom(x)) + cdecom(cnegimag(cdupreal(x)))) * T(0.5);
    return ccomp(select(dupodd(x) < T(), cdecom(cnegimag(ccomp(t))), t));
}

KFR_I_CONVERTER(csin)
KFR_I_CONVERTER(csinh)
KFR_I_CONVERTER(ccos)
KFR_I_CONVERTER(ccosh)
KFR_I_CONVERTER(clog)
KFR_I_CONVERTER(clog2)
KFR_I_CONVERTER(clog10)
KFR_I_CONVERTER(cexp)
KFR_I_CONVERTER(cexp2)
KFR_I_CONVERTER(cexp10)
KFR_I_CONVERTER(polar)
KFR_I_CONVERTER(cartesian)
KFR_I_CONVERTER(csqrt)

template <typename T1>
KFR_SINTRIN realtype<T1> cabs(const T1& a)
{
    using vecout = vec1<T1>;
    return to_scalar(intrinsics::cabs(vecout(a)));
}
template <typename T1>
KFR_SINTRIN realtype<T1> carg(const T1& a)
{
    using vecout = vec1<T1>;
    return to_scalar(intrinsics::carg(vecout(a)));
}
}

KFR_I_FN(csin)
KFR_I_FN(csinh)
KFR_I_FN(ccos)
KFR_I_FN(ccosh)
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

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 csin(const T1& x)
{
    return intrinsics::csin(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::csin, E1> csin(E1&& x)
{
    return { fn::csin(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 csinh(const T1& x)
{
    return intrinsics::csinh(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::csinh, E1> csinh(E1&& x)
{
    return { fn::csinh(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 ccos(const T1& x)
{
    return intrinsics::ccos(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::ccos, E1> ccos(E1&& x)
{
    return { fn::ccos(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 ccosh(const T1& x)
{
    return intrinsics::ccosh(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::ccosh, E1> ccosh(E1&& x)
{
    return { fn::ccosh(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN realtype<T1> cabs(const T1& x)
{
    return intrinsics::cabs(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::cabs, E1> cabs(E1&& x)
{
    return { fn::cabs(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN realtype<T1> carg(const T1& x)
{
    return intrinsics::carg(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::carg, E1> carg(E1&& x)
{
    return { fn::carg(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 clog(const T1& x)
{
    return intrinsics::clog(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::clog, E1> clog(E1&& x)
{
    return { fn::clog(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 clog2(const T1& x)
{
    return intrinsics::clog2(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::clog2, E1> clog2(E1&& x)
{
    return { fn::clog2(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 clog10(const T1& x)
{
    return intrinsics::clog10(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::clog10, E1> clog10(E1&& x)
{
    return { fn::clog10(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cexp(const T1& x)
{
    return intrinsics::cexp(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::cexp, E1> cexp(E1&& x)
{
    return { fn::cexp(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cexp2(const T1& x)
{
    return intrinsics::cexp2(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::cexp2, E1> cexp2(E1&& x)
{
    return { fn::cexp2(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cexp10(const T1& x)
{
    return intrinsics::cexp10(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::cexp10, E1> cexp10(E1&& x)
{
    return { fn::cexp10(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 polar(const T1& x)
{
    return intrinsics::polar(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::polar, E1> polar(E1&& x)
{
    return { fn::polar(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 cartesian(const T1& x)
{
    return intrinsics::cartesian(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::cartesian, E1> cartesian(E1&& x)
{
    return { fn::cartesian(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 csqrt(const T1& x)
{
    return intrinsics::csqrt(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::csqrt, E1> csqrt(E1&& x)
{
    return { fn::csqrt(), std::forward<E1>(x) };
}
}
