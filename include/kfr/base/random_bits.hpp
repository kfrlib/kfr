/** @addtogroup random
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

#include "../simd/impl/function.hpp"
#include "../simd/operators.hpp"
#include "../simd/shuffle.hpp"
#include "../simd/vec.hpp"
#include "expression.hpp"
#include <functional>

#ifdef CMT_ARCH_ARM
#define KFR_DISABLE_READCYCLECOUNTER
#endif

namespace kfr
{

#ifndef KFR_DISABLE_READCYCLECOUNTER
struct seed_from_rdtsc_t
{
};

constexpr seed_from_rdtsc_t seed_from_rdtsc{};
#endif

struct random_state
{
    constexpr random_state() : v{ 0, 0, 0, 0 } {}
    constexpr random_state(random_state&&)                 = default;
    constexpr random_state(const random_state&)            = default;
    constexpr random_state& operator=(random_state&&)      = default;
    constexpr random_state& operator=(const random_state&) = default;
    // internal field
    portable_vec<u32, 4> v;
};

#ifndef KFR_DISABLE_READCYCLECOUNTER
#ifdef CMT_COMPILER_CLANG
#define KFR_builtin_readcyclecounter()                                                                       \
    static_cast<u64>(__builtin_readcyclecounter()) // Intel C++ requires cast here
#else
#define KFR_builtin_readcyclecounter() static_cast<u64>(__rdtsc())
#endif
#endif

static_assert(sizeof(random_state) == 16, "sizeof(random_state) == 16");

inline namespace CMT_ARCH_NAME
{

KFR_INTRINSIC void random_next(random_state& state)
{
    constexpr static portable_vec<u32, 4> mul{ 214013u, 17405u, 214013u, 69069u };
    constexpr static portable_vec<u32, 4> add{ 2531011u, 10395331u, 13737667u, 1u };
    state.v = bitcast<u32>(rotateright<3>(
        bitcast<u8>(fmadd(static_cast<u32x4>(state.v), static_cast<u32x4>(mul), static_cast<u32x4>(add)))));
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
KFR_INTRINSIC random_state random_init()
{
    random_state state;
    state.v = portable_vec<u32, 4>{ bitcast<u32>(make_vector(
        KFR_builtin_readcyclecounter(), (KFR_builtin_readcyclecounter() << 11) ^ 0x710686d615e2257bull)) };
    random_next(state);
    return state;
}
#endif

KFR_INTRINSIC random_state random_init(u32 x0, u32 x1, u32 x2, u32 x3)
{
    random_state state;
    state.v = portable_vec<u32, 4>{ x0, x1, x2, x3 };
    random_next(state);
    return state;
}

KFR_INTRINSIC random_state random_init(u64 x0, u64 x1)
{
    random_state state;
    state.v = portable_vec<u32, 4>{ static_cast<u32>(x0), static_cast<u32>(x0 >> 32), static_cast<u32>(x1),
                                    static_cast<u32>(x1 >> 32) };
    random_next(state);
    return state;
}

template <size_t N, KFR_ENABLE_IF(N <= sizeof(random_state))>
KFR_INTRINSIC vec<u8, N> random_bits(random_state& state)
{
    random_next(state);
    return narrow<N>(bitcast<u8>(u32x4(state.v)));
}

template <size_t N, KFR_ENABLE_IF(N > sizeof(random_state))>
KFR_INTRINSIC vec<u8, N> random_bits(random_state& state)
{
    constexpr size_t N2         = prev_poweroftwo(N - 1);
    const vec<u8, N2> bits1     = random_bits<N2>(state);
    const vec<u8, N - N2> bits2 = random_bits<N - N2>(state);
    return concat(bits1, bits2);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
