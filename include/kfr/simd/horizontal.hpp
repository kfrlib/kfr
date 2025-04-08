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
#include "min_max.hpp"

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

/**
 * @brief Applies a reduction function horizontally across all elements of the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @tparam ReduceFn The reduction function type.
 * @param value The input vector.
 * @param reduce The reduction function to apply.
 * @return The result of the reduction across all elements.
 */
template <typename T, size_t N, typename ReduceFn>
KFR_INTRINSIC T horizontal(const vec<T, N>& value, ReduceFn&& reduce)
{
    return intrinsics::horizontal_impl(value, std::forward<ReduceFn>(reduce));
}

/**
 * @brief Computes the sum of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The sum of all vector elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hadd(const vec<T, N>& value)
{
    return horizontal(value, fn::add());
}
KFR_FN(hadd)

/**
 * @brief Computes the sum of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The sum of all vector elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hsum(const vec<T, N>& value)
{
    return horizontal(value, fn::add());
}
KFR_FN(hsum)

/**
 * @brief Computes the product of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The product of all vector elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hmul(const vec<T, N>& value)
{
    return horizontal(value, fn::mul());
}
KFR_FN(hmul)

/**
 * @brief Computes the product of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The product of all vector elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hproduct(const vec<T, N>& value)
{
    return horizontal(value, fn::mul());
}
KFR_FN(hproduct)

/**
 * @brief Computes the bitwise AND of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The result of bitwise AND across all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hbitwiseand(const vec<T, N>& value)
{
    return horizontal(value, fn::bitwiseand());
}
KFR_FN(hbitwiseand)

/**
 * @brief Computes the bitwise OR of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The result of bitwise OR across all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hbitwiseor(const vec<T, N>& value)
{
    return horizontal(value, fn::bitwiseor());
}
KFR_FN(hbitwiseor)

/**
 * @brief Computes the bitwise XOR of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The result of bitwise XOR across all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hbitwisexor(const vec<T, N>& value)
{
    return horizontal(value, fn::bitwisexor());
}
KFR_FN(hbitwisexor)

/**
 * @brief Computes the dot product of two vectors.
 *
 * @tparam T The element type of the vectors.
 * @tparam N The number of elements in the vectors.
 * @param x The first vector.
 * @param y The second vector.
 * @return The dot product of x and y.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hdot(const vec<T, N>& x, const vec<T, N>& y)
{
    return hadd(x * y);
}
KFR_FN(hdot)

/**
 * @brief Computes the arithmetic mean (average) of all elements in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The average value of all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T havg(const vec<T, N>& value)
{
    return hadd(value) / N;
}
KFR_FN(havg)

/**
 * @brief Computes the root-mean-square (RMS) of the vector elements.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The RMS value of all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hrms(const vec<T, N>& value)
{
    return builtin_sqrt(hadd(value * value) / N);
}
KFR_FN(hrms)

/**
 * @brief Computes the minimum element in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The minimum value among all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hmin(const vec<T, N>& value)
{
    return horizontal(value, fn::min());
}
KFR_FN(hmin)

/**
 * @brief Computes the maximum element in the vector.
 *
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param value The input vector.
 * @return The maximum value among all elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC T hmax(const vec<T, N>& value)
{
    return horizontal(value, fn::max());
}
KFR_FN(hmax)

} // namespace CMT_ARCH_NAME
} // namespace kfr
