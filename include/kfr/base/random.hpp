/*
  Copyright (C) 2016-2026 Dan Casarin (https://www.kfrlib.com)
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

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Generates a vector of uniformly distributed integers by reinterpreting raw random bits.
 *
 * @tparam T Integral type of the result.
 * @tparam N Number of values to generate.
 * @param state Reference to the random number generator state.
 * @return Vector of N uniformly distributed integers.
 */
template <std::integral T, size_t N>
KFR_INTRINSIC vec<T, N> random_uniform(random_state& state)
{
    return bitcast<T>(random_bits<N * sizeof(T)>(state));
}

/**
 * @brief Generates a vector of f32 values with mantissas in [1.0, 2.0).
 *
 * The raw bits are masked to keep only the mantissa bits and to force the
 * exponent to that of 1.0, producing values in [1.0, 2.0).
 *
 * @tparam T Must be f32.
 * @tparam N Number of values to generate.
 * @param state Reference to the random number generator state.
 * @return Vector of N floats in [1.0, 2.0).
 */
template <std::same_as<f32> T, size_t N>
KFR_INTRINSIC vec<f32, N> randommantissa(random_state& state)
{
    return bitcast<f32>((random_uniform<u32, N>(state) & u32(0x7FFFFFu)) | u32(0x3f800000u)) + 0.0f;
}

/**
 * @brief Generates a vector of f64 values with mantissas in [1.0, 2.0).
 *
 * The raw bits are masked to keep only the mantissa bits and to force the
 * exponent to that of 1.0, producing values in [1.0, 2.0).
 *
 * @tparam T Must be f64.
 * @tparam N Number of values to generate.
 * @param state Reference to the random number generator state.
 * @return Vector of N doubles in [1.0, 2.0).
 */
template <std::same_as<f64> T, size_t N>
KFR_INTRINSIC vec<f64, N> randommantissa(random_state& state)
{
    return bitcast<f64>((random_uniform<u64, N>(state) & u64(0x000FFFFFFFFFFFFFull)) |
                        u64(0x3FF0000000000000ull)) +
           0.0;
}

/**
 * @brief Generates a vector of uniformly distributed floating-point numbers in [0.0, 1.0).
 *
 * Derived by generating mantissas in [1.0, 2.0) and subtracting 1.0.
 *
 * @tparam T Floating-point type (f32 or f64).
 * @tparam N Number of random floats to generate.
 * @param state Reference to the random number generator state.
 * @return Vector of N floats in the range [0.0, 1.0).
 */
template <f_class T, size_t N>
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
template <size_t N, f_class T>
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
template <size_t N, not_f_class T>
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

/**
 * @brief Expression that produces uniformly distributed random values in [0, 1).
 *
 * The expression has infinite size along all dimensions. The generator state is
 * advanced each time samples are requested.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @tparam Reference When true, holds a reference to the state; otherwise holds a copy.
 */
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

    /// Internal ADL-provided implementation for `expression_random_uniform` expressions
    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_random_uniform& self, shape<Dims>,
                                                axis_params<VecAxis, N>)
    {
        return random_uniform<T, N>(*self.state);
    }
};

/**
 * @brief Expression that produces uniformly distributed random values in [min, max).
 *
 * The expression has infinite size along all dimensions. The generator state is
 * advanced each time samples are requested.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @tparam Reference When true, holds a reference to the state; otherwise holds a copy.
 */
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

    /// Internal ADL-provided implementation for `expression_random_range` expressions
    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_random_range& self, shape<Dims>,
                                                axis_params<VecAxis, N>)
    {
        return random_range<N, T>(*self.state, self.min, self.max);
    }
};

/**
 * @brief Expression that produces normally distributed (Gaussian) random values.
 *
 * The expression has infinite size along all dimensions. The generator state is
 * advanced each time samples are requested.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @tparam Reference When true, holds a reference to the state; otherwise holds a copy.
 */
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

    /// Internal ADL-provided implementation for `expression_random_normal` expressions
    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_random_normal& self, shape<Dims>,
                                                axis_params<VecAxis, N>)
    {
        return random_normal<N, T>(*self.state, self.mu, self.sigma);
    }
};

/**
 * @brief Creates an expression that produces uniform pseudorandom values in [0, 1) using a copied state.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param state Random generator state to copy into the expression.
 * @return Expression generating uniform random values.
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_uniform<T, Dims> gen_random_uniform(const random_state& state)
{
    return { {}, state };
}

/**
 * @brief Creates an expression that produces uniform pseudorandom values in [0, 1) using a referenced state.
 *
 * Use std::ref(state) to select this overload.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param state Reference wrapper to the random generator state.
 * @return Expression generating uniform random values.
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_uniform<T, Dims, true> gen_random_uniform(
    std::reference_wrapper<random_state> state)
{
    return { {}, state };
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/**
 * @brief Creates an expression that produces uniform pseudorandom values in [0, 1) seeded from the cycle
 * counter.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @return Expression generating uniform random values.
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_uniform<T, Dims> gen_random_uniform()
{
    return expression_random_uniform<T, Dims>{ random_init() };
}
#endif

/**
 * @brief Creates an expression that produces random values in [min, max) using a copied state.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param state Random generator state to copy into the expression.
 * @param min Lower bound of the range (inclusive).
 * @param max Upper bound of the range (exclusive).
 * @return Expression generating random values in [min, max).
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims> gen_random_range(const random_state& state, T min, T max)
{
    return { {}, state, min, max };
}

/**
 * @brief Creates an expression that produces random values in [min, max) using a referenced state.
 *
 * Use std::ref(state) to select this overload.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param state Reference wrapper to the random generator state.
 * @param min Lower bound of the range (inclusive).
 * @param max Upper bound of the range (exclusive).
 * @return Expression generating random values in [min, max).
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims, true> gen_random_range(
    std::reference_wrapper<random_state> state, T min, T max)
{
    return { {}, state, min, max };
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/**
 * @brief Creates an expression that produces random values in [min, max) seeded from the cycle counter.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param min Lower bound of the range (inclusive).
 * @param max Upper bound of the range (exclusive).
 * @return Expression generating random values in [min, max).
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_range<T, Dims> gen_random_range(T min, T max)
{
    return { {}, random_init(), min, max };
}
#endif

/**
 * @brief Creates an expression that produces normally distributed values using a copied state.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param state Random generator state to copy into the expression.
 * @param sigma Standard deviation of the distribution.
 * @param mu Mean of the distribution.
 * @return Expression generating values from N(mu, sigma^2).
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_normal<T, Dims> gen_random_normal(const random_state& state, T sigma = 1,
                                                                 T mu = 0)
{
    return { {}, state, sigma, mu };
}

/**
 * @brief Creates an expression that produces normally distributed values using a referenced state.
 *
 * Use std::ref(state) to select this overload.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param state Reference wrapper to the random generator state.
 * @param sigma Standard deviation of the distribution.
 * @param mu Mean of the distribution.
 * @return Expression generating values from N(mu, sigma^2).
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_normal<T, Dims, true> gen_random_normal(
    std::reference_wrapper<random_state> state, T sigma = 1, T mu = 0)
{
    return { {}, state, sigma, mu };
}

#ifndef KFR_DISABLE_READCYCLECOUNTER
/**
 * @brief Creates an expression that produces normally distributed values seeded from the cycle counter.
 *
 * @tparam T Value type of the generated samples.
 * @tparam Dims Number of dimensions of the expression.
 * @param sigma Standard deviation of the distribution.
 * @param mu Mean of the distribution.
 * @return Expression generating values from N(mu, sigma^2).
 */
template <typename T, index_t Dims = 1>
KFR_FUNCTION expression_random_normal<T, Dims> gen_random_normal(T sigma = 1, T mu = 0)
{
    return { {}, random_init(), sigma, mu };
}
#endif

} // namespace KFR_ARCH_NAME

} // namespace kfr
