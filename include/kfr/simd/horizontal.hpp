/** @addtogroup horizontal
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

#include "operators.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename T, typename ReduceFn>
KFR_INTRINSIC T horizontal_impl(const vec<T, 1>& value, ReduceFn&&)
{
    return T(value.front());
}

template <typename T, size_t N, typename ReduceFn, KFR_ENABLE_IF(N > 1 && is_poweroftwo(N))>
KFR_INTRINSIC T horizontal_impl(const vec<T, N>& value, ReduceFn&& reduce)
{
    return horizontal_impl(reduce(low(value), high(value)), std::forward<ReduceFn>(reduce));
}
template <typename T, size_t N, typename ReduceFn, KFR_ENABLE_IF(N > 1 && !is_poweroftwo(N))>
KFR_INTRINSIC T horizontal_impl(const vec<T, N>& value, ReduceFn&& reduce)
{
    const T initial = reduce(initialvalue<T>());
    return horizontal_impl(widen<next_poweroftwo(N)>(value, initial), std::forward<ReduceFn>(reduce));
}
} // namespace intrinsics

template <typename T, size_t N, typename ReduceFn>
KFR_INTRINSIC T horizontal(const vec<T, N>& value, ReduceFn&& reduce)
{
    return intrinsics::horizontal_impl(value, std::forward<ReduceFn>(reduce));
}

/// @brief Sum all elements of the vector
template <typename T, size_t N>
KFR_INTRINSIC T hadd(const vec<T, N>& value)
{
    return horizontal(value, fn::add());
}
KFR_FN(hadd)

/// @brief Sum all elements of the vector
template <typename T, size_t N>
KFR_INTRINSIC T hsum(const vec<T, N>& value)
{
    return horizontal(value, fn::add());
}
KFR_FN(hsum)

/// @brief Multiply all elements of the vector
template <typename T, size_t N>
KFR_INTRINSIC T hmul(const vec<T, N>& value)
{
    return horizontal(value, fn::mul());
}
KFR_FN(hmul)

/// @brief Multiply all elements of the vector
template <typename T, size_t N>
KFR_INTRINSIC T hproduct(const vec<T, N>& value)
{
    return horizontal(value, fn::mul());
}
KFR_FN(hproduct)

template <typename T, size_t N>
KFR_INTRINSIC T hbitwiseand(const vec<T, N>& value)
{
    return horizontal(value, fn::bitwiseand());
}
KFR_FN(hbitwiseand)
template <typename T, size_t N>
KFR_INTRINSIC T hbitwiseor(const vec<T, N>& value)
{
    return horizontal(value, fn::bitwiseor());
}
KFR_FN(hbitwiseor)
template <typename T, size_t N>
KFR_INTRINSIC T hbitwisexor(const vec<T, N>& value)
{
    return horizontal(value, fn::bitwisexor());
}
KFR_FN(hbitwisexor)

/// @brief Calculate the Dot-Product of two vectors
template <typename T, size_t N>
KFR_INTRINSIC T hdot(const vec<T, N>& x, const vec<T, N>& y)
{
    return hadd(x * y);
}
KFR_FN(hdot)

/// @brief Calculate the Arithmetic mean of all elements in the vector
template <typename T, size_t N>
KFR_INTRINSIC T havg(const vec<T, N>& value)
{
    return hadd(value) / N;
}
KFR_FN(havg)

/// @brief Calculate the RMS of all elements in the vector
template <typename T, size_t N>
KFR_INTRINSIC T hrms(const vec<T, N>& value)
{
    return builtin_sqrt(hadd(value * value) / N);
}
KFR_FN(hrms)
} // namespace CMT_ARCH_NAME
} // namespace kfr
