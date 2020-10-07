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

template <typename T, size_t N, KFR_ENABLE_IF(is_integral<T>)>
KFR_INTRINSIC vec<T, N> random_uniform(random_bit_generator& gen)
{
    return bitcast<T>(random_bits<N * sizeof(T)>(gen));
}

template <typename T, size_t N, KFR_ENABLE_IF(is_same<T, f32>)>
KFR_INTRINSIC vec<f32, N> randommantissa(random_bit_generator& gen)
{
    return bitcast<f32>((random_uniform<u32, N>(gen) & u32(0x7FFFFFu)) | u32(0x3f800000u)) + 0.0f;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_same<T, f64>)>
KFR_INTRINSIC vec<f64, N> randommantissa(random_bit_generator& gen)
{
    return bitcast<f64>((random_uniform<u64, N>(gen) & u64(0x000FFFFFFFFFFFFFull)) |
                        u64(0x3FF0000000000000ull)) +
           0.0;
}

template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> random_uniform(random_bit_generator& gen)
{
    return randommantissa<T, N>(gen) - 1.f;
}

template <size_t N, typename T, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> random_range(random_bit_generator& gen, T min, T max)
{
    return mix(random_uniform<T, N>(gen), min, max);
}

template <size_t N, typename T, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> random_range(random_bit_generator& gen, T min, T max)
{
    using big_type = findinttype<sqr(std::numeric_limits<T>::min()), sqr(std::numeric_limits<T>::max())>;

    vec<T, N> u                = random_uniform<T, N>(gen);
    const vec<big_type, N> tmp = u;
    return (tmp * (max - min) + min) >> typebits<T>::bits;
}

namespace internal
{
template <typename T, typename Gen = random_bit_generator>
struct expression_random_uniform : input_expression
{
    using value_type = T;
    constexpr expression_random_uniform(Gen gen) CMT_NOEXCEPT : gen(gen) {}
    template <size_t N>
    friend vec<T, N> get_elements(const expression_random_uniform& self, cinput_t, size_t, vec_shape<T, N>)
    {
        return random_uniform<T, N>(self.gen);
    }
    mutable Gen gen;
};

template <typename T, typename Gen = random_bit_generator>
struct expression_random_range : input_expression
{
    using value_type = T;
    constexpr expression_random_range(Gen gen, T min, T max) CMT_NOEXCEPT : gen(gen), min(min), max(max) {}

    template <size_t N>
    friend vec<T, N> get_elements(const expression_random_range& self, cinput_t, size_t, vec_shape<T, N>)
    {
        return random_range<N, T>(self.gen, self.min, self.max);
    }
    mutable Gen gen;
    const T min;
    const T max;
};
} // namespace internal

/// @brief Returns expression that returns pseudo random values. Copies the given generator
template <typename T>
KFR_FUNCTION internal::expression_random_uniform<T> gen_random_uniform(const random_bit_generator& gen)
{
    return internal::expression_random_uniform<T>(gen);
}

/// @brief Returns expression that returns pseudo random values. References the given
/// generator. Use std::ref(gen) to force this overload
template <typename T>
KFR_FUNCTION internal::expression_random_uniform<T, std::reference_wrapper<random_bit_generator>>
gen_random_uniform(std::reference_wrapper<random_bit_generator> gen)
{
    return internal::expression_random_uniform<T, std::reference_wrapper<random_bit_generator>>(gen);
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/// @brief Returns expression that returns pseudo random values
template <typename T>
KFR_FUNCTION internal::expression_random_uniform<T> gen_random_uniform()
{
    return internal::expression_random_uniform<T>(random_bit_generator(seed_from_rdtsc));
}
#endif

/// @brief Returns expression that returns pseudo random values of the given range. Copies the given generator
template <typename T>
KFR_FUNCTION internal::expression_random_range<T> gen_random_range(const random_bit_generator& gen, T min,
                                                                   T max)
{
    return internal::expression_random_range<T>(gen, min, max);
}

/// @brief Returns expression that returns pseudo random values of the given range. References the given
/// generator. Use std::ref(gen) to force this overload
template <typename T>
KFR_FUNCTION internal::expression_random_range<T, std::reference_wrapper<random_bit_generator>>
gen_random_range(std::reference_wrapper<random_bit_generator> gen, T min, T max)
{
    return internal::expression_random_range<T, std::reference_wrapper<random_bit_generator>>(gen, min, max);
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/// @brief Returns expression that returns pseudo random values of the given range
template <typename T>
KFR_FUNCTION internal::expression_random_range<T> gen_random_range(T min, T max)
{
    return internal::expression_random_range<T>(random_bit_generator(seed_from_rdtsc), min, max);
}
#endif
} // namespace CMT_ARCH_NAME
} // namespace kfr
