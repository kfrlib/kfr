/** @addtogroup array
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

#include "impl/static_array.hpp"

#include "../math/min_max.hpp"
#include "../simd/shuffle.hpp"
#include "../simd/types.hpp"
#include "../simd/vec.hpp"

#include <bitset>

namespace kfr
{

#ifndef KFR_32BIT_INDICES
using index_t = size_t;
#else
using index_t = uint32_t;
#endif
constexpr inline index_t max_index_t = std::numeric_limits<index_t>::max();

constexpr inline index_t infinite_size = max_index_t;

constexpr inline index_t maximum_dims = 8;

using dimset = vec<i8, maximum_dims>;

CMT_INTRINSIC constexpr index_t size_add(index_t x, index_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : x + y;
}

CMT_INTRINSIC constexpr index_t size_sub(index_t x, index_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : (x > y ? x - y : 0);
}

CMT_INTRINSIC constexpr index_t size_min(index_t x) CMT_NOEXCEPT { return x; }

template <typename... Ts>
CMT_INTRINSIC constexpr index_t size_min(index_t x, index_t y, Ts... rest) CMT_NOEXCEPT
{
    return size_min(x < y ? x : y, rest...);
}

template <index_t dims>
struct shape;

namespace internal_generic
{
template <index_t dims>
KFR_INTRINSIC bool increment_indices(shape<dims>& indices, const shape<dims>& start, const shape<dims>& stop,
                                     index_t dim = dims - 1);
}

template <index_t dims>
struct shape : static_array_base<index_t, csizeseq_t<dims>>
{
    using static_array_base<index_t, csizeseq_t<dims>>::static_array_base;

    static_assert(dims < maximum_dims);

    bool ge(const shape& other) const
    {
        if constexpr (dims == 1)
        {
            return front() >= other.front();
        }
        else
        {
            return all(**this >= *other);
        }
    }

    bool le(const shape& other) const
    {
        if constexpr (dims == 1)
        {
            return front() <= other.front();
        }
        else
        {
            return all(**this <= *other);
        }
    }

    shape add(index_t value) const
    {
        shape result = *this;
        result.back() += value;
        return result;
    }
    shape add(const shape& other) const { return **this + *other; }
    shape sub(const shape& other) const { return **this - *other; }

    shape add_inf(const shape& other) const
    {
        vec<index_t, dims> x    = **this;
        vec<index_t, dims> y    = *other;
        mask<index_t, dims> inf = (x == infinite_size) || (y == infinite_size);
        return select(inf, infinite_size, x + y);
    }
    shape sub_inf(const shape& other) const
    {
        vec<index_t, dims> x    = **this;
        vec<index_t, dims> y    = *other;
        mask<index_t, dims> inf = (x == infinite_size) || (y == infinite_size);
        return select(inf, infinite_size, x - y);
    }

    friend shape min(const shape& x, const shape& y) { return kfr::min(*x, *y); }

    KFR_MEM_INTRINSIC size_t to_flat(const shape<dims>& indices) const
    {
        size_t result = 0;
        size_t scale  = 1;
        for (size_t i = 0; i < dims; ++i)
        {
            result += scale * indices[dims - 1 - i];
            scale *= (*this)[dims - 1 - i];
        }
        return result;
    }
    KFR_MEM_INTRINSIC shape<dims> from_flat(size_t index) const
    {
        shape<dims> indices;
        for (size_t i = 0; i < dims; ++i)
        {
            size_t sz             = (*this)[dims - 1 - i];
            indices[dims - 1 - i] = index % sz;
            index /= sz;
        }
        return indices;
    }

    KFR_MEM_INTRINSIC index_t dot(const shape& other) const
    {
        if constexpr (dims == 1)
        {
            return (*this)[0] * other[0];
        }
        else if constexpr (dims == 2)
        {
            return (*this)[0] * other[0] + (*this)[1] * other[1];
        }
        else
        {
            return hdot(**this, *other);
        }
    }

    template <index_t indims>
    KFR_MEM_INTRINSIC shape adapt(const shape<indims>& other) const
    {
        static_assert(indims >= dims);
        return min(other.template trim<dims>(), **this - 1);
    }

    KFR_MEM_INTRINSIC index_t product() const { return hproduct(**this); }
    KFR_MEM_INTRINSIC constexpr index_t cproduct() const
    {
        index_t result = this->front();
        for (index_t i = 1; i < dims; i++)
            result *= this->operator[](i);
        return result;
    }

    KFR_MEM_INTRINSIC dimset tomask() const
    {
        dimset result = 0;
        for (index_t i = 0; i < dims; ++i)
        {
            result[i + maximum_dims - dims] = this->operator[](i) == 1 ? 0 : -1;
        }
        return result;
    }

    template <size_t odims>
    shape<odims> trim() const
    {
        static_assert(odims <= dims);
        if constexpr (odims > 0)
        {
            return slice<dims - odims, odims>(**this);
        }
        else
        {
            return {};
        }
    }

    KFR_MEM_INTRINSIC shape<dims - 1> trunc() const
    {
        if constexpr (dims > 1)
        {
            return slice<0, dims - 1>(**this);
        }
        else
        {
            return {};
        }
    }

    shape<dims + 1> extend() const { return concat(**this, vec{ index_t(0) }); }

    KFR_MEM_INTRINSIC constexpr index_t revindex(size_t index) const
    {
        return index < dims ? this->operator[](dims - 1 - index) : 1;
    }
    KFR_MEM_INTRINSIC constexpr void set_revindex(size_t index, index_t val)
    {
        if (index < dims)
            this->operator[](dims - 1 - index) = val;
    }
};

template <>
struct shape<0>
{
    static constexpr size_t static_size = 0;

    static constexpr size_t size() { return static_size; }

    shape() = default;
    shape(index_t value) {}

    KFR_MEM_INTRINSIC size_t to_flat(const shape<0>& indices) const { return 0; }
    KFR_MEM_INTRINSIC shape<0> from_flat(size_t index) const { return {}; }

    template <index_t odims>
    KFR_MEM_INTRINSIC shape<0> adapt(const shape<odims>& other) const
    {
        return {};
    }

    KFR_MEM_INTRINSIC size_t product() const { return 0; }

    KFR_MEM_INTRINSIC dimset tomask() const { return -1; }

    shape<1> extend() const { return { 0 }; }

    template <size_t new_dims>
    shape<new_dims> trim() const
    {
        static_assert(new_dims == 0);
        return {};
    }

    KFR_MEM_INTRINSIC bool operator==(const shape<0>& other) const { return true; }
    KFR_MEM_INTRINSIC bool operator!=(const shape<0>& other) const { return false; }

    KFR_MEM_INTRINSIC index_t revindex(size_t index) const { return 1; }
    KFR_MEM_INTRINSIC void set_revindex(size_t index, index_t val) {}
};

template <typename... Args>
shape(Args&&... args) -> shape<sizeof...(Args)>;

namespace internal_generic
{

template <index_t outdims, index_t indims>
KFR_MEM_INTRINSIC shape<outdims> adapt(const shape<indims>& in, const dimset& set)
{
    static_assert(indims >= outdims);
    if constexpr (outdims == 0)
    {
        return {};
    }
    else
    {
        const vec<std::make_signed_t<index_t>, maximum_dims> eset = cast<std::make_signed_t<index_t>>(set);
        return slice<indims - outdims, outdims>(*in) & slice<maximum_dims - outdims, outdims>(eset);
    }
}
template <index_t outdims>
KFR_MEM_INTRINSIC shape<outdims> adapt(const shape<0>& in, const dimset& set)
{
    static_assert(outdims == 0);
    return {};
}
} // namespace internal_generic

template <size_t Dims>
struct cursor
{
    shape<Dims> current;
    shape<Dims> minimum;
    shape<Dims> maximum;
};

struct tensor_range
{
    index_t start = 0;
    index_t stop  = max_index_t;
    index_t step  = 1;

    constexpr KFR_INTRINSIC index_t size() const { return stop - start; }
};

constexpr KFR_INTRINSIC tensor_range tstart(index_t start, index_t step = 1)
{
    return { start, max_index_t, step };
}
constexpr KFR_INTRINSIC tensor_range tstop(index_t stop, index_t step = 1) { return { 0, stop, step }; }
constexpr KFR_INTRINSIC tensor_range trange(index_t start, index_t stop, index_t step = 1)
{
    return { start, stop, step };
}
constexpr KFR_INTRINSIC tensor_range trange_n(index_t start, index_t size, index_t step = 1)
{
    return { start, start + size, step };
}
constexpr KFR_INTRINSIC tensor_range tall() { return { 0, max_index_t, 1 }; }

namespace internal_generic
{

constexpr inline index_t null_index = max_index_t;

template <index_t dims>
constexpr KFR_INTRINSIC shape<dims> strides_for_shape(const shape<dims>& sh, index_t stride = 1)
{
    shape<dims> strides;
    index_t n = stride;
    for (index_t i = 0; i < dims; ++i)
    {
        strides[dims - 1 - i] = n;
        n *= sh[dims - 1 - i];
    }
    return strides;
}

template <typename Index>
constexpr KFR_INTRINSIC index_t get_start(const Index& idx)
{
    if constexpr (std::is_same_v<std::decay_t<Index>, tensor_range>)
    {
        return idx.start;
    }
    else
    {
        static_assert(std::is_convertible_v<Index, index_t>);
        return static_cast<index_t>(idx);
    }
}
template <typename Index>
constexpr KFR_INTRINSIC index_t get_stop(const Index& idx)
{
    if constexpr (std::is_same_v<std::decay_t<Index>, tensor_range>)
    {
        return idx.stop;
    }
    else
    {
        static_assert(std::is_convertible_v<Index, index_t>);
        return static_cast<index_t>(idx) + 1;
    }
}
template <size_t dims, size_t outdims, bool... ranges>
constexpr KFR_INTRINSIC shape<outdims> compact_shape(const shape<dims>& in)
{
    shape<outdims> result;
    constexpr std::array flags{ ranges... };
    size_t j = 0;
    for (size_t i = 0; i < dims; ++i)
    {
        if (i >= flags.size() || flags[i])
        {
            result[j++] = in[i];
        }
    }
    return result;
}

template <index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
bool can_assign_from(const shape<dims1>& dst_shape, const shape<dims2>& src_shape)
{
    for (size_t i = 0; i < outdims; ++i)
    {
        index_t dst_size = dst_shape.revindex(i);
        index_t src_size = src_shape.revindex(i);
        if (src_size == 1 || src_size == infinite_size || src_size == dst_size)
        {
        }
        else
        {
            return false;
        }
    }
    return true;
}

template <index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
constexpr shape<outdims> common_shape(const shape<dims1>& shape1, const shape<dims2>& shape2)
{
    shape<outdims> result;
    for (size_t i = 0; i < outdims; ++i)
    {
        index_t size1 = shape1.revindex(i);
        index_t size2 = shape2.revindex(i);
        if (size1 == 1 || size2 == 1 || size1 == size2)
        {
            result[outdims - 1 - i] = std::max(size1, size2);
        }
        else
        {
            // broadcast failed
            result = 0;
            return result;
        }
    }
    return result;
}

template <>
KFR_MEM_INTRINSIC shape<0> common_shape(const shape<0>& shape1, const shape<0>& shape2)
{
    return {};
}

template <index_t dims1, index_t dims2>
KFR_MEM_INTRINSIC bool same_layout(const shape<dims1>& x, const shape<dims2>& y)
{
    for (index_t i = 0, j = 0;;)
    {
        while (i < dims1 && x[i] == 1)
            ++i;
        while (j < dims2 && y[j] == 1)
            ++j;
        if (i == dims1 && j == dims2)
        {
            return true;
        }
        if (i < dims1 && j < dims2)
        {
            if (x[i] != y[j])
                return false;
        }
        else
        {
            return false;
        }
        ++i;
        ++j;
    }
}

#ifdef KFR_VEC_INDICES
template <size_t step, index_t dims>
KFR_INTRINSIC vec<index_t, dims> increment_indices(vec<index_t, dims> indices,
                                                   const vec<index_t, dims>& start,
                                                   const vec<index_t, dims>& stop)
{
    indices = indices + make_vector(cconcat(cvalseq<index_t, dims - 1 - step, 0, 0>, cvalseq<index_t, 1, 1>,
                                            cvalseq<index_t, step, 0, 0>));

    if constexpr (step + 1 < dims)
    {
        vec<bit<index_t>, dims> mask = indices >= stop;
        if (CMT_LIKELY(!any(mask)))
            return indices;
        indices = blend(indices, start, cconcat(csizeseq<dims - step - 1, 0, 0>, csizeseq<step + 1, 1, 0>));

        return increment_indices<step + 1>(indices, stop);
    }
    else
    {
        return indices;
    }
}
#endif

template <index_t dims>
KFR_INTRINSIC bool compare_indices(const shape<dims>& indices, const shape<dims>& stop,
                                   index_t dim = dims - 1)
{
    CMT_LOOP_UNROLL
    for (int i = dim; i >= 0; --i)
    {
        if (CMT_UNLIKELY(indices[i] >= stop[i]))
            return false;
    }
    return true;
}

template <index_t dims>
KFR_INTRINSIC bool increment_indices(shape<dims>& indices, const shape<dims>& start, const shape<dims>& stop,
                                     index_t dim)
{
#ifdef KFR_VEC_INDICES
    vec<index_t, dims> idx = increment_indices<0>(*indices, *start, *stop);
    indices                = idx;
    if (any(idx == *stop))
        return false;
    return true;
#else

    indices[dim] += 1;
    CMT_LOOP_UNROLL
    for (int i = dim; i >= 0;)
    {
        if (CMT_LIKELY(indices[i] < stop[i]))
            return true;
        // carry
        indices[i] = start[i];
        --i;
        if (i < 0)
        {
            return false;
        }
        indices[i] += 1;
    }
    return true;
#endif
}

template <index_t dims>
KFR_INTRINSIC shape<dims> increment_indices_return(const shape<dims>& indices, const shape<dims>& start,
                                                   const shape<dims>& stop, index_t dim = dims - 1)
{
    shape<dims> result = indices;
    if (increment_indices(result, start, stop, dim))
    {
        return result;
    }
    else
    {
        return null_index;
    }
}

template <typename... Index>
constexpr KFR_INTRINSIC size_t count_dimensions()
{
    return ((std::is_same_v<std::decay_t<Index>, tensor_range> ? 1 : 0) + ...);
}
} // namespace internal_generic

template <index_t dims>
constexpr KFR_INTRINSIC index_t size_of_shape(const shape<dims>& shape)
{
    index_t n = 1;
    for (index_t i = 0; i < dims; ++i)
    {
        n *= shape[i];
    }
    return n;
}

} // namespace kfr
