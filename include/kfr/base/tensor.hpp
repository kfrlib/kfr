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

#include <optional>

#include "../meta/array.hpp"

#include "../simd/horizontal.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/logical.hpp"
#include "../simd/min_max.hpp"
#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include "expression.hpp"
#include "memory.hpp"
#include "shape.hpp"
#include "transpose.hpp"

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4324))

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

/**
 * @brief Shared ownership handle for a memory finalizer.
 *
 * Keeps referenced memory alive for the lifetime of the handle. Used by tensors
 * that reference externally owned or allocated data.
 */
using memory_finalizer = std::shared_ptr<internal_generic::memory_finalizer_base>;

/**
 * @brief Creates a memory finalizer that invokes @p fn when the handle is destroyed.
 * @tparam Fn Callable type invoked with no arguments.
 * @param fn Callable invoked on finalizer destruction.
 * @return Shared handle holding the finalizer.
 */
template <typename Fn>
memory_finalizer KFR_INTRINSIC make_memory_finalizer(Fn&& fn)
{
    return memory_finalizer(new internal_generic::memory_finalizer_func<Fn>{ std::move(fn) });
}

/**
 * @brief Base for the subscript operator helper of tensor.
 *
 * Provides a variadic @c operator() that forwards to the derived tensor's
 * @c access method, converting the provided indices into a shape.
 *
 * @tparam T element type.
 * @tparam Derived derived tensor type.
 * @tparam Dims integer sequence encoding the number of dimensions.
 */
template <typename T, typename Derived, typename Dims>
struct tensor_subscript;

/**
 * @brief Specialization of tensor_subscript for an explicit integer sequence of dimensions.
 *
 * Exposes an @c operator() taking one index per dimension and forwarding to
 * @c Derived::access with the corresponding shape.
 *
 * @tparam T element type.
 * @tparam Derived derived tensor type.
 * @tparam Dims... compile-time dimension sizes.
 */
template <typename T, typename Derived, index_t... Dims>
struct tensor_subscript<T, Derived, std::integer_sequence<index_t, Dims...>>
{
    constexpr static inline size_t dims = sizeof...(Dims);

    using reference       = T&;
    using const_reference = const T&;

    /**
     * @brief Accesses the element at the given multi-dimensional index.
     * @param idx... one index per dimension.
     * @return Reference to the element at the given position.
     */
    reference KFR_MEM_INTRINSIC operator()(type_for<index_t, Dims>... idx) const
    {
        return static_cast<const Derived&>(*this).access(shape<dims>{ idx... });
    }
};

/**
 * @brief Holds or references multidimensional data and provides access to individual elements
 *        and complex operations on the data.
 *
 * The number of elements in each axis of the array is defined by its shape. A tensor may own its
 * data (allocated on construction) or reference externally owned data through a memory finalizer.
 *
 * @tparam T element type.
 * @tparam NDims number of dimensions.
 */
template <typename T, index_t NDims>
struct tensor : public tensor_subscript<T, tensor<T, NDims>, std::make_integer_sequence<index_t, NDims>>
{
public:
    using value_type      = T; ///< Element type.
    using pointer         = T* KFR_RESTRICT; ///< Pointer to element.
    using const_pointer   = const T* KFR_RESTRICT; ///< Pointer to constant element.
    using reference       = T&; ///< Reference to element.
    using const_reference = const T&; ///< Reference to constant element.
    using size_type       = index_t; ///< Size type.

    constexpr static inline index_t dims = NDims; ///< Number of dimensions.

    using shape_type = kfr::shape<dims>; ///< Shape type used by this tensor.

    /// @brief Tensor iterator. Iterates through the flattened array in row-major order.
    struct tensor_iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        const tensor* src;
        shape_type indices;

        /// @brief Returns the flat (linear) index of the current element.
        KFR_MEM_INTRINSIC intptr_t flat_index() const { return src->calc_index(indices); }

        /// @brief Returns @c true if this iterator is the end sentinel.
        KFR_MEM_INTRINSIC bool is_end() const { return indices.front() == internal_generic::null_index; }

        /// @brief Dereferences the iterator to the referenced element.
        KFR_MEM_INTRINSIC T& operator*() { return src->m_data[flat_index()]; }
        /// @brief Accesses the referenced element by pointer.
        KFR_MEM_INTRINSIC T* operator->() { return &operator*(); }

        /// @brief Prefix increment, advancing to the next element in row-major order.
        // prefix
        KFR_MEM_INTRINSIC tensor_iterator& operator++()
        {
            if (!internal_generic::increment_indices(indices, shape_type(0), src->m_shape))
            {
                indices = shape_type(internal_generic::null_index);
            }
            return *this;
        }

        /// @brief Postfix increment.
        // postfix
        KFR_MEM_INTRINSIC tensor_iterator operator++(int)
        {
            tensor_iterator temp = *this;
            ++*this;
            return temp;
        }

        /// @brief Returns @c true if two iterators refer to the same position.
        KFR_MEM_INTRINSIC bool operator==(const tensor_iterator& it) const
        {
            return src == it.src && indices == it.indices;
        }
        /// @brief Returns @c true if two iterators refer to different positions.
        KFR_MEM_INTRINSIC bool operator!=(const tensor_iterator& it) const { return !operator==(it); }
    };

    using iterator       = tensor_iterator;
    using const_iterator = tensor_iterator;

    using contiguous_iterator       = pointer;
    using const_contiguous_iterator = pointer;

    /// @brief Default constructor. Creates a tensor with a null shape.
    KFR_MEM_INTRINSIC constexpr tensor()
        : m_data(0), m_size(0), m_is_contiguous(false), m_shape{}, m_strides{}
    {
    }

    /// @brief Constructs a tensor from an external pointer, shape, strides, and finalizer.
    /// @param data pointer to the external data.
    /// @param shape shape of the tensor.
    /// @param strides strides of the tensor.
    /// @param finalizer memory finalizer keeping @p data alive.
    KFR_MEM_INTRINSIC tensor(T* data, const shape_type& shape, const shape_type& strides,
                             memory_finalizer finalizer)
        : m_data(data), m_size(size_of_shape(shape)),
          m_is_contiguous(strides == internal_generic::strides_for_shape(shape)), m_shape(shape),
          m_strides(strides), m_finalizer(std::move(finalizer))
    {
    }

    /// @brief Constructs a tensor from an external pointer, shape, and finalizer using default strides.
    /// @param data pointer to the external data.
    /// @param shape shape of the tensor.
    /// @param finalizer memory finalizer keeping @p data alive.
    KFR_MEM_INTRINSIC tensor(T* data, const shape_type& shape, memory_finalizer finalizer)
        : m_data(data), m_size(size_of_shape(shape)), m_is_contiguous(true), m_shape(shape),
          m_strides(internal_generic::strides_for_shape(shape)), m_finalizer(std::move(finalizer))
    {
    }

    /// @brief Allocates aligned memory for @p size elements.
    /// @return Pointer to the allocated memory.
    KFR_INTRINSIC static T* allocate(size_t size) { return aligned_allocate<T>(size, 64); }

    /// @brief Deallocates memory previously returned by allocate().
    /// @param ptr pointer returned by allocate().
    KFR_INTRINSIC static void deallocate(T* ptr) { aligned_deallocate(ptr); }

    /// @brief Constructs a tensor with the given shape and allocates memory for it.
    /// @param shape shape of the tensor.
    KFR_INTRINSIC explicit tensor(const shape_type& shape)
        : m_size(size_of_shape(shape)), m_is_contiguous(true), m_shape(shape),
          m_strides(internal_generic::strides_for_shape(shape))
    {
        T* ptr      = allocate(m_size);
        m_data      = ptr;
        m_finalizer = make_memory_finalizer([ptr]() { deallocate(ptr); });
    }

    /// @brief Constructs a tensor with the given shape and strides and allocates memory for it.
    /// @param shape shape of the tensor.
    /// @param strides strides of the tensor.
    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides)
        : m_size(size_of_shape(shape)),
          m_is_contiguous(strides == internal_generic::strides_for_shape(shape)), m_shape(shape),
          m_strides(strides)
    {
        T* ptr      = allocate(m_size);
        m_data      = ptr;
        m_finalizer = make_memory_finalizer([ptr]() { deallocate(ptr); });
    }

    /// @brief Constructs a tensor with the given shape, allocates memory, and fills it with @p value.
    /// @param shape shape of the tensor.
    /// @param value value to fill the tensor with.
    KFR_INTRINSIC tensor(const shape_type& shape, T value) : tensor(shape)
    {
        std::fill(contiguous_begin_unsafe(), contiguous_end_unsafe(), value);
    }

    /// @brief Constructs a tensor with the given shape and strides, allocates memory, and fills it with
    /// `value`.
    /// @param shape shape of the tensor.
    /// @param strides strides of the tensor.
    /// @param value value to fill the tensor with.
    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides, T value) : tensor(shape, strides)
    {
        std::fill(begin(), end(), value);
    }

    /// @brief Constructs a tensor with the given shape, allocates memory, and fills it from a flat
    /// initializer list.
    /// @param shape shape of the tensor.
    /// @param values flat initializer list of values.
    KFR_INTRINSIC tensor(const shape_type& shape, const std::initializer_list<T>& values) : tensor(shape)
    {
        if (values.size() != m_size)
            KFR_REPORT_LOGIC_ERROR("Invalid initializer provided for kfr::tensor");
        std::copy(values.begin(), values.end(), contiguous_begin_unsafe());
    }

    /// @brief Initializes a 1D tensor from a braced list.
    /// Defined for 1D tensors only.
    /// @param values initializer list of values.
    template <typename U, KFR_ENABLE_IF(std::is_convertible_v<U, T>&& dims == 1)>
    KFR_INTRINSIC tensor(const std::initializer_list<U>& values) : tensor(shape_type(values.size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Initializes a 2D tensor from a braced list.
    /// Defined for 2D tensors only.
    /// @param values nested initializer list of values.
    template <std::convertible_to<T> U>
        requires(dims == 2)
    KFR_INTRINSIC tensor(const std::initializer_list<std::initializer_list<U>>& values)
        : tensor(shape_type(values.size(), values.begin()->size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Initializes a 3D tensor from a braced list.
    /// Defined for 3D tensors only.
    /// @param values nested initializer list of values.
    template <std::convertible_to<T> U>
        requires(dims == 3)
    KFR_INTRINSIC tensor(const std::initializer_list<std::initializer_list<std::initializer_list<U>>>& values)
        : tensor(shape_type(values.size(), values.begin()->size(), values.begin()->begin()->size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Initializes a 4D tensor from a braced list.
    /// Defined for 4D tensors only.
    /// @param values nested initializer list of values.
    template <std::convertible_to<T> U>
        requires(dims == 4)
    KFR_INTRINSIC tensor(
        const std::initializer_list<std::initializer_list<std::initializer_list<std::initializer_list<U>>>>&
            values)
        : tensor(shape_type(values.size(), values.begin()->size(), values.begin()->begin()->size(),
                            values.begin()->begin()->begin()->size()))
    {
        internal_generic::list_copy_recursively(values, contiguous_begin_unsafe());
    }

    /// @brief Constructs a tensor with the given shape and strides and fills it from a flat initializer list.
    /// @param shape shape of the tensor.
    /// @param strides strides of the tensor.
    /// @param values flat initializer list of values.
    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides, std::initializer_list<T> values)
        : tensor(shape, strides)
    {
        if (values.size() != m_size)
            KFR_REPORT_LOGIC_ERROR("Invalid initializer provided for kfr::tensor");
        std::copy(values.begin(), values.end(), begin());
    }

    /// @brief Constructs a tensor by evaluating an expression.
    /// @param input expression to evaluate and store in the tensor.
    template <expression_argument Input>
    KFR_MEM_INTRINSIC tensor(Input&& input) : tensor(get_shape(input))
    {
        static_assert(expression_traits<Input>::dims == dims);
        process(*this, input);
    }

    /// @brief Returns a pointer to the underlying data.
    KFR_INTRINSIC pointer data() const { return m_data; }

    /// @brief Returns the total number of elements in the tensor.
    KFR_INTRINSIC size_type size() const { return m_size; }

    /// @brief Returns @c true if the tensor has no elements.
    KFR_INTRINSIC bool empty() const { return m_size == 0; }

    /// @brief Returns an iterator to the first element.
    KFR_INTRINSIC tensor_iterator begin() const
    {
        if (empty())
            return tensor_iterator{ this, shape_type(internal_generic::null_index) };
        else
            return tensor_iterator{ this, shape_type(0) };
    }
    /// @brief Returns the end sentinel iterator.
    KFR_INTRINSIC tensor_iterator end() const
    {
        return tensor_iterator{ this, shape_type(internal_generic::null_index) };
    }

    /// @brief Throws if the tensor is not contiguous.
    KFR_INTRINSIC void require_contiguous() const
    {
        if (!m_is_contiguous)
            KFR_REPORT_LOGIC_ERROR("Contiguous array is required");
    }

    /// @brief Returns a contiguous iterator to the first element.
    /// The tensor must be contiguous.
    KFR_INTRINSIC contiguous_iterator contiguous_begin() const
    {
        require_contiguous();
        return m_data;
    }
    /// @brief Returns a contiguous iterator past the last element.
    /// The tensor must be contiguous.
    KFR_INTRINSIC contiguous_iterator contiguous_end() const
    {
        require_contiguous();
        return m_data + m_size;
    }

    /// @brief Returns a contiguous iterator to the first element without checking contiguity.
    KFR_INTRINSIC contiguous_iterator contiguous_begin_unsafe() const { return m_data; }
    /// @brief Returns a contiguous iterator past the last element without checking contiguity.
    KFR_INTRINSIC contiguous_iterator contiguous_end_unsafe() const { return m_data + m_size; }

    /// @brief Computes the linear offset of the element at @p indices using the tensor's strides.
    /// @param indices multi-dimensional index.
    /// @return Flat offset into the underlying data.
    KFR_MEM_INTRINSIC intptr_t calc_index(const shape_type& indices) const
    {
        return static_cast<intptr_t>(static_cast<signed_index_t>(indices.dot(m_strides)));
    }

    /// @brief Returns a reference to the element at @p indices.
    /// @param indices multi-dimensional index.
    /// @return Reference to the element.
    KFR_MEM_INTRINSIC reference access(const shape_type& indices) const
    {
        return m_data[calc_index(indices)];
    }

    /// @brief Returns a reference to the element at the given flat index.
    /// @param flat_index linear offset into the underlying data.
    KFR_MEM_INTRINSIC reference operator[](index_t flat_index) const { return m_data[flat_index]; }

    /// @brief Returns a sub-tensor (slice) spanning from @p start to @p stop.
    /// @param start starting indices of the slice.
    /// @param stop stopping indices of the slice (exclusive).
    /// @return New tensor referencing the sliced region.
    KFR_MEM_INTRINSIC tensor operator()(const shape_type& start, const shape_type& stop) const
    {
        return tensor{
            m_data + calc_index(start),
            stop - start,
            m_strides,
            m_finalizer,
        };
    }

    /// @brief Copy-constructs a tensor sharing the data of @p other.
    /// @param other tensor to copy from.
#if defined(KFR_COMPILER_IS_MSVC)
    tensor(const tensor& other)
        : m_data(other.m_data), m_size(other.m_size), m_is_contiguous(other.m_is_contiguous),
          m_shape(other.m_shape), m_strides(other.m_strides), m_finalizer(other.m_finalizer)
    {
    }
    /// @brief Move-constructs a tensor from @p other.
    /// @param other tensor to move from.
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

#if defined(KFR_COMPILER_IS_MSVC) || true
    /// @brief Copy-assigns the tensor, sharing the data of @p src.
    /// @param src tensor to copy from.
    /// @return Reference to @c *this.
    tensor& operator=(const tensor& src) &
    {
        this->~tensor();
        new (this) tensor(src);
        return *this;
    }
    /// @brief Move-assigns the tensor from @p src.
    /// @param src tensor to move from.
    /// @return Reference to @c *this.
    tensor& operator=(tensor&& src) &
    {
        this->~tensor();
        new (this) tensor(std::move(src));
        return *this;
    }
#else
    tensor& operator=(const tensor& src) & = default;
    tensor& operator=(tensor&& src) &      = default;
#endif

    /// @brief Copy-assigns the tensor from @p src (const overload).
    /// @param src tensor to copy from.
    /// @return Reference to @c *this.
    KFR_MEM_INTRINSIC const tensor& operator=(const tensor& src) const&
    {
        assign(src);
        return *this;
    }
    /// @brief Copy-assigns the tensor from @p src (rvalue overload).
    /// @param src tensor to copy from.
    /// @return Reference to @c *this.
    KFR_MEM_INTRINSIC tensor& operator=(const tensor& src) &&
    {
        assign(src);
        return *this;
    }
    /// @brief Fills the tensor with a scalar value (const overload).
    /// @param scalar value to assign to every element.
    /// @return Reference to @c *this.
    KFR_MEM_INTRINSIC const tensor& operator=(const T& scalar) const&
    {
        assign(scalar);
        return *this;
    }
    /// @brief Fills the tensor with a scalar value (rvalue overload).
    /// @param scalar value to assign to every element.
    /// @return Reference to @c *this.
    KFR_MEM_INTRINSIC tensor& operator=(const T& scalar) &&
    {
        assign(scalar);
        return *this;
    }

    /// @brief Copies the contents of @p src into this tensor.
    /// Both tensors must have the same shape.
    /// @param src tensor to copy from.
    KFR_MEM_INTRINSIC void assign(const tensor& src) const
    {
        if (src.shape() != m_shape)
            KFR_REPORT_LOGIC_ERROR("Tensors must have same shape");
        std::copy(src.begin(), src.end(), begin());
    }
    /// @brief Fills the tensor with a scalar value.
    /// @param scalar value to assign to every element.
    KFR_MEM_INTRINSIC void assign(const T& scalar) const { std::fill(begin(), end(), scalar); }

    /// @brief Returns @c true if any of @p Index... is a tensor_range.
    template <typename... Index>
    static constexpr bool has_tensor_range = (std::is_same_v<Index, tensor_range> || ...);

    /// @brief Computes the range for a single integer index.
    /// @param[out] start computed start offset.
    /// @param[out] shape computed shape (0 or 1).
    /// @param[out] step computed step.
    /// @param tsize size of the axis.
    /// @param iidx integer index (negative values count from the end).
    KFR_MEM_INTRINSIC static void get_range(index_t& start, index_t& shape, index_t& step,
                                            signed_index_t tsize, index_t iidx)
    {
        signed_index_t tstart = iidx;
        tstart                = tstart < 0 ? tsize + tstart : tstart;
        start                 = tstart;
        shape                 = tstart < tsize ? 1 : 0;
        step                  = 1;
    }

    /// @brief Computes the range for a tensor_range specifier.
    /// Handles optional start/stop/step, negative indices, and clamping to the axis size.
    /// @param[out] start computed start offset.
    /// @param[out] shape computed number of elements in the range.
    /// @param[out] step computed step.
    /// @param tsize size of the axis.
    /// @param iidx tensor_range describing the slice.
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

    /// @brief Computes the start, shape, and step for each axis given a set of indices and ranges.
    /// Axes without a corresponding index cover the full extent of the axis.
    /// @param[out] start computed start offsets per axis.
    /// @param[out] shape computed shape per axis.
    /// @param[out] step computed step per axis.
    /// @param indices compile-time axis index sequence.
    /// @param idx tuple of indices and ranges.
    template <index_t... Num, typename... Index>
    KFR_MEM_INTRINSIC void get_ranges(shape_type& start, shape_type& shape, shape_type& step,
                                      cvals_t<index_t, Num...> indices, const std::tuple<Index...>& idx) const
    {
        cforeach(indices,
                 [&](auto i_) KFR_INLINE_LAMBDA
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

    /// @brief Returns a sub-tensor (slice) selected by a mix of integer indices and tensor_range specifiers.
    /// Axes indexed by integers are removed from the result; axes indexed by tensor_range are kept.
    /// @param idx... one index or range per axis (trailing axes may be omitted to select the full extent).
    /// @return New tensor referencing the sliced region.
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

    /// @brief Returns a tensor with the order of axes reversed.
    /// For 0D and 1D tensors returns @c *this.
    /// @return New tensor referencing the same data with transposed shape and strides.
    KFR_MEM_INTRINSIC tensor transpose() const
    {
        if constexpr (dims <= 1)
        {
            return *this;
        }
        else
        {
            return tensor<T, dims>{
                m_data,
                m_shape.transpose(),
                m_strides.transpose(),
                m_finalizer,
            };
        }
    }

    /// @brief Returns a tensor with a different shape, copying if necessary.
    /// @tparam dims number of dimensions of the result.
    /// @param new_shape new shape; must have the same total number of elements.
    /// @param allow_copy if @c true, a copy is made when the tensor is not contiguous;
    ///                   otherwise an error is reported for non-contiguous tensors.
    /// @return New tensor with the requested shape.
    template <index_t dims>
    KFR_MEM_INTRINSIC tensor<T, dims> reshape_may_copy(const kfr::shape<dims>& new_shape,
                                                       bool allow_copy = true) const
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

    /// @brief Returns a tensor with a different shape without copying.
    /// The tensor must be contiguous.
    /// @tparam dims number of dimensions of the result.
    /// @param new_shape new shape; must have the same total number of elements.
    /// @return New tensor with the requested shape.
    template <index_t dims>
    KFR_MEM_INTRINSIC tensor<T, dims> reshape(const kfr::shape<dims>& new_shape) const
    {
        return reshape_may_copy(new_shape, false);
    }

    /// @brief Returns a 1D tensor view of the data.
    KFR_MEM_INTRINSIC tensor<T, 1> flatten() const { return reshape(kfr::shape<1>{ m_size }); }

    /// @brief Returns a 1D tensor view of the data, copying if necessary.
    /// @param allow_copy if @c true, a copy is made when the tensor is not contiguous.
    /// @return New 1D tensor.
    KFR_MEM_INTRINSIC tensor<T, 1> flatten_may_copy(bool allow_copy = true) const
    {
        return reshape_may_copy(kfr::shape<1>{ m_size }, allow_copy);
    }

    /// @brief Returns a deep copy of the tensor.
    KFR_MEM_INTRINSIC tensor copy() const
    {
        tensor result(m_shape);
        std::copy(begin(), end(), result.contiguous_begin());
        return result;
    }

    /// @brief Copies the contents of @p other into this tensor.
    /// @param other tensor to copy from.
    KFR_MEM_INTRINSIC void copy_from(const tensor& other) { std::copy(other.begin(), other.end(), begin()); }

    /// @brief Calls @p fn for each element together with its multi-dimensional index.
    /// @param fn callable invoked as @c fn(element, indices).
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

    /// @brief Applies @p fn to each element and returns the resulting tensor.
    /// @tparam Fn unary callable type.
    /// @param fn callable returning the new element value.
    /// @return New tensor with the result of applying @p fn to each element.
    template <typename Fn, typename Tout = std::invoke_result_t<Fn, T>>
    KFR_MEM_INTRINSIC tensor<Tout, dims> map(Fn&& fn) const
    {
        return unary(std::forward<Fn>(fn));
    }

    /// @brief Applies a unary function to each element and returns the resulting tensor.
    /// @tparam Fn unary callable type.
    /// @param fn callable returning the new element value.
    /// @return New tensor with the result of applying @p fn to each element.
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

    /// @brief Applies @p fn to each element in place.
    /// @tparam Fn unary callable type.
    /// @param fn callable returning the new element value.
    /// @return Reference to @c *this.
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

    /// @brief Reduces all elements using @p fn and an initial value.
    /// @tparam Fn binary callable type.
    /// @param fn callable invoked as @c fn(element, accumulator).
    /// @param initial initial value of the accumulator.
    /// @return The final accumulator value.
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

    /// @brief Applies a binary function element-wise with @p rhs and returns the resulting tensor.
    /// @tparam Fn binary callable type.
    /// @tparam U element type of @p rhs.
    /// @param rhs right-hand tensor; must have the same shape as @c *this.
    /// @param fn callable invoked as @c fn(lhs_element, rhs_element).
    /// @return New tensor with the result of the element-wise operation.
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

    /// @brief Applies a binary function element-wise with @p rhs in place.
    /// @tparam Fn binary callable type.
    /// @tparam U element type of @p rhs.
    /// @param rhs right-hand tensor; must have the same shape as @c *this.
    /// @param fn callable invoked as @c fn(lhs_element, rhs_element).
    /// @return Reference to @c *this.
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

    /// @brief Returns a tensor with elements converted to type @p U.
    /// @tparam U target element type.
    /// @return New tensor with each element static_cast'ed to @p U.
    template <typename U>
    KFR_MEM_INTRINSIC tensor<U, dims> astype() const
    {
        return unary([](T value) { return static_cast<U>(value); });
    }

    /// @brief Copies the tensor elements into a std::array.
    /// @tparam Nout size of the resulting array; must equal size().
    /// @return Array containing the tensor elements.
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
    /// @brief Iterator over the first axis yielding sub-tensors of one fewer dimension.
    struct nested_iterator_t
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = tensor<T, dims - 1>;
        using pointer           = const value_type*;
        using reference         = const value_type&;

        const tensor* src;
        size_t index;

        /// @brief Dereferences the iterator to the sub-tensor at the current index.
        KFR_MEM_INTRINSIC value_type operator*() { return src->operator()(index); }
        /// @brief Accesses the sub-tensor at the current index by pointer.
        KFR_MEM_INTRINSIC pointer operator->() { return &operator*(); }

        /// @brief Prefix increment, advancing to the next sub-tensor.
        // prefix
        KFR_MEM_INTRINSIC nested_iterator_t& operator++()
        {
            ++index;
            return *this;
        }

        /// @brief Postfix increment.
        // postfix
        KFR_MEM_INTRINSIC nested_iterator_t operator++(int)
        {
            nested_iterator_t temp = *this;
            ++*this;
            return temp;
        }

        /// @brief Returns @c true if two iterators refer to the same position.
        KFR_MEM_INTRINSIC bool operator==(const nested_iterator_t& it) const
        {
            return src == it.src && index == it.index;
        }
        /// @brief Returns @c true if two iterators refer to different positions.
        KFR_MEM_INTRINSIC bool operator!=(const nested_iterator_t& it) const { return !operator==(it); }
    };

    using nested_iterator = std::conditional_t<dims == 1, tensor_iterator, nested_iterator_t>;

    /// @brief Returns a nested iterator to the first sub-tensor along the first axis.
    KFR_MEM_INTRINSIC nested_iterator nested_begin() const
    {
        if constexpr (dims == 1)
            return begin();
        else
            return { this, 0 };
    }
    /// @brief Returns the nested iterator sentinel past the last sub-tensor.
    KFR_MEM_INTRINSIC nested_iterator nested_end() const
    {
        if constexpr (dims == 1)
            return end();
        else
            return { this, m_shape[0] };
    }

    /// @brief Returns the memory finalizer holding the data alive.
    KFR_MEM_INTRINSIC memory_finalizer finalizer() const { return m_finalizer; }

    /// @brief Evaluates @p input and assigns the result to this tensor (const overload).
    /// @param input expression to evaluate.
    /// @return Reference to @c *this.
    template <expression_argument Input>
    KFR_MEM_INTRINSIC const tensor& operator=(Input&& input) const&
    {
        process(*this, input);
        return *this;
    }
    /// @brief Evaluates @p input and assigns the result to this tensor (rvalue overload).
    /// @param input expression to evaluate.
    /// @return Reference to @c *this.
    template <expression_argument Input>
    KFR_MEM_INTRINSIC tensor& operator=(Input&& input) &&
    {
        process(*this, input);
        return *this;
    }
    /// @brief Evaluates @p input and assigns the result to this tensor (lvalue overload).
    /// @param input expression to evaluate.
    /// @return Reference to @c *this.
    template <expression_argument Input>
    KFR_MEM_INTRINSIC tensor& operator=(Input&& input) &
    {
        process(*this, input);
        return *this;
    }

    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(const tensor& lhs, const tensor& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(const tensor& lhs, tensor&& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(const tensor& lhs, tensor& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(tensor&& lhs, const tensor& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(tensor&& lhs, tensor&& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(tensor&& lhs, tensor& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(tensor& lhs, const tensor& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(tensor& lhs, tensor&& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }
    /// @brief Returns @c true if both tensors have the same shape and equal elements.
    friend bool operator==(tensor& lhs, tensor& rhs)
    {
        return lhs.shape() == rhs.shape() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    /// @brief Returns the shape of the tensor.
    KFR_MEM_INTRINSIC const shape_type& shape() const { return m_shape; }
    /// @brief Returns the strides of the tensor.
    KFR_MEM_INTRINSIC const shape_type& strides() const { return m_strides; }

    /// @brief Returns @c true if the tensor data is stored contiguously.
    KFR_MEM_INTRINSIC bool is_contiguous() const { return m_is_contiguous; }

    /// @brief Returns @c true if the last axis has stride 1.
    KFR_MEM_INTRINSIC bool is_last_contiguous() const { return m_strides.back() == 1; }

    /// @brief Returns a string representation of the tensor.
    /// @param max_columns maximum number of columns to print per row.
    /// @param max_dimensions maximum number of dimensions to print.
    /// @param separator separator between elements.
    /// @param open string used to open a row.
    /// @param close string used to close a row.
    /// @return Formatted string representation.
    template <typename Fmt = void>
    std::string to_string(int max_columns = 16, int max_dimensions = INT_MAX, std::string separator = ", ",
                          std::string open = "{", std::string close = "}") const
    {
        if constexpr (dims == 0)
        {
            if (empty())
                return {};
            else
                return as_string(wrap_fmt(access(shape_type{}), kfr::ctype<Fmt>));
        }
        else
        {
            return kfr::array_to_string<Fmt>(
                m_shape.template to_std_array<size_t>(),
                [this](std::array<size_t, dims> index) KFR_INLINE_LAMBDA
                { return access(shape_type::from_std_array(index)); }, max_columns, max_dimensions,
                std::move(separator), std::move(open), std::move(close));
        }
    }

private:
    /// @brief Evaluates @p input and assigns the result to this tensor.
    template <typename Input>
    KFR_MEM_INTRINSIC void assign_expr(Input&& input) const
    {
        process(*this, std::forward<Input>(input));
    }

    T* m_data; ///< Pointer to the underlying data.
    const index_t m_size; ///< Total number of elements.
    const bool m_is_contiguous; ///< Whether the data is stored contiguously.
    const shape_type m_shape; ///< Shape of the tensor.
    const shape_type m_strides; ///< Strides of the tensor.
    memory_finalizer m_finalizer; ///< Memory finalizer holding the data alive.
};

/// @brief Specialization of tensor for a dynamic shape. Not implemented yet.
template <typename T>
struct tensor<T, dynamic_shape>
{
    // Not implemented yet
};

// template <typename T>
// struct tensor<T, 0>
// {
// private:
// };

/// @brief Creates a 1D tensor referencing the data of a container.
/// The container is moved into a memory finalizer so the data remains valid for the tensor's lifetime.
/// @tparam Container container type with data() and size() methods.
/// @tparam T element type deduced from the container.
/// @param container container to reference.
/// @return 1D tensor referencing the container's data.
template <has_data_size Container, typename T = typename Container::value_type>
KFR_INTRINSIC tensor<T, 1> tensor_from_container(Container container)
{
    using container_finalizer = internal_generic::memory_finalizer_data<Container>;
    memory_finalizer mem      = memory_finalizer(new container_finalizer{ std::move(container) });

    Container* ptr = &static_cast<container_finalizer*>(mem.get())->data;

    return tensor<T, 1>(ptr->data(), shape<1>(ptr->size()), std::move(mem));
}

/// @brief Expression traits for tensor, enabling its use as an expression argument.
/// @tparam T element type.
/// @tparam Dims number of dimensions.
template <typename T, index_t Dims>
struct expression_traits<tensor<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    /// @brief Returns the shape of @p self.
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape(const tensor<T, Dims>& self)
    {
        return self.shape();
    }
    /// @brief Returns the static shape of the expression, with undefined extents.
    KFR_MEM_INTRINSIC constexpr static shape<dims> get_shape() { return shape<dims>{ undefined_size }; }
};

inline namespace KFR_ARCH_NAME
{

/// @brief Internal ADL-provided implementation for tensor expressions.
/// Reads @p N elements along @p Axis starting at @p index from @p self.
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

/// @brief Internal ADL-provided implementation for tensor expressions.
/// Writes @p value as @p N elements along @p Axis starting at @p index into @p self.
template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const tensor<T, NDims>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>&, const std::type_identity_t<vec<T, N>>& value)
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

/// @brief Evaluates an expression and returns the result as a tensor.
/// @tparam width processing width.
/// @tparam Axis axis to process along.
/// @tparam E expression type.
/// @param expr expression to evaluate.
/// @return Tensor holding the result of evaluating @p expr.
template <size_t width = 0, index_t Axis = infinite_size, typename E, typename Traits = expression_traits<E>>
tensor<typename Traits::value_type, Traits::dims> trender(const E& expr)
{
    static_assert(!Traits::get_shape().has_infinity());
    shape sh = Traits::get_shape(expr);
    tensor<typename Traits::value_type, Traits::dims> result(sh);
    process<width, Axis>(result, expr);
    return result;
}

/// @brief Evaluates an expression up to @p size and returns the result as a tensor.
/// @tparam width processing width.
/// @tparam Axis axis to process along.
/// @tparam E expression type.
/// @param expr expression to evaluate.
/// @param size maximum shape of the result.
/// @return Tensor holding the result of evaluating @p expr.
template <size_t width = 0, index_t Axis = infinite_size, typename E, typename Traits = expression_traits<E>>
tensor<typename Traits::value_type, Traits::dims> trender(const E& expr, shape<Traits::dims> size)
{
    shape sh = min(Traits::get_shape(expr), size);
    tensor<typename Traits::value_type, Traits::dims> result(sh);
    process<width, Axis>(result, expr, shape<Traits::dims>{ 0 }, sh);
    return result;
}

} // namespace KFR_ARCH_NAME

} // namespace kfr

namespace kfr
{
/// @brief Specialization of representation for tensor, providing string conversion.
template <typename T, kfr::index_t dims>
struct representation<kfr::tensor<T, dims>>
{
    using type = std::string;
    /// @brief Returns the string representation of @p value.
    static std::string get(const kfr::tensor<T, dims>& value) { return value.to_string(); }
};

/// @brief Specialization of representation for a formatted tensor, providing formatted string conversion.
template <char t, int width, int prec, typename T, kfr::index_t dims>
struct representation<fmt_t<kfr::tensor<T, dims>, t, width, prec>>
{
    using type = std::string;
    /// @brief Returns the formatted string representation of @p value.
    static std::string get(const fmt_t<kfr::tensor<T, dims>, t, width, prec>& value)
    {
        return array_to_string<fmt_t<T, t, width, prec>>(value.value.size(), value.value.data());
    }
};

} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
