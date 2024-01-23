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

#ifndef CMT_CLANG_EXT

#include "simd.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wuninitialized")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wunknown-warning-option")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wmaybe-uninitialized")

namespace kfr
{
template <size_t bits, size_t...>
struct shuffle_mask;

template <size_t i0, size_t i1, size_t i2, size_t i3, size_t i4, size_t i5, size_t i6, size_t i7>
struct shuffle_mask<8, i0, i1, i2, i3, i4, i5, i6, i7>
{
    constexpr static inline size_t Nmax  = 1;
    constexpr static inline size_t value = (const_min(i7, Nmax) << 7) | (const_min(i6, Nmax) << 6) |
                                           (const_min(i5, Nmax) << 5) | (const_min(i4, Nmax) << 4) |
                                           (const_min(i3, Nmax) << 3) | (const_min(i2, Nmax) << 2) |
                                           (const_min(i1, Nmax) << 1) | const_min(i0, Nmax);
};

template <size_t i0, size_t i1, size_t i2, size_t i3>
struct shuffle_mask<8, i0, i1, i2, i3>
{
    constexpr static inline size_t Nmax  = 3;
    constexpr static inline size_t value = (const_min(i3, Nmax) << 6) | (const_min(i2, Nmax) << 4) |
                                           (const_min(i1, Nmax) << 2) | const_min(i0, Nmax);
};

template <size_t i0, size_t i1, size_t i2, size_t i3>
struct shuffle_mask<4, i0, i1, i2, i3>
{
    constexpr static inline size_t Nmax  = 1;
    constexpr static inline size_t value = (const_min(i3, Nmax) << 3) | (const_min(i2, Nmax) << 2) |
                                           (const_min(i1, Nmax) << 1) | const_min(i0, Nmax);
};

template <size_t i0, size_t i1>
struct shuffle_mask<2, i0, i1>
{
    constexpr static inline size_t Nmax  = 1;
    constexpr static inline size_t value = (const_min(i1, Nmax) << 1) | const_min(i0, Nmax);
};

#if KFR_SHOW_NOT_OPTIMIZED
CMT_PUBLIC_C CMT_DLL_EXPORT void not_optimized(const char* fn) CMT_NOEXCEPT;
#else
#define not_optimized(...) CMT_NOOP
#endif

inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T, size_t N>
using simd = typename simd_type<unwrap_bit<T>, next_poweroftwo(static_cast<size_t>(N))>::type;

template <typename T, size_t N, typename U>
struct simd_small_array
{
    static_assert(is_poweroftwo(N), "");
    static_assert(sizeof(T) * N == sizeof(U), "");
    U whole;

    using value_type             = T;
    constexpr static size_t size = N;
    using packed_type            = U;

#ifdef CMT_COMPILER_IS_MSVC
    KFR_INTRINSIC constexpr simd_small_array() CMT_NOEXCEPT = default;
#else
    KFR_INTRINSIC simd_small_array() CMT_NOEXCEPT {}
#endif

    KFR_INTRINSIC constexpr simd_small_array(U whole) CMT_NOEXCEPT : whole(whole) {}

    template <typename... Args>
    KFR_INTRINSIC constexpr simd_small_array(T a, T b, Args... args) CMT_NOEXCEPT
        : whole(pack_elements<U, T>(a, b, args...))
    {
    }

    KFR_INTRINSIC static constexpr simd_small_array from(U whole) CMT_NOEXCEPT { return { whole }; }
};

template <>
struct simd_small_array<f32, 2, f64>
{
    f64 whole;

    using value_type             = f32;
    constexpr static size_t size = 2;
    using packed_type            = f64;

#ifdef CMT_COMPILER_IS_MSVC
    KFR_MEM_INTRINSIC constexpr simd_small_array() CMT_NOEXCEPT = default;
#else
    KFR_MEM_INTRINSIC simd_small_array() CMT_NOEXCEPT {}
#endif

#ifdef CMT_COMPILER_IS_MSVC
    // MSVC Internal Compiler Error workaround
    KFR_MEM_INTRINSIC constexpr simd_small_array(const simd_small_array& v) CMT_NOEXCEPT : whole(v.whole) {}
    KFR_MEM_INTRINSIC constexpr simd_small_array(simd_small_array&& v) CMT_NOEXCEPT : whole(v.whole) {}
    KFR_MEM_INTRINSIC constexpr simd_small_array& operator=(const simd_small_array& v) CMT_NOEXCEPT
    {
        whole = v.whole;
        return *this;
    }
    KFR_MEM_INTRINSIC constexpr simd_small_array& operator=(simd_small_array&& v) CMT_NOEXCEPT
    {
        whole = v.whole;
        return *this;
    }
#endif

    KFR_MEM_INTRINSIC constexpr simd_small_array(f64 whole) CMT_NOEXCEPT : whole(whole) {}

    KFR_MEM_INTRINSIC simd_small_array(f32 x, f32 y) CMT_NOEXCEPT
    {
#ifdef CMT_COMPILER_IS_MSVC
#ifdef CMT_ARCH_SSE2
        // whole = _mm_cvtsd_f64(_mm_castps_pd(_mm_setr_ps(x, y, x, y)));
        whole = _mm_cvtsd_f64(_mm_castps_pd(_mm_unpacklo_ps(_mm_set_ss(x), _mm_set_ss(y))));
#else
        union
        {
            struct
            {
                f32 x;
                f32 y;
            };
            f64 r;
        } u;
        u.x   = x;
        u.y   = y;
        whole = u.r;
#endif
#else
        union
        {
            struct
            {
                f32 x;
                f32 y;
            };
            f64 r;
        } u{ { x, y } };
        whole = u.r;
#endif
    }

    KFR_MEM_INTRINSIC static constexpr simd_small_array from(f64 whole) CMT_NOEXCEPT { return { whole }; }
};

template <typename T>
constexpr inline bool is_simd_small_array = false;

template <typename T, size_t N, typename U>
constexpr inline bool is_simd_small_array<simd_small_array<T, N, U>> = true;

#define KFR_SIMD_TYPE(T, N, ...)                                                                             \
    template <>                                                                                              \
    struct simd_type<T, N>                                                                                   \
    {                                                                                                        \
        using type = __VA_ARGS__;                                                                            \
    };

#define KFR_SIMD_SMALL_TYPE(T, N, U)                                                                         \
    template <>                                                                                              \
    struct simd_type<T, N>                                                                                   \
    {                                                                                                        \
        using type = simd_small_array<T, N, U>;                                                              \
    };

template <typename T>
struct simd_type<T, 1>
{
    using type = T;
};

template <typename T, size_t N>
struct simd_type
{
    using type = simd_halves<T, N>;
};

KFR_SIMD_SMALL_TYPE(u8, 2, u16)
KFR_SIMD_SMALL_TYPE(i8, 2, u16)

KFR_SIMD_SMALL_TYPE(u8, 4, u32)
KFR_SIMD_SMALL_TYPE(u16, 2, u32)
KFR_SIMD_SMALL_TYPE(i8, 4, u32)
KFR_SIMD_SMALL_TYPE(i16, 2, u32)

KFR_SIMD_SMALL_TYPE(u8, 8, u64)
KFR_SIMD_SMALL_TYPE(u16, 4, u64)
KFR_SIMD_SMALL_TYPE(u32, 2, u64)
KFR_SIMD_SMALL_TYPE(i8, 8, u64)
KFR_SIMD_SMALL_TYPE(i16, 4, u64)
KFR_SIMD_SMALL_TYPE(i32, 2, u64)

#ifdef CMT_ARCH_SSE
#ifndef KFR_f32x2_array
KFR_SIMD_SMALL_TYPE(f32, 2, f64)
#endif

KFR_SIMD_TYPE(f32, 4, __m128)
KFR_SIMD_TYPE(f64, 2, __m128d)
#endif // CMT_ARCH_SSE

#ifdef CMT_ARCH_SSE2
KFR_SIMD_TYPE(u8, 16, __m128i)
KFR_SIMD_TYPE(u16, 8, __m128i)
KFR_SIMD_TYPE(u32, 4, __m128i)
KFR_SIMD_TYPE(u64, 2, __m128i)
KFR_SIMD_TYPE(i8, 16, __m128i)
KFR_SIMD_TYPE(i16, 8, __m128i)
KFR_SIMD_TYPE(i32, 4, __m128i)
KFR_SIMD_TYPE(i64, 2, __m128i)
#endif // CMT_ARCH_SSE2

#ifdef CMT_ARCH_AVX
KFR_SIMD_TYPE(float, 8, __m256)
KFR_SIMD_TYPE(double, 4, __m256d)
KFR_SIMD_TYPE(u8, 32, __m256i)
KFR_SIMD_TYPE(u16, 16, __m256i)
KFR_SIMD_TYPE(u32, 8, __m256i)
KFR_SIMD_TYPE(u64, 4, __m256i)
KFR_SIMD_TYPE(i8, 32, __m256i)
KFR_SIMD_TYPE(i16, 16, __m256i)
KFR_SIMD_TYPE(i32, 8, __m256i)
KFR_SIMD_TYPE(i64, 4, __m256i)
#endif // CMT_ARCH_AVX

#ifdef CMT_ARCH_AVX512
KFR_SIMD_TYPE(float, 16, __m512)
KFR_SIMD_TYPE(double, 8, __m512d)
KFR_SIMD_TYPE(u8, 64, __m512i)
KFR_SIMD_TYPE(u16, 32, __m512i)
KFR_SIMD_TYPE(u32, 16, __m512i)
KFR_SIMD_TYPE(u64, 8, __m512i)
KFR_SIMD_TYPE(i8, 64, __m512i)
KFR_SIMD_TYPE(i16, 32, __m512i)
KFR_SIMD_TYPE(i32, 16, __m512i)
KFR_SIMD_TYPE(i64, 8, __m512i)
#endif // CMT_ARCH_AVX512

#ifdef CMT_ARCH_NEON
KFR_SIMD_TYPE(u8, 16, uint8x16_t);
KFR_SIMD_TYPE(u16, 8, uint16x8_t);
KFR_SIMD_TYPE(u32, 4, uint32x4_t);
KFR_SIMD_TYPE(u64, 2, uint64x2_t);
KFR_SIMD_TYPE(i8, 16, int8x16_t);
KFR_SIMD_TYPE(i16, 8, int16x8_t);
KFR_SIMD_TYPE(i32, 4, int32x4_t);
KFR_SIMD_TYPE(i64, 2, int64x2_t);
KFR_SIMD_TYPE(f32, 4, float32x4_t);
#ifdef CMT_ARCH_NEON64
KFR_SIMD_TYPE(f64, 2, float64x2_t);
#endif // CMT_ARCH_NEON64
#endif // CMT_ARCH_NEON

#if defined CMT_COMPILER_IS_MSVC
#define KFR_i8sse_INDEX(x, i) x.m128i_i8[i]
#define KFR_i16sse_INDEX(x, i) x.m128i_i16[i]
#define KFR_i32sse_INDEX(x, i) x.m128i_i32[i]
#define KFR_i64sse_INDEX(x, i) x.m128i_i64[i]
#define KFR_u8sse_INDEX(x, i) x.m128i_u8[i]
#define KFR_u16sse_INDEX(x, i) x.m128i_u16[i]
#define KFR_u32sse_INDEX(x, i) x.m128i_u32[i]
#define KFR_u64sse_INDEX(x, i) x.m128i_u64[i]
#define KFR_f32sse_INDEX(x, i) x.m128_f32[i]
#define KFR_f64sse_INDEX(x, i) x.m128d_f64[i]
#else
#define KFR_i8sse_INDEX(x, i) bitcast_anything<simd_array<i8, 16>>(x).val[i]
#define KFR_i16sse_INDEX(x, i) bitcast_anything<simd_array<i16, 8>>(x).val[i]
#define KFR_i32sse_INDEX(x, i) _mm_cvtsi128_si32(_mm_shuffle_epi32(x, _MM_SHUFFLE(3, 2, 1, i)))
#define KFR_i64sse_INDEX(x, i) _mm_cvtsi128_si64(_mm_shuffle_epi32(x, _MM_SHUFFLE(3, 2, (i) * 2 + 1, i * 2)))
#define KFR_u8sse_INDEX(x, i) bitcast_anything<simd_array<u8, 16>>(x).val[i]
#define KFR_u16sse_INDEX(x, i) bitcast_anything<simd_array<u16, 8>>(x).val[i]
#define KFR_u32sse_INDEX(x, i) _mm_cvtsi128_si32(_mm_shuffle_epi32(x, _MM_SHUFFLE(3, 2, 1, i)))
#define KFR_u64sse_INDEX(x, i) _mm_cvtsi128_si64(_mm_shuffle_epi32(x, _MM_SHUFFLE(3, 2, (i) * 2 + 1, i * 2)))
#define KFR_f32sse_INDEX(x, i) _mm_cvtss_f32(_mm_shuffle_ps(x, x, _MM_SHUFFLE(3, 2, 1, i)))
#define KFR_f64sse_INDEX(x, i) _mm_cvtsd_f64(_mm_shuffle_pd(x, x, _MM_SHUFFLE2(1, i)))
#endif

// specializations

template <typename T, size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> universal_shuffle(simd_t<T, N>, const simd<T, N>& x, csizes_t<indices...>);

#ifdef KFR_NATIVE_INTRINSICS

#define KFR_GEN_ty(n, ty) ty(n)
#define KFR_GEN_arg_def(n, ty) ty arg##n
#define KFR_GEN_arg(n, ty) arg##n

#define KFR_INTRIN_MAKE(n, ty, intrin)                                                                       \
    KFR_INTRINSIC simd<ty, n> simd_make(ctype_t<ty>, CMT_GEN_LIST(n, KFR_GEN_arg_def, ty)) CMT_NOEXCEPT      \
    {                                                                                                        \
        return intrin(CMT_GEN_LIST(n, KFR_GEN_arg, ty));                                                     \
    }

#ifdef CMT_ARCH_SSE2

KFR_INTRINSIC double take_hi_sd(__m128d x) { return _mm_cvtsd_f64(_mm_unpackhi_pd(x, x)); }

KFR_INTRINSIC __m128i KFR_mm_setr_epi64x(int64_t q0, int64_t q1) CMT_NOEXCEPT
{
    return _mm_set_epi64x(q1, q0);
}
KFR_INTRINSIC __m128i KFR_mm_setr_epi32(int32_t q0, int32_t q1, int32_t q2, int32_t q3) CMT_NOEXCEPT
{
    return _mm_set_epi32(q3, q2, q1, q0);
}
KFR_INTRIN_MAKE(2, i64, KFR_mm_setr_epi64x)
KFR_INTRIN_MAKE(2, u64, KFR_mm_setr_epi64x)
KFR_INTRIN_MAKE(2, f64, _mm_setr_pd)
KFR_INTRIN_MAKE(4, i32, KFR_mm_setr_epi32)
KFR_INTRIN_MAKE(4, u32, KFR_mm_setr_epi32)
KFR_INTRIN_MAKE(4, f32, _mm_setr_ps)
KFR_INTRIN_MAKE(8, i16, _mm_setr_epi16)
KFR_INTRIN_MAKE(8, u16, _mm_setr_epi16)
KFR_INTRIN_MAKE(16, i8, _mm_setr_epi8)
KFR_INTRIN_MAKE(16, u8, _mm_setr_epi8)

#define KFR_INTRIN_BITCAST(Tout, Tin, N, ...)                                                                \
    KFR_INTRINSIC simd<Tout, N> simd_bitcast(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x) CMT_NOEXCEPT   \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }
KFR_INTRIN_BITCAST(f32, i32, 4, _mm_castsi128_ps(x))
KFR_INTRIN_BITCAST(i32, f32, 4, _mm_castps_si128(x))
KFR_INTRIN_BITCAST(f64, i64, 2, _mm_castsi128_pd(x))
KFR_INTRIN_BITCAST(i64, f64, 2, _mm_castpd_si128(x))

#define KFR_INTRIN_BROADCAST(T, N, ...)                                                                      \
    KFR_INTRINSIC simd<T, N> simd_broadcast(simd_t<T, N>, T value) CMT_NOEXCEPT { return __VA_ARGS__; }

KFR_INTRIN_BROADCAST(i8, 16, _mm_set1_epi8(value))
KFR_INTRIN_BROADCAST(i16, 8, _mm_set1_epi16(value))
KFR_INTRIN_BROADCAST(i32, 4, _mm_set1_epi32(value))
KFR_INTRIN_BROADCAST(i64, 2, _mm_set1_epi64x(value))
KFR_INTRIN_BROADCAST(u8, 16, _mm_set1_epi8(value))
KFR_INTRIN_BROADCAST(u16, 8, _mm_set1_epi16(value))
KFR_INTRIN_BROADCAST(u32, 4, _mm_set1_epi32(value))
KFR_INTRIN_BROADCAST(u64, 2, _mm_set1_epi64x(value))
KFR_INTRIN_BROADCAST(f32, 4, _mm_set1_ps(value))
KFR_INTRIN_BROADCAST(f64, 2, _mm_set1_pd(value))

KFR_INTRIN_BROADCAST(i32, 2, simd<i32, 2>(value, value))
KFR_INTRIN_BROADCAST(u32, 2, simd<u32, 2>(value, value))
KFR_INTRIN_BROADCAST(f32, 2, simd<f32, 2>{ value, value })

template <size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<float, Nout> simd_shuffle(simd_t<float, N>, const simd<float, N>& x,
                                             csizes_t<indices...> ind, overload_priority<2>) CMT_NOEXCEPT
{
    return universal_shuffle(simd_t<float, N>{}, x, ind);
}

template <size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<double, Nout> simd_shuffle(simd_t<double, N>, const simd<double, N>& x,
                                              csizes_t<indices...> ind, overload_priority<2>) CMT_NOEXCEPT
{
    return universal_shuffle(simd_t<double, N>{}, x, ind);
}

template <size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<float, Nout> simd_shuffle(simd2_t<float, N, N>, const simd<float, N>& x,
                                             const simd<float, N>& y, csizes_t<indices...> ind,
                                             overload_priority<2>) CMT_NOEXCEPT
{
    return universal_shuffle(simd_t<float, 2 * N>{}, simd_from_halves(simd_t<float, 2 * N>{}, x, y), ind);
}

template <size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<double, Nout> simd_shuffle(simd2_t<double, N, N>, const simd<double, N>& x,
                                              const simd<double, N>& y, csizes_t<indices...> ind,
                                              overload_priority<2>) CMT_NOEXCEPT
{
    return universal_shuffle(simd_t<double, 2 * N>{}, simd_from_halves(simd_t<double, 2 * N>{}, x, y), ind);
}

#define KFR_INTRIN_SHUFFLE_DUPHALVES(T, N, ...)                                                              \
    KFR_INTRINSIC simd<T, N * 2> simd_shuffle(simd_t<T, N>, const simd<T, N>& x,                             \
                                              decltype(csizeseq<N * 2> % csize<N>), overload_priority<9>)    \
        CMT_NOEXCEPT                                                                                         \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }

#define KFR_INTRIN_SHUFFLE_SWAP(T, N, ...)                                                                   \
    KFR_INTRINSIC simd<T, N> simd_shuffle(simd_t<T, N>, const simd<T, N>& x,                                 \
                                          decltype(csizeseq<N> ^ csize<1>), overload_priority<9>)            \
        CMT_NOEXCEPT                                                                                         \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }

#define KFR_INTRIN_SHUFFLE_LINEAR(T, Nout, Nin, ...)                                                         \
    KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd_t<T, Nin>, const simd<T, Nin>& x, csizeseq_t<Nout>,        \
                                             overload_priority<9>) CMT_NOEXCEPT                              \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }
#define KFR_INTRIN_SHUFFLE_LINEAR_START(T, Nout, Nin, Nstart, ...)                                           \
    KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd_t<T, Nin>, const simd<T, Nin>& x,                          \
                                             csizeseq_t<Nout, Nstart>, overload_priority<9>) CMT_NOEXCEPT    \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }

#define KFR_INTRIN_SHUFFLE_CONCAT(T, Nin, ...)                                                               \
    KFR_INTRINSIC simd<T, Nin + Nin> simd_shuffle(simd2_t<T, Nin, Nin>, const simd<T, Nin>& x,               \
                                                  const simd<T, Nin>& y, csizeseq_t<Nin + Nin>,              \
                                                  overload_priority<9>) CMT_NOEXCEPT                         \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }

KFR_INTRINSIC __m128 KFR_swap_ps(__m128 x) { return _mm_shuffle_ps(x, x, _MM_SHUFFLE(2, 3, 0, 1)); }

#ifndef KFR_f32x2_array
// KFR_INTRIN_SHUFFLE_CONCAT(f32, 2, _mm_castpd_ps(_mm_setr_pd(x.whole, y.whole)))
KFR_INTRIN_SHUFFLE_SWAP(f32, 2,
                        _mm_cvtsd_f64(_mm_castps_pd(KFR_swap_ps(_mm_castpd_ps(_mm_set1_pd(x.whole))))))
#else
KFR_INTRIN_SHUFFLE_CONCAT(f32, 2, _mm_setr_ps(x.low, x.high, y.low, y.high))
KFR_INTRIN_SHUFFLE_SWAP(f32, 2, simd<f32, 2>{ x.high, x.low })
#endif

#if defined CMT_COMPILER_IS_MSVC && defined CMT_ARCH_X32
KFR_INTRINSIC __m128i _mm_cvtsi64_si128(int64_t u)
{
    __m128i r      = _mm_setzero_si128();
    r.m128i_i64[0] = u;
    return r;
}
KFR_INTRINSIC int64_t _mm_cvtsi128_si64(const __m128i& i) { return i.m128i_i64[0]; }
KFR_INTRINSIC int64_t _mm_cvttsd_si64(const __m128d& d) { return static_cast<int64_t>(d.m128d_f64[0]); }
KFR_INTRINSIC __m128d _mm_cvtsi64_sd(const __m128d& a, int64_t b)
{
    __m128d r      = a;
    r.m128d_f64[0] = static_cast<double>(b);
    return r;
}
#endif

KFR_INTRIN_BITCAST(f32, i32, 1, _mm_cvtss_f32(_mm_castsi128_ps(_mm_cvtsi32_si128(x))))
KFR_INTRIN_BITCAST(i32, f32, 1, _mm_cvtsi128_si32(_mm_castps_si128(_mm_set_ss(x))))
KFR_INTRIN_BITCAST(f64, i64, 1, _mm_cvtsd_f64(_mm_castsi128_pd(_mm_cvtsi64_si128(x))))
KFR_INTRIN_BITCAST(i64, f64, 1, _mm_cvtsi128_si64(_mm_castpd_si128(_mm_set_sd(x))))

#ifndef CMT_ARCH_AVX
KFR_INTRIN_SHUFFLE_DUPHALVES(i8, 16, simd<i8, 32>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(u8, 16, simd<u8, 32>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(i16, 8, simd<i16, 16>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(u16, 8, simd<u16, 16>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(i32, 4, simd<i32, 8>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(u32, 4, simd<u32, 8>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(i64, 2, simd<i64, 4>{ x, x })
KFR_INTRIN_SHUFFLE_DUPHALVES(u64, 2, simd<u64, 4>{ x, x })
#endif

// extend
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16, 1, _mm_cvtsi32_si128(u8(x)))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8, 1, _mm_cvtsi32_si128(u16(x)))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4, 1, _mm_cvtsi32_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2, 1, _mm_cvtsi64_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16, 1, _mm_cvtsi32_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8, 1, _mm_cvtsi32_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4, 1, _mm_cvtsi32_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u64, 2, 1, _mm_cvtsi64_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4, 1, _mm_set_ss(x))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 2, 1, _mm_set_sd(x))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16, 2, _mm_cvtsi32_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16, 2, _mm_cvtsi32_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16, 4, _mm_cvtsi32_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16, 4, _mm_cvtsi32_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16, 8, _mm_cvtsi64_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16, 8, _mm_cvtsi64_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8, 2, _mm_cvtsi32_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8, 2, _mm_cvtsi32_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8, 4, _mm_cvtsi64_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8, 4, _mm_cvtsi64_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4, 2, _mm_cvtsi64_si128(x.whole))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4, 2, _mm_cvtsi64_si128(x.whole))

// slice
KFR_INTRIN_SHUFFLE_LINEAR(i32, 1, 4, _mm_cvtsi128_si32(x))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 1, 4, _mm_cvtsi128_si32(x))
#if defined CMT_COMPILER_IS_MSVC && _MSC_VER > 1936
KFR_INTRIN_SHUFFLE_LINEAR(i64, 1, 2, i64(x.m128i_i64[0]))
#else
KFR_INTRIN_SHUFFLE_LINEAR(i64, 1, 2, _mm_cvtsi128_si64(x))
#endif
KFR_INTRIN_SHUFFLE_LINEAR(u64, 1, 2, _mm_cvtsi128_si64(x))
KFR_INTRIN_SHUFFLE_LINEAR(f32, 1, 4, _mm_cvtss_f32(x))
KFR_INTRIN_SHUFFLE_LINEAR(f32, 2, 4, bitcast_anything<simd<float, 2>>(_mm_cvtsd_f64(_mm_castps_pd(x))))
#ifndef KFR_f32x2_array
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4, 2, _mm_castpd_ps(_mm_set_sd(x.whole)))
#else
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4, 2, _mm_unpacklo_ps(_mm_set_ss(x.low), _mm_set_ss(x.high)))
#endif
KFR_INTRIN_SHUFFLE_LINEAR(f64, 1, 2, _mm_cvtsd_f64(x))

KFR_INTRIN_SHUFFLE_LINEAR(i8, 2, 16, simd<i8, 2>::from(u16(_mm_cvtsi128_si32(x))))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 4, 16, simd<i8, 4>::from(_mm_cvtsi128_si32(x)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 8, 16, simd<i8, 8>::from(_mm_cvtsi128_si64(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 2, 16, simd<u8, 2>::from(u16(_mm_cvtsi128_si32(x))))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 4, 16, simd<u8, 4>::from(_mm_cvtsi128_si32(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 8, 16, simd<u8, 8>::from(_mm_cvtsi128_si64(x)))

KFR_INTRIN_SHUFFLE_LINEAR(i16, 2, 8, simd<i16, 2>::from(_mm_cvtsi128_si32(x)))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 4, 8, simd<i16, 4>::from(_mm_cvtsi128_si64(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 2, 8, simd<u16, 2>::from(_mm_cvtsi128_si32(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 4, 8, simd<u16, 4>::from(_mm_cvtsi128_si64(x)))

KFR_INTRIN_SHUFFLE_LINEAR(i32, 2, 4, simd<i32, 2>::from(_mm_cvtsi128_si64(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 2, 4, simd<u32, 2>::from(_mm_cvtsi128_si64(x)))

// high
KFR_INTRIN_SHUFFLE_LINEAR_START(u8, 8, 16, 8, simd<u8, 8>::from(KFR_u64sse_INDEX(x, 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(i8, 8, 16, 8, simd<i8, 8>::from(KFR_u64sse_INDEX(x, 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(u16, 4, 8, 4, simd<u16, 4>::from(KFR_u64sse_INDEX(x, 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(i16, 4, 8, 4, simd<i16, 4>::from(KFR_u64sse_INDEX(x, 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(u32, 2, 4, 2, simd<u32, 2>::from(KFR_u64sse_INDEX(x, 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(i32, 2, 4, 2, simd<i32, 2>::from(KFR_u64sse_INDEX(x, 1)))

#ifndef KFR_f32x2_array
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 2, 4, 2, simd<f32, 2>::from(take_hi_sd(_mm_castps_pd(x))))
#else
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 2, 4, 2,
                                simd_halves<f32, 2>{ KFR_f32sse_INDEX(x, 2), KFR_f32sse_INDEX(x, 3) })
#endif

#define KFR_INTRIN_CONVERT(Tout, Tin, N, ...)                                                                \
    KFR_INTRINSIC simd<Tout, N> simd_convert(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x) CMT_NOEXCEPT   \
    {                                                                                                        \
        return __VA_ARGS__;                                                                                  \
    }

#define KFR_INTRIN_CONVERT_NOOP_REF(Tout, Tin, N)                                                            \
    KFR_INTRINSIC const simd<Tout, N>& simd_convert(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x)         \
        CMT_NOEXCEPT                                                                                         \
    {                                                                                                        \
        return x;                                                                                            \
    }
#define KFR_INTRIN_CONVERT_NOOP(Tout, Tin, N)                                                                \
    KFR_INTRINSIC simd<Tout, N> simd_convert(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x) CMT_NOEXCEPT   \
    {                                                                                                        \
        return x;                                                                                            \
    }

KFR_INTRIN_CONVERT(f32, i32, 4, _mm_cvtepi32_ps(x))
KFR_INTRIN_CONVERT(i32, f32, 4, _mm_cvttps_epi32(x))
KFR_INTRIN_CONVERT(i32, f64, 2, simd<i32, 2>::from(_mm_cvtsi128_si64(_mm_cvttpd_epi32(x))))
#ifdef CMT_COMPILER_IS_MSVC
KFR_INTRIN_CONVERT(f64, i32, 2,
                   _mm_cvtepi32_pd(_mm_setr_epi32(bitcast_anything<simd_array<i32, 2>>(x).val[0],
                                                  bitcast_anything<simd_array<i32, 2>>(x).val[1], 0, 0)))
#else
KFR_INTRIN_CONVERT(f64, i32, 2, _mm_cvtepi32_pd(KFR_mm_setr_epi64x(x.whole, 0)))
#endif
KFR_INTRIN_CONVERT(i64, f64, 2,
                   KFR_mm_setr_epi64x(_mm_cvttsd_si64(x), _mm_cvttsd_si64(_mm_unpackhi_pd(x, x))))
KFR_INTRIN_CONVERT(f64, i64, 2,
                   _mm_unpacklo_pd(_mm_cvtsi64_sd(_mm_setzero_pd(), _mm_cvtsi128_si64(x)),
                                   _mm_cvtsi64_sd(_mm_setzero_pd(), KFR_i64sse_INDEX(x, 1))))
#ifdef CMT_ARCH_AVX
KFR_INTRIN_CONVERT(f64, f32, 4, _mm256_cvtps_pd(x))
#else
KFR_INTRIN_CONVERT(f64, f32, 4,
                   simd<f64, 4>{ _mm_cvtps_pd(x),
                                 _mm_cvtps_pd(_mm_shuffle_ps(x, x, _MM_SHUFFLE(1, 0, 3, 2))) })
#endif
#ifdef CMT_ARCH_AVX
KFR_INTRIN_CONVERT(f32, f64, 4, _mm256_cvtpd_ps(x))
#else
KFR_INTRIN_CONVERT(f32, f64, 4,
                   simd<f32, 4>{ _mm_castpd_ps(_mm_unpacklo_pd(_mm_castps_pd(_mm_cvtpd_ps(x.low)),
                                                               _mm_castps_pd(_mm_cvtpd_ps(x.high)))) })
#endif

KFR_INTRIN_CONVERT_NOOP(u8, i8, 1)
KFR_INTRIN_CONVERT_NOOP(i8, u8, 1)
KFR_INTRIN_CONVERT_NOOP(u16, i16, 1)
KFR_INTRIN_CONVERT_NOOP(i16, u16, 1)
KFR_INTRIN_CONVERT_NOOP(u32, i32, 1)
KFR_INTRIN_CONVERT_NOOP(i32, u32, 1)
KFR_INTRIN_CONVERT_NOOP(u64, i64, 1)
KFR_INTRIN_CONVERT_NOOP(i64, u64, 1)

KFR_INTRIN_CONVERT_NOOP_REF(u8, i8, 16)
KFR_INTRIN_CONVERT_NOOP_REF(i8, u8, 16)
KFR_INTRIN_CONVERT_NOOP_REF(u16, i16, 8)
KFR_INTRIN_CONVERT_NOOP_REF(i16, u16, 8)
KFR_INTRIN_CONVERT_NOOP_REF(u32, i32, 4)
KFR_INTRIN_CONVERT_NOOP_REF(i32, u32, 4)
KFR_INTRIN_CONVERT_NOOP_REF(u64, i64, 2)
KFR_INTRIN_CONVERT_NOOP_REF(i64, u64, 2)

#endif // CMT_ARCH_SSE2

#ifdef CMT_ARCH_SSE41

KFR_INTRIN_CONVERT(i16, i8, 8, _mm_cvtepi8_epi16(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(u16, u8, 8, _mm_cvtepu8_epi16(_mm_cvtsi64_si128(x.whole)))

KFR_INTRIN_CONVERT(i32, i16, 4, _mm_cvtepi16_epi32(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(u32, u16, 4, _mm_cvtepu16_epi32(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(i32, i8, 4, _mm_cvtepi8_epi32(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_CONVERT(u32, u8, 4, _mm_cvtepu8_epi32(_mm_cvtsi32_si128(x.whole)))

KFR_INTRIN_CONVERT(i64, i32, 2, _mm_cvtepi32_epi64(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(u64, u32, 2, _mm_cvtepu32_epi64(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(i64, i16, 2, _mm_cvtepi16_epi64(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_CONVERT(u64, u16, 2, _mm_cvtepu16_epi64(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_CONVERT(i64, i8, 2, _mm_cvtepi8_epi64(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_CONVERT(u64, u8, 2, _mm_cvtepu8_epi64(_mm_cvtsi32_si128(x.whole)))

KFR_INTRIN_CONVERT(f32, i8, 4, _mm_cvtepi32_ps(_mm_cvtepi8_epi32(_mm_cvtsi32_si128(x.whole))))
KFR_INTRIN_CONVERT(f32, i16, 4, _mm_cvtepi32_ps(_mm_cvtepi16_epi32(_mm_cvtsi64_si128(x.whole))))
KFR_INTRIN_CONVERT(f32, u8, 4, _mm_cvtepi32_ps(_mm_cvtepu8_epi32(_mm_cvtsi32_si128(x.whole))))
KFR_INTRIN_CONVERT(f32, u16, 4, _mm_cvtepi32_ps(_mm_cvtepu16_epi32(_mm_cvtsi64_si128(x.whole))))

#ifndef CMT_ARCH_AVX
KFR_INTRIN_CONVERT(i64, i32, 4,
                   simd<i64, 4>{ _mm_cvtepi32_epi64(x),
                                 _mm_cvtepi32_epi64(_mm_shuffle_epi32(x, _MM_SHUFFLE(1, 0, 3, 2))) })
#endif
#endif

#ifdef CMT_ARCH_AVX
KFR_INTRIN_MAKE(4, f64, _mm256_setr_pd)
KFR_INTRIN_MAKE(8, f32, _mm256_setr_ps)

KFR_INTRIN_BITCAST(f32, i32, 8, _mm256_castsi256_ps(x))

KFR_INTRIN_BITCAST(i32, f32, 8, _mm256_castps_si256(x))
KFR_INTRIN_BITCAST(f64, i64, 4, _mm256_castsi256_pd(x))
KFR_INTRIN_BITCAST(i64, f64, 4, _mm256_castpd_si256(x))

#ifndef CMT_ARCH_AVX512
KFR_INTRINSIC simd<float, 8> simd_shuffle(simd_t<float, 16>, const simd<float, 16>& x,
                                          csizes_t<2, 3, 6, 7, 10, 11, 14, 15>, overload_priority<9>)
{
    const __m256 t1 = _mm256_permute2f128_ps(x.low, x.high, (0 << 0) | (2 << 4));
    const __m256 t2 = _mm256_permute2f128_ps(x.low, x.high, (1 << 0) | (3 << 4));
    return _mm256_shuffle_ps(t1, t2, (shuffle_mask<8, 2, 3, 2, 3>::value));
}

KFR_INTRINSIC simd<float, 8> simd_shuffle(simd_t<float, 16>, const simd<float, 16>& x,
                                          csizes_t<0, 1, 4, 5, 8, 9, 12, 13>, overload_priority<9>)
{
    const __m256 t1 = _mm256_permute2f128_ps(x.low, x.high, (0 << 0) | (2 << 4));
    const __m256 t2 = _mm256_permute2f128_ps(x.low, x.high, (1 << 0) | (3 << 4));
    return _mm256_shuffle_ps(t1, t2, (shuffle_mask<8, 0, 1, 0, 1>::value));
}
#endif

#ifndef CMT_ARCH_AVX2
KFR_INTRIN_SHUFFLE_DUPHALVES(i8, 16, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(u8, 16, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(i16, 8, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(u16, 8, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(i32, 4, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(u32, 4, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(i64, 2, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
KFR_INTRIN_SHUFFLE_DUPHALVES(u64, 2, _mm256_insertf128_si256(_mm256_castsi128_si256(x), x, 1))
#endif

KFR_INTRINSIC __m256 KFR_mm256_setr_m128(__m128 x, __m128 y)
{
    return _mm256_insertf128_ps(_mm256_castps128_ps256(x), y, 1);
}

KFR_INTRINSIC __m256d KFR_mm256_setr_m128d(__m128d x, __m128d y)
{
    return _mm256_insertf128_pd(_mm256_castpd128_pd256(x), y, 1);
}
KFR_INTRINSIC __m256i KFR_mm256_setr_m128i(__m128i x, __m128i y)
{
#ifdef CMT_ARCH_AVX2
    return _mm256_inserti128_si256(_mm256_castsi128_si256(x), y, 1);
#else
    return _mm256_insertf128_si256(_mm256_castsi128_si256(x), y, 1);
#endif
}

KFR_INTRIN_SHUFFLE_CONCAT(f32, 4, KFR_mm256_setr_m128(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(f64, 2, KFR_mm256_setr_m128d(x, y))

// concat
KFR_INTRIN_SHUFFLE_CONCAT(i8, 16, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i16, 8, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i32, 4, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i64, 2, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u8, 16, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u16, 8, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u32, 4, KFR_mm256_setr_m128i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u64, 2, KFR_mm256_setr_m128i(x, y))

#ifndef CMT_COMPILER_GCC
// GCC bug workaround
KFR_INTRIN_SHUFFLE_CONCAT(i8, 1, simd<i8, 2>(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u8, 1, simd<u8, 2>(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i16, 1, simd<i16, 2>(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u16, 1, simd<u16, 2>(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i32, 1, simd<i32, 2>(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u32, 1, simd<u32, 2>(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(f32, 1, simd<f32, 2>{ x, y })
#endif

KFR_INTRIN_SHUFFLE_CONCAT(f64, 1, _mm_setr_pd(x, y))

KFR_INTRIN_SHUFFLE_DUPHALVES(f32, 4, KFR_mm256_setr_m128(x, x))
KFR_INTRIN_SHUFFLE_DUPHALVES(f64, 2, KFR_mm256_setr_m128d(x, x))

// low
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4, 8, _mm256_castps256_ps128(x))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 2, 4, _mm256_castpd256_pd128(x))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16, 32, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8, 16, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4, 8, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2, 4, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16, 32, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8, 16, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4, 8, _mm256_castsi256_si128(x))
KFR_INTRIN_SHUFFLE_LINEAR(u64, 2, 4, _mm256_castsi256_si128(x))

KFR_INTRIN_SHUFFLE_LINEAR(f32, 2, 8, _mm_cvtsd_f64(_mm_castps_pd(_mm256_castps256_ps128(x))))
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 2, 8, 2, take_hi_sd(_mm_castps_pd(_mm256_castps256_ps128(x))))
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 2, 8, 4, _mm_cvtsd_f64(_mm_castps_pd(_mm256_extractf128_ps(x, 1))))
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 2, 8, 6, take_hi_sd(_mm_castps_pd(_mm256_extractf128_ps(x, 1))))

// extend
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4 * 2, 4, _mm256_castps128_ps256(x))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 2 * 2, 2, _mm256_castpd128_pd256(x))
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4 * 2, 2, _mm256_castps128_ps256(_mm_castpd_ps(_mm_set_sd(x.whole))))

// high
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 4, 8, 4, _mm256_extractf128_ps(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(f64, 2, 4, 2, _mm256_extractf128_pd(x, 1))

#ifndef CMT_ARCH_AVX2
// high
KFR_INTRIN_SHUFFLE_LINEAR_START(i8, 16, 32, 16,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(i16, 8, 16, 8,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(i32, 4, 8, 4,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(i64, 2, 4, 2,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(u8, 16, 32, 16,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(u16, 8, 16, 8,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(u32, 4, 8, 4,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
KFR_INTRIN_SHUFFLE_LINEAR_START(u64, 2, 4, 2,
                                _mm_castps_si128(_mm256_extractf128_ps(_mm256_castsi256_ps(x), 1)))
#endif

KFR_INTRIN_BROADCAST(f32, 8, _mm256_set1_ps(value))
KFR_INTRIN_BROADCAST(f64, 4, _mm256_set1_pd(value))

KFR_INTRIN_SHUFFLE_LINEAR(f32, 8, 1, _mm256_castps128_ps256(_mm_set_ss(x)))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 4, 1, _mm256_castpd128_pd256(_mm_set_sd(x)))
#endif // CMT_ARCH_AVX

#ifdef CMT_ARCH_AVX2
KFR_INTRIN_MAKE(4, i64, _mm256_setr_epi64x)
KFR_INTRIN_MAKE(4, u64, _mm256_setr_epi64x)
KFR_INTRIN_MAKE(8, i32, _mm256_setr_epi32)
KFR_INTRIN_MAKE(8, u32, _mm256_setr_epi32)
KFR_INTRIN_MAKE(16, i16, _mm256_setr_epi16)
KFR_INTRIN_MAKE(16, u16, _mm256_setr_epi16)
KFR_INTRIN_MAKE(32, i8, _mm256_setr_epi8)
KFR_INTRIN_MAKE(32, u8, _mm256_setr_epi8)

KFR_INTRIN_CONVERT(i16, i8, 16, _mm256_cvtepi8_epi16(x))
KFR_INTRIN_CONVERT(u16, u8, 16, _mm256_cvtepu8_epi16(x))

KFR_INTRIN_CONVERT(i32, i16, 8, _mm256_cvtepi16_epi32(x))
KFR_INTRIN_CONVERT(u32, u16, 8, _mm256_cvtepu16_epi32(x))
KFR_INTRIN_CONVERT(i32, i8, 8, _mm256_cvtepi8_epi32(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(u32, u8, 8, _mm256_cvtepu8_epi32(_mm_cvtsi64_si128(x.whole)))

KFR_INTRIN_CONVERT(i64, i32, 4, _mm256_cvtepi32_epi64(x))
KFR_INTRIN_CONVERT(u64, u32, 4, _mm256_cvtepu32_epi64(x))
KFR_INTRIN_CONVERT(i64, i16, 4, _mm256_cvtepi16_epi64(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(u64, u16, 4, _mm256_cvtepu16_epi64(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_CONVERT(i64, i8, 4, _mm256_cvtepi8_epi64(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_CONVERT(u64, u8, 4, _mm256_cvtepu8_epi64(_mm_cvtsi32_si128(x.whole)))

KFR_INTRIN_CONVERT(f32, i8, 8, _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(_mm_cvtsi64_si128(x.whole))))
KFR_INTRIN_CONVERT(f32, i16, 8, _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(x)))
KFR_INTRIN_CONVERT(f32, u8, 8, _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_cvtsi64_si128(x.whole))))
KFR_INTRIN_CONVERT(f32, u16, 8, _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(x)))

KFR_INTRIN_SHUFFLE_LINEAR_START(i8, 16, 32, 16, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(i16, 8, 16, 8, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(i32, 4, 8, 4, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(i64, 2, 4, 2, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(u8, 16, 32, 16, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(u16, 8, 16, 8, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(u32, 4, 8, 4, _mm256_extracti128_si256(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(u64, 2, 4, 2, _mm256_extracti128_si256(x, 1))

KFR_INTRIN_BROADCAST(i8, 32, _mm256_set1_epi8(value))
KFR_INTRIN_BROADCAST(i16, 16, _mm256_set1_epi16(value))
KFR_INTRIN_BROADCAST(i32, 8, _mm256_set1_epi32(value))
KFR_INTRIN_BROADCAST(i64, 4, _mm256_set1_epi64x(value))
KFR_INTRIN_BROADCAST(u8, 32, _mm256_set1_epi8(value))
KFR_INTRIN_BROADCAST(u16, 16, _mm256_set1_epi16(value))
KFR_INTRIN_BROADCAST(u32, 8, _mm256_set1_epi32(value))
KFR_INTRIN_BROADCAST(u64, 4, _mm256_set1_epi64x(value))

KFR_INTRINSIC __m256i KFR_mm_broadcastsi128_si256(const __m128i& x)
{
    return _mm256_inserti128_si256(_mm256_castsi128_si256(x), x, 1);
}

KFR_INTRIN_SHUFFLE_DUPHALVES(i8, 16, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(u8, 16, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(i16, 8, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(u16, 8, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(i32, 4, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(u32, 4, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(i64, 2, KFR_mm_broadcastsi128_si256(x))
KFR_INTRIN_SHUFFLE_DUPHALVES(u64, 2, KFR_mm_broadcastsi128_si256(x))

KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 2, 16, _mm256_castsi128_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 2, 8, _mm256_castsi128_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 2, 4, _mm256_castsi128_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2 * 2, 2, _mm256_castsi128_si256(x))

KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi32_si128(u8(x))))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi32_si128(u16(x))))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi64_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u64, 2 * 2, 1, _mm256_castsi128_si256(_mm_cvtsi64_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 2, 2, _mm256_castsi128_si256(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 2, 2, _mm256_castsi128_si256(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 2, 4, _mm256_castsi128_si256(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 2, 4, _mm256_castsi128_si256(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 2, 8, _mm256_castsi128_si256(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 2, 8, _mm256_castsi128_si256(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 2, 2, _mm256_castsi128_si256(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 2, 2, _mm256_castsi128_si256(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 2, 4, _mm256_castsi128_si256(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 2, 4, _mm256_castsi128_si256(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4 * 2, 2, _mm256_castsi128_si256(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 2, 2, _mm256_castsi128_si256(_mm_cvtsi64_si128(x.whole)))

KFR_INTRIN_CONVERT(i32, f32, 8, _mm256_cvttps_epi32(x))
KFR_INTRIN_CONVERT(f32, i32, 8, _mm256_cvtepi32_ps(x))
KFR_INTRIN_CONVERT(f64, i32, 4, _mm256_cvtepi32_pd(x))
KFR_INTRIN_CONVERT(i32, f64, 4, _mm256_cvttpd_epi32(x))
#endif // CMT_ARCH_AVX2

#ifdef CMT_ARCH_AVX512

static inline __m512d KFR_mm512_setr_pd(f64 x0, f64 x1, f64 x2, f64 x3, f64 x4, f64 x5, f64 x6, f64 x7)
{
    return _mm512_set_pd(x7, x6, x5, x4, x3, x2, x1, x0);
}
static inline __m512 KFR_mm512_setr_ps(f32 x0, f32 x1, f32 x2, f32 x3, f32 x4, f32 x5, f32 x6, f32 x7, f32 x8,
                                       f32 x9, f32 x10, f32 x11, f32 x12, f32 x13, f32 x14, f32 x15)
{
    return _mm512_set_ps(x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0);
}
static inline __m512i KFR_mm512_setr_epi64(i64 x0, i64 x1, i64 x2, i64 x3, i64 x4, i64 x5, i64 x6, i64 x7)
{
    return _mm512_set_epi64(x7, x6, x5, x4, x3, x2, x1, x0);
}
static inline __m512i KFR_mm512_setr_epi32(i32 x0, i32 x1, i32 x2, i32 x3, i32 x4, i32 x5, i32 x6, i32 x7,
                                           i32 x8, i32 x9, i32 x10, i32 x11, i32 x12, i32 x13, i32 x14,
                                           i32 x15)
{
    return _mm512_set_epi32(x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0);
}
static inline __m512i KFR_mm512_setr_epi16(i16 x0, i16 x1, i16 x2, i16 x3, i16 x4, i16 x5, i16 x6, i16 x7,
                                           i16 x8, i16 x9, i16 x10, i16 x11, i16 x12, i16 x13, i16 x14,
                                           i16 x15, i16 x16, i16 x17, i16 x18, i16 x19, i16 x20, i16 x21,
                                           i16 x22, i16 x23, i16 x24, i16 x25, i16 x26, i16 x27, i16 x28,
                                           i16 x29, i16 x30, i16 x31)
{
#ifdef CMT_COMPILER_GCC
    typedef short v32hi __attribute__((__vector_size__(64)));
    return __extension__(__m512i)(v32hi){ x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,  x8,  x9,  x10,
                                          x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21,
                                          x22, x23, x24, x25, x26, x27, x28, x29, x30, x31 };
#else
    return _mm512_set_epi16(x31, x30, x29, x28, x27, x26, x25, x24, x23, x22, x21, x20, x19, x18, x17, x16,
                            x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0);
#endif
}
static inline __m512i KFR_mm512_setr_epi8(i8 x0, i8 x1, i8 x2, i8 x3, i8 x4, i8 x5, i8 x6, i8 x7, i8 x8,
                                          i8 x9, i8 x10, i8 x11, i8 x12, i8 x13, i8 x14, i8 x15, i8 x16,
                                          i8 x17, i8 x18, i8 x19, i8 x20, i8 x21, i8 x22, i8 x23, i8 x24,
                                          i8 x25, i8 x26, i8 x27, i8 x28, i8 x29, i8 x30, i8 x31, i8 x32,
                                          i8 x33, i8 x34, i8 x35, i8 x36, i8 x37, i8 x38, i8 x39, i8 x40,
                                          i8 x41, i8 x42, i8 x43, i8 x44, i8 x45, i8 x46, i8 x47, i8 x48,
                                          i8 x49, i8 x50, i8 x51, i8 x52, i8 x53, i8 x54, i8 x55, i8 x56,
                                          i8 x57, i8 x58, i8 x59, i8 x60, i8 x61, i8 x62, i8 x63)
{
#ifdef CMT_COMPILER_GCC
    typedef char v64qi __attribute__((__vector_size__(64)));
    return __extension__(__m512i)(v64qi){ x0,  x1,  x2,  x3,  x4,  x5,  x6,  x7,  x8,  x9,  x10, x11, x12,
                                          x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25,
                                          x26, x27, x28, x29, x30, x31, x32, x33, x34, x35, x36, x37, x38,
                                          x39, x40, x41, x42, x43, x44, x45, x46, x47, x48, x49, x50, x51,
                                          x52, x53, x54, x55, x56, x57, x58, x59, x60, x61, x62, x63 };
#else
    return _mm512_set_epi8(x63, x62, x61, x60, x59, x58, x57, x56, x55, x54, x53, x52, x51, x50, x49, x48,
                           x47, x46, x45, x44, x43, x42, x41, x40, x39, x38, x37, x36, x35, x34, x33, x32,
                           x31, x30, x29, x28, x27, x26, x25, x24, x23, x22, x21, x20, x19, x18, x17, x16,
                           x15, x14, x13, x12, x11, x10, x9, x8, x7, x6, x5, x4, x3, x2, x1, x0);
#endif
}

KFR_INTRINSIC __m512 KFR_mm512_setr_m256(__m256 x, __m256 y)
{
    return _mm512_insertf32x8(_mm512_castps256_ps512(x), y, 1);
}

KFR_INTRINSIC __m512d KFR_mm512_setr_m256d(__m256d x, __m256d y)
{
    return _mm512_insertf64x4(_mm512_castpd256_pd512(x), y, 1);
}
KFR_INTRINSIC __m512i KFR_mm512_setr_m256i(__m256i x, __m256i y)
{
    return _mm512_inserti32x8(_mm512_castsi256_si512(x), y, 1);
}

KFR_INTRIN_MAKE(8, f64, KFR_mm512_setr_pd)
KFR_INTRIN_MAKE(16, f32, KFR_mm512_setr_ps)

KFR_INTRIN_MAKE(8, i64, KFR_mm512_setr_epi64)
KFR_INTRIN_MAKE(8, u64, KFR_mm512_setr_epi64)
KFR_INTRIN_MAKE(16, i32, KFR_mm512_setr_epi32)
KFR_INTRIN_MAKE(16, u32, KFR_mm512_setr_epi32)
KFR_INTRIN_MAKE(32, i16, KFR_mm512_setr_epi16)
KFR_INTRIN_MAKE(32, u16, KFR_mm512_setr_epi16)
KFR_INTRIN_MAKE(64, i8, KFR_mm512_setr_epi8)
KFR_INTRIN_MAKE(64, u8, KFR_mm512_setr_epi8)

KFR_INTRIN_BROADCAST(f32, 16, _mm512_set1_ps(value))
KFR_INTRIN_BROADCAST(f64, 8, _mm512_set1_pd(value))

KFR_INTRIN_BROADCAST(i8, 64, _mm512_set1_epi8(value))
KFR_INTRIN_BROADCAST(i16, 32, _mm512_set1_epi16(value))
KFR_INTRIN_BROADCAST(i32, 16, _mm512_set1_epi32(value))
KFR_INTRIN_BROADCAST(i64, 8, _mm512_set1_epi64(value))
KFR_INTRIN_BROADCAST(u8, 64, _mm512_set1_epi8(value))
KFR_INTRIN_BROADCAST(u16, 32, _mm512_set1_epi16(value))
KFR_INTRIN_BROADCAST(u32, 16, _mm512_set1_epi32(value))
KFR_INTRIN_BROADCAST(u64, 8, _mm512_set1_epi64(value))

KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi32_si128(u8(x))))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi32_si128(u16(x))))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi64_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi32_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u64, 2 * 4, 1, _mm512_castsi128_si512(_mm_cvtsi64_si128(x)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 4, 2, _mm512_castsi128_si512(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 4, 2, _mm512_castsi128_si512(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 4, 4, _mm512_castsi128_si512(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 4, 4, _mm512_castsi128_si512(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 4, 8, _mm512_castsi128_si512(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 4, 8, _mm512_castsi128_si512(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 4, 2, _mm512_castsi128_si512(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 4, 2, _mm512_castsi128_si512(_mm_cvtsi32_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 4, 4, _mm512_castsi128_si512(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 4, 4, _mm512_castsi128_si512(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4 * 4, 2, _mm512_castsi128_si512(_mm_cvtsi64_si128(x.whole)))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 4, 2, _mm512_castsi128_si512(_mm_cvtsi64_si128(x.whole)))

KFR_INTRIN_CONVERT(i32, f32, 16, _mm512_cvttps_epi32(x))
KFR_INTRIN_CONVERT(f32, i32, 16, _mm512_cvtepi32_ps(x))
KFR_INTRIN_CONVERT(f64, i32, 8, _mm512_cvtepi32_pd(x))
KFR_INTRIN_CONVERT(i32, f64, 8, _mm512_cvttpd_epi32(x))

KFR_INTRIN_SHUFFLE_LINEAR(f32, 4 * 4, 4, _mm512_castps128_ps512(x))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 2 * 4, 2, _mm512_castpd128_pd512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 4, 16, _mm512_castsi128_si512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 4, 8, _mm512_castsi128_si512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 4, 4, _mm512_castsi128_si512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2 * 4, 2, _mm512_castsi128_si512(x))

KFR_INTRIN_SHUFFLE_LINEAR(f32, 4 * 4, 2 * 4, _mm512_castps256_ps512(x))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 2 * 4, 2 * 2, _mm512_castpd256_pd512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 4, 2 * 16, _mm512_castsi256_si512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 4, 2 * 8, _mm512_castsi256_si512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 4, 2 * 4, _mm512_castsi256_si512(x))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2 * 4, 2 * 2, _mm512_castsi256_si512(x))

// low
KFR_INTRIN_SHUFFLE_LINEAR(f32, 4 * 2, 8 * 2, _mm512_castps512_ps256(x))
KFR_INTRIN_SHUFFLE_LINEAR(f64, 2 * 2, 4 * 2, _mm512_castpd512_pd256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i8, 16 * 2, 32 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i16, 8 * 2, 16 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i32, 4 * 2, 8 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(i64, 2 * 2, 4 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(u8, 16 * 2, 32 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(u16, 8 * 2, 16 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(u32, 4 * 2, 8 * 2, _mm512_castsi512_si256(x))
KFR_INTRIN_SHUFFLE_LINEAR(u64, 2 * 2, 4 * 2, _mm512_castsi512_si256(x))

// high
KFR_INTRIN_SHUFFLE_LINEAR_START(f32, 4 * 2, 8 * 2, 4 * 2, _mm512_extractf32x8_ps(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(f64, 2 * 2, 4 * 2, 2 * 2, _mm512_extractf64x4_pd(x, 1))

KFR_INTRIN_SHUFFLE_LINEAR_START(i32, 4 * 2, 8 * 2, 4 * 2, _mm512_extracti32x8_epi32(x, 1))
KFR_INTRIN_SHUFFLE_LINEAR_START(i64, 2 * 2, 4 * 2, 2 * 2, _mm512_extracti64x4_epi64(x, 1))

// concat
KFR_INTRIN_SHUFFLE_CONCAT(f32, 4 * 2, KFR_mm512_setr_m256(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(f64, 2 * 2, KFR_mm512_setr_m256d(x, y))

KFR_INTRIN_SHUFFLE_CONCAT(i8, 16 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i16, 8 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i32, 4 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(i64, 2 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u8, 16 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u16, 8 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u32, 4 * 2, KFR_mm512_setr_m256i(x, y))
KFR_INTRIN_SHUFFLE_CONCAT(u64, 2 * 2, KFR_mm512_setr_m256i(x, y))
#endif

#endif

// generic functions

template <typename T, size_t N1>
KFR_INTRINSIC const simd<T, N1>& simd_concat(const simd<T, N1>& x) CMT_NOEXCEPT;

template <typename T, size_t N1, size_t N2, size_t... Ns, size_t Nscount = csum(csizes<Ns...>)>
KFR_INTRINSIC simd<T, N1 + N2 + Nscount> simd_concat(const simd<T, N1>& x, const simd<T, N2>& y,
                                                     const simd<T, Ns>&... z) CMT_NOEXCEPT;

template <typename T, size_t N>
KFR_INTRINSIC simd_array<T, N> to_simd_array(const simd<T, N>& x) CMT_NOEXCEPT
{
    return bitcast_anything<simd_array<T, N>>(x);
}

#if defined CMT_COMPILER_IS_MSVC

template <typename T, size_t N, KFR_ENABLE_IF(!is_simd_small_array<simd<T, N>>)>
KFR_INTRINSIC simd<T, N> from_simd_array(const simd_array<T, N>& x) CMT_NOEXCEPT
{
    return bitcast_anything<simd<T, N>>(x);
}

template <typename T, size_t N, size_t... indices>
KFR_INTRINSIC simd<T, N> from_simd_array_impl(const simd_array<T, N>& x, csizes_t<indices...>) CMT_NOEXCEPT
{
    return { unwrap_bit_value(x.val[indices])... };
}

template <typename T, size_t N, KFR_ENABLE_IF(is_simd_small_array<simd<T, N>>)>
KFR_INTRINSIC simd<T, N> from_simd_array(const simd_array<T, N>& x) CMT_NOEXCEPT
{
    return from_simd_array_impl(x, csizeseq<N>);
}
#else
template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> from_simd_array(const simd_array<T, N>& x) CMT_NOEXCEPT
{
    return bitcast_anything<simd<T, N>>(x);
}

#endif

template <typename Tout>
KFR_INTRINSIC void simd_make(ctype_t<Tout>) CMT_NOEXCEPT = delete;

template <typename Tout, typename Arg>
KFR_INTRINSIC simd<Tout, 1> simd_make(ctype_t<Tout>, const Arg& arg) CMT_NOEXCEPT
{
    return simd<Tout, 1>{ unwrap_bit_value(static_cast<Tout>(arg)) };
}

template <typename T, size_t... indices, typename... Args, size_t N = sizeof...(indices)>
KFR_INTRINSIC simd<T, N> simd_make_helper(csizes_t<indices...>, const Args&... args) CMT_NOEXCEPT;

template <typename Tout, typename... Args, size_t N = sizeof...(Args), KFR_ENABLE_IF(N > 1)>
KFR_INTRINSIC simd<Tout, N> simd_make(ctype_t<Tout>, const Args&... args) CMT_NOEXCEPT
{
    constexpr size_t Nlow = prev_poweroftwo(N - 1);
    return simd_concat<Tout, Nlow, N - Nlow>(simd_make_helper<Tout>(csizeseq<Nlow>, args...),
                                             simd_make_helper<Tout>(csizeseq<N - Nlow, Nlow>, args...));
}

template <typename T, size_t... indices, typename... Args, size_t N>
KFR_INTRINSIC simd<T, N> simd_make_helper(csizes_t<indices...>, const Args&... args) CMT_NOEXCEPT
{
    const T temp[] = { static_cast<T>(args)... };
    return simd_make(cometa::ctype<T>, temp[indices]...);
}

/// @brief Returns vector with undefined value
template <typename Tout, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_undefined() CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);
    simd<Tout, N> x;
    return x;
}

/// @brief Returns vector with all zeros
template <typename Tout, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_zeros() CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);
    return from_simd_array<Tout, N>({ Tout() });
}

/// @brief Returns vector with all ones
template <typename Tout, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_allones() CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);
    simd_array<Tout, N> x{};
    KFR_COMPONENTWISE(x.val[i] = special_constants<Tout>::allones());
    return from_simd_array(x);
}

/// @brief Converts input vector to vector with subtype Tout
template <typename Tout, typename Tin, size_t N, size_t Nout = (sizeof(Tin) * N / sizeof(Tout))
#ifdef CMT_COMPILER_IS_MSVC
                                                     ,
          KFR_ENABLE_IF((Nout == 1 || N == 1) && !std::is_same_v<Tout, Tin>)
#else
                                                     ,
          KFR_ENABLE_IF(Nout == 1 || N == 1)
#endif
          >
KFR_INTRINSIC simd<Tout, Nout> simd_bitcast(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x) CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);
    return bitcast_anything<simd<Tout, Nout>>(x);
}

/// @brief Converts input vector to vector with subtype Tout
template <typename Tout, typename Tin, size_t N, size_t Nout = (sizeof(Tin) * N / sizeof(Tout))
#ifdef CMT_COMPILER_IS_MSVC
                                                     ,
          KFR_ENABLE_IF(Nout > 1 && N > 1 && !std::is_same_v<Tout, Tin>)
#else
                                                     ,
          KFR_ENABLE_IF(Nout > 1 && N > 1)
#endif
          >
KFR_INTRINSIC simd<Tout, Nout> simd_bitcast(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x) CMT_NOEXCEPT
{
    constexpr size_t Nlow = prev_poweroftwo(N - 1);
    return simd_concat<Tout, Nlow * Nout / N, (N - Nlow) * Nout / N>(
        simd_bitcast(simd_cvt_t<Tout, Tin, Nlow>{},
                     unwrap_bit_value(simd_shuffle(simd_t<Tin, N>{}, x, csizeseq<Nlow>, overload_auto))),
        simd_bitcast(
            simd_cvt_t<Tout, Tin, N - Nlow>{},
            unwrap_bit_value(simd_shuffle(simd_t<Tin, N>{}, x, csizeseq<N - Nlow, Nlow>, overload_auto))));
}

template <typename T, size_t N>
KFR_INTRINSIC const simd<T, N>& simd_bitcast(simd_cvt_t<T, T, N>, const simd<T, N>& x) CMT_NOEXCEPT
{
    return x;
}

template <typename T, size_t N, size_t index>
KFR_INTRINSIC T simd_get_element(const simd<T, N>& value, csize_t<index>) CMT_NOEXCEPT
{
    return wrap_bit_value<T>(simd_shuffle(simd_t<T, N>{}, value, csizes<index>, overload_auto));
}

template <typename T, size_t N, size_t index>
KFR_INTRINSIC simd<T, N> simd_set_element(simd<T, N> value, csize_t<index>, unwrap_bit<T> x) CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);
    simd_array<T, N> arr = to_simd_array<T, N>(value);
    arr.val[index]       = x;
    return from_simd_array(arr);
}

template <typename T, size_t N>
KFR_INTRINSIC const simd<T, N>& simd_shuffle(simd_t<T, N>, const simd<T, N>& x, csizeseq_t<N>,
                                             overload_priority<10>) CMT_NOEXCEPT
{
    return x;
}

template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC const simd<T, N1>& simd_shuffle(simd2_t<T, N1, N2>, const simd<T, N1>& x, const simd<T, N2>&,
                                              csizeseq_t<N1>, overload_priority<9>) CMT_NOEXCEPT
{
    return x;
}

template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC const simd<T, N2>& simd_shuffle(simd2_t<T, N1, N2>, const simd<T, N1>&, const simd<T, N2>& y,
                                              csizeseq_t<N2, N1>, overload_priority<9>) CMT_NOEXCEPT
{
    return y;
}

// concat()
template <typename T, size_t N,
          KFR_ENABLE_IF(is_poweroftwo(N) &&
                        std::is_same_v<simd<T, N + N>, simd_halves<unwrap_bit<T>, N + N>>)>
KFR_INTRINSIC simd<T, N + N> simd_shuffle(simd2_t<T, N, N>, const simd<T, N>& x, const simd<T, N>& y,
                                          csizeseq_t<N + N>, overload_priority<8>) CMT_NOEXCEPT
{
    return simd<T, N + N>{ x, y };
}

template <typename T>
KFR_INTRINSIC simd<T, 1> simd_broadcast(simd_t<T, 1>, identity<T> value) CMT_NOEXCEPT
{
    return { unwrap_bit_value(value) };
}

template <typename T, size_t N, KFR_ENABLE_IF(N >= 2), size_t Nlow = prev_poweroftwo(N - 1)>
KFR_INTRINSIC simd<T, N> simd_broadcast(simd_t<T, N>, identity<T> value) CMT_NOEXCEPT
{
    return simd_concat<T, Nlow, N - Nlow>(simd_broadcast(simd_t<T, Nlow>{}, value),
                                          simd_broadcast(simd_t<T, N - Nlow>{}, value));
}

template <typename T, size_t N,
          KFR_ENABLE_IF(is_poweroftwo(N) && std::is_same_v<simd<T, N>, simd_halves<unwrap_bit<T>, N>>)>
KFR_INTRINSIC simd<T, N / 2> simd_shuffle(simd_t<T, N>, const simd<T, N>& x, csizeseq_t<N / 2>,
                                          overload_priority<7>) CMT_NOEXCEPT
{
    return x.low;
}

template <typename T, size_t N,
          KFR_ENABLE_IF(is_poweroftwo(N) && std::is_same_v<simd<T, N>, simd_halves<unwrap_bit<T>, N>>)>
KFR_INTRINSIC simd<T, N / 2> simd_shuffle(simd_t<T, N>, const simd<T, N>& x, csizeseq_t<N / 2, N / 2>,
                                          overload_priority<7>) CMT_NOEXCEPT
{
    return x.high;
}

template <typename T, size_t N, size_t index>
KFR_INTRINSIC T simd_shuffle(simd_t<T, N>, const simd<T, N>& x, csizes_t<index>,
                             overload_priority<6>) CMT_NOEXCEPT
{
    return to_simd_array<T, N>(x).val[index];
}

template <typename T, size_t Nout, size_t N>
simd_array<T, Nout> simd_shuffle_generic(const simd_array<T, N>& x, const unsigned (&indices)[Nout])
{
    simd_array<T, Nout> result;
    for (size_t i = 0; i < Nout; ++i)
    {
        const size_t index = indices[i];
        result.val[i]      = index >= N ? T() : static_cast<T>(x.val[index]);
    }
    return result;
}

template <typename T, size_t Nout, size_t N1, size_t N2>
simd_array<T, Nout> simd_shuffle2_generic(const simd_array<T, N1>& x, const simd_array<T, N2>& y,
                                          const unsigned (&indices)[Nout])
{
    simd_array<T, Nout> result;
    for (size_t i = 0; i < Nout; ++i)
    {
        const size_t index = indices[i];
        result.val[i]      = index >= N1 + N2 ? T()
                             : index >= N1    ? static_cast<T>(y.val[index - N1])
                                              : static_cast<T>(x.val[index]);
    }
    return result;
}

template <typename T, size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd_t<T, N>, const simd<T, N>& x, csizes_t<indices...>,
                                         overload_generic) CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);
#ifdef CMT_COMPILER_IS_MSVC
    const simd_array<T, N> xx                 = to_simd_array<T, N>(x);
    constexpr static unsigned indices_array[] = { static_cast<unsigned>(indices)... };
    return from_simd_array<T, Nout>(simd_shuffle_generic<T, Nout, N>(xx, indices_array));
#else
    return from_simd_array<T, Nout>({ (indices >= N ? T() : to_simd_array<T, N>(x).val[indices])... });
#endif
}

template <typename T, size_t N, size_t N2 = N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd2_t<T, N, N>, const simd<T, N>& x, const simd<T, N>& y,
                                         csizes_t<indices...>, overload_generic) CMT_NOEXCEPT
{
    static_assert(N == N2, "");
    not_optimized(CMT_FUNC_SIGNATURE);
#ifdef CMT_COMPILER_IS_MSVC
    const simd_array<T, N> xx                 = to_simd_array<T, N>(x);
    const simd_array<T, N> yy                 = to_simd_array<T, N>(y);
    constexpr static unsigned indices_array[] = { static_cast<unsigned>(indices)... };
    return from_simd_array<T, Nout>(simd_shuffle2_generic<T, Nout, N, N>(xx, yy, indices_array));
#else
    return from_simd_array<T, Nout>({ (indices >= N * 2 ? T()
                                       : indices >= N   ? to_simd_array<T, N>(y).val[indices - N]
                                                        : to_simd_array<T, N>(x).val[indices])... });
#endif
}

template <typename T, size_t N1, size_t N2, size_t... indices, KFR_ENABLE_IF(N1 != N2),
          size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd2_t<T, N1, N2>, const simd<T, N1>& x, const simd<T, N2>& y,
                                         csizes_t<indices...>, overload_generic) CMT_NOEXCEPT
{
    not_optimized(CMT_FUNC_SIGNATURE);

#ifdef CMT_COMPILER_IS_MSVC
    const simd_array<T, N1> xx                = to_simd_array<T, N1>(x);
    const simd_array<T, N2> yy                = to_simd_array<T, N2>(y);
    constexpr static unsigned indices_array[] = { static_cast<unsigned>(indices)... };
    return from_simd_array<T, Nout>(simd_shuffle2_generic<T, Nout, N1, N2>(xx, yy, indices_array));
#else

    return from_simd_array<T, Nout>({ (indices > N1 + N2 ? T()
                                       : indices >= N1   ? to_simd_array<T, N2>(y).val[indices - N1]
                                                         : to_simd_array<T, N1>(x).val[indices])... });
#endif
}

template <typename T, size_t N1>
KFR_INTRINSIC const simd<T, N1>& simd_concat(const simd<T, N1>& x) CMT_NOEXCEPT
{
    return x;
}

template <typename T, size_t N1, size_t N2, size_t N3, size_t N4>
KFR_INTRINSIC simd<T, N1 + N2 + N3 + N4> simd_concat4(const simd<T, N1>& x, const simd<T, N2>& y,
                                                      const simd<T, N3>& z, const simd<T, N4>& w) CMT_NOEXCEPT
{
    return simd_shuffle(simd2_t<T, N1 + N2, N3 + N4>{},
                        simd_shuffle(simd2_t<T, N1, N2>{}, x, y, csizeseq<N1 + N2>, overload_auto),
                        simd_shuffle(simd2_t<T, N3, N4>{}, z, w, csizeseq<N3 + N4>, overload_auto),
                        csizeseq<N1 + N2 + N3 + N4>, overload_auto);
}

template <typename T, size_t N1, size_t N2, size_t... Ns, size_t Nscount /*= csum(csizes<Ns...>)*/>
KFR_INTRINSIC simd<T, N1 + N2 + Nscount> simd_concat(const simd<T, N1>& x, const simd<T, N2>& y,
                                                     const simd<T, Ns>&... z) CMT_NOEXCEPT
{
    if constexpr (sizeof...(Ns) == 2)
    {
        return simd_concat4<T, N1, N2, Ns...>(x, y, z...);
    }
    else
    {
        return simd_shuffle(simd2_t<T, N1, N2 + Nscount>{}, x, simd_concat<T, N2, Ns...>(y, z...),
                            csizeseq<N1 + N2 + Nscount>, overload_auto);
    }
}

template <typename Tout, typename Tin, size_t N, size_t... indices>
KFR_INTRINSIC simd<Tout, N> simd_convert__(const simd<Tin, N>& x, csizes_t<indices...>) CMT_NOEXCEPT
{
    const simd_array<Tin, N> xx = to_simd_array<Tin, N>(x);
    return simd_make(cometa::ctype<Tout>, static_cast<Tout>(xx.val[indices])...);
}

/// @brief Converts input vector to vector with subtype Tout
template <typename Tout, typename Tin, KFR_ENABLE_IF(!std::is_same<Tout, Tin>::value)>
KFR_INTRINSIC simd<Tout, 1> simd_convert(simd_cvt_t<Tout, Tin, 1>, const simd<Tin, 1>& x) CMT_NOEXCEPT
{
    return simd_make(cometa::ctype<Tout>, static_cast<Tout>(x));
}

/// @brief Converts input vector to vector with subtype Tout
template <typename Tout, typename Tin, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_convert(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x) CMT_NOEXCEPT
{
    constexpr size_t Nlow = prev_poweroftwo(N - 1);
    return simd_concat<Tout, Nlow, N - Nlow>(
        simd_convert(simd_cvt_t<Tout, Tin, Nlow>{},
                     simd_shuffle(simd_t<Tin, N>{}, x, csizeseq<Nlow>, overload_auto)),
        simd_convert(simd_cvt_t<Tout, Tin, N - Nlow>{},
                     simd_shuffle(simd_t<Tin, N>{}, x, csizeseq<N - Nlow, Nlow>, overload_auto)));
}

/// @brief Converts input vector to vector with subtype Tout
template <typename T, size_t N>
KFR_INTRINSIC const simd<T, N>& simd_convert(simd_cvt_t<T, T, N>, const simd<T, N>& x) CMT_NOEXCEPT
{
    return x;
}

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wignored-attributes")

template <typename T, size_t N, bool A>
using simd_storage = struct_with_alignment<simd<T, N>, A>;

CMT_PRAGMA_GNU(GCC diagnostic pop)

template <typename T, size_t N>
KFR_INTRINSIC T simd_get_element(const simd<T, N>& value, size_t index) CMT_NOEXCEPT
{
    return to_simd_array<T, N>(value).val[index];
}

template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> simd_set_element(const simd<T, N>& value, size_t index, unwrap_bit<T> x) CMT_NOEXCEPT
{
    simd_array<T, N> arr = to_simd_array<T, N>(value);
    arr.val[index]       = x;
    return from_simd_array(arr);
}

#define SIMD_TYPE_INTRIN(T, N, TO_SCALAR, FROM_SCALAR, FROM_BROADCAST, FROM_ZERO)                            \
    KFR_INTRINSIC T simd_to_scalar(simd_t<T, N>, const simd<T, N>& x) { return TO_SCALAR; }                  \
    KFR_INTRINSIC simd<T, N> simd_from_scalar(simd_t<T, N>, unwrap_bit<T> x) { return FROM_SCALAR; }         \
    KFR_INTRINSIC simd<T, N> simd_from_broadcast(simd_t<T, N>, unwrap_bit<T> x) { return FROM_BROADCAST; }   \
    KFR_INTRINSIC simd<T, N> simd_from_zero(simd_t<T, N>) { return FROM_ZERO; }

#define SIMD_TYPE_INTRIN_EX(T, N, TO_SCALAR, FROM_SCALAR, FROM_BROADCAST, FROM_ZERO, GET_LOW, GET_HIGH,      \
                            FROM_HALVES)                                                                     \
    SIMD_TYPE_INTRIN(T, N, TO_SCALAR, FROM_SCALAR, FROM_BROADCAST, FROM_ZERO)                                \
    KFR_INTRINSIC simd<T, N / 2> simd_get_low(simd_t<T, N>, const simd<T, N>& x) { return GET_LOW; }         \
    KFR_INTRINSIC simd<T, N / 2> simd_get_high(simd_t<T, N>, const simd<T, N>& x) { return GET_HIGH; }       \
    KFR_INTRINSIC simd<T, N> simd_from_halves(simd_t<T, N>, const simd<T, N / 2>& x,                         \
                                              const simd<T, N / 2>& y)                                       \
    {                                                                                                        \
        return FROM_HALVES;                                                                                  \
    }

template <typename T, size_t Nout, size_t Nin>
KFR_INTRINSIC simd<T, Nout> simd_from_partial(simd2_t<T, Nout, Nin>, const simd<T, Nin>& x)
{
#ifdef CMT_COMPILER_IS_MSVC
    union
    {
        simd<T, Nin> in;
        simd<T, Nout> out;
    } u;
    u.in = x;
    return u.out;
#else
    union
    {
        simd<T, Nin> in;
        simd<T, Nout> out;
    } u{ x };
    return u.out;
#endif
}
template <typename T, size_t N>
KFR_INTRINSIC simd<T, N / 2> simd_get_low(simd_t<T, N>, const simd<T, N>& x)
{
    return x.low;
}
template <typename T, size_t N>
KFR_INTRINSIC simd<T, N / 2> simd_get_high(simd_t<T, N>, const simd<T, N>& x)
{
    return x.high;
}
template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> simd_from_halves(simd_t<T, N>, const simd<T, N / 2>& x, const simd<T, N / 2>& y)
{
    return { x, y };
}

KFR_INTRINSIC simd<float, 4> simd_from_halves(simd_t<float, 4>, const simd<float, 2>& x,
                                              const simd<float, 2>& y)
{
#ifndef KFR_f32x2_array
    return _mm_castpd_ps(_mm_setr_pd(x.whole, y.whole));
#else
    return _mm_setr_ps(x.low, x.high, y.low, y.high);
#endif
}

KFR_INTRINSIC simd<double, 2> simd_from_halves(simd_t<double, 2>, const simd<double, 1>& x,
                                               const simd<double, 1>& y)
{
    return _mm_setr_pd(x, y);
}

SIMD_TYPE_INTRIN(f32, 4, _mm_cvtss_f32(x), _mm_set_ss(x), _mm_set1_ps(x), _mm_setzero_ps())
SIMD_TYPE_INTRIN(f64, 2, _mm_cvtsd_f64(x), _mm_set_sd(x), _mm_set1_pd(x), _mm_setzero_pd())

#ifdef CMT_ARCH_AVX
SIMD_TYPE_INTRIN_EX(f32, 8, _mm256_cvtss_f32(x), _mm256_castps128_ps256(_mm_set_ss(x)), _mm256_set1_ps(x),
                    _mm256_setzero_ps(), _mm256_castps256_ps128(x), _mm256_extractf128_ps(x, 1),
                    KFR_mm256_setr_m128(x, y))
SIMD_TYPE_INTRIN_EX(f64, 4, _mm256_cvtsd_f64(x), _mm256_castpd128_pd256(_mm_set_sd(x)), _mm256_set1_pd(x),
                    _mm256_setzero_pd(), _mm256_castpd256_pd128(x), _mm256_extractf128_pd(x, 1),
                    KFR_mm256_setr_m128d(x, y))
#endif

#ifdef CMT_ARCH_AVX512
SIMD_TYPE_INTRIN_EX(f32, 16, _mm512_cvtss_f32(x), _mm512_castps128_ps512(_mm_set_ss(x)), _mm512_set1_ps(x),
                    _mm512_setzero_ps(), _mm512_castps512_ps256(x), _mm512_extractf32x8_ps(x, 1),
                    KFR_mm512_setr_m256(x, y))
SIMD_TYPE_INTRIN_EX(f64, 8, _mm512_cvtsd_f64(x), _mm512_castpd128_pd512(_mm_set_sd(x)), _mm512_set1_pd(x),
                    _mm512_setzero_pd(), _mm512_castpd512_pd256(x), _mm512_extractf64x4_pd(x, 1),
                    KFR_mm512_setr_m256d(x, y))
#endif

#ifdef CMT_ARCH_SSE2

template <size_t I0, size_t I1, size_t I2, size_t I3>
KFR_INTRINSIC simd<float, 4> simd_vec_shuffle(simd_t<float, 4>, const simd<float, 4>& x,
                                              csizes_t<I0, I1, I2, I3>)
{
    // SSE -> SSE
    return _mm_shuffle_ps(x, x, (shuffle_mask<8, I0, I1, I2, I3>::value));
}

template <size_t I0, size_t I1>
KFR_INTRINSIC simd<double, 2> simd_vec_shuffle(simd_t<double, 2>, const simd<double, 2>& x, csizes_t<I0, I1>)
{
    // SSE -> SSE
    return _mm_shuffle_pd(x, x, (shuffle_mask<2, I0, I1>::value));
}
#endif

template <uint8_t max>
KFR_INTRINSIC constexpr uint8_t vec_idx(size_t value)
{
    return value >= static_cast<size_t>(max) ? 0 : static_cast<uint8_t>(value);
}

#ifdef CMT_ARCH_AVX512

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7, size_t I8,
          size_t I9, size_t I10, size_t I11, size_t I12, size_t I13, size_t I14, size_t I15>
KFR_INTRINSIC simd<float, 16> simd_vec_shuffle(
    simd_t<float, 16>, const simd<float, 16>& x,
    csizes_t<I0, I1, I2, I3, I4, I5, I6, I7, I8, I9, I10, I11, I12, I13, I14, I15>)
{
    // AVX512 -> AVX512
    return _mm512_permutexvar_ps(
        _mm512_setr_epi32(vec_idx<16>(I0), vec_idx<16>(I1), vec_idx<16>(I2), vec_idx<16>(I3), vec_idx<16>(I4),
                          vec_idx<16>(I5), vec_idx<16>(I6), vec_idx<16>(I7), vec_idx<16>(I8), vec_idx<16>(I9),
                          vec_idx<16>(I10), vec_idx<16>(I11), vec_idx<16>(I12), vec_idx<16>(I13),
                          vec_idx<16>(I14), vec_idx<16>(I15)),
        x);
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7>
KFR_INTRINSIC simd<double, 8> simd_vec_shuffle(simd_t<double, 8>, const simd<double, 8>& x,
                                               csizes_t<I0, I1, I2, I3, I4, I5, I6, I7>)
{
    // AVX512 -> AVX512
    return _mm512_permutexvar_pd(_mm512_setr_epi64(vec_idx<8>(I0), vec_idx<8>(I1), vec_idx<8>(I2),
                                                   vec_idx<8>(I3), vec_idx<8>(I4), vec_idx<8>(I5),
                                                   vec_idx<8>(I6), vec_idx<8>(I7)),
                                 x);
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7>
KFR_INTRINSIC simd<float, 8> simd_vec_shuffle(simd_t<float, 16>, const simd<float, 16>& x,
                                              csizes_t<I0, I1, I2, I3, I4, I5, I6, I7>)
{
    // AVX512 -> AVX
    return _mm512_castps512_ps256(_mm512_permutexvar_ps(
        _mm512_setr_epi32(vec_idx<16>(I0), vec_idx<16>(I1), vec_idx<16>(I2), vec_idx<16>(I3), vec_idx<16>(I4),
                          vec_idx<16>(I5), vec_idx<16>(I6), vec_idx<16>(I7), vec_idx<16>(I0), vec_idx<16>(I1),
                          vec_idx<16>(I2), vec_idx<16>(I3), vec_idx<16>(I4), vec_idx<16>(I5), vec_idx<16>(I6),
                          vec_idx<16>(I7)),
        x));
}

template <size_t I0, size_t I1, size_t I2, size_t I3>
KFR_INTRINSIC simd<float, 4> simd_vec_shuffle(simd_t<float, 16>, const simd<float, 16>& x,
                                              csizes_t<I0, I1, I2, I3>)
{
    // AVX512 -> SSE
    return _mm512_castps512_ps128(_mm512_permutexvar_ps(
        _mm512_setr_epi32(vec_idx<16>(I0), vec_idx<16>(I1), vec_idx<16>(I2), vec_idx<16>(I3), vec_idx<16>(I0),
                          vec_idx<16>(I1), vec_idx<16>(I2), vec_idx<16>(I3), vec_idx<16>(I0), vec_idx<16>(I1),
                          vec_idx<16>(I2), vec_idx<16>(I3), vec_idx<16>(I0), vec_idx<16>(I1), vec_idx<16>(I2),
                          vec_idx<16>(I3)),
        x));
}

template <size_t I0, size_t I1, size_t I2, size_t I3>
KFR_INTRINSIC simd<double, 4> simd_vec_shuffle(simd_t<double, 8>, const simd<double, 8>& x,
                                               csizes_t<I0, I1, I2, I3>)
{
    // AVX512 -> AVX
    return _mm512_castpd512_pd256(_mm512_permutexvar_pd(
        _mm512_setr_epi64(vec_idx<8>(I0), vec_idx<8>(I1), vec_idx<8>(I2), vec_idx<8>(I3), vec_idx<8>(I0),
                          vec_idx<8>(I1), vec_idx<8>(I2), vec_idx<8>(I3)),
        x));
}

template <size_t I0, size_t I1>
KFR_INTRINSIC simd<double, 2> simd_vec_shuffle(simd_t<double, 8>, const simd<double, 8>& x, csizes_t<I0, I1>)
{
    // AVX512 -> SSE
    return _mm512_castpd512_pd128(_mm512_permutexvar_pd(
        _mm512_setr_epi64(vec_idx<8>(I0), vec_idx<8>(I1), vec_idx<8>(I0), vec_idx<8>(I1), vec_idx<8>(I0),
                          vec_idx<8>(I1), vec_idx<8>(I0), vec_idx<8>(I1)),
        x));
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7, size_t I8,
          size_t I9, size_t I10, size_t I11, size_t I12, size_t I13, size_t I14, size_t I15>
KFR_INTRINSIC simd<float, 16> simd_vec_shuffle(
    simd_t<float, 8>, const simd<float, 8>& x,
    csizes_t<I0, I1, I2, I3, I4, I5, I6, I7, I8, I9, I10, I11, I12, I13, I14, I15>)
{
    // AVX -> AVX512
    return _mm512_permutexvar_ps(
        _mm512_setr_epi32(vec_idx<8>(I0), vec_idx<8>(I1), vec_idx<8>(I2), vec_idx<8>(I3), vec_idx<8>(I4),
                          vec_idx<8>(I5), vec_idx<8>(I6), vec_idx<8>(I7), vec_idx<8>(I8), vec_idx<8>(I9),
                          vec_idx<8>(I10), vec_idx<8>(I11), vec_idx<8>(I12), vec_idx<8>(I13), vec_idx<8>(I14),
                          vec_idx<8>(I15)),
        _mm512_castps256_ps512(x));
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7, size_t I8,
          size_t I9, size_t I10, size_t I11, size_t I12, size_t I13, size_t I14, size_t I15>
KFR_INTRINSIC simd<float, 16> simd_vec_shuffle(
    simd_t<float, 4>, const simd<float, 4>& x,
    csizes_t<I0, I1, I2, I3, I4, I5, I6, I7, I8, I9, I10, I11, I12, I13, I14, I15>)
{
    // SSE -> AVX512
    return _mm512_permutexvar_ps(
        _mm512_setr_epi32(vec_idx<4>(I0), vec_idx<4>(I1), vec_idx<4>(I2), vec_idx<4>(I3), vec_idx<4>(I4),
                          vec_idx<4>(I5), vec_idx<4>(I6), vec_idx<4>(I7), vec_idx<4>(I8), vec_idx<4>(I9),
                          vec_idx<4>(I10), vec_idx<4>(I11), vec_idx<4>(I12), vec_idx<4>(I13), vec_idx<4>(I14),
                          vec_idx<4>(I15)),
        _mm512_castps128_ps512(x));
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7>
KFR_INTRINSIC simd<double, 8> simd_vec_shuffle(simd_t<double, 4>, const simd<double, 4>& x,
                                               csizes_t<I0, I1, I2, I3, I4, I5, I6, I7>)
{
    // AVX -> AVX512
    return _mm512_permutexvar_pd(_mm512_setr_epi64(vec_idx<4>(I0), vec_idx<4>(I1), vec_idx<4>(I2),
                                                   vec_idx<4>(I3), vec_idx<4>(I4), vec_idx<4>(I5),
                                                   vec_idx<4>(I6), vec_idx<4>(I7)),
                                 _mm512_castpd256_pd512(x));
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7>
KFR_INTRINSIC simd<double, 8> simd_vec_shuffle(simd_t<double, 2>, const simd<double, 2>& x,
                                               csizes_t<I0, I1, I2, I3, I4, I5, I6, I7>)
{
    // SSE -> AVX512
    return _mm512_permutexvar_pd(_mm512_setr_epi64(vec_idx<2>(I0), vec_idx<2>(I1), vec_idx<2>(I2),
                                                   vec_idx<2>(I3), vec_idx<2>(I4), vec_idx<2>(I5),
                                                   vec_idx<2>(I6), vec_idx<2>(I7)),
                                 _mm512_castpd128_pd512(x));
}

#endif

#ifdef CMT_ARCH_AVX

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7>
KFR_INTRINSIC simd<float, 8> simd_vec_shuffle(simd_t<float, 8>, const simd<float, 8>& x,
                                              csizes_t<I0, I1, I2, I3, I4, I5, I6, I7>)
{
    // AVX -> AVX
    if constexpr (cmaxof(csizes<I0, I1, I2, I3>) < 4 && csizes<I0, I1, I2, I3>.equal(csizes<I4, I5, I6, I7>))
    {
        const simd<float, 4> tmp = universal_shuffle(simd_t<float, 4>{}, simd_get_low(simd_t<float, 8>{}, x),
                                                     csizes<I0, I1, I2, I3>);
        return simd_from_halves(simd_t<float, 8>{}, tmp, tmp);
    }
    else if constexpr (cmaxof(csizes<I0, I1, I2, I3>) < 4 && cminof(csizes<I4, I5, I6, I7>) >= 4)
    {
        if constexpr (csizes<I0, I1, I2, I3, I4, I5, I6, I7>.equal(
                          csizes<I0, I1, I2, I3, I0 + 4, I1 + 4, I2 + 4, I3 + 4>))
        {
            return _mm256_shuffle_ps(x, x, (shuffle_mask<8, I0, I1, I2, I3>::value));
        }
        else
        {
            return simd_from_halves(simd_t<float, 8>{},
                                    universal_shuffle(simd_t<float, 4>{}, simd_get_low(simd_t<float, 8>{}, x),
                                                      csizes<I0, I1, I2, I3>),
                                    universal_shuffle(simd_t<float, 4>{},
                                                      simd_get_high(simd_t<float, 8>{}, x),
                                                      csizes<I4, I5, I6, I7>));
        }
    }
    else
    {
        const __m256 sw = _mm256_permute2f128_ps(x, x, 1); // swap lanes
        const __m256 t1 = _mm256_permutevar_ps(
            x, _mm256_setr_epi32(I0 % 4, I1 % 4, I2 % 4, I3 % 4, I4 % 4, I5 % 4, I6 % 4, I7 % 4));
        const __m256 t2 = _mm256_permutevar_ps(
            sw, _mm256_setr_epi32(I0 % 4, I1 % 4, I2 % 4, I3 % 4, I4 % 4, I5 % 4, I6 % 4, I7 % 4));
        return _mm256_blend_ps(t1, t2,
                               (shuffle_mask<8, I0 / 4, I1 / 4, I2 / 4, I3 / 4, 1 - I4 / 4, 1 - I5 / 4,
                                             1 - I6 / 4, 1 - I7 / 4>::value));
    }
}

template <size_t I0, size_t I1, size_t I2, size_t I3>
KFR_INTRINSIC simd<double, 4> simd_vec_shuffle(simd_t<double, 4>, const simd<double, 4>& x,
                                               csizes_t<I0, I1, I2, I3>)
{
    // AVX -> AVX
    if constexpr (cmaxof(csizes<I0, I1>) < 2 && csizes<I0, I1>.equal(csizes<I2, I3>))
    {
        const simd<double, 2> tmp =
            universal_shuffle(simd_t<double, 2>{}, simd_get_low(simd_t<double, 4>{}, x), csizes<I0, I1>);
        return simd_from_halves(simd_t<double, 4>{}, tmp, tmp);
    }
    else if constexpr (cmaxof(csizes<I0, I1>) < 4 && cminof(csizes<I2, I3>) >= 4)
    {
        if constexpr (csizes<I0, I1, I2, I3>.equal(csizes<I0, I1, I2 + 2, I3 + 2>))
        {
            return _mm256_shuffle_pd(x, x, (shuffle_mask<2, I0, I1>::value));
        }
        else
        {
            return simd_from_halves(
                simd_t<double, 4>{},
                universal_shuffle(simd_t<double, 2>{}, simd_get_low(simd_t<double, 4>{}, x), csizes<I0, I1>),
                universal_shuffle(simd_t<double, 2>{}, simd_get_high(simd_t<double, 4>{}, x),
                                  csizes<I2, I3>));
        }
    }
    else
    {
        const __m256d sw = _mm256_permute2f128_pd(x, x, 1); // swap lanes
        const __m256d t1 = _mm256_permutevar_pd(
            x, _mm256_setr_epi64x((I0 % 2) << 1, (I1 % 2) << 1, (I2 % 2) << 1, (I3 % 2) << 1));
        const __m256d t2 = _mm256_permutevar_pd(
            sw, _mm256_setr_epi64x((I0 % 2) << 1, (I1 % 2) << 1, (I2 % 2) << 1, (I3 % 2) << 1));
        return _mm256_blend_pd(t1, t2, (shuffle_mask<4, I0 / 2, I1 / 2, 1 - I2 / 2, 1 - I3 / 2>::value));
    }
}

template <size_t I0, size_t I1, size_t I2, size_t I3>
KFR_INTRINSIC simd<float, 4> simd_vec_shuffle(simd_t<float, 8>, const simd<float, 8>& x,
                                              csizes_t<I0, I1, I2, I3>)
{
    // AVX -> SSE
    if constexpr (I0 % 4 == 0 && I1 % 4 == 1 && I2 % 4 == 2 && I3 % 4 == 3)
    {
        __m128 t1 = simd_get_low(simd_t<float, 8>{}, x);
        __m128 t2 = simd_get_high(simd_t<float, 8>{}, x);
        return _mm_blend_ps(t1, t2, (shuffle_mask<4, I0 / 4, I1 / 4, I2 / 4, I3 / 4>::value));
    }
    else
    {
        __m128 t1 = simd_get_low(simd_t<float, 8>{}, x);
        __m128 t2 = simd_get_high(simd_t<float, 8>{}, x);
        t1        = _mm_permute_ps(t1, (shuffle_mask<8, I0 % 4, I1 % 4, I2 % 4, I3 % 4>::value));
        t2        = _mm_permute_ps(t2, (shuffle_mask<8, I0 % 4, I1 % 4, I2 % 4, I3 % 4>::value));
        return _mm_blend_ps(t1, t2, (shuffle_mask<4, I0 / 4, I1 / 4, I2 / 4, I3 / 4>::value));
    }
}

template <size_t I0, size_t I1>
KFR_INTRINSIC simd<double, 2> simd_vec_shuffle(simd_t<double, 4>, const simd<double, 4>& x, csizes_t<I0, I1>)
{
    // AVX -> SSE
    if constexpr (I0 % 2 == 0 && I1 % 2 == 1)
    {
        __m128d t1 = simd_get_low(simd_t<double, 4>{}, x);
        __m128d t2 = simd_get_high(simd_t<double, 4>{}, x);
        return _mm_blend_pd(t1, t2, (shuffle_mask<2, I0 / 2, I1 / 2>::value));
    }
    else
    {
        __m128d t1 = simd_get_low(simd_t<double, 4>{}, x);
        __m128d t2 = simd_get_high(simd_t<double, 4>{}, x);
        t1         = _mm_permute_pd(t1, (shuffle_mask<2, I0 % 2, I1 % 2>::value));
        t2         = _mm_permute_pd(t2, (shuffle_mask<2, I0 % 2, I1 % 2>::value));
        return _mm_blend_pd(t1, t2, (shuffle_mask<2, I0 / 2, I1 / 2>::value));
    }
}

template <size_t I0, size_t I1, size_t I2, size_t I3, size_t I4, size_t I5, size_t I6, size_t I7>
KFR_INTRINSIC simd<float, 8> simd_vec_shuffle(simd_t<float, 4>, const simd<float, 4>& x,
                                              csizes_t<I0, I1, I2, I3, I4, I5, I6, I7>)
{
    // SSE -> AVX
    return KFR_mm256_setr_m128(_mm_shuffle_ps(x, x, (shuffle_mask<8, I0, I1, I2, I3>::value)),
                               _mm_shuffle_ps(x, x, (shuffle_mask<8, I4, I5, I6, I7>::value)));
}

template <size_t I0, size_t I1, size_t I2, size_t I3>
KFR_INTRINSIC simd<double, 4> simd_vec_shuffle(simd_t<double, 2>, const simd<double, 2>& x,
                                               csizes_t<I0, I1, I2, I3>)
{
    // SSE -> AVX
    return KFR_mm256_setr_m128d(_mm_shuffle_pd(x, x, (shuffle_mask<2, I0, I1>::value)),
                                _mm_shuffle_pd(x, x, (shuffle_mask<2, I2, I3>::value)));
}

#endif

template <typename T, size_t Nin, size_t... indices, size_t Nout>
KFR_INTRINSIC simd<T, Nout> universal_shuffle(simd_t<T, Nin>, const simd<T, Nin>& x, csizes_t<indices...>)
{
    using Indices = csizes_t<indices...>;

    constexpr size_t minwidth = minimum_vector_width<T>;
    constexpr size_t maxwidth = vector_width<T>;
    constexpr size_t minindex = cminof(Indices{});
    constexpr size_t maxindex = cmaxof(csizes<(indices >= Nin ? 0 : indices)...>);

    if constexpr (Nin == 1 && Nout == 1)
    {
        return x;
    }
    else if constexpr (next_poweroftwo(Nin) == next_poweroftwo(Nout) && Indices{}.equal(csizeseq<Nout>))
    {
        return x;
    }
    else if constexpr (!is_poweroftwo(Nin) || !is_poweroftwo(Nout))
    {
        // Fix if not power of two
        return universal_shuffle(
            simd_t<T, next_poweroftwo(Nin)>{}, x,
            cconcat(Indices{}, csizeseq<next_poweroftwo(Nout) - Nout, index_undefined, 0>));
    }
    else if constexpr (Nout < minwidth)
    {
        // Expand indices if less than vector
        const simd<T, minwidth> tmp = universal_shuffle(
            simd_t<T, Nin>{}, x, cconcat(Indices{}, csizeseq<minwidth - Nout, index_undefined, 0>));

        if constexpr (Nout == 1)
        {
            return simd_to_scalar(simd_t<T, minwidth>{}, tmp);
        }
        else
        {
            union
            {
                simd<T, minwidth> tmp;
                simd<T, Nout> r;
            } u{ tmp };
            return u.r;
        }
    }
    else if constexpr (Nout > maxwidth)
    {
        auto lowi  = Indices{}[csizeseq<Nout / 2, 0>];
        auto highi = Indices{}[csizeseq<Nout / 2, Nout / 2>];
        if constexpr (lowi.equal(highi))
        {
            auto tmp = universal_shuffle(simd_t<T, Nin>{}, x, lowi);
            return { tmp, tmp };
        }
        else
        {
            return { universal_shuffle(simd_t<T, Nin>{}, x, lowi),
                     universal_shuffle(simd_t<T, Nin>{}, x, highi) };
        }
    }
    else if constexpr (minindex >= Nin)
    {
        return simd_from_zero(simd_t<T, Nout>{});
    }
    else if constexpr (Nin == 1)
    {
        return simd_from_broadcast(simd_t<T, Nout>{}, x);
    }
    else if constexpr (Nin < minwidth)
    {
        return universal_shuffle(simd_t<T, minwidth>{}, simd_from_partial(simd2_t<T, minwidth, Nin>{}, x),
                                 Indices{});
    }
    else if constexpr (Nin > Nout && maxindex < Nin / 2)
    {
        return universal_shuffle(simd_t<T, Nin / 2>{}, simd_get_low(simd_t<T, Nin>{}, x), Indices{});
    }
    else if constexpr (Nin > Nout && minindex >= Nin / 2)
    {
        return universal_shuffle(simd_t<T, Nin / 2>{}, simd_get_high(simd_t<T, Nin>{}, x),
                                 csizes<(indices < Nin ? indices - csize<Nin / 2> : indices)...>);
    }
    else if constexpr (Nin >= minwidth && Nin <= maxwidth && Nout >= minwidth && Nout <= maxwidth)
    {
        return simd_vec_shuffle(simd_t<T, Nin>{}, x, Indices{});
    }
    else
    {
        not_optimized(CMT_FUNC_SIGNATURE);
        const simd_array<T, Nin> xx               = to_simd_array<T, Nin>(x);
        constexpr static unsigned indices_array[] = { static_cast<unsigned>(indices)... };
        return from_simd_array<T, Nout>(simd_shuffle_generic<T, Nout, Nin>(xx, indices_array));
    }
}

} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
#endif
