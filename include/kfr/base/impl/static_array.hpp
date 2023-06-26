/** @addtogroup expressions
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

#include "../../cometa.hpp"
#include "../../kfr.h"

namespace kfr
{
using namespace cometa;

template <typename T, size_t>
using type_for = T;

template <typename T, typename indices_t>
struct static_array_base;

template <typename T, size_t Size>
using static_array_of_size = static_array_base<T, csizeseq_t<Size>>;

template <typename T, size_t... indices>
struct static_array_base<T, csizes_t<indices...>>
{
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using reference       = value_type&;
    using pointer         = value_type*;
    using iterator        = pointer;
    using const_reference = const value_type&;
    using const_pointer   = const value_type*;
    using const_iterator  = const_pointer;

    constexpr static size_t static_size = sizeof...(indices);

    constexpr static_array_base() noexcept : array{ (static_cast<void>(indices), 0)... } {}
    constexpr static_array_base(const static_array_base&) noexcept = default;
    constexpr static_array_base(static_array_base&&) noexcept      = default;

    KFR_MEM_INTRINSIC constexpr static_array_base(type_for<value_type, indices>... args) noexcept
        : array{ args... }
    {
    }

    template <typename U, typename otherindices_t>
    friend struct static_array_base;

    template <size_t... idx1, size_t... idx2>
    KFR_MEM_INTRINSIC constexpr static_array_base(
        const static_array_base<T, csizes_t<idx1...>>& first,
        const static_array_base<T, csizes_t<idx2...>>& second) noexcept
        : array{ (indices >= sizeof...(idx1) ? second.array[indices - sizeof...(idx1)]
                                             : first.array[indices])... }
    {
        static_assert(sizeof...(idx1) + sizeof...(idx2) == static_size);
    }

    template <size_t... idx>
    constexpr static_array_base<T, csizeseq_t<sizeof...(idx)>> shuffle(csizes_t<idx...>) const noexcept
    {
        return static_array_base<T, csizeseq_t<sizeof...(idx)>>{ array[idx]... };
    }
    template <size_t... idx>
    constexpr static_array_base<T, csizeseq_t<sizeof...(idx)>> shuffle(csizes_t<idx...>,
                                                                       T filler) const noexcept
    {
        return static_array_base<T, csizeseq_t<sizeof...(idx)>>{ (idx >= static_size ? filler
                                                                                     : array[idx])... };
    }

    template <size_t start, size_t size>
    constexpr static_array_base<T, csizeseq_t<size>> slice() const noexcept
    {
        return shuffle(csizeseq<size, start>);
    }

    constexpr static_array_base& operator=(const static_array_base&) = default;
    constexpr static_array_base& operator=(static_array_base&&) = default;

    template <int dummy = 0, CMT_ENABLE_IF(dummy == 0 && static_size > 1)>
    KFR_MEM_INTRINSIC constexpr explicit static_array_base(value_type value) noexcept
        : array{ (static_cast<void>(indices), value)... }
    {
    }

    KFR_MEM_INTRINSIC constexpr const value_type* data() const noexcept { return std::data(array); }
    KFR_MEM_INTRINSIC constexpr value_type* data() noexcept { return std::data(array); }

    KFR_MEM_INTRINSIC constexpr const_iterator begin() const noexcept { return std::begin(array); }
    KFR_MEM_INTRINSIC constexpr iterator begin() noexcept { return std::begin(array); }
    KFR_MEM_INTRINSIC constexpr const_iterator cbegin() const noexcept { return std::begin(array); }

    KFR_MEM_INTRINSIC constexpr const_iterator end() const noexcept { return std::end(array); }
    KFR_MEM_INTRINSIC constexpr iterator end() noexcept { return std::end(array); }
    KFR_MEM_INTRINSIC constexpr const_iterator cend() const noexcept { return std::end(array); }

    KFR_MEM_INTRINSIC constexpr const_reference operator[](size_t index) const noexcept
    {
        return array[index];
    }
    KFR_MEM_INTRINSIC constexpr reference operator[](size_t index) noexcept { return array[index]; }

    KFR_MEM_INTRINSIC constexpr const_reference front() const noexcept { return array[0]; }
    KFR_MEM_INTRINSIC constexpr reference front() noexcept { return array[0]; }

    KFR_MEM_INTRINSIC constexpr const_reference back() const noexcept { return array[static_size - 1]; }
    KFR_MEM_INTRINSIC constexpr reference back() noexcept { return array[static_size - 1]; }

    KFR_MEM_INTRINSIC constexpr bool empty() const noexcept { return false; }

    KFR_MEM_INTRINSIC constexpr size_t size() const noexcept { return std::size(array); }

    KFR_MEM_INTRINSIC constexpr bool operator==(const static_array_base& other) const noexcept
    {
        return ((array[indices] == other.array[indices]) && ...);
    }
    KFR_MEM_INTRINSIC constexpr bool operator!=(const static_array_base& other) const noexcept
    {
        return !operator==(other);
    }
    constexpr T minof() const noexcept
    {
        T result = std::numeric_limits<T>::max();
        (static_cast<void>(result = std::min(result, array[indices])), ...);
        return result;
    }
    constexpr T maxof() const noexcept
    {
        T result = std::numeric_limits<T>::lowest();
        (static_cast<void>(result = std::max(result, array[indices])), ...);
        return result;
    }
    constexpr T sum() const noexcept
    {
        T result = 0;
        (static_cast<void>(result += array[indices]), ...);
        return result;
    }
    constexpr T product() const noexcept
    {
        T result = 1;
        (static_cast<void>(result *= array[indices]), ...);
        return result;
    }

    constexpr static_array_base min(const static_array_base& y) const noexcept
    {
        return static_array_base{ std::min(array[indices], y.array[indices])... };
    }
    constexpr static_array_base max(const static_array_base& y) const noexcept
    {
        return static_array_base{ std::max(array[indices], y.array[indices])... };
    }
    template <typename Fn>
    constexpr static_array_base bin(const static_array_base& y, Fn&& fn) const noexcept
    {
        return static_array_base{ fn(array[indices], y.array[indices])... };
    }
    template <typename Fn>
    constexpr static_array_base un(Fn&& fn) const noexcept
    {
        return static_array_base{ fn(array[indices])... };
    }
    template <typename U>
    constexpr static_array_base<U, csizes_t<indices...>> cast() const noexcept
    {
        return static_array_base<U, csizes_t<indices...>>{ static_cast<U>(array[indices])... };
    }

    constexpr static_array_base operator+(const static_array_base& y) const noexcept
    {
        return static_array_base{ (array[indices] + y.array[indices])... };
    }
    constexpr static_array_base operator-(const static_array_base& y) const noexcept
    {
        return static_array_base{ (array[indices] - y.array[indices])... };
    }
    constexpr static_array_base operator*(const static_array_base& y) const noexcept
    {
        return static_array_base{ (array[indices] * y.array[indices])... };
    }
    constexpr static_array_base operator&(const static_array_base& y) const noexcept
    {
        return static_array_base{ (array[indices] & y.array[indices])... };
    }
    constexpr static_array_base operator|(const static_array_base& y) const noexcept
    {
        return static_array_base{ (array[indices] | y.array[indices])... };
    }
    constexpr static_array_base operator^(const static_array_base& y) const noexcept
    {
        return static_array_base{ (array[indices] ^ y.array[indices])... };
    }
    constexpr static_array_base operator+(const T& y) const noexcept
    {
        return static_array_base{ (array[indices] + y)... };
    }
    constexpr static_array_base operator-(const T& y) const noexcept
    {
        return static_array_base{ (array[indices] - y)... };
    }
    constexpr T dot(const static_array_base& y) const noexcept { return (operator*(y)).sum(); }

private:
    T array[static_size];
};
} // namespace kfr
