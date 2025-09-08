/** @addtogroup oscillators
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

#include "../base/basic_expressions.hpp"
#include "../base/simd_expressions.hpp"
#include "../math/sin_cos.hpp"
#include "../simd/round.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

template <typename T = fbase>
KFR_FUNCTION static auto phasor(std::type_identity_t<T> frequency, std::type_identity_t<T> sample_rate,
                                std::type_identity_t<T> phase = 0)
{
    return fract(counter(phase, frequency / sample_rate));
}

template <typename T = fbase>
KFR_FUNCTION static auto phasor(std::type_identity_t<T> frequency)
{
    return phasor(frequency, 1, 0);
}

namespace intr
{
template <typename T>
KFR_INTRINSIC T rawsine(const T& x)
{
    return intr::fastsin(x * constants<T>::pi_s(2));
}
template <typename T>
KFR_INTRINSIC T sinenorm(const T& x)
{
    return intr::rawsine(fract(x));
}
template <typename T>
KFR_INTRINSIC T sine(const T& x)
{
    return intr::sinenorm(constants<T>::recip_pi_s(1, 2) * x);
}

template <typename T>
KFR_INTRINSIC T rawsquare(const T& x)
{
    return select(x < T(0.5), T(1), -T(1));
}
template <typename T>
KFR_INTRINSIC T squarenorm(const T& x)
{
    return intr::rawsquare(fract(x));
}
template <typename T>
KFR_INTRINSIC T square(const T& x)
{
    return intr::squarenorm(constants<T>::recip_pi_s(1, 2) * x);
}

template <typename T>
KFR_INTRINSIC T rawsawtooth(const T& x)
{
    return T(1) - 2 * x;
}
template <typename T>
KFR_INTRINSIC T sawtoothnorm(const T& x)
{
    return intr::rawsawtooth(fract(x));
}
template <typename T>
KFR_INTRINSIC T sawtooth(const T& x)
{
    return intr::sawtoothnorm(constants<T>::recip_pi_s(1, 2) * x);
}

template <typename T>
KFR_INTRINSIC T isawtoothnorm(const T& x)
{
    return T(-1) + 2 * fract(x + 0.5);
}
template <typename T>
KFR_INTRINSIC T isawtooth(const T& x)
{
    return intr::isawtoothnorm(constants<T>::recip_pi_s(1, 2) * x);
}

template <typename T>
KFR_INTRINSIC T rawtriangle(const T& x)
{
    return 1 - abs(4 * x - 2);
}
template <typename T>
KFR_INTRINSIC T trianglenorm(const T& x)
{
    return intr::rawtriangle(fract(x + 0.25));
}
template <typename T>
KFR_INTRINSIC T triangle(const T& x)
{
    return intr::trianglenorm(constants<T>::recip_pi_s(1, 2) * x);
}
} // namespace intr
KFR_I_FN(rawsine)
KFR_I_FN(sine)
KFR_I_FN(sinenorm)
KFR_I_FN(rawsquare)
KFR_I_FN(square)
KFR_I_FN(squarenorm)
KFR_I_FN(rawtriangle)
KFR_I_FN(triangle)
KFR_I_FN(trianglenorm)
KFR_I_FN(rawsawtooth)
KFR_I_FN(sawtooth)
KFR_I_FN(sawtoothnorm)
KFR_I_FN(isawtooth)
KFR_I_FN(isawtoothnorm)

template <numeric T1>
KFR_FUNCTION T1 rawsine(const T1& x)
{
    return intr::rawsine(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::rawsine, E1> rawsine(E1&& x)
{
    return { std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 sine(const T1& x)
{
    return intr::sine(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::sine, E1> sine(E1&& x)
{
    return { std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 sinenorm(const T1& x)
{
    return intr::sinenorm(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::sinenorm, E1> sinenorm(E1&& x)
{
    return { fn::sinenorm(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 rawsquare(const T1& x)
{
    return intr::rawsquare(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::rawsquare, E1> rawsquare(E1&& x)
{
    return { fn::rawsquare(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 square(const T1& x)
{
    return intr::square(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::square, E1> square(E1&& x)
{
    return { fn::square(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 squarenorm(const T1& x)
{
    return intr::squarenorm(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::squarenorm, E1> squarenorm(E1&& x)
{
    return { fn::squarenorm(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 rawtriangle(const T1& x)
{
    return intr::rawtriangle(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::rawtriangle, E1> rawtriangle(E1&& x)
{
    return { fn::rawtriangle(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 triangle(const T1& x)
{
    return intr::triangle(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::triangle, E1> triangle(E1&& x)
{
    return { fn::triangle(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 trianglenorm(const T1& x)
{
    return intr::trianglenorm(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::trianglenorm, E1> trianglenorm(E1&& x)
{
    return { fn::trianglenorm(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 rawsawtooth(const T1& x)
{
    return intr::rawsawtooth(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::rawsawtooth, E1> rawsawtooth(E1&& x)
{
    return { fn::rawsawtooth(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 sawtooth(const T1& x)
{
    return intr::sawtooth(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::sawtooth, E1> sawtooth(E1&& x)
{
    return { fn::sawtooth(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 sawtoothnorm(const T1& x)
{
    return intr::sawtoothnorm(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::sawtoothnorm, E1> sawtoothnorm(E1&& x)
{
    return { fn::sawtoothnorm(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 isawtooth(const T1& x)
{
    return intr::isawtooth(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::isawtooth, E1> isawtooth(E1&& x)
{
    return { fn::isawtooth(), std::forward<E1>(x) };
}
template <numeric T1>
KFR_FUNCTION T1 isawtoothnorm(const T1& x)
{
    return intr::isawtoothnorm(x);
}
template <expression_argument E1>
KFR_FUNCTION expression_function<fn::isawtoothnorm, E1> isawtoothnorm(E1&& x)
{
    return { fn::isawtoothnorm(), std::forward<E1>(x) };
}
} // namespace KFR_ARCH_NAME

} // namespace kfr
