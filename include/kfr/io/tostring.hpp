/** @addtogroup string_io
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

template <typename T>
struct representation<kfr::complex<T>>
{
    using type = std::string;
    static std::string get(const kfr::complex<T>& value)
    {
        return as_string(value.real()) + " + " + as_string(value.imag()) + "j";
    }
};

template <char t, int width, int prec, typename T>
struct representation<details::fmt_t<kfr::complex<T>, t, width, prec>>
{
    using type = std::string;
    static std::string get(const details::fmt_t<kfr::complex<T>, t, width, prec>& value)
    {
        return as_string(fmt<t, width, prec>(value.value.real())) + " + " +
               as_string(fmt<t, width, prec>(value.value.imag())) + "j";
    }
};

template <>
struct representation<kfr::cpu_t>
{
    using type = std::string;
    static std::string get(kfr::cpu_t value) { return kfr::cpu_name(value); }
};

template <typename T, size_t N>
struct representation<kfr::vec<T, N>>
{
    using type = std::string;
    static std::string get(const kfr::vec<T, N>& value)
    {
        return details::array_to_string(ptr_cast<T>(&value), value.size());
    }
};

template <typename T, size_t N>
struct representation<kfr::mask<T, N>>
{
    using type = std::string;
    static std::string get(const kfr::mask<T, N>& value)
    {
        bool values[N];
        for (size_t i = 0; i < N; i++)
            values[i] = value[i];
        return details::array_to_string(values, N);
    }
};

template <typename T, kfr::univector_tag Tag>
struct representation<kfr::univector<T, Tag>>
{
    using type = std::string;
    static std::string get(const kfr::univector<T, Tag>& value)
    {
        return details::array_to_string(value.data(), value.size());
    }
};
template <typename T, size_t Size>
struct representation<std::array<T, Size>>
{
    using type = std::string;
    static std::string get(const std::array<T, Size>& value)
    {
        return details::array_to_string(value.data(), value.size());
    }
};
template <typename T, typename Allocator>
struct representation<std::vector<T, Allocator>>
{
    using type = std::string;
    static std::string get(const std::vector<T, Allocator>& value)
    {
        return details::array_to_string(value.data(), value.size());
    }
};
} // namespace cometa

namespace kfr
{
inline namespace CMT_ARCH_NAME
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
        process(*this, input);
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
        process(*this, input);
        return input;
    }
};
} // namespace internal

/// @brief Returns an output expression that prints the values
inline internal::expression_printer printer() { return internal::expression_printer(); }

/// @brief Returns an output expression that prints the values with their types (used for debug)
inline internal::expression_debug_printer debug_printer() { return internal::expression_debug_printer(); }
} // namespace CMT_ARCH_NAME

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
