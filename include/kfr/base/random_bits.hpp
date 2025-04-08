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
/**
 * @brief Advances the internal state of the pseudo-random number generator.
 *
 * This function uses a SIMD-optimized linear congruential method with distinct
 * multiplier and adder constants for each 32-bit lane. The resulting values are
 * further randomized using a byte-wise rotate operation.
 *
 * @param state Reference to the internal random number generator state to update.
 */
KFR_INTRINSIC void random_next(random_state& state)
{
    constexpr static portable_vec<u32, 4> mul{ 214013u, 17405u, 214013u, 69069u };
    constexpr static portable_vec<u32, 4> add{ 2531011u, 10395331u, 13737667u, 1u };
    state.v = bitcast<u32>(rotateright<3>(
        bitcast<u8>(fmadd(static_cast<u32x4>(state.v), static_cast<u32x4>(mul), static_cast<u32x4>(add)))));
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/**
 * @brief Initializes the random number generator state using the CPU cycle counter.
 *
 * This variant seeds the generator using the result of `KFR_builtin_readcyclecounter()`,
 * ensuring variability across executions. It is only available when
 * `KFR_DISABLE_READCYCLECOUNTER` is not defined.
 *
 * @return A new random_state initialized with cycle counter entropy.
 */
KFR_INTRINSIC random_state random_init()
{
    random_state state;
    state.v = portable_vec<u32, 4>{ bitcast<u32>(make_vector(
        KFR_builtin_readcyclecounter(), (KFR_builtin_readcyclecounter() << 11) ^ 0x710686d615e2257bull)) };
    random_next(state);
    return state;
}
#endif

/**
 * @brief Initializes the random number generator with four 32-bit seed values.
 *
 * This overload allows precise seeding of the internal 128-bit state using four
 * 32-bit unsigned integers.
 *
 * @param x0 First 32-bit seed value.
 * @param x1 Second 32-bit seed value.
 * @param x2 Third 32-bit seed value.
 * @param x3 Fourth 32-bit seed value.
 * @return A new random_state initialized with the provided seeds.
 */
KFR_INTRINSIC random_state random_init(u32 x0, u32 x1, u32 x2, u32 x3)
{
    random_state state;
    state.v = portable_vec<u32, 4>{ x0, x1, x2, x3 };
    random_next(state);
    return state;
}

/**
 * @brief Initializes the random number generator with two 64-bit seed values.
 *
 * This overload combines two 64-bit unsigned integers into four 32-bit lanes
 * for initializing the internal 128-bit state.
 *
 * @param x0 First 64-bit seed value.
 * @param x1 Second 64-bit seed value.
 * @return A new random_state initialized with the provided seeds.
 */
KFR_INTRINSIC random_state random_init(u64 x0, u64 x1)
{
    random_state state;
    state.v = portable_vec<u32, 4>{ static_cast<u32>(x0), static_cast<u32>(x0 >> 32), static_cast<u32>(x1),
                                    static_cast<u32>(x1 >> 32) };
    random_next(state);
    return state;
}

/**
 * @brief Generates up to 16 bytes of random data.
 *
 * For output sizes less than or equal to 16 bytes (128 bits), this function returns
 * a random byte vector generated from a single call to `random_next`.
 *
 * @tparam N Number of random bytes to generate (N <= 16).
 * @param state Reference to the random number generator state.
 * @return A vector of N random bytes.
 *
 * @note This generator is stateless beyond its internal 128-bit state. It holds
 * no internal buffer, so **each call to `random_bits` advances the state** and
 * generates **at least 128 bits** of random data. To maintain deterministic
 * behavior across runs, always request the same size `N` in each usage scenario.
 */
template <size_t N, KFR_ENABLE_IF(N <= sizeof(random_state))>
KFR_INTRINSIC vec<u8, N> random_bits(random_state& state)
{
    random_next(state);
    return narrow<N>(bitcast<u8>(u32x4(state.v)));
}

/**
 * @brief Generates more than 16 bytes of random data.
 *
 * For output sizes greater than 16 bytes, this function recursively combines smaller
 * calls to `random_bits` to generate the requested number of random bytes.
 *
 * @tparam N Number of random bytes to generate (N > 16).
 * @param state Reference to the random number generator state.
 * @return A vector of N random bytes.
 *
 * @note This generator is stateless beyond its internal 128-bit state. It holds
 * no internal buffer, so **each call to `random_bits` advances the state** and
 * generates **at least 128 bits** of random data. To maintain deterministic
 * behavior across runs, always request the same size `N` in each usage scenario.
 */
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
