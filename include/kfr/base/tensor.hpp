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

#include <optional>

#include "../cometa/array.hpp"

#include "../math/logical.hpp"
#include "../math/min_max.hpp"
#include "../simd/horizontal.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/read_write.hpp"
#include "../simd/types.hpp"
#include "memory.hpp"

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4324))

namespace kfr
{

using memory_finalizer = std::shared_ptr<void>;

template <typename Fn>
memory_finalizer KFR_INTRINSIC make_memory_finalizer(Fn&& fn)
{
    return std::shared_ptr<void>(nullptr, [fn = std::move(fn)](void*) { fn(); });
}

template <typename T, typename Derived, typename Dims>
struct tensor_subscript;

template <typename T, typename Derived, index_t... Dims>
struct tensor_subscript<T, Derived, std::integer_sequence<size_t, Dims...>>
{
    constexpr static inline size_t dims = sizeof...(Dims);

    using reference       = T&;
    using const_reference = const T&;

    reference KFR_MEM_INTRINSIC operator()(type_for<index_t, Dims>... idx) const
    {
        return static_cast<const Derived&>(*this).access(shape<dims>{ idx... });
    }
};

template <typename T, index_t NDims>
struct tensor : public tensor_subscript<T, tensor<T, NDims>, std::make_index_sequence<NDims>>
{
public:
    using value_type      = T;
    using pointer         = T* CMT_RESTRICT;
    using const_pointer   = const T* CMT_RESTRICT;
    using reference       = T&;
    using const_reference = const T&;
    using size_type       = index_t;

    constexpr static inline index_t dims = NDims;

    using shape_type = shape<dims>;

    struct tensor_iterator
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        const tensor* src;
        shape_type indices;

        KFR_MEM_INTRINSIC index_t flat_index() const { return src->calc_index(indices); }

        KFR_MEM_INTRINSIC bool is_end() const { return indices.front() == internal_generic::null_index; }

        template <size_t num>
        KFR_MEM_INTRINSIC shape<num> advance()
        {
            shape<num> result;
            for (size_t i = 0; i < num; ++i)
            {
                result[i] = src->calc_index(indices);
                ++*this;
                if (is_end())
                    break;
            }
            return result;
        }

        KFR_MEM_INTRINSIC T& operator*() { return src->m_data[flat_index()]; }
        KFR_MEM_INTRINSIC T* operator->() { return &operator*(); }

        // prefix
        KFR_MEM_INTRINSIC tensor_iterator& operator++()
        {
            if (!internal_generic::increment_indices(indices, kfr::shape<dims>(0), src->m_shape))
            {
                indices = internal_generic::null_index;
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

    KFR_MEM_INTRINSIC constexpr tensor()
        : m_data(0), m_size(0), m_is_contiguous(false), m_shape{}, m_strides{}
    {
    }

    KFR_MEM_INTRINSIC tensor(T* data, const shape_type& shape, const shape_type& strides,
                             memory_finalizer finalizer)
        : m_data(data), m_size(size_of_shape(shape)),
          m_is_contiguous(strides == internal_generic::strides_for_shape(shape)), m_shape(shape),
          m_strides(strides), m_finalizer(std::move(finalizer))
    {
    }

    KFR_MEM_INTRINSIC tensor(T* data, const shape_type& shape, memory_finalizer finalizer)
        : m_data(data), m_size(size_of_shape(shape)), m_is_contiguous(true), m_shape(shape),
          m_strides(internal_generic::strides_for_shape(shape)), m_finalizer(std::move(finalizer))
    {
    }

    KFR_INTRINSIC static T* allocate(size_t size) { return aligned_allocate<T>(size, 64); }

    KFR_INTRINSIC static void deallocate(T* ptr) { aligned_deallocate(ptr); }

    KFR_INTRINSIC explicit tensor(const shape_type& shape)
        : m_size(size_of_shape(shape)), m_is_contiguous(true), m_shape(shape),
          m_strides(internal_generic::strides_for_shape(shape))
    {
        T* ptr      = allocate(m_size);
        m_data      = ptr;
        m_finalizer = make_memory_finalizer([ptr]() { deallocate(ptr); });
    }
    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides)
        : m_size(size_of_shape(shape)),
          m_is_contiguous(strides == internal_generic::strides_for_shape(shape)), m_shape(shape),
          m_strides(strides)
    {
        T* ptr      = allocate(m_size);
        m_data      = ptr;
        m_finalizer = make_memory_finalizer([ptr]() { deallocate(ptr); });
    }
    KFR_INTRINSIC tensor(const shape_type& shape, T value) : tensor(shape)
    {
        std::fill(contiguous_begin(), contiguous_end(), value);
    }

    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides, T value) : tensor(shape, strides)
    {
        std::fill(begin(), end(), value);
    }
    KFR_INTRINSIC tensor(const shape_type& shape, std::initializer_list<T> values) : tensor(shape)
    {
        if (values.size() != m_size)
            throw std::runtime_error("Invalid initializer provided for kfr::tensor");
        std::copy(values.begin(), values.end(), contiguous_begin());
    }

    KFR_INTRINSIC tensor(const shape_type& shape, const shape_type& strides, std::initializer_list<T> values)
        : tensor(shape, strides)
    {
        if (values.size() != m_size)
            throw std::runtime_error("Invalid initializer provided for kfr::tensor");
        std::copy(values.begin(), values.end(), begin());
    }

    KFR_INTRINSIC pointer data() const { return m_data; }

    KFR_INTRINSIC size_type size() const { return m_size; }

    KFR_INTRINSIC tensor_iterator begin() const { return tensor_iterator{ this, 0 }; }
    KFR_INTRINSIC tensor_iterator end() const
    {
        return tensor_iterator{ this, internal_generic::null_index };
    }

    KFR_INTRINSIC void require_contiguous() const
    {
        if (!m_is_contiguous)
            throw std::runtime_error("Contiguous array is required");
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

    KFR_MEM_INTRINSIC index_t calc_index(const shape_type& indices) const { return indices.dot(m_strides); }

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

    tensor(const tensor&) = default;
    tensor(tensor&&)      = default;

#if defined(CMT_COMPILER_MSVC) && !defined(CMT_COMPILER_CLANG)
    tensor& operator=(const tensor& src) &
    {
        this->~tensor();
        new (this) tensor(src);
    }
    tensor& operator=(tensor&& src) &
    {
        this->~tensor();
        new (this) tensor(std::move(src));
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
            throw std::range_error("Tensors must have smae shape");
        std::copy(src.begin(), src.end(), begin());
    }
    KFR_MEM_INTRINSIC void assign(const T& scalar) const { std::fill(begin(), end(), scalar); }

    template <typename... Index>
    static constexpr bool has_tensor_range = (std::is_same_v<Index, tensor_range> || ...);

    template <typename... Index,
              size_t ndimOut = internal_generic::count_dimensions<Index...>() + (dims - sizeof...(Index)),
              std::enable_if_t<has_tensor_range<Index...> || (sizeof...(Index) < dims)>* = nullptr>
    KFR_MEM_INTRINSIC tensor<T, ndimOut> operator()(const Index&... idx) const
    {
        shape_type start{ internal_generic::get_start(idx)... };
        shape_type stop{ internal_generic::get_stop(idx)... };
        stop               = min(*stop, *m_shape);
        shape_type strides = m_strides;
        for (index_t i = sizeof...(Index); i < dims; ++i)
        {
            start[i] = 0;
            stop[i]  = m_shape[i];
        }
        T* data          = m_data + calc_index(start);
        shape_type shape = *stop - *start;

        return tensor<T, ndimOut>{
            data,
            internal_generic::compact_shape<dims, ndimOut, std::is_same_v<Index, tensor_range>...>(shape),
            internal_generic::compact_shape<dims, ndimOut, std::is_same_v<Index, tensor_range>...>(strides),
            m_finalizer,
        };
    }

    using tensor_subscript<T, tensor<T, NDims>, std::make_index_sequence<NDims>>::operator();

    template <index_t dims>
    KFR_MEM_INTRINSIC tensor<T, dims> reshape_may_copy(const shape<dims>& new_shape,
                                                       bool allow_copy = false) const
    {
        if (size_of_shape(new_shape) != m_size)
        {
            throw std::runtime_error("Invalid shape provided");
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
                throw std::runtime_error("reshape requires contiguous array");
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
    KFR_MEM_INTRINSIC tensor<T, dims> reshape(const shape<dims>& new_shape) const
    {
        return reshape_may_copy(new_shape, false);
    }

    KFR_MEM_INTRINSIC tensor<T, 1> flatten() const { return reshape(shape<1>{ m_size }, false); }

    KFR_MEM_INTRINSIC tensor<T, 1> flatten_may_copy(bool allow_copy = false) const
    {
        return reshape(shape<1>{ m_size }, allow_copy);
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
            throw std::range_error("Nout != m_size");
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

    template <typename Input, index_t Dims = expression_traits<Input>::dims>
    KFR_MEM_INTRINSIC const tensor& operator=(Input&& input) const&
    {
        tprocess(*this, input);
        return *this;
    }
    template <typename Input, index_t Dims = expression_traits<Input>::dims>
    KFR_MEM_INTRINSIC tensor& operator=(Input&& input) &&
    {
        tprocess(*this, input);
        return *this;
    }

    // template <typename Input, KFR_ENABLE_IF(is_input_expression<Input>)>
    // tensor(Input&& input) : tensor(make_vector<index_t>(input.size()))
    // {
    //     this->assign_expr(std::forward<Input>(input));
    // }

    template <size_t N, bool only_last = false>
    KFR_MEM_INTRINSIC shape<N> offsets(shape<dims> indices) const
    {
        kfr::shape<N> result;
        if constexpr (only_last)
        {
            index_t base = calc_index(indices);
            result       = base + enumerate(vec_shape<index_t, N>{}, m_shape.back());
        }
        else
        {
            for (index_t i = 0; i < N; ++i)
            {
                result[i] = calc_index(indices);
                internal_generic::increment_indices(indices, kfr::shape<dims>(0), m_shape);
            }
        }
        return result;
    }

    template <size_t N, bool only_last = false>
    KFR_MEM_INTRINSIC shape<N> offsets(size_t flat_index) const
    {
        kfr::shape<dims> indices;
        for (index_t i = 0; i < dims; ++i)
        {
            indices[dims - 1 - i] = flat_index % m_shape[dims - 1 - i];
            flat_index /= m_shape[dims - 1 - i];
        }
        return offsets<N, only_last>(indices);
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

template <typename T>
struct tensor<T, 0>
{
};

template <typename Container, CMT_ENABLE_IF(kfr::has_data_size<Container>),
          typename T = typename Container::value_type>
KFR_INTRINSIC tensor<T, 1> tensor_from_container(Container vector)
{
    struct vector_finalizer
    {
        mutable std::optional<Container> vector;
        void operator()(void*) const { vector.reset(); }
    };

    vector_finalizer finalizer{ std::move(vector) };

    memory_finalizer mem  = std::shared_ptr<void>(nullptr, std::move(finalizer));
    vector_finalizer* fin = std::get_deleter<vector_finalizer>(mem);

    return tensor<T, 1>(fin->vector->data(), make_vector<index_t>(fin->vector->size()), std::move(mem));
}

template <typename T, index_t Dims>
struct expression_traits<tensor<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;

    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof(const tensor<T, Dims>& self)
    {
        return self.shape();
    }
    KFR_MEM_INTRINSIC constexpr static shape<dims> shapeof() { return { 0 }; }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const tensor<T, NDims>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>&)
{
    const T* data = self.data() + self.calc_index(index);
    if (self.strides()[Axis] == 1)
        return read<N>(data);
    return gather_stride<N>(data, self.strides()[Axis]);
}

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const tensor<T, NDims>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>&, const identity<vec<T, N>>& value)
{
    T* data = self.data() + self.calc_index(index);
    if (self.strides()[Axis] == 1)
        return write(data, value);
    scatter_stride(data, value, self.strides()[Axis]);
}

template <typename T, index_t dims1, index_t dims2, typename Fn, index_t outdims = const_max(dims1, dims2)>
tensor<T, outdims> tapply(const tensor<T, dims1>& x, const tensor<T, dims2>& y, Fn&& fn)
{
    shape<outdims> xyshape = internal_generic::common_shape(x.shape(), y.shape());

    tensor<T, outdims> result(xyshape);

    shape<outdims> xshape = padlow<outdims - dims1>(*x.shape(), 1);
    shape<outdims> yshape = padlow<outdims - dims2>(*y.shape(), 1);

    tensor<T, outdims> xx = x.reshape(xshape);
    tensor<T, outdims> yy = y.reshape(yshape);

    result.iterate([&](T& val, const shape<outdims>& index)
                   { val = fn(xx.access(xshape.adapt(index)), yy.access(yshape.adapt(index))); });

    return result;
}

} // namespace CMT_ARCH_NAME

} // namespace kfr

namespace cometa
{
template <size_t dims>
struct representation<kfr::shape<dims>>
{
    using type = std::string;
    static std::string get(const kfr::shape<dims>& value)
    {
        if constexpr (dims == 0)
        {
            return "()";
        }
        else
        {
            std::string s;
            for (size_t i = 0; i < dims; ++i)
            {
                if (CMT_LIKELY(i > 0))
                    s += ", ";
                s += as_string(value[i]);
            }
            return s;
        }
    }
};
} // namespace cometa

CMT_PRAGMA_MSVC(warning(pop))
