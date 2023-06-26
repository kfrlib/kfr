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
#include "constants.hpp"
#include "impl/function.hpp"
#include "operators.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4814))

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

template <typename T>
KFR_INTRINSIC complex<T> operator+(const complex<T>& x, const complex<T>& y)
{
    return (make_vector(x) + make_vector(y))[0];
}
template <typename T>
KFR_INTRINSIC complex<T> operator-(const complex<T>& x, const complex<T>& y)
{
    return (make_vector(x) - make_vector(y))[0];
}
template <typename T>
KFR_INTRINSIC complex<T> operator*(const complex<T>& x, const complex<T>& y)
{
    return (make_vector(x) * make_vector(y))[0];
}
template <typename T>
KFR_INTRINSIC complex<T> operator/(const complex<T>& x, const complex<T>& y)
{
    return (make_vector(x) / make_vector(y))[0];
}
template <typename T>
KFR_INTRINSIC complex<T>& operator+=(complex<T>& x, const complex<T>& y)
{
    x = x + y;
    return x;
}
template <typename T>
KFR_INTRINSIC complex<T>& operator-=(complex<T>& x, const complex<T>& y)
{
    x = x - y;
    return x;
}
template <typename T>
KFR_INTRINSIC complex<T>& operator*=(complex<T>& x, const complex<T>& y)
{
    x = x * y;
    return x;
}
template <typename T>
KFR_INTRINSIC complex<T>& operator/=(complex<T>& x, const complex<T>& y)
{
    x = x / y;
    return x;
}

template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator+(const complex<T>& x, const U& y)
{
    return static_cast<C>(x) + static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator-(const complex<T>& x, const U& y)
{
    return static_cast<C>(x) - static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator*(const complex<T>& x, const U& y)
{
    return static_cast<C>(x) * static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator/(const complex<T>& x, const U& y)
{
    return static_cast<C>(x) / static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>)>
KFR_INTRINSIC complex<T>& operator+=(complex<T>& x, const U& y)
{
    x = x + y;
    return x;
}
template <typename T, typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>)>
KFR_INTRINSIC complex<T>& operator-=(complex<T>& x, const U& y)
{
    x = x - y;
    return x;
}
template <typename T, typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>)>
KFR_INTRINSIC complex<T>& operator*=(complex<T>& x, const U& y)
{
    x = x * y;
    return x;
}
template <typename T, typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>)>
KFR_INTRINSIC complex<T>& operator/=(complex<T>& x, const U& y)
{
    x = x / y;
    return x;
}

template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator+(const U& x, const complex<T>& y)
{
    return static_cast<C>(x) + static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator-(const U& x, const complex<T>& y)
{
    return static_cast<C>(x) - static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator*(const U& x, const complex<T>& y)
{
    return static_cast<C>(x) * static_cast<C>(y);
}
template <typename T, typename U, KFR_ENABLE_IF(is_number<U>), typename C = std::common_type_t<complex<T>, U>>
KFR_INTRINSIC C operator/(const U& x, const complex<T>& y)
{
    return static_cast<C>(x) / static_cast<C>(y);
}
template <typename T>
KFR_INTRINSIC complex<T> operator-(const complex<T>& x)
{
    return (-make_vector(x))[0];
}
template <typename T>
KFR_INTRINSIC complex<T> operator+(const complex<T>& x)
{
    return x;
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
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
} // namespace cometa
namespace kfr
{

/// @brief Alias for complex<f32>
using c32 = complex<f32>;

/// @brief Alias for complex<f64>
using c64 = complex<f64>;

/// @brief Alias for complex<fbase>
using cbase = complex<fbase>;

inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{
template <typename T>
constexpr inline complex<T> vcomplex(const vec<T, 2>& v)
{
    return complex<T>(v.front(), v.back());
}
template <typename T>
constexpr inline vec<T, 2> vcomplex(const complex<T>& v)
{
    return vec<T, 2>(v.real(), v.imag());
}
template <typename T>
constexpr inline simd<T, 2> vvcomplex(const complex<T>& v)
{
    return intrinsics::simd_make(cometa::ctype<T>, v.real(), v.imag());
}
} // namespace intrinsics

template <typename T, size_t N, size_t... indices>
KFR_INTRINSIC vec<complex<T>, sizeof...(indices)> shufflevector(const vec<complex<T>, N>& x,
                                                                csizes_t<indices...>) CMT_NOEXCEPT
{
    return intrinsics::simd_shuffle(intrinsics::simd_t<unwrap_bit<T>, N>{}, x.v, scale<2, indices...>(),
                                    overload_auto);
}
template <typename T, size_t N, size_t... indices>
KFR_INTRINSIC vec<complex<T>, sizeof...(indices)> shufflevectors(const vec<complex<T>, N>& x,
                                                                 const vec<T, N>& y,
                                                                 csizes_t<indices...>) CMT_NOEXCEPT
{
    return intrinsics::simd_shuffle(intrinsics::simd2_t<unwrap_bit<T>, N, N>{}, x.v, y.v,
                                    scale<2, indices...>(), overload_auto);
}
namespace internal
{
template <typename T>
struct compoundcast<complex<T>>
{
    static vec<T, 2> to_flat(const complex<T>& x) { return { x.real(), x.imag() }; }
    static complex<T> from_flat(const vec<T, 2>& x) { return { x.front(), x.back() }; }
};

template <typename T, size_t N>
struct compoundcast<vec<complex<T>, N>>
{
    static vec<T, N * 2> to_flat(const vec<complex<T>, N>& x) { return x.flatten(); }
    static vec<complex<T>, N / 2> from_flat(const vec<T, N>& x)
    {
        return vec<complex<T>, N / 2>::from_flatten(x);
    }
};
} // namespace internal

template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<complex<T>, N / 2> ccomp(const vec<T, N>& x)
{
    return vec<complex<T>, N / 2>::from_flatten(x);
}

template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N * 2> cdecom(const vec<complex<T>, N>& x)
{
    return x.flatten();
}

/// @brief Returns vector of complex values with real part duplicated
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cdupreal(const vec<complex<T>, N>& x)
{
    return ccomp(dupeven(cdecom(x)));
}
KFR_FN(cdupreal)

/// @brief Returns vector of complex values with imaginary part duplicated
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cdupimag(const vec<complex<T>, N>& x)
{
    return ccomp(dupodd(cdecom(x)));
}
KFR_FN(cdupimag)

/// @brief Returns vector of complex values with real and imaginary parts swapped
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cswapreim(const vec<complex<T>, N>& x)
{
    return ccomp(swap<2>(cdecom(x)));
}
KFR_FN(cswapreim)

/// @brief Returns vector of complex values with real part negated
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cnegreal(const vec<complex<T>, N>& x)
{
    return x ^ complex<T>(-T(), T());
}
KFR_FN(cnegreal)

/// @brief Returns vector of complex values with imaginary part negated
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cnegimag(const vec<complex<T>, N>& x)
{
    return x ^ complex<T>(T(), -T());
}
KFR_FN(cnegimag)

/// @brief Returns mask with true for real elements
template <typename T>
KFR_INTRINSIC bool isreal(const complex<T>& x)
{
    return x.imag() == 0;
}
KFR_FN(isreal)

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
template <typename To, typename From, size_t N, conv_t conv>
struct conversion<1, 1, vec<complex<To>, N>, vec<complex<From>, N>, conv>
{
    static_assert(!is_compound_type<To>, "");
    static_assert(!is_compound_type<From>, "");
    static vec<complex<To>, N> cast(const vec<complex<From>, N>& value)
    {
        return vec<To, N * 2>(value.flatten()).v;
    }
};

// vector to vector<complex>
template <typename To, typename From, size_t N, conv_t conv>
struct conversion<1, 1, vec<complex<To>, N>, vec<From, N>, conv>
{
    static_assert(!is_compound_type<To>, "");
    static_assert(!is_compound_type<From>, "");
    static vec<complex<To>, N> cast(const vec<From, N>& value)
    {
        const vec<To, N> casted = static_cast<vec<To, N>>(value);
        return interleave(casted, zerovector(casted)).v;
    }
};

} // namespace internal

/// @brief Returns the real part of the complex value
template <typename T, KFR_ENABLE_IF(is_numeric<T>)>
constexpr KFR_INTRINSIC T real(const T& value)
{
    return value;
}

/// @brief Returns the real part of the complex value
template <typename T>
constexpr KFR_INTRINSIC T real(const complex<T>& value)
{
    return value.real();
}

/// @brief Returns the real part of the complex value
template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> real(const vec<complex<T>, N>& value)
{
    return even(cdecom(value));
}

template <typename T>
using realtype = decltype(kfr::real(std::declval<T>()));
template <typename T>
using realftype = ftype<decltype(kfr::real(std::declval<T>()))>;

KFR_FN(real)

/// @brief Returns the imaginary part of the complex value
template <typename T>
constexpr KFR_INTRINSIC T imag(const complex<T>& value)
{
    return value.imag();
}

/// @brief Returns the imaginary part of the complex value
template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> imag(const vec<complex<T>, N>& value)
{
    return odd(cdecom(value));
}
KFR_FN(imag)

/// @brief Constructs complex value from real and imaginary parts
template <typename T1, typename T2 = T1, size_t N, typename T = std::common_type_t<T1, T2>>
constexpr KFR_INTRINSIC vec<complex<T>, N> make_complex(const vec<T1, N>& real,
                                                        const vec<T2, N>& imag = T2(0))
{
    return ccomp(interleave(promoteto<T>(real), promoteto<T>(imag)));
}

/// @brief Constructs complex value from real and imaginary parts
template <typename T1, typename T2 = T1, typename T = std::common_type_t<T1, T2>,
          KFR_ENABLE_IF(is_numeric_args<T1, T2>)>
constexpr KFR_INTRINSIC complex<T> make_complex(T1 real, T2 imag = T2(0))
{
    return complex<T>(promoteto<T>(real), promoteto<T>(imag));
}
KFR_FN(make_complex)

namespace intrinsics
{
template <typename T, size_t N>
KFR_INTRINSIC vec<complex<T>, N> cconj(const vec<complex<T>, N>& x)
{
    return cnegimag(x);
}

KFR_HANDLE_SCALAR(cconj)
} // namespace intrinsics
KFR_I_FN(cconj)

/// @brief Returns the complex conjugate of the complex number x
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_INTRINSIC T1 cconj(const T1& x)
{
    return intrinsics::cconj(x);
}

template <size_t N>
struct vec_of_complex
{
    template <typename T>
    using type = vec<complex<T>, N>;
};
} // namespace CMT_ARCH_NAME
} // namespace kfr

namespace std
{

template <typename T1, typename T2>
struct common_type<kfr::complex<T1>, kfr::complex<T2>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::complex>
{
};
template <typename T1, typename T2>
struct common_type<kfr::complex<T1>, T2> : kfr::construct_common_type<std::common_type<T1, T2>, kfr::complex>
{
};
template <typename T1, typename T2>
struct common_type<T1, kfr::complex<T2>> : kfr::construct_common_type<std::common_type<T1, T2>, kfr::complex>
{
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::complex<T1>, kfr::vec<T2, N>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vec_of_complex<N>::template type>
{
};
template <typename T1, typename T2, size_t N>
struct common_type<kfr::vec<T1, N>, kfr::complex<T2>>
    : kfr::construct_common_type<std::common_type<T1, T2>, kfr::vec_of_complex<N>::template type>
{
};
} // namespace std

CMT_PRAGMA_MSVC(warning(pop))
