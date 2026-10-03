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

#include "memory.hpp"
#include <algorithm>
#include <cstdint>

namespace kfr
{
/**
 * @brief A dynamically-sized buffer with a small-size optimization.
 *
 * Stores up to @p Capacity elements inline without heap allocation. When the
 * requested size exceeds @p Capacity, storage is obtained from the aligned
 * allocator. The buffer is movable and copyable and keeps ownership of its
 * data.
 *
 * @tparam T        Element type.
 * @tparam Capacity Number of elements kept in the inline (preallocated) storage.
 */
template <typename T, std::size_t Capacity = 16>
struct small_buffer
{
public:
    /** @brief Constructs an empty buffer pointing at the inline storage. */
    small_buffer() noexcept : m_size(0), m_data(m_preallocated) {}

    /**
     * @brief Constructs a buffer with @p size default-initialized elements.
     * @param size Number of elements to allocate.
     */
    small_buffer(std::size_t size) : small_buffer() { resize(size); }

    /**
     * @brief Swaps the contents of two buffers.
     *
     * After the swap each buffer correctly points either at its own inline
     * storage or at its heap allocation depending on the resulting size.
     */
    friend void swap(small_buffer<T, Capacity>& first, small_buffer<T, Capacity>& second) noexcept
    {
        using std::swap;

        swap(first.m_size, second.m_size);
        swap(first.m_data, second.m_data);
        swap(first.m_preallocated, second.m_preallocated);
        first.m_data  = first.m_size <= Capacity ? first.m_preallocated : first.m_data;
        second.m_data = second.m_size <= Capacity ? second.m_preallocated : second.m_data;
    }
    /** @brief Move-constructs from @p other, leaving @p other in an empty state. */
    small_buffer(small_buffer<T, Capacity>&& other) : small_buffer() { swap(other, *this); }

    /** @brief Copy-constructs from @p other. */
    small_buffer(const small_buffer<T, Capacity>& other) : small_buffer() { assign(other); }
    /**
     * @brief Copy- and move-assigns from @p other using copy-and-swap.
     * @return Reference to @c *this.
     */
    small_buffer<T, Capacity>& operator=(small_buffer<T, Capacity> other)
    {
        swap(other, *this);
        return *this;
    }

    /** @brief Destructor; releases any heap allocation. */
    ~small_buffer() { clear(); }

    /**
     * @brief Replaces the contents with a copy of @p other.
     * @param other Buffer to copy from.
     */
    void assign(const small_buffer<T, Capacity>& other)
    {
        resize(other.m_size);
        std::copy_n(other.m_data, m_size, m_data);
    }

    /**
     * @brief Resizes the buffer to @p newsize elements.
     *
     * Existing elements are moved to the new storage. When @p newsize exceeds
     * @p Capacity the buffer switches to heap-allocated aligned storage; when
     * it shrinks back to @p Capacity or below, the inline storage is used
     * again and any heap allocation is released.
     * @param newsize New number of elements.
     */
    void resize(std::size_t newsize)
    {
        T* m_newdata;
        if (newsize <= Capacity)
        {
            m_newdata = m_preallocated;
        }
        else
        {
            m_newdata = aligned_allocate<T>(newsize);
        }
        if (m_newdata != m_data)
        {
            std::copy_n(std::make_move_iterator(m_data), std::min(newsize, m_size), m_newdata);
        }
        if (m_data != m_preallocated)
            aligned_deallocate(m_data);
        m_data = m_newdata;
        m_size = newsize;
    }
    /** @return @c true if the buffer contains no elements. */
    bool empty() const { return !size(); }
    /** @return Number of elements currently stored. */
    std::size_t size() const { return m_size; }
    /** @return Const pointer to the first element. */
    const T* begin() const { return m_data; }
    /** @return Const pointer one past the last element. */
    const T* end() const { return m_data + m_size; }
    /** @return Const pointer to the first element. */
    const T* cbegin() const { return m_data; }
    /** @return Const pointer one past the last element. */
    const T* cend() const { return m_data + m_size; }
    /** @return Pointer to the first element. */
    T* begin() { return m_data; }
    /** @return Pointer one past the last element. */
    T* end() { return m_data + m_size; }
    /** @brief Resets the buffer to zero elements, releasing any heap allocation. */
    void clear() { resize(0); }
    /** @return Const reference to the first element. */
    const T& front() const { return m_data[0]; }
    /** @return Const reference to the last element. */
    const T& back() const { return m_data[m_size - 1]; }
    /** @return Reference to the first element. */
    T& front() { return m_data[0]; }
    /** @return Reference to the last element. */
    T& back() { return m_data[m_size - 1]; }
    /** @brief Removes the last element. */
    void pop_back() { resize(m_size - 1); }
    /** @return Pointer to the underlying element storage. */
    T* data() { return m_data; }
    /** @return Const pointer to the underlying element storage. */
    const T* data() const { return m_data; }
    /** @return Reference to the element at index @p i. */
    T& operator[](std::size_t i) { return m_data[i]; }
    /** @return Const reference to the element at index @p i. */
    const T& operator[](std::size_t i) const { return m_data[i]; }
    /**
     * @brief Appends @p value to the end of the buffer.
     * @param value Value to append.
     */
    void push_back(const T& value)
    {
        resize(m_size + 1);
        m_data[m_size - 1] = value;
    }

protected:
    T m_preallocated[Capacity]; ///< Inline storage for up to @p Capacity elements.
    std::size_t m_size; ///< Number of elements currently stored.
    T* m_data; ///< Pointer to the active storage (inline or heap).
};
} // namespace kfr
