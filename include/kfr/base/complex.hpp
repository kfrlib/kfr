/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
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

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

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
};

#endif
#endif

using c32   = complex<f32>;
using c64   = complex<f64>;
using cbase = complex<fbase>;

template <typename T>
struct vec_op<complex<T>> : private vec_op<T>
{
    using scalar_type = T;
    using vec_op<scalar_type>::add;
    using vec_op<scalar_type>::sub;
    using vec_op<scalar_type>::eq;
    using vec_op<scalar_type>::ne;
    using vec_op<scalar_type>::band;
    using vec_op<scalar_type>::bor;
    using vec_op<scalar_type>::bxor;
    using vec_op<scalar_type>::bnot;
    using vec_op<scalar_type>::neg;

    template <simdindex N>
    constexpr static simd<scalar_type, N> mul(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        const vec<scalar_type, N> xx = x;
        const vec<scalar_type, N> yy = y;
        return *subadd(xx * dupeven(yy), swap<2>(xx) * dupodd(yy));
    }

    template <simdindex N>
    constexpr static simd<scalar_type, N> div(simd<scalar_type, N> x, simd<scalar_type, N> y) noexcept
    {
        const vec<scalar_type, N> xx = x;
        const vec<scalar_type, N> yy = y;
        const vec<scalar_type, N> m  = (sqr(dupeven(yy)) + sqr(dupodd(yy)));
        return *swap<2>(subadd(swap<2>(xx) * dupeven(yy), xx * dupodd(yy)) / m);
    }
};

template <typename T, size_t N>
KFR_INLINE vec<complex<T>, N> cdupreal(const vec<complex<T>, N>& x)
{
    return subcast<complex<T>>(dupeven(subcast<T>(x)));
}
KFR_FN(cdupreal)

template <typename T, size_t N>
KFR_INLINE vec<complex<T>, N> cdupimag(const vec<complex<T>, N>& x)
{
    return subcast<complex<T>>(dupodd(subcast<T>(x)));
}
KFR_FN(cdupimag)

template <typename T, size_t N>
KFR_INLINE vec<complex<T>, N> cswapreim(const vec<complex<T>, N>& x)
{
    return subcast<complex<T>>(swap<2>(subcast<T>(x)));
}
KFR_FN(cswapreim)

template <typename T, size_t N>
KFR_INLINE vec<complex<T>, N> cnegreal(const vec<complex<T>, N>& x)
{
    return x ^ complex<T>(-T(), T());
}
KFR_FN(cnegreal)
template <typename T, size_t N>
KFR_INLINE vec<complex<T>, N> cnegimag(const vec<complex<T>, N>& x)
{
    return x ^ complex<T>(T(), -T());
}
KFR_FN(cnegimag)

template <typename T, size_t N>
KFR_INLINE vec<complex<T>, N> cconj(const vec<complex<T>, N>& x)
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
}

// real to complex
template <typename To, typename From, size_t N, KFR_ENABLE_IF(internal::is_complex_impl<To>::value)>
constexpr KFR_INLINE vec<To, N> cast(vec<From, N> value) noexcept
{
    const vec<subtype<To>, N> casted = cast<subtype<To>>(value);
    return subcast<To>(interleave(casted, zerovector(casted)));
}

// complex to complex
template <typename To, typename From, size_t N, KFR_ENABLE_IF(internal::is_complex_impl<To>::value)>
constexpr KFR_INLINE vec<To, N> cast(vec<complex<From>, N> value) noexcept
{
    return subcast<To>(cast<subtype<To>>(subcast<From>(value)));
}

// complex to real
template <typename To, typename From, size_t N, KFR_ENABLE_IF(!internal::is_complex_impl<To>::value)>
constexpr KFR_INLINE vec<To, N> cast(vec<complex<From>, N> value) noexcept
{
    static_assert(sizeof(To) == 0, "Can't cast complex to real");
    return {};
}

template <typename T, size_t N>
constexpr KFR_INLINE vec<complex<T>, N / 2> ccomp(const vec<T, N>& x)
{
    return subcast<complex<T>>(x);
}

template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N * 2> cdecom(const vec<complex<T>, N>& x)
{
    return subcast<T>(x);
}

template <typename T>
constexpr KFR_INLINE T real(const complex<T>& value)
{
    return value.real();
}
template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> real(const vec<complex<T>, N>& value)
{
    return even(subcast<T>(value));
}

template <typename T>
using realtype = decltype(kfr::real(std::declval<T>()));
template <typename T>
using realftype = ftype<decltype(kfr::real(std::declval<T>()))>;

KFR_FN(real)
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_real, E1> real(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T>
constexpr KFR_INLINE T imag(const complex<T>& value)
{
    return value.imag();
}
template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> imag(const vec<complex<T>, N>& value)
{
    return odd(subcast<T>(value));
}
KFR_FN(imag)
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_imag, E1> imag(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

template <typename T1, typename T2 = T1, size_t N, typename T = common_type<T1, T2>>
constexpr KFR_INLINE vec<complex<T>, N> make_complex(const vec<T1, N>& real, const vec<T2, N>& imag = T2(0))
{
    return subcast<complex<T>>(interleave(cast<T>(real), cast<T>(imag)));
}

template <typename T1, typename T2 = T1, typename T = common_type<T1, T2>>
constexpr KFR_INLINE complex<T> make_complex(T1 real, T2 imag = T2(0))
{
    return complex<T>(cast<T>(real), cast<T>(imag));
}

namespace internal
{

template <cpu_t c = cpu_t::native>
struct in_complex : in_select<c>, in_sin_cos<c>, in_hyperbolic<c>, in_sqrt<c>, in_atan<c>, in_log_exp<c>
{
    constexpr static cpu_t cur = c;
    using in_sqrt<c>::sqrt;
    using in_sin_cos<c>::sincos;
    using in_sin_cos<c>::cossin;
    using in_hyperbolic<c>::sinhcosh;
    using in_hyperbolic<c>::coshsinh;
    using in_atan<c>::atan2;
    using in_log_exp<c>::log;
    using in_log_exp<c>::log2;
    using in_log_exp<c>::log10;
    using in_log_exp<c>::exp;
    using in_log_exp<c>::exp2;
    using in_log_exp<c>::exp10;

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
    KFR_SINTRIN vec<T, N> cabsdup(vec<T, N> x)
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

    KFR_HANDLE_SCALAR(csin)
    KFR_HANDLE_SCALAR(csinh)
    KFR_HANDLE_SCALAR(ccos)
    KFR_HANDLE_SCALAR(ccosh)
    KFR_HANDLE_SCALAR(cabs)
    KFR_HANDLE_SCALAR(carg)
    KFR_HANDLE_SCALAR(clog)
    KFR_HANDLE_SCALAR(clog2)
    KFR_HANDLE_SCALAR(clog10)
    KFR_HANDLE_SCALAR(cexp)
    KFR_HANDLE_SCALAR(cexp2)
    KFR_HANDLE_SCALAR(cexp10)
    KFR_HANDLE_SCALAR(polar)
    KFR_HANDLE_SCALAR(cartesian)
    KFR_HANDLE_SCALAR(csqrt)

    KFR_SPEC_FN(in_complex, csin)
    KFR_SPEC_FN(in_complex, csinh)
    KFR_SPEC_FN(in_complex, ccos)
    KFR_SPEC_FN(in_complex, ccosh)
    KFR_SPEC_FN(in_complex, cabs)
    KFR_SPEC_FN(in_complex, carg)
    KFR_SPEC_FN(in_complex, clog)
    KFR_SPEC_FN(in_complex, clog2)
    KFR_SPEC_FN(in_complex, clog10)
    KFR_SPEC_FN(in_complex, cexp)
    KFR_SPEC_FN(in_complex, cexp2)
    KFR_SPEC_FN(in_complex, cexp10)
    KFR_SPEC_FN(in_complex, polar)
    KFR_SPEC_FN(in_complex, cartesian)
    KFR_SPEC_FN(in_complex, csqrt)
};
}

namespace native
{
using fn_csin = internal::in_complex<>::fn_csin;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> csin(const T1& x)
{
    return internal::in_complex<>::csin(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_csin, E1> csin(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

using fn_csinh = internal::in_complex<>::fn_csinh;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> csinh(const T1& x)
{
    return internal::in_complex<>::csinh(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_csinh, E1> csinh(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}

using fn_ccos = internal::in_complex<>::fn_ccos;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> ccos(const T1& x)
{
    return internal::in_complex<>::ccos(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_ccos, E1> ccos(E1&& x)
{
    return { fn_ccos(), std::forward<E1>(x) };
}

using fn_ccosh = internal::in_complex<>::fn_ccosh;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> ccosh(const T1& x)
{
    return internal::in_complex<>::ccosh(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_ccosh, E1> ccosh(E1&& x)
{
    return { fn_ccosh(), std::forward<E1>(x) };
}

using fn_cabs = internal::in_complex<>::fn_cabs;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN realftype<T1> cabs(const T1& x)
{
    return internal::in_complex<>::cabs(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_cabs, E1> cabs(E1&& x)
{
    return { fn_cabs(), std::forward<E1>(x) };
}

using fn_carg = internal::in_complex<>::fn_carg;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN realftype<T1> carg(const T1& x)
{
    return internal::in_complex<>::carg(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_carg, E1> carg(E1&& x)
{
    return { fn_carg(), std::forward<E1>(x) };
}

using fn_clog = internal::in_complex<>::fn_clog;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> clog(const T1& x)
{
    return internal::in_complex<>::clog(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_clog, E1> clog(E1&& x)
{
    return { fn_clog(), std::forward<E1>(x) };
}

using fn_clog2 = internal::in_complex<>::fn_clog2;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> clog2(const T1& x)
{
    return internal::in_complex<>::clog2(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_clog2, E1> clog2(E1&& x)
{
    return { fn_clog2(), std::forward<E1>(x) };
}

using fn_clog10 = internal::in_complex<>::fn_clog10;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> clog10(const T1& x)
{
    return internal::in_complex<>::clog10(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_clog10, E1> clog10(E1&& x)
{
    return { fn_clog10(), std::forward<E1>(x) };
}

using fn_cexp = internal::in_complex<>::fn_cexp;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> cexp(const T1& x)
{
    return internal::in_complex<>::cexp(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_cexp, E1> cexp(E1&& x)
{
    return { fn_cexp(), std::forward<E1>(x) };
}

using fn_cexp2 = internal::in_complex<>::fn_cexp2;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> cexp2(const T1& x)
{
    return internal::in_complex<>::cexp2(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_cexp2, E1> cexp2(E1&& x)
{
    return { fn_cexp2(), std::forward<E1>(x) };
}

using fn_cexp10 = internal::in_complex<>::fn_cexp10;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> cexp10(const T1& x)
{
    return internal::in_complex<>::cexp10(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_cexp10, E1> cexp10(E1&& x)
{
    return { fn_cexp10(), std::forward<E1>(x) };
}

using fn_polar = internal::in_complex<>::fn_polar;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> polar(const T1& x)
{
    return internal::in_complex<>::polar(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_polar, E1> polar(E1&& x)
{
    return { fn_polar(), std::forward<E1>(x) };
}

using fn_cartesian = internal::in_complex<>::fn_cartesian;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> cartesian(const T1& x)
{
    return internal::in_complex<>::cartesian(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_cartesian, E1> cartesian(E1&& x)
{
    return { fn_cartesian(), std::forward<E1>(x) };
}

using fn_csqrt = internal::in_complex<>::fn_csqrt;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> csqrt(const T1& x)
{
    return internal::in_complex<>::csqrt(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_csqrt, E1> csqrt(E1&& x)
{
    return { fn_csqrt(), std::forward<E1>(x) };
}
}
}
namespace cometa
{
template <typename T>
struct compound_type_traits<kfr::complex<T>>
{
    constexpr static size_t width   = 2;
    using subtype                   = T;
    using deep_subtype              = cometa::deep_subtype<T>;
    constexpr static bool is_scalar = false;
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

#pragma clang diagnostic pop
