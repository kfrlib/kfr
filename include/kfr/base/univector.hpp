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

#include "../meta/array.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include "expression.hpp"
#include "memory.hpp"

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{

/// @brief Tag type that selects the storage strategy of a @ref univector or @ref abstract_vector.
using univector_tag = size_t;

/// @brief Compile-time tags selecting the storage strategy of a @ref univector.
enum : size_t
{
    tag_array_ref      = 0, ///< Tag for a non-owning reference to external data.
    tag_dynamic_vector = max_size_t, ///< Tag for a dynamically sized, owning vector.
};

/// @brief Generic 2D/3D container of @ref univector rows, parameterized by the row storage tag.
template <typename T, univector_tag Tag = tag_dynamic_vector>
struct abstract_vector;

/// @brief Specialization of @ref abstract_vector backed by a fixed-size @c std::array.
template <typename T, univector_tag Size>
struct abstract_vector : std::array<T, Size>
{
    using std::array<T, Size>::array;
};

/// @brief Specialization of @ref abstract_vector backed by a dynamically sized @c std::vector
/// using KFR's @ref data_allocator.
template <typename T>
struct abstract_vector<T, tag_dynamic_vector> : std::vector<T, data_allocator<T>>
{
    using std::vector<T, data_allocator<T>>::vector;
};

/// @brief Specialization of @ref abstract_vector that only holds a non-owning reference to external data.
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
template <typename T, univector_tag Size = tag_dynamic_vector>
struct univector;

/// @brief Base class for all @ref univector specializations, providing common
/// operations such as slicing, references and ring-buffer helpers.
/// @tparam T Element type of the vector.
/// @tparam Class Concrete derived univector type (CRTP).
/// @tparam is_expression Whether the derived type can act as an output expression.
template <typename T, typename Class, bool is_expression>
struct univector_base;

/// @brief Specialization for univectors that can be assigned from expressions.
template <typename T, typename Class>
struct univector_base<T, Class, true>
{
    /// @brief Assigns an expression to the vector.
    /// The expression must have at most one dimension; its elements are written
    /// to the vector starting at index 0.
    /// @tparam Input Type of the input expression.
    /// @param input Expression to assign.
    /// @return Reference to @c *this.
    template <expression_argument Input>
    KFR_MEM_INTRINSIC Class& operator=(Input&& input)
    {
        constexpr index_t dims = expression_dims<Input>;
        static_assert(dims <= 1, "univector accepts only expressions with dims <= 1");
        assign_expr(std::forward<Input>(input));
        return *derived_cast<Class>(this);
    }

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

    /// @brief Returns a mutable reference to the underlying data.
    array_ref<T> ref()
    {
        T* data           = get_data();
        const size_t size = get_size();
        return array_ref<T>(data, size);
    }
    /// @brief Returns a const reference to the underlying data.
    array_ref<const T> ref() const
    {
        const T* data     = get_data();
        const size_t size = get_size();
        return array_ref<const T>(data, size);
    }
    /// @brief Returns a const reference to the underlying data.
    array_ref<const T> cref() const
    {
        const T* data     = get_data();
        const size_t size = get_size();
        return array_ref<const T>(data, size);
    }

    /// @brief Writes @p srcsize elements from @p src into the ring buffer starting at @p cursor.
    /// The buffer is treated as circular: data that does not fit before the end wraps around to the
    /// beginning. If @p srcsize exceeds the buffer capacity, only the most recent @c size() elements
    /// are kept and the rest is skipped. @p cursor is advanced by the number of elements actually
    /// written and wrapped into @c [0, size()).
    /// @param cursor Read/write cursor into the buffer (updated in place).
    /// @param src Pointer to the source data.
    /// @param srcsize Number of elements to write.
    void ringbuf_write(size_t& cursor, const T* src, size_t srcsize)
    {
        if (KFR_UNLIKELY(srcsize == 0))
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
        if (KFR_LIKELY(srcsize <= fsize))
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
    /// @brief Writes the @p N elements of the SIMD vector @p x into the ring buffer at @p cursor.
    /// @copydetails ringbuf_write(size_t&, const T*, size_t)
    /// @param x SIMD vector to write.
    template <size_t N>
    void ringbuf_write(size_t& cursor, const vec<T, N>& x)
    {
        ringbuf_write(cursor, ptr_cast<T>(&x), N);
    }
    /// @brief Writes a single @p value into the ring buffer at @p cursor and advances the cursor.
    /// @param cursor Read/write cursor into the buffer (updated in place).
    /// @param value Value to write.
    void ringbuf_write(size_t& cursor, const T& value)
    {
        T* data      = get_data();
        data[cursor] = value;
        ringbuf_step(cursor, 1);
    }
    /// @brief Advances @p cursor by @p step, wrapping it into the @c [0, size()) range.
    /// @param cursor Cursor to advance (updated in place).
    /// @param step Number of positions to advance.
    void ringbuf_step(size_t& cursor, size_t step) const
    {
        const size_t size = get_size();
        cursor            = cursor + step;
        cursor            = cursor >= size ? cursor - size : cursor;
    }
    /// @brief Reads a single element from the ring buffer at @p cursor into @p value and advances the cursor.
    /// @param cursor Read cursor into the buffer (updated in place).
    /// @param value Output value.
    void ringbuf_read(size_t& cursor, T& value)
    {
        T* data = get_data();
        value   = data[cursor];
        ringbuf_step(cursor, 1);
    }
    /// @brief Reads @p N elements from the ring buffer at @p cursor into the SIMD vector @p x.
    /// @param cursor Read cursor into the buffer (updated in place).
    /// @param x Output SIMD vector.
    template <size_t N>
    void ringbuf_read(size_t& cursor, vec<T, N>& x)
    {
        ringbuf_read(cursor, ptr_cast<T>(&x), N);
    }
    /// @brief Reads @p destsize elements from the ring buffer at @p cursor into @p dest.
    /// The buffer is treated as circular: reads that cross the end wrap around to the beginning.
    /// If @p destsize exceeds the buffer capacity, only the most recent @c size() elements are
    /// produced and the leading portion of @p dest is skipped. @p cursor is advanced by the number
    /// of elements actually read and wrapped into @c [0, size()).
    /// @param cursor Read cursor into the buffer (updated in place).
    /// @param dest Destination buffer.
    /// @param destsize Number of elements to read.
    void ringbuf_read(size_t& cursor, T* dest, size_t destsize) const
    {
        if (KFR_UNLIKELY(destsize == 0))
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
        if (KFR_LIKELY(destsize <= fsize))
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
    /// @brief Evaluates @p input and writes the result into the derived vector.
    /// @tparam Input Type of the input expression.
    /// @param input Expression to evaluate.
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

/// @brief Specialization for univectors that cannot be assigned from expressions.
/// Provides only reference access to the underlying data.
template <typename T, typename Class>
struct univector_base<T, Class, false>
{
    /// @brief Returns a mutable reference to the underlying data.
    array_ref<T> ref()
    {
        T* data           = get_data();
        const size_t size = get_size();
        return array_ref<T>(data, size);
    }
    /// @brief Returns a const reference to the underlying data.
    array_ref<const T> ref() const
    {
        const T* data     = get_data();
        const size_t size = get_size();
        return array_ref<const T>(data, size);
    }
    /// @brief Returns a const reference to the underlying data.
    array_ref<const T> cref() const
    {
        const T* data     = get_data();
        const size_t size = get_size();
        return array_ref<const T>(data, size);
    }

    /// @brief Assignment from an expression is disabled for non-expression vectors.
    /// Always triggers a static assertion failure.
    template <expression_argument Input>
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

/// @brief Fixed-size, owning univector backed by @c std::array and aligned to the platform's
/// maximum vector alignment. Suitable for stack-allocated buffers of known size.
/// @tparam T Element type. Must not be const-qualified.
/// @tparam Size Number of elements.
template <typename T, size_t Size>
struct alignas(platform<>::maximum_vector_alignment) univector
    : std::array<T, Size>,
      univector_base<T, univector<T, Size>, is_vec_element<T>>
{
    static_assert(!std::is_const_v<T>, "Static vector doesn't allow T to be const");

    using std::array<T, Size>::size;
    using size_type = size_t;
#if !defined KFR_COMPILER_MSVC || defined KFR_COMPILER_CLANG
    univector(univector& v) : univector(const_cast<const univector&>(v)) {}
#endif
    univector(const univector& v)   = default;
    univector(univector&&) noexcept = default;
    /// @brief Constructs the vector and initializes it from an input expression.
    /// @tparam Input Type of the input expression.
    /// @param input Expression to assign.
    template <expression_argument Input>
    univector(Input&& input)
    {
        this->assign_expr(std::forward<Input>(input));
    }
    /// @brief Constructs the vector from a brace-enclosed list of values.
    /// @param x First element value.
    /// @param args Remaining element values.
    template <typename... Args>
    constexpr univector(const T& x, const Args&... args) noexcept
        : std::array<T, Size>{ { x, static_cast<T>(args)... } }
    {
    }

    constexpr univector() noexcept(noexcept(std::array<T, Size>())) = default;
    /// @brief Constructs the vector with @p Size copies of @p value.
    /// @param value Value to fill the vector with.
    constexpr univector(size_t, const T& value) { std::fill(this->begin(), this->end(), value); }
    constexpr static bool size_known    = true; ///< @brief True: the size is known at compile time.
    constexpr static size_t static_size = Size; ///< @brief Compile-time size of the vector.
    constexpr static bool is_array      = true; ///< @brief True: backed by @c std::array.
    constexpr static bool is_array_ref  = false; ///< @brief False: owns its data.
    constexpr static bool is_vector     = false; ///< @brief False: not dynamically sized.
    constexpr static bool is_aligned    = true; ///< @brief True: aligned to the maximum vector alignment.
    using value_type                    = T;

    /// @brief Returns the element at @p index, or @p fallback_value if @p index is out of range.
    /// @param index Index of the element to access.
    /// @param fallback_value Value returned when @p index is out of range.
    /// @return The element at @p index, or @p fallback_value.
    value_type get(size_t index, value_type fallback_value) const noexcept
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector, is_vec_element<T>>::operator=;

    /// @brief No-op for API compatibility with dynamically sized vectors.
    void resize(size_t) noexcept {}
};

/// @brief Non-owning univector specialization that holds a reference to external data.
/// @tparam T Element type. May be const-qualified to expose a read-only view.
template <typename T>
struct univector<T, tag_array_ref> : array_ref<T>,
                                     univector_base<T, univector<T, tag_array_ref>, is_vec_element<T>>
{
    using array_ref<T>::size;
    using array_ref<T>::array_ref;
    using size_type = size_t;
#if !defined KFR_COMPILER_MSVC || defined KFR_COMPILER_CLANG
    univector(univector& v) : univector(const_cast<const univector&>(v)) {}
#endif
    univector(const univector& v)   = default;
    univector(univector&&) noexcept = default;
    constexpr univector(const array_ref<T>& other) : array_ref<T>(other) {}
    constexpr univector(array_ref<T>&& other) : array_ref<T>(std::move(other)) {}

    /// @brief Constructs a reference from a const univector of any storage tag.
    /// @tparam Tag Storage tag of the source vector.
    /// @param other Source vector to reference.
    template <univector_tag Tag>
    constexpr univector(const univector<T, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    /// @brief Constructs a reference from a mutable univector of any storage tag.
    /// @tparam Tag Storage tag of the source vector.
    /// @param other Source vector to reference.
    template <univector_tag Tag>
    constexpr univector(univector<T, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    /// @brief Constructs a const reference from a const univector of a different element type.
    /// Only participates in overload resolution when @c T is const-qualified and the underlying
    /// element types match after removing const.
    /// @tparam U Element type of the source vector.
    /// @tparam Tag Storage tag of the source vector.
    /// @param other Source vector to reference.
    template <typename U, univector_tag Tag>
        requires(std::is_same_v<std::remove_const_t<T>, U> && std::is_const_v<T>)
    constexpr univector(const univector<U, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    /// @brief Constructs a const reference from a mutable univector of a different element type.
    /// @copydetails univector(const univector<U, Tag>&)
    template <typename U, univector_tag Tag>
        requires(std::is_same_v<std::remove_const_t<T>, U> && std::is_const_v<T>)
    constexpr univector(univector<U, Tag>& other) : array_ref<T>(other.data(), other.size())
    {
    }
    /// @brief Constructs a const reference by taking a reference to the data of a temporary univector.
    /// @copydetails univector(const univector<U, Tag>&)
    template <typename U, univector_tag Tag>
        requires(std::is_same_v<std::remove_const_t<T>, U> && std::is_const_v<T>)
    constexpr univector(univector<U, Tag>&& other) : array_ref<T>(other.data(), other.size())
    {
    }
    /// @brief No-op for API compatibility with dynamically sized vectors.
    void resize(size_t) noexcept {}
    constexpr static bool size_known   = false; ///< @brief False: the size is not known at compile time.
    constexpr static bool is_array     = false; ///< @brief False: not backed by @c std::array.
    constexpr static bool is_array_ref = true; ///< @brief True: holds a reference to external data.
    constexpr static bool is_vector    = false; ///< @brief False: not dynamically sized.
    constexpr static bool is_aligned = false; ///< @brief False: alignment of the referenced data is unknown.
    using value_type                 = std::remove_const_t<T>;

    /// @brief Returns the element at @p index, or @p fallback_value if @p index is out of range.
    /// @param index Index of the element to access.
    /// @param fallback_value Value returned when @p index is out of range.
    /// @return The element at @p index, or @p fallback_value.
    value_type get(size_t index, value_type fallback_value) const noexcept
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector, is_vec_element<T>>::operator=;

    /// @brief Returns @c *this as a reference. Intended for use on rvalues to forward the
    /// contained reference without copying.
    univector<T, tag_array_ref>& ref() && { return *this; }
};

/// @brief Dynamically sized, owning univector backed by @c std::vector using KFR's
/// @ref data_allocator. Memory is aligned to the maximum vector alignment.
/// @tparam T Element type. Must not be const-qualified.
template <typename T>
struct univector<T, tag_dynamic_vector>
    : std::vector<T, data_allocator<T>>,
      univector_base<T, univector<T, tag_dynamic_vector>, is_vec_element<T>>
{
    static_assert(!std::is_const_v<T>, "Dynamic vector doesn't allow T to be const");

    using std::vector<T, data_allocator<T>>::size;
    using std::vector<T, data_allocator<T>>::vector;
    using size_type = size_t;
#if !defined KFR_COMPILER_IS_MSVC
    univector(univector& v) : univector(const_cast<const univector&>(v)) {}
#endif
    univector(const univector& v)   = default;
    univector(univector&&) noexcept = default;
    /// @brief Constructs the vector from an input expression, resizing to the expression's size.
    /// The expression must be finite and have at most one dimension.
    /// @tparam Input Type of the input expression.
    /// @param input Expression to assign.
    template <expression_argument Input>
    univector(Input&& input)
    {
        static_assert(!is_infinite<Input>, "Dynamically sized vector requires finite input expression");
        constexpr index_t dims = expression_dims<Input>;
        static_assert(dims <= 1, "univector accepts only expressions with dims <= 1");
        if constexpr (dims > 0)
        {
            this->resize(get_shape(input).front());
        }
        this->assign_expr(std::forward<Input>(input));
    }
    constexpr univector() noexcept(noexcept(std::vector<T, data_allocator<T>>())) = default;
    /// @brief Copy-constructs from a @c std::vector using the same allocator.
    constexpr univector(const std::vector<T, data_allocator<T>>& other)
        : std::vector<T, data_allocator<T>>(other)
    {
    }
    /// @brief Move-constructs from a @c std::vector using the same allocator.
    constexpr univector(std::vector<T, data_allocator<T>>&& other)
        : std::vector<T, data_allocator<T>>(std::move(other))
    {
    }
    /// @brief Constructs from a mutable @ref array_ref by copying its elements.
    constexpr univector(const array_ref<T>& other)
        : std::vector<T, data_allocator<T>>(other.begin(), other.end())
    {
    }
    /// @brief Constructs from a const @ref array_ref by copying its elements.
    constexpr univector(const array_ref<const T>& other)
        : std::vector<T, data_allocator<T>>(other.begin(), other.end())
    {
    }
    /// @brief Deleted overload to prevent construction from a @c std::vector with a foreign allocator.
    template <typename Allocator>
    constexpr univector(const std::vector<T, Allocator>&) = delete;
    /// @brief Deleted overload to prevent construction from a @c std::vector with a foreign allocator.
    template <typename Allocator>
    constexpr univector(std::vector<T, Allocator>&&) = delete;
    constexpr static bool size_known   = false; ///< @brief False: the size is not known at compile time.
    constexpr static bool is_array     = false; ///< @brief False: not backed by @c std::array.
    constexpr static bool is_array_ref = false; ///< @brief False: owns its data.
    constexpr static bool is_vector    = true; ///< @brief True: dynamically sized.
    constexpr static bool is_aligned   = true; ///< @brief True: aligned to the maximum vector alignment.
    using value_type                   = T;

    /// @brief Returns the element at @p index, or @p fallback_value if @p index is out of range.
    /// @param index Index of the element to access.
    /// @param fallback_value Value returned when @p index is out of range.
    /// @return The element at @p index, or @p fallback_value.
    value_type get(size_t index, value_type fallback_value) const noexcept
    {
        return index < this->size() ? this->operator[](index) : fallback_value;
    }
    using univector_base<T, univector, is_vec_element<T>>::operator=;
#ifdef KFR_COMPILER_IS_MSVC
    /// @brief Copy assignment. Reconstructs the object in place to work around MSVC issues.
    univector& operator=(const univector& other)
    {
        this->~univector();
        new (this) univector(other);
        return *this;
    }
    /// @brief Move assignment. Reconstructs the object in place to work around MSVC issues.
    univector& operator=(univector&& other)
    {
        this->~univector();
        new (this) univector(std::move(other));
        return *this;
    }
#else
    univector& operator=(const univector&) = default;
    univector& operator=(univector&&)      = default;
#endif
    /// @brief Copy assignment from a mutable univector (forwards to the const overload).
    KFR_MEM_INTRINSIC univector& operator=(univector& other) { return operator=(std::as_const(other)); }
    /// @brief Assigns an expression to the vector, resizing it when the expression has a finite size.
    /// @tparam Input Type of the input expression.
    /// @param input Expression to assign.
    /// @return Reference to @c *this.
    template <expression_argument Input>
    KFR_MEM_INTRINSIC univector& operator=(Input&& input)
    {
        constexpr index_t dims = expression_dims<Input>;
        static_assert(dims <= 1, "univector accepts only expressions with dims <= 1");
        if constexpr (dims > 0)
        {
            if (get_shape(input).front() != infinite_size)
                this->resize(get_shape(input).front());
        }
        this->assign_expr(std::forward<Input>(input));
        return *this;
    }
};

/// @brief Expression traits for @ref univector, allowing it to participate in KFR expression evaluation.
template <typename T, univector_tag Tag>
struct expression_traits<univector<T, Tag>> : public expression_traits_defaults
{
    using value_type             = std::remove_const_t<T>;
    constexpr static size_t dims = 1;
    /// @brief Returns the runtime shape of @p u.
    constexpr static shape<dims> get_shape(const univector<T, Tag>& u) { return shape<1>(u.size()); }
    /// @brief Returns the static shape of the univector type.
    /// For fixed-size vectors this is the compile-time size; otherwise it is @ref undefined_size.
    constexpr static shape<dims> get_shape()
    {
        if constexpr (univector<T, Tag>::size_known)
            return shape<1>{ univector<T, Tag>::static_size };
        else
            return shape<1>{ undefined_size };
    }
};

/// @brief Returns @c true if the two univectors have equal size and element-wise equal contents.
template <typename T, univector_tag T1, univector_tag T2>
KFR_FUNCTION bool operator==(const univector<T, T1>& x, const univector<T, T2>& y)
{
    return std::equal(x.begin(), x.end(), y.begin(), y.end());
}
/// @brief Returns @c true if the two univectors differ in size or in any element.
template <typename T, univector_tag T1, univector_tag T2>
KFR_FUNCTION bool operator!=(const univector<T, T1>& x, const univector<T, T2>& y)
{
    return !operator==(x, y);
}

/// @brief Alias for ``univector<T, tag_array_ref>``;
template <typename T>
using univector_ref = univector<T, tag_array_ref>;

/// @brief Alias for ``univector<T, tag_dynamic_vector>``;
template <typename T>
using univector_dyn = univector<T, tag_dynamic_vector>;

/// @brief Alias for a 2D container of univectors.
/// @tparam T Element type of the innermost univector.
/// @tparam Size1 Storage tag for the outer dimension.
/// @tparam Size2 Storage tag for the inner dimension.
template <typename T, univector_tag Size1 = tag_dynamic_vector, univector_tag Size2 = tag_dynamic_vector>
using univector2d = abstract_vector<univector<T, Size2>, Size1>;

/// @brief Alias for a 3D container of univectors.
/// @tparam T Element type of the innermost univector.
/// @tparam Size1 Storage tag for the outermost dimension.
/// @tparam Size2 Storage tag for the middle dimension.
/// @tparam Size3 Storage tag for the innermost dimension.
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
template <has_data_size Container, typename T = value_type_of<Container>>
KFR_INTRINSIC univector_ref<const T> make_univector(const Container& container)
{
    return univector_ref<const T>(container.data(), container.size());
}

/// @brief Creates univector from a container (must have data() and size() methods)
template <has_data_size Container, typename T = value_type_of<Container>>
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

/// @brief Single producer single consumer lock-free ring buffer.
///
/// The buffer itself is provided externally (as a @ref univector); this class only tracks
/// the read and write cursors atomically and performs the circular copy. The @c front and
/// @c tail indices are separated by a cache-line-sized filler to avoid false sharing between
/// the producer and consumer threads.
/// @tparam T Element type stored in the buffer.
template <typename T>
struct lockfree_ring_buffer
{
    /// @brief Constructs an empty ring buffer with @c front and @c tail set to 0.
    lockfree_ring_buffer() : front(0), tail(0) {}

    /// @brief Returns the number of elements currently available for dequeue.
    size_t size() const
    {
        return tail.load(std::memory_order_relaxed) - front.load(std::memory_order_relaxed);
    }

    /// @brief Attempts to enqueue @p size elements from @p source into @p buffer.
    ///
    /// When @p partial is @c false the call fails atomically and returns 0 if the requested
    /// @p size does not fit in the available space. When @p partial is @c true, as many elements
    /// as possible are written and the actual count is returned. The data is copied in up to two
    /// fragments to handle wrap-around at the end of the buffer.
    /// @param source Pointer to the source data.
    /// @param size Number of elements to enqueue.
    /// @param buffer Backing storage (its @c size() defines the capacity).
    /// @param partial When @c true, allow writing fewer than @p size elements.
    /// @return The number of elements actually enqueued.
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

    /// @brief Attempts to dequeue @p size elements from @p buffer into @p dest.
    ///
    /// When @p partial is @c false the call fails atomically and returns 0 if fewer than @p size
    /// elements are available. When @p partial is @c true, as many elements as available are read
    /// and the actual count is returned. The data is copied in up to two fragments to handle
    /// wrap-around at the end of the buffer.
    /// @param dest Destination buffer.
    /// @param size Number of elements to dequeue.
    /// @param buffer Backing storage (its @c size() defines the capacity).
    /// @param partial When @c true, allow reading fewer than @p size elements.
    /// @return The number of elements actually dequeued.
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
    char cacheline_filler[KFR_CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];
    std::atomic<size_t> tail;
};
inline namespace KFR_ARCH_NAME
{

/// @brief Internal ADL-provided implementation for `get_elements` expressions.
template <typename T, univector_tag Tag, size_t N>
KFR_INTRINSIC vec<std::remove_const_t<T>, N> get_elements(const univector<T, Tag>& self,
                                                          const shape<1>& index, const axis_params<0, N>&)
{
    const T* data = self.data();
    return read<N>(ptr_cast<T>(data) + index.front());
}

/// @brief Internal ADL-provided implementation for `set_elements` expressions.
template <typename T, univector_tag Tag, size_t N>
    requires(!std::is_const_v<T>)
KFR_INTRINSIC void set_elements(univector<T, Tag>& self, const shape<1>& index, const axis_params<0, N>&,
                                const std::type_identity_t<vec<T, N>>& value)
{
    T* data = self.data();
    write(ptr_cast<T>(data) + index.front(), value);
}

/// @brief Converts an expression to a dynamically sized univector.
///
/// The expression must be one-dimensional and finite; the result is resized to the expression's
/// shape and filled with its elements.
/// @tparam Expr Type of the expression.
/// @tparam T Element type of the result (deduced from @p Expr).
/// @param expr Expression to render.
/// @return A @ref univector holding the evaluated expression.
template <typename Expr, typename T = expression_value_type<Expr>>
KFR_INTRINSIC univector<T> render(Expr&& expr)
{
    static_assert(expression_dims<Expr> == 1);
    static_assert(!is_infinite<Expr>,
                  "render: Can't process infinite expressions. Pass size as a second argument to render.");
    univector<T> result;
    result.resize(get_shape(expr).front());
    result = expr;
    return result;
}

/// @brief Converts a slice of an expression to a dynamically sized univector of the given size.
/// @tparam Expr Type of the expression.
/// @tparam T Element type of the result (deduced from @p Expr).
/// @param expr Expression to render.
/// @param size Number of elements to render.
/// @param offset Index of the first element to render.
/// @return A @ref univector holding the evaluated slice.
template <typename Expr, typename T = expression_value_type<Expr>>
KFR_INTRINSIC univector<T> render(Expr&& expr, size_t size, size_t offset = 0)
{
    univector<T> result;
    result.resize(size);
    result = slice(expr, offset, size);
    return result;
}

/// @brief Converts an expression to a fixed-size univector.
/// @tparam Expr Type of the expression.
/// @tparam Size Compile-time size of the result.
/// @tparam T Element type of the result (deduced from @p Expr).
/// @param expr Expression to render.
/// @return A @ref univector<T, Size> holding the evaluated expression.
template <typename Expr, size_t Size, typename T = expression_value_type<Expr>>
KFR_INTRINSIC univector<T, Size> render(Expr&& expr, csize_t<Size>)
{
    univector<T, Size> result;
    result = expr;
    return result;
}
} // namespace KFR_ARCH_NAME
} // namespace kfr

namespace kfr
{
/// @brief @ref representation specialization that formats a @ref univector as a string
/// using KFR's built-in array formatting.
template <typename T, kfr::univector_tag Tag>
struct representation<kfr::univector<T, Tag>>
{
    using type = std::string;
    /// @brief Returns a string representation of @p value.
    static std::string get(const kfr::univector<T, Tag>& value)
    {
        return array_to_string(value.size(), value.data());
    }
};

/// @brief @ref representation specialization that formats a fmt_t-wrapped @ref univector
/// as a string, applying the wrapped format options to each element.
template <char t, int width, int prec, typename T, kfr::univector_tag Tag>
struct representation<fmt_t<kfr::univector<T, Tag>, t, width, prec>>
{
    using type = std::string;
    /// @brief Returns a string representation of @p value with per-element formatting.
    static std::string get(const fmt_t<kfr::univector<T, Tag>, t, width, prec>& value)
    {
        return array_to_string<fmt_t<T, t, width, prec>>(value.value.size(), value.value.data());
    }
};

} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
