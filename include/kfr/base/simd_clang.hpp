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
#include "platform.hpp"
#include "types.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{

template <typename T, size_t... Ns>
constexpr vec<T, csum<size_t, Ns...>()> concat(const vec<T, Ns>&... vs) noexcept;

#define KFR_NATIVE_INTRINSICS 1

namespace internal
{
template <typename TT, size_t NN>
using simd_type = TT __attribute__((ext_vector_type(NN)));

template <typename T, size_t N, bool A>
using simd_storage = internal::struct_with_alignment<simd_type<T, N>, A>;

template <typename T, size_t N, size_t... indices>
CMT_INLINE simd_type<T, sizeof...(indices)> simd_shuffle(const simd_type<T, N>& x, const simd_type<T, N>& y,
                                                         csizes_t<indices...>)
{
    return __builtin_shufflevector(x, y, ((indices >= N * 2) ? -1 : static_cast<int>(indices))...);
}
template <typename T, size_t N, size_t... indices>
CMT_INLINE simd_type<T, sizeof...(indices)> simd_shuffle(const simd_type<T, N>& x, csizes_t<indices...>)
{
    return __builtin_shufflevector(x, x, ((indices >= N) ? -1 : static_cast<int>(indices))...);
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
CMT_INLINE simd_type<T, N> simd_read(const T* src)
{
    return ptr_cast<simd_storage<T, N, A>>(src)->value;
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
CMT_INLINE simd_type<T, N> simd_read(const T* src)
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
CMT_INLINE void simd_write(T* dest, const simd_type<T, N>& value)
{
    ptr_cast<simd_storage<T, N, A>>(dest)->value = value;
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
CMT_INLINE void simd_write(T* dest, const simd_type<T, N>& value)
{
    constexpr size_t first = prev_poweroftwo(N);
    constexpr size_t rest  = N - first;
    simd_write<A, first>(dest, simd_shuffle(value, csizeseq_t<first>()));
    simd_write<false, rest>(dest + first, simd_shuffle(value, csizeseq_t<rest, first>()));
}
}

template <typename T, size_t N>
struct vec : public vec_t<T, N>
{
    static_assert(is_simd_type<T>::value || !compound_type_traits<T>::is_scalar, "Invalid vector type");

    // type and size
    using value_type = T;
    constexpr static size_t size() noexcept { return N; }

    using scalar_type = T;
    constexpr static size_t scalar_size() noexcept { return N; }

    using mask_t = mask<T, N>;

    using simd_type    = internal::simd_type<T, N>;
    using uvalue_type  = utype<T>;
    using iuvalue_type = conditional<is_i_class<T>::value, T, uvalue_type>;
    using usimd_type   = internal::simd_type<uvalue_type, N>;
    using iusimd_type  = internal::simd_type<iuvalue_type, N>;

    // constructors and assignment
    // default
    constexpr vec() noexcept = default;
    // copy
    constexpr vec(const vec&) noexcept = default;
    // assignment
    constexpr vec& operator=(const vec&) noexcept = default;
    // from scalar
    template <typename U, typename = enable_if<(std::is_convertible<U, value_type>::value)>>
    constexpr vec(const U& s) noexcept : simd(s)
    {
    }
    // from list
    template <typename... Us>
    constexpr vec(const value_type& s0, const value_type& s1, const Us&... rest) noexcept
        : simd{ s0, s1, static_cast<value_type>(rest)... }
    {
    }
    // from vector of another type
    template <typename U, typename = enable_if<is_simd_type<U>::value>>
    constexpr vec(const vec<U, N>& v) noexcept : simd(__builtin_convertvector(v.simd, simd_type))
    {
    }
    constexpr vec(const simd_type& simd) noexcept : simd(simd) {}
    // from list of vectors
    template <size_t... Ns, typename = enable_if<csum<size_t, Ns...>() == N>>
    constexpr vec(const vec<T, Ns>&... vs) noexcept : simd(*concat(vs...))
    {
    }
    constexpr vec(czeros_t) noexcept : simd(0) {}
    constexpr vec(cones_t) noexcept : simd(*(vec() == vec())) {}

    template <typename U, size_t M, KFR_ENABLE_IF(sizeof(U) * M == sizeof(T) * N)>
    constexpr static vec frombits(const vec<U, M>& v) noexcept
    {
        return (simd_type)(v.flatten().simd);
    }

#define KFR_U(x) ((usimd_type)(x))
#define KFR_IU(x) ((iusimd_type)(x))
#define KFR_S(x) ((simd_type)(x))

    // math / bitwise / comparison operators
    constexpr friend vec operator+(const vec& x) noexcept { return x; }
    constexpr friend vec operator-(const vec& x) noexcept { return KFR_S(-*x); }
    constexpr friend vec operator~(const vec& x) noexcept { return KFR_S(~KFR_U(*x)); }

    constexpr friend vec operator+(const vec& x, const vec& y) noexcept { return *x + *y; }
    constexpr friend vec operator-(const vec& x, const vec& y) noexcept { return *x - *y; }
    constexpr friend vec operator*(const vec& x, const vec& y) noexcept { return *x * *y; }
    constexpr friend vec operator/(const vec& x, const vec& y) noexcept { return *x / *y; }

    constexpr friend vec operator<<(const vec& x, int shift) noexcept { return KFR_S(KFR_IU(*x) << shift); }
    constexpr friend vec operator>>(const vec& x, int shift) noexcept { return KFR_S(KFR_IU(*x) >> shift); }
    constexpr friend vec operator&(const vec& x, const vec& y) noexcept
    {
        return KFR_S(KFR_U(*x) & KFR_U(*y));
    }
    constexpr friend vec operator|(const vec& x, const vec& y) noexcept
    {
        return KFR_S(KFR_U(*x) | KFR_U(*y));
    }
    constexpr friend vec operator^(const vec& x, const vec& y) noexcept
    {
        return KFR_S(KFR_U(*x) ^ KFR_U(*y));
    }

    constexpr friend mask_t operator==(const vec& x, const vec& y) noexcept { return KFR_S(*x == *y); }
    constexpr friend mask_t operator!=(const vec& x, const vec& y) noexcept { return KFR_S(*x != *y); }
    constexpr friend mask_t operator<(const vec& x, const vec& y) noexcept { return KFR_S(*x < *y); }
    constexpr friend mask_t operator>(const vec& x, const vec& y) noexcept { return KFR_S(*x > *y); }
    constexpr friend mask_t operator<=(const vec& x, const vec& y) noexcept { return KFR_S(*x <= *y); }
    constexpr friend mask_t operator>=(const vec& x, const vec& y) noexcept { return KFR_S(*x >= *y); }

    constexpr mask_t asmask() const noexcept { return mask_t(*this); }

#undef KFR_S
#undef KFR_U

    constexpr friend vec& operator+=(vec& x, const vec& y) noexcept { return x = x + y; }
    constexpr friend vec& operator-=(vec& x, const vec& y) noexcept { return x = x - y; }
    constexpr friend vec& operator*=(vec& x, const vec& y) noexcept { return x = x * y; }
    constexpr friend vec& operator/=(vec& x, const vec& y) noexcept { return x = x / y; }

    constexpr friend vec& operator<<=(vec& x, int shift) noexcept { return x = x << shift; }
    constexpr friend vec& operator>>=(vec& x, int shift) noexcept { return x = x >> shift; }
    constexpr friend vec& operator&=(vec& x, const vec& y) noexcept { return x = x & y; }
    constexpr friend vec& operator|=(vec& x, const vec& y) noexcept { return x = x | y; }
    constexpr friend vec& operator^=(vec& x, const vec& y) noexcept { return x = x ^ y; }

    constexpr friend vec& operator++(vec& x) noexcept { return x = x + vec(1); }
    constexpr friend vec& operator--(vec& x) noexcept { return x = x - vec(1); }
    constexpr friend vec operator++(vec& x, int)noexcept
    {
        const vec z = x;
        ++x;
        return z;
    }
    constexpr friend vec operator--(vec& x, int)noexcept
    {
        const vec z = x;
        --x;
        return z;
    }

    // shuffle
    template <size_t... indices>
    constexpr vec<value_type, sizeof...(indices)> shuffle(csizes_t<indices...>) const noexcept
    {
        return __builtin_shufflevector(simd, simd, (indices >= N ? -1 : int(indices))...);
    }
    template <size_t... indices>
    constexpr vec<value_type, sizeof...(indices)> shuffle(const vec& y, csizes_t<indices...>) const noexcept
    {
        return __builtin_shufflevector(simd, y.simd, (indices >= N * 2 ? -1 : int(indices))...);
    }

    // element access
    struct element;
    constexpr value_type operator[](size_t index) const & noexcept { return get(index); }
    constexpr value_type operator[](size_t index) && noexcept { return get(index); }
    constexpr element operator[](size_t index) & noexcept { return { *this, index }; }

    constexpr value_type get(size_t index) const noexcept { return simd[index]; }
    constexpr void set(size_t index, const value_type& s) noexcept { simd[index] = s; }
    template <size_t index>
    constexpr value_type get(csize_t<index>) const noexcept
    {
        return simd[index];
    }
    template <size_t index>
    constexpr void set(csize_t<index>, const value_type& s) noexcept
    {
        simd[index] = s;
    }
    struct element
    {
        constexpr operator value_type() const noexcept { return v.get(index); }
        element& operator=(const value_type& s) noexcept
        {
            v.set(index, s);
            return *this;
        }
        element& operator=(const element& s) noexcept
        {
            v.set(index, static_cast<value_type>(s));
            return *this;
        }
        template <typename U, size_t M>
        element& operator=(const typename vec<U, M>::element& s) noexcept
        {
            v.set(index, static_cast<value_type>(static_cast<U>(s)));
            return *this;
        }
        vec& v;
        size_t index;
    };

    // read/write
    template <bool aligned = false>
    explicit constexpr vec(const value_type* src, cbool_t<aligned> = cbool_t<aligned>()) noexcept
        : simd(internal::simd_read<N, aligned>(src))
    {
    }
    template <bool aligned = false>
    const vec& write(value_type* dest, cbool_t<aligned> = cbool_t<aligned>()) const noexcept
    {
        internal::simd_write<aligned, N>(dest, simd);
        return *this;
    }

    // native SIMD type access
    const vec& flatten() const noexcept { return *this; }
    simd_type operator*() const noexcept { return simd; }
    simd_type& operator*() noexcept { return simd; }

protected:
    template <typename U, size_t M>
    friend struct vec;

    simd_type simd;

private:
};

namespace internal
{
template <typename T, size_t N>
CMT_INLINE vec<T, N> concat_impl(const vec<T, N>& x)
{
    return x;
}

template <typename T, size_t N>
CMT_INLINE vec<T, N * 2> concat_impl(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq_t<N * 2>());
}

template <typename T, size_t N1, size_t N2, KFR_ENABLE_IF(N1 > N2)>
CMT_INLINE vec<T, N1 + N2> concat_impl(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(y.shuffle(csizeseq_t<N1>()), csizeseq_t<N1 * 2>()).shuffle(csizeseq_t<N1 + N2>());
}

template <typename T, size_t N1, size_t N2, KFR_ENABLE_IF(N1 < N2)>
CMT_INLINE vec<T, N1 + N2> concat_impl(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(csizeseq_t<N2, -(N2 - N1)>())
        .shuffle(y, csizeseq_t<N2 * 2>())
        .shuffle(csizeseq_t<N1 + N2, N2 - N1>());
}

template <typename T, size_t N1, size_t N2, size_t... Sizes>
CMT_INLINE vec<T, csum<size_t, N1, N2, Sizes...>()> concat_impl(const vec<T, N1>& x, const vec<T, N2>& y,
                                                                const vec<T, Sizes>&... args)
{
    return concat_impl(concat_impl(x, y), args...);
}
}

template <typename T, size_t... Ns>
constexpr inline vec<T, csum<size_t, Ns...>()> concat(const vec<T, Ns>&... vs) noexcept
{
    return internal::concat_impl(vs...);
}
}

CMT_PRAGMA_MSVC(warning(pop))
