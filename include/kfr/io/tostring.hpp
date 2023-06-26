/** @addtogroup string_io
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

#include "../base/univector.hpp"
#include "../cometa/string.hpp"
#include "../simd/complex.hpp"
#include "../simd/vec.hpp"
#include <cmath>
#include <vector>

namespace cometa
{

template <>
struct representation<cometa::special_value>
{
    using type = std::string;
    static std::string get(const cometa::special_value& value)
    {
        using cometa::special_constant;
        switch (value.c)
        {
        case special_constant::default_constructed:
            return "default_constructed";
        case special_constant::infinity:
            return "infinity";
        case special_constant::neg_infinity:
            return "neg_infinity";
        case special_constant::min:
            return "min";
        case special_constant::max:
            return "max";
        case special_constant::neg_max:
            return "neg_max";
        case special_constant::lowest:
            return "lowest";
        case special_constant::integer:
            return as_string(value.ll);
        case special_constant::floating_point:
            return as_string(value.d);
        case special_constant::epsilon:
            return "epsilon";
        case special_constant::random_bits:
            return "random_bits";
        default:
            return "unknown";
        }
    }
};

namespace details
{

constexpr size_t number_width           = 9;
constexpr size_t number_precision       = 6;
constexpr size_t number_precision_short = 2;
constexpr size_t number_columns         = 8;

template <typename T>
std::string fmtvalue(std::true_type, const T& x)
{
    std::string str = as_string(cometa::fmt<'g', number_width, number_precision>(x));
    if (str.size() > number_width)
        str = as_string(cometa::fmt<'g', number_width, number_precision_short>(x));
    return str;
}

template <typename T>
std::string fmtvalue(std::true_type, const kfr::complex<T>& x)
{
    std::string restr = as_string(cometa::fmt<'g', number_width, number_precision>(x.real()));
    if (restr.size() > number_width)
        restr = as_string(cometa::fmt<'g', number_width, number_precision_short>(x.real()));

    std::string imstr = as_string(cometa::fmt<'g', -1, number_precision>(std::abs(x.imag())));
    if (imstr.size() > number_width)
        imstr = as_string(cometa::fmt<'g', -1, number_precision_short>(std::abs(x.imag())));

    return restr + (x.imag() < T(0) ? "-" : "+") + padleft(number_width, imstr + "j");
}

template <typename T>
std::string fmtvalue(std::false_type, const T& x)
{
    return as_string(fmtwidth<number_width>(representation<T>::get(x)));
}

template <typename T>
inline std::string array_to_string(const T* source, size_t N)
{
    std::string str;
    for (size_t i = 0; i < N; i++)
    {
        if (i > 0)
        {
            if (i % details::number_columns == 0 || kfr::is_vec<T>)
                str += "\n";
            else
                str += " ";
        }
        str += as_string(details::fmtvalue(std::is_floating_point<T>(), source[i]));
    }
    return str;
}

template <typename T>
inline std::string array_to_string(const kfr::complex<T>* source, size_t N)
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
} // namespace details

template <>
struct representation<kfr::cpu_t>
{
    using type = std::string;
    static std::string get(kfr::cpu_t value) { return kfr::cpu_name(value); }
};

} // namespace cometa

namespace kfr
{

/// @brief Converts dB value to string (uses oo for infinity symbol)
template <typename T>
std::string dB_to_string(const T& value, double minimum = -140.0)
{
    if (value <= minimum)
        return "-oo dB";
    return as_string(fmtwidth<0, 2>(value), " dB");
}

/// @brief Converts dB value to string (uses infinity symbol in utf-8 encoding)
template <typename T>
std::string dB_to_utf8string(const T& value, double minimum = -140.0)
{
    if (value <= minimum)
        return "-\xE2\x88\x9E dB"; // infinity symbol
    return as_string(fmtwidth<0, 2>(value), " dB");
}
} // namespace kfr
