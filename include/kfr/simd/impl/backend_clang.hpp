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

#include "simd.hpp"

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wc99-extensions")

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

namespace intrinsics
{

template <typename TT, size_t NN>
using simd = unwrap_bit<TT> __attribute__((ext_vector_type(NN)));

template <typename T, size_t N1>
KFR_INTRINSIC simd<T, N1> simd_concat(const simd<T, N1>& x);

template <typename T, size_t N1, size_t N2, size_t... Ns, size_t Nscount = csum(csizes<Ns...>)>
KFR_INTRINSIC simd<T, N1 + N2 + Nscount> simd_concat(const simd<T, N1>& x, const simd<T, N2>& y,
                                                     const simd<T, Ns>&... z);

template <typename Tout>
KFR_INTRINSIC void simd_make(ctype_t<Tout>) = delete;

template <typename Tout, typename Arg>
KFR_INTRINSIC simd<Tout, 1> simd_make(ctype_t<Tout>, const Arg& arg)
{
    return (simd<Tout, 1>){ unwrap_bit_value(arg) };
}

template <typename Tout, typename... Args, size_t N = sizeof...(Args), KFR_ENABLE_IF(N > 1)>
KFR_INTRINSIC simd<Tout, N> simd_make(ctype_t<Tout>, const Args&... args)
{
    return (simd<Tout, N>){ unwrap_bit_value(args)... };
}

// @brief Returns vector with undefined value
template <typename Tout, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_undefined()
{
    simd<Tout, N> x;
    return x;
}

// @brief Returns vector with all zeros
template <typename Tout, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_zeros()
{
    return Tout();
}

// @brief Returns vector with all ones
template <typename Tout, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_allones()
{
    return unwrap_bit_value(special_constants<Tout>::allones());
}

// @brief Converts input vector to vector with subtype Tout
template <typename Tout, typename Tin, size_t N, size_t Nout = (sizeof(Tin) * N / sizeof(Tout))>
KFR_INTRINSIC simd<Tout, Nout> simd_bitcast(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x)
{
    return (simd<Tout, Nout>)x;
}

template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> simd_bitcast(simd_cvt_t<T, T, N>, const simd<T, N>& x)
{
    return x;
}

template <typename T, size_t N, size_t index>
KFR_INTRINSIC T simd_get_element(const simd<T, N>& value, csize_t<index>)
{
    return wrap_bit_value<T>(value[index]);
}

template <typename T, size_t N, size_t index>
KFR_INTRINSIC simd<T, N> simd_set_element(simd<T, N> value, csize_t<index>, T x)
{
    value[index] = unwrap_bit_value(x);
    return value;
}

template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> simd_broadcast(simd_t<T, N>, identity<T> value)
{
    return unwrap_bit_value(value);
}

template <typename T, size_t N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd_t<T, N>, const simd<T, N>& x, csizes_t<indices...>,
                                         overload_generic)
{
    return __builtin_shufflevector(x, x, (indices > N ? -1 : static_cast<int>(indices))...);
}

template <typename T, size_t N, size_t N2 = N, size_t... indices, size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd2_t<T, N, N>, const simd<T, N>& x, const simd<T, N>& y,
                                         csizes_t<indices...>, overload_generic)
{
    static_assert(N == N2, "");
    return __builtin_shufflevector(x, y, (indices > 2 * N ? -1 : static_cast<int>(indices))...);
}

template <typename T, size_t N1, size_t N2, size_t... indices, KFR_ENABLE_IF(N1 != N2),
          size_t Nout = sizeof...(indices)>
KFR_INTRINSIC simd<T, Nout> simd_shuffle(simd2_t<T, N1, N2>, const simd<T, N1>& x, const simd<T, N2>& y,
                                         csizes_t<indices...>, overload_generic)
{
    constexpr size_t Nmax = (N1 > N2 ? N1 : N2);
    return simd_shuffle(simd2_t<T, Nmax, Nmax>{},
                        simd_shuffle(simd_t<T, N1>{}, x, csizeseq<Nmax>, overload_auto),
                        simd_shuffle(simd_t<T, N2>{}, y, csizeseq<Nmax>, overload_auto),
                        csizes<(indices < N1        ? indices
                                : indices < N1 + N2 ? indices + (Nmax - N1)
                                                    : index_undefined)...>,
                        overload_auto);
}

template <typename T, size_t N1>
KFR_INTRINSIC simd<T, N1> simd_concat(const simd<T, N1>& x)
{
    return x;
}

template <typename T, size_t N1, size_t N2, size_t... Ns, size_t Nscount /*= csum(csizes<Ns...>)*/>
KFR_INTRINSIC simd<T, N1 + N2 + Nscount> simd_concat(const simd<T, N1>& x, const simd<T, N2>& y,
                                                     const simd<T, Ns>&... z)
{
    return simd_shuffle(simd2_t<T, N1, N2 + Nscount>{}, x, simd_concat<T, N2, Ns...>(y, z...),
                        csizeseq<N1 + N2 + Nscount>, overload_auto);
}

// @brief Converts input vector to vector with subtype Tout
template <typename Tout, typename Tin, size_t N>
KFR_INTRINSIC simd<Tout, N> simd_convert(simd_cvt_t<Tout, Tin, N>, const simd<Tin, N>& x)
{
    return __builtin_convertvector(x, simd<Tout, N>);
}

// @brief Converts input vector to vector with subtype Tout
template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> simd_convert(simd_cvt_t<T, T, N>, const simd<T, N>& x)
{
    return x;
}

template <typename T, size_t N, bool A>
using simd_storage = struct_with_alignment<simd<T, N>, A>;

template <typename T, size_t N>
KFR_INTRINSIC T simd_get_element(const simd<T, N>& value, size_t index)
{
    return wrap_bit_value<T>(value[index]);
}

template <typename T, size_t N>
KFR_INTRINSIC simd<T, N> simd_set_element(simd<T, N> value, size_t index, T x)
{
    value[index] = unwrap_bit_value(x);
    return value;
}
} // namespace intrinsics
} // namespace CMT_ARCH_NAME

} // namespace kfr

CMT_PRAGMA_GNU(GCC diagnostic pop)
