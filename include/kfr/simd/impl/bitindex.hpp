/** @addtogroup basic_math
 *  @{
 */
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

#include "../types.hpp"

namespace kfr
{

template <size_t k>
using bitperm = std::array<uint8_t, k>;

template <size_t k>
constexpr bitperm<k> bitperm_natural() noexcept
{
    bitperm<k> p{};
    std::iota(p.begin(), p.end(), 0);
    return p;
}

namespace internal_generic
{

template <uint8_t b0, uint8_t b1>
constexpr size_t swap_bit_indices(size_t i)
{
    constexpr size_t mask0 = 1u << b0;
    constexpr size_t mask1 = 1u << b1;
    return ((i & ~mask0 & ~mask1) | ((i & mask0) ? mask1 : 0) | ((i & mask1) ? mask0 : 0));
}

template <size_t k>
constexpr size_t shuffle_bits(size_t i, const bitperm<k>& perm)
{
    size_t result = 0;
    for (size_t j = 0; j < k; ++j)
        result |= ((i >> j) & 1) << perm[j];
    return result;
}

template <size_t N, size_t k = std::countr_zero(N)>
constexpr bitperm<k> to_bit_indices(const std::array<size_t, N>& indices)
{
    static_assert(std::has_single_bit(N));
    if (indices.front() != 0 || indices.back() != N - 1)
        return {};
    bitperm<k> result{};
    for (size_t i = 0; i < k; ++i)
    {
        if (!std::has_single_bit(indices[1 << i]))
            return {};
        result[i] = static_cast<uint8_t>(std::countr_zero(indices[1 << i]));
    }
    for (size_t i = 0; i < N; ++i)
        if (indices[i] != shuffle_bits(i, result))
            return {};
    return result;
}

template <size_t k, size_t N = 1u << k>
constexpr std::array<size_t, N> from_bit_indices(const bitperm<k>& bit_indices)
{
    std::array<size_t, N> result{};
    for (size_t i = 0; i < N; ++i)
        result[i] = shuffle_bits(i, bit_indices);
    return result;
}

template <size_t k, bitperm<k> bit_indices>
constexpr auto to_elements()
{
    return []<size_t... I>(csizes_t<I...>)
    { return elements_t<shuffle_bits(I, bit_indices)...>{}; }(csizeseq_t<(1u << k)>{});
}

template <typename T, size_t N>
constexpr bool is_permutation(std::array<T, N> arr)
{
    std::array<bool, N> seen{};
    for (auto f : arr)
    {
        if (f < 0 || f >= N || seen[static_cast<size_t>(f)])
            return false;
        seen[static_cast<size_t>(f)] = true;
    }
    return true;
}

constexpr uint64_t factorial(uint32_t n)
{
    uint64_t res = 1;
    for (uint32_t i = 2; i <= n; ++i)
        res *= i;

    return res;
}

} // namespace internal_generic

} // namespace kfr
