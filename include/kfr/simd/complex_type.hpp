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

#include "../meta/string.hpp"
#include "constants.hpp"

#include <complex>

namespace kfr
{
#ifndef KFR_CUSTOM_COMPLEX
/**
 * @brief Complex number type, aliased to `std::complex<T>` by default.
 * @details Define `KFR_CUSTOM_COMPLEX` to substitute a user-provided
 * complex type implementation.
 * @tparam T The underlying real element type (e.g. `float`, `double`).
 */
template <typename T>
using complex = std::complex<T>;
#endif

} // namespace kfr

namespace kfr
{
/**
 * @brief String representation specialization for `kfr::complex<T>`.
 * @details Renders a complex value as `"<real> + <imag>j"` using
 * `as_string` for each component.
 * @tparam T The underlying real element type.
 */
template <typename T>
struct representation<kfr::complex<T>>
{
    /** Result type of the conversion. */
    using type = std::string;
    /**
     * @brief Converts a complex value to its string form.
     * @param value The complex value to format.
     * @return A string of the form `"<real> + <imag>j"`.
     */
    static std::string get(const kfr::complex<T>& value)
    {
        return as_string(value.real()) + " + " + as_string(value.imag()) + "j";
    }
};

/**
 * @brief String representation specialization for a formatted complex value.
 * @details Applies the format specifier (`t`, `width`, `prec`) to both the
 * real and imaginary parts before joining them as `"<real> + <imag>j"`.
 * @tparam t Format character (see `fmt_t`).
 * @tparam width Minimum field width.
 * @tparam prec Precision specifier.
 * @tparam T The underlying real element type.
 */
template <char t, int width, int prec, typename T>
struct representation<fmt_t<kfr::complex<T>, t, width, prec>>
{
    /** Result type of the conversion. */
    using type = std::string;
    /**
     * @brief Converts a formatted complex value to its string form.
     * @param value The formatted complex value to render.
     * @return A string of the form `"<real> + <imag>j"` with each
     * component formatted according to `t`, `width`, and `prec`.
     */
    static std::string get(const fmt_t<kfr::complex<T>, t, width, prec>& value)
    {
        return as_string(kfr::fmt<t, width, prec>(value.value.real())) + " + " +
               as_string(kfr::fmt<t, width, prec>(value.value.imag())) + "j";
    }
};
} // namespace kfr
