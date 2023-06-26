/** @addtogroup tensor
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

#include <optional>

#include "../cometa/array.hpp"

#include "../simd/horizontal.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/logical.hpp"
#include "../simd/min_max.hpp"
#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include "expression.hpp"
#include "memory.hpp"
#include "shape.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{

namespace internal_generic
{
struct memory_finalizer_base
{
    virtual ~memory_finalizer_base() {}
};
template <typename Data>
struct memory_finalizer_data : public memory_finalizer_base
{
    constexpr KFR_INTRINSIC memory_finalizer_data(Data&& data) : data(std::move(data)) {}
    Data data;
};
template <typename Func>
struct memory_finalizer_func : public memory_finalizer_data<Func>
{
    using memory_finalizer_data<Func>::memory_finalizer_data;
    KFR_INTRINSIC ~memory_finalizer_func() { this->data(); }
};
} // namespace internal_generic

using memory_finalizer = std::shared_ptr<internal_generic::memory_finalizer_base>;

template <typename Fn>
memory_finalizer KFR_INTRINSIC make_memory_finalizer(Fn&& fn)
{
    return memory_finalizer(new internal_generic::memory_finalizer_func<Fn>{ std::move(fn) });
}

template <typename T, typename Derived, typename Dims>
struct tensor_subscript;

template <typename T, typename Derived, index_t... Dims>
struct tensor_subscript<T, Derived, std::integer_sequence<index_t, Dims...>>
{
    constexpr static inline size_t dims = sizeof...(Dims);

    using reference       = T&;
    using const_reference = const T&;

    reference KFR_MEM_INTRINSIC operator()(type_for<index_t, Dims>... idx) const
    {
        return static_cast<const Derived&>(*this).access(shape<dims>{ idx... });
    }
};

/// @brief tensor holds or references multidimensional data and
/// provides a way to access individual elements and perform complex operations on the data.
///
/// The number of elements in each axis of the array is defined by its shape.
/// @tparam T element type
/// @tparam NDims number of dimensions
template <typename T, index_t NDims>
struct tensor : public tensor_subscript<T, tensor<T, NDims>, std::make_integer_sequence<index_t, NDims>>
{
public:
    using value_type      = T;
    using pointer         = T* CMT_RESTRICT;
    using const_pointer   = const T* CMT_RESTRICT;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = index_t;

    constexpr static inline index_t dims = NDims;

    using shape_type = kfr::shape<dims>;

    /// @brief Tensor iterator. Iterates through flattened array
    struct tensor_iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        const tensor* src;
        shape_type indices;

        KFR_MEM_INTRINSIC intptr_t flat_index() const { return src->calc_index(indices); }

        KFR_MEM_INTRINSIC bool is_end() const { return indices.front() == internal_generic::null_index; }

        KFR_MEM_INTRINSIC T& operator*() { return src->m_data[flat_index()]; }
        KFR_MEM_INTRINSIC T* operator->() { return &operator*(); }

        // prefix
        KFR_MEM_INTRINSIC tensor_iterator& operator++()
        {
            if (!internal_generic::increment_indices(indices, shape_type(0), src->m_shape))
            {
                indices = shape_type(internal_generic::null_index);
            }
            return *this;
        }

        // postfix
        KFR_MEM_INTRINSIC tensor_iterator operator++(int)
        {
            tensor_iterator temp = *this;
            ++*this;
            return temp;
        }

        KFR_MEM_INTRINSIC bool operator==(const tensor_iterator& it) const
        {
            return src == it.src && indices == it.indices;
        }
        KFR_MEM_INTRINSIC bool operator!=(const tensor_iterator& it) const { return !operator==(it); }
    };

    using iterator       = tensor_iterator;
    using const_iterator = tensor_iterator;

    using contiguous_iterator       = pointer;
    using const_contiguous_iterator = pointer;

    /// @brief Default constructor. Creates tensor with null shape
    KFR_MEM_INTRINSIC constexpr tensor()
        : m_data(0), m_size(0), m_is_contiguous(false), m_shape{}, m_strides{}
    {
    }

    /// @brief Construct from external pointer, shape, strides and finalizer
    KFR_MEM_INTRINSIC tensor(T* data, const shape_type& shape, const shape_type& strides,
                             memory_finalizer finalizer)
        : m_data(data), m_size(size_of_shape(shape)),
          m_is_contiguous(strides == internal_generic::strides_for_shape(shape)), m_shape(shape),
          m_strides(strides), m_finalizer(std::move(finalizer))
    {
    }

    /// @brief Construct from external pointer, shape and finalizer with default strides
    KFR_MEM_INTRINSIC tensor(T* data, const shape_type& shape, memory_finalizer finalizer)
        : m_data(data), m_size(size_of_shape(shape)), m_is_contiguous(true), m_shape(shape),
          m_strides(internal_generic::strides_for_shape(shape)), m_finalizer(std::move(finalizer))
    {
    }

    KFR_INTRINSIC static T* allocate(size_t size) { return aligned_allocate<T>(size, 64); }

    KFR_INTRINSIC static void deallocate(T* ptr) { aligned_deallocate(ptr); }

    /// @brief Construct from shape and allocate memory
    KFR_INTRINSIC explicit tensor(const shape_type& shape)
        : m_size(size_of_shape(shape)), m_is_contiguous(true), m_shape(shape),
          m_strides(internal_generic::strides_for_shape(shape))
    {
        T* ptr      = allocate(m_size);
        m_data      = ptr;
        m_finalizer = make_memory_finalizer([ptr]() { deallocate(ptr); });
    }

    /// @brief Construct from shape, strides and allocate memory
    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides)
        : m_size(size_of_shape(shape)),
          m_is_contiguous(strides == internal_generic::strides_for_shape(shape)), m_shape(shape),
          m_strides(strides)
    {
        T* ptr      = allocate(m_size);
        m_data      = ptr;
        m_finalizer = make_memory_finalizer([ptr]() { deallocate(ptr); });
    }

    /// @brief Construct from shape, allocate memory and fill with value
    KFR_INTRINSIC tensor(const shape_type& shape, T value) : tensor(shape)
    {
        std::fill(contiguous_begin_unsafe(), contiguous_end_unsafe(), value);
    }

    /// @brief Construct from shape, strides, allocate memory and fill with value
    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides, T value) : tensor(shape, strides)
    {
        std::fill(begin(), end(), value);
    }

    /// @brief Construct from shape, allocate memory and fill with flat list
    KFR_INTRINSIC tensor(const shape_type& shape, const std::initializer_list<T>& values) : tensor(shape)
    {
        if (values.size() != m_size)
            KFR_REPORT_LOGIC_ERROR("Invalid initializer provided for kfr::tensor");
        std::copy(values.begin(), values.end(), contiguous_begin_unsafe());
    }

    /// @brief Initialize with braced list. Defined for 1D tensor only
    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>&& dims == 1)>
    KFR_INTRINSIC tensor(const std::initializer_list<U>& values) : tensor(shape_type(values.size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Initialize with braced list. Defined for 2D tensor only
    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>&& dims == 2)>
    KFR_INTRINSIC tensor(const std::initializer_list<std::initializer_list<U>>& values)
        : tensor(shape_type(values.size(), values.begin()->size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Initialize with braced list. Defined for 3D tensor only
    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>&& dims == 3)>
    KFR_INTRINSIC tensor(const std::initializer_list<std::initializer_list<std::initializer_list<U>>>& values)
        : tensor(shape_type(values.size(), values.begin()->size(), values.begin()->begin()->size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Initialize with braced list. Defined for 4D tensor only
    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>&& dims == 4)>
    KFR_INTRINSIC tensor(
        const std::initializer_list<std::initializer_list<std::initializer_list<std::initializer_list<U>>>>&
            values)
        : tensor(shape_type(values.size(), values.begin()->size(), values.begin()->begin()->size(),
                            values.begin()->begin()->begin()->size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides, std::initializer_list<T> values)
        : tensor(shape, strides)
    {
        if (values.size() != m_size)
            KFR_REPORT_LOGIC_ERROR("Invalid initializer provided for kfr::tensor");
        std::copy(values.begin(), values.end(), begin());
    }

    template <typename Input, KFR_ACCEPT_EXPRESSIONS(Input)>
    KFR_MEM_INTRINSIC tensor(Input&& input) : tensor(get_shape(input))
    {
        static_assert(expression_traits<Input>::dims == dims);
        process(*this, input);
    }

    KFR_INTRINSIC pointer data() const { return m_data; }

    KFR_INTRINSIC size_type size() const { return m_size; }

    KFR_INTRINSIC bool empty() const { return m_size == 0; }

    KFR_INTRINSIC tensor_iterator begin() const
    {
        if (empty())
            return tensor_iterator{ this, shape_type(internal_generic::null_index) };
        else
            return tensor_iterator{ this, shape_type(0) };
    }
    KFR_INTRINSIC tensor_iterator end() const
    {
        return tensor_iterator{ this, shape_type(internal_generic::null_index) };
    }

    KFR_INTRINSIC void require_contiguous() const
    {
        if (!m_is_contiguous)
            KFR_REPORT_LOGIC_ERROR("Contiguous array is required");
    }

    KFR_INTRINSIC contiguous_iterator contiguous_begin() const
    {
        require_contiguous();
        return m_data;
    }
    KFR_INTRINSIC contiguous_iterator contiguous_end() const
    {
        require_contiguous();
        return m_data + m_size;
    }

    KFR_INTRINSIC contiguous_iterator contiguous_begin_unsafe() const { return m_data; }
    KFR_INTRINSIC contiguous_iterator contiguous_end_unsafe() const { return m_data + m_size; }

    KFR_MEM_INTRINSIC intptr_t calc_index(const shape_type& indices) const
    {
        return static_cast<intptr_t>(static_cast<signed_index_t>(indices.dot(m_strides)));
    }

    KFR_MEM_INTRINSIC reference access(const shape_type& indices) const
    {
        return m_data[calc_index(indices)];
    }

    KFR_MEM_INTRINSIC reference operator[](index_t flat_index) const { return m_data[flat_index]; }

    KFR_MEM_INTRINSIC tensor operator()(const shape_type& start, const shape_type& stop) const
    {
        return tensor{
            m_data + calc_index(start),
            stop - start,
            m_strides,
            m_finalizer,
        };
    }

#if defined(CMT_COMPILER_IS_MSVC)
    tensor(const tensor& other)
        : m_data(other.m_data), m_size(other.m_size), m_is_contiguous(other.m_is_contiguous),
          m_shape(other.m_shape), m_strides(other.m_strides), m_finalizer(other.m_finalizer)
    {
    }
    tensor(tensor&& other)
        : m_data(other.m_data), m_size(other.m_size), m_is_contiguous(other.m_is_contiguous),
          m_shape(other.m_shape), m_strides(other.m_strides), m_finalizer(std::move(other.m_finalizer))
    {
    }
    tensor(tensor& other) : tensor(const_cast<const tensor&>(other)) {}
    tensor(const tensor&& other) : tensor(static_cast<const tensor&>(other)) {}
#else
    tensor(const tensor&) = default;
    tensor(tensor&&)      = default;
    tensor(tensor& other) : tensor(const_cast<const tensor&>(other)) {}
    tensor(const tensor&& other) : tensor(static_cast<const tensor&>(other)) {}
#endif

#if defined(CMT_COMPILER_IS_MSVC)
    tensor& operator=(const tensor& src) &
    {
        this->~tensor();
        new (this) tensor(src);
        return *this;
    }
    tensor& operator=(tensor&& src) &
    {
        this->~tensor();
        new (this) tensor(std::move(src));
        return *this;
    }
#else
    tensor& operator=(const tensor& src) & = default;
    tensor& operator=(tensor&& src) & = default;
#endif

    KFR_MEM_INTRINSIC const tensor& operator=(const tensor& src) const&
    {
        assign(src);
        return *this;
    }
    KFR_MEM_INTRINSIC tensor& operator=(const tensor& src) &&
    {
        assign(src);
        return *this;
    }
    KFR_MEM_INTRINSIC const tensor& operator=(const T& scalar) const&
    {
        assign(scalar);
        return *this;
    }
    KFR_MEM_INTRINSIC tensor& operator=(const T& scalar) &&
    {
        assign(scalar);
        return *this;
    }

    KFR_MEM_INTRINSIC void assign(const tensor& src) const
    {
        if (src.shape() != m_shape)
            KFR_REPORT_LOGIC_ERROR("Tensors must have same shape");
        std::copy(src.begin(), src.end(), begin());
    }
    KFR_MEM_INTRINSIC void assign(const T& scalar) const { std::fill(begin(), end(), scalar); }

    template <typename... Index>
    static constexpr bool has_tensor_range = (std::is_same_v<Index, tensor_range> || ...);

    KFR_MEM_INTRINSIC static void get_range(index_t& start, index_t& shape, index_t& step,
                                            signed_index_t tsize, index_t iidx)
    {
        signed_index_t tstart = iidx;
        tstart                = tstart < 0 ? tsize + tstart : tstart;
        start                 = tstart;
        shape                 = tstart < tsize ? 1 : 0;
        step                  = 1;
    }

    KFR_MEM_INTRINSIC static void get_range(index_t& start, index_t& shape, index_t& step,
                                            signed_index_t tsize, const tensor_range& iidx)
    {
        signed_index_t tstep = iidx.step.value_or(1);
        signed_index_t tstart;
        signed_index_t tstop;
        if (tstep >= 0)
        {
            tstart = iidx.start.value_or(0);
            tstop  = iidx.stop.value_or(tsize);
        }
        else
        {
            tstart = iidx.start ? *iidx.start + 1 : tsize;
            tstop  = iidx.stop ? *iidx.stop + 1 : 0;
        }
        tstart = tstart < 0 ? tsize + tstart : tstart;
        tstart = std::max(std::min(tstart, tsize), signed_index_t(0));
        if (tstep == 0)
        {
            start = tstart;
            shape = tstop - tstart;
            step  = 0;
        }
        else
        {
            tstop = tstop < 0 ? tsize + tstop : tstop;
            tstop = std::max(std::min(tstop, tsize), signed_index_t(0));
            if (tstep >= 0)
            {
                tstop = std::max(tstop, tstart);
                start = tstart;
                shape = (tstop - tstart + tstep - 1) / tstep;
                step  = tstep;
            }
            else
            {
                tstart = std::max(tstart, tstop);
                shape  = (tstart - tstop + -tstep - 1) / -tstep;
                start  = tstart - 1;
                step   = tstep;
            }
        }
    }

    template <index_t... Num, typename... Index>
    KFR_MEM_INTRINSIC void get_ranges(shape_type& start, shape_type& shape, shape_type& step,
                                      cvals_t<index_t, Num...> indices, const std::tuple<Index...>& idx) const
    {
        cforeach(indices,
                 [&](auto i_) CMT_INLINE_LAMBDA
                 {
                     constexpr index_t i  = val_of(decltype(i_)());
                     signed_index_t tsize = static_cast<signed_index_t>(m_shape[i]);
                     if constexpr (i < sizeof...(Index))
                     {
                         get_range(start[i], shape[i], step[i], tsize, std::get<i>(idx));
                     }
                     else
                     {
                         start[i] = 0;
                         shape[i] = tsize;
                         step[i]  = 1;
                     }
                 });
    }

    template <typename... Index,
              size_t ndimOut = internal_generic::count_dimensions<Index...>() + (dims - sizeof...(Index)),
              std::enable_if_t<has_tensor_range<Index...> || (sizeof...(Index) < dims)>* = nullptr>
    KFR_MEM_INTRINSIC tensor<T, ndimOut> operator()(const Index&... idx) const
    {
        shape_type start;
        shape_type shape;
        shape_type step;
        get_ranges(start, shape, step, cvalseq<index_t, dims>, std::make_tuple(idx...));
        shape_type strides = *step * *m_strides;
        // shape_type absstep = abs(*step);

        T* data = m_data + calc_index(start);
        // shape_type shape = ((*stop - *start) + (*absstep - 1)) / *absstep;

        return tensor<T, ndimOut>{
            data,
            internal_generic::compact_shape<dims, ndimOut, std::is_same_v<Index, tensor_range>...>(shape),
            internal_generic::compact_shape<dims, ndimOut, std::is_same_v<Index, tensor_range>...>(strides),
            m_finalizer,
        };
    }

    using tensor_subscript<T, tensor<T, NDims>, std::make_integer_sequence<index_t, NDims>>::operator();

    template <index_t dims>
    KFR_MEM_INTRINSIC tensor<T, dims> reshape_may_copy(const kfr::shape<dims>& new_shape,
                                                       bool allow_copy = false) const
    {
        if (size_of_shape(new_shape) != m_size)
        {
            KFR_REPORT_LOGIC_ERROR("Invalid shape provided");
        }
        /*
            TODO: reshape must be possible with non-contiguous arrays:
            [256, 256, 1] -> [256, 256]
            [256, 256] -> [256, 256, 1]
            [256, 256] -> [256, 1, 256]
        */
        if (!is_contiguous())
        {
            if (allow_copy)
            {
                tensor<T, dims> result(new_shape);
                std::copy(begin(), end(), result.contiguous_begin());
                return result;
            }
            else
            {
                KFR_REPORT_LOGIC_ERROR("reshape requires contiguous array");
            }
        }
        return tensor<T, dims>{
            m_data,
            new_shape,
            internal_generic::strides_for_shape(new_shape),
            m_finalizer,
        };
    }

    template <index_t dims>
    KFR_MEM_INTRINSIC tensor<T, dims> reshape(const kfr::shape<dims>& new_shape) const
    {
        return reshape_may_copy(new_shape, false);
    }

    KFR_MEM_INTRINSIC tensor<T, 1> flatten() const { return reshape(kfr::shape<1>{ m_size }, false); }

    KFR_MEM_INTRINSIC tensor<T, 1> flatten_may_copy(bool allow_copy = false) const
    {
        return reshape_may_copy(kfr::shape<1>{ m_size }, allow_copy);
    }

    KFR_MEM_INTRINSIC tensor copy() const
    {
        tensor result(m_shape);
        std::copy(begin(), end(), result.contiguous_begin());
        return result;
    }

    KFR_MEM_INTRINSIC void copy_from(const tensor& other) { std::copy(other.begin(), other.end(), begin()); }

    template <typename Fn>
    KFR_MEM_INTRINSIC void iterate(Fn&& fn) const
    {
        auto it = begin();
        while (it != end())
        {
            fn(*it, it.indices);
            ++it;
        }
    }

    template <typename Fn, typename Tout = std::invoke_result_t<Fn, T>>
    KFR_MEM_INTRINSIC tensor<Tout, dims> map(Fn&& fn) const
    {
        return unary(std::forward<Fn>(fn));
    }

    template <typename Fn, typename Tout = std::invoke_result_t<Fn, T>>
    KFR_MEM_INTRINSIC tensor<Tout, dims> unary(Fn&& fn) const
    {
        tensor<Tout, dims> result(m_shape);
        auto dst = result.contiguous_begin_unsafe();
        if (is_contiguous())
        {
            auto src = contiguous_begin_unsafe();
            while (src != contiguous_end_unsafe())
            {
                *dst = fn(*src);
                ++src;
                ++dst;
            }
        }
        else
        {
            auto src = begin();
            while (src != end())
            {
                *dst = fn(*src);
                ++src;
                ++dst;
            }
        }
        return result;
    }

    template <typename Fn>
    KFR_MEM_INTRINSIC const tensor& unary_inplace(Fn&& fn) const
    {
        if (is_contiguous())
        {
            auto it = contiguous_begin_unsafe();
            while (it != contiguous_end_unsafe())
            {
                *it = fn(*it);
                ++it;
            }
        }
        else
        {
            auto it = begin();
            while (it != end())
            {
                *it = fn(*it);
                ++it;
            }
        }
        return *this;
    }

    template <typename Fn>
    KFR_MEM_INTRINSIC T reduce(Fn&& fn, T initial = T{}) const
    {
        T result = initial;
        if (is_contiguous())
        {
            auto src = contiguous_begin_unsafe();
            while (src != contiguous_end_unsafe())
            {
                result = fn(*src, result);
                ++src;
            }
        }
        else
        {
            auto src = begin();
            while (src != end())
            {
                result = fn(*src, result);
                ++src;
            }
        }
        return result;
    }

    template <typename Fn, typename U, typename Tout = std::invoke_result_t<Fn, T, U>>
    KFR_MEM_INTRINSIC tensor<Tout, dims> binary(const tensor<U, dims>& rhs, Fn&& fn) const
    {
        tensor<Tout, dims> result(m_shape);
        if (is_contiguous() && rhs.is_contiguous())
        {
            auto src1 = contiguous_begin_unsafe();
            auto src2 = rhs.contiguous_begin_unsafe();
            auto dst  = result.contiguous_begin_unsafe();
            while (src1 != contiguous_end_unsafe())
            {
                *dst = fn(*src1, *src2);
                ++src1;
                ++src2;
                ++dst;
            }
        }
        else
        {
            auto src1 = begin();
            auto src2 = rhs.begin();
            auto dst  = result.contiguous_begin();
            while (src1 != end())
            {
                *dst = fn(*src1, *src2);
                ++src1;
                ++src2;
                ++dst;
            }
        }
        return result;
    }

    template <typename Fn, typename U>
    KFR_MEM_INTRINSIC const tensor& binary_inplace(const tensor<U, dims>& rhs, Fn&& fn) const
    {
        if (is_contiguous() && rhs.is_contiguous())
        {
            auto it   = contiguous_begin_unsafe();
            auto src2 = rhs.contiguous_begin_unsafe();
            while (it != contiguous_end_unsafe())
            {
                *it = fn(*it, *src2);
                ++it;
                ++src2;
            }
        }
        else
        {
            auto it   = begin();
            auto src2 = rhs.begin();
            while (it != end())
            {
                *it = fn(*it, *src2);
                ++it;
                ++src2;
            }
        }
        return *this;
    }

    template <typename U>
    KFR_MEM_INTRINSIC tensor<U, dims> astype() const
    {
        return unary([](T value) { return static_cast<U>(value); });
    }

    template <size_t Nout>
    KFR_MEM_INTRINSIC std::array<T, Nout> to_array() const
    {
        if (m_size != Nout)
            KFR_REPORT_LOGIC_ERROR("Nout != m_size");
        std::array<T, Nout> result;
        if (is_contiguous())
            std::copy(contiguous_begin(), contiguous_end(), result.begin());
        else
            std::copy(begin(), end(), result.begin());
        return result;
    }
    struct nested_iterator_t
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = tensor<T, dims - 1>;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        const tensor* src;
        size_t index;

        KFR_MEM_INTRINSIC value_type operator*() { return src->operator()(index); }
        KFR_MEM_INTRINSIC pointer operator->() { return &operator*(); }

        // prefix
        KFR_MEM_INTRINSIC nested_iterator_t& operator++()
        {
            ++index;
            return *this;
        }

        // postfix
        KFR_MEM_INTRINSIC nested_iterator_t operator++(int)
        {
            nested_iterator_t temp = *this;
            ++*this;
            return temp;
        }

        KFR_MEM_INTRINSIC bool operator==(const nested_iterator_t& it) const
        {
            return src == it.src && index == it.index;
        }
        KFR_MEM_INTRINSIC bool operator!=(const nested_iterator_t& it) const { return !operator==(it); }
    };

    using nested_iterator = std::conditional_t<dims == 1, tensor_iterator, nested_iterator_t>;

    KFR_MEM_INTRINSIC nested_iterator nested_begin() const
    {
        if constexpr (dims == 1)
            return begin();
        else
            return { this, 0 };
    }
    KFR_MEM_INTRINSIC nested_iterator nested_end() const
    {
        if constexpr (dims == 1)
            return end();
        else
            return { this, m_shape[0] };
    }

    KFR_MEM_INTRINSIC memory_finalizer finalizer() const { return m_finalizer; }

    template <typename Input, KFR_ACCEPT_EXPRESSIONS(Input)>
    KFR_MEM_INTRINSIC const tensor& operator=(Input&& input) const&
    {
        process(*this, input);
        return *this;
    }
    template <typename Input, KFR_ACCEPT_EXPRESSIONS(Input)>
    KFR_MEM_INTRINSIC tensor& operator=(Input&& input) &&
    {
        process(*this, input);
        return *this;
    }
    template <typename Input, KFR_ACCEPT_EXPRESSIONS(Input)>
    KFR_MEM_INTRINSIC tensor& operator=(Input&& input) &
    {
        process(*this, input);
        return *this;
    }

    bool operator==(const tensor& other) const
    {
        return shape() == other.shape() && std::equal(begin(), end(), other.begin());
    }
    bool operator!=(const tensor& other) const { return !operator==(other); }

    KFR_MEM_INTRINSIC const shape_type& shape() const { return m_shape; }
    KFR_MEM_INTRINSIC const shape_type& strides() const { return m_strides; }

    KFR_MEM_INTRINSIC bool is_contiguous() const { return m_is_contiguous; }

    KFR_MEM_INTRINSIC bool is_last_contiguous() const { return m_strides.back() == 1; }

    template <typename Fmt = void>
    std::string to_string(int max_columns = 16, int max_dimensions = INT_MAX, std::string separator = ", ",
                          std::string open = "{", std::string close = "}") const
    {
        if constexpr (dims == 0)
        {
            if (empty())
                return {};
            else
                return as_string(wrap_fmt(access(shape_type{}), cometa::ctype<Fmt>));
        }
        else
        {
            return cometa::array_to_string<Fmt>(
                m_shape.template to_std_array<size_t>(),
                [this](std::array<size_t, dims> index) CMT_INLINE_LAMBDA
                { return access(shape_type::from_std_array(index)); },
                max_columns, max_dimensions, std::move(separator), std::move(open), std::move(close));
        }
    }

private:
    template <typename Input>
    KFR_MEM_INTRINSIC void assign_expr(Input&& input) const
    {
        process(*this, std::forward<Input>(input));
    }

    T* m_data;
    const index_t m_size;
    const bool m_is_contiguous;
    const shape_type m_shape;
    const shape_type m_strides;
    memory_finalizer m_finalizer;
};

// template <typename T>
// struct tensor<T, 0>
// {
// private:
// };

template <typename Container, CMT_ENABLE_IF(kfr::has_data_size<Container>),
          typename T = typename Container::value_type>
KFR_INTRINSIC tensor<T, 1> tensor_from_container(Container container)
{
    using container_finalizer = internal_generic::memory_finalizer_data<Container>;
    memory_finalizer mem      = memory_finalizer(new container_finalizer{ std::move(container) });

    Container* ptr = &static_cast<container_finalizer*>(mem.get())->data;

    return tensor<T, 1>(ptr->data(), shape<1>(ptr->size()), std::move(mem));
}

template <typename T, index_t Dims>
struct expression_traits<tensor<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const tensor<T, Dims>& self)
    {
        return self.shape();
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return shape<dims>{ undefined_size }; }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const tensor<T, NDims>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>&)
{
    static_assert(Axis < NDims || NDims == 0);
    const T* data = self.data() + self.calc_index(index);
    if constexpr (NDims == 0)
    {
        static_assert(N == 1);
        return *data;
    }
    else
    {
        if (self.strides()[Axis] == 1)
            return read<N>(data);
        return gather_stride<N>(data, self.strides()[Axis]);
    }
}

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const tensor<T, NDims>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>&, const identity<vec<T, N>>& value)
{
    static_assert(Axis < NDims || NDims == 0);
    T* data = self.data() + self.calc_index(index);
    if constexpr (NDims == 0)
    {
        static_assert(N == 1);
        *data = value.front();
    }
    else
    {
        if (self.strides()[Axis] == 1)
            return write(data, value);
        scatter_stride(data, value, self.strides()[Axis]);
    }
}

template <size_t width = 0, index_t Axis = infinite_size, typename E, typename Traits = expression_traits<E>>
tensor<typename Traits::value_type, Traits::dims> trender(const E& expr)
{
    static_assert(!Traits::get_shape().has_infinity());
    shape sh = Traits::get_shape(expr);
    tensor<typename Traits::value_type, Traits::dims> result(sh);
    process<width, Axis>(result, expr);
    return result;
}

template <size_t width = 0, index_t Axis = infinite_size, typename E, typename Traits = expression_traits<E>>
tensor<typename Traits::value_type, Traits::dims> trender(const E& expr, shape<Traits::dims> size)
{
    shape sh = min(Traits::get_shape(expr), size);
    tensor<typename Traits::value_type, Traits::dims> result(sh);
    process<width, Axis>(result, expr, shape<Traits::dims>{ 0 }, sh);
    return result;
}

} // namespace CMT_ARCH_NAME

} // namespace kfr

namespace cometa
{
template <typename T, kfr::index_t dims>
struct representation<kfr::tensor<T, dims>>
{
    using type = std::string;
    static std::string get(const kfr::tensor<T, dims>& value) { return value.to_string(); }
};

} // namespace cometa

CMT_PRAGMA_MSVC(warning(pop))
