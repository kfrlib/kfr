/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

#include "../cometa/array.hpp"

#include "../simd/impl/function.hpp"
#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include "memory.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{

using univector_tag = size_t;

enum : size_t
{
    tag_array_ref      = 0,
    tag_dynamic_vector = max_size_t,
};

template <typename T, univector_tag Tag = tag_dynamic_vector>
struct abstract_vector;

template <typename T, univector_tag Size>
struct abstract_vector : std::array<T, Size>
{
    using std::array<T, Size>::array;
};

template <typename T>
struct abstract_vector<T, tag_dynamic_vector> : std::vector<T, cometa::allocator<T>>
{
    using std::vector<T, cometa::allocator<T>>::vector;
};

template <typename T>
struct abstract_vector<T, tag_array_ref> : array_ref<T>
{
    using array_ref<T>::array_ref;
};

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
template <typename T, univector_tag Tag = tag_dynamic_vector>
struct univector;

/// @brief Base class for all univector specializations.
template <typename T, typename Class, bool is_expression>
struct univector_base;

template <typename T, typename Class>
struct univector_base<T, Class, true> : input_expression, output_expression
{
    using input_expression::begin_block;
    using input_expression::end_block;
    using output_expression::begin_block;
    using output_expression::end_block;

    template <typename U, size_t N>
    KFR_MEM_INTRINSIC void operator()(coutput_t, size_t index, const vec<U, N>& value)
    {
        T* data = derived_cast<Class>(this)->data();
        write(ptr_cast<T>(data) + index, vec<T, N>(value));
    }

    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    KFR_MEM_INTRINSIC Class& operator=(Input&& input)
    {
        assign_expr(std::forward<Input>(input));
        return *derived_cast<Class>(this);
    }

#define KFR_UVEC_ASGN_OP(aop, op)                                                                            \
    template <typename Input>                                                                                \
    KFR_MEM_INTRINSIC Class& aop(Input&& input)                                                              \
    {                                                                                                        \
        assign_expr(*derived_cast<Class>(this) op std::forward<Input>(input));                               \
        return *derived_cast<Class>(this);                                                                   \
    }
    KFR_UVEC_ASGN_OP(operator+=, +)
    KFR_UVEC_ASGN_OP(operator-=, -)
    KFR_UVEC_ASGN_OP(operator*=, *)
    KFR_UVEC_ASGN_OP(operator/=, /)
    KFR_UVEC_ASGN_OP(operator%=, %)

    KFR_UVEC_ASGN_OP(operator&=, &)
    KFR_UVEC_ASGN_OP(operator|=, |)
    KFR_UVEC_ASGN_OP(operator^=, ^)

    KFR_UVEC_ASGN_OP(operator<<=, <<)
    KFR_UVEC_ASGN_OP(operator>>=, >>)

    /// @brief Returns subrange of the vector.
    /// If start is greater or equal to this->size, returns empty univector
    /// If requested size is greater than this->size, returns only available elements
    univector<T, 0> slice(size_t start = 0, size_t size = max_size_t)
    {
        T* data                = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<T>(data + start, std::min(size, start < this_size ? this_size - start : 0));
    }

    /// @brief Returns subrange of the vector.
    /// If start is greater or equal to this->size, returns empty univector
    /// If requested size is greater than this->size, returns only available elements
    univector<const T, 0> slice(size_t start = 0, size_t size = max_size_t) const
    {
        const T* data          = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<const T>(data + start, std::min(size, start < this_size ? this_size - start : 0));
    }

    /// @brief Returns subrange of the vector starting from 0.
    /// If requested size is greater than this->size, returns only available elements
    univector<T, 0> truncate(size_t size = max_size_t)
    {
        T* data                = derived_cast<Class>(this)->data();
        const size_t this_size = derived_cast<Class>(this)->size();
        return array_ref<T>(data, std::min(size, this_size));
    }

    /// @brief Returns subrange of the vector starting from 0.
    /// If requested size is greater than this->size, returns only available elements
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
    array_ref<const T> cref() const
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
    KFR_MEM_INTRINSIC void assign_expr(Input&& input)
    {
        process(*derived_cast<Class>(this), std::forward<Input>(input));
    }

private:
    KFR_MEM_INTRINSIC size_t get_size() const { return derived_cast<Class>(this)->size(); }
    KFR_MEM_INTRINSIC const T* get_data() const { return derived_cast<Class>(this)->data(); }
    KFR_MEM_INTRINSIC T* get_data() { return derived_cast<Class>(this)->data(); }

    static void copy(T* dest, const T* src, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
            *dest++ = *src++;
    }
};

template <typename T, typename Class>
struct univector_base<T, Class, false>
{
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
    array_ref<const T> cref() const
    {
        const T* data     = get_data();
        const size_t size = get_size();
        return array_ref<const T>(data, size);
    }

    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    KFR_MEM_INTRINSIC Class& operator=(Input&& input)
    {
        static_assert(sizeof(Input) == 0, "Can't assign expression to non-expression");
        return *derived_cast<Class>(this);
    }

private:
    KFR_MEM_INTRINSIC size_t get_size() const { return derived_cast<Class>(this)->size(); }
    KFR_MEM_INTRINSIC const T* get_data() const { return derived_cast<Class>(this)->data(); }
    KFR_MEM_INTRINSIC T* get_data() { return derived_cast<Class>(this)->data(); }
};

template <typename T, size_t Size>
struct alignas(platform<>::maximum_vector_alignment) univector
    : std::array<T, Size>,
      univector_base<T, univector<T, Size>, is_vec_element<T>>
{
    using std::array<T, Size>::size;
    using size_type = size_t;
#if !defined CMT_COMPILER_MSVC || defined CMT_COMPILER_CLANG
    univector(univector& v) : univector(const_cast<const univector&>(v)) {}
#endif
    univector(const univector& v)   = default;
    univector(univector&&) noexcept = default;
    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    univector(Input&& input)
    {
        this->assign_expr(std::forward<Input>(input));
    }
    template <typename... Args>
    constexpr univector(const T& x, const Args&... args) CMT_NOEXCEPT
        : std::array<T, Size>{ { x, static_cast<T>(args)... } }
    {
    }

    constexpr univector() CMT_NOEXCEPT_SPEC(noexcept(std::array<T, Size>())) = default;
    constexpr univector(size_t, const T& value) { std::fill(this->begin(), this->end(), value); }
    constexpr static bool size_known   = true;
    constexpr static bool is_array     = true;
    constexpr static bool is_array_ref = false;
    constexpr static bool is_vector    = false;
    constexpr static bool is_aligned   = true;
    constexpr static bool is_pod       = kfr::is_pod<T>;
    using value_type                   = T;

    value_type get(size_t index, value_type fallback_value) const CMT_NOEXCEPT
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector, is_vec_element<T>>::operator=;

    void resize(size_t) CMT_NOEXCEPT {}
};

template <typename T>
struct univector<T, tag_array_ref> : array_ref<T>,
                                     univector_base<T, univector<T, tag_array_ref>, is_vec_element<T>>
{
    using array_ref<T>::size;
    using array_ref<T>::array_ref;
    using size_type = size_t;
#if !defined CMT_COMPILER_MSVC || defined CMT_COMPILER_CLANG
    univector(univector& v) : univector(const_cast<const univector&>(v)) {}
#endif
    univector(const univector& v)   = default;
    univector(univector&&) noexcept = default;
    constexpr univector(const array_ref<T>& other) : array_ref<T>(other) {}
    constexpr univector(array_ref<T>&& other) : array_ref<T>(std::move(other)) {}

    template <univector_tag Tag>
    constexpr univector(const univector<T, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    template <univector_tag Tag>
    constexpr univector(univector<T, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    template <typename U, univector_tag Tag, KFR_ENABLE_IF(cometa::is_same<cometa::remove_const<T>, U>&& cometa::is_const<T>)>
    constexpr univector(const univector<U, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    template <typename U, univector_tag Tag, KFR_ENABLE_IF(cometa::is_same<cometa::remove_const<T>, U>&& cometa::is_const<T>)>
    constexpr univector(univector<U, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    void resize(size_t) CMT_NOEXCEPT {}
    constexpr static bool size_known   = false;
    constexpr static bool is_array     = false;
    constexpr static bool is_array_ref = true;
    constexpr static bool is_vector    = false;
    constexpr static bool is_aligned   = false;
    using value_type                   = cometa::remove_const<T>;

    value_type get(size_t index, value_type fallback_value) const CMT_NOEXCEPT
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector, is_vec_element<T>>::operator=;

    univector<T, tag_array_ref>& ref() && { return *this; }
};

template <typename T>
struct univector<T, tag_dynamic_vector>
    : std::vector<T, cometa::allocator<T>>, univector_base<T, univector<T, tag_dynamic_vector>, is_vec_element<T>>
{
    using std::vector<T, cometa::allocator<T>>::size;
    using std::vector<T, cometa::allocator<T>>::vector;
    using size_type = size_t;
#if !defined CMT_COMPILER_MSVC || defined CMT_COMPILER_CLANG
    univector(univector& v) : univector(const_cast<const univector&>(v)) {}
#endif
    univector(const univector& v)   = default;
    univector(univector&&) noexcept = default;
    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    univector(Input&& input)
    {
        static_assert(!is_infinite<Input>, "Dynamically sized vector requires finite input expression");
        this->resize(input.size());
        this->assign_expr(std::forward<Input>(input));
    }
    constexpr univector() CMT_NOEXCEPT_SPEC(noexcept(std::vector<T, cometa::allocator<T>>())) = default;
    constexpr univector(const std::vector<T, cometa::allocator<T>>& other) : std::vector<T, cometa::allocator<T>>(other) {}
    constexpr univector(std::vector<T, cometa::allocator<T>>&& other) : std::vector<T, cometa::allocator<T>>(std::move(other))
    {
    }
    constexpr univector(const array_ref<T>& other) : std::vector<T, cometa::allocator<T>>(other.begin(), other.end())
    {
    }
    constexpr univector(const array_ref<const T>& other)
        : std::vector<T, cometa::allocator<T>>(other.begin(), other.end())
    {
    }
    template <typename Allocator>
    constexpr univector(const std::vector<T, Allocator>&) = delete;
    template <typename Allocator>
    constexpr univector(std::vector<T, Allocator>&&) = delete;
    constexpr static bool size_known                 = false;
    constexpr static bool is_array                   = false;
    constexpr static bool is_array_ref               = false;
    constexpr static bool is_vector                  = true;
    constexpr static bool is_aligned                 = true;
    using value_type                                 = T;

    value_type get(size_t index, value_type fallback_value) const CMT_NOEXCEPT
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector, is_vec_element<T>>::operator=;
    univector& operator=(const univector&) = default;
    template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    KFR_MEM_INTRINSIC univector& operator=(Input&& input)
    {
        if (input.size() != infinite_size)
            this->resize(input.size());
        this->assign_expr(std::forward<Input>(input));
        return *this;
    }
};

/// @brief Alias for ``univector<T, tag_array_ref>``;
template <typename T>
using univector_ref = univector<T, tag_array_ref>;

/// @brief Alias for ``univector<T, tag_dynamic_vector>``;
template <typename T>
using univector_dyn = univector<T, tag_dynamic_vector>;

template <typename T, univector_tag Size1 = tag_dynamic_vector, univector_tag Size2 = tag_dynamic_vector>
using univector2d = abstract_vector<univector<T, Size2>, Size1>;

template <typename T, univector_tag Size1 = tag_dynamic_vector, univector_tag Size2 = tag_dynamic_vector,
          univector_tag Size3 = tag_dynamic_vector>
using univector3d = abstract_vector<abstract_vector<univector<T, Size3>, Size2>, Size1>;

/// @brief Creates univector from data and size
template <typename T>
KFR_INTRINSIC univector_ref<T> make_univector(T* data, size_t size)
{
    return univector_ref<T>(data, size);
}

/// @brief Creates univector from data and size
template <typename T>
KFR_INTRINSIC univector_ref<const T> make_univector(const T* data, size_t size)
{
    return univector_ref<const T>(data, size);
}

/// @brief Creates univector from a container (must have data() and size() methods)
template <typename Container, KFR_ENABLE_IF(kfr::has_data_size<Container>),
          typename T = value_type_of<Container>>
KFR_INTRINSIC univector_ref<const T> make_univector(const Container& container)
{
    return univector_ref<const T>(container.data(), container.size());
}

/// @brief Creates univector from a container (must have data() and size() methods)
template <typename Container, KFR_ENABLE_IF(kfr::has_data_size<Container>),
          typename T = value_type_of<Container>>
KFR_INTRINSIC univector_ref<T> make_univector(Container& container)
{
    return univector_ref<T>(container.data(), container.size());
}

/// @brief Creates univector from a sized array
template <typename T, size_t N>
KFR_INTRINSIC univector_ref<T> make_univector(T (&arr)[N])
{
    return univector_ref<T>(arr, N);
}

/// @brief Creates univector from a sized array
template <typename T, size_t N>
KFR_INTRINSIC univector_ref<const T> make_univector(const T (&arr)[N])
{
    return univector_ref<const T>(arr, N);
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

    template <univector_tag Tag>
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
        builtin_memcpy(buffer.data() + real_tail, source, first_size * sizeof(T));
        builtin_memcpy(buffer.data(), source + first_size, (size - first_size) * sizeof(T));

        std::atomic_thread_fence(std::memory_order_release);

        tail.store(cur_tail + size, std::memory_order_relaxed);
        return size;
    }

    template <univector_tag Tag>
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
        builtin_memcpy(dest, buffer.data() + real_front, first_size * sizeof(T));
        builtin_memcpy(dest + first_size, buffer.data(), (size - first_size) * sizeof(T));

        std::atomic_thread_fence(std::memory_order_release);

        front.store(cur_front + size, std::memory_order_relaxed);
        return size;
    }

private:
    std::atomic<size_t> front;
    char cacheline_filler[64 - sizeof(std::atomic<size_t>)];
    std::atomic<size_t> tail;
};
inline namespace CMT_ARCH_NAME
{

template <typename T, univector_tag Tag, typename U, size_t N>
KFR_INTRINSIC vec<U, N> get_elements(const univector<T, Tag>& self, cinput_t, size_t index, vec_shape<U, N>)
{
    const T* data = self.data();
    return static_cast<vec<U, N>>(read<N>(ptr_cast<T>(data) + index));
}

/// @brief Converts an expression to univector
template <typename Expr, typename T = value_type_of<Expr>>
KFR_INTRINSIC univector<T> render(Expr&& expr)
{
    static_assert(!is_infinite<Expr>,
                  "render: Can't process infinite expressions. Pass size as a second argument to render.");
    univector<T> result;
    result.resize(expr.size());
    result = expr;
    return result;
}

/// @brief Converts an expression to univector
template <typename Expr, typename T = value_type_of<Expr>>
KFR_INTRINSIC univector<T> render(Expr&& expr, size_t size, size_t offset = 0)
{
    univector<T> result;
    result.resize(size);
    result = slice(expr, offset, size);
    return result;
}

/// @brief Converts an expression to univector
template <typename Expr, size_t Size, typename T = value_type_of<Expr>>
KFR_INTRINSIC univector<T, Size> render(Expr&& expr, csize_t<Size>)
{
    univector<T, Size> result;
    result = expr;
    return result;
}
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_MSVC(warning(pop))
