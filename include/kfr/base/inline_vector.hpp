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

#include <cstdlib>
#include <cstdint>
#include <type_traits>
#include <algorithm>

namespace kfr
{

template <typename T, size_t N>
struct inline_vector
{
    static_assert(N > 0 && N <= INT32_MAX);
    static_assert(std::is_trivially_copy_constructible_v<T>);
    // it's ok for T to be non-trivially default constructible as long as it's trivial in any other aspect
    static_assert(std::is_trivially_copy_assignable_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);
    static_assert(std::is_trivially_move_constructible_v<T>);
    static_assert(std::is_trivially_destructible_v<T>);

    using size_type        = size_t;
    using stored_size_type = std::conditional_t<(N >= UINT16_MAX), uint32_t,
                                                std::conditional_t<(N >= UINT8_MAX), uint16_t, uint8_t>>;
    using value_type       = T;
    using pointer          = T*;
    using const_pointer    = const T*;
    using reference        = T&;
    using const_reference  = const T&;
    using iterator         = pointer;
    using const_iterator   = const_pointer;

    constexpr inline_vector() noexcept : m_size(0) {}
    constexpr inline_vector(const inline_vector&) noexcept            = default;
    constexpr inline_vector(inline_vector&&) noexcept                 = default;
    constexpr inline_vector& operator=(const inline_vector&) noexcept = default;
    constexpr inline_vector& operator=(inline_vector&&) noexcept      = default;

    constexpr inline_vector(size_type initial_size) : m_size(initial_size)
    {
        KFR_LOGIC_CHECK(initial_size <= N, "inline_vector: invalid initial_size");
    }
    constexpr inline_vector(size_type initial_size, T initial_value) : m_size(initial_size)
    {
        KFR_LOGIC_CHECK(initial_size <= N, "inline_vector: invalid initial_size");
        std::fill_n(begin(), initial_size, initial_value);
    }
    constexpr inline_vector(std::initializer_list<T> list) : inline_vector(list.begin(), list.end()) {}

    template <typename Iter>
    constexpr inline_vector(Iter first, Iter last)
    {
        iterator dest = begin();
        size_t i      = 0;
        for (; i < N && first != last; ++i)
            *dest++ = *first++;
        KFR_LOGIC_CHECK(first == last, "inline_vector: too many items");
        m_size = i;
    }

    constexpr const_reference at(size_type index) const
    {
        KFR_LOGIC_CHECK(index < m_size, "inline_vector: invalid index");
        return m_values[index];
    }
    constexpr reference at(size_type index)
    {
        KFR_LOGIC_CHECK(index < m_size, "inline_vector: invalid index");
        return m_values[index];
    }

    constexpr const_reference operator[](size_type index) const noexcept { return m_values[index]; }
    constexpr reference operator[](size_type index) noexcept { return m_values[index]; }

    constexpr size_type size() const noexcept { return m_size; }
    constexpr pointer data() noexcept { return m_values; }
    constexpr const_pointer data() const noexcept { return m_values; }
    constexpr bool empty() const noexcept { return m_size == 0; }
    constexpr iterator begin() noexcept { return m_values; }
    constexpr iterator end() noexcept { return m_values + m_size; }
    constexpr const_iterator begin() const noexcept { return m_values; }
    constexpr const_iterator end() const noexcept { return m_values + m_size; }
    constexpr const_iterator cbegin() const noexcept { return m_values; }
    constexpr const_iterator cend() const noexcept { return m_values + m_size; }

    constexpr reference front() noexcept { return m_values[0]; }
    constexpr reference back() noexcept { return m_values[m_size - 1]; }
    constexpr const_reference front() const noexcept { return m_values[0]; }
    constexpr const_reference back() const noexcept { return m_values[m_size - 1]; }

    constexpr void push_back(T value)
    {
        KFR_LOGIC_CHECK(m_size < N, "inline_vector: vector is full");
        m_values[m_size++] = value;
    }

    constexpr bool operator==(const inline_vector& other) const
    {
        return std::equal(begin(), end(), other.begin());
    }
    constexpr bool operator!=(const inline_vector& other) const { return !operator==(other); }

    T m_values[N];
    stored_size_type m_size;
};
} // namespace kfr
