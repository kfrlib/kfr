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

#include "../except.hpp"
#include "impl/static_array.hpp"

#include "../cometa/string.hpp"
#include "../simd/logical.hpp"
#include "../simd/min_max.hpp"
#include "../simd/shuffle.hpp"
#include "../simd/types.hpp"

#include <bitset>
#include <optional>

namespace kfr
{

#ifndef KFR_32BIT_INDICES
#if SIZE_MAX == UINT64_MAX
using index_t        = uint64_t;
using signed_index_t = int64_t;
#else
using index_t        = uint32_t;
using signed_index_t = int32_t;
#endif
#else
using index_t        = uint32_t;
using signed_index_t = int32_t;
#endif
constexpr inline index_t max_index_t         = std::numeric_limits<index_t>::max();
constexpr inline signed_index_t max_sindex_t = std::numeric_limits<signed_index_t>::max();

template <index_t val>
using cindex_t = cval_t<index_t, val>;

template <index_t val>
constexpr inline cindex_t<val> cindex{};

constexpr inline index_t infinite_size = max_index_t;

constexpr inline index_t undefined_size = 0;

constexpr inline index_t maximum_dims = 8;
CMT_INTRINSIC constexpr size_t size_add(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : x + y;
}

CMT_INTRINSIC constexpr size_t size_sub(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : (x > y ? x - y : 0);
}

CMT_INTRINSIC constexpr size_t size_min(size_t x) CMT_NOEXCEPT { return x; }

template <typename... Ts>
CMT_INTRINSIC constexpr size_t size_min(size_t x, size_t y, Ts... rest) CMT_NOEXCEPT
{
    return size_min(x < y ? x : y, rest...);
}

using dimset = static_array_of_size<i8, maximum_dims>; // std::array<i8, maximum_dims>;

template <index_t dims>
struct shape;

namespace internal_generic
{
template <index_t dims>
KFR_INTRINSIC bool increment_indices(shape<dims>& indices, const shape<dims>& start, const shape<dims>& stop,
                                     index_t dim = dims - 1);
} // namespace internal_generic

template <index_t dims>
struct shape : static_array_base<index_t, csizeseq_t<dims>>
{
    using base = static_array_base<index_t, csizeseq_t<dims>>;

    using base::base;

    constexpr shape(const base& a) : base(a) {}

    static_assert(dims < maximum_dims);

    template <int dummy = 0, KFR_ENABLE_IF(dummy == 0 && dims == 1)>
    operator index_t() const
    {
        return this->front();
    }

    template <typename TI>
    static constexpr shape from_std_array(const std::array<TI, dims>& a)
    {
        shape result;
        std::copy(a.begin(), a.end(), result.begin());
        return result;
    }

    template <typename TI = index_t>
    constexpr std::array<TI, dims> to_std_array() const
    {
        std::array<TI, dims> result{};
        std::copy(this->begin(), this->end(), result.begin());
        return result;
    }

    bool ge(const shape& other) const
    {
        if constexpr (dims == 1)
        {
            return this->front() >= other.front();
        }
        else
        {
            return all(**this >= *other);
        }
    }

    index_t trailing_zeros() const
    {
        for (index_t i = 0; i < dims; ++i)
        {
            if (revindex(i) != 0)
                return i;
        }
        return dims;
    }

    bool le(const shape& other) const
    {
        if constexpr (dims == 1)
        {
            return this->front() <= other.front();
        }
        else
        {
            return all(**this <= *other);
        }
    }

    constexpr shape add(index_t value) const
    {
        shape result = *this;
        result.back() += value;
        return result;
    }
    template <index_t Axis>
    constexpr shape add_at(index_t value, cval_t<index_t, Axis> = {}) const
    {
        shape result = *this;
        result[Axis] += value;
        return result;
    }
    constexpr shape add(const shape& other) const { return **this + *other; }
    constexpr shape sub(const shape& other) const { return **this - *other; }
    constexpr index_t sum() const { return (*this)->sum(); }

    constexpr bool has_infinity() const
    {
        for (index_t i = 0; i < dims; ++i)
        {
            if (CMT_UNLIKELY(this->operator[](i) == infinite_size))
                return true;
        }
        return false;
    }

    friend constexpr shape add_shape(const shape& lhs, const shape& rhs)
    {
        return lhs.bin(rhs, [](index_t x, index_t y) { return std::max(std::max(x, y), x + y); });
    }
    friend constexpr shape sub_shape(const shape& lhs, const shape& rhs)
    {
        return lhs.bin(rhs, [](index_t x, index_t y)
                       { return std::max(x, y) == infinite_size ? infinite_size : x - y; });
    }
    friend constexpr shape add_shape_undef(const shape& lhs, const shape& rhs)
    {
        return lhs.bin(rhs,
                       [](index_t x, index_t y)
                       {
                           bool inf   = std::max(x, y) == infinite_size;
                           bool undef = std::min(x, y) == undefined_size;
                           return inf ? infinite_size : undef ? undefined_size : x + y;
                       });
    }
    friend constexpr shape sub_shape_undef(const shape& lhs, const shape& rhs)
    {
        return lhs.bin(rhs,
                       [](index_t x, index_t y)
                       {
                           bool inf   = std::max(x, y) == infinite_size;
                           bool undef = std::min(x, y) == undefined_size;
                           return inf ? infinite_size : undef ? undefined_size : x - y;
                       });
    }

    friend constexpr shape min(const shape& x, const shape& y) { return x->min(*y); }

    constexpr const base& operator*() const { return static_cast<const base&>(*this); }

    constexpr const base* operator->() const { return static_cast<const base*>(this); }

    KFR_MEM_INTRINSIC constexpr size_t to_flat(const shape<dims>& indices) const
    {
        if constexpr (dims == 1)
        {
            return indices[0];
        }
        else if constexpr (dims == 2)
        {
            return (*this)[1] * indices[0] + indices[1];
        }
        else
        {
            size_t result = 0;
            size_t scale  = 1;
            CMT_LOOP_UNROLL
            for (size_t i = 0; i < dims; ++i)
            {
                result += scale * indices[dims - 1 - i];
                scale *= (*this)[dims - 1 - i];
            }
            return result;
        }
    }
    KFR_MEM_INTRINSIC constexpr shape<dims> from_flat(size_t index) const
    {
        if constexpr (dims == 1)
        {
            return { static_cast<index_t>(index) };
        }
        else if constexpr (dims == 2)
        {
            index_t sz = (*this)[1];
            return { static_cast<index_t>(index / sz), static_cast<index_t>(index % sz) };
        }
        else
        {
            shape<dims> indices;
            CMT_LOOP_UNROLL
            for (size_t i = 0; i < dims; ++i)
            {
                size_t sz             = (*this)[dims - 1 - i];
                indices[dims - 1 - i] = index % sz;
                index /= sz;
            }
            return indices;
        }
    }

    KFR_MEM_INTRINSIC constexpr index_t dot(const shape& other) const { return (*this)->dot(*other); }

    template <index_t indims>
    KFR_MEM_INTRINSIC constexpr shape adapt(const shape<indims>& other) const
    {
        static_assert(indims >= dims);
        return other.template trim<dims>()->min(**this - 1);
    }

    KFR_MEM_INTRINSIC constexpr index_t product() const { return (*this)->product(); }

    KFR_MEM_INTRINSIC constexpr dimset tomask() const
    {
        dimset result(0);
        for (index_t i = 0; i < dims; ++i)
        {
            result[i + maximum_dims - dims] = this->operator[](i) == 1 ? 0 : -1;
        }
        return result;
    }

    template <index_t new_dims>
    constexpr KFR_MEM_INTRINSIC shape<new_dims> extend(index_t value = infinite_size) const
    {
        static_assert(new_dims >= dims);
        if constexpr (new_dims == dims)
            return *this;
        else
            return shape<new_dims>{ shape<new_dims - dims>(value), *this };
    }

    template <index_t odims>
    constexpr shape<odims> trim() const
    {
        static_assert(odims <= dims);
        if constexpr (odims > 0)
        {
            return this->template slice<dims - odims, odims>();
        }
        else
        {
            return {};
        }
    }

    constexpr KFR_MEM_INTRINSIC shape<dims - 1> trunc() const
    {
        if constexpr (dims > 1)
        {
            return this->template slice<0, dims - 1>();
        }
        else
        {
            return {};
        }
    }

    KFR_MEM_INTRINSIC constexpr index_t revindex(size_t index) const
    {
        return index < dims ? this->operator[](dims - 1 - index) : 1;
    }
    KFR_MEM_INTRINSIC constexpr void set_revindex(size_t index, index_t val)
    {
        if (CMT_LIKELY(index < dims))
            this->operator[](dims - 1 - index) = val;
    }
};

template <>
struct shape<0>
{
    static constexpr size_t static_size = 0;

    static constexpr size_t size() { return static_size; }

    constexpr shape() = default;
    constexpr shape(index_t value) {}

    constexpr bool has_infinity() const { return false; }

    KFR_MEM_INTRINSIC size_t to_flat(const shape<0>& indices) const { return 0; }
    KFR_MEM_INTRINSIC shape<0> from_flat(size_t index) const { return {}; }

    template <index_t odims>
    KFR_MEM_INTRINSIC shape<0> adapt(const shape<odims>& other) const
    {
        return {};
    }

    index_t trailing_zeros() const { return 0; }

    KFR_MEM_INTRINSIC index_t dot(const shape& other) const { return 0; }

    KFR_MEM_INTRINSIC index_t product() const { return 0; }

    KFR_MEM_INTRINSIC dimset tomask() const { return dimset(-1); }

    template <index_t new_dims>
    constexpr KFR_MEM_INTRINSIC shape<new_dims> extend(index_t value = infinite_size) const
    {
        if constexpr (new_dims == 0)
            return *this;
        else
            return shape<new_dims>{ value };
    }

    template <index_t new_dims>
    constexpr shape<new_dims> trim() const
    {
        static_assert(new_dims == 0);
        return {};
    }

    KFR_MEM_INTRINSIC constexpr bool operator==(const shape<0>& other) const { return true; }
    KFR_MEM_INTRINSIC constexpr bool operator!=(const shape<0>& other) const { return false; }

    KFR_MEM_INTRINSIC constexpr index_t revindex(size_t index) const { return 1; }
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
        const static_array_of_size<index_t, maximum_dims> eset = set.template cast<index_t>();
        return in->template slice<indims - outdims, outdims>() &
               eset.template slice<maximum_dims - outdims, outdims>();
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

using opt_index_t = std::optional<signed_index_t>;

struct tensor_range
{
    opt_index_t start;
    opt_index_t stop;
    opt_index_t step;
};

constexpr KFR_INTRINSIC tensor_range trange(std::optional<signed_index_t> start = std::nullopt,
                                            std::optional<signed_index_t> stop  = std::nullopt,
                                            std::optional<signed_index_t> step  = std::nullopt)
{
    return { start, stop, step };
}

constexpr KFR_INTRINSIC tensor_range tall() { return trange(); }
constexpr KFR_INTRINSIC tensor_range tstart(signed_index_t start, signed_index_t step = 1)
{
    return trange(start, std::nullopt, step);
}
constexpr KFR_INTRINSIC tensor_range tstop(signed_index_t stop, signed_index_t step = 1)
{
    return trange(std::nullopt, stop, step);
}
constexpr KFR_INTRINSIC tensor_range tstep(signed_index_t step = 1)
{
    return trange(std::nullopt, std::nullopt, step);
}

namespace internal_generic
{

constexpr inline index_t null_index = max_index_t;

template <index_t dims>
constexpr KFR_INTRINSIC shape<dims> strides_for_shape(const shape<dims>& sh, index_t stride = 1)
{
    shape<dims> strides;
    if constexpr (dims > 0)
    {
        index_t n = stride;
        for (index_t i = 0; i < dims; ++i)
        {
            strides[dims - 1 - i] = n;
            n *= sh[dims - 1 - i];
        }
    }
    return strides;
}

template <size_t dims, size_t outdims, bool... ranges>
constexpr KFR_INTRINSIC shape<outdims> compact_shape(const shape<dims>& in)
{
    shape<outdims> result;
    constexpr std::array flags{ ranges... };
    size_t j = 0;
    for (size_t i = 0; i < dims; ++i)
    {
        if (CMT_LIKELY(i >= flags.size() || flags[i]))
        {
            result[j++] = in[i];
        }
    }
    return result;
}

template <index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
constexpr bool can_assign_from(const shape<dims1>& dst_shape, const shape<dims2>& src_shape)
{
    if constexpr (dims2 == 0)
    {
        return true;
    }
    else
    {
        for (size_t i = 0; i < outdims; ++i)
        {
            index_t dst_size = dst_shape.revindex(i);
            index_t src_size = src_shape.revindex(i);
            if (CMT_LIKELY(src_size == 1 || src_size == infinite_size || src_size == dst_size ||
                           dst_size == infinite_size))
            {
            }
            else
            {
                return false;
            }
        }
        return true;
    }
}

template <bool checked = false, index_t dims>
constexpr shape<dims> common_shape(const shape<dims>& shape)
{
    return shape;
}

template <bool checked = false, index_t dims1, index_t dims2, index_t outdims = const_max(dims1, dims2)>
KFR_MEM_INTRINSIC constexpr shape<outdims> common_shape(const shape<dims1>& shape1,
                                                        const shape<dims2>& shape2)
{
    shape<outdims> result;
    for (size_t i = 0; i < outdims; ++i)
    {
        index_t size1 = shape1.revindex(i);
        index_t size2 = shape2.revindex(i);
        if (CMT_UNLIKELY(!size1 || !size2))
        {
            result[outdims - 1 - i] = 0;
            continue;
        }

        if (CMT_UNLIKELY(size1 == infinite_size))
        {
            if (CMT_UNLIKELY(size2 == infinite_size))
            {
                result[outdims - 1 - i] = infinite_size;
            }
            else
            {
                result[outdims - 1 - i] = size2 == 1 ? infinite_size : size2;
            }
        }
        else
        {
            if (CMT_UNLIKELY(size2 == infinite_size))
            {
                result[outdims - 1 - i] = size1 == 1 ? infinite_size : size1;
            }
            else
            {
                if (CMT_LIKELY(size1 == 1 || size2 == 1 || size1 == size2))
                {
                    result[outdims - 1 - i] = std::max(size1, size2);
                }
                else
                {
                    // broadcast failed
                    if constexpr (checked)
                    {
                        KFR_LOGIC_CHECK(false, "invalid or incompatible shapes: ", shape1, " and ", shape2);
                    }
                    else
                    {
                        result = shape<outdims>(0);
                        return result;
                    }
                }
            }
        }
    }
    return result;
}

template <bool checked = false>
KFR_MEM_INTRINSIC constexpr shape<0> common_shape(const shape<0>& shape1, const shape<0>& shape2)
{
    return {};
}

template <bool checked    = false, index_t dims1, index_t dims2, index_t... dims,
          index_t outdims = const_max(dims1, dims2, dims...)>
KFR_MEM_INTRINSIC constexpr shape<outdims> common_shape(const shape<dims1>& shape1,
                                                        const shape<dims2>& shape2,
                                                        const shape<dims>&... shapes)
{
    return common_shape<checked>(shape1, common_shape(shape2, shapes...));
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
    if constexpr (dims > 0)
    {
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
    }
    else
    {
        return false;
    }
#endif
}

template <index_t dims>
KFR_INTRINSIC shape<dims> increment_indices_return(const shape<dims>& indices, const shape<dims>& start,
                                                   const shape<dims>& stop, index_t dim = dims - 1)
{
    shape<dims> result = indices;
    if (CMT_LIKELY(increment_indices(result, start, stop, dim)))
    {
        return result;
    }
    else
    {
        return shape<dims>(null_index);
    }
}

template <typename... Index>
constexpr KFR_INTRINSIC size_t count_dimensions()
{
    return ((std::is_same_v<std::decay_t<Index>, tensor_range> ? 1 : 0) + ...);
}

template <typename U>
struct type_of_list
{
    using value_type = U;
};

template <typename U>
struct type_of_list<std::initializer_list<U>>
{
    using value_type = typename type_of_list<U>::value_type;
};

template <typename U>
using type_of_list_t = typename type_of_list<U>::value_type;

template <typename U>
constexpr KFR_INTRINSIC shape<1> shape_of_list(const std::initializer_list<U>& list)
{
    return list.size();
}

template <typename U>
constexpr KFR_INTRINSIC auto shape_of_list(const std::initializer_list<std::initializer_list<U>>& list)
{
    return shape_of_list(*list.begin());
}

template <typename U>
constexpr KFR_INTRINSIC U list_get(const std::initializer_list<U>& list, const shape<1>& idx)
{
    return list[idx.front()];
}

template <typename U, index_t dims>
constexpr KFR_INTRINSIC auto list_get(const std::initializer_list<std::initializer_list<U>>& list,
                                      const shape<dims>& idx)
{
    return list_get(list[idx[0]], idx.template trim<dims - 1>());
}

template <typename U, typename T>
KFR_FUNCTION T* list_copy_recursively(const std::initializer_list<U>& list, T* dest)
{
    for (const auto& value : list)
        *dest++ = static_cast<T>(value);
    return dest;
}

template <typename U, typename T>
KFR_FUNCTION T* list_copy_recursively(const std::initializer_list<std::initializer_list<U>>& list, T* dest)
{
    for (const auto& sublist : list)
        dest = list_copy_recursively(sublist, dest);
    return dest;
}

} // namespace internal_generic

template <index_t dims>
constexpr KFR_INTRINSIC index_t size_of_shape(const shape<dims>& shape)
{
    index_t n = 1;
    if constexpr (dims > 0)
    {
        for (index_t i = 0; i < dims; ++i)
        {
            n *= shape[i];
        }
    }
    return n;
}

template <index_t Axis, size_t N>
struct axis_params
{
    constexpr static index_t axis  = Axis;
    constexpr static index_t width = N;
    constexpr static index_t value = N;

    constexpr axis_params() = default;
};

template <index_t Axis, size_t N>
constexpr inline const axis_params<Axis, N> axis_params_v{};

} // namespace kfr

namespace cometa
{
template <kfr::index_t dims>
struct representation<kfr::shape<dims>>
{
    using type = std::string;
    static std::string get(const kfr::shape<dims>& value)
    {
        if constexpr (dims == 0)
        {
            return "shape{}";
        }
        else
        {
            return "shape" + array_to_string(dims, value.data());
        }
    }
};

} // namespace cometa
