/** @addtogroup types
 *  @{
 */
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

#include "../except.hpp"
#include "impl/static_array.hpp"

#include "../meta/string.hpp"
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
/// @brief Maximum representable value of @ref index_t.
constexpr inline index_t max_index_t = std::numeric_limits<index_t>::max();

/// @brief Maximum representable value of @ref signed_index_t.
constexpr inline signed_index_t max_sindex_t = std::numeric_limits<signed_index_t>::max();

/// @brief Compile-time constant of type @ref index_t.
///
/// Used to construct values of type @c index_t that are known at compile time
/// and can participate in constant expressions, similar to @c std::integral_constant.
/// @tparam val The value of the constant.
template <index_t val>
using cindex_t = cval_t<index_t, val>;

/// @brief Variable template form of @ref cindex_t.
///
/// @code
/// constexpr auto a = cindex<3>;  // kfr::cindex_t<3>{}
/// @endcode
/// @tparam val The value of the constant.
template <index_t val>
constexpr inline cindex_t<val> cindex{};

/// @brief Sentinel value denoting a dimension of unbounded/unknown size.
///
/// Used in @ref shape and tensor operations to indicate that a dimension
/// can grow to any size (for example, the inner dimension of a broadcast
/// or an output axis of a generator expression).
constexpr inline index_t infinite_size = max_index_t;

/// @brief Sentinel value denoting a dimension whose size is not yet known.
///
/// Used in @ref shape to indicate that a dimension is unspecified
/// (for example, the default shape of an unevaluated expression).
constexpr inline index_t undefined_size = 0;

/// @brief Upper bound on the number of dimensions supported by @ref shape.
constexpr inline index_t maximum_dims = 16;

/// @brief Saturating addition for shape sizes.
///
/// Returns @ref infinite_size if either operand is @ref infinite_size,
/// otherwise returns the regular sum of @p x and @p y.
/// @param x Left operand.
/// @param y Right operand.
/// @return The sum, or @ref infinite_size if either operand is infinite.
KFR_INTRINSIC constexpr size_t size_add(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : x + y;
}

/// @brief Saturating subtraction for shape sizes.
///
/// Returns @ref infinite_size if either operand is @ref infinite_size,
/// otherwise returns @c x - @c y, clamped to zero (never negative).
/// @param x Left operand.
/// @param y Right operand.
/// @return The difference (clamped to zero), or @ref infinite_size
///         if either operand is infinite.
KFR_INTRINSIC constexpr size_t size_sub(size_t x, size_t y)
{
    return (x == infinite_size || y == infinite_size) ? infinite_size : (x > y ? x - y : 0);
}

/// @brief Returns the minimum of a single value.
/// @param x The value to return.
/// @return @p x.
KFR_INTRINSIC constexpr size_t size_min(size_t x) noexcept { return x; }

/// @brief Returns the minimum of two or more values (recursively).
///
/// @c size_min propagates the @ref infinite_size and @ref undefined_size
/// sentinels transparently because the comparison is performed on the
/// raw values; callers that need to treat those sentinels specially
/// should do so explicitly.
/// @tparam Ts Types of the remaining arguments (deduced).
/// @param x First value.
/// @param y Second value.
/// @param rest Remaining values.
/// @return The smallest of all the arguments.
template <typename... Ts>
KFR_INTRINSIC constexpr size_t size_min(size_t x, size_t y, Ts... rest) noexcept
{
    return size_min(x < y ? x : y, rest...);
}

/// @brief Fixed-size bitmask indexed by tensor axis.
///
/// Each element is non-zero (all bits set) if the corresponding axis
/// is "active" in the mask, or zero if it is collapsed. The array is
/// aligned to @ref maximum_dims so it can be sliced to match tensors
/// of any rank up to that limit.
using dimset = static_array_of_size<i8, maximum_dims>; // std::array<i8, maximum_dims>;

/// @brief Fixed-rank multidimensional shape used by @ref tensor.
///
/// A @c shape<Dims> stores the size of each axis of a tensor. Dims is a
/// compile-time constant. Use @ref shape<0> for a scalar and
/// @ref shape<dynamic_shape> for a shape whose rank is only known at
/// run time.
///
/// @tparam dims Number of dimensions. Use 0 for a scalar or
///         @ref dynamic_shape for a run-time rank.
template <index_t dims>
struct shape;

namespace internal_generic
{
template <index_t dims>
KFR_INTRINSIC bool increment_indices(shape<dims>& indices, const shape<dims>& start, const shape<dims>& stop,
                                     index_t dim = dims - 1);
} // namespace internal_generic

/// @brief Fixed-rank multidimensional shape used by @ref tensor.
///
/// A @c shape<Dims> stores the size of each axis of a tensor and provides
/// utilities for layout arithmetic (broadcasting, slicing, transposing,
/// flat indexing, and so on). The specializations @ref shape<0> and
/// @ref shape<dynamic_shape> model scalar and run-time-rank shapes,
/// respectively.
///
/// @tparam Dims Number of dimensions. Use 0 for a scalar,
///         @ref dynamic_shape for a run-time rank, or any other value up
///         to @ref maximum_dims.
template <index_t Dims>
struct shape : static_array_base<index_t, csizeseq_t<Dims>>
{
    static_assert(Dims <= 256, "Too many dimensions");
    /// @brief Underlying static array storing one @ref index_t per axis.
    using base = static_array_base<index_t, csizeseq_t<Dims>>;

    using base::base;

    /// @brief Construct a shape from a base static array.
    constexpr shape(const base& a) : base(a) {}

    static_assert(Dims <= maximum_dims);

    /// @brief Returns the number of dimensions (compile-time constant).
    static constexpr size_t dims() { return base::static_size; }

    /// @brief Implicit conversion to @ref index_t for 1D shapes.
    ///
    /// Allows a 1D shape to be used wherever a single size is expected.
    /// @return The size of the only axis.
    template <int dummy = 0>
        requires(Dims == 1)
    operator index_t() const
    {
        return this->front();
    }

    /// @brief Build a @c shape from a @c std::array.
    /// @tparam TI Element type of the input array (typically @ref index_t).
    /// @param a Source array of exactly @c Dims elements.
    /// @return A shape whose elements are copied from @p a.
    template <typename TI>
    static constexpr shape from_std_array(const std::array<TI, Dims>& a)
    {
        shape result;
        std::copy(a.begin(), a.end(), result.begin());
        return result;
    }

    /// @brief Convert the shape to a @c std::array.
    /// @tparam TI Element type of the output array (defaults to @ref index_t).
    /// @return A new @c std::array containing the shape's elements.
    template <typename TI = index_t>
    constexpr std::array<TI, Dims> to_std_array() const
    {
        std::array<TI, Dims> result{};
        std::copy(this->begin(), this->end(), result.begin());
        return result;
    }

    /// @brief Element-wise greater-than-or-equal comparison.
    /// @param other The shape to compare against.
    /// @return @c true if every axis of @c *this is at least as large as
    ///         the corresponding axis of @p other.
    bool ge(const shape& other) const
    {
        if constexpr (Dims == 1)
        {
            return this->front() >= other.front();
        }
        else
        {
            return all(**this >= *other);
        }
    }

    /// @brief Number of trailing axes of size 1, counting from the back.
    ///
    /// Returns the position of the first axis (from the front) whose
    /// reverse-indexed size is not zero. In other words, the number of
    /// trailing length-1 axes in a row-major layout.
    /// @return The number of trailing size-1 axes, in the range @c [0, Dims].
    index_t trailing_zeros() const
    {
        for (index_t i = 0; i < Dims; ++i)
        {
            if (revindex(i) != 0)
                return i;
        }
        return Dims;
    }

    /// @brief Element-wise less-than-or-equal comparison.
    /// @param other The shape to compare against.
    /// @return @c true if every axis of @c *this is at most as large as
    ///         the corresponding axis of @p other.
    bool le(const shape& other) const
    {
        if constexpr (Dims == 1)
        {
            return this->front() <= other.front();
        }
        else
        {
            return all(**this <= *other);
        }
    }

    /// @brief Add @p value to the last axis.
    /// @param value Amount to add to the trailing axis.
    /// @return A new shape with the trailing axis increased by @p value.
    constexpr shape add(index_t value) const
    {
        shape result = *this;
        result.back() += value;
        return result;
    }
    /// @brief Add @p value to the axis selected by the template parameter.
    /// @tparam Axis Index of the axis to modify (compile-time constant).
    /// @param value Amount to add to @c Axis.
    /// @return A new shape with axis @c Axis increased by @p value.
    template <index_t Axis>
    constexpr shape add_at(index_t value, cval_t<index_t, Axis> = {}) const
    {
        shape result = *this;
        result[Axis] += value;
        return result;
    }
    /// @brief Element-wise addition of two shapes.
    /// @param other Shape to add.
    /// @return A new shape where each axis is the sum of the two operands.
    constexpr shape add(const shape& other) const { return **this + *other; }
    /// @brief Element-wise subtraction of two shapes.
    /// @param other Shape to subtract.
    /// @return A new shape where each axis is the difference of the two operands.
    constexpr shape sub(const shape& other) const { return **this - *other; }
    /// @brief Sum of every axis size.
    /// @return The total number of elements summed across all axes.
    constexpr index_t sum() const { return (*this)->sum(); }

    /// @brief Detect the @ref infinite_size sentinel among the axes.
    /// @return @c true if any axis equals @ref infinite_size.
    constexpr bool has_infinity() const
    {
        for (index_t i = 0; i < Dims; ++i)
        {
            if (KFR_UNLIKELY(this->operator[](i) == infinite_size))
                return true;
        }
        return false;
    }

    /// @brief Combine two shapes using the broadcasting rule
    /// @c max(max(x, y), x + y).
    ///
    /// Per axis the result is @c max(x, y) when one operand is @c 1 or
    /// the two are equal, and the sum when the axes are concatenated.
    /// @param lhs Left-hand shape.
    /// @param rhs Right-hand shape.
    /// @return A shape representing the union of the two extents.
    friend constexpr shape add_shape(const shape& lhs, const shape& rhs)
    {
        return lhs.bin(rhs, [](index_t x, index_t y) { return std::max(std::max(x, y), x + y); });
    }
    /// @brief Subtract one shape from another, treating
    /// @ref infinite_size as infinite.
    ///
    /// Per axis the result is @c x - y, or @ref infinite_size if either
    /// operand is @ref infinite_size (since subtracting from infinity
    /// stays infinite).
    /// @param lhs Left-hand shape.
    /// @param rhs Right-hand shape.
    /// @return A shape representing the difference, with @ref infinite_size
    ///         propagated.
    friend constexpr shape sub_shape(const shape& lhs, const shape& rhs)
    {
        return lhs.bin(rhs, [](index_t x, index_t y)
                       { return std::max(x, y) == infinite_size ? infinite_size : x - y; });
    }
    /// @brief Variant of @ref add_shape that also propagates
    /// @ref undefined_size.
    ///
    /// Per axis: if either operand is @ref infinite_size the result is
    /// @ref infinite_size; else if either is @ref undefined_size the result
    /// is @ref undefined_size; otherwise the sum.
    /// @param lhs Left-hand shape.
    /// @param rhs Right-hand shape.
    /// @return A shape with the sentinels propagated appropriately.
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
    /// @brief Variant of @ref sub_shape that also propagates
    /// @ref undefined_size.
    ///
    /// Per axis: if either operand is @ref infinite_size the result is
    /// @ref infinite_size; else if either is @ref undefined_size the result
    /// is @ref undefined_size; otherwise the difference.
    /// @param lhs Left-hand shape.
    /// @param rhs Right-hand shape.
    /// @return A shape with the sentinels propagated appropriately.
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

    /// @brief Element-wise minimum of two shapes.
    /// @param x First shape.
    /// @param y Second shape.
    /// @return A shape where each axis is the smaller of the two operands.
    friend constexpr shape min(const shape& x, const shape& y) { return x->min(*y); }

    /// @brief Access the underlying static array.
    /// @return A const reference to the @ref base storage.
    constexpr const base& operator*() const { return static_cast<const base&>(*this); }

    /// @brief Pointer-like access to the underlying static array.
    /// @return A const pointer to the @ref base storage.
    constexpr const base* operator->() const { return static_cast<const base*>(this); }

    /// @brief Convert multi-dimensional indices to a flat offset.
    ///
    /// Computes the linear position of @p indices in a contiguous
    /// row-major layout with the sizes given by @c *this.
    /// @param indices Multi-dimensional index.
    /// @return The flat linear offset.
    KFR_MEM_INTRINSIC constexpr size_t to_flat(const shape<Dims>& indices) const
    {
        if constexpr (Dims == 1)
        {
            return indices[0];
        }
        else if constexpr (Dims == 2)
        {
            return (*this)[1] * indices[0] + indices[1];
        }
        else
        {
            size_t result = 0;
            size_t scale  = 1;
            KFR_LOOP_UNROLL
            for (size_t i = 0; i < Dims; ++i)
            {
                result += scale * indices[Dims - 1 - i];
                scale *= (*this)[Dims - 1 - i];
            }
            return result;
        }
    }
    /// @brief Convert a flat offset to multi-dimensional indices.
    ///
    /// Inverse of @ref to_flat: given a linear offset into a contiguous
    /// row-major buffer, returns the corresponding multi-dimensional
    /// indices.
    /// @param index Flat linear offset.
    /// @return The multi-dimensional index.
    KFR_MEM_INTRINSIC constexpr shape<Dims> from_flat(size_t index) const
    {
        if constexpr (Dims == 1)
        {
            return { static_cast<index_t>(index) };
        }
        else if constexpr (Dims == 2)
        {
            index_t sz = (*this)[1];
            return { static_cast<index_t>(index / sz), static_cast<index_t>(index % sz) };
        }
        else
        {
            shape<Dims> indices;
            KFR_LOOP_UNROLL
            for (size_t i = 0; i < Dims; ++i)
            {
                size_t sz             = (*this)[Dims - 1 - i];
                indices[Dims - 1 - i] = index % sz;
                index /= sz;
            }
            return indices;
        }
    }

    /// @brief Dot product of the two shapes (element-wise multiply and sum).
    /// @param other The other shape.
    /// @return The sum of the products of corresponding axes.
    KFR_MEM_INTRINSIC constexpr index_t dot(const shape& other) const { return (*this)->dot(*other); }

    /// @brief Adapt a higher-rank shape to the current rank for indexing.
    ///
    /// Trims @p other to @c Dims axes and clamps each axis to the range
    /// @c [0, size - 1] (when @c stop is @c false) or @c [0, size]
    /// (when @c stop is @c true). Used internally by tensor subscript
    /// operations to map a caller-provided shape to the storage layout.
    /// @tparam indims Rank of the source shape (must be at least @c Dims).
    /// @tparam stop   When @c true, the upper bound is inclusive.
    /// @param other   The higher-rank shape to adapt.
    /// @return The adapted shape of rank @c Dims.
    template <index_t indims, bool stop = false>
    KFR_MEM_INTRINSIC constexpr shape adapt(const shape<indims>& other, cbool_t<stop> = {}) const
    {
        static_assert(indims >= Dims);
        if constexpr (stop)
            return other.template trim<Dims>()->min(**this);
        else
            return other.template trim<Dims>()->min(**this - 1);
    }

    /// @brief Product of every axis size (total number of elements).
    /// @return The product of all axes.
    KFR_MEM_INTRINSIC constexpr index_t product() const { return (*this)->product(); }

    /// @brief Build a @ref dimset mask with bits set for non-collapsed axes.
    ///
    /// An axis contributes 0 to the mask if its size is 1 (collapsed for
    /// broadcasting) and -1 (all bits set) otherwise. The mask is aligned
    /// to @ref maximum_dims so it can be sliced to match any rank.
    /// @return A @ref dimset describing which axes are non-trivial.
    KFR_MEM_INTRINSIC constexpr dimset tomask() const
    {
        dimset result(0);
        for (index_t i = 0; i < Dims; ++i)
        {
            result[i + maximum_dims - Dims] = this->operator[](i) == 1 ? 0 : -1;
        }
        return result;
    }

    /// @brief Promote the shape to a higher rank by prepending axes.
    ///
    /// Adds @c (new_dims - Dims) leading axes of size @p value
    /// (defaulting to @ref infinite_size) and keeps the existing axes
    /// unchanged. When @c new_dims == @c Dims, returns the shape as is.
    /// @tparam new_dims The desired rank (must be at least @c Dims).
    /// @param value Size of the prepended axes.
    /// @return A shape of rank @c new_dims.
    template <index_t new_dims>
    constexpr KFR_MEM_INTRINSIC shape<new_dims> extend(index_t value = infinite_size) const
    {
        static_assert(new_dims >= Dims);
        if constexpr (new_dims == Dims)
            return *this;
        else
            return shape<new_dims>{ shape<new_dims - Dims>(value), *this };
    }

    /// @brief Reduce the shape to a lower rank by dropping leading axes.
    ///
    /// Keeps the last @c odims axes. When @c odims is zero, returns
    /// the empty shape @ref shape<0>.
    /// @tparam odims Desired rank (must not exceed @c Dims).
    /// @return A shape of rank @c odims.
    template <index_t odims>
    constexpr shape<odims> trim() const
    {
        static_assert(odims <= Dims);
        if constexpr (odims > 0)
        {
            return this->template slice<Dims - odims, odims>();
        }
        else
        {
            return {};
        }
    }

    /// @brief Rotate the axes left by one position.
    ///
    /// For @c Dims == 4, @c [a, b, c, d] becomes @c [b, c, d, a].
    /// @return A new shape with the axes cyclically shifted left.
    // 0,1,2,3 -> 1,2,3,0
    constexpr KFR_MEM_INTRINSIC shape rotate_left() const
    {
        return this->shuffle(csizeseq<Dims, 1> % csize<Dims>);
    }

    /// @brief Rotate the axes right by one position.
    ///
    /// For @c Dims == 4, @c [a, b, c, d] becomes @c [d, a, b, c].
    /// @return A new shape with the axes cyclically shifted right.
    // 0,1,2,3 -> 3,0,1,2
    constexpr KFR_MEM_INTRINSIC shape rotate_right() const
    {
        return this->shuffle(csizeseq<Dims, Dims - 1> % csize<Dims>);
    }

    /// @brief Drop the last axis.
    /// @return A shape of rank @c Dims - 1 (or the empty shape for
    ///         @c Dims == 1).
    constexpr KFR_MEM_INTRINSIC shape<Dims - 1> remove_back() const
    {
        if constexpr (Dims > 1)
        {
            return this->template slice<0, Dims - 1>();
        }
        else
        {
            return {};
        }
    }
    /// @brief Drop the first axis.
    /// @return A shape of rank @c Dims - 1 (or the empty shape for
    ///         @c Dims == 1).
    constexpr KFR_MEM_INTRINSIC shape<Dims - 1> remove_front() const
    {
        if constexpr (Dims > 1)
        {
            return this->template slice<1, Dims - 1>();
        }
        else
        {
            return {};
        }
    }

    /// @brief Drop the last axis. Synonym for @ref remove_back.
    /// @return A shape of rank @c Dims - 1.
    constexpr KFR_MEM_INTRINSIC shape<Dims - 1> trunc() const { return remove_back(); }

    /// @brief Read an axis by its position from the back.
    /// @param index Number of axes from the back (0 is the last axis).
    /// @return The size of that axis, or 1 if @p index is out of range.
    KFR_MEM_INTRINSIC constexpr index_t revindex(size_t index) const
    {
        return index < Dims ? this->operator[](Dims - 1 - index) : 1;
    }
    /// @brief Write an axis by its position from the back.
    /// @param index Number of axes from the back (0 is the last axis).
    /// @param val   New value to store.
    /// @note Out-of-range indices are silently ignored.
    KFR_MEM_INTRINSIC constexpr void set_revindex(size_t index, index_t val)
    {
        if (KFR_LIKELY(index < Dims))
            this->operator[](Dims - 1 - index) = val;
    }

    /// @brief Reverse the order of the axes.
    /// @return A new shape with the axes in reverse order.
    KFR_MEM_INTRINSIC constexpr shape transpose() const
    {
        return this->shuffle(csizeseq<Dims, Dims - 1, -1>);
    }
};

/// @brief Scalar shape specialization.
///
/// Models the shape of a zero-dimensional tensor. The constructor
/// ignores its argument so that expressions like
/// @c shape<0>(undefined_size) and @c shape<0>(infinite_size) compile.
template <>
struct shape<0>
{
    /// @brief Compile-time size of the shape, always zero.
    static constexpr size_t static_size = 0;

    /// @brief Returns the number of elements (always 0).
    static constexpr size_t size() { return static_size; }

    /// @brief Returns the number of dimensions (always 0).
    static constexpr size_t dims() { return static_size; }

    /// @brief Default-construct an empty (scalar) shape.
    constexpr shape() = default;
    /// @brief Construct a scalar shape; the value is ignored.
    constexpr shape(index_t value) {}

    /// @brief A scalar shape has no axes, so it can never contain
    /// @ref infinite_size.
    /// @return Always @c false.
    constexpr bool has_infinity() const { return false; }

    /// @brief Flat offset for a scalar; the only valid offset is 0.
    /// @return 0.
    KFR_MEM_INTRINSIC size_t to_flat(const shape<0>& indices) const { return 0; }
    /// @brief Inverse of @ref to_flat: returns the empty shape.
    /// @return The empty @ref shape<0>.
    KFR_MEM_INTRINSIC shape<0> from_flat(size_t index) const { return {}; }

    /// @brief Adapt any shape to a scalar; always returns the empty shape.
    /// @tparam odims Rank of the input shape.
    /// @return The empty @ref shape<0>.
    template <index_t odims, bool stop = false>
    KFR_MEM_INTRINSIC shape<0> adapt(const shape<odims>& other, cbool_t<stop> = {}) const
    {
        return {};
    }

    /// @brief A scalar shape has no trailing collapsed axes.
    /// @return 0.
    index_t trailing_zeros() const { return 0; }

    /// @brief Dot product with another scalar shape; the empty product is 0.
    /// @return 0.
    KFR_MEM_INTRINSIC index_t dot(const shape& other) const { return 0; }

    /// @brief Product of all axes; the empty product is 0.
    /// @return 0.
    KFR_MEM_INTRINSIC index_t product() const { return 0; }

    /// @brief Build a @ref dimset with all bits set.
    /// @return A fully-active mask.
    KFR_MEM_INTRINSIC dimset tomask() const { return dimset(-1); }

    /// @brief Promote the scalar shape to a higher rank by prepending axes
    /// of size @p value.
    /// @tparam new_dims Desired rank.
    /// @param value Size of the prepended axes (defaults to @ref infinite_size).
    /// @return A shape of rank @c new_dims with all axes of size @p value,
    ///         or the empty shape when @c new_dims is 0.
    template <index_t new_dims>
    constexpr KFR_MEM_INTRINSIC shape<new_dims> extend(index_t value = infinite_size) const
    {
        if constexpr (new_dims == 0)
            return *this;
        else
            return shape<new_dims>{ value };
    }

    /// @brief Reduce the scalar shape to a smaller rank.
    ///
    /// Only the degenerate case @c new_dims == 0 is supported.
    /// @tparam new_dims Must be 0.
    /// @return The empty @ref shape<0>.
    template <index_t new_dims>
    constexpr shape<new_dims> trim() const
    {
        static_assert(new_dims == 0);
        return {};
    }

    /// @brief Two scalar shapes are always equal.
    KFR_MEM_INTRINSIC constexpr bool operator==(const shape<0>& other) const { return true; }
    /// @brief Two scalar shapes are never unequal.
    KFR_MEM_INTRINSIC constexpr bool operator!=(const shape<0>& other) const { return false; }

    /// @brief Read an axis by its position from the back; always returns 1
    /// because there are no axes.
    KFR_MEM_INTRINSIC constexpr index_t revindex(size_t index) const { return 1; }
    /// @brief Set an axis by its position from the back; a no-op.
    KFR_MEM_INTRINSIC void set_revindex(size_t index, index_t val) {}
};

/// @brief Sentinel rank value indicating a run-time-determined rank.
///
/// Used as the template argument of @ref shape to request the
/// variable-rank specialization backed by @c std::vector.
constexpr inline size_t dynamic_shape = std::numeric_limits<size_t>::max();

/// @brief Run-time-rank shape specialization.
///
/// Backed by a @c std::vector so the number of dimensions is only known
/// at run time. Supports conversion from any fixed-rank @ref shape.
template <>
struct shape<dynamic_shape> : protected std::vector<index_t>
{
    using std::vector<index_t>::vector;

    using std::vector<index_t>::begin;
    using std::vector<index_t>::end;
    using std::vector<index_t>::data;
    using std::vector<index_t>::size;
    using std::vector<index_t>::front;
    using std::vector<index_t>::back;
    using std::vector<index_t>::operator[];

    /// @brief Construct a dynamic-rank shape from a fixed-rank one.
    /// @tparam Dims Rank of the source shape (must not be
    ///         @ref dynamic_shape).
    /// @param sh The source shape, copied axis by axis.
    template <index_t Dims>
        requires(Dims != dynamic_shape)
    shape(shape<Dims> sh) : shape(sh.begin(), sh.end())
    {
    }

    /// @brief Returns the number of dimensions at run time.
    size_t dims() const { return size(); }

    /// @brief Product of every axis size; 0 for an empty shape.
    /// @return The total number of elements, or 0 if the shape is empty.
    KFR_MEM_INTRINSIC index_t product() const
    {
        if (std::vector<index_t>::empty())
            return 0;
        index_t p = this->front();
        for (size_t i = 1; i < size(); ++i)
        {
            p *= this->operator[](i);
        }
        return p;
    }

    /// @brief Rotate the axes left by one position.
    ///
    /// For @c [a, b, c, d], returns @c [b, c, d, a]. Shapes of size
    /// 0 or 1 are returned unchanged.
    /// @return A new shape with the axes cyclically shifted left.
    // 0,1,2,3 -> 1,2,3,0
    KFR_MEM_INTRINSIC shape rotate_left() const
    {
        shape result = *this;
        if (result.size() > 1)
            std::rotate(result.begin(), result.begin() + 1, result.end());
        return result;
    }

    /// @brief Rotate the axes right by one position.
    ///
    /// For @c [a, b, c, d], returns @c [d, a, b, c]. Shapes of size
    /// 0 or 1 are returned unchanged.
    /// @return A new shape with the axes cyclically shifted right.
    // 0,1,2,3 -> 3,0,1,2
    KFR_MEM_INTRINSIC shape rotate_right() const
    {
        shape result = *this;
        if (result.size() > 1)
            std::rotate(result.begin(), result.end() - 1, result.end());
        return result;
    }

    /// @brief Drop the last axis.
    /// @return A new shape with one fewer axis (or a copy if empty).
    KFR_MEM_INTRINSIC shape remove_back() const
    {
        shape result = *this;
        if (!result.empty())
            result.erase(result.end() - 1);
        return result;
    }
    /// @brief Drop the first axis.
    /// @return A new shape with one fewer axis (or a copy if empty).
    KFR_MEM_INTRINSIC shape remove_front() const
    {
        shape result = *this;
        if (!result.empty())
        {
            result.erase(result.begin());
        }
        return result;
    }
};

/// @brief Deduction guide for @ref shape from a parameter pack.
///
/// The number of arguments becomes the compile-time rank of the
/// resulting @ref shape.
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

/// @brief Multi-dimensional iteration cursor.
///
/// Stores the current position and the lower/upper bounds of an
/// iteration over a @ref shape. Used by tensor traversal helpers that
/// need to maintain a counter across several nested loops.
/// @tparam Dims Number of dimensions.
template <size_t Dims>
struct cursor
{
    /// @brief Current multi-dimensional index.
    shape<Dims> current;
    /// @brief Lower bound (inclusive) for each axis.
    shape<Dims> minimum;
    /// @brief Upper bound (exclusive) for each axis.
    shape<Dims> maximum;
};

/// @brief Optional signed index used by @ref tensor_range.
///
/// A @c std::nullopt value means "unspecified" for the corresponding
/// start/stop/step component.
using opt_index_t = std::optional<signed_index_t>;

/// @brief Half-open or one-ended range descriptor for tensor slicing.
///
/// A negative value is interpreted relative to the end of the
/// corresponding axis. Used in combination with the
/// @c trange/@c tall/@c tstart/@c tstop/@c tstep factories.
struct tensor_range
{
    /// @brief Start of the range (inclusive). @c std::nullopt means
    /// "from the beginning" (or "to the end" for negative steps).
    opt_index_t start;
    /// @brief End of the range (exclusive). @c std::nullopt means
    /// "to the end" (or "from the beginning" for negative steps).
    opt_index_t stop;
    /// @brief Step between elements. Defaults to 1. May be negative for
    /// a reverse iteration.
    opt_index_t step;
};

/// @brief Build a @ref tensor_range from explicit start/stop/step values.
///
/// Any of the parameters may be @c std::nullopt to leave that
/// component unspecified, in which case tensor subscript logic will
/// pick a sensible default.
/// @param start Optional starting index (inclusive).
/// @param stop  Optional stopping index (exclusive).
/// @param step  Optional step value.
constexpr KFR_INTRINSIC tensor_range trange(std::optional<signed_index_t> start = std::nullopt,
                                            std::optional<signed_index_t> stop  = std::nullopt,
                                            std::optional<signed_index_t> step  = std::nullopt)
{
    return { start, stop, step };
}

/// @brief Build a range that spans the entire axis (defaults for all
/// three components).
constexpr KFR_INTRINSIC tensor_range tall() { return trange(); }
/// @brief Build a range that starts at @p start and continues to the
/// end of the axis.
/// @param start Starting index (inclusive).
/// @param step  Step value (defaults to 1).
constexpr KFR_INTRINSIC tensor_range tstart(signed_index_t start, signed_index_t step = 1)
{
    return trange(start, std::nullopt, step);
}
/// @brief Build a range that stops at @p stop.
/// @param stop Stopping index (exclusive).
/// @param step Step value (defaults to 1).
constexpr KFR_INTRINSIC tensor_range tstop(signed_index_t stop, signed_index_t step = 1)
{
    return trange(std::nullopt, stop, step);
}
/// @brief Build a range that uses the specified step but no explicit
/// start or stop.
/// @param step Step value (defaults to 1).
constexpr KFR_INTRINSIC tensor_range tstep(signed_index_t step = 1)
{
    return trange(std::nullopt, std::nullopt, step);
}

namespace internal_generic
{

constexpr inline index_t null_index = max_index_t;

template <index_t dims, bool fortran_order = false>
constexpr KFR_INTRINSIC shape<dims> strides_for_shape(const shape<dims>& sh, index_t stride = 1)
{
    shape<dims> strides;
    if constexpr (dims > 0)
    {
        index_t n = stride;
        for (index_t i = 0; i < dims; ++i)
        {
            strides[fortran_order ? i : dims - 1 - i] = n;
            n *= sh[fortran_order ? i : dims - 1 - i];
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
        if (KFR_LIKELY(i >= flags.size() || flags[i]))
        {
            result[j++] = in[i];
        }
    }
    return result;
}

template <index_t dims1, index_t dims2, index_t outdims = std::max(dims1, dims2)>
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
            if (KFR_LIKELY(src_size == 1 || src_size == infinite_size || src_size == dst_size ||
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

template <bool checked = false, index_t dims1, index_t dims2, index_t outdims = std::max(dims1, dims2)>
KFR_MEM_INTRINSIC constexpr shape<outdims> common_shape(const shape<dims1>& shape1,
                                                        const shape<dims2>& shape2)
{
    shape<outdims> result;
    for (size_t i = 0; i < outdims; ++i)
    {
        index_t size1 = shape1.revindex(i);
        index_t size2 = shape2.revindex(i);
        if (KFR_UNLIKELY(!size1 || !size2))
        {
            result[outdims - 1 - i] = 0;
            continue;
        }

        if (KFR_UNLIKELY(size1 == infinite_size))
        {
            if (KFR_UNLIKELY(size2 == infinite_size))
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
            if (KFR_UNLIKELY(size2 == infinite_size))
            {
                result[outdims - 1 - i] = size1 == 1 ? infinite_size : size1;
            }
            else
            {
                if (KFR_LIKELY(size1 == 1 || size2 == 1 || size1 == size2))
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
          index_t outdims = std::max({ dims1, dims2, dims... })>
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
        if (KFR_LIKELY(!any(mask)))
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
    KFR_LOOP_UNROLL
    for (int i = static_cast<int>(dim); i >= 0; --i)
    {
        if (KFR_UNLIKELY(indices[i] >= stop[i]))
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
        KFR_LOOP_UNROLL
        for (int i = static_cast<int>(dim); i >= 0;)
        {
            if (KFR_LIKELY(indices[i] < stop[i]))
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
    if (KFR_LIKELY(increment_indices(result, start, stop, dim)))
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

/// @brief Total number of elements described by a shape.
///
/// Equivalent to the product of all axis sizes; returns 1 for a
/// @ref shape<0>.
/// @tparam dims Rank of the shape.
/// @param shape The shape to measure.
/// @return The product of all axis sizes.
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

/// @brief Compile-time axis descriptor used by SIMD tensor accessors.
///
/// Bundles an axis index with a vector width so that expressions like
/// @c get_elements(tensor, idx, axis_params_v<0, 4>) can read 4
/// elements along axis 0 in a single vector load.
/// @tparam Axis Index of the axis.
/// @tparam N    Number of elements to access (vector width).
template <index_t Axis, size_t N>
struct axis_params
{
    /// @brief The axis index.
    constexpr static index_t axis = Axis;
    /// @brief The vector width.
    constexpr static index_t width = N;
    /// @brief Alias of @c width kept for backwards compatibility.
    constexpr static index_t value = N;

    /// @brief Default-construct an axis parameter object.
    constexpr axis_params() = default;
};

/// @brief Inline variable form of @ref axis_params for use in expressions.
///
/// @code
/// using namespace kfr;
/// get_elements(t, idx, axis_params_v<0, 4>);
/// @endcode
/// @tparam Axis Index of the axis.
/// @tparam N    Number of elements to access.
template <index_t Axis, size_t N>
constexpr inline const axis_params<Axis, N> axis_params_v{};

} // namespace kfr

namespace kfr
{
/// @brief String representation of a @ref shape, used by
/// @ref as_string and the formatting framework.
///
/// The format is @c "shape{a, b, c, ...}" for non-empty shapes and
/// @c "shape{}" for @ref shape<0>.
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

} // namespace kfr
