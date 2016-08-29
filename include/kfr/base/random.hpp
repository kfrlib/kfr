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
#include "shuffle.hpp"
#include "vec.hpp"

namespace kfr
{

using random_state = u32x4;

struct seed_from_rdtsc_t
{
};

constexpr seed_from_rdtsc_t seed_from_rdtsc{};

struct random_bit_generator
{
    random_bit_generator(seed_from_rdtsc_t) noexcept
        : state(bitcast<u32>(make_vector(__builtin_readcyclecounter(),
                                         (__builtin_readcyclecounter() << 11) ^ 0x710686d615e2257bull)))
    {
        (void)operator()();
    }
    constexpr random_bit_generator(u32 x0, u32 x1, u32 x2, u32 x3) noexcept : state(x0, x1, x2, x3)
    {
        (void)operator()();
    }
    constexpr random_bit_generator(u64 x0, u64 x1) noexcept : state(bitcast<u32>(make_vector(x0, x1)))
    {
        (void)operator()();
    }

    inline random_state operator()()
    {
        constexpr static random_state mul{ 214013u, 17405u, 214013u, 69069u };
        constexpr static random_state add{ 2531011u, 10395331u, 13737667u, 1u };
        state = bitcast<u32>(rotateright<3>(bitcast<u8>(fmadd(state, mul, add))));
        return state;
    }

protected:
    random_state state;
};

template <size_t N, KFR_ENABLE_IF(N <= sizeof(random_state))>
inline vec<u8, N> random_bits(random_bit_generator& gen)
{
    return narrow<N>(bitcast<u8>(gen()));
}
template <size_t N, KFR_ENABLE_IF(N > sizeof(random_state))>
inline vec<u8, N> random_bits(random_bit_generator& gen)
{
    constexpr size_t N2 = prev_poweroftwo(N - 1);
    return concat(random_bits<N2>(gen), random_bits<N - N2>(gen));
}

template <typename T, size_t N, KFR_ENABLE_IF(std::is_integral<T>::value)>
inline vec<T, N> random_uniform(random_bit_generator& gen)
{
    return bitcast<T>(random_bits<N * sizeof(T)>(gen));
}

template <typename T, size_t N, KFR_ENABLE_IF(std::is_same<T, f32>::value)>
inline vec<f32, N> randommantissa(random_bit_generator& gen)
{
    return bitcast<f32>((random_uniform<u32, N>(gen) & 0x7FFFFFu) | 0x3f800000u) + 0.0f;
}

template <typename T, size_t N, KFR_ENABLE_IF(std::is_same<T, f64>::value)>
inline vec<f64, N> randommantissa(random_bit_generator& gen)
{
    return bitcast<f64>((random_uniform<u64, N>(gen) & 0x000FFFFFFFFFFFFFull) | 0x3FF0000000000000ull) + 0.0;
}

template <typename T, size_t N>
inline enable_if_f<vec<T, N>> random_uniform(random_bit_generator& gen)
{
    return randommantissa<T, N>(gen) - 1.f;
}

template <size_t N, typename T>
inline enable_if_f<vec<T, N>> random_range(random_bit_generator& gen, T min, T max)
{
    return mix(random_uniform<T, N>(gen), min, max);
}

template <size_t N, typename T>
inline enable_if_not_f<vec<T, N>> random_range(random_bit_generator& gen, T min, T max)
{
    using big_type = findinttype<sqr(std::numeric_limits<T>::min()), sqr(std::numeric_limits<T>::max())>;

    vec<T, N> u                = random_uniform<T, N>(gen);
    const vec<big_type, N> tmp = u;
    return (tmp * (max - min) + min) >> typebits<T>::bits;
}

namespace internal
{
template <typename T>
struct expression_random_uniform : input_expression
{
    using value_type = T;
    constexpr expression_random_uniform(const random_bit_generator& gen) noexcept : gen(gen) {}
    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t, vec_t<T, N>) const
    {
        return random_uniform<T, N>(gen);
    }
    mutable random_bit_generator gen;
};

template <typename T>
struct expression_random_range : input_expression
{
    using value_type = T;
    constexpr expression_random_range(const random_bit_generator& gen, T min, T max) noexcept : gen(gen),
                                                                                                min(min),
                                                                                                max(max)
    {
    }

    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t, vec_t<T, N>) const
    {
        return random_range<N, T>(gen, min, max);
    }
    mutable random_bit_generator gen;
    const T min;
    const T max;
};
}

template <typename T>
inline internal::expression_random_uniform<T> gen_random_uniform(const random_bit_generator& gen)
{
    return internal::expression_random_uniform<T>(gen);
}

template <typename T>
inline internal::expression_random_range<T> gen_random_range(const random_bit_generator& gen, T min, T max)
{
    return internal::expression_random_range<T>(gen, min, max);
}

template <typename T>
inline internal::expression_random_uniform<T> gen_random_uniform()
{
    return internal::expression_random_uniform<T>(random_bit_generator(seed_from_rdtsc));
}

template <typename T>
inline internal::expression_random_range<T> gen_random_range(T min, T max)
{
    return internal::expression_random_range<T>(random_bit_generator(seed_from_rdtsc), min, max);
}
}
