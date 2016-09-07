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

namespace kfr
{

constexpr size_t tag_array_ref      = 0;
constexpr size_t tag_dynamic_vector = max_size_t;

template <typename T, size_t Size = tag_dynamic_vector>
struct univector;

template <typename T, typename Class>
struct univector_base : input_expression, output_expression
{
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
        ringbuf_write(cursor, x.data(), N);
    }
    void ringbuf_write(size_t& cursor, const T value)
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
        ringbuf_read(cursor, x.data(), N);
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
        process<T>(*derived_cast<Class>(this), std::forward<Input>(input));
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
struct alignas(maximum_vector_alignment) univector : std::array<T, Size>,
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
    constexpr univector(T x, Args... args) noexcept : std::array<T, Size>{ { x, static_cast<T>(args)... } }
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
    constexpr static bool size_known   = false;
    constexpr static bool is_array     = false;
    constexpr static bool is_array_ref = true;
    constexpr static bool is_vector    = false;
    constexpr static bool is_aligned   = false;
    using value_type                   = remove_const<T>;

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

template <cpu_t c = cpu_t::native, size_t Tag, typename T, typename Fn>
CMT_INLINE void process(univector<T, Tag>& vector, Fn&& fn)
{
    static_assert(is_input_expression<Fn>::value, "Fn must be an expression");
    return process<T, c>(vector, std::forward<Fn>(fn), vector.size());
}

template <cpu_t c = cpu_t::native, typename T, size_t Nsize, typename Fn>
CMT_INLINE void process(T (&dest)[Nsize], Fn&& fn)
{
    static_assert(is_input_expression<Fn>::value, "Fn must be an expression");
    return process<T, c>(univector<T, tag_array_ref>(dest), std::forward<Fn>(fn), Nsize);
}
template <cpu_t c = cpu_t::native, typename T, typename Fn>
CMT_INLINE void process(const array_ref<T>& vector, Fn&& fn)
{
    static_assert(is_input_expression<Fn>::value, "Fn must be an expression");
    return process<T, c>(univector<T, tag_array_ref>(vector), std::forward<Fn>(fn), vector.size());
}

template <typename T>
CMT_INLINE univector_ref<T> make_univector(T* data, size_t size)
{
    return univector_ref<T>(data, size);
}

template <typename T>
CMT_INLINE univector_ref<const T> make_univector(const T* data, size_t size)
{
    return univector_ref<const T>(data, size);
}

template <typename Expr, typename T = value_type_of<Expr>>
CMT_INLINE univector<T> render(Expr&& expr)
{
    univector<T> result;
    result.resize(expr.size());
    result = expr;
    return result;
}

template <typename Expr, typename T = value_type_of<Expr>>
CMT_INLINE univector<T> render(Expr&& expr, size_t size)
{
    univector<T> result;
    result.resize(size);
    result = expr;
    return result;
}
}
