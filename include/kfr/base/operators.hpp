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

#include "function.hpp"
#include <algorithm>
#include <utility>

namespace kfr
{
namespace internal
{

template <typename T, typename ReduceFn>
KFR_INLINE T horizontal_impl(vec<T, 1> value, ReduceFn&&)
{
    return T(value[0]);
}

template <typename T, size_t N, typename ReduceFn, KFR_ENABLE_IF(N > 1 && is_poweroftwo(N))>
KFR_INLINE T horizontal_impl(vec<T, N> value, ReduceFn&& reduce)
{
    return horizontal_impl(reduce(low(value), high(value)), std::forward<ReduceFn>(reduce));
}
template <typename T, size_t N, typename ReduceFn, KFR_ENABLE_IF(N > 1 && !is_poweroftwo(N))>
KFR_INLINE T horizontal_impl(vec<T, N> value, ReduceFn&& reduce)
{
    const T initial = reduce(initialvalue<T>());
    return horizontal_impl(widen<next_poweroftwo(N)>(value, initial), std::forward<ReduceFn>(reduce));
}
}

template <typename T, size_t N, typename ReduceFn>
KFR_INLINE T horizontal(vec<T, N> value, ReduceFn&& reduce)
{
    return internal::horizontal_impl(value, std::forward<ReduceFn>(reduce));
}

template <typename T>
constexpr inline T add(T x)
{
    return x;
}
template <typename T1, typename T2, typename... Ts>
constexpr inline common_type<T1, T2, Ts...> add(T1 x, T2 y, Ts... rest)
{
    return x + add(std::forward<T2>(y), std::forward<Ts>(rest)...);
}
template <typename T>
constexpr inline T add(initialvalue<T>)
{
    return T(0);
}
KFR_FN(add)

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_add, E1, E2> add(E1&& x, E2&& y)
{
    return { fn_add(), std::forward<E1>(x), std::forward<E2>(y) };
}
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_add, E1> add(E1&& x, E2&& y, E3&& z)
{
    return { fn_add(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z)

    };
}

template <typename T1, typename T2>
constexpr inline common_type<T1, T2> sub(T1 x, T2 y)
{
    return x - y;
}
template <typename T>
constexpr inline T sub(initialvalue<T>)
{
    return T(0);
}
KFR_FN(sub)

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_sub, E1, E2> sub(E1&& x, E2&& y)
{
    return { fn_sub(), std::forward<E1>(x), std::forward<E2>(y)

    };
}

template <typename T1>
constexpr inline T1 mul(T1 x)
{
    return x;
}
template <typename T1, typename T2, typename... Ts>
constexpr inline common_type<T1, T2, Ts...> mul(T1 x, T2 y, Ts... rest)
{
    return x * mul(std::forward<T2>(y), std::forward<Ts>(rest)...);
}

template <typename T>
constexpr inline T mul(initialvalue<T>)
{
    return T(1);
}
KFR_FN(mul)
template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_mul, E1, E2> mul(E1&& x, E2&& y)
{
    return { fn_mul(), std::forward<E1>(x), std::forward<E2>(y) };
}
template <typename E1, typename E2, typename E3, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_mul, E1> mul(E1&& x, E2&& y, E3&& z)
{
    return { fn_mul(), std::forward<E1>(x), std::forward<E2>(y), std::forward<E3>(z) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
constexpr inline T1 sqr(T1 x)
{
    return x * x;
}
KFR_FN(sqr)
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_sqr, E1> sqr(E1&& x)
{
    return { fn_sqr(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
constexpr inline T1 cub(T1 x)
{
    return sqr(x) * x;
}
KFR_FN(cub)

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_cub, E1> cub(E1&& x)
{
    return { fn_cub(), std::forward<E1>(x)

    };
}

template <typename T>
constexpr inline T pow2(T x)
{
    return sqr(x);
}

template <typename T>
constexpr inline T pow3(T x)
{
    return cub(x);
}

template <typename T>
constexpr inline T pow4(T x)
{
    return sqr(sqr(x));
}

template <typename T>
constexpr inline T pow5(T x)
{
    return pow4(x) * x;
}
KFR_FN(pow2)
KFR_FN(pow3)
KFR_FN(pow4)
KFR_FN(pow5)

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_pow2, E1> pow2(E1&& x)
{
    return { fn_pow2(), std::forward<E1>(x)

    };
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_pow3, E1> pow3(E1&& x)
{
    return { fn_pow3(), std::forward<E1>(x)

    };
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_pow4, E1> pow4(E1&& x)
{
    return { fn_pow4(), std::forward<E1>(x)

    };
}
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INLINE expr_func<fn_pow5, E1> pow5(E1&& x)
{
    return { fn_pow5(), std::forward<E1>(x)

    };
}

/// Raise x to the power base $x^{base}$
/// @code
/// CHECK( ipow( 10, 3 ) == 1000 );
/// CHECK( ipow( 0.5, 2 ) == 0.25 );
/// @endcode
template <typename T>
constexpr inline T ipow(T x, int base)
{
    T result = T(1);
    while (base)
    {
        if (base & 1)
            result *= x;
        base >>= 1;
        x *= x;
    }
    return result;
}
KFR_FN(ipow)

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_ipow, E1, E2> ipow(E1&& x, E2&& b)
{
    return { fn_ipow(), std::forward<E1>(x), std::forward<E2>(b)

    };
}

/// Return square of the sum of all arguments
/// *Example*::
///
/// CHECK(sqrsum(1,2,3) == 36);
template <typename T1, typename... Ts>
constexpr inline common_type<T1, Ts...> sqrsum(T1 x, Ts... rest)
{
    return sqr(add(x, std::forward<Ts>(rest)...));
}

template <typename T1, typename T2>
constexpr inline common_type<T1, T2> sqrdiff(T1 x, T2 y)
{
    return sqr(x - y);
}
KFR_FN(sqrsum)
KFR_FN(sqrdiff)

/// Division
template <typename T1, typename T2>
inline common_type<T1, T2> div(T1 x, T2 y)
{
    return x / y;
}
KFR_FN(div)

/// Remainder
template <typename T1, typename T2>
inline common_type<T1, T2> rem(T1 x, T2 y)
{
    return x % y;
}
KFR_FN(rem)

/// Negation
template <typename T1>
inline T1 neg(T1 x)
{
    return -x;
}
KFR_FN(neg)

/// Bitwise Not
template <typename T1>
inline T1 bitwisenot(T1 x)
{
    return ~x;
}
KFR_FN(bitwisenot)

/// Bitwise And
template <typename T1, typename T2>
inline common_type<T1, T2> bitwiseand(T1 x, T2 y)
{
    return x & y;
}
template <typename T>
constexpr inline T bitwiseand(initialvalue<T>)
{
    return internal::allones<subtype<T>>;
}
KFR_FN(bitwiseand)

/// Bitwise And-Not
template <typename T1, typename T2>
inline common_type<T1, T2> bitwiseandnot(T1 x, T2 y)
{
    return x & ~y;
}
template <typename T>
constexpr inline T bitwiseandnot(initialvalue<T>)
{
    return internal::allones<subtype<T>>;
}
KFR_FN(bitwiseandnot)

/// Bitwise Or
template <typename T1, typename T2>
inline common_type<T1, T2> bitwiseor(T1 x, T2 y)
{
    return x | y;
}
template <typename T>
constexpr inline T bitwiseor(initialvalue<T>)
{
    return subtype<T>();
}
KFR_FN(bitwiseor)

/// Bitwise Xor (Exclusive Or)
template <typename T1, typename T2>
inline common_type<T1, T2> bitwisexor(T1 x, T2 y)
{
    return x ^ y;
}
template <typename T>
constexpr inline T bitwisexor(initialvalue<T>)
{
    return subtype<T>();
}
KFR_FN(bitwisexor)

/// Bitwise Left shift
template <typename T1, typename T2>
inline common_type<T1, T2> shl(T1 left, T2 right)
{
    return left << right;
}
KFR_FN(shl)

/// Bitwise Right shift
template <typename T1, typename T2>
inline common_type<T1, T2> shr(T1 left, T2 right)
{
    return left >> right;
}
KFR_FN(shr)

/// Bitwise Left Rotate
template <typename T1, typename T2>
inline common_type<T1, T2> rol(T1 left, T2 right)
{
    return shl(left, right) | shr(left, (static_cast<subtype<T1>>(typebits<T1>::bits) - right));
}
KFR_FN(rol)

/// Bitwise Right Rotate
template <typename T1, typename T2>
inline common_type<T1, T2> ror(T1 left, T2 right)
{
    return shr(left, right) | shl(left, (static_cast<subtype<T1>>(typebits<T1>::bits) - right));
}
KFR_FN(ror)

template <typename T1, typename T2>
inline common_type<T1, T2> equal(T1 x, T2 y)
{
    return bitcast<subtype<common_type<T1, T2>>>(x == y);
}
template <typename T1, typename T2>
inline common_type<T1, T2> notequal(T1 x, T2 y)
{
    return bitcast<subtype<common_type<T1, T2>>>(x != y);
}
template <typename T1, typename T2>
inline common_type<T1, T2> less(T1 x, T2 y)
{
    return bitcast<subtype<common_type<T1, T2>>>(x < y);
}
template <typename T1, typename T2>
inline common_type<T1, T2> greater(T1 x, T2 y)
{
    return bitcast<subtype<common_type<T1, T2>>>(x > y);
}
template <typename T1, typename T2>
inline common_type<T1, T2> lessorequal(T1 x, T2 y)
{
    return bitcast<subtype<common_type<T1, T2>>>(x <= y);
}
template <typename T1, typename T2>
inline common_type<T1, T2> greaterorequal(T1 x, T2 y)
{
    return bitcast<subtype<common_type<T1, T2>>>(x >= y);
}
KFR_FN(equal)
KFR_FN(notequal)
KFR_FN(less)
KFR_FN(greater)
KFR_FN(lessorequal)
KFR_FN(greaterorequal)

/// Fused Multiply-Add
template <typename T1, typename T2, typename T3>
constexpr inline common_type<T1, T2, T3> fmadd(T1 x, T2 y, T3 z)
{
    return x * y + z;
}
/// Fused Multiply-Sub
template <typename T1, typename T2, typename T3>
constexpr inline common_type<T1, T2, T3> fmsub(T1 x, T2 y, T3 z)
{
    return x * y - z;
}
KFR_FN(fmadd)
KFR_FN(fmsub)

/// Linear blend of `x` and `y` (`c` must be in the range 0...+1)
/// Returns `x + ( y - x ) * c`
template <typename T1, typename T2, typename T3>
constexpr inline common_type<T1, T2, T3> mix(T1 c, T2 x, T3 y)
{
    return fmadd(c, y - x, x);
}

/// Linear blend of `x` and `y` (`c` must be in the range -1...+1)
template <typename T1, typename T2, typename T3>
constexpr inline common_type<T1, T2, T3> mixs(T1 c, T2 x, T3 y)
{
    return mix(fmadd(c, 0.5, 0.5), x, y);
}
KFR_FN(mix)
KFR_FN(mixs)

namespace internal
{

template <typename T1, typename T2>
constexpr KFR_INLINE T1 horner(T1, T2 c0)
{
    return c0;
}

template <typename T1, typename T2, typename T3, typename... Ts>
constexpr KFR_INLINE T1 horner(T1 x, T2 c0, T3 c1, Ts... values)
{
    return fmadd(horner(x, c1, values...), x, c0);
}
}

/// Calculate polynomial using Horner's method
///
/// ``horner(x, 1, 2, 3)`` is equivalent to \(3x^2 + 2x + 1\)
template <typename T1, typename... Ts>
constexpr KFR_INLINE T1 horner(T1 x, Ts... c)
{
    return internal::horner(x, c...);
}
KFR_FN(horner)

/// Calculate Multiplicative Inverse of `x`
/// Returns `1/x`
template <typename T>
constexpr KFR_INLINE T reciprocal(T x)
{
    static_assert(std::is_floating_point<subtype<T>>::value, "T must be floating point type");
    return subtype<T>(1) / x;
}
KFR_FN(reciprocal)

template <typename T, size_t N>
KFR_INLINE vec<T, N> mulsign(vec<T, N> x, vec<T, N> y)
{
    return x ^ (y & internal::highbitmask<T>);
}
KFR_FN_S(mulsign)
KFR_FN(mulsign)

template <typename T, size_t N>
constexpr KFR_INLINE vec<T, N> copysign(vec<T, N> x, vec<T, N> y)
{
    return (x & internal::highbitmask<T>) | (y & internal::highbitmask<T>);
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_INLINE vec<T, N> fmod(vec<T, N> x, vec<T, N> y)
{
    return x - cast<itype<T>>(x / y) * y;
}

KFR_FN_S(fmod)
KFR_FN(fmod)

template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
constexpr KFR_INLINE vec<T, N> rem(vec<T, N> x, vec<T, N> y)
{
    return x % y;
}
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_INLINE vec<T, N> rem(vec<T, N> x, vec<T, N> y)
{
    return fmod(x, y);
}

template <typename T, size_t N>
KFR_INLINE mask<T, N> isnan(vec<T, N> x)
{
    return x != x;
}

template <typename T, size_t N>
KFR_INLINE mask<T, N> isinf(vec<T, N> x)
{
    return x == c_infinity<T> || x == -c_infinity<T>;
}

template <typename T, size_t N>
KFR_INLINE mask<T, N> isfinite(vec<T, N> x)
{
    return !isnan(x) && !isinf(x);
}

template <typename T, size_t N>
KFR_INLINE mask<T, N> isnegative(vec<T, N> x)
{
    return (x & internal::highbitmask<T>) != 0;
}

template <typename T, size_t N>
KFR_INLINE mask<T, N> ispositive(vec<T, N> x)
{
    return !isnegative(x);
}

template <typename T, size_t N>
KFR_INLINE mask<T, N> iszero(vec<T, N> x)
{
    return x == T();
}

/// Swap byte order
template <typename T, size_t N, KFR_ENABLE_IF(sizeof(vec<T, N>) > 8)>
KFR_INLINE vec<T, N> swapbyteorder(vec<T, N> x)
{
    return bitcast<T>(swap<sizeof(T)>(bitcast<u8>(x)));
}
template <typename T, KFR_ENABLE_IF(sizeof(T) == 8)>
KFR_INLINE T swapbyteorder(T x)
{
    return reinterpret_cast<const T&>(__builtin_bswap64(reinterpret_cast<const u64&>(x)));
}
template <typename T, KFR_ENABLE_IF(sizeof(T) == 4)>
KFR_INLINE T swapbyteorder(T x)
{
    return reinterpret_cast<const T&>(__builtin_bswap32(reinterpret_cast<const u32&>(x)));
}
template <typename T, KFR_ENABLE_IF(sizeof(T) == 2)>
KFR_INLINE T swapbyteorder(T x)
{
    return reinterpret_cast<const T&>(__builtin_bswap16(reinterpret_cast<const u16&>(x)));
}
KFR_FN(swapbyteorder)

/// Sum all elements of the vector
template <typename T, size_t N>
KFR_INLINE T hadd(vec<T, N> value)
{
    return horizontal(value, fn_add());
}
KFR_FN(hadd)

/// Multiply all elements of the vector
template <typename T, size_t N>
KFR_INLINE T hmul(vec<T, N> value)
{
    return horizontal(value, fn_mul());
}
KFR_FN(hmul)

template <typename T, size_t N>
KFR_INLINE T hbitwiseand(vec<T, N> value)
{
    return horizontal(value, fn_bitwiseand());
}
KFR_FN(hbitwiseand)
template <typename T, size_t N>
KFR_INLINE T hbitwiseor(vec<T, N> value)
{
    return horizontal(value, fn_bitwiseor());
}
KFR_FN(hbitwiseor)
template <typename T, size_t N>
KFR_INLINE T hbitwisexor(vec<T, N> value)
{
    return horizontal(value, fn_bitwisexor());
}
KFR_FN(hbitwisexor)

/// Calculate the Dot-Product of two vectors
template <typename T, size_t N>
KFR_INLINE T dot(vec<T, N> x, vec<T, N> y)
{
    return hadd(x * y);
}
KFR_FN(dot)

/// Calculate the Arithmetic mean of all elements in the vector
template <typename T, size_t N>
KFR_INLINE T avg(vec<T, N> value)
{
    return hadd(value) / N;
}
KFR_FN(avg)

/// Calculate the RMS of all elements in the vector
template <typename T, size_t N>
KFR_INLINE T rms(vec<T, N> value)
{
    return internal::builtin_sqrt(hadd(value * value) / N);
}
KFR_FN(rms)

template <typename T, size_t N, KFR_ENABLE_IF(N >= 2)>
KFR_INLINE vec<T, N> subadd(vec<T, N> a, vec<T, N> b)
{
    return blend<1, 0>(a + b, a - b);
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= 2)>
KFR_INLINE vec<T, N> addsub(vec<T, N> a, vec<T, N> b)
{
    return blend<0, 1>(a + b, a - b);
}
KFR_FN(subadd)
KFR_FN(addsub)

template <typename T, size_t N>
KFR_INLINE vec<T, N> negeven(const vec<T, N>& x)
{
    return x ^ broadcast<N>(-T(), T());
}
template <typename T, size_t N>
KFR_INLINE vec<T, N> negodd(const vec<T, N>& x)
{
    return x ^ broadcast<N>(T(), -T());
}
}
