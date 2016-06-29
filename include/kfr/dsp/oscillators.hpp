/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "../base/sin_cos.hpp"
#include "../base/vec.hpp"
#include "../expressions/basic.hpp"

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

inline auto simpleimpulse()
{
    return lambda([](cinput_t, size_t index, auto x) {
        if (index == 0)
            return onoff(x);
        else
            return zerovector(x);
    });
}

template <typename T>
auto jaehne(T magn, size_t size)
{
    using namespace native;
    return typed<T>(magn * sin(c_pi<T, 1, 2> * sqr(linspace(T(0), T(size), size, false)) / size), size);
}

template <typename T>
auto swept(T magn, size_t size)
{
    using namespace native;
    return typed<T>(
        magn * sin(c_pi<T, 1, 4> * sqr(sqr(linspace(T(0), T(size), size, false)) / sqr(T(size))) * T(size)),
        size);
}

namespace internal
{
template <cpu_t c = cpu_t::native, cpu_t cc = c>
struct in_oscillators : in_sin_cos<cc>, in_select<cc>, in_round<cc>, in_abs<cc>
{
private:
    using in_sin_cos<cc>::fastsin;
    using in_sin_cos<cc>::sin;
    using in_select<cc>::select;
    using in_round<cc>::fract;
    using in_abs<cc>::abs;

public:
    template <typename T>
    KFR_SINTRIN T rawsine(T x)
    {
        return fastsin(x * c_pi<T, 2>);
    }
    template <typename T>
    KFR_SINTRIN T sinenorm(T x)
    {
        return rawsine(fract(x));
    }
    template <typename T>
    KFR_SINTRIN T sine(T x)
    {
        return sinenorm(c_recip_pi<T, 1, 2> * x);
    }

    template <typename T>
    KFR_SINTRIN T rawsquare(T x)
    {
        return select(x < T(0.5), T(1), -T(1));
    }
    template <typename T>
    KFR_SINTRIN T squarenorm(T x)
    {
        return rawsquare(fract(x));
    }
    template <typename T>
    KFR_SINTRIN T square(T x)
    {
        return squarenorm(c_recip_pi<T, 1, 2> * x);
    }

    template <typename T>
    KFR_SINTRIN T rawsawtooth(T x)
    {
        return T(1) - 2 * x;
    }
    template <typename T>
    KFR_SINTRIN T sawtoothnorm(T x)
    {
        return rawsawtooth(fract(x));
    }
    template <typename T>
    KFR_SINTRIN T sawtooth(T x)
    {
        return sawtoothnorm(c_recip_pi<T, 1, 2> * x);
    }

    template <typename T>
    KFR_SINTRIN T isawtoothnorm(T x)
    {
        return T(-1) + 2 * fract(x + 0.5);
    }
    template <typename T>
    KFR_SINTRIN T isawtooth(T x)
    {
        return isawtoothnorm(c_recip_pi<T, 1, 2> * x);
    }

    template <typename T>
    KFR_SINTRIN T rawtriangle(T x)
    {
        return 1 - abs(4 * x - 2);
    }
    template <typename T>
    KFR_SINTRIN T trianglenorm(T x)
    {
        return rawtriangle(fract(x + 0.25));
    }
    template <typename T>
    KFR_SINTRIN T triangle(T x)
    {
        return trianglenorm(c_recip_pi<T, 1, 2> * x);
    }

    KFR_SPEC_FN(in_oscillators, rawsine)
    KFR_SPEC_FN(in_oscillators, sine)
    KFR_SPEC_FN(in_oscillators, sinenorm)
    KFR_SPEC_FN(in_oscillators, rawsquare)
    KFR_SPEC_FN(in_oscillators, square)
    KFR_SPEC_FN(in_oscillators, squarenorm)
    KFR_SPEC_FN(in_oscillators, rawtriangle)
    KFR_SPEC_FN(in_oscillators, triangle)
    KFR_SPEC_FN(in_oscillators, trianglenorm)
    KFR_SPEC_FN(in_oscillators, rawsawtooth)
    KFR_SPEC_FN(in_oscillators, sawtooth)
    KFR_SPEC_FN(in_oscillators, sawtoothnorm)
    KFR_SPEC_FN(in_oscillators, isawtooth)
    KFR_SPEC_FN(in_oscillators, isawtoothnorm)
};
}

using fn_rawsine = internal::in_oscillators<>::fn_rawsine;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> rawsine(const T1& x)
{
    return internal::in_oscillators<>::rawsine(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_rawsine, E1> rawsine(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_sine = internal::in_oscillators<>::fn_sine;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sine(const T1& x)
{
    return internal::in_oscillators<>::sine(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_sine, E1> sine(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_sinenorm = internal::in_oscillators<>::fn_sinenorm;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sinenorm(const T1& x)
{
    return internal::in_oscillators<>::sinenorm(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_sinenorm, E1> sinenorm(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_rawsquare = internal::in_oscillators<>::fn_rawsquare;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> rawsquare(const T1& x)
{
    return internal::in_oscillators<>::rawsquare(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_rawsquare, E1> rawsquare(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_square = internal::in_oscillators<>::fn_square;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> square(const T1& x)
{
    return internal::in_oscillators<>::square(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_square, E1> square(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_squarenorm = internal::in_oscillators<>::fn_squarenorm;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> squarenorm(const T1& x)
{
    return internal::in_oscillators<>::squarenorm(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_squarenorm, E1> squarenorm(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_rawtriangle = internal::in_oscillators<>::fn_rawtriangle;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> rawtriangle(const T1& x)
{
    return internal::in_oscillators<>::rawtriangle(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_rawtriangle, E1> rawtriangle(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_triangle = internal::in_oscillators<>::fn_triangle;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> triangle(const T1& x)
{
    return internal::in_oscillators<>::triangle(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_triangle, E1> triangle(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_trianglenorm = internal::in_oscillators<>::fn_trianglenorm;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> trianglenorm(const T1& x)
{
    return internal::in_oscillators<>::trianglenorm(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_trianglenorm, E1> trianglenorm(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_rawsawtooth = internal::in_oscillators<>::fn_rawsawtooth;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> rawsawtooth(const T1& x)
{
    return internal::in_oscillators<>::rawsawtooth(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_rawsawtooth, E1> rawsawtooth(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_sawtooth = internal::in_oscillators<>::fn_sawtooth;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sawtooth(const T1& x)
{
    return internal::in_oscillators<>::sawtooth(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_sawtooth, E1> sawtooth(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_sawtoothnorm = internal::in_oscillators<>::fn_sawtoothnorm;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> sawtoothnorm(const T1& x)
{
    return internal::in_oscillators<>::sawtoothnorm(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_sawtoothnorm, E1> sawtoothnorm(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_isawtooth = internal::in_oscillators<>::fn_isawtooth;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> isawtooth(const T1& x)
{
    return internal::in_oscillators<>::isawtooth(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_isawtooth, E1> isawtooth(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
using fn_isawtoothnorm = internal::in_oscillators<>::fn_isawtoothnorm;
template <typename T1, KFR_ENABLE_IF(is_numeric<T1>::value)>
KFR_INTRIN ftype<T1> isawtoothnorm(const T1& x)
{
    return internal::in_oscillators<>::isawtoothnorm(x);
}

template <typename E1, KFR_ENABLE_IF(is_input_expression<E1>::value)>
KFR_INTRIN expr_func<fn_isawtoothnorm, E1> isawtoothnorm(E1&& x)
{
    return { {}, std::forward<E1>(x) };
}
}

#pragma clang diagnostic pop
