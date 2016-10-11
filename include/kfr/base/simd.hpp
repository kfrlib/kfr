/** @addtogroup types
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

#include "kfr.h"
#include "types.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{
namespace internal
{

constexpr size_t index_undefined = static_cast<size_t>(-1);

#ifdef CMT_COMPILER_CLANG
#define KFR_NATIVE_SIMD 1
#define KFR_NATIVE_INTRINSICS 1
#endif

#ifdef KFR_NATIVE_SIMD

template <typename T, size_t N>
using internal_simd_type = T __attribute__((ext_vector_type(N)));

template <typename T, size_t N>
using simd = identity<internal_simd_type<T, N>>;

template <typename T, size_t N, bool A>
using simd_storage = internal::struct_with_alignment<simd<T, N>, A>;

template <typename T, size_t N, size_t... indices>
CMT_INLINE simd<T, sizeof...(indices)> simd_shuffle(const simd<T, N>& x, const simd<T, N>& y,
                                                    csizes_t<indices...>)
{
    return __builtin_shufflevector(x, y,
                                   ((indices == index_undefined) ? -1 : static_cast<intptr_t>(indices))...);
}
template <typename T, size_t N, size_t... indices>
CMT_INLINE simd<T, sizeof...(indices)> simd_shuffle(const simd<T, N>& x, csizes_t<indices...>)
{
    return __builtin_shufflevector(x, x,
                                   ((indices == index_undefined) ? -1 : static_cast<intptr_t>(indices))...);
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE simd<T, N> simd_read(const T* src)
{
    return ptr_cast<simd_storage<T, N, A>>(src)->value;
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
CMT_INLINE simd<T, N> simd_read(const T* src)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    constexpr auto extend_indices =
        cconcat(csizeseq_t<rest>(), csizeseq_t<first - rest, index_undefined, 0>());
    constexpr auto concat_indices = cvalseq_t<size_t, N>();
    return simd_shuffle<T, first>(simd_read<first, A>(src),
                                  simd_shuffle<T, rest>(simd_read<rest, false>(src + first), extend_indices),
                                  concat_indices);
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE void simd_write(T* dest, const simd<T, N>& value)
{
    ptr_cast<simd_storage<T, N, A>>(dest)->value = value;
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
CMT_INLINE void simd_write(T* dest, const simd<T, N>& value)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    simd_write<A, first>(dest, simd_shuffle(value, csizeseq_t<first>()));
    simd_write<false, rest>(dest + first, simd_shuffle(value, csizeseq_t<rest, first>()));
}

#define KFR_SIMD_SET(T, ...) (T{ __VA_ARGS__ })
#define KFR_SIMD_CAST(T, N, X) __builtin_convertvector(X, ::kfr::simd<T, N>)
#define KFR_SIMD_BITCAST(T, N, X) ((::kfr::simd<T, N>)(X))
#define KFR_SIMD_BROADCAST(T, N, X) ((::kfr::simd<T, N>)(X))
#define KFR_SIMD_SHUFFLE(X, Y, ...) __builtin_shufflevector(X, Y, __VA_ARGS__)

#else

template <typename T>
constexpr inline T maskbits(bool value);

template <typename T>
struct simd_float_ops
{
    constexpr static T neg(T x) { return -x; }
    constexpr static T bnot(T x) { return ~x; }

    constexpr static T add(T x, T y) { return x + y; }
    constexpr static T sub(T x, T y) { return x - y; }
    constexpr static T mul(T x, T y) { return x * y; }
    constexpr static T div(T x, T y) { return x / y; }

    constexpr static T rem(T x, T y) { return std::numeric_limits<T>::quiet_NaN(); }
    constexpr static T band(T x, T y) { return std::numeric_limits<T>::quiet_NaN(); }
    constexpr static T bor(T x, T y) { return std::numeric_limits<T>::quiet_NaN(); }
    constexpr static T bxor(T x, T y) { return std::numeric_limits<T>::quiet_NaN(); }
    constexpr static T shl(T x, T y) { return std::numeric_limits<T>::quiet_NaN(); }
    constexpr static T shr(T x, T y) { return std::numeric_limits<T>::quiet_NaN(); }

    constexpr static T eq(T x, T y) { return maskbits<T>(x == y); }
    constexpr static T ne(T x, T y) { return maskbits<T>(x != y); }
    constexpr static T lt(T x, T y) { return maskbits<T>(x < y); }
    constexpr static T gt(T x, T y) { return maskbits<T>(x > y); }
    constexpr static T le(T x, T y) { return maskbits<T>(x <= y); }
    constexpr static T ge(T x, T y) { return maskbits<T>(x >= y); }
};
template <typename T>
struct simd_int_ops : simd_float_ops<T>
{
    constexpr static T rem(T x, T y) { return x % y; }
    constexpr static T band(T x, T y) { return x & y; }
    constexpr static T bor(T x, T y) { return x | y; }
    constexpr static T bxor(T x, T y) { return x ^ y; }
    constexpr static T shl(T x, T y) { return x << y; }
    constexpr static T shr(T x, T y) { return x >> y; }
};

template <typename T, size_t N>
struct alignas(const_min(size_t(64), next_poweroftwo(N * sizeof(T)))) simd
{
    using ops =
        conditional<std::is_floating_point<T>::value, internal::simd_float_ops<T>, internal::simd_int_ops<T>>;
    constexpr static simd broadcast(T value) { return broadcast_impl(value, cvalseq_t<size_t, N>()); }
    constexpr friend simd operator+(const simd& x) { return x; }
    constexpr friend simd operator-(const simd& x) { return op_impl<ops::neg>(x, cvalseq_t<size_t, N>()); }
    constexpr friend simd operator~(const simd& x) { return op_impl<ops::bnot>(x, cvalseq_t<size_t, N>()); }

    constexpr friend simd operator+(const simd& x, const simd& y)
    {
        return op_impl<ops::add>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator-(const simd& x, const simd& y)
    {
        return op_impl<ops::sub>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator*(const simd& x, const simd& y)
    {
        return op_impl<ops::mul>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator/(const simd& x, const simd& y)
    {
        return op_impl<ops::div>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator&(const simd& x, const simd& y)
    {
        return op_impl<ops::band>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator|(const simd& x, const simd& y)
    {
        return op_impl<ops::bor>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator^(const simd& x, const simd& y)
    {
        return op_impl<ops::bxor>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator<<(const simd& x, const simd& y)
    {
        return op_impl<ops::shl>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator>>(const simd& x, const simd& y)
    {
        return op_impl<ops::shr>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator==(const simd& x, const simd& y)
    {
        return op_impl<ops::eq>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator!=(const simd& x, const simd& y)
    {
        return op_impl<ops::ne>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator<(const simd& x, const simd& y)
    {
        return op_impl<ops::lt>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator>(const simd& x, const simd& y)
    {
        return op_impl<ops::gt>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator<=(const simd& x, const simd& y)
    {
        return op_impl<ops::le>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr friend simd operator>=(const simd& x, const simd& y)
    {
        return op_impl<ops::ge>(x, y, cvalseq_t<size_t, N>());
    }
    constexpr T operator[](size_t index) const { return items[index]; }
    T& operator[](size_t index) { return items[index]; }
    T items[N];

    template <typename U>
    constexpr simd<U, N> cast() const
    {
        return cast_impl<U>(*this, cvalseq_t<size_t, N>());
    }

private:
    template <typename U, size_t... indices>
    constexpr static simd<U, N> cast_impl(const simd& x, csizes_t<indices...>)
    {
        return simd<U, N>{ { static_cast<U>(x.items[indices])... } };
    }
    template <T (*fn)(T), size_t... indices>
    constexpr static simd op_impl(const simd& x, csizes_t<indices...>)
    {
        return simd{ { fn(x.items[indices])... } };
    }
    template <T (*fn)(T, T), size_t... indices>
    constexpr static simd op_impl(const simd& x, const simd& y, csizes_t<indices...>)
    {
        return simd{ { fn(x.items[indices], y.items[indices])... } };
    }
    template <size_t... indices>
    constexpr static simd broadcast_impl(T value, csizes_t<indices...>)
    {
        return simd{ { ((void)indices, value)... } };
    }
};

template <typename To, typename From, size_t N>
constexpr CMT_INLINE simd<To, N> simd_cast(const simd<From, N>& value) noexcept
{
    return value.template cast<To>();
}

template <typename T, size_t N, int... indices>
constexpr CMT_INLINE simd<T, sizeof...(indices)> simd_shuffle(const simd<T, N>& x, const simd<T, N>& y,
                                                              cints_t<indices...>) noexcept
{
    return simd<T, sizeof...(indices)>{ { (
        indices == -1 ? T() : ((indices >= N) ? y[indices - N] : x[indices]))... } };
}

template <typename To, typename From, size_t N, size_t Nout = N * sizeof(From) / sizeof(To)>
CMT_INLINE simd<To, Nout> simd_bitcast(const simd<From, N>& value) noexcept
{
    union {
        const simd<From, N> from;
        const simd<To, Nout> to;
    } u{ value };
    return u.to;
}

template <size_t N, typename T>
CMT_INLINE simd<T, N> simd_read_impl(const T* src, cfalse_t)
{
    simd<T, N> temp;
    internal::builtin_memcpy(temp.items, src, N * sizeof(T));
    return temp;
}
template <size_t N, typename T>
CMT_INLINE simd<T, N> simd_read_impl(const T* src, ctrue_t)
{
    return *ptr_cast<simd<T, N>>(src);
}

template <size_t N, typename T>
CMT_INLINE void simd_write_impl(T* dest, const simd<T, N>& value, cfalse_t)
{
    internal::builtin_memcpy(dest, value.items, N * sizeof(T));
}
template <size_t N, typename T>
CMT_INLINE void simd_write_impl(T* dest, const simd<T, N>& value, ctrue_t)
{
    *ptr_cast<simd<T, N>>(dest) = value;
}

template <size_t N, bool A = false, typename T>
CMT_INLINE simd<T, N> simd_read(const T* src)
{
    return simd_read_impl<N>(src, cbool_t<A>());
}

template <bool A = false, size_t N, typename T>
CMT_INLINE void simd_write(T* dest, const simd<T, N>& value)
{
    return simd_write_impl<N>(dest, value, cbool_t<A>());
}

#define KFR_SIMD_SET(T, ...) (T{ { __VA_ARGS__ } })
#define KFR_SIMD_CAST(T, N, X) ((void)N, ::kfr::internal::simd_cast<T>(X))
#define KFR_SIMD_BITCAST(T, N, X) ((void)N, ::kfr::internal::simd_bitcast<T>(X))
#define KFR_SIMD_BROADCAST(T, N, X) (::kfr::internal::simd<T, N>::broadcast(X))
#define KFR_SIMD_SHUFFLE(X, Y, ...) (::kfr::internal::simd_shuffle(X, Y, cints_t<__VA_ARGS__>()))

#endif
}
}

CMT_PRAGMA_MSVC(warning(pop))
