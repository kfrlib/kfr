/** @addtogroup read_write
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

#include "impl/read_write.hpp"
#include <array>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <size_t N, bool A = false, typename T>
KFR_INTRINSIC vec<T, N> read(const T* src)
{
    return vec<T, N>::from_flatten(intrinsics::read(cbool<A>, csize<N * compound_type_traits<T>::deep_width>,
                                                    ptr_cast<deep_subtype<T>>(src)));
}

template <bool A = false, size_t N, typename T>
KFR_INTRINSIC void write(T* dest, const vec<T, N>& value)
{
    intrinsics::write(cbool<A>, ptr_cast<deep_subtype<T>>(dest), value.flatten());
}

namespace internal
{
template <size_t group, size_t count, size_t N, bool A, typename T, size_t... indices>
KFR_INTRINSIC vec<T, group * count * N> read_group_impl(const T* src, size_t stride, csizes_t<indices...>)
{
    return concat(intrinsics::read(cbool<A>, csize<N * group>, src + group * stride * indices)...);
}
template <size_t group, size_t count, size_t N, bool A, typename T, size_t... indices>
KFR_INTRINSIC void write_group_impl(T* dest, size_t stride, const vec<T, group * count * N>& value,
                                    csizes_t<indices...>)
{
    swallow{ (write<A>(dest + group * stride * indices, slice<group * indices * N, group * N>(value)),
              0)... };
}
} // namespace internal

template <size_t count, size_t N, size_t group = 1, bool A = false, typename T>
KFR_INTRINSIC vec<T, group * count * N> read_group(const T* src, size_t stride)
{
    return internal::read_group_impl<group, count, N, A>(ptr_cast<T>(src), stride, csizeseq_t<count>());
}

template <size_t count, size_t N, size_t group = 1, bool A = false, typename T>
KFR_INTRINSIC void write_group(T* dest, size_t stride, const vec<T, group * count * N>& value)
{
    return internal::write_group_impl<group, count, N, A>(dest, stride, value, csizeseq_t<count>());
}

template <typename... Indices, typename T, size_t Nout = 1 + sizeof...(Indices)>
KFR_INTRINSIC vec<T, Nout> gather(const T* base, size_t index, Indices... indices)
{
    return make_vector(base[index], base[indices]...);
}

template <size_t Index, size_t... Indices, typename T, size_t Nout = 1 + sizeof...(Indices)>
KFR_INTRINSIC vec<T, Nout> gather(const T* base)
{
    return make_vector(base[Index], base[Indices]...);
}

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
    return concat(read<groupsize>(base + Indices * groupsize * stride)...);
}
} // namespace internal

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> gather(const T* base, const vec<u32, N>& indices)
{
    return internal::gather(base, indices, csizeseq<N>);
}

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
    return concat(read<groupsize>(base + groupsize * offset[Indices])...);
}
} // namespace internal
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

template <size_t groupsize = 1, typename T, size_t N, size_t Nout = N * groupsize, typename IT>
KFR_INTRINSIC void scatter(T* base, const vec<IT, N>& offset, const vec<T, Nout>& value)
{
    return internal::scatter_helper<groupsize>(base, offset, value, csizeseq<N>);
}

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

template <typename T, size_t groupsize = 1>
struct stride_pointer : public stride_pointer<const T, groupsize>
{
    template <size_t N>
    void write(const vec<T, N>& val, csize_t<N> = csize_t<N>())
    {
        kfr::scatter_stride<N, groupsize>(this->ptr, val);
    }
};

template <typename T, size_t groupsize>
struct stride_pointer<const T, groupsize>
{
    const T* ptr;
    const size_t stride;

    template <size_t N>
    vec<T, N> read(csize_t<N> = csize_t<N>())
    {
        return kfr::gather_stride<N, groupsize>(ptr, stride);
    }
};

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> to_vec(const std::array<T, N>& a)
{
    return read<N>(a.data());
}

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

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> partial_mask(size_t index)
{
    static_assert(N <= arraysize(partial_masks<T>) / 2,
                  "N must not be greater than half of partial_masks array");
    return read<N>(&partial_masks<T>[0] + arraysize(partial_masks<T>) / 2 - index);
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> partial_mask(size_t index, vec_shape<T, N>)
{
    return partial_mask<T, N>(index);
}

// read/write
template <typename T, size_t N>
template <bool aligned>
KFR_MEM_INTRINSIC constexpr vec<T, N>::vec(const value_type* src, cbool_t<aligned>) CMT_NOEXCEPT
    : vec(vec<T, N>::from_flatten(intrinsics::read(cbool<aligned>,
                                                   csize<N * compound_type_traits<T>::deep_width>,
                                                   ptr_cast<deep_subtype<T>>(src))))
{
}

template <typename T, size_t N>
template <bool aligned>
KFR_MEM_INTRINSIC const vec<T, N>& vec<T, N>::write(value_type* dest, cbool_t<aligned>) const CMT_NOEXCEPT
{
    intrinsics::write(cbool<aligned>, ptr_cast<deep_subtype<T>>(dest), flatten());
    return *this;
}

} // namespace CMT_ARCH_NAME
} // namespace kfr
