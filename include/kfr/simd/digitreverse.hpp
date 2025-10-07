/** @addtogroup shuffle
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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
#include "shuffle.hpp"
#include "types.hpp"

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

namespace internal
{

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshift-count-overflow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshift-count-negative")

constexpr KFR_INTRINSIC u32 bit_permute_step_impl(u32 x, cvals_t<u32>) { return x; }

template <u32 m, u32 shift, u32... values>
constexpr KFR_INTRINSIC u32 bit_permute_step_impl(u32 x, cvals_t<u32, m, shift, values...>)
{
    return bit_permute_step_impl(((x & m) << shift) | ((x >> shift) & m), cvals_t<u32, values...>());
}

template <size_t bits>
constexpr KFR_INTRINSIC u32 digitreverse_impl(u32 x, csize_t<2>)
{
    constexpr cvals_t<u32, 0x55555555, 1, 0x33333333, 2, 0x0f0f0f0f, 4, 0x00ff00ff, 8, 0x0000ffff, 16>
        steps{};
    if constexpr (bits > 16)
        return bit_permute_step_impl(x, steps) >> (32 - bits);
    else if constexpr (bits > 8)
        return bit_permute_step_impl(x, steps[csizeseq<8>]) >> (16 - bits);
    else
        return bit_permute_step_impl(x, steps[csizeseq<6>]) >> (8 - bits);
}

template <size_t bits>
constexpr KFR_INTRINSIC u32 digitreverse_impl(u32 x, csize_t<4>)
{
    constexpr cvals_t<u32, 0x33333333, 2, 0x0f0f0f0f, 4, 0x00ff00ff, 8, 0x0000ffff, 16> steps{};
    if constexpr (bits > 16)
        return bit_permute_step_impl(x, steps) >> (32 - bits);
    else if constexpr (bits > 8)
        return bit_permute_step_impl(x, steps[csizeseq<6>]) >> (16 - bits);
    else
        return bit_permute_step_impl(x, steps[csizeseq<4>]) >> (8 - bits);
}

KFR_PRAGMA_GNU(GCC diagnostic pop)

template <size_t radix, size_t bits>
struct shuffle_index_digitreverse
{
    constexpr KFR_INTRINSIC size_t operator()(size_t index) const noexcept
    {
        return digitreverse_impl<bits>(static_cast<u32>(index), csize_t<radix>());
    }
};
} // namespace internal

/**
 * @brief Reorders the elements of a vector by reversing the digits of their indices in the specified radix.
 *
 * Optionally groups elements before reversing.
 *
 * @tparam radix The numeric base used for digit reversal (2 or 4).
 * @tparam group The grouping size; elements are grouped before index reversal.
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param x The input vector to be reordered.
 * @return A vector with elements reordered based on digit-reversed indices.
 */
template <size_t radix, size_t group = 1, typename T, size_t N>
KFR_INTRINSIC vec<T, N> digitreverse(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(
        csizeseq<N / group>.map(internal::shuffle_index_digitreverse<radix, ilog2(N / group)>())));
}

/**
 * @brief Reorders the elements of a vector by reversing the bits of their indices.
 *
 * A specialization of digitreverse with radix 2 (binary). Optionally supports grouped bit reversal.
 *
 * @tparam groupsize Number of elements per group before applying bit reversal.
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param x The input vector to be reordered.
 * @return A vector with bit-reversed indices.
 */
template <size_t groupsize = 1, typename T, size_t N>
KFR_INTRINSIC vec<T, N> bitreverse(const vec<T, N>& x)
{
    return digitreverse<2, groupsize>(x);
}

/**
 * @brief Reorders the elements of a vector by reversing base-4 (quaternary) digits of their indices.
 *
 * Similar to bitreverse, but uses radix 4 for digit reversal.
 *
 * @tparam groupsize Number of elements per group before applying reversal.
 * @tparam T The element type of the vector.
 * @tparam N The number of elements in the vector.
 * @param x The input vector to be reordered.
 * @return A vector with base-4 digit-reversed indices.
 */
template <size_t groupsize = 1, typename T, size_t N>
KFR_INTRINSIC vec<T, N> digitreverse4(const vec<T, N>& x)
{
    return digitreverse<4, groupsize>(x);
}

/**
 * @brief Reverses the lowest `bits` bits of the given unsigned integer.
 *
 * @tparam bits Number of bits to reverse.
 * @param x The input 32-bit unsigned integer.
 * @return The bit-reversed integer.
 */
template <size_t bits>
constexpr KFR_INTRINSIC u32 bitreverse(u32 x)
{
    return internal::digitreverse_impl<bits>(x, csize_t<2>());
}

/**
 * @brief Reverses the digits of the given unsigned integer in base-4 (quaternary), using the lowest `bits`
 * digits.
 *
 * Useful for radix-4 FFT or similar algorithms.
 *
 * @tparam bits Number of base-4 digits to reverse.
 * @param x The input 32-bit unsigned integer.
 * @return The base-4 digit-reversed integer.
 */
template <size_t bits>
constexpr KFR_INTRINSIC u32 digitreverse4(u32 x)
{
    return internal::digitreverse_impl<bits>(x, csize_t<4>());
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
