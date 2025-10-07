/** @addtogroup dsp_extra
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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

template <typename T>
struct expression_goertzel : expression_traits_defaults
{
    constexpr static size_t dims = 1;

    using value_type = T;

    constexpr static shape<1> get_shape(const expression_goertzel&) { return shape<1>(infinite_size); }
    constexpr static shape<1> get_shape() { return shape<1>(infinite_size); }

    expression_goertzel(complex<T>& result, T omega)
        : result(result), omega(omega), coeff(2 * cos(omega)), q0(0), q1(0), q2(0)
    {
    }
    ~expression_goertzel()
    {
        result.real(q1 - q2 * cos(omega));
        result.imag(q2 * sin(omega));
    }

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
    complex<T>& result;
    const T omega;
    const T coeff;
    T q0;
    T q1;
    T q2;
};

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

    expression_parallel_goertzel(complex<T> result[], vec<T, width> omega)
        : result(result), omega(omega), coeff(2 * cos(omega)), q0(T(0)), q1(T(0)), q2(T(0))
    {
    }
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
    complex<T>* result;
    const vec<T, width> omega;
    const vec<T, width> coeff;
    vec<T, width> q0;
    vec<T, width> q1;
    vec<T, width> q2;
};

template <typename T>
KFR_INTRINSIC expression_goertzel<T> goertzel(complex<T>& result, std::type_identity_t<T> omega)
{
    return expression_goertzel<T>(result, omega);
}

template <typename T, size_t width>
KFR_INTRINSIC expression_parallel_goertzel<T, width> goertzel(complex<T> (&result)[width],
                                                              const T (&omega)[width])
{
    return expression_parallel_goertzel<T, width>(result, read<width>(omega));
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
