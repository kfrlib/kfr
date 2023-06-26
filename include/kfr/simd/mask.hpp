/** @addtogroup logical
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

#include "vec.hpp"

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

template <typename T>
using maskfor = typename T::mask_t;

namespace internal
{

template <typename T, size_t Nout, size_t N1, size_t... indices>
constexpr vec<T, Nout> partial_mask_helper(csizes_t<indices...>)
{
    return make_vector(maskbits<T>(indices < N1)...);
}

template <typename T, size_t Nout, size_t N1>
constexpr vec<T, Nout> partial_mask()
{
    return internal::partial_mask_helper<T, Nout, N1>(csizeseq_t<Nout>());
}
} // namespace internal

template <typename T, typename... Args, size_t Nout = (sizeof...(Args) + 1)>
constexpr KFR_INTRINSIC vec<bit<T>, Nout> make_mask(bool arg, Args... args)
{
    return vec<bit<T>, Nout>(arg, static_cast<bool>(args)...);
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
