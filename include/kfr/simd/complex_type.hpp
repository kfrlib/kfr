/** @addtogroup complex
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

#include "../cometa/string.hpp"
#include "constants.hpp"

#include <complex>

namespace kfr
{
#ifndef KFR_CUSTOM_COMPLEX
template <typename T>
using complex = std::complex<T>;
#endif

} // namespace kfr

namespace cometa
{
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
struct representation<fmt_t<kfr::complex<T>, t, width, prec>>
{
    using type = std::string;
    static std::string get(const fmt_t<kfr::complex<T>, t, width, prec>& value)
    {
        return as_string(cometa::fmt<t, width, prec>(value.value.real())) + " + " +
               as_string(cometa::fmt<t, width, prec>(value.value.imag())) + "j";
    }
};
} // namespace cometa
