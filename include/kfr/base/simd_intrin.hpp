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

#include "constants.hpp"
#include "platform.hpp"
#include "types.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))
CMT_PRAGMA_MSVC(warning(disable : 4814))

#ifdef CMT_INTRINSICS_IS_CONSTEXPR
#define KFR_I_CE constexpr
#else
#define KFR_I_CE
#endif

namespace kfr
{

template <typename T, size_t... Ns>
constexpr vec<T, csum<size_t, Ns...>()> concat(const vec<T, Ns>&... vs) noexcept;

#define KFR_NATIVE_INTRINSICS 1

template <typename T, size_t N>
struct simd_type_holder
{
    using type = struct
    {
        T v[N];
    };
};

#define KFR_SIMD_SPEC_TYPE(T, N, MM)                                                                         \
    template <>                                                                                              \
    struct simd_type_holder<T, N>                                                                            \
    {                                                                                                        \
        using type = MM;                                                                                     \
    };

#ifdef CMT_ARCH_SSE2
KFR_SIMD_SPEC_TYPE(u8, 16, __m128i);
KFR_SIMD_SPEC_TYPE(u16, 8, __m128i);
KFR_SIMD_SPEC_TYPE(u32, 4, __m128i);
KFR_SIMD_SPEC_TYPE(u64, 2, __m128i);
KFR_SIMD_SPEC_TYPE(i8, 16, __m128i);
KFR_SIMD_SPEC_TYPE(i16, 8, __m128i);
KFR_SIMD_SPEC_TYPE(i32, 4, __m128i);
KFR_SIMD_SPEC_TYPE(i64, 2, __m128i);
KFR_SIMD_SPEC_TYPE(f32, 4, __m128);
KFR_SIMD_SPEC_TYPE(f64, 2, __m128d);
#endif

#ifdef CMT_ARCH_AVX
KFR_SIMD_SPEC_TYPE(u8, 32, __m256i);
KFR_SIMD_SPEC_TYPE(u16, 16, __m256i);
KFR_SIMD_SPEC_TYPE(u32, 8, __m256i);
KFR_SIMD_SPEC_TYPE(u64, 4, __m256i);
KFR_SIMD_SPEC_TYPE(i8, 32, __m256i);
KFR_SIMD_SPEC_TYPE(i16, 16, __m256i);
KFR_SIMD_SPEC_TYPE(i32, 8, __m256i);
KFR_SIMD_SPEC_TYPE(i64, 4, __m256i);
KFR_SIMD_SPEC_TYPE(f32, 8, __m256);
KFR_SIMD_SPEC_TYPE(f64, 4, __m256d);
#endif

#ifdef CMT_ARCH_NEON
KFR_SIMD_SPEC_TYPE(u8, 16, uint8x16_t);
KFR_SIMD_SPEC_TYPE(u16, 8, uint16x8_t);
KFR_SIMD_SPEC_TYPE(u32, 4, uint32x4_t);
KFR_SIMD_SPEC_TYPE(u64, 2, uint64x2_t);
KFR_SIMD_SPEC_TYPE(i8, 16, int8x16_t);
KFR_SIMD_SPEC_TYPE(i16, 8, int16x8_t);
KFR_SIMD_SPEC_TYPE(i32, 4, int32x4_t);
KFR_SIMD_SPEC_TYPE(i64, 2, int64x2_t);
KFR_SIMD_SPEC_TYPE(f32, 4, float32x4_t);
#ifdef CMT_ARCH_NEON64
KFR_SIMD_SPEC_TYPE(f64, 2, float64x2_t);
#endif
#endif

template <size_t N>
struct raw_bytes
{
    u8 raw[N];
};

#define KFR_CYCLE(...)                                                                                       \
    for (size_t i = 0; i < N; i++)                                                                           \
    __VA_ARGS__

#define KFR_C_CYCLE(...)                                                                                     \
    for (size_t i = 0; i < N; i++)                                                                           \
    vs[i]         = __VA_ARGS__

#define KFR_R_CYCLE(...)                                                                                     \
    vec<T, N> result;                                                                                        \
    for (size_t i    = 0; i < N; i++)                                                                        \
        result.vs[i] = __VA_ARGS__;                                                                          \
    return result

#define KFR_B_CYCLE(...)                                                                                     \
    vec<T, N> result;                                                                                        \
    for (size_t i    = 0; i < N; i++)                                                                        \
        result.vs[i] = (__VA_ARGS__) ? constants<value_type>::allones() : value_type(0);                     \
    return result

template <typename T, size_t N>
struct alignas(const_min(platform<>::maximum_vector_alignment, sizeof(T) * next_poweroftwo(N))) vec
    : vec_t<T, N>
{
    constexpr static size_t simd_width = platform<T>::vector_width;
    constexpr static size_t count      = (N + simd_width - 1) / simd_width;

    static_assert(is_simd_type<T>::value || !compound_type_traits<T>::is_scalar, "Invalid vector type");

    // type and size
    using value_type = T;
    constexpr static size_t size() noexcept { return N; }

    using scalar_type = T;
    constexpr static size_t scalar_size() noexcept { return N; }

    using simd_type = typename simd_type_holder<T, N>::type;

    using uvalue_type  = utype<T>;
    using iuvalue_type = conditional<is_i_class<T>::value, T, uvalue_type>;

    using mask_t = mask<T, N>;

    using uvec  = vec<uvalue_type, N>;
    using iuvec = vec<iuvalue_type, N>;

    // constructors and assignment
    // default
    constexpr vec() noexcept = default;
    // copy
    vec(const vec&) noexcept = default;
    // assignment
    CMT_GNU_CONSTEXPR vec& operator=(const vec&) CMT_GNU_NOEXCEPT = default;

    template <size_t... indices>
    KFR_I_CE vec<value_type, sizeof...(indices)> shuffle(csizes_t<indices...>) const noexcept
    {
        return vec<value_type, sizeof...(indices)>((indices < N ? vs[indices % N] : 0)...);
    }
    template <size_t... indices>
    KFR_I_CE vec<value_type, sizeof...(indices)> shuffle(const vec& y, csizes_t<indices...>) const noexcept
    {
        return vec<value_type, sizeof...(indices)>(
            (indices < N ? vs[indices % N] : indices < 2 * N ? y.vs[(indices - N) % N] : 0)...);
    }

    template <typename U, typename = enable_if<(std::is_convertible<U, value_type>::value)>>
    KFR_I_CE vec(const U& s) noexcept
    {
        KFR_C_CYCLE(s);
    }

    constexpr vec(const simd_type& simd) noexcept : simd(simd) {}
    // from vector of another type
    template <typename U, typename = enable_if<is_simd_type<U>::value>>
    KFR_I_CE vec(const vec<U, N>& v) noexcept
    {
        KFR_C_CYCLE(static_cast<value_type>(v.vs[i]));
    }
    // from list
    template <typename... Us>
    KFR_I_CE vec(const value_type& s0, const value_type& s1, const Us&... rest) noexcept
        : vs{ s0, s1, static_cast<value_type>(rest)... }
    {
    }
    template <size_t N1, size_t... Ns, typename = enable_if<(csum<size_t, N1, Ns...>() == N)>>
    KFR_I_CE vec(const vec<T, N1>& v0, const vec<T, Ns>&... vecs) noexcept : simd(*concat(v0, vecs...))
    {
    }

    KFR_I_CE vec(czeros_t) noexcept { KFR_C_CYCLE(value_type(0)); }
    KFR_I_CE vec(cones_t) noexcept { KFR_C_CYCLE(constants<value_type>::allones()); }

    template <typename U, size_t M, KFR_ENABLE_IF(sizeof(U) * M == sizeof(T) * N)>
    KFR_I_CE static vec frombits(const vec<U, M>& v) noexcept
    {
        vec r;
        r.bytes = v.flatten().bytes;
        return r;
    }

    KFR_I_CE vec operator+() const noexcept { return *this; }
    KFR_I_CE vec operator-() const noexcept { KFR_R_CYCLE(-this->vs[i]); }
    KFR_I_CE vec operator~() const noexcept
    {
        uvec xx = uvec::frombits(*this);
        KFR_CYCLE(xx.vs[i] = ~xx.vs[i]);
        return frombits(xx);
    }

    KFR_I_CE vec operator+(const vec& y) const noexcept { KFR_R_CYCLE(this->vs[i] + y.vs[i]); }
    KFR_I_CE vec operator-(const vec& y) const noexcept { KFR_R_CYCLE(this->vs[i] - y.vs[i]); }
    KFR_I_CE vec operator*(const vec& y) const noexcept { KFR_R_CYCLE(this->vs[i] * y.vs[i]); }
    KFR_I_CE vec operator/(const vec& y) const noexcept { KFR_R_CYCLE(this->vs[i] / y.vs[i]); }

    KFR_I_CE vec operator<<(int shift) const noexcept
    {
        iuvec xx = iuvec::frombits(*this);
        KFR_CYCLE(xx.vs[i] <<= shift);
        return frombits(xx);
    }
    KFR_I_CE vec operator>>(int shift) const noexcept
    {
        iuvec xx = iuvec::frombits(*this);
        KFR_CYCLE(xx.vs[i] >>= shift);
        return frombits(xx);
    }
    KFR_I_CE vec operator&(const vec& y) const noexcept
    {
        uvec xx = uvec::frombits(*this);
        uvec yy = uvec::frombits(y);
        KFR_CYCLE(xx.vs[i] &= yy.vs[i]);
        return frombits(xx);
    }
    KFR_I_CE vec operator|(const vec& y) const noexcept
    {
        uvec xx = uvec::frombits(*this);
        uvec yy = uvec::frombits(y);
        KFR_CYCLE(xx.vs[i] |= yy.vs[i]);
        return frombits(xx);
    }
    KFR_I_CE vec operator^(const vec& y) const noexcept
    {
        uvec xx = uvec::frombits(*this);
        uvec yy = uvec::frombits(y);
        KFR_CYCLE(xx.vs[i] ^= yy.vs[i]);
        return frombits(xx);
    }

    KFR_I_CE mask_t operator==(const vec& y) const noexcept { KFR_B_CYCLE(this->vs[i] == y.vs[i]); }
    KFR_I_CE mask_t operator!=(const vec& y) const noexcept { KFR_B_CYCLE(this->vs[i] != y.vs[i]); }
    KFR_I_CE mask_t operator<(const vec& y) const noexcept { KFR_B_CYCLE(this->vs[i] < y.vs[i]); }
    KFR_I_CE mask_t operator>(const vec& y) const noexcept { KFR_B_CYCLE(this->vs[i] > y.vs[i]); }
    KFR_I_CE mask_t operator<=(const vec& y) const noexcept { KFR_B_CYCLE(this->vs[i] <= y.vs[i]); }
    KFR_I_CE mask_t operator>=(const vec& y) const noexcept { KFR_B_CYCLE(this->vs[i] >= y.vs[i]); }

    constexpr mask_t asmask() const noexcept { return mask_t(*this); }

    KFR_I_CE vec& operator+=(const vec& y) noexcept { return *this = *this + y; }
    KFR_I_CE vec& operator-=(const vec& y) noexcept { return *this = *this - y; }
    KFR_I_CE vec& operator*=(const vec& y) noexcept { return *this = *this * y; }
    KFR_I_CE vec& operator/=(const vec& y) noexcept { return *this = *this / y; }
    KFR_I_CE vec& operator<<=(int shift) noexcept { return *this = *this << shift; }
    KFR_I_CE vec& operator>>=(int shift) noexcept { return *this = *this >> shift; }
    KFR_I_CE vec& operator&=(const vec& y) noexcept { return *this = *this & y; }
    KFR_I_CE vec& operator|=(const vec& y) noexcept { return *this = *this | y; }
    KFR_I_CE vec& operator^=(const vec& y) noexcept { return *this = *this ^ y; }

    KFR_I_CE vec& operator++() noexcept { return *this = *this + vec(1); }
    KFR_I_CE vec& operator--() noexcept { return *this = *this - vec(1); }
    KFR_I_CE vec operator++(int)noexcept
    {
        const vec z = *this;
        ++*this;
        return z;
    }
    KFR_I_CE vec operator--(int)noexcept
    {
        const vec z = *this;
        --*this;
        return z;
    }

    explicit KFR_I_CE vec(const value_type* src) { KFR_C_CYCLE(src[i]); }
    explicit KFR_I_CE vec(const value_type* src, cunaligned_t) { KFR_C_CYCLE(src[i]); }
    explicit KFR_I_CE vec(const value_type* src, caligned_t) { KFR_C_CYCLE(src[i]); }

    const vec& write(value_type* dest) const
    {
        KFR_CYCLE(dest[i] = vs[i]);
        return *this;
    }
    const vec& write(value_type* dest, cunaligned_t) const
    {
        KFR_CYCLE(dest[i] = vs[i]);
        return *this;
    }
    const vec& write(value_type* dest, caligned_t) const
    {
        KFR_CYCLE(dest[i] = vs[i]);
        return *this;
    }

    KFR_I_CE value_type operator[](size_t index) const noexcept { return vs[index]; }
    KFR_I_CE value_type& operator[](size_t index) noexcept { return vs[index]; }

    const vec& flatten() const noexcept { return *this; }
    simd_type operator*() const noexcept { return simd; }
    simd_type& operator*() noexcept { return simd; }
protected:
    template <typename, size_t>
    friend struct vec;

    union {
        T vs[N];
        simd_type simd;
        raw_bytes<N * sizeof(T)> bytes;
    };
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
