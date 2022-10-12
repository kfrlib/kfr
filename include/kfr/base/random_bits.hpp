/** @addtogroup random
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

inline namespace CMT_ARCH_NAME
{

using random_state = u32x4;

#ifndef KFR_DISABLE_READCYCLECOUNTER
#ifdef CMT_COMPILER_CLANG
#define KFR_builtin_readcyclecounter()                                                                       \
    static_cast<u64>(__builtin_readcyclecounter()) // Intel C++ requires cast here
#else
#define KFR_builtin_readcyclecounter() static_cast<u64>(__rdtsc())
#endif
#endif

struct random_bit_generator
{
#ifndef KFR_DISABLE_READCYCLECOUNTER
    KFR_MEM_INTRINSIC random_bit_generator(seed_from_rdtsc_t) CMT_NOEXCEPT
        : state(bitcast<u32>(make_vector(KFR_builtin_readcyclecounter(),
                                         (KFR_builtin_readcyclecounter() << 11) ^ 0x710686d615e2257bull)))
    {
        (void)operator()();
    }
#endif
    KFR_MEM_INTRINSIC random_bit_generator(u32 x0, u32 x1, u32 x2, u32 x3) CMT_NOEXCEPT
        : state(x0, x1, x2, x3)
    {
        (void)operator()();
    }
    KFR_MEM_INTRINSIC random_bit_generator(u64 x0, u64 x1) CMT_NOEXCEPT
        : state(bitcast<u32>(make_vector(x0, x1)))
    {
        (void)operator()();
    }

    KFR_MEM_INTRINSIC random_state operator()()
    {
        const static random_state mul{ 214013u, 17405u, 214013u, 69069u };
        const static random_state add{ 2531011u, 10395331u, 13737667u, 1u };
        state = bitcast<u32>(rotateright<3>(bitcast<u8>(fmadd(state, mul, add))));
        return state;
    }

protected:
    random_state state;
};

static_assert(sizeof(random_state) == 16, "sizeof(random_state) == 16");

template <size_t N, KFR_ENABLE_IF(N <= sizeof(random_state))>
KFR_INTRINSIC vec<u8, N> random_bits(random_bit_generator& gen)
{
    return narrow<N>(bitcast<u8>(gen()));
}

template <size_t N, KFR_ENABLE_IF(N > sizeof(random_state))>
KFR_INTRINSIC vec<u8, N> random_bits(random_bit_generator& gen)
{
    constexpr size_t N2         = prev_poweroftwo(N - 1);
    const vec<u8, N2> bits1     = random_bits<N2>(gen);
    const vec<u8, N - N2> bits2 = random_bits<N - N2>(gen);
    return concat(bits1, bits2);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
