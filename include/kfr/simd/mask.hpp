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

#include "vec.hpp"

namespace kfr
{

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Obtains the mask type associated with a SIMD type.
 *
 * @tparam T A SIMD vector type that defines a nested `mask_t` alias
 *           (e.g. `vec<T, N>`).
 */
template <typename T>
using maskfor = typename T::mask_t;

namespace internal
{

/**
 * @brief Builds a constant vector whose elements are the result of
 *        `maskbits<T>(indices < N1)`.
 *
 * Used to construct a partial selection mask of width @p Nout where the
 * first @p N1 lanes are set and the remaining lanes are cleared.
 *
 * @tparam T      Element type of the resulting vector.
 * @tparam Nout   Width of the resulting vector.
 * @tparam N1     Number of leading lanes that should be set.
 * @tparam indices Compile-time index sequence covering `[0, Nout)`.
 * @return A `vec<T, Nout>` of mask bits.
 */
template <typename T, size_t Nout, size_t N1, size_t... indices>
constexpr vec<T, Nout> partial_mask_helper(csizes_t<indices...>)
{
    return make_vector(maskbits<T>(indices < N1)...);
}

/**
 * @brief Returns a constant partial mask of width @p Nout with the first
 *        @p N1 lanes set.
 *
 * @tparam T    Element type of the resulting vector.
 * @tparam Nout Width of the resulting vector.
 * @tparam N1   Number of leading lanes that should be set.
 * @return A `vec<T, Nout>` of mask bits.
 */
template <typename T, size_t Nout, size_t N1>
constexpr vec<T, Nout> partial_mask()
{
    return internal::partial_mask_helper<T, Nout, N1>(csizeseq_t<Nout>());
}
} // namespace internal

/**
 * @brief Constructs a SIMD mask vector from boolean values.
 *
 * Each boolean argument is converted to a `bit<T>` element of the
 * resulting vector. The width of the mask is `sizeof...(Args) + 1`.
 *
 * @tparam T     Element type of the underlying vector the mask applies to.
 * @tparam Args  Remaining boolean values.
 * @tparam Nout  Width of the resulting mask (deduced).
 * @param arg    First boolean value.
 * @param args   Remaining boolean values.
 * @return A `vec<bit<T>, Nout>` representing the mask.
 */
template <typename T, typename... Args, size_t Nout = (sizeof...(Args) + 1)>
constexpr KFR_INTRINSIC vec<bit<T>, Nout> make_mask(bool arg, Args... args)
{
    return vec<bit<T>, Nout>(arg, static_cast<bool>(args)...);
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
