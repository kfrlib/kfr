/** @addtogroup types
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

#include "shuffle.hpp"
#include "types.hpp"
#include "vec.hpp"

namespace kfr
{

template <size_t N, bool A = false, typename T>
CMT_INLINE vec<T, N> read(const T* src)
{
    return internal_read_write::read<N, A, T>(src);
}

template <bool A = false, size_t N, typename T>
CMT_INLINE void write(T* dest, const vec<T, N>& value)
{
    internal_read_write::write<A, N, T>(dest, value);
}

template <typename... Indices, typename T, size_t Nout = 1 + sizeof...(Indices)>
CMT_INLINE vec<T, Nout> gather(const T* base, size_t index, Indices... indices)
{
    return make_vector(base[index], base[indices]...);
}

template <size_t Index, size_t... Indices, typename T, size_t Nout = 1 + sizeof...(Indices)>
CMT_INLINE vec<T, Nout> gather(const T* base)
{
    return make_vector(base[Index], base[Indices]...);
}

template <size_t Index, size_t... Indices, typename T, size_t N, size_t InIndex = 0>
CMT_INLINE void scatter(const T* base, const vec<T, N>& value)
{
    base[Index] = value[InIndex];
    scatter<Indices..., T, N, InIndex + 1>(base, value);
}

namespace internal
{
template <typename T, size_t N, size_t... Indices>
CMT_INLINE vec<T, N> gather(const T* base, const vec<u32, N>& indices, csizes_t<Indices...>)
{
    return make_vector(base[indices[Indices]]...);
}
template <size_t Nout, size_t Stride, typename T, size_t... Indices>
CMT_INLINE vec<T, Nout> gather_stride(const T* base, csizes_t<Indices...>)
{
    return make_vector(base[Indices * Stride]...);
}
template <size_t Nout, typename T, size_t... Indices>
CMT_INLINE vec<T, Nout> gather_stride_s(const T* base, size_t stride, csizes_t<Indices...>)
{
    return make_vector(base[Indices * stride]...);
}
}

template <typename T, size_t N>
CMT_INLINE vec<T, N> gather(const T* base, const vec<u32, N>& indices)
{
    return internal::gather(base, indices, csizeseq<N>);
}

template <size_t Nout, typename T>
CMT_INLINE vec<T, Nout> gather_stride(const T* base, size_t stride)
{
    return internal::gather_stride_s<Nout>(base, stride, csizeseq<Nout>);
}

template <size_t Nout, size_t Stride, typename T>
CMT_INLINE vec<T, Nout> gather_stride(const T* base)
{
    return internal::gather_stride<Nout, Stride>(base, csizeseq<Nout>);
}

template <size_t groupsize, typename T, size_t N, typename IT, size_t... Indices>
CMT_INLINE vec<T, N * groupsize> gather_helper(const T* base, const vec<IT, N>& offset, csizes_t<Indices...>)
{
    return concat(read<groupsize>(base + groupsize * (*offset)[Indices])...);
}
template <size_t groupsize = 1, typename T, size_t N, typename IT>
CMT_INLINE vec<T, N * groupsize> gather(const T* base, const vec<IT, N>& offset)
{
    return gather_helper<groupsize>(base, offset, csizeseq<N>);
}

template <size_t groupsize, typename T, size_t N, size_t Nout = N* groupsize, typename IT, size_t... Indices>
CMT_INLINE void scatter_helper(T* base, const vec<IT, N>& offset, const vec<T, Nout>& value,
                               csizes_t<Indices...>)
{
    swallow{ (write(base + groupsize * (*offset)[Indices], slice<Indices * groupsize, groupsize>(value)),
              0)... };
}
template <size_t groupsize = 1, typename T, size_t N, size_t Nout = N* groupsize, typename IT>
CMT_INLINE void scatter(T* base, const vec<IT, N>& offset, const vec<T, Nout>& value)
{
    return scatter_helper<groupsize>(base, offset, value, csizeseq<N>);
}

template <typename T>
constexpr T partial_masks[] = { internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
                                internal::allones<T>,
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

template <typename T, size_t N>
CMT_INLINE vec<T, N> partial_mask(size_t index)
{
    static_assert(N <= arraysize(partial_masks<T>) / 2,
                  "N must not be greater than half of partial_masks expression_array");
    return read<N>(&partial_masks<T>[0] + arraysize(partial_masks<T>) / 2 - index);
}
template <typename T, size_t N>
CMT_INLINE vec<T, N> partial_mask(size_t index, vec_t<T, N>)
{
    return partial_mask<T, N>(index);
}
}
