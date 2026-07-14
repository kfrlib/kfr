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

#include "impl/read_write.hpp"
#include <array>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{

/**
 * @brief Reads a vector of @c N elements of type @c T from memory.
 * @tparam N Number of elements to read.
 * @tparam A When @c true, requires (and may exploit) an aligned source pointer.
 * @tparam T Element type.
 * @param src Pointer to the first element to read.
 * @return A @ref vec holding the loaded elements.
 */
template <size_t N, bool A = false, typename T>
KFR_INTRINSIC vec<T, N> read(const T* src)
{
    return vec<T, N>::from_flatten(
        intr::read(cbool<A>, csize<N * compound_type_traits<T>::deep_width>, ptr_cast<deep_subtype<T>>(src)));
}

/**
 * @brief Writes a vector of @c N elements of type @c T to memory.
 * @tparam A When @c true, requires (and may exploit) an aligned destination pointer.
 * @tparam N Number of elements to write.
 * @tparam T Element type.
 * @param dest Pointer to the destination storage.
 * @param value Vector whose elements are stored to @p dest.
 */
template <bool A = false, size_t N, typename T>
KFR_INTRINSIC void write(T* dest, const vec<T, N>& value)
{
    intr::write(cbool<A>, ptr_cast<deep_subtype<T>>(dest), value.flatten());
}

namespace internal
{
template <typename T, size_t N, size_t count, size_t... indices>
KFR_INTRINSIC vec<T, N * count> concat_read_chunks(const vec<T, N> (&chunks)[count], csizes_t<indices...>)
{
    static_assert(count == sizeof...(indices));
    return concat(chunks[indices]...);
}

template <size_t group, size_t count, size_t N, bool A, typename T, size_t... indices>
KFR_INTRINSIC vec<T, group * count * N> read_group_impl(const T* src, size_t stride, csizes_t<indices...>)
{
    const vec<T, group * N> chunks[] = { intr::read(cbool<A>, csize<N * group>,
                                                    src + group * stride * indices)... };
    return concat_read_chunks(chunks, csizes_t<indices...>());
}
template <size_t group, size_t count, size_t N, bool A, typename T, size_t... indices>
KFR_INTRINSIC void write_group_impl(T* dest, size_t stride, const vec<T, group * count * N>& value,
                                    csizes_t<indices...>)
{
    swallow{ (write<A>(dest + group * stride * indices, slice<group * indices * N, group * N>(value)),
              0)... };
}
} // namespace internal

/**
 * @brief Reads @c count groups of @c group*N elements separated by @p stride groups.
 *
 * @p stride is measured in @b groups (units of @c group elements),
 * so @c stride == 1 always selects contiguous access regardless of @c group.
 *
 * Example:
 * @code
 * int arr[] = {0, 1, 2, 3, 4, 5, 6, 7};
 * read_group<2, 1, 2>(arr, 2) == vec{0, 1, 4, 5}; // 2 groups of 2, stride 2 groups
 * @endcode
 *
 * @tparam count Number of groups to read.
 * @tparam N Base number of elements per group.
 * @tparam group Number of contiguous elements in each chunk.
 * @tparam A When @c true, requires aligned source pointers.
 * @tparam T Element type.
 * @param src Pointer to the first element.
 * @param stride Offset (in groups, i.e. units of @c group elements) between consecutive groups.
 * @return A vector of @c group*count*N elements assembled from the groups.
 */
template <size_t count, size_t N, size_t group = 1, bool A = false, typename T>
KFR_INTRINSIC vec<T, group * count * N> read_group(const T* src, size_t stride)
{
    return internal::read_group_impl<group, count, N, A>(ptr_cast<T>(src), stride, csizeseq_t<count>());
}

/**
 * @brief Writes @c count groups of @c group*N elements separated by @p stride groups.
 *
 * @p stride is measured in @b groups (units of @c group elements),
 * so @c stride == 1 always selects contiguous access regardless of @c group.
 *
 * @tparam count Number of groups to write.
 * @tparam N Base number of elements per group.
 * @tparam group Number of contiguous elements in each chunk.
 * @tparam A When @c true, requires aligned destination pointers.
 * @tparam T Element type.
 * @param dest Pointer to the first destination element.
 * @param stride Offset (in groups, i.e. units of @c group elements) between consecutive groups.
 * @param value Vector whose elements are scattered into the groups.
 */
template <size_t count, size_t N, size_t group = 1, bool A = false, typename T>
KFR_INTRINSIC void write_group(T* dest, size_t stride, const vec<T, group * count * N>& value)
{
    return internal::write_group_impl<group, count, N, A>(dest, stride, value, csizeseq_t<count>());
}

/**
 * @brief Gathers elements from @p base at the given runtime indices.
 * @tparam Indices Types of the additional index arguments (must be convertible to @c size_t).
 * @tparam T Element type.
 * @tparam Nout Output vector length (deduced as @c 1 + sizeof...(Indices)).
 * @param base Pointer to the source array.
 * @param index Index of the first element to gather.
 * @param indices Indices of the remaining elements to gather.
 * @return A vector holding @c base[index], @c base[indices]... in order.
 */
template <typename... Indices, typename T, size_t Nout = 1 + sizeof...(Indices)>
KFR_INTRINSIC vec<T, Nout> gather(const T* base, size_t index, Indices... indices)
{
    return make_vector(base[index], base[indices]...);
}

/**
 * @brief Gathers elements from @p base at the given compile-time indices.
 * @tparam Index First compile-time index.
 * @tparam Indices Remaining compile-time indices.
 * @tparam T Element type.
 * @tparam Nout Output vector length (deduced as @c 1 + sizeof...(Indices)).
 * @param base Pointer to the source array.
 * @return A vector holding @c base[Index], @c base[Indices]... in order.
 */
template <size_t Index, size_t... Indices, typename T, size_t Nout = 1 + sizeof...(Indices)>
KFR_INTRINSIC vec<T, Nout> gather(const T* base)
{
    return make_vector(base[Index], base[Indices]...);
}

/**
 * @brief Scatters elements of @p value to @p base at the given compile-time indices.
 * @tparam Index First compile-time destination index.
 * @tparam Indices Remaining compile-time destination indices.
 * @tparam T Element type.
 * @tparam N Input vector length.
 * @tparam InIndex Position within @p value to write next (defaults to 0).
 * @param base Pointer to the destination array.
 * @param value Vector whose elements are written to @c base[Index], @c base[Indices]... in order.
 */
template <size_t Index, size_t... Indices, typename T, size_t N, size_t InIndex = 0>
KFR_INTRINSIC void scatter(const T* base, const vec<T, N>& value)
{
    base[Index] = value[InIndex];
    scatter<Indices..., T, N, InIndex + 1>(base, value);
}

namespace internal
{
template <typename T, size_t N, size_t... Indices>
KFR_INTRINSIC vec<T, N> gather(const T* base, const vec<u32, N>& indices, csizes_t<Indices...>)
{
    return make_vector(base[indices[Indices]]...);
}
template <size_t Nout, size_t Stride, typename T, size_t... Indices>
KFR_INTRINSIC vec<T, Nout> gather_stride(const T* base, csizes_t<Indices...>)
{
    return make_vector(base[Indices * Stride]...);
}
template <size_t Nout, size_t groupsize, typename T, size_t... Indices>
KFR_INTRINSIC vec<T, Nout> gather_stride_s(const T* base, size_t stride, csizes_t<Indices...>)
{
    const vec<T, groupsize> chunks[] = { read<groupsize>(base + Indices * groupsize * stride)... };
    return concat_read_chunks(chunks, csizes_t<Indices...>());
}
} // namespace internal

/**
 * @brief Gathers elements from @p base using a vector of indices.
 * @tparam T Element type.
 * @tparam N Number of indices (and output length).
 * @param base Pointer to the source array.
 * @param indices Vector of @c u32 indices into @p base.
 * @return A vector holding @c base[indices[0]], @c base[indices[1]], ... in order.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> gather(const T* base, const vec<u32, N>& indices)
{
    return internal::gather(base, indices, csizeseq<N>);
}

/**
 * @brief Gathers @c Nout groups of @c groupsize contiguous elements from @p base separated by @p stride.
 * @tparam Nout Number of groups to gather.
 * @tparam groupsize Number of contiguous elements in each group.
 * @tparam T Element type.
 * @param base Pointer to the source array.
 * @param stride Offset (in groups) between consecutive groups.
 * @return A vector of @c Nout*groupsize elements assembled from the gathered groups.
 */
template <size_t Nout, size_t groupsize = 1, typename T>
KFR_INTRINSIC vec<T, Nout * groupsize> gather_stride(const T* base, size_t stride)
{
    if constexpr (Nout > 2)
    {
        constexpr size_t Nlow = prev_poweroftwo(Nout - 1);
        return concat(internal::gather_stride_s<Nlow, groupsize>(base, stride, csizeseq<Nlow>),
                      internal::gather_stride_s<Nout - Nlow, groupsize>(base + Nlow * stride, stride,
                                                                        csizeseq<Nout - Nlow>));
    }
    else
        return internal::gather_stride_s<Nout, groupsize>(base, stride, csizeseq<Nout>);
}

/**
 * @brief Gathers @c Nout elements from @p base at compile-time stride @p Stride.
 * @tparam Nout Number of elements to gather.
 * @tparam Stride Fixed offset (in elements) between consecutive gathered elements.
 * @tparam T Element type.
 * @param base Pointer to the source array.
 * @return A vector holding @c base[0], @c base[Stride], @c base[2*Stride], ... in order.
 */
template <size_t Nout, size_t Stride, typename T>
KFR_INTRINSIC vec<T, Nout> gather_stride(const T* base)
{
    return internal::gather_stride<Nout, Stride>(base, csizeseq<Nout>);
}

namespace internal
{
template <size_t groupsize, typename T, size_t N, typename IT, size_t... Indices>
KFR_INTRINSIC vec<T, N * groupsize> gather_helper(const T* base, const vec<IT, N>& offset,
                                                  csizes_t<Indices...>)
{
    const vec<T, groupsize> chunks[] = { read<groupsize>(base + groupsize * offset[Indices])... };
    return concat_read_chunks(chunks, csizes_t<Indices...>());
}
} // namespace internal
/**
 * @brief Gathers @c N groups of @c groupsize contiguous elements from @p base using a vector of offsets.
 * @tparam groupsize Number of contiguous elements per group.
 * @tparam T Element type.
 * @tparam N Number of offsets (and groups).
 * @tparam IT Offset integer type.
 * @param base Pointer to the source array.
 * @param offset Vector of offsets (in groups) into @p base.
 * @return A vector of @c N*groupsize elements assembled from the gathered groups.
 */
template <size_t groupsize = 1, typename T, size_t N, typename IT>
KFR_INTRINSIC vec<T, N * groupsize> gather(const T* base, const vec<IT, N>& offset)
{
    return internal::gather_helper<groupsize>(base, offset, csizeseq<N>);
}

namespace internal
{
template <size_t groupsize, typename T, size_t N, size_t Nout = N * groupsize, typename IT, size_t... Indices>
KFR_INTRINSIC void scatter_helper(T* base, const vec<IT, N>& offset, const vec<T, Nout>& value,
                                  csizes_t<Indices...>)
{
    swallow{ (write(base + groupsize * offset[Indices], slice<Indices * groupsize, groupsize>(value)),
              0)... };
}
template <size_t groupsize, typename T, size_t N, size_t... Indices>
KFR_INTRINSIC void scatter_helper_s(T* base, size_t stride, const vec<T, N>& value, csizes_t<Indices...>)
{
    swallow{ (write(base + groupsize * Indices * stride, slice<Indices * groupsize, groupsize>(value)),
              0)... };
}
} // namespace internal

/**
 * @brief Scatters @c N groups of @c groupsize contiguous elements of @p value to @p base using a vector of
 * offsets.
 * @tparam groupsize Number of contiguous elements per group.
 * @tparam T Element type.
 * @tparam N Number of offsets (and groups).
 * @tparam Nout Total number of elements to write (defaults to @c N*groupsize).
 * @tparam IT Offset integer type.
 * @param base Pointer to the destination array.
 * @param offset Vector of offsets (in groups) into @p base.
 * @param value Vector whose elements are scattered to the destination groups.
 */
template <size_t groupsize = 1, typename T, size_t N, size_t Nout = N * groupsize, typename IT>
KFR_INTRINSIC void scatter(T* base, const vec<IT, N>& offset, const vec<T, Nout>& value)
{
    return internal::scatter_helper<groupsize>(base, offset, value, csizeseq<N>);
}

/**
 * @brief Scatters @c N/groupsize groups of @c groupsize contiguous elements of @p value to @p base separated
 * by @p stride.
 * @tparam groupsize Number of contiguous elements per group.
 * @tparam T Element type.
 * @tparam N Total number of elements in @p value (must be a multiple of @c groupsize).
 * @param base Pointer to the destination array.
 * @param value Vector whose elements are scattered to the destination groups.
 * @param stride Offset (in groups) between consecutive destination groups.
 */
template <size_t groupsize = 1, typename T, size_t N>
KFR_INTRINSIC void scatter_stride(T* base, const vec<T, N>& value, size_t stride)
{
    constexpr size_t Nout = N / groupsize;
    if constexpr (Nout > 2)
    {
        constexpr size_t Nlow = prev_poweroftwo(Nout - 1);
        internal::scatter_helper_s<groupsize>(base, stride, slice<0, Nlow>(value), csizeseq<Nlow>);
        internal::scatter_helper_s<groupsize>(base + Nlow * stride, stride, slice<Nlow, Nout - Nlow>(value),
                                              csizeseq<(Nout - Nlow)>);
    }
    else
        return internal::scatter_helper_s<groupsize>(base, stride, value, csizeseq<Nout>);
}

/**
 * @brief Mutable strided pointer: a pointer paired with a stride that supports grouped scatter writes.
 * @tparam T Element type.
 * @tparam groupsize Number of contiguous elements per group.
 */
template <typename T, size_t groupsize = 1>
struct stride_pointer : public stride_pointer<const T, groupsize>
{
    /**
     * @brief Scatters @c N elements of @p val to the underlying pointer using the stored stride.
     * @tparam N Number of elements to write.
     * @param val Vector whose elements are scattered.
     */
    template <size_t N>
    void write(const vec<T, N>& val, csize_t<N> = csize_t<N>())
    {
        kfr::scatter_stride<N, groupsize>(this->ptr, val);
    }
};

/**
 * @brief Read-only strided pointer: a pointer paired with a stride that supports grouped gather reads.
 * @tparam T Element type.
 * @tparam groupsize Number of contiguous elements per group.
 */
template <typename T, size_t groupsize>
struct stride_pointer<const T, groupsize>
{
    const T* ptr; ///< Pointer to the first element.
    const size_t stride; ///< Offset (in groups) between consecutive groups.

    /**
     * @brief Gathers @c N elements from the underlying pointer using the stored stride.
     * @tparam N Number of elements to read.
     * @return A vector holding the gathered elements.
     */
    template <size_t N>
    vec<T, N> read(csize_t<N> = csize_t<N>())
    {
        return kfr::gather_stride<N, groupsize>(ptr, stride);
    }
};

/**
 * @brief Converts a @c std::array of @c N elements to a vector.
 * @tparam T Element type.
 * @tparam N Number of elements.
 * @param a The source array.
 * @return A vector holding the elements of @p a.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> to_vec(const std::array<T, N>& a)
{
    return read<N>(a.data());
}

/**
 * @brief Lookup table used by @ref partial_mask to build vectors with a leading run of all-ones elements.
 *
 * The first half of the array holds all-ones values and the second half holds zero values, so that a
 * contiguous slice of length @c N starting at <tt>size/2 - index</tt> yields a vector with @c index
 * leading all-ones elements followed by zeros.
 */
template <typename T>
constexpr T partial_masks[] = { special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                special_constants<T>::allones(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T(),
                                T() };

/**
 * @brief Returns a vector of @c N elements with @p index leading all-ones elements followed by zeros.
 * @tparam T Element type.
 * @tparam N Output vector length (must not exceed half of @ref partial_masks<T>).
 * @param index Number of leading all-ones elements (0 to @c N).
 * @return A vector acting as a mask selecting the first @p index lanes.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> partial_mask(size_t index)
{
    static_assert(N <= std::size(partial_masks<T>) / 2,
                  "N must not be greater than half of partial_masks array");
    return read<N>(&partial_masks<T>[0] + std::size(partial_masks<T>) / 2 - index);
}

/**
 * @brief Returns a vector of @c N elements with @p index leading all-ones elements followed by zeros.
 * @tparam T Element type.
 * @tparam N Output vector length.
 * @param index Number of leading all-ones elements (0 to @c N).
 * @param shape Tag value carrying the element type and length.
 * @return A vector acting as a mask selecting the first @p index lanes.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> partial_mask(size_t index, vec_shape<T, N>)
{
    return partial_mask<T, N>(index);
}

// read/write
/**
 * @brief Constructs a vector by loading @c N elements from @p src.
 * @tparam T Element type.
 * @tparam N Vector length.
 * @tparam aligned When @c true, requires (and may exploit) an aligned source pointer.
 * @param src Pointer to the first element to load.
 */
template <typename T, size_t N>
template <bool aligned>
KFR_MEM_INTRINSIC constexpr vec<T, N>::vec(const value_type* src, cbool_t<aligned>) noexcept
    : vec(vec<T, N>::from_flatten(intr::read(cbool<aligned>, csize<N * compound_type_traits<T>::deep_width>,
                                             ptr_cast<deep_subtype<T>>(src))))
{
}

/**
 * @brief Stores the elements of this vector to @p dest.
 * @tparam T Element type.
 * @tparam N Vector length.
 * @tparam aligned When @c true, requires (and may exploit) an aligned destination pointer.
 * @param dest Pointer to the destination storage.
 * @return A reference to @c *this.
 */
template <typename T, size_t N>
template <bool aligned>
KFR_MEM_INTRINSIC const vec<T, N>& vec<T, N>::write(value_type* dest, cbool_t<aligned>) const noexcept
{
    intr::write(cbool<aligned>, ptr_cast<deep_subtype<T>>(dest), flatten());
    return *this;
}

} // namespace KFR_ARCH_NAME
} // namespace kfr
