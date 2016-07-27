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
#include "operators.hpp"
#include "select.hpp"

namespace kfr
{

namespace intrinsics
{
// floating point
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(vec<T, N> x)
{
    return x & internal::invhighbitmask<T>;
}

#if defined CID_ARCH_SSSE3

KFR_SINTRIN i64sse abs(i64sse x) { return select(x >= 0, x, -x); }
KFR_SINTRIN i32sse abs(i32sse x) { return _mm_abs_epi32(*x); }
KFR_SINTRIN i16sse abs(i16sse x) { return _mm_abs_epi16(*x); }
KFR_SINTRIN i8sse abs(i8sse x) { return _mm_abs_epi8(*x); }
KFR_SINTRIN u64sse abs(u64sse x) { return x; }
KFR_SINTRIN u32sse abs(u32sse x) { return x; }
KFR_SINTRIN u16sse abs(u16sse x) { return x; }
KFR_SINTRIN u8sse abs(u8sse x) { return x; }

#if defined CID_ARCH_AVX2
KFR_SINTRIN i64avx abs(i64avx x) { return select(x >= 0, x, -x); }
KFR_SINTRIN i32avx abs(i32avx x) { return _mm256_abs_epi32(*x); }
KFR_SINTRIN i16avx abs(i16avx x) { return _mm256_abs_epi16(*x); }
KFR_SINTRIN i8avx abs(i8avx x) { return _mm256_abs_epi8(*x); }
KFR_SINTRIN u64avx abs(u64avx x) { return x; }
KFR_SINTRIN u32avx abs(u32avx x) { return x; }
KFR_SINTRIN u16avx abs(u16avx x) { return x; }
KFR_SINTRIN u8avx abs(u8avx x) { return x; }
#endif

KFR_HANDLE_ALL_SIZES_NOT_F_1(abs)

#else

// fallback
template <typename T, size_t N, KFR_ENABLE_IF(!is_f_class<T>::value)>
KFR_SINTRIN vec<T, N> abs(vec<T, N> x)
{
    return select(x >= T(), x, -x);
}
#endif
KFR_I_CONVERTER(abs)
}

KFR_I_FN(abs)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN T1 abs(const T1& x)
{
    return intrinsics::abs(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn::abs, E1> abs(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}
