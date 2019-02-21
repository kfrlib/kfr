/** @addtogroup logical
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

#include "vec.hpp"

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

template <typename T>
using maskfor = typename T::mask_t;

namespace internal
{

template <typename T>
constexpr inline T maskbits(bool value)
{
    return value ? special_constants<T>::allones() : special_constants<T>::allzeros();
}
} // namespace internal

template <typename T, size_t N>
struct mask : protected vec<T, N>
{
    using base = vec<T, N>;

    KFR_MEM_INTRINSIC mask() CMT_NOEXCEPT = default;

    KFR_MEM_INTRINSIC mask(const mask&) CMT_NOEXCEPT = default;

    KFR_MEM_INTRINSIC mask& operator=(const mask&) CMT_NOEXCEPT = default;

    using simd_type = typename base::simd_type;

    KFR_MEM_INTRINSIC mask(bool arg) : base(internal::maskbits<T>(arg)) {}

    template <typename... Args>
    KFR_MEM_INTRINSIC mask(bool arg1, bool arg2, Args... args)
        : base(internal::maskbits<T>(arg1), internal::maskbits<T>(arg2),
               internal::maskbits<T>(static_cast<bool>(args))...)
    {
    }

    using vec<T, N>::v;

    KFR_MEM_INTRINSIC mask(const base& v) CMT_NOEXCEPT;

    KFR_MEM_INTRINSIC mask(const simd_type& simd) : base(simd) {}

    template <typename U, KFR_ENABLE_IF(sizeof(T) == sizeof(U))>
    KFR_MEM_INTRINSIC mask(const mask<U, N>& m) : base(base::frombits(m.asvec()))
    {
    }

    template <typename U, KFR_ENABLE_IF(sizeof(T) != sizeof(U))>
    KFR_MEM_INTRINSIC mask(const mask<U, N>& m)
        : base(base::frombits(innercast<itype<T>>(vec<itype<U>, N>::frombits(m.asvec()))))
    {
    }

    KFR_MEM_INTRINSIC bool operator[](size_t index) const CMT_NOEXCEPT;

    KFR_MEM_INTRINSIC constexpr base asvec() const CMT_NOEXCEPT { return base(v); }
};

namespace internal
{

template <typename T, size_t Nout, size_t N1, size_t... indices>
constexpr vec<T, Nout> partial_mask_helper(csizes_t<indices...>)
{
    return make_vector(maskbits<T>(indices < N1)...);
}

template <typename T, size_t Nout, size_t N1>
constexpr vec<T, Nout> partial_mask()
{
    return internal::partial_mask_helper<T, Nout, N1>(csizeseq_t<Nout>());
}
} // namespace internal

template <typename T, size_t N>
KFR_MEM_INTRINSIC bool mask<T, N>::operator[](size_t index) const CMT_NOEXCEPT
{
    return ibitcast(base::operator[](index)) < 0;
}

template <typename T, typename... Args, size_t Nout = (sizeof...(Args) + 1)>
constexpr KFR_INTRINSIC mask<T, Nout> make_mask(bool arg, Args... args)
{
    return vec<T, Nout>(internal::maskbits<T>(arg), internal::maskbits<T>(static_cast<bool>(args))...);
}

} // namespace CMT_ARCH_NAME
} // namespace kfr

namespace cometa
{

template <typename T, size_t N>
struct compound_type_traits<kfr::mask<T, N>>
{
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static size_t width      = N;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;
    template <typename U>
    using rebind = kfr::mask<U, N>;
    template <typename U>
    using deep_rebind = kfr::mask<typename compound_type_traits<subtype>::template deep_rebind<U>, N>;

    KFR_MEM_INTRINSIC static constexpr subtype at(const kfr::mask<T, N>& value, size_t index)
    {
        return value[index];
    }
};
} // namespace cometa

namespace std
{
template <typename T1, typename T2, size_t N>
struct common_type<kfr::mask<T1, N>, kfr::mask<T2, N>>
{
    using type = kfr::mask<typename common_type<T1, T2>::type, N>;
};
} // namespace std
