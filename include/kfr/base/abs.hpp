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

#include "function.hpp"
#include "operators.hpp"
#include "select.hpp"

namespace kfr
{

namespace intrinsics
{

#if defined CMT_ARCH_SSSE3

// floating point
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(const vec<T, N>& x)
{
    return x & internal::invhighbitmask<T>;
}

KFR_SINTRIN i64sse abs(const i64sse& x) { return select(x >= 0, x, -x); }
KFR_SINTRIN i32sse abs(const i32sse& x) { return _mm_abs_epi32(*x); }
KFR_SINTRIN i16sse abs(const i16sse& x) { return _mm_abs_epi16(*x); }
KFR_SINTRIN i8sse abs(const i8sse& x) { return _mm_abs_epi8(*x); }
KFR_SINTRIN u64sse abs(const u64sse& x) { return x; }
KFR_SINTRIN u32sse abs(const u32sse& x) { return x; }
KFR_SINTRIN u16sse abs(const u16sse& x) { return x; }
KFR_SINTRIN u8sse abs(const u8sse& x) { return x; }

#if defined CMT_ARCH_AVX2
KFR_SINTRIN i64avx abs(const i64avx& x) { return select(x >= 0, x, -x); }
KFR_SINTRIN i32avx abs(const i32avx& x) { return _mm256_abs_epi32(*x); }
KFR_SINTRIN i16avx abs(const i16avx& x) { return _mm256_abs_epi16(*x); }
KFR_SINTRIN i8avx abs(const i8avx& x) { return _mm256_abs_epi8(*x); }
KFR_SINTRIN u64avx abs(const u64avx& x) { return x; }
KFR_SINTRIN u32avx abs(const u32avx& x) { return x; }
KFR_SINTRIN u16avx abs(const u16avx& x) { return x; }
KFR_SINTRIN u8avx abs(const u8avx& x) { return x; }
#endif

KFR_HANDLE_ALL_SIZES_NOT_F_1(abs)

#elif defined CMT_ARCH_NEON

KFR_SINTRIN i8neon abs(const i8neon& x) { return vabsq_s8(*x); }
KFR_SINTRIN i16neon abs(const i16neon& x) { return vabsq_s16(*x); }
KFR_SINTRIN i32neon abs(const i32neon& x) { return vabsq_s32(*x); }
#if defined CMT_ARCH_NEON64
KFR_SINTRIN i64neon abs(const i64neon& x) { return vabsq_s64(*x); }
#else
KFR_SINTRIN i64neon abs(const i64neon& x) { return select(x >= 0, x, -x); }
#endif

KFR_SINTRIN u8neon abs(const u8neon& x) { return x; }
KFR_SINTRIN u16neon abs(const u16neon& x) { return x; }
KFR_SINTRIN u32neon abs(const u32neon& x) { return x; }
KFR_SINTRIN u64neon abs(const u64neon& x) { return x; }

KFR_SINTRIN f32neon abs(const f32neon& x) { return vabsq_f32(*x); }
#if defined CMT_ARCH_NEON64
KFR_SINTRIN f64neon abs(const f64neon& x) { return vabsq_f64(*x); }
#else
KFR_SINTRIN f64neon abs(const f64neon& x) { return x & internal::invhighbitmask<f64>; }
#endif

KFR_HANDLE_ALL_SIZES_1(abs)

#else

// floating point
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(const vec<T, N>& x)
{
    return x & internal::invhighbitmask<T>;
}

// fallback
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(const vec<T, N>& x)
{
    return select(x >= T(), x, -x);
}
#endif
KFR_I_CONVERTER(abs)
}

KFR_I_FN(abs)
/**
 * @brief Returns the absolute value of x.
 */
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 abs(const T1& x)
{
    return intrinsics::abs(x);
}

/**
 * @brief Returns template expression that returns the absolute value of x.
 */
template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN internal::expression_function<fn::abs, E1> abs(E1&& x)
{
    return { fn::abs(), std::forward<E1>(x) };
}
}
