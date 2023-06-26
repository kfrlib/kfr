/** @addtogroup types
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

#include "memory.hpp"
#include <algorithm>
#include <cstdint>

namespace kfr
{
template <typename T, std::size_t Capacity = 16>
struct small_buffer
{
public:
    small_buffer() CMT_NOEXCEPT : m_size(0), m_data(m_preallocated) {}

    small_buffer(std::size_t size) : small_buffer() { resize(size); }

    friend void swap(small_buffer<T, Capacity>& first, small_buffer<T, Capacity>& second) CMT_NOEXCEPT
    {
        using std::swap;

        swap(first.m_size, second.m_size);
        swap(first.m_data, second.m_data);
        swap(first.m_preallocated, second.m_preallocated);
        first.m_data  = first.m_size <= Capacity ? first.m_preallocated : first.m_data;
        second.m_data = second.m_size <= Capacity ? second.m_preallocated : second.m_data;
    }
    small_buffer(small_buffer<T, Capacity>&& other) : small_buffer() { swap(other, *this); }

    small_buffer(const small_buffer<T, Capacity>& other) : small_buffer() { assign(other); }
    small_buffer<T, Capacity>& operator=(small_buffer<T, Capacity> other)
    {
        swap(other, *this);
        return *this;
    }

    ~small_buffer() { clear(); }

    void assign(const small_buffer<T, Capacity>& other)
    {
        resize(other.m_size);
        std::copy_n(other.m_data, m_size, m_data);
    }

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
        std::copy_n(std::make_move_iterator(m_data), std::min(newsize, m_size), m_newdata);
        if (m_data != m_preallocated)
            aligned_deallocate(m_data);
        m_data = m_newdata;
        m_size = newsize;
    }
    bool empty() const { return !size(); }
    std::size_t size() const { return m_size; }
    const T* begin() const { return m_data; }
    const T* end() const { return m_data + m_size; }
    const T* cbegin() const { return m_data; }
    const T* cend() const { return m_data + m_size; }
    T* begin() { return m_data; }
    T* end() { return m_data + m_size; }
    void clear() { resize(0); }
    const T& front() const { return m_data[0]; }
    const T& back() const { return m_data[m_size - 1]; }
    T& front() { return m_data[0]; }
    T& back() { return m_data[m_size - 1]; }
    void pop_back() { resize(m_size - 1); }
    T* data() { return m_data; }
    const T* data() const { return m_data; }
    T& operator[](std::size_t i) { return m_data[i]; }
    const T& operator[](std::size_t i) const { return m_data[i]; }
    void push_back(const T& value)
    {
        resize(m_size + 1);
        m_data[m_size - 1] = value;
    }

protected:
    T m_preallocated[Capacity];
    std::size_t m_size;
    T* m_data;
};
} // namespace kfr
