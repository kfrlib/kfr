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

#include "impl/operators.hpp"
#include "mask.hpp"
#include <algorithm>
#include <utility>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4244))

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

#define KFR_VEC_OPERATOR1(op, fn)                                                                            \
    template <typename T, size_t N>                                                                          \
    constexpr KFR_INTRINSIC vec<T, N> operator op(const vec<T, N>& x)                                        \
    {                                                                                                        \
        return intr::fn(x);                                                                                  \
    }

#define KFR_VEC_OPERATOR2(op, asgnop, fn)                                                                    \
    template <typename T1, typename T2, size_t N>                                                            \
        requires(vec_rank<T1> == vec_rank<T2>)                                                               \
    constexpr KFR_INTRINSIC vec<T1, N>& operator asgnop(vec<T1, N>& x, const vec<T2, N>& y)                  \
    {                                                                                                        \
        x = intr::fn(x, promoteto<T1>(y));                                                                   \
        return x;                                                                                            \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N>                                                            \
        requires(1 + vec_rank<T1> > vec_rank<T2>)                                                            \
    constexpr KFR_INTRINSIC vec<T1, N>& operator asgnop(vec<T1, N>& x, const T2& y)                          \
    {                                                                                                        \
        x = intr::fn(x, T1(y));                                                                              \
        return x;                                                                                            \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>                   \
        requires(1 + vec_rank<T1> > vec_rank<T2>)                                                            \
    constexpr KFR_INTRINSIC vec<C, N> operator op(const vec<T1, N>& x, const T2& y)                          \
    {                                                                                                        \
        return intr::fn(promoteto<C>(x), C(y));                                                              \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>                   \
        requires(vec_rank<T1> < 1 + vec_rank<T2>)                                                            \
    constexpr KFR_INTRINSIC vec<C, N> operator op(const T1& x, const vec<T2, N>& y)                          \
    {                                                                                                        \
        return intr::fn(C(x), promoteto<C>(y));                                                              \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>                   \
        requires(vec_rank<T1> == vec_rank<T2>)                                                               \
    constexpr KFR_INTRINSIC vec<C, N> operator op(const vec<T1, N>& x, const vec<T2, N>& y)                  \
    {                                                                                                        \
        return intr::fn(promoteto<C>(x), promoteto<C>(y));                                                   \
    }

#define KFR_VEC_SHIFT_OPERATOR(op, asgnop, fn)                                                               \
    template <typename T1, size_t N>                                                                         \
    constexpr KFR_INTRINSIC vec<T1, N>& operator asgnop(vec<T1, N>& x, unsigned y)                           \
    {                                                                                                        \
        x = intr::fn(x, y);                                                                                  \
        return x;                                                                                            \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N>                                                            \
        requires(vec_rank<T1> == vec_rank<T2>)                                                               \
    constexpr KFR_INTRINSIC vec<T1, N>& operator asgnop(vec<T1, N>& x, const vec<T2, N>& y)                  \
    {                                                                                                        \
        x = intr::fn(x, promoteto<utype<T1>>(y));                                                            \
        return x;                                                                                            \
    }                                                                                                        \
    template <typename T1, size_t N>                                                                         \
    constexpr KFR_INTRINSIC vec<T1, N> operator op(const vec<T1, N>& x, unsigned y)                          \
    {                                                                                                        \
        return intr::fn(x, y);                                                                               \
    }                                                                                                        \
    template <simd_compat T1, typename T2, size_t N>                                                         \
        requires(vec_rank<T1> < 1 + vec_rank<T2>)                                                            \
    constexpr KFR_INTRINSIC vec<T1, N> operator op(const T1& x, const vec<T2, N>& y)                         \
    {                                                                                                        \
        return intr::fn(broadcastto<T1>(x), promoteto<utype<T1>>(y));                                        \
    }                                                                                                        \
    template <simd_compat T1, typename T2, size_t N>                                                         \
        requires(vec_rank<T1> == vec_rank<T2>)                                                               \
    constexpr KFR_INTRINSIC vec<T1, N> operator op(const vec<T1, N>& x, const vec<T2, N>& y)                 \
    {                                                                                                        \
        return intr::fn(x, promoteto<utype<T1>>(y));                                                         \
    }

#define KFR_VEC_CMP_OPERATOR(op, fn)                                                                         \
    template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>                   \
        requires(1 + vec_rank<T1> > vec_rank<T2>)                                                            \
    constexpr KFR_INTRINSIC mask<C, N> operator op(const vec<T1, N>& x, const T2& y)                         \
    {                                                                                                        \
        return intr::fn(promoteto<C>(x), vec<C, N>(y)).asmask();                                             \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>                   \
        requires(vec_rank<T1> < 1 + vec_rank<T2>)                                                            \
    constexpr KFR_INTRINSIC mask<C, N> operator op(const T1& x, const vec<T2, N>& y)                         \
    {                                                                                                        \
        return intr::fn(vec<C, N>(x), promoteto<C>(y)).asmask();                                             \
    }                                                                                                        \
    template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>                   \
        requires(vec_rank<T1> == vec_rank<T2>)                                                               \
    constexpr KFR_INTRINSIC mask<C, N> operator op(const vec<T1, N>& x, const vec<T2, N>& y)                 \
    {                                                                                                        \
        return intr::fn(promoteto<C>(x), promoteto<C>(y)).asmask();                                          \
    }

KFR_VEC_OPERATOR1(-, neg)
KFR_VEC_OPERATOR1(~, bnot)

KFR_VEC_OPERATOR2(+, +=, add)
KFR_VEC_OPERATOR2(-, -=, sub)
KFR_VEC_OPERATOR2(*, *=, mul)
KFR_VEC_OPERATOR2(/, /=, div)
KFR_VEC_OPERATOR2(%, %=, mod)

KFR_VEC_OPERATOR2(&, &=, band)
KFR_VEC_OPERATOR2(|, |=, bor)
KFR_VEC_OPERATOR2(^, ^=, bxor)
KFR_VEC_SHIFT_OPERATOR(<<, <<=, shl)
KFR_VEC_SHIFT_OPERATOR(>>, >>=, shr)

KFR_VEC_CMP_OPERATOR(==, eq)
KFR_VEC_CMP_OPERATOR(!=, ne)
KFR_VEC_CMP_OPERATOR(>=, ge)
KFR_VEC_CMP_OPERATOR(<=, le)
KFR_VEC_CMP_OPERATOR(>, gt)
KFR_VEC_CMP_OPERATOR(<, lt)

/** @brief Bitwise AND of two masks (bit-level, size must match). */
template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>
    requires(sizeof(T1) == sizeof(T2))
KFR_INTRINSIC mask<C, N> operator&(const mask<T1, N>& x, const mask<T2, N>& y) noexcept
{
    return mask<C, N>((bitcast<C>(vec<T1, N>(x.v)) & bitcast<C>(vec<T2, N>(y.v))).v);
}
/** @brief Bitwise OR of two masks (bit-level, size must match). */
template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>
    requires(sizeof(T1) == sizeof(T2))
KFR_INTRINSIC mask<C, N> operator|(const mask<T1, N>& x, const mask<T2, N>& y) noexcept
{
    return mask<C, N>((bitcast<C>(vec<T1, N>(x.v)) | bitcast<C>(vec<T2, N>(y.v))).v);
}
/** @brief Logical AND of two masks (bit-level conjunction, size must match). */
template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>
    requires(sizeof(T1) == sizeof(T2))
KFR_INTRINSIC mask<C, N> operator&&(const mask<T1, N>& x, const mask<T2, N>& y) noexcept
{
    return mask<C, N>((bitcast<C>(vec<T1, N>(x.v)) & bitcast<C>(vec<T2, N>(y.v))).v);
}
/** @brief Logical OR of two masks (bit-level disjunction, size must match). */
template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>
    requires(sizeof(T1) == sizeof(T2))
KFR_INTRINSIC mask<C, N> operator||(const mask<T1, N>& x, const mask<T2, N>& y) noexcept
{
    return mask<C, N>((bitcast<C>(vec<T1, N>(x.v)) | bitcast<C>(vec<T2, N>(y.v))).v);
}
/** @brief Bitwise XOR of two masks (bit-level, size must match). */
template <typename T1, typename T2, size_t N, typename C = std::common_type_t<T1, T2>>
    requires(sizeof(T1) == sizeof(T2))
KFR_INTRINSIC mask<C, N> operator^(const mask<T1, N>& x, const mask<T2, N>& y) noexcept
{
    return mask<C, N>((bitcast<C>(vec<T1, N>(x.v)) ^ bitcast<C>(vec<T2, N>(y.v))).v);
}

/** @brief Bitwise NOT of a mask. */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> operator~(const mask<T, N>& x) noexcept
{
    return mask<T, N>(~x.asvec());
}
/** @brief Logical NOT of a mask (equivalent to bitwise NOT). */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> operator!(const mask<T, N>& x) noexcept
{
    return mask<T, N>(~x.asvec());
}

/** @copydoc bitwisenot(const T1&) */
KFR_INTRINSIC float bitwisenot(float x) { return fbitcast(~ubitcast(x)); }
/** @copydoc bitwiseor(const T1&,const T2&) */
KFR_INTRINSIC float bitwiseor(float x, float y) { return fbitcast(ubitcast(x) | ubitcast(y)); }
/** @copydoc bitwiseand(const T1&,const T2&) */
KFR_INTRINSIC float bitwiseand(float x, float y) { return fbitcast(ubitcast(x) & ubitcast(y)); }
/** @copydoc bitwiseandnot(const T1&,const T2&) */
KFR_INTRINSIC float bitwiseandnot(float x, float y) { return fbitcast(ubitcast(x) & ~ubitcast(y)); }
/** @copydoc bitwisexor(const T1&,const T2&) */
KFR_INTRINSIC float bitwisexor(float x, float y) { return fbitcast(ubitcast(x) ^ ubitcast(y)); }

/** @copydoc bitwisenot(const T1&) */
KFR_INTRINSIC double bitwisenot(double x) { return fbitcast(~ubitcast(x)); }
/** @copydoc bitwiseor(const T1&,const T2&) */
KFR_INTRINSIC double bitwiseor(double x, double y) { return fbitcast(ubitcast(x) | ubitcast(y)); }
/** @copydoc bitwiseand(const T1&,const T2&) */
KFR_INTRINSIC double bitwiseand(double x, double y) { return fbitcast(ubitcast(x) & ubitcast(y)); }
/** @copydoc bitwiseandnot(const T1&,const T2&) */
KFR_INTRINSIC double bitwiseandnot(double x, double y) { return fbitcast(ubitcast(x) & ~ubitcast(y)); }
/** @copydoc bitwisexor(const T1&,const T2&) */
KFR_INTRINSIC double bitwisexor(double x, double y) { return fbitcast(ubitcast(x) ^ ubitcast(y)); }

/**
 * @brief Bitwise NOT of `x`.
 *
 * Returns the one's complement of `x`, flipping every bit.
 */
template <typename T1>
KFR_INTRINSIC T1 bitwisenot(const T1& x)
{
    return ~x;
}
KFR_FN(bitwisenot)

/**
 * @brief Bitwise AND of `x` and `y`.
 *
 * Returns the bitwise conjunction of the two operands.
 */
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> bitwiseand(const T1& x, const T2& y)
{
    return x & y;
}
/** @brief Identity element for `bitwiseand` (returns all ones). */
template <typename T>
constexpr KFR_INTRINSIC T bitwiseand(initialvalue<T>)
{
    return special_constants<T>::allones();
}
KFR_FN(bitwiseand)

/**
 * @brief Bitwise AND-NOT of `x` and `y`.
 *
 * Returns `x & ~y`, i.e. the bits set in `x` that are not set in `y`.
 */
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> bitwiseandnot(const T1& x, const T2& y)
{
    return x & ~y;
}
/** @brief Identity element for `bitwiseandnot` (returns all ones). */
template <typename T>
constexpr inline T bitwiseandnot(initialvalue<T>)
{
    return special_constants<T>::allones();
}
KFR_FN(bitwiseandnot)

/**
 * @brief Bitwise OR of `x` and `y`.
 *
 * Returns the bitwise disjunction of the two operands.
 */
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> bitwiseor(const T1& x, const T2& y)
{
    return x | y;
}
/** @brief Identity element for `bitwiseor` (returns zero). */
template <typename T>
constexpr KFR_INTRINSIC T bitwiseor(initialvalue<T>)
{
    return subtype<T>(0);
}
KFR_FN(bitwiseor)

/**
 * @brief Bitwise XOR (exclusive OR) of `x` and `y`.
 *
 * Returns the bits that differ between the two operands.
 */
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> bitwisexor(const T1& x, const T2& y)
{
    return x ^ y;
}
/** @brief Identity element for `bitwisexor` (returns zero). */
template <typename T>
constexpr KFR_INTRINSIC T bitwisexor(initialvalue<T>)
{
    return subtype<T>();
}
KFR_FN(bitwisexor)

/**
 * @brief Bitwise left shift.
 *
 * Returns `left` shifted left by `right` bits.
 */
template <typename T1, typename T2>
KFR_INTRINSIC T1 shl(const T1& left, const T2& right)
{
    return left << right;
}
KFR_FN(shl)

/**
 * @brief Bitwise right shift.
 *
 * Returns `left` shifted right by `right` bits.
 */
template <typename T1, typename T2>
KFR_INTRINSIC T1 shr(const T1& left, const T2& right)
{
    return left >> right;
}
KFR_FN(shr)

/**
 * @brief Bitwise left rotate.
 *
 * Rotates the bits of `left` to the left by `right` positions, wrapping the
 * high-order bits back to the low end.
 */
template <typename T1, typename T2>
KFR_INTRINSIC T1 rol(const T1& left, const T2& right)
{
    return shl(left, right) | shr(left, (static_cast<subtype<T1>>(typebits<T1>::bits) - right));
}
KFR_FN(rol)

/**
 * @brief Bitwise right rotate.
 *
 * Rotates the bits of `left` to the right by `right` positions, wrapping the
 * low-order bits back to the high end.
 */
template <typename T1, typename T2>
KFR_INTRINSIC T1 ror(const T1& left, const T2& right)
{
    return shr(left, right) | shl(left, (static_cast<subtype<T1>>(typebits<T1>::bits) - right));
}
KFR_FN(ror)

/** @brief Identity add (single argument). */
template <typename T>
constexpr KFR_INTRINSIC T add(const T& x)
{
    return x;
}

/**
 * @brief Returns sum of all the arguments passed to a function.
 */
template <numeric T1, numeric T2, numeric... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2, Ts...> add(const T1& x, const T2& y, const Ts&... rest)
{
    return x + add(y, rest...);
}
/** @brief Identity element for `add` (returns zero). */
template <typename T>
constexpr KFR_INTRINSIC T add(initialvalue<T>)
{
    return T(0);
}
KFR_FN(add)

/** @brief Subtraction of two values. */
template <typename T1, typename T2>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2> sub(const T1& x, const T2& y)
{
    return x - y;
}
/** @brief Identity element for `sub` (returns zero). */
template <typename T>
constexpr KFR_INTRINSIC T sub(initialvalue<T>)
{
    return T(0);
}
KFR_FN(sub)

/** @brief Identity mul (single argument). */
template <typename T1>
constexpr KFR_INTRINSIC T1 mul(const T1& x)
{
    return x;
}

/**
 * @brief Returns product of all the arguments passed to a function.
 */
template <typename T1, typename T2, typename... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2, Ts...> mul(const T1& x, const T2& y, const Ts&... rest)
{
    return x * mul(y, rest...);
}

/** @brief Identity element for `mul` (returns one). */
template <typename T>
constexpr KFR_INTRINSIC T mul(initialvalue<T>)
{
    return T(1);
}
KFR_FN(mul)

/**
 * @brief Returns square of x.
 */
template <numeric T1>
constexpr inline T1 sqr(const T1& x)
{
    return x * x;
}
KFR_FN(sqr)

/**
 * @brief Returns cube of x.
 */
template <numeric T1>
constexpr inline T1 cub(const T1& x)
{
    return sqr(x) * x;
}
KFR_FN(cub)

/** @brief Returns \f$ x^2 \f$. */
template <numeric T>
constexpr KFR_INTRINSIC T pow2(const T& x)
{
    return sqr(x);
}

/** @brief Returns \f$ x^3 \f$. */
template <numeric T>
constexpr KFR_INTRINSIC T pow3(const T& x)
{
    return cub(x);
}

/** @brief Returns \f$ x^4 \f$. */
template <numeric T>
constexpr KFR_INTRINSIC T pow4(const T& x)
{
    return sqr(sqr(x));
}

/** @brief Returns \f$ x^5 \f$. */
template <numeric T>
constexpr KFR_INTRINSIC T pow5(const T& x)
{
    return pow4(x) * x;
}
KFR_FN(pow2)
KFR_FN(pow3)
KFR_FN(pow4)
KFR_FN(pow5)

/**
 * @brief Integer power.
 *
 * Returns \f$ x^{base} \f$ computed by repeated squaring.
 * @code
 * CHECK( ipow( 10, 3 ) == 1000 );
 * CHECK( ipow( 0.5, 2 ) == 0.25 );
 * @endcode
 */
template <typename T>
constexpr inline T ipow(const T& x, int base)
{
    T xx     = x;
    T result = T(1);
    while (base)
    {
        if (base & 1)
            result *= xx;
        base >>= 1;
        xx *= xx;
    }
    return result;
}
KFR_FN(ipow)

/**
 * @brief Square of the sum of all arguments.
 *
 * Returns \f$ (x_0 + x_1 + \dots + x_n)^2 \f$.
 * @code
 * CHECK(sqrsum(1,2,3) == 36);
 * @endcode
 */
template <typename T1, typename... Ts>
constexpr inline std::common_type_t<T1, Ts...> sqrsum(const T1& x, const Ts&... rest)
{
    return sqr(add(x, rest...));
}

/** @brief Returns square of the difference of two arguments. */
template <typename T1, typename T2>
constexpr inline std::common_type_t<T1, T2> sqrdiff(const T1& x, const T2& y)
{
    return sqr(x - y);
}
KFR_FN(sqrsum)
KFR_FN(sqrdiff)

/**
 * @brief Division of `x` by `y`.
 *
 * Returns the quotient of the two operands, cast to their common type.
 */
template <typename T1, typename T2, typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout div(const T1& x, const T2& y)
{
    return static_cast<Tout>(x) / static_cast<Tout>(y);
}
KFR_FN(div)

/**
 * @brief Modulo of `x` by `y`.
 *
 * Returns the remainder of the integer division of `x` by `y`, cast to their
 * common type.
 */
template <typename T1, typename T2, typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout mod(const T1& x, const T2& y)
{
    return static_cast<Tout>(x) % static_cast<Tout>(y);
}
KFR_FN(mod)
/**
 * @brief Remainder of `x` divided by `y`.
 *
 * Equivalent to `mod`; provided for naming symmetry with `std::remainder`.
 */
template <typename T1, typename T2, typename Tout = std::common_type_t<T1, T2>>
KFR_INTRINSIC Tout rem(const T1& x, const T2& y)
{
    return static_cast<Tout>(x) % static_cast<Tout>(y);
}
KFR_FN(rem)

/**
 * @brief Arithmetic negation of `x`.
 *
 * Returns the additive inverse of `x`.
 */
template <typename T1>
inline T1 neg(const T1& x)
{
    return -x;
}
KFR_FN(neg)

/**
 * @brief Fused multiply-add.
 *
 * Returns `x * y + z`.
 */
template <typename T1, typename T2, typename T3>
KFR_INTRINSIC constexpr std::common_type_t<T1, T2, T3> fmadd(const T1& x, const T2& y, const T3& z)
{
    return x * y + z;
}
/**
 * @brief Fused multiply-subtract.
 *
 * Returns `x * y - z`.
 */
template <typename T1, typename T2, typename T3>
KFR_INTRINSIC constexpr std::common_type_t<T1, T2, T3> fmsub(const T1& x, const T2& y, const T3& z)
{
    return x * y - z;
}
KFR_FN(fmadd)
KFR_FN(fmsub)

/**
 * @brief Linear blend of `x` and `y`.
 *
 * Returns `x + (y - x) * c`. The blend factor `c` must be in the range
 * \f$ [0, 1] \f$, where `0` yields `x` and `1` yields `y`.
 */
template <numeric T1, numeric T2, numeric T3>
KFR_INTRINSIC constexpr std::common_type_t<T1, T2, T3> mix(const T1& c, const T2& x, const T3& y)
{
    return fmadd(c, y - x, x);
}

/**
 * @brief Signed linear blend of `x` and `y`.
 *
 * As `mix` but accepts a blend factor `c` in the range \f$ [-1, 1] \f$, mapping
 * it to \f$ [0, 1] \f$ internally.
 */
template <numeric T1, numeric T2, numeric T3>
KFR_INTRINSIC constexpr std::common_type_t<T1, T2, T3> mixs(const T1& c, const T2& x, const T3& y)
{
    return mix(fmadd(c, 0.5, 0.5), x, y);
}
KFR_FN(mix)
KFR_FN(mixs)

namespace intr
{

template <typename T1, typename T2>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2> horner(const T1&, const T2& c0)
{
    return c0;
}

template <typename T1, typename T2, typename T3, typename... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2, T3, Ts...> horner(const T1& x, const T2& c0, const T3& c1,
                                                                     const Ts&... values)
{
    return fmadd(horner(x, c1, values...), x, c0);
}

template <typename T1, typename T2>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2> horner_even(const T1&, const T2& c0)
{
    return c0;
}

template <typename T1, typename T2, typename T3, typename... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2, T3, Ts...> horner_even(const T1& x, const T2& c0,
                                                                          const T3& c2, const Ts&... values)
{
    const T1 x2 = x * x;
    return fmadd(horner(x2, c2, values...), x2, c0);
}

template <typename T1, typename T2>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2> horner_odd(const T1& x, const T2& c1)
{
    return c1 * x;
}

template <typename T1, typename T2, typename T3, typename... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, T2, T3, Ts...> horner_odd(const T1& x, const T2& c1,
                                                                         const T3& c3, const Ts&... values)
{
    const T1 x2 = x * x;
    return fmadd(horner(x2, c3, values...), x2, c1) * x;
}
} // namespace intr

/// @brief Calculate polynomial using Horner's method
///
/// ``horner(x, 1, 2, 3)`` is equivalent to \(3x^2 + 2x + 1\)
template <numeric T1, numeric... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, Ts...> horner(const T1& x, const Ts&... c)
{
    return intr::horner(x, c...);
}
KFR_FN(horner)

/// @brief Calculate polynomial using Horner's method (even powers)
///
/// ``horner_even(x, 1, 2, 3)`` is equivalent to \(3x^4 + 2x^2 + 1\)
template <numeric T1, numeric... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, Ts...> horner_even(const T1& x, const Ts&... c)
{
    return intr::horner_even(x, c...);
}
KFR_FN(horner_even)

/// @brief Calculate polynomial using Horner's method (odd powers)
///
/// ``horner_odd(x, 1, 2, 3)`` is equivalent to \(3x^5 + 2x^3 + 1x\)
template <numeric T1, numeric... Ts>
constexpr KFR_INTRINSIC std::common_type_t<T1, Ts...> horner_odd(const T1& x, const Ts&... c)
{
    return intr::horner_odd(x, c...);
}
KFR_FN(horner_odd)

/**
 * @brief Multiplicative inverse of `x`.
 *
 * Returns \f$ 1/x \f$. `T` must be a floating-point type.
 */
template <typename T>
constexpr KFR_INTRINSIC T reciprocal(const T& x)
{
    static_assert(std::is_floating_point_v<subtype<T>>, "T must be floating point type");
    return subtype<T>(1) / x;
}
KFR_FN(reciprocal)

/** @brief Multiply `x` by the sign of `y` without a conditional branch. */
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> mulsign(const T1& x, const T2& y)
{
    return bitwisexor(x, bitwiseand(y, special_constants<T2>::highbitmask()));
}
KFR_FN(mulsign)

/** @brief Compose `x` with the magnitude of `x` and the sign of `y`. */
template <typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> copysign(const vec<T, N>& x, const vec<T, N>& y)
{
    return (x & special_constants<T>::highbitmask()) | (y & special_constants<T>::highbitmask());
}

/**
 * @brief Swap byte order of `x`.
 *
 * Reverses the bytes of `x` to convert between big-endian and little-endian
 * representations.
 */
template <typename T>
KFR_INTRINSIC T swapbyteorder(const T& x)
{
    return bitcast_anything<T>(swap<sizeof(deep_subtype<T>)>(bitcast_anything<vec<u8, sizeof(T)>>(x)));
}
KFR_FN(swapbyteorder)

/** @brief Returns a vector with `a-b` in the low half and `a+b` in the high half. */
template <typename T, size_t N>
    requires(N >= 2)
KFR_INTRINSIC vec<T, N> subadd(const vec<T, N>& a, const vec<T, N>& b)
{
    return blend<1, 0>(a + b, a - b);
}
/** @brief Returns a vector with `a+b` in the low half and `a-b` in the high half. */
template <typename T, size_t N>
    requires(N >= 2)
KFR_INTRINSIC vec<T, N> addsub(const vec<T, N>& a, const vec<T, N>& b)
{
    return blend<0, 1>(a + b, a - b);
}
KFR_FN(subadd)
KFR_FN(addsub)

/** @brief Negates the even-indexed lanes of `x`. */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> negeven(const vec<T, N>& x)
{
    return x ^ broadcast<N>(-T(), T());
}
/** @brief Negates the odd-indexed lanes of `x`. */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> negodd(const vec<T, N>& x)
{
    return x ^ broadcast<N>(T(), -T());
}

/**
 * @brief Transposes and packs `sizeof...(Ns)+1` vectors of length `N1` into a
 *        vector of vectors of length `N1`.
 */
template <typename T, size_t N1, size_t... Ns>
vec<vec<T, sizeof...(Ns) + 1>, N1> packtranspose(const vec<T, N1>& x, const vec<T, Ns>&... rest)
{
    const vec<T, N1 * (sizeof...(Ns) + 1)> t = transpose<N1>(concat(x, rest...));
    return t.v;
}

KFR_FN(packtranspose)

#if 0
template <typename T, size_t N>
KFR_I_CE vec<bit<T>, N>::vec(const base& v) noexcept
{
    this->v = base::frombits((vec<itype<T>, N>::frombits(v) < itype<T>(0)).asvec()).v;
}
#endif

} // namespace KFR_ARCH_NAME
} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
