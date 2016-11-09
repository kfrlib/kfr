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

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4814))

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
    constexpr complex(const complex<U>& other) noexcept
        : re(static_cast<T>(other.re)), im(static_cast<T>(other.im))
    {
    }
    template <typename U>
    constexpr complex(complex<U>&& other) noexcept : re(std::move(other.re)), im(std::move(other.im))
    {
    }
#ifdef CMT_COMPILER_GNU
    constexpr complex& operator=(const complex&) noexcept = default;
    constexpr complex& operator=(complex&&) noexcept = default;
#else
    complex& operator=(const complex&) = default;
    complex& operator=(complex&&) = default;
#endif
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

    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator+(const complex& x, const U& y)
    {
        return static_cast<C>(x) + static_cast<C>(y);
    }
    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator-(const complex& x, const U& y)
    {
        return static_cast<C>(x) - static_cast<C>(y);
    }
    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator*(const complex& x, const U& y)
    {
        return static_cast<C>(x) * static_cast<C>(y);
    }
    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator/(const complex& x, const U& y)
    {
        return static_cast<C>(x) / static_cast<C>(y);
    }

    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator+(const U& x, const complex& y)
    {
        return static_cast<C>(x) + static_cast<C>(y);
    }
    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator-(const U& x, const complex& y)
    {
        return static_cast<C>(x) - static_cast<C>(y);
    }
    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator*(const U& x, const complex& y)
    {
        return static_cast<C>(x) * static_cast<C>(y);
    }
    template <typename U, KFR_ENABLE_IF(is_number<U>::value), typename C = common_type<complex, U>>
    KFR_INTRIN friend C operator/(const U& x, const complex& y)
    {
        return static_cast<C>(x) / static_cast<C>(y);
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
    using deep_rebind = kfr::complex<typename compound_type_traits<subtype>::template deep_rebind<U>>;

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

namespace internal
{
template <typename T>
constexpr inline vec<T, 2> vcomplex(const complex<T>& v)
{
    return vec<T, 2>(v.real(), v.imag());
}
}

template <typename T, size_t N>
struct vec<complex<T>, N> : private vec<T, 2 * N>
{
    using base = vec<T, 2 * N>;

    using value_type = complex<T>;
    constexpr static size_t size() noexcept { return N; }

    using scalar_type = T;
    constexpr static size_t scalar_size() noexcept { return 2 * N; }

    using simd_type = typename base::simd_type;

    constexpr vec() noexcept           = default;
    constexpr vec(const vec&) noexcept = default;
    CMT_GNU_CONSTEXPR vec& operator=(const vec&) CMT_GNU_NOEXCEPT = default;
    template <int                                                 = 0>
    constexpr vec(const simd_type& simd) noexcept : base(simd)
    {
    }
    KFR_I_CE vec(czeros_t) noexcept : base(czeros) {}
    KFR_I_CE vec(cones_t) noexcept : base(cones) {}
    KFR_I_CE vec(const value_type& s) noexcept : base(repeat<N>(vec<T, 2>(s.real(), s.imag()))) {}

    template <typename U>
    KFR_I_CE vec(const complex<U>& s) noexcept
        : base(repeat<N>(vec<T, 2>(static_cast<T>(s.real()), static_cast<T>(s.imag()))))
    {
    }
    template <typename U>
    KFR_I_CE vec(const vec<complex<U>, N>& v) noexcept : base(static_cast<vec<T, N * 2>>(v.flatten()))
    {
    }

    explicit KFR_I_CE vec(const vec<T, N * 2>& v) noexcept : base(v) {}

    // from real
    KFR_I_CE vec(const T& r) noexcept : base(interleave(vec<T, N>(r), vec<T, N>(czeros))) {}
    // from real
    template <typename U, typename = enable_if<std::is_convertible<U, T>::value>>
    KFR_I_CE vec(const vec<U, N>& r) noexcept : base(interleave(vec<T, N>(r), vec<T, N>(czeros)))
    {
    }

    // from list of vectors
    template <typename... Us>
    KFR_I_CE vec(const value_type& s0, const value_type& s1, const Us&... rest) noexcept
        : base(internal::vcomplex(s0), internal::vcomplex(s1),
               internal::vcomplex(static_cast<value_type>(rest))...)
    {
    }

    template <typename U, size_t M, KFR_ENABLE_IF(sizeof(U) * M == sizeof(value_type) * N)>
    KFR_I_CE static vec frombits(const vec<U, M>& v) noexcept
    {
        return vec(vec<T, scalar_size()>::frombits(v.flatten()));
    }

#define KFR_B(x) static_cast<const base&>(x)
    // math / bitwise / comparison operators
    KFR_I_CE friend vec operator+(const vec& x) noexcept { return x; }
    KFR_I_CE friend vec operator-(const vec& x) noexcept { return vec(-KFR_B(x)); }
    KFR_I_CE friend vec operator~(const vec& x) noexcept { return vec(~KFR_B(x)); }

    KFR_I_CE friend vec operator+(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) + KFR_B(y)); }
    KFR_I_CE friend vec operator-(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) - KFR_B(y)); }
    CMT_GNU_CONSTEXPR friend vec operator*(const vec& x, const vec& y) noexcept
    {
        const vec<scalar_type, N* 2> xx = x;
        const vec<scalar_type, N* 2> yy = y;
        return vec(subadd(xx * dupeven(yy), swap<2>(xx) * dupodd(yy)));
    }
    CMT_GNU_CONSTEXPR friend vec operator/(const vec& x, const vec& y) noexcept
    {
        const vec<scalar_type, N* 2> xx = x;
        const vec<scalar_type, N* 2> yy = y;
        const vec<scalar_type, N* 2> m  = (sqr(dupeven(yy)) + sqr(dupodd(yy)));
        return vec(swap<2>(subadd(swap<2>(xx) * dupeven(yy), xx * dupodd(yy)) / m));
    }

    KFR_I_CE friend vec operator&(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) & KFR_B(y)); }
    KFR_I_CE friend vec operator|(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) | KFR_B(y)); }
    KFR_I_CE friend vec operator^(const vec& x, const vec& y) noexcept { return vec(KFR_B(x) ^ KFR_B(y)); }

    KFR_I_CE friend vec& operator+=(vec& x, const vec& y) noexcept { return x = x + y; }
    KFR_I_CE friend vec& operator-=(vec& x, const vec& y) noexcept { return x = x - y; }
    KFR_I_CE friend vec& operator*=(vec& x, const vec& y) noexcept { return x = x * y; }
    KFR_I_CE friend vec& operator/=(vec& x, const vec& y) noexcept { return x = x / y; }

    KFR_I_CE friend vec& operator&=(vec& x, const vec& y) noexcept { return x = x & y; }
    KFR_I_CE friend vec& operator|=(vec& x, const vec& y) noexcept { return x = x | y; }
    KFR_I_CE friend vec& operator^=(vec& x, const vec& y) noexcept { return x = x ^ y; }

    KFR_I_CE friend vec& operator++(vec& x) noexcept { return x = x + vec(1); }
    KFR_I_CE friend vec& operator--(vec& x) noexcept { return x = x - vec(1); }
    KFR_I_CE friend vec operator++(vec& x, int)noexcept
    {
        const vec z = x;
        ++x;
        return z;
    }
    KFR_I_CE friend vec operator--(vec& x, int)noexcept
    {
        const vec z = x;
        --x;
        return z;
    }

    // shuffle
    template <size_t... indices>
    KFR_I_CE vec<value_type, sizeof...(indices)> shuffle(csizes_t<indices...>) const noexcept
    {
        return *base::shuffle(scale<2, indices...>());
    }
    template <size_t... indices>
    KFR_I_CE vec<value_type, sizeof...(indices)> shuffle(const vec& y, csizes_t<indices...>) const noexcept
    {
        return *base::shuffle(y, scale<2, indices...>());
    }

    // element access
    struct element;
    KFR_I_CE value_type operator[](size_t index) const noexcept { return get(index); }
    KFR_I_CE element operator[](size_t index) noexcept { return { *this, index }; }

    KFR_I_CE value_type get(size_t index) const noexcept
    {
        return reinterpret_cast<const value_type(&)[N]>(*this)[index];
    }
    KFR_I_CE void set(size_t index, const value_type& s) noexcept
    {
        reinterpret_cast<value_type(&)[N]>(*this)[index] = s;
    }
    template <size_t index>
    KFR_I_CE value_type get(csize_t<index>) const noexcept
    {
        return static_cast<const base&>(*this).shuffle(csizeseq_t<2, index * 2>());
    }
    template <size_t index>
    KFR_I_CE void set(csize_t<index>, const value_type& s) noexcept
    {
        *this = vec(static_cast<const base&>(*this))
                    .shuffle(s, csizeseq_t<N>() +
                                    (csizeseq_t<N>() >= csize_t<index * 2>() &&
                                     csizeseq_t<N>() < csize_t<(index + 1) * 2>()) *
                                        N);
    }
    struct element
    {
        KFR_I_CE operator value_type() const noexcept { return v.get(index); }
        element& operator=(const value_type& s) noexcept
        {
            v.set(index, s);
            return *this;
        }
        vec& v;
        size_t index;
    };

    template <bool aligned = false>
    explicit KFR_I_CE vec(const value_type* src, cbool_t<aligned> = cbool_t<aligned>()) noexcept
        : base(ptr_cast<T>(src), cbool_t<aligned>())
    {
    }
    template <bool aligned = false>
    const vec& write(value_type* dest, cbool_t<aligned> = cbool_t<aligned>()) const noexcept
    {
        base::write(ptr_cast<T>(dest), cbool_t<aligned>());
        return *this;
    }

    const base& flatten() const noexcept { return *this; }
    simd_type operator*() const noexcept { return base::operator*(); }
    simd_type& operator*() noexcept { return base::operator*(); }
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

template <typename T, KFR_ENABLE_IF(is_numeric<T>::value)>
constexpr CMT_INLINE T real(const T& value)
{
    return value;
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

template <typename T, size_t N>
KFR_SINTRIN vec<T, N> cabs(const vec<T, N>& a)
{
    return to_scalar(intrinsics::cabs(static_cast<vec<complex<T>, N>>(a)));
}
template <typename T, size_t N>
KFR_SINTRIN vec<T, N> carg(const vec<T, N>& a)
{
    return to_scalar(intrinsics::carg(static_cast<vec<complex<T>, N>>(a)));
}
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
KFR_FUNC T1 csin(const T1& x)
{
    return intrinsics::csin(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::csin, E1> csin(E1&& x)
{
    return { fn::csin(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 csinh(const T1& x)
{
    return intrinsics::csinh(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::csinh, E1> csinh(E1&& x)
{
    return { fn::csinh(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 ccos(const T1& x)
{
    return intrinsics::ccos(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::ccos, E1> ccos(E1&& x)
{
    return { fn::ccos(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 ccosh(const T1& x)
{
    return intrinsics::ccosh(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::ccosh, E1> ccosh(E1&& x)
{
    return { fn::ccosh(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC realtype<T1> cabs(const T1& x)
{
    return intrinsics::cabs(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::cabs, E1> cabs(E1&& x)
{
    return { fn::cabs(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC realtype<T1> carg(const T1& x)
{
    return intrinsics::carg(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::carg, E1> carg(E1&& x)
{
    return { fn::carg(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 clog(const T1& x)
{
    return intrinsics::clog(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::clog, E1> clog(E1&& x)
{
    return { fn::clog(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 clog2(const T1& x)
{
    return intrinsics::clog2(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::clog2, E1> clog2(E1&& x)
{
    return { fn::clog2(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 clog10(const T1& x)
{
    return intrinsics::clog10(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::clog10, E1> clog10(E1&& x)
{
    return { fn::clog10(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 cexp(const T1& x)
{
    return intrinsics::cexp(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::cexp, E1> cexp(E1&& x)
{
    return { fn::cexp(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 cexp2(const T1& x)
{
    return intrinsics::cexp2(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::cexp2, E1> cexp2(E1&& x)
{
    return { fn::cexp2(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 cexp10(const T1& x)
{
    return intrinsics::cexp10(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::cexp10, E1> cexp10(E1&& x)
{
    return { fn::cexp10(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 polar(const T1& x)
{
    return intrinsics::polar(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::polar, E1> polar(E1&& x)
{
    return { fn::polar(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 cartesian(const T1& x)
{
    return intrinsics::cartesian(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::cartesian, E1> cartesian(E1&& x)
{
    return { fn::cartesian(), std::forward<E1>(x) };
}
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_FUNC T1 csqrt(const T1& x)
{
    return intrinsics::csqrt(x);
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_FUNC internal::expression_function<fn::csqrt, E1> csqrt(E1&& x)
{
    return { fn::csqrt(), std::forward<E1>(x) };
}
}

namespace std
{
template <typename T1, typename T2>
struct common_type<kfr::complex<T1>, kfr::complex<T2>>
{
    using type = kfr::complex<typename common_type<T1, T2>::type>;
};
template <typename T1, typename T2>
struct common_type<kfr::complex<T1>, T2>
{
    using type = kfr::complex<typename common_type<T1, T2>::type>;
};
template <typename T1, typename T2>
struct common_type<T1, kfr::complex<T2>>
{
    using type = kfr::complex<typename common_type<T1, T2>::type>;
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::complex<T1>, kfr::vec<kfr::complex<T2>, N>>
{
    using type = kfr::vec<kfr::complex<typename common_type<T1, T2>::type>, N>;
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::vec<kfr::complex<T1>, N>, kfr::complex<T2>>
{
    using type = kfr::vec<kfr::complex<typename common_type<T1, T2>::type>, N>;
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::complex<T1>, kfr::vec<T2, N>>
{
    using type = kfr::vec<kfr::complex<typename common_type<T1, T2>::type>, N>;
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::vec<T1, N>, kfr::complex<T2>>
{
    using type = kfr::vec<kfr::complex<typename common_type<T1, T2>::type>, N>;
};
}

CMT_PRAGMA_MSVC(warning(pop))
