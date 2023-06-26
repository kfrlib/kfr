/** @addtogroup read_write
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

#include "../shuffle.hpp"
#include "../types.hpp"
#include "../vec.hpp"
#include "function.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
namespace intrinsics
{

#ifndef CMT_CLANG_EXT

#ifdef CMT_ARCH_SSE2

template <typename T>
KFR_INTRINSIC vec<T, 1> read(cunaligned_t, csize_t<1>, const T* ptr)
{
    return *ptr;
}

KFR_INTRINSIC f32x2 read(cunaligned_t, csize_t<2>, const f32* ptr)
{
    return f32x2::simd_type{ ptr[0], ptr[1] };
}

#if !defined(CMT_COMPILER_GCC)

KFR_INTRINSIC u8x2 read(cunaligned_t, csize_t<2>, const u8* ptr)
{
    return u8x2::simd_type::from(*reinterpret_cast<const u16*>(ptr));
}
KFR_INTRINSIC i8x2 read(cunaligned_t, csize_t<2>, const i8* ptr)
{
    return i8x2::simd_type::from(*reinterpret_cast<const u16*>(ptr));
}
KFR_INTRINSIC u8x4 read(cunaligned_t, csize_t<4>, const u8* ptr)
{
    return u8x4::simd_type::from(*reinterpret_cast<const u32*>(ptr));
}
KFR_INTRINSIC i8x4 read(cunaligned_t, csize_t<4>, const i8* ptr)
{
    return i8x4::simd_type::from(*reinterpret_cast<const u32*>(ptr));
}
KFR_INTRINSIC u16x2 read(cunaligned_t, csize_t<2>, const u16* ptr)
{
    return u16x2::simd_type::from(*reinterpret_cast<const u32*>(ptr));
}
KFR_INTRINSIC i16x2 read(cunaligned_t, csize_t<2>, const i16* ptr)
{
    return i16x2::simd_type::from(*reinterpret_cast<const u32*>(ptr));
}
KFR_INTRINSIC u8x8 read(cunaligned_t, csize_t<8>, const u8* ptr)
{
    return u8x8::simd_type::from(*reinterpret_cast<const u64*>(ptr));
}
KFR_INTRINSIC i8x8 read(cunaligned_t, csize_t<8>, const i8* ptr)
{
    return i8x8::simd_type::from(*reinterpret_cast<const u64*>(ptr));
}
KFR_INTRINSIC u16x4 read(cunaligned_t, csize_t<4>, const u16* ptr)
{
    return u16x4::simd_type::from(*reinterpret_cast<const u64*>(ptr));
}
KFR_INTRINSIC i16x4 read(cunaligned_t, csize_t<4>, const i16* ptr)
{
    return i16x4::simd_type::from(*reinterpret_cast<const u64*>(ptr));
}
KFR_INTRINSIC u32x2 read(cunaligned_t, csize_t<2>, const u32* ptr)
{
    return u32x2::simd_type::from(*reinterpret_cast<const u64*>(ptr));
}
KFR_INTRINSIC i32x2 read(cunaligned_t, csize_t<2>, const i32* ptr)
{
    return i32x2::simd_type::from(*reinterpret_cast<const u64*>(ptr));
}

#endif

KFR_INTRINSIC f32sse read(cunaligned_t, csize_t<4>, const f32* ptr) { return _mm_loadu_ps(ptr); }
KFR_INTRINSIC f64sse read(cunaligned_t, csize_t<2>, const f64* ptr) { return _mm_loadu_pd(ptr); }
KFR_INTRINSIC u8sse read(cunaligned_t, csize_t<16>, const u8* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC i8sse read(cunaligned_t, csize_t<16>, const i8* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC u16sse read(cunaligned_t, csize_t<8>, const u16* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC i16sse read(cunaligned_t, csize_t<8>, const i16* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC u32sse read(cunaligned_t, csize_t<4>, const u32* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC i32sse read(cunaligned_t, csize_t<4>, const i32* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC u64sse read(cunaligned_t, csize_t<2>, const u64* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}
KFR_INTRINSIC i64sse read(cunaligned_t, csize_t<2>, const i64* ptr)
{
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
}

template <typename T>
KFR_INTRINSIC void write(cunaligned_t, T* ptr, const vec<T, 1>& x)
{
    *ptr = x.front();
}
KFR_INTRINSIC void write(cunaligned_t, f32* ptr, const f32x2& x)
{
#ifndef KFR_f32x2_array
    *reinterpret_cast<f64*>(ptr) = x.v.whole;
#else
    ptr[0] = x.v.low;
    ptr[1] = x.v.high;
#endif
}

KFR_INTRINSIC void write(cunaligned_t, u8* ptr, const u8x2& x) { *reinterpret_cast<u16*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, i8* ptr, const i8x2& x) { *reinterpret_cast<u16*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, u8* ptr, const u8x4& x) { *reinterpret_cast<u32*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, i8* ptr, const i8x4& x) { *reinterpret_cast<u32*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, u16* ptr, const u16x2& x) { *reinterpret_cast<u32*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, i16* ptr, const i16x2& x) { *reinterpret_cast<u32*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, u8* ptr, const u8x8& x) { *reinterpret_cast<u64*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, i8* ptr, const i8x8& x) { *reinterpret_cast<u64*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, u16* ptr, const u16x4& x) { *reinterpret_cast<u64*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, i16* ptr, const i16x4& x) { *reinterpret_cast<u64*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, u32* ptr, const u32x2& x) { *reinterpret_cast<u64*>(ptr) = x.v.whole; }
KFR_INTRINSIC void write(cunaligned_t, i32* ptr, const i32x2& x) { *reinterpret_cast<u64*>(ptr) = x.v.whole; }

KFR_INTRINSIC void write(cunaligned_t, f32* ptr, const f32sse& x) { _mm_storeu_ps(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, f64* ptr, const f64sse& x) { _mm_storeu_pd(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, u8* ptr, const u8sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i8* ptr, const i8sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, u16* ptr, const u16sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i16* ptr, const i16sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, u32* ptr, const u32sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i32* ptr, const i32sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, u64* ptr, const u64sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i64* ptr, const i64sse& x)
{
    _mm_storeu_si128(reinterpret_cast<__m128i*>(ptr), x.v);
}

#if defined CMT_ARCH_AVX

KFR_INTRINSIC f32avx read(cunaligned_t, csize_t<8>, const f32* ptr) { return _mm256_loadu_ps(ptr); }
KFR_INTRINSIC f64avx read(cunaligned_t, csize_t<4>, const f64* ptr) { return _mm256_loadu_pd(ptr); }

KFR_INTRINSIC void write(cunaligned_t, f32* ptr, const f32avx& x) { _mm256_storeu_ps(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, f64* ptr, const f64avx& x) { _mm256_storeu_pd(ptr, x.v); }

#if defined CMT_ARCH_AVX2

KFR_INTRINSIC u8avx read(cunaligned_t, csize_t<32>, const u8* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC i8avx read(cunaligned_t, csize_t<32>, const i8* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC u16avx read(cunaligned_t, csize_t<16>, const u16* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC i16avx read(cunaligned_t, csize_t<16>, const i16* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC u32avx read(cunaligned_t, csize_t<8>, const u32* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC i32avx read(cunaligned_t, csize_t<8>, const i32* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC u64avx read(cunaligned_t, csize_t<4>, const u64* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}
KFR_INTRINSIC i64avx read(cunaligned_t, csize_t<4>, const i64* ptr)
{
    return _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
}

KFR_INTRINSIC void write(cunaligned_t, u8* ptr, const u8avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i8* ptr, const i8avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, u16* ptr, const u16avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i16* ptr, const i16avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, u32* ptr, const u32avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i32* ptr, const i32avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, u64* ptr, const u64avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}
KFR_INTRINSIC void write(cunaligned_t, i64* ptr, const i64avx& x)
{
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(ptr), x.v);
}

#if defined CMT_ARCH_AVX512

KFR_INTRINSIC f32avx512 read(cunaligned_t, csize_t<16>, const f32* ptr) { return _mm512_loadu_ps(ptr); }
KFR_INTRINSIC f64avx512 read(cunaligned_t, csize_t<8>, const f64* ptr) { return _mm512_loadu_pd(ptr); }

KFR_INTRINSIC u8avx512 read(cunaligned_t, csize_t<64>, const u8* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC i8avx512 read(cunaligned_t, csize_t<64>, const i8* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC u16avx512 read(cunaligned_t, csize_t<32>, const u16* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC i16avx512 read(cunaligned_t, csize_t<32>, const i16* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC u32avx512 read(cunaligned_t, csize_t<16>, const u32* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC i32avx512 read(cunaligned_t, csize_t<16>, const i32* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC u64avx512 read(cunaligned_t, csize_t<8>, const u64* ptr) { return _mm512_loadu_si512(ptr); }
KFR_INTRINSIC i64avx512 read(cunaligned_t, csize_t<8>, const i64* ptr) { return _mm512_loadu_si512(ptr); }

KFR_INTRINSIC void write(cunaligned_t, f32* ptr, const f32avx512& x) { _mm512_storeu_ps(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, f64* ptr, const f64avx512& x) { _mm512_storeu_pd(ptr, x.v); }

KFR_INTRINSIC void write(cunaligned_t, u8* ptr, const u8avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, i8* ptr, const i8avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, u16* ptr, const u16avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, i16* ptr, const i16avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, u32* ptr, const u32avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, i32* ptr, const i32avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, u64* ptr, const u64avx512& x) { _mm512_storeu_si512(ptr, x.v); }
KFR_INTRINSIC void write(cunaligned_t, i64* ptr, const i64avx512& x) { _mm512_storeu_si512(ptr, x.v); }

#endif
#endif
#endif
#else

// fallback

template <size_t N, typename T, KFR_ENABLE_IF(N == 1 || is_simd_size<T>(N))>
KFR_INTRINSIC vec<T, N> read(cunaligned_t, csize_t<N>, const T* ptr) CMT_NOEXCEPT
{
    vec<T, N> result{};
    for (size_t i = 0; i < N; i++)
        result[i] = ptr[i];
    return result;
}

template <size_t N, typename T, KFR_ENABLE_IF(N == 1 || is_simd_size<T>(N))>
KFR_INTRINSIC void write(cunaligned_t, T* ptr, const vec<T, N>& x) CMT_NOEXCEPT
{
    for (size_t i = 0; i < N; i++)
        ptr[i] = x[i];
}

#endif

template <size_t N, typename T, KFR_ENABLE_IF(N != 1 && !is_simd_size<T>(N)),
          size_t Nlow = prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec<T, N> read(cunaligned_t, csize_t<N>, const T* ptr) CMT_NOEXCEPT
{
    return concat(read(cunaligned, csize<Nlow>, ptr), read(cunaligned, csize<N - Nlow>, ptr + Nlow));
}

template <size_t N, typename T, KFR_ENABLE_IF(N != 1 && !is_simd_size<T>(N)),
          size_t Nlow = prev_poweroftwo(N - 1)>
KFR_INTRINSIC void write(cunaligned_t, T* ptr, const vec<T, N>& x) CMT_NOEXCEPT
{
    write(cunaligned, ptr, x.shuffle(csizeseq<Nlow>));
    write(cunaligned, ptr + Nlow, x.shuffle(csizeseq<N - Nlow, Nlow>));
}

#else

template <size_t N, typename T>
KFR_INTRINSIC simd<T, N> simd_read(const T* src) CMT_NOEXCEPT
{
    return reinterpret_cast<typename simd_storage<T, N, false>::const_pointer>(src)->value;
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
KFR_INTRINSIC vec<T, N> read(cunaligned_t, csize_t<N>, const T* src) CMT_NOEXCEPT
{
    // Clang requires a separate function returning vector (simd).
    // Direct returning vec causes aligned read instruction
    return simd_read<N>(src);
}

template <size_t N, bool A = false, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)), typename = void>
KFR_INTRINSIC vec<T, N> read(cunaligned_t, csize_t<N>, const T* src) CMT_NOEXCEPT
{
    constexpr size_t first = prev_poweroftwo(N);
    return concat(read(cunaligned, csize<first>, src), read(cunaligned, csize<N - first>, src + first));
}

template <bool A = false, size_t N, typename T, KFR_ENABLE_IF(is_poweroftwo(N))>
KFR_INTRINSIC void write(cunaligned_t, T* dest, const vec<T, N>& x) CMT_NOEXCEPT
{
    reinterpret_cast<typename simd_storage<T, N, A>::pointer>(dest)->value = x.v;
}

template <bool A      = false, size_t N, typename T, KFR_ENABLE_IF(!is_poweroftwo(N)),
          size_t Nlow = prev_poweroftwo(N - 1)>
KFR_INTRINSIC void write(cunaligned_t, T* dest, const vec<T, N>& x) CMT_NOEXCEPT
{
    write(cunaligned, dest, x.shuffle(csizeseq<Nlow>));
    write(cunaligned, dest + Nlow, x.shuffle(csizeseq<N - Nlow, Nlow>));
}

#endif

template <size_t N, typename T>
KFR_INTRINSIC vec<T, N> read(caligned_t, csize_t<N>, const T* __restrict ptr) CMT_NOEXCEPT
{
    return *reinterpret_cast<const typename vec<T, N>::simd_type*>(ptr);
}

template <size_t N, typename T>
KFR_INTRINSIC void write(caligned_t, T* __restrict ptr, const vec<T, N>& __restrict x) CMT_NOEXCEPT
{
    *reinterpret_cast<typename vec<T, N>::simd_type*>(ptr) = x.v;
}

} // namespace intrinsics

} // namespace CMT_ARCH_NAME
} // namespace kfr
