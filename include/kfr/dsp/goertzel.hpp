/** @addtogroup dsp_extra
 *  @{
 */
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

#include "../base/basic_expressions.hpp"
#include "../math/sin_cos.hpp"
#include "../simd/complex.hpp"
#include "../simd/vec.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Single-bin Goertzel algorithm expression.
 *
 * Recursively filters the input sample stream through the second-order Goertzel
 * resonator tuned to the angular frequency `omega`. The complex DFT bin value
 * for `omega` is computed incrementally as samples are pushed via
 * `set_elements` and is written to `result` when the expression is destroyed
 * (i.e. when `process` completes).
 *
 * @tparam T Sample and result type (floating point).
 *
 * @param result Reference that receives the complex bin value on destruction.
 * @param omega  Target angular frequency in radians per sample
 *               (omega = 2*pi*f/fs).
 *
 * @note The result is only valid after the expression object has been
 *       destroyed; the final rotation into the complex value happens in the
 *       destructor.
 */
template <typename T>
struct expression_goertzel : expression_traits_defaults
{
    constexpr static size_t dims = 1;

    using value_type = T;

    constexpr static shape<1> get_shape(const expression_goertzel&) { return shape<1>(infinite_size); }
    constexpr static shape<1> get_shape() { return shape<1>(infinite_size); }

    /**
     * @brief Construct a Goertzel resonator for frequency `omega`.
     * @param result Output complex bin value (filled on destruction).
     * @param omega  Target angular frequency [rad/sample].
     */
    expression_goertzel(complex<T>& result, T omega)
        : result(result), omega(omega), coeff(2 * cos(omega)), q0(0), q1(0), q2(0)
    {
    }
    /**
     * @brief Destructor: rotates the final filter state into the complex DFT
     *        bin value and stores it in `result`.
     */
    ~expression_goertzel()
    {
        result.real(q1 - q2 * cos(omega));
        result.imag(q2 * sin(omega));
    }

    /**
     * @brief Internal ADL-provided implementation for `expression_goertzel` expressions.
     */
    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC void set_elements(expression_goertzel& self, shape<1>, axis_params<VecAxis, N>,
                                           const std::type_identity_t<vec<T, N>>& x)
    {
        vec<T, N> in = x;
        KFR_LOOP_UNROLL
        for (size_t i = 0; i < N; i++)
        {
            self.q0 = self.coeff * self.q1 - self.q2 + in[i];
            self.q2 = self.q1;
            self.q1 = self.q0;
        }
    }
    complex<T>& result; ///< Output complex bin value (written on destruction).
    const T omega; ///< Target angular frequency [rad/sample].
    const T coeff; ///< Precomputed resonator coefficient `2*cos(omega)`.
    T q0; ///< Current filter output (working state).
    T q1; ///< Previous output sample (working state).
    T q2; ///< Output sample two steps back (working state).
};

/**
 * @brief Parallel (multi-bin) Goertzel algorithm expression.
 *
 * Evaluates `width` independent Goertzel resonators simultaneously, one per
 * element of `omega`, using SIMD vectors. Each resonator tracks the same input
 * stream but a different target frequency. On destruction the `width` complex
 * bin values are written to the `result` array.
 *
 * @tparam T     Sample and result type (floating point).
 * @tparam width Number of parallel resonators (must match the SIMD vector
 *               width used to construct the expression).
 *
 * @param result Array of `width` complex values (filled on destruction).
 * @param omega  SIMD vector of `width` angular frequencies [rad/sample].
 *
 * @note Results are only valid after the expression object has been destroyed.
 */
template <typename T, size_t width>
struct expression_parallel_goertzel : expression_traits_defaults
{
    constexpr static size_t dims = 1;

    using value_type = T;

    constexpr static shape<1> get_shape(const expression_parallel_goertzel&)
    {
        return shape<1>(infinite_size);
    }
    constexpr static shape<1> get_shape() { return shape<1>(infinite_size); }

    /**
     * @brief Construct `width` parallel Goertzel resonators.
     * @param result Output array of `width` complex bin values (filled on destruction).
     * @param omega  SIMD vector of `width` angular frequencies [rad/sample].
     */
    expression_parallel_goertzel(complex<T> result[], vec<T, width> omega)
        : result(result), omega(omega), coeff(2 * cos(omega)), q0(T(0)), q1(T(0)), q2(T(0))
    {
    }
    /**
     * @brief Destructor: rotates the final filter states into the `width`
     *        complex DFT bin values and stores them in `result`.
     */
    ~expression_parallel_goertzel()
    {
        const vec<T, width> re = q1 - q2 * cos(omega);
        const vec<T, width> im = q2 * sin(omega);
        for (size_t i = 0; i < width; i++)
        {
            result[i].real(re[i]);
            result[i].imag(im[i]);
        }
    }
    /**
     * @brief Internal ADL-provided implementation for `expression_parallel_goertzel` expressions.
     */
    template <size_t N, index_t VecAxis>
    friend KFR_INTRINSIC void set_elements(expression_parallel_goertzel& self, shape<1>,
                                           axis_params<VecAxis, N>, const std::type_identity_t<vec<T, N>>& x)
    {
        const vec<T, N> in = x;
        KFR_LOOP_UNROLL
        for (size_t i = 0; i < N; i++)
        {
            self.q0 = self.coeff * self.q1 - self.q2 + in[i];
            self.q2 = self.q1;
            self.q1 = self.q0;
        }
    }
    complex<T>* result; ///< Output array of `width` complex bin values.
    const vec<T, width> omega; ///< SIMD vector of target angular frequencies.
    const vec<T, width> coeff; ///< Precomputed coefficients `2*cos(omega)`.
    vec<T, width> q0; ///< Current filter outputs (working state).
    vec<T, width> q1; ///< Previous output samples (working state).
    vec<T, width> q2; ///< Output samples two steps back (working state).
};

/**
 * @brief Create a single-bin Goertzel expression.
 *
 * Intended to be passed to `process(expr, input)`; the complex DFT bin value
 * for `omega` is written to `result` when the resulting expression is
 * destroyed (i.e. when `process` returns).
 *
 * @tparam T    Sample and result type.
 * @param result Reference receiving the complex bin value.
 * @param omega  Target angular frequency [rad/sample].
 * @return A Goertzel expression to feed to `process`.
 */
template <typename T>
KFR_INTRINSIC expression_goertzel<T> goertzel(complex<T>& result, std::type_identity_t<T> omega)
{
    return expression_goertzel<T>(result, omega);
}

/**
 * @brief Create `width` parallel Goertzel expressions.
 *
 * Intended to be passed to `process(expr, input)`; the `width` complex DFT bin
 * values for the frequencies in `omega` are written to `result` when the
 * resulting expression is destroyed (i.e. when `process` returns).
 *
 * @tparam T     Sample and result type.
 * @tparam width Number of parallel resonators (deduced from `result`/`omega`).
 * @param result Array of `width` complex values receiving the bin values.
 * @param omega  C array of `width` angular frequencies [rad/sample].
 * @return A parallel Goertzel expression to feed to `process`.
 */
template <typename T, size_t width>
KFR_INTRINSIC expression_parallel_goertzel<T, width> goertzel(complex<T> (&result)[width],
                                                              const T (&omega)[width])
{
    return expression_parallel_goertzel<T, width>(result, read<width>(omega));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
