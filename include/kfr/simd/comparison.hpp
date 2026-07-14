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

#include "constants.hpp"
#include "impl/function.hpp"
#include "vec.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Element-wise equality comparison.
 * @param x Left-hand operand.
 * @param y Right-hand operand.
 * @return Mask of elements where `x == y`.
 */
template <typename T1, typename T2>
inline maskfor<std::common_type_t<T1, T2>> equal(const T1& x, const T2& y)
{
    return x == y;
}
/**
 * @brief Element-wise inequality comparison.
 * @param x Left-hand operand.
 * @param y Right-hand operand.
 * @return Mask of elements where `x != y`.
 */
template <typename T1, typename T2>
inline maskfor<std::common_type_t<T1, T2>> notequal(const T1& x, const T2& y)
{
    return x != y;
}
/**
 * @brief Element-wise less-than comparison.
 * @param x Left-hand operand.
 * @param y Right-hand operand.
 * @return Mask of elements where `x < y`.
 */
template <typename T1, typename T2>
inline maskfor<std::common_type_t<T1, T2>> less(const T1& x, const T2& y)
{
    return x < y;
}
/**
 * @brief Element-wise greater-than comparison.
 * @param x Left-hand operand.
 * @param y Right-hand operand.
 * @return Mask of elements where `x > y`.
 */
template <typename T1, typename T2>
inline maskfor<std::common_type_t<T1, T2>> greater(const T1& x, const T2& y)
{
    return x > y;
}
/**
 * @brief Element-wise less-or-equal comparison.
 * @param x Left-hand operand.
 * @param y Right-hand operand.
 * @return Mask of elements where `x <= y`.
 */
template <typename T1, typename T2>
inline maskfor<std::common_type_t<T1, T2>> lessorequal(const T1& x, const T2& y)
{
    return x <= y;
}
/**
 * @brief Element-wise greater-or-equal comparison.
 * @param x Left-hand operand.
 * @param y Right-hand operand.
 * @return Mask of elements where `x >= y`.
 */
template <typename T1, typename T2>
inline maskfor<std::common_type_t<T1, T2>> greaterorequal(const T1& x, const T2& y)
{
    return x >= y;
}
KFR_FN(equal)
KFR_FN(notequal)
KFR_FN(less)
KFR_FN(greater)
KFR_FN(lessorequal)
KFR_FN(greaterorequal)

/**
 * @brief Detect NaN (Not-a-Number) values element-wise.
 *
 * A value is NaN if it is not equal to itself.
 * @param x Input vector.
 * @return Mask of elements where `x` is NaN.
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isnan(const vec<T, N>& x)
{
    return x != x;
}

/**
 * @brief Detect infinite values element-wise.
 *
 * Matches positive or negative infinity.
 * @param x Input vector.
 * @return Mask of elements where `x` is infinite.
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isinf(const vec<T, N>& x)
{
    return x == constants<T>::infinity || x == -constants<T>::infinity;
}

/**
 * @brief Detect finite values element-wise.
 *
 * A value is finite if it is neither NaN nor infinite.
 * @param x Input vector.
 * @return Mask of elements where `x` is finite.
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isfinite(const vec<T, N>& x)
{
    return !isnan(x) && !isinf(x);
}

/**
 * @brief Detect negative values element-wise by inspecting the sign bit.
 *
 * Tests the high (sign) bit of each element's representation, so it
 * distinguishes `-0.0` from `+0.0`.
 * @param x Input vector.
 * @return Mask of elements whose sign bit is set.
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> isnegative(const vec<T, N>& x)
{
    return (x & special_constants<T>::highbitmask()) != 0;
}

/**
 * @brief Detect non-negative values element-wise.
 *
 * The complement of @ref isnegative.
 * @param x Input vector.
 * @return Mask of elements whose sign bit is clear.
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> ispositive(const vec<T, N>& x)
{
    return !isnegative(x);
}

/**
 * @brief Detect zero values element-wise.
 * @param x Input vector.
 * @return Mask of elements equal to zero.
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> iszero(const vec<T, N>& x)
{
    return x == T();
}

/**
 * @brief Test whether each element lies within a closed range.
 * @param x Value to test.
 * @param min Lower bound (inclusive).
 * @param max Upper bound (inclusive).
 * @return Mask of elements where `min <= x <= max`.
 */
template <typename T1, typename T2, typename T3>
KFR_INTRINSIC maskfor<std::common_type_t<T1, T2, T3>> inrange(const T1& x, const T2& min, const T3& max)
{
    return x >= min && x <= max;
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
