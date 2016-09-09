/** @addtogroup io
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

#include "../base/complex.hpp"
#include "../base/univector.hpp"
#include "../base/vec.hpp"
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
}

template <typename T>
struct representation<kfr::complex<T>>
{
    static std::string get(const kfr::complex<T>& value)
    {
        return as_string(value.real()) + " + " + as_string(value.imag()) + "j";
    }
};

template <>
struct representation<kfr::cpu_t>
{
    static std::string get(kfr::cpu_t value) { return kfr::cpu_name(value); }
};

template <typename T, size_t N>
struct representation<kfr::vec<T, N>>
{
    static std::string get(const kfr::vec<T, N>& value)
    {
        return details::array_to_string(value.data(), value.size());
    }
};

template <typename T, size_t Tag>
struct representation<kfr::univector<T, Tag>>
{
    static std::string get(const kfr::univector<T, Tag>& value)
    {
        return details::array_to_string(value.data(), value.size());
    }
};
}

namespace kfr
{

namespace internal
{
struct expression_printer : output_expression
{
    template <typename T, size_t N>
    void operator()(coutput_t, size_t index, const vec<T, N>& value)
    {
        for (size_t i = 0; i < N; i++)
        {
            if (index + i != 0)
                print(", ");
            print(value[i]);
        }
    }
    template <typename InputExpr>
    InputExpr& operator=(const InputExpr& input)
    {
        process<value_type_of<InputExpr>>(*this, input);
        return input;
    }
};

struct expression_debug_printer : output_expression
{
    template <typename T, size_t N>
    void operator()(coutput_t, size_t index, const vec<T, N>& value)
    {
        println(fmtwidth<7>(index), ": (", value, ")");
    }
    template <typename InputExpr>
    InputExpr& operator=(const InputExpr& input)
    {
        process<value_type_of<InputExpr>>(*this, input);
        return input;
    }
};
}
inline internal::expression_printer printer() { return internal::expression_printer(); }
inline internal::expression_debug_printer debug_printer() { return internal::expression_debug_printer(); }
}
