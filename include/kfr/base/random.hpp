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

#include "../math/log_exp.hpp"
#include "../math/sin_cos.hpp"
#include "../math/sqrt.hpp"
#include "random_bits.hpp"
#include "state_holder.hpp"

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

template <typename T, size_t N, KFR_ENABLE_IF(std::is_integral_v<T>)>
KFR_INTRINSIC vec<T, N> random_uniform(random_state& state)
{
    return bitcast<T>(random_bits<N * sizeof(T)>(state));
}

template <typename T, size_t N, KFR_ENABLE_IF(std::is_same_v<T, f32>)>
KFR_INTRINSIC vec<f32, N> randommantissa(random_state& state)
{
    return bitcast<f32>((random_uniform<u32, N>(state) & u32(0x7FFFFFu)) | u32(0x3f800000u)) + 0.0f;
}

template <typename T, size_t N, KFR_ENABLE_IF(std::is_same_v<T, f64>)>
KFR_INTRINSIC vec<f64, N> randommantissa(random_state& state)
{
    return bitcast<f64>((random_uniform<u64, N>(state) & u64(0x000FFFFFFFFFFFFFull)) |
                        u64(0x3FF0000000000000ull)) +
           0.0;
}

/**
 * @brief Generates a vector of uniformly distributed floating-point numbers in [0.0, 1.0).
 *
 * This is derived by generating mantissas in [1.0, 2.0) and subtracting 1.0.
 *
 * @tparam T Floating-point type (f32 or f64).
 * @tparam N Number of random floats to generate.
 * @param state Reference to the random number generator state.
 * @return Vector of N floats in the range [0.0, 1.0).
 */
template <typename T, size_t N, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> random_uniform(random_state& state)
{
    return randommantissa<T, N>(state) - 1.f;
}

/**
 * @brief Generates random values uniformly distributed in the range [min, max) for floating-point types.
 *
 * @tparam N Number of values to generate.
 * @tparam T Floating-point type.
 * @param state Reference to the random number generator state.
 * @param min Lower bound of range.
 * @param max Upper bound of range.
 * @return Vector of N values in the range [min, max).
 */
template <size_t N, typename T, KFR_ENABLE_IF(is_f_class<T>)>
KFR_INTRINSIC vec<T, N> random_range(random_state& state, T min, T max)
{
    return mix(random_uniform<T, N>(state), min, max);
}

/**
 * @brief Generates random values uniformly distributed in the range [min, max) for integral types.
 *
 * Uses integer scaling and rounding for range generation.
 *
 * @tparam N Number of values to generate.
 * @tparam T Integral type.
 * @param state Reference to the random number generator state.
 * @param min Lower bound of range.
 * @param max Upper bound of range.
 * @return Vector of N values in the range [min, max).
 */
template <size_t N, typename T, KFR_ENABLE_IF(!is_f_class<T>)>
KFR_INTRINSIC vec<T, N> random_range(random_state& state, T min, T max)
{
    using big_type = findinttype<sqr(std::numeric_limits<T>::min()), sqr(std::numeric_limits<T>::max())>;

    vec<T, N> u                = random_uniform<T, N>(state);
    const vec<big_type, N> tmp = u;
    return (tmp * (max - min) + min) >> typebits<T>::bits;
}

/**
 * @brief Generates N normally distributed (Gaussian) random values using Box-Muller transform.
 *
 * Uses 2x uniform samples to generate normal values, rounded to the nearest power-of-two vector length.
 *
 * @tparam N Number of normal values to generate.
 * @tparam T Floating-point type.
 * @param state Reference to the random number generator state.
 * @param mu Mean of the distribution.
 * @param sigma Standard deviation of the distribution.
 * @return Vector of N values from N(mu, sigma^2).
 */
template <size_t N, typename T>
KFR_INTRINSIC vec<T, N> random_normal(random_state& state, T mu, T sigma)
{
    static_assert(std::is_floating_point_v<T>, "random_normal requires floating point type");

    constexpr size_t M = align_up(N, 2); // round up to 2

    vec<T, M> u = random_uniform<T, M>(state);

    vec<T, M / 2> mag = sigma * sqrt(T(-2.0) * log(even(u)));
    vec<T, M> z       = dup(mag) * cossin(c_pi<T, 2> * dupodd(u)) + mu;
    return slice<0, N>(z);
}

template <typename T, index_t Dims, bool Reference = false>
struct expression_random_uniform : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_random_uniform&)
    {
        return shape<dims>(infinite_size);
    }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }

    mutable state_holder<random_state, Reference> state;

    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_random_uniform& self, shape<Dims>,
                                                axis_params<VecAxis, N>)
    {
        return random_uniform<N, T>(*self.state);
    }
};

template <typename T, index_t Dims, bool Reference = false>
struct expression_random_range : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_random_range&)
    {
        return shape<dims>(infinite_size);
    }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }

    mutable state_holder<random_state, Reference> state;
    T min;
    T max;

    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_random_range& self, shape<Dims>,
                                                axis_params<VecAxis, N>)
    {
        return random_range<N, T>(*self.state, self.min, self.max);
    }
};

template <typename T, index_t Dims, bool Reference = false>
struct expression_random_normal : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_random_normal&)
    {
        return shape<dims>(infinite_size);
    }
    constexpr static shape<dims> get_shape() { return shape<dims>(infinite_size); }

    mutable state_holder<random_state, Reference> state;
    T sigma{ 1 };
    T mu{ 0 };

    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_random_normal& self, shape<Dims>,
                                                axis_params<VecAxis, N>)
    {
        return random_normal<N, T>(*self.state, self.mu, self.sigma);
    }
};

/**
 * @brief Returns expression that produces uniform pseudorandom values using a copied state.
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_uniform<T, Dims> gen_random_uniform(const random_state& state)
{
    return { {}, state };
}

/**
 * @brief Returns expression that produces uniform pseudorandom values using a referenced state.
 * Use std::ref(gen) to use this overload.
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims, true> gen_random_uniform(
    std::reference_wrapper<random_state> state)
{
    return { {}, state };
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/// @brief Returns expression that produces uniform pseudorandom values using cycle counter entropy.
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims> gen_random_uniform()
{
    return expression_random_uniform<T, Dims>{ random_init() };
}
#endif

/// @brief Returns expression that produces random values in [min, max) using a copied state.
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims> gen_random_range(const random_state& state, T min, T max)
{
    return { {}, state, min, max };
}

/**
 * @brief Returns expression that produces random values in [min, max) using a referenced state.
 * Use std::ref(gen) to use this overload.
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims, true> gen_random_range(
    std::reference_wrapper<random_state> state, T min, T max)
{
    return { {}, state, min, max };
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/// @brief Returns expression that produces random values in [min, max) using cycle counter entropy
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims> gen_random_range(T min, T max)
{
    return { {}, random_init(), min, max };
}
#endif

/// @brief Returns expression that produces normally distributed values using a copied state.
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_normal<T, Dims> gen_random_normal(const random_state& state, T sigma = 1,
                                                                 T mu = 0)
{
    return { {}, state, sigma, mu };
}

/// @brief Returns expression that produces normally distributed values using a referenced state.
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_normal<T, Dims, true> gen_random_normal(
    std::reference_wrapper<random_state> state, T sigma = 1, T mu = 0)
{
    return { {}, state, sigma, mu };
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/// @brief Returns expression that produces normally distributed values using cycle counter entropy.
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_normal<T, Dims> gen_random_normal(T sigma = 1, T mu = 0)
{
    return { {}, random_init(), sigma, mu };
}
#endif

} // namespace CMT_ARCH_NAME

} // namespace kfr
