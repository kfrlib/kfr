/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../cometa/array.hpp"

#include "function.hpp"
#include "memory.hpp"
#include "read_write.hpp"
#include "types.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{

constexpr size_t tag_array_ref      = 0;
constexpr size_t tag_dynamic_vector = max_size_t;

/**
 * @brief Class that represent data in KFR. Many KFR functions can take this class as an argument.
 * Can inherit from std::vector, std::array or keep only reference to data and its size.
 *
 * univector<float> is inherited from std::vector<float>
 * univector<float, 10> is inherited from std::array<float, 10>
 * univector<float, 0> contains only reference to data
 *
 * To convert a plain pointer to univector, call make_univector:
 * @code
 * double* buffer;
 * size_t size;
 * univector<double, 0> v = make_univector(buffer, size);
 * // or pass result vector directly to a function:
 * some_function(make_univector(buffer, size));
 * @endcode
 */
template <typename T, size_t Size = tag_dynamic_vector>
struct univector;

template <typename T, typename Class>
struct univector_base : input_expression, output_expression
{
    using input_expression::begin_block;
    using input_expression::end_block;
    using output_expression::begin_block;
    using output_expression::end_block;

    template <typename U, size_t N>
    CMT_INLINE void operator()(coutput_t, size_t index, const vec<U, N>& value)
    {
        T* data = derived_cast<Class>(this)->data();
        write(ptr_cast<T>(data) + index, vec<T, N>(value));
    }
    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N>) const
    {
        const T* data = derived_cast<Class>(this)->data();
        return static_cast<vec<U, N>>(read<N>(ptr_cast<T>(data) + index));
    }

    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>::value)>
    CMT_INLINE Class& operator=(Input&& input)
    {
        assign_expr(std::forward<Input>(input));
        return *derived_cast<Class>(this);
    }
    univector<T, 0> slice(size_t start = 0, size_t size = max_size_t)
    {
        T* data                = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<T>(data + start, std::min(size, this_size - start));
    }
    univector<const T, 0> slice(size_t start = 0, size_t size = max_size_t) const
    {
        const T* data          = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<const T>(data + start, std::min(size, this_size - start));
    }
    univector<T, 0> truncate(size_t size = max_size_t)
    {
        T* data                = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<T>(data, std::min(size, this_size));
    }
    univector<const T, 0> truncate(size_t size = max_size_t) const
    {
        const T* data          = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<const T>(data, std::min(size, this_size));
    }

    array_ref<T> ref()
    {
        T* data           = get_data();
        const size_t size = get_size();
        return array_ref<T>(data, size);
    }
    array_ref<const T> ref() const
    {
        const T* data     = get_data();
        const size_t size = get_size();
        return array_ref<const T>(data, size);
    }

    void ringbuf_write(size_t& cursor, const T* src, size_t srcsize)
    {
        if (srcsize == 0)
            return;
        // skip redundant data
        const size_t size = get_size();
        T* data           = get_data();
        if (srcsize > size)
        {
            src     = src + srcsize / size;
            srcsize = srcsize % size;
        }
        const size_t fsize = size - cursor;
        // one fragment
        if (srcsize <= fsize)
        {
            copy(data + cursor, src, srcsize);
        }
        else // two fragments
        {
            copy(data + cursor, src, fsize);
            copy(data, src + fsize, srcsize - fsize);
        }
        ringbuf_step(cursor, srcsize);
    }
    template <size_t N>
    void ringbuf_write(size_t& cursor, const vec<T, N>& x)
    {
        ringbuf_write(cursor, ptr_cast<T>(&x), N);
    }
    void ringbuf_write(size_t& cursor, const T& value)
    {
        T* data      = get_data();
        data[cursor] = value;
        ringbuf_step(cursor, 1);
    }
    void ringbuf_step(size_t& cursor, size_t step) const
    {
        const size_t size = get_size();
        cursor            = cursor + step;
        cursor            = cursor >= size ? cursor - size : cursor;
    }
    void ringbuf_read(size_t& cursor, T& value)
    {
        T* data = get_data();
        value   = data[cursor];
        ringbuf_step(cursor, 1);
    }
    template <size_t N>
    void ringbuf_read(size_t& cursor, vec<T, N>& x)
    {
        ringbuf_read(cursor, ptr_cast<T>(&x), N);
    }
    void ringbuf_read(size_t& cursor, T* dest, size_t destsize) const
    {
        if (destsize == 0)
            return;
        // skip redundant data
        const size_t size = get_size();
        const T* data     = get_data();
        if (destsize > size)
        {
            dest     = dest + destsize / size;
            destsize = destsize % size;
        }
        const size_t fsize = size - cursor;
        // one fragment
        if (destsize <= fsize)
        {
            copy(dest, data + cursor, destsize);
        }
        else // two fragments
        {
            copy(dest, data + cursor, fsize);
            copy(dest + fsize, data, destsize - fsize);
        }
        ringbuf_step(cursor, destsize);
    }

protected:
    template <typename Input>
    CMT_INLINE void assign_expr(Input&& input)
    {
        process(*derived_cast<Class>(this), std::forward<Input>(input));
    }

private:
    CMT_INLINE size_t get_size() const { return derived_cast<Class>(this)->size(); }
    CMT_INLINE const T* get_data() const { return derived_cast<Class>(this)->data(); }
    CMT_INLINE T* get_data() { return derived_cast<Class>(this)->data(); }

    static void copy(T* dest, const T* src, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            *dest++ = *src++;
    }
};

template <typename T, size_t Size>
struct alignas(platform<>::maximum_vector_alignment) univector : std::array<T, Size>,
                                                                 univector_base<T, univector<T, Size>>
{
    using std::array<T, Size>::size;
    using size_type = size_t;
    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>::value)>
    univector(Input&& input)
    {
        this->assign_expr(std::forward<Input>(input));
    }
    template <typename... Args>
    constexpr univector(const T& x, const Args&... args) noexcept
        : std::array<T, Size>{ { x, static_cast<T>(args)... } }
    {
    }

    constexpr univector() noexcept(noexcept(std::array<T, Size>())) = default;
    constexpr univector(size_t, const T& value) { std::fill(this->begin(), this->end(), value); }
    constexpr static bool size_known   = true;
    constexpr static bool is_array     = true;
    constexpr static bool is_array_ref = false;
    constexpr static bool is_vector    = false;
    constexpr static bool is_aligned   = true;
    constexpr static bool is_pod       = kfr::is_pod<T>::value;
    using value_type                   = T;

    value_type get(size_t index, value_type fallback_value) const noexcept
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector>::operator=;
};

template <typename T>
struct univector<T, tag_array_ref> : array_ref<T>, univector_base<T, univector<T, tag_array_ref>>
{
    using array_ref<T>::size;
    using array_ref<T>::array_ref;
    using size_type = size_t;
    constexpr univector(const array_ref<T>& other) : array_ref<T>(other) {}
    constexpr univector(array_ref<T>&& other) : array_ref<T>(std::move(other)) {}

    template <size_t Tag>
    constexpr univector(const univector<T, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    template <size_t Tag>
    constexpr univector(univector<T, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    template <typename U, size_t Tag, KFR_ENABLE_IF(is_same<remove_const<T>, U>::value&& is_const<T>::value)>
    constexpr univector(const univector<U, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    template <typename U, size_t Tag, KFR_ENABLE_IF(is_same<remove_const<T>, U>::value&& is_const<T>::value)>
    constexpr univector(univector<U, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    void resize(size_t) noexcept {}
    constexpr static bool size_known   = false;
    constexpr static bool is_array     = false;
    constexpr static bool is_array_ref = true;
    constexpr static bool is_vector    = false;
    constexpr static bool is_aligned   = false;
    using value_type                   = remove_const<T>;

    value_type get(size_t index, value_type fallback_value) const noexcept
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector>::operator=;
};

template <typename T>
struct univector<T, tag_dynamic_vector> : std::vector<T, allocator<T>>,
                                          univector_base<T, univector<T, tag_dynamic_vector>>
{
    using std::vector<T, allocator<T>>::size;
    using std::vector<T, allocator<T>>::vector;
    using size_type = size_t;
    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>::value && !is_infinite<Input>::value)>
    univector(Input&& input)
    {
        this->resize(input.size());
        this->assign_expr(std::forward<Input>(input));
    }
    constexpr univector() noexcept(noexcept(std::vector<T, allocator<T>>())) = default;
    constexpr univector(const std::vector<T>& other) : std::vector<T, allocator<T>>(other) {}
    constexpr univector(std::vector<T>&& other) : std::vector<T, allocator<T>>(std::move(other)) {}
    constexpr univector(const array_ref<T>& other) : std::vector<T, allocator<T>>(other.begin(), other.end())
    {
    }
    constexpr univector(const array_ref<const T>& other)
        : std::vector<T, allocator<T>>(other.begin(), other.end())
    {
    }
    constexpr static bool size_known   = false;
    constexpr static bool is_array     = false;
    constexpr static bool is_array_ref = false;
    constexpr static bool is_vector    = true;
    constexpr static bool is_aligned   = true;
    using value_type                   = T;

    value_type get(size_t index, value_type fallback_value) const noexcept
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector>::operator=;
};

template <typename T>
using univector_ref = univector<T, tag_array_ref>;

template <typename T>
using univector_dyn = univector<T, tag_dynamic_vector>;

template <typename T, size_t Size1 = tag_dynamic_vector, size_t Size2 = tag_dynamic_vector>
using univector2d = univector<univector<T, Size2>, Size1>;

template <typename T, size_t Size1 = tag_dynamic_vector, size_t Size2 = tag_dynamic_vector,
          size_t Size3 = tag_dynamic_vector>
using univector3d      = univector<univector<univector<T, Size3>, Size2>, Size1>;

/// @brief Creates univector from data and size
template <typename T>
CMT_INLINE univector_ref<T> make_univector(T* data, size_t size)
{
    return univector_ref<T>(data, size);
}

/// @brief Creates univector from data and size
template <typename T>
CMT_INLINE univector_ref<const T> make_univector(const T* data, size_t size)
{
    return univector_ref<const T>(data, size);
}

/// @brief Converts an expression to univector
template <typename Expr, typename T = value_type_of<Expr>>
CMT_INLINE univector<T> render(Expr&& expr)
{
    static_assert(!is_infinite<Expr>::value,
                  "render: Can't process infinite expressions. Pass size as a second argument to render.");
    univector<T> result;
    result.resize(expr.size());
    result = expr;
    return result;
}

/// @brief Converts an expression to univector
template <typename Expr, typename T = value_type_of<Expr>>
CMT_INLINE univector<T> render(Expr&& expr, size_t size)
{
    univector<T> result;
    result.resize(size);
    result = expr;
    return result;
}

/// @brief Single producer single consumer lock-free ring buffer
template <typename T>
struct lockfree_ring_buffer
{
    lockfree_ring_buffer() : front(0), tail(0) {}

    size_t size() const
    {
        return tail.load(std::memory_order_relaxed) - front.load(std::memory_order_relaxed);
    }

    template <size_t Tag>
    size_t try_enqueue(const T* source, size_t size, univector<T, Tag>& buffer, bool partial = false)
    {
        const size_t cur_tail   = tail.load(std::memory_order_relaxed);
        const size_t avail_size = buffer.size() - (cur_tail - front.load(std::memory_order_relaxed));
        if (size > avail_size)
        {
            if (!partial)
                return 0;
            size = std::min(size, avail_size);
        }
        std::atomic_thread_fence(std::memory_order_acquire);

        const size_t real_tail  = cur_tail % buffer.size();
        const size_t first_size = std::min(buffer.size() - real_tail, size);
        internal::builtin_memcpy(buffer.data() + real_tail, source, first_size * sizeof(T));
        internal::builtin_memcpy(buffer.data(), source + first_size, (size - first_size) * sizeof(T));

        std::atomic_thread_fence(std::memory_order_release);

        tail.store(cur_tail + size, std::memory_order_relaxed);
        return size;
    }

    template <size_t Tag>
    size_t try_dequeue(T* dest, size_t size, const univector<T, Tag>& buffer, bool partial = false)
    {
        const size_t cur_front  = front.load(std::memory_order_relaxed);
        const size_t avail_size = tail.load(std::memory_order_relaxed) - cur_front;
        if (size > avail_size)
        {
            if (!partial)
                return 0;
            size = std::min(size, avail_size);
        }
        std::atomic_thread_fence(std::memory_order_acquire);

        const size_t real_front = cur_front % buffer.size();
        const size_t first_size = std::min(buffer.size() - real_front, size);
        internal::builtin_memcpy(dest, buffer.data() + real_front, first_size * sizeof(T));
        internal::builtin_memcpy(dest + first_size, buffer.data(), (size - first_size) * sizeof(T));

        std::atomic_thread_fence(std::memory_order_release);

        front.store(cur_front + size, std::memory_order_relaxed);
        return size;
    }

private:
    std::atomic<size_t> front;
    char cacheline_filler[64 - sizeof(std::atomic<size_t>)];
    std::atomic<size_t> tail;
};
}

CMT_PRAGMA_MSVC(warning(pop))
