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

#include "../base/complex.hpp"
#include "../base/univector.hpp"
#include "../base/vec.hpp"

namespace cometa
{

inline std::string repr(kfr::cpu_t v);

template <typename T>
inline std::string repr(const kfr::complex<T>& v);

template <typename T, int N>
inline std::string repr(kfr::simd<T, N> v);

template <typename T, size_t N>
inline std::string repr(kfr::vec<T, N> v);

template <typename T, size_t Tag>
inline std::string repr(const kfr::univector<T, Tag>& v);
}
#include "../cometa/string.hpp"
#include <cmath>

namespace cometa
{

namespace details
{

constexpr size_t number_width           = 9;
constexpr size_t number_precision       = 6;
constexpr size_t number_precision_short = 2;
constexpr size_t number_columns         = 8;

template <typename T>
std::string fmtvalue(std::true_type, const T& x)
{
    std::string str = as_string(fmt<'g', number_width, number_precision>(x));
    if (str.size() > number_width)
        str = as_string(fmt<'g', number_width, number_precision_short>(x));
    return str;
}

template <typename T>
std::string fmtvalue(std::true_type, const kfr::complex<T>& x)
{
    std::string restr = as_string(fmt<'g', number_width, number_precision>(x.real()));
    if (restr.size() > number_width)
        restr = as_string(fmt<'g', number_width, number_precision_short>(x.real()));

    std::string imstr = as_string(fmt<'g', -1, number_precision>(std::abs(x.imag())));
    if (imstr.size() > number_width)
        imstr = as_string(fmt<'g', -1, number_precision_short>(std::abs(x.imag())));

    return restr + (x.imag() < T(0) ? "-" : "+") + padleft(number_width, imstr + "j");
}

template <typename T>
std::string fmtvalue(std::false_type, const T& x)
{
    return as_string(fmtwidth<number_width>(repr(x)));
}
}

template <typename T>
inline std::string repr(const kfr::complex<T>& v)
{
    return as_string(v.real()) + " + " + as_string(v.imag()) + "j";
}

inline std::string repr(kfr::cpu_t v) { return kfr::cpu_name(v); }

template <typename T>
inline std::string repr(const T* source, size_t N)
{
    std::string str;
    for (size_t i = 0; i < N; i++)
    {
        if (i > 0)
        {
            if (i % details::number_columns == 0 || kfr::is_vec<T>::value)
                str += "\n";
            else
                str += " ";
        }
        str += as_string(details::fmtvalue(std::is_floating_point<T>(), source[i]));
    }
    return str;
}

template <typename T>
inline std::string repr(const kfr::complex<T>* source, size_t N)
{
    std::string str;
    for (size_t i = 0; i < N; i++)
    {
        if (i > 0)
        {
            if (i % (details::number_columns / 2) == 0)
                str += "\n";
            else
                str += " ";
        }
        str += as_string(details::fmtvalue(std::true_type{}, source[i]));
    }
    return str;
}

template <typename T, int N>
inline std::string repr(kfr::simd<T, N> v)
{
    return repr(tovec(v));
}

template <typename T, size_t N>
inline std::string repr(kfr::vec<T, N> v)
{
    return repr(v.data(), v.size());
}

template <typename T, size_t Tag>
inline std::string repr(const kfr::univector<T, Tag>& v)
{
    return repr(v.data(), v.size());
}
}
