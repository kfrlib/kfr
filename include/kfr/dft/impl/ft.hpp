/** @addtogroup dft
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

#include "../../base/small_buffer.hpp"
#include "../../base/univector.hpp"
#include "../../math/sin_cos.hpp"
#include "../../simd/complex.hpp"
#include "../../simd/constants.hpp"
#include "../../simd/digitreverse.hpp"
#include "../../simd/read_write.hpp"
#include "../../simd/vec.hpp"

#include "../../base/memory.hpp"
#include "../data/sincos.hpp"


CMT_PRAGMA_GNU(GCC diagnostic push)
#if CMT_HAS_WARNING("-Wpass-failed")
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wpass-failed")
#endif

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 4127))

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template <typename T, size_t N>
using cvec = vec<T, N * 2>;

namespace intrinsics
{

template <typename T, size_t N, KFR_ENABLE_IF(N >= 2)>
KFR_INTRINSIC vec<T, N> cmul_impl(const vec<T, N>& x, const vec<T, N>& y)
{
    return subadd(x * dupeven(y), swap<2>(x) * dupodd(y));
}
template <typename T, size_t N, KFR_ENABLE_IF(N > 2)>
KFR_INTRINSIC vec<T, N> cmul_impl(const vec<T, N>& x, const vec<T, 2>& y)
{
    vec<T, N> yy = resize<N>(y);
    return cmul_impl(x, yy);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > 2)>
KFR_INTRINSIC vec<T, N> cmul_impl(const vec<T, 2>& x, const vec<T, N>& y)
{
    vec<T, N> xx = resize<N>(x);
    return cmul_impl(xx, y);
}

/// Complex Multiplication
template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, const_max(N1, N2)> cmul(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return intrinsics::cmul_impl(x, y);
}

template <typename T, size_t N, KFR_ENABLE_IF(N >= 2)>
KFR_INTRINSIC vec<T, N> cmul_conj(const vec<T, N>& x, const vec<T, N>& y)
{
    return swap<2>(subadd(swap<2>(x) * dupeven(y), x * dupodd(y)));
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= 2)>
KFR_INTRINSIC vec<T, N> cmul_2conj(const vec<T, N>& in0, const vec<T, N>& in1, const vec<T, N>& tw)
{
    return (in0 + in1) * dupeven(tw) + swap<2>(cnegimag(in0 - in1)) * dupodd(tw);
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= 2)>
KFR_INTRINSIC void cmul_2conj(vec<T, N>& out0, vec<T, N>& out1, const vec<T, 2>& in0, const vec<T, 2>& in1,
                              const vec<T, N>& tw)
{
    const vec<T, N> twr   = dupeven(tw);
    const vec<T, N> twi   = dupodd(tw);
    const vec<T, 2> sum   = (in0 + in1);
    const vec<T, 2> dif   = swap<2>(negodd(in0 - in1));
    const vec<T, N> sumtw = resize<N>(sum) * twr;
    const vec<T, N> diftw = resize<N>(dif) * twi;
    out0 += sumtw + diftw;
    out1 += sumtw - diftw;
}
template <typename T, size_t N, KFR_ENABLE_IF(N > 2)>
KFR_INTRINSIC vec<T, N> cmul_conj(const vec<T, N>& x, const vec<T, 2>& y)
{
    vec<T, N> yy = resize<N>(y);
    return cmul_conj(x, yy);
}
template <typename T, size_t N, KFR_ENABLE_IF(N > 2)>
KFR_INTRINSIC vec<T, N> cmul_conj(const vec<T, 2>& x, const vec<T, N>& y)
{
    vec<T, N> xx = resize<N>(x);
    return cmul_conj(xx, y);
}

template <size_t N, bool A = false, typename T>
KFR_INTRINSIC cvec<T, N> cread(const complex<T>* src)
{
    return cvec<T, N>(ptr_cast<T>(src), cbool_t<A>());
}

template <size_t N, bool A = false, typename T>
KFR_INTRINSIC void cwrite(complex<T>* dest, const cvec<T, N>& value)
{
    value.write(ptr_cast<T>(dest), cbool_t<A>());
}

template <size_t count, size_t N, size_t stride, bool A, typename T, size_t... indices>
KFR_INTRINSIC cvec<T, count * N> cread_group_impl(const complex<T>* src, csizes_t<indices...>)
{
    return concat(read(cbool<A>, csize<N * 2>, ptr_cast<T>(src + stride * indices))...);
}
template <size_t count, size_t N, size_t stride, bool A, typename T, size_t... indices>
KFR_INTRINSIC void cwrite_group_impl(complex<T>* dest, const cvec<T, count * N>& value, csizes_t<indices...>)
{
    swallow{ (write<A>(ptr_cast<T>(dest + stride * indices), slice<indices * N * 2, N * 2>(value)), 0)... };
}

template <size_t count, size_t N, bool A, typename T, size_t... indices>
KFR_INTRINSIC cvec<T, count * N> cread_group_impl(const complex<T>* src, size_t stride, csizes_t<indices...>)
{
    return concat(read(cbool<A>, csize<N * 2>, ptr_cast<T>(src + stride * indices))...);
}
template <size_t count, size_t N, bool A, typename T, size_t... indices>
KFR_INTRINSIC void cwrite_group_impl(complex<T>* dest, size_t stride, const cvec<T, count * N>& value,
                                     csizes_t<indices...>)
{
    swallow{ (write<A>(ptr_cast<T>(dest + stride * indices), slice<indices * N * 2, N * 2>(value)), 0)... };
}

template <size_t count, size_t N, size_t stride, bool A = false, typename T>
KFR_INTRINSIC cvec<T, count * N> cread_group(const complex<T>* src)
{
    return cread_group_impl<count, N, stride, A>(src, csizeseq_t<count>());
}

template <size_t count, size_t N, size_t stride, bool A = false, typename T>
KFR_INTRINSIC void cwrite_group(complex<T>* dest, const cvec<T, count * N>& value)
{
    return cwrite_group_impl<count, N, stride, A>(dest, value, csizeseq_t<count>());
}

template <size_t count, size_t N, bool A = false, typename T>
KFR_INTRINSIC cvec<T, count * N> cread_group(const complex<T>* src, size_t stride)
{
    return cread_group_impl<count, N, A>(src, stride, csizeseq_t<count>());
}

template <size_t count, size_t N, bool A = false, typename T>
KFR_INTRINSIC void cwrite_group(complex<T>* dest, size_t stride, const cvec<T, count * N>& value)
{
    return cwrite_group_impl<count, N, A>(dest, stride, value, csizeseq_t<count>());
}

template <size_t N, bool A = false, bool split = false, typename T>
KFR_INTRINSIC cvec<T, N> cread_split(const complex<T>* src)
{
    cvec<T, N> temp = cvec<T, N>(ptr_cast<T>(src), cbool_t<A>());
    if (split)
        temp = splitpairs(temp);
    return temp;
}

template <size_t N, bool A = false, bool split = false, typename T>
KFR_INTRINSIC void cwrite_split(complex<T>* dest, const cvec<T, N>& value)
{
    cvec<T, N> v = value;
    if (split)
        v = interleavehalves(v);
    v.write(ptr_cast<T>(dest), cbool_t<A>());
}

template <>
inline cvec<f32, 8> cread_split<8, false, true, f32>(const complex<f32>* src)
{
    const cvec<f32, 4> l = concat(cread<2>(src), cread<2>(src + 4));
    const cvec<f32, 4> h = concat(cread<2>(src + 2), cread<2>(src + 6));

    return concat(shuffle<0, 2, 8 + 0, 8 + 2>(l, h), shuffle<1, 3, 8 + 1, 8 + 3>(l, h));
}
template <>
inline cvec<f32, 8> cread_split<8, true, true, f32>(const complex<f32>* src)
{
    const cvec<f32, 4> l = concat(cread<2, true>(src), cread<2, true>(src + 4));
    const cvec<f32, 4> h = concat(cread<2, true>(src + 2), cread<2, true>(src + 6));

    return concat(shuffle<0, 2, 8 + 0, 8 + 2>(l, h), shuffle<1, 3, 8 + 1, 8 + 3>(l, h));
}

template <>
inline cvec<f64, 4> cread_split<4, false, true, f64>(const complex<f64>* src)
{
    const cvec<f64, 2> l = concat(cread<1>(src), cread<1>(src + 2));
    const cvec<f64, 2> h = concat(cread<1>(src + 1), cread<1>(src + 3));

    return concat(shuffle<0, 4, 2, 6>(l, h), shuffle<1, 5, 3, 7>(l, h));
}

template <>
inline void cwrite_split<8, false, true, f32>(complex<f32>* dest, const cvec<f32, 8>& x)
{
    const cvec<f32, 8> xx =
        concat(shuffle<0, 8 + 0, 1, 8 + 1>(low(x), high(x)), shuffle<2, 8 + 2, 3, 8 + 3>(low(x), high(x)));

    cvec<f32, 2> a, b, c, d;
    split<f32, 16>(xx, a, b, c, d);
    cwrite<2>(dest, a);
    cwrite<2>(dest + 4, b);
    cwrite<2>(dest + 2, c);
    cwrite<2>(dest + 6, d);
}
template <>
inline void cwrite_split<8, true, true, f32>(complex<f32>* dest, const cvec<f32, 8>& x)
{
    const cvec<f32, 8> xx =
        concat(shuffle<0, 8 + 0, 1, 8 + 1>(low(x), high(x)), shuffle<2, 8 + 2, 3, 8 + 3>(low(x), high(x)));

    cvec<f32, 2> a, b, c, d;
    split<f32, 16>(xx, a, b, c, d);
    cwrite<2, true>(dest + 0, a);
    cwrite<2, true>(dest + 4, b);
    cwrite<2, true>(dest + 2, c);
    cwrite<2, true>(dest + 6, d);
}

template <>
inline void cwrite_split<4, false, true, f64>(complex<f64>* dest, const cvec<f64, 4>& x)
{
    const cvec<f64, 4> xx =
        concat(shuffle<0, 4, 2, 6>(low(x), high(x)), shuffle<1, 5, 3, 7>(low(x), high(x)));
    cwrite<1>(dest, part<4, 0>(xx));
    cwrite<1>(dest + 2, part<4, 1>(xx));
    cwrite<1>(dest + 1, part<4, 2>(xx));
    cwrite<1>(dest + 3, part<4, 3>(xx));
}
template <>
inline void cwrite_split<4, true, true, f64>(complex<f64>* dest, const cvec<f64, 4>& x)
{
    const cvec<f64, 4> xx =
        concat(shuffle<0, 4, 2, 6>(low(x), high(x)), shuffle<1, 5, 3, 7>(low(x), high(x)));
    cwrite<1, true>(dest + 0, part<4, 0>(xx));
    cwrite<1, true>(dest + 2, part<4, 1>(xx));
    cwrite<1, true>(dest + 1, part<4, 2>(xx));
    cwrite<1, true>(dest + 3, part<4, 3>(xx));
}

template <size_t N, size_t stride, typename T, size_t... Indices>
KFR_INTRINSIC cvec<T, N> cgather_helper(const complex<T>* base, csizes_t<Indices...>)
{
    return concat(ref_cast<cvec<T, 1>>(base[Indices * stride])...);
}

template <size_t N, size_t stride, typename T>
KFR_INTRINSIC cvec<T, N> cgather(const complex<T>* base)
{
    if (stride == 1)
    {
        return ref_cast<cvec<T, N>>(*base);
    }
    else
        return cgather_helper<N, stride, T>(base, csizeseq_t<N>());
}

KFR_INTRINSIC size_t cgather_next(size_t& index, size_t stride, size_t size, size_t)
{
    size_t temp = index;
    index += stride;
    if (index >= size)
        index -= size;
    return temp;
}
KFR_INTRINSIC size_t cgather_next(size_t& index, size_t stride, size_t)
{
    size_t temp = index;
    index += stride;
    return temp;
}

template <size_t N, typename T, size_t... Indices>
KFR_INTRINSIC cvec<T, N> cgather_helper(const complex<T>* base, size_t& index, size_t stride,
                                        csizes_t<Indices...>)
{
    return concat(ref_cast<cvec<T, 1>>(base[cgather_next(index, stride, Indices)])...);
}

template <size_t N, typename T>
KFR_INTRINSIC cvec<T, N> cgather(const complex<T>* base, size_t& index, size_t stride)
{
    return cgather_helper<N, T>(base, index, stride, csizeseq_t<N>());
}
template <size_t N, typename T>
KFR_INTRINSIC cvec<T, N> cgather(const complex<T>* base, size_t stride)
{
    size_t index = 0;
    return cgather_helper<N, T>(base, index, stride, csizeseq_t<N>());
}

template <size_t N, typename T, size_t... Indices>
KFR_INTRINSIC cvec<T, N> cgather_helper(const complex<T>* base, size_t& index, size_t stride, size_t size,
                                        csizes_t<Indices...>)
{
    return concat(ref_cast<cvec<T, 1>>(base[cgather_next(index, stride, size, Indices)])...);
}

template <size_t N, typename T>
KFR_INTRINSIC cvec<T, N> cgather(const complex<T>* base, size_t& index, size_t stride, size_t size)
{
    return cgather_helper<N, T>(base, index, stride, size, csizeseq_t<N>());
}

template <size_t N, size_t stride, typename T, size_t... Indices>
KFR_INTRINSIC void cscatter_helper(complex<T>* base, const cvec<T, N>& value, csizes_t<Indices...>)
{
    swallow{ (cwrite<1>(base + Indices * stride, slice<Indices * 2, 2>(value)), 0)... };
}

template <size_t N, size_t stride, typename T>
KFR_INTRINSIC void cscatter(complex<T>* base, const cvec<T, N>& value)
{
    if (stride == 1)
    {
        cwrite<N>(base, value);
    }
    else
    {
        return cscatter_helper<N, stride, T>(base, value, csizeseq_t<N>());
    }
}

template <size_t N, typename T, size_t... Indices>
KFR_INTRINSIC void cscatter_helper(complex<T>* base, size_t stride, const cvec<T, N>& value,
                                   csizes_t<Indices...>)
{
    swallow{ (cwrite<1>(base + Indices * stride, slice<Indices * 2, 2>(value)), 0)... };
}

template <size_t N, typename T>
KFR_INTRINSIC void cscatter(complex<T>* base, size_t stride, const cvec<T, N>& value)
{
    return cscatter_helper<N, T>(base, stride, value, csizeseq_t<N>());
}

template <size_t groupsize = 1, typename T, size_t N, typename IT>
KFR_INTRINSIC vec<T, N * 2 * groupsize> cgather(const complex<T>* base, const vec<IT, N>& offset)
{
    return internal::gather_helper<2 * groupsize>(ptr_cast<T>(base), offset, csizeseq_t<N>());
}

template <size_t groupsize = 1, typename T, size_t N, typename IT>
KFR_INTRINSIC void cscatter(complex<T>* base, const vec<IT, N>& offset, vec<T, N * 2 * groupsize> value)
{
    return internal::scatter_helper<2 * groupsize>(ptr_cast<T>(base), offset, value, csizeseq_t<N>());
}

template <typename T>
KFR_INTRINSIC void transpose4x8(const cvec<T, 8>& z0, const cvec<T, 8>& z1, const cvec<T, 8>& z2,
                                const cvec<T, 8>& z3, cvec<T, 4>& w0, cvec<T, 4>& w1, cvec<T, 4>& w2,
                                cvec<T, 4>& w3, cvec<T, 4>& w4, cvec<T, 4>& w5, cvec<T, 4>& w6,
                                cvec<T, 4>& w7)
{
    cvec<T, 16> a = concat(low(z0), low(z1), low(z2), low(z3));
    cvec<T, 16> b = concat(high(z0), high(z1), high(z2), high(z3));
    a             = digitreverse4<2>(a);
    b             = digitreverse4<2>(b);
    w0            = part<4, 0>(a);
    w1            = part<4, 1>(a);
    w2            = part<4, 2>(a);
    w3            = part<4, 3>(a);
    w4            = part<4, 0>(b);
    w5            = part<4, 1>(b);
    w6            = part<4, 2>(b);
    w7            = part<4, 3>(b);
}

template <typename T>
KFR_INTRINSIC void transpose4x8(const cvec<T, 4>& w0, const cvec<T, 4>& w1, const cvec<T, 4>& w2,
                                const cvec<T, 4>& w3, const cvec<T, 4>& w4, const cvec<T, 4>& w5,
                                const cvec<T, 4>& w6, const cvec<T, 4>& w7, cvec<T, 8>& z0, cvec<T, 8>& z1,
                                cvec<T, 8>& z2, cvec<T, 8>& z3)
{
    cvec<T, 16> a = concat(w0, w1, w2, w3);
    cvec<T, 16> b = concat(w4, w5, w6, w7);
    a             = digitreverse4<2>(a);
    b             = digitreverse4<2>(b);
    z0            = concat(part<4, 0>(a), part<4, 0>(b));
    z1            = concat(part<4, 1>(a), part<4, 1>(b));
    z2            = concat(part<4, 2>(a), part<4, 2>(b));
    z3            = concat(part<4, 3>(a), part<4, 3>(b));
}

template <typename T>
KFR_INTRINSIC void transpose4(cvec<T, 16>& a, cvec<T, 16>& b, cvec<T, 16>& c, cvec<T, 16>& d)
{
    cvec<T, 4> a0, a1, a2, a3;
    cvec<T, 4> b0, b1, b2, b3;
    cvec<T, 4> c0, c1, c2, c3;
    cvec<T, 4> d0, d1, d2, d3;

    split<T, 32>(a, a0, a1, a2, a3);
    split<T, 32>(b, b0, b1, b2, b3);
    split<T, 32>(c, c0, c1, c2, c3);
    split<T, 32>(d, d0, d1, d2, d3);

    a = concat(a0, b0, c0, d0);
    b = concat(a1, b1, c1, d1);
    c = concat(a2, b2, c2, d2);
    d = concat(a3, b3, c3, d3);
}
template <typename T>
KFR_INTRINSIC void transpose4(cvec<T, 16>& a, cvec<T, 16>& b, cvec<T, 16>& c, cvec<T, 16>& d, cvec<T, 16>& aa,
                              cvec<T, 16>& bb, cvec<T, 16>& cc, cvec<T, 16>& dd)
{
    cvec<T, 4> a0, a1, a2, a3;
    cvec<T, 4> b0, b1, b2, b3;
    cvec<T, 4> c0, c1, c2, c3;
    cvec<T, 4> d0, d1, d2, d3;

    split<T, 32>(a, a0, a1, a2, a3);
    split<T, 32>(b, b0, b1, b2, b3);
    split<T, 32>(c, c0, c1, c2, c3);
    split<T, 32>(d, d0, d1, d2, d3);

    aa = concat(a0, b0, c0, d0);
    bb = concat(a1, b1, c1, d1);
    cc = concat(a2, b2, c2, d2);
    dd = concat(a3, b3, c3, d3);
}

template <bool b, typename T>
constexpr KFR_INTRINSIC T chsign(T x)
{
    return b ? -x : x;
}

template <typename T, size_t N, size_t size, size_t start, size_t step, bool inverse = false,
          size_t... indices>
constexpr KFR_INTRINSIC cvec<T, N> get_fixed_twiddle_helper(csizes_t<indices...>)
{
    return make_vector((indices & 1 ? chsign<inverse>(-sin_using_table<T>(size, (indices / 2 * step + start)))
                                    : cos_using_table<T>(size, (indices / 2 * step + start)))...);
}

template <typename T, size_t width, size_t... indices>
constexpr KFR_INTRINSIC cvec<T, width> get_fixed_twiddle_helper(csizes_t<indices...>, size_t size,
                                                                size_t start, size_t step)
{
    return make_vector((indices & 1 ? -sin_using_table<T>(size, indices / 2 * step + start)
                                    : cos_using_table<T>(size, indices / 2 * step + start))...);
}

template <typename T, size_t width, size_t size, size_t start, size_t step = 0, bool inverse = false>
constexpr KFR_INTRINSIC cvec<T, width> fixed_twiddle()
{
    return get_fixed_twiddle_helper<T, width, size, start, step, inverse>(csizeseq_t<width * 2>());
}

template <typename T, size_t width>
constexpr KFR_INTRINSIC cvec<T, width> fixed_twiddle(size_t size, size_t start, size_t step = 0)
{
    return get_fixed_twiddle_helper<T, width>(csizeseq_t<width * 2>(), start, step, size);
}

// template <typename T, size_t N, size_t size, size_t start, size_t step = 0, bool inverse = false>
// constexpr cvec<T, N> fixed_twiddle = get_fixed_twiddle<T, N, size, start, step, inverse>();

template <typename T, size_t N, bool inverse>
constexpr static inline cvec<T, N> twiddleimagmask()
{
    return inverse ? broadcast<N * 2, T>(-1, +1) : broadcast<N * 2, T>(+1, -1);
}

CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wconversion")

CMT_PRAGMA_GNU(GCC diagnostic pop)

template <typename T, size_t N>
CMT_NOINLINE static vec<T, N> cossin_conj(const vec<T, N>& x)
{
    return negodd(cossin(x));
}

template <size_t k, size_t size, bool inverse = false, typename T, size_t width,
          size_t kk = (inverse ? size - k : k) % size>
KFR_INTRINSIC vec<T, width> cmul_by_twiddle(const vec<T, width>& x)
{
    constexpr T isqrt2 = static_cast<T>(0.70710678118654752440084436210485);
    if (kk == 0)
    {
        return x;
    }
    else if (kk == size * 1 / 8)
    {
        return swap<2>(subadd(swap<2>(x), x)) * isqrt2;
    }
    else if (kk == size * 2 / 8)
    {
        return negodd(swap<2>(x));
    }
    else if (kk == size * 3 / 8)
    {
        return subadd(x, swap<2>(x)) * -isqrt2;
    }
    else if (kk == size * 4 / 8)
    {
        return -x;
    }
    else if (kk == size * 5 / 8)
    {
        return swap<2>(subadd(swap<2>(x), x)) * -isqrt2;
    }
    else if (kk == size * 6 / 8)
    {
        return swap<2>(negodd(x));
    }
    else if (kk == size * 7 / 8)
    {
        return subadd(x, swap<2>(x)) * isqrt2;
    }
    else
    {
        return cmul(x, resize<width>(fixed_twiddle<T, 1, size, kk>()));
    }
}

template <size_t N, typename T>
KFR_INTRINSIC void butterfly2(const cvec<T, N>& a0, const cvec<T, N>& a1, cvec<T, N>& w0, cvec<T, N>& w1)
{
    const cvec<T, N> sum = a0 + a1;
    const cvec<T, N> dif = a0 - a1;
    w0                   = sum;
    w1                   = dif;
}

template <size_t N, typename T>
KFR_INTRINSIC void butterfly2(cvec<T, N>& a0, cvec<T, N>& a1)
{
    butterfly2<N>(a0, a1, a0, a1);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly4(cfalse_t /*split_format*/, const cvec<T, N>& a0, const cvec<T, N>& a1,
                              const cvec<T, N>& a2, const cvec<T, N>& a3, cvec<T, N>& w0, cvec<T, N>& w1,
                              cvec<T, N>& w2, cvec<T, N>& w3)
{
    cvec<T, N> sum02, sum13, diff02, diff13;
    cvec<T, N * 2> a01, a23, sum0213, diff0213;

    a01      = concat(a0, a1);
    a23      = concat(a2, a3);
    sum0213  = a01 + a23;
    diff0213 = a01 - a23;

    sum02  = low(sum0213);
    sum13  = high(sum0213);
    diff02 = low(diff0213);
    diff13 = high(diff0213);
    w0     = sum02 + sum13;
    w2     = sum02 - sum13;
    if (inverse)
    {
        diff13 = (diff13 ^ broadcast<N * 2, T>(T(), -T()));
        diff13 = swap<2>(diff13);
    }
    else
    {
        diff13 = swap<2>(diff13);
        diff13 = (diff13 ^ broadcast<N * 2, T>(T(), -T()));
    }

    w1 = diff02 + diff13;
    w3 = diff02 - diff13;
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly4(ctrue_t /*split_format*/, const cvec<T, N>& a0, const cvec<T, N>& a1,
                              const cvec<T, N>& a2, const cvec<T, N>& a3, cvec<T, N>& w0, cvec<T, N>& w1,
                              cvec<T, N>& w2, cvec<T, N>& w3)
{
    vec<T, N> re0, im0, re1, im1, re2, im2, re3, im3;
    vec<T, N> wre0, wim0, wre1, wim1, wre2, wim2, wre3, wim3;

    cvec<T, N> sum02, sum13, diff02, diff13;
    vec<T, N> sum02re, sum13re, diff02re, diff13re;
    vec<T, N> sum02im, sum13im, diff02im, diff13im;

    sum02 = a0 + a2;
    sum13 = a1 + a3;

    w0 = sum02 + sum13;
    w2 = sum02 - sum13;

    diff02 = a0 - a2;
    diff13 = a1 - a3;
    split(diff02, diff02re, diff02im);
    split(diff13, diff13re, diff13im);

    (inverse ? w3 : w1) = concat(diff02re + diff13im, diff02im - diff13re);
    (inverse ? w1 : w3) = concat(diff02re - diff13im, diff02im + diff13re);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly8(const cvec<T, N>& a0, const cvec<T, N>& a1, const cvec<T, N>& a2,
                              const cvec<T, N>& a3, const cvec<T, N>& a4, const cvec<T, N>& a5,
                              const cvec<T, N>& a6, const cvec<T, N>& a7, cvec<T, N>& w0, cvec<T, N>& w1,
                              cvec<T, N>& w2, cvec<T, N>& w3, cvec<T, N>& w4, cvec<T, N>& w5, cvec<T, N>& w6,
                              cvec<T, N>& w7)
{
    cvec<T, N> b0 = a0, b2 = a2, b4 = a4, b6 = a6;
    butterfly4<N, inverse>(cfalse, b0, b2, b4, b6, b0, b2, b4, b6);
    cvec<T, N> b1 = a1, b3 = a3, b5 = a5, b7 = a7;
    butterfly4<N, inverse>(cfalse, b1, b3, b5, b7, b1, b3, b5, b7);
    w0 = b0 + b1;
    w4 = b0 - b1;

    b3 = cmul_by_twiddle<1, 8, inverse>(b3);
    b5 = cmul_by_twiddle<2, 8, inverse>(b5);
    b7 = cmul_by_twiddle<3, 8, inverse>(b7);

    w1 = b2 + b3;
    w5 = b2 - b3;
    w2 = b4 + b5;
    w6 = b4 - b5;
    w3 = b6 + b7;
    w7 = b6 - b7;
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly8(cvec<T, N>& a0, cvec<T, N>& a1, cvec<T, N>& a2, cvec<T, N>& a3, cvec<T, N>& a4,
                              cvec<T, N>& a5, cvec<T, N>& a6, cvec<T, N>& a7)
{
    butterfly8<N, inverse>(a0, a1, a2, a3, a4, a5, a6, a7, a0, a1, a2, a3, a4, a5, a6, a7);
}

template <bool inverse = false, typename T>
KFR_INTRINSIC void butterfly8(cvec<T, 2>& a01, cvec<T, 2>& a23, cvec<T, 2>& a45, cvec<T, 2>& a67)
{
    cvec<T, 2> b01 = a01, b23 = a23, b45 = a45, b67 = a67;

    butterfly4<2, inverse>(cfalse, b01, b23, b45, b67, b01, b23, b45, b67);

    cvec<T, 2> b02, b13, b46, b57;

    cvec<T, 8> b01234567 = concat(b01, b23, b45, b67);
    cvec<T, 8> b02461357 = concat(even<2>(b01234567), odd<2>(b01234567));
    split<T, 16>(b02461357, b02, b46, b13, b57);

    b13 = cmul(b13, fixed_twiddle<T, 2, 8, 0, 1, inverse>());
    b57 = cmul(b57, fixed_twiddle<T, 2, 8, 2, 1, inverse>());
    a01 = b02 + b13;
    a23 = b46 + b57;
    a45 = b02 - b13;
    a67 = b46 - b57;
}

template <bool inverse = false, typename T>
KFR_INTRINSIC void butterfly8(cvec<T, 8>& v8)
{
    cvec<T, 2> w0, w1, w2, w3;
    split<T, 16>(v8, w0, w1, w2, w3);
    butterfly8<inverse>(w0, w1, w2, w3);
    v8 = concat(w0, w1, w2, w3);
}

template <bool inverse = false, typename T>
KFR_INTRINSIC void butterfly32(cvec<T, 32>& v32)
{
    cvec<T, 4> w0, w1, w2, w3, w4, w5, w6, w7;
    split(v32, w0, w1, w2, w3, w4, w5, w6, w7);
    butterfly8<4, inverse>(w0, w1, w2, w3, w4, w5, w6, w7);

    w1 = cmul(w1, fixed_twiddle<T, 4, 32, 0, 1, inverse>());
    w2 = cmul(w2, fixed_twiddle<T, 4, 32, 0, 2, inverse>());
    w3 = cmul(w3, fixed_twiddle<T, 4, 32, 0, 3, inverse>());
    w4 = cmul(w4, fixed_twiddle<T, 4, 32, 0, 4, inverse>());
    w5 = cmul(w5, fixed_twiddle<T, 4, 32, 0, 5, inverse>());
    w6 = cmul(w6, fixed_twiddle<T, 4, 32, 0, 6, inverse>());
    w7 = cmul(w7, fixed_twiddle<T, 4, 32, 0, 7, inverse>());

    cvec<T, 8> z0, z1, z2, z3;
    transpose4x8(w0, w1, w2, w3, w4, w5, w6, w7, z0, z1, z2, z3);

    butterfly4<8, inverse>(cfalse, z0, z1, z2, z3, z0, z1, z2, z3);
    v32 = concat(z0, z1, z2, z3);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly4(cvec<T, N * 4>& a0123)
{
    cvec<T, N> a0;
    cvec<T, N> a1;
    cvec<T, N> a2;
    cvec<T, N> a3;
    split<T, N * 4 * 2>(a0123, a0, a1, a2, a3);
    butterfly4<N, inverse>(cfalse, a0, a1, a2, a3, a0, a1, a2, a3);
    a0123 = concat(a0, a1, a2, a3);
}

template <size_t N, typename T>
KFR_INTRINSIC void butterfly2(cvec<T, N * 2>& a01)
{
    cvec<T, N> a0;
    cvec<T, N> a1;
    split(a01, a0, a1);
    butterfly2<N>(a0, a1);
    a01 = concat(a0, a1);
}

template <size_t N, bool inverse = false, bool split_format = false, typename T>
KFR_INTRINSIC void apply_twiddle(const cvec<T, N>& a1, const cvec<T, N>& tw1, cvec<T, N>& w1)
{
    if (split_format)
    {
        vec<T, N> re1, im1, tw1re, tw1im;
        split<T, 2 * N>(a1, re1, im1);
        split<T, 2 * N>(tw1, tw1re, tw1im);
        vec<T, N> b1re = re1 * tw1re;
        vec<T, N> b1im = im1 * tw1re;
        if (inverse)
            w1 = concat(b1re + im1 * tw1im, b1im - re1 * tw1im);
        else
            w1 = concat(b1re - im1 * tw1im, b1im + re1 * tw1im);
    }
    else
    {
        const cvec<T, N> b1  = a1 * dupeven(tw1);
        const cvec<T, N> a1_ = swap<2>(a1);

        cvec<T, N> tw1_ = tw1;
        if (inverse)
            tw1_ = -(tw1_);
        w1 = subadd(b1, a1_ * dupodd(tw1_));
    }
}

template <size_t N, bool inverse = false, bool split_format = false, typename T>
KFR_INTRINSIC void apply_twiddles4(const cvec<T, N>& a1, const cvec<T, N>& a2, const cvec<T, N>& a3,
                                   const cvec<T, N>& tw1, const cvec<T, N>& tw2, const cvec<T, N>& tw3,
                                   cvec<T, N>& w1, cvec<T, N>& w2, cvec<T, N>& w3)
{
    apply_twiddle<N, inverse, split_format>(a1, tw1, w1);
    apply_twiddle<N, inverse, split_format>(a2, tw2, w2);
    apply_twiddle<N, inverse, split_format>(a3, tw3, w3);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void apply_twiddles4(cvec<T, N>& __restrict a1, cvec<T, N>& __restrict a2,
                                   cvec<T, N>& __restrict a3, const cvec<T, N>& tw1, const cvec<T, N>& tw2,
                                   const cvec<T, N>& tw3)
{
    apply_twiddles4<N, inverse>(a1, a2, a3, tw1, tw2, tw3, a1, a2, a3);
}

template <size_t N, bool inverse = false, typename T, typename = u8[N - 1]>
KFR_INTRINSIC void apply_twiddles4(cvec<T, N>& __restrict a1, cvec<T, N>& __restrict a2,
                                   cvec<T, N>& __restrict a3, const cvec<T, 1>& tw1, const cvec<T, 1>& tw2,
                                   const cvec<T, 1>& tw3)
{
    apply_twiddles4<N, inverse>(a1, a2, a3, resize<N * 2>(tw1), resize<N * 2>(tw2), resize<N * 2>(tw3));
}

template <size_t N, bool inverse = false, typename T, typename = u8[N - 2]>
KFR_INTRINSIC void apply_twiddles4(cvec<T, N>& __restrict a1, cvec<T, N>& __restrict a2,
                                   cvec<T, N>& __restrict a3, cvec<T, N / 2> tw1, cvec<T, N / 2> tw2,
                                   cvec<T, N / 2> tw3)
{
    apply_twiddles4<N, inverse>(a1, a2, a3, resize<N * 2>(tw1), resize<N * 2>(tw2), resize<N * 2>(tw3));
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void apply_vertical_twiddles4(cvec<T, N * 4>& b, cvec<T, N * 4>& c, cvec<T, N * 4>& d)
{
    cvec<T, 4> b0, b1, b2, b3;
    cvec<T, 4> c0, c1, c2, c3;
    cvec<T, 4> d0, d1, d2, d3;

    split(b, b0, b1, b2, b3);
    split(c, c0, c1, c2, c3);
    split(d, d0, d1, d2, d3);

    b1 = cmul_by_twiddle<4, 64, inverse>(b1);
    b2 = cmul_by_twiddle<8, 64, inverse>(b2);
    b3 = cmul_by_twiddle<12, 64, inverse>(b3);

    c1 = cmul_by_twiddle<8, 64, inverse>(c1);
    c2 = cmul_by_twiddle<16, 64, inverse>(c2);
    c3 = cmul_by_twiddle<24, 64, inverse>(c3);

    d1 = cmul_by_twiddle<12, 64, inverse>(d1);
    d2 = cmul_by_twiddle<24, 64, inverse>(d2);
    d3 = cmul_by_twiddle<36, 64, inverse>(d3);

    b = concat(b0, b1, b2, b3);
    c = concat(c0, c1, c2, c3);
    d = concat(d0, d1, d2, d3);
}

template <size_t n2, size_t nnstep, size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void apply_twiddles4(cvec<T, N * 4>& __restrict a0123)
{
    cvec<T, N> a0;
    cvec<T, N> a1;
    cvec<T, N> a2;
    cvec<T, N> a3;
    split<T, 2 * N * 4>(a0123, a0, a1, a2, a3);

    cvec<T, N> tw1 = fixed_twiddle<T, N, 64, n2 * nnstep * 1, nnstep * 1, inverse>(),
               tw2 = fixed_twiddle<T, N, 64, n2 * nnstep * 2, nnstep * 2, inverse>(),
               tw3 = fixed_twiddle<T, N, 64, n2 * nnstep * 3, nnstep * 3, inverse>();

    apply_twiddles4<N>(a1, a2, a3, tw1, tw2, tw3);

    a0123 = concat(a0, a1, a2, a3);
}

template <bool inverse, bool aligned, typename T>
KFR_INTRINSIC void butterfly64(cbool_t<inverse>, cbool_t<aligned>, complex<T>* out, const complex<T>* in)
{
    cvec<T, 16> w0, w1, w2, w3;

    w0 = cread_group<4, 4, 16, aligned>(
        in); // concat(cread<4>(in + 0), cread<4>(in + 16), cread<4>(in + 32), cread<4>(in + 48));
    butterfly4<4, inverse>(w0);
    apply_twiddles4<0, 1, 4, inverse>(w0);

    w1 = cread_group<4, 4, 16, aligned>(
        in + 4); // concat(cread<4>(in + 4), cread<4>(in + 20), cread<4>(in + 36), cread<4>(in + 52));
    butterfly4<4, inverse>(w1);
    apply_twiddles4<4, 1, 4, inverse>(w1);

    w2 = cread_group<4, 4, 16, aligned>(
        in + 8); // concat(cread<4>(in + 8), cread<4>(in + 24), cread<4>(in + 40), cread<4>(in + 56));
    butterfly4<4, inverse>(w2);
    apply_twiddles4<8, 1, 4, inverse>(w2);

    w3 = cread_group<4, 4, 16, aligned>(
        in + 12); // concat(cread<4>(in + 12), cread<4>(in + 28), cread<4>(in + 44), cread<4>(in + 60));
    butterfly4<4, inverse>(w3);
    apply_twiddles4<12, 1, 4, inverse>(w3);

    transpose4(w0, w1, w2, w3);
    // pass 2:

    butterfly4<4, inverse>(w0);
    butterfly4<4, inverse>(w1);
    butterfly4<4, inverse>(w2);
    butterfly4<4, inverse>(w3);

    transpose4(w0, w1, w2, w3);

    w0 = digitreverse4<2>(w0);
    w1 = digitreverse4<2>(w1);
    w2 = digitreverse4<2>(w2);
    w3 = digitreverse4<2>(w3);

    apply_vertical_twiddles4<4, inverse>(w1, w2, w3);

    // pass 3:
    butterfly4<4, inverse>(w3);
    cwrite_group<4, 4, 16, aligned>(out + 12, w3); //        split(w3, out[3], out[7], out[11], out[15]);

    butterfly4<4, inverse>(w2);
    cwrite_group<4, 4, 16, aligned>(out + 8, w2); //        split(w2, out[2], out[6], out[10], out[14]);

    butterfly4<4, inverse>(w1);
    cwrite_group<4, 4, 16, aligned>(out + 4, w1); //     split(w1, out[1], out[5], out[9], out[13]);

    butterfly4<4, inverse>(w0);
    cwrite_group<4, 4, 16, aligned>(out, w0); //     split(w0, out[0], out[4], out[8], out[12]);
}

template <bool inverse = false, typename T>
KFR_INTRINSIC void butterfly16(cvec<T, 16>& v16)
{
    butterfly4<4, inverse>(v16);
    apply_twiddles4<0, 4, 4, inverse>(v16);
    v16 = digitreverse4<2>(v16);
    butterfly4<4, inverse>(v16);
}

template <size_t index, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly16_multi_natural(complex<T>* out, const complex<T>* in)
{
    constexpr size_t N = 4;

    cvec<T, 4> a1  = cread<4>(in + index * 4 + 16 * 1);
    cvec<T, 4> a5  = cread<4>(in + index * 4 + 16 * 5);
    cvec<T, 4> a9  = cread<4>(in + index * 4 + 16 * 9);
    cvec<T, 4> a13 = cread<4>(in + index * 4 + 16 * 13);
    butterfly4<N, inverse>(cfalse, a1, a5, a9, a13, a1, a5, a9, a13);
    a5  = cmul_by_twiddle<1, 16, inverse>(a5);
    a9  = cmul_by_twiddle<2, 16, inverse>(a9);
    a13 = cmul_by_twiddle<3, 16, inverse>(a13);

    cvec<T, 4> a2  = cread<4>(in + index * 4 + 16 * 2);
    cvec<T, 4> a6  = cread<4>(in + index * 4 + 16 * 6);
    cvec<T, 4> a10 = cread<4>(in + index * 4 + 16 * 10);
    cvec<T, 4> a14 = cread<4>(in + index * 4 + 16 * 14);
    butterfly4<N, inverse>(cfalse, a2, a6, a10, a14, a2, a6, a10, a14);
    a6  = cmul_by_twiddle<2, 16, inverse>(a6);
    a10 = cmul_by_twiddle<4, 16, inverse>(a10);
    a14 = cmul_by_twiddle<6, 16, inverse>(a14);

    cvec<T, 4> a3  = cread<4>(in + index * 4 + 16 * 3);
    cvec<T, 4> a7  = cread<4>(in + index * 4 + 16 * 7);
    cvec<T, 4> a11 = cread<4>(in + index * 4 + 16 * 11);
    cvec<T, 4> a15 = cread<4>(in + index * 4 + 16 * 15);
    butterfly4<N, inverse>(cfalse, a3, a7, a11, a15, a3, a7, a11, a15);
    a7  = cmul_by_twiddle<3, 16, inverse>(a7);
    a11 = cmul_by_twiddle<6, 16, inverse>(a11);
    a15 = cmul_by_twiddle<9, 16, inverse>(a15);

    cvec<T, 4> a0  = cread<4>(in + index * 4 + 16 * 0);
    cvec<T, 4> a4  = cread<4>(in + index * 4 + 16 * 4);
    cvec<T, 4> a8  = cread<4>(in + index * 4 + 16 * 8);
    cvec<T, 4> a12 = cread<4>(in + index * 4 + 16 * 12);
    butterfly4<N, inverse>(cfalse, a0, a4, a8, a12, a0, a4, a8, a12);
    butterfly4<N, inverse>(cfalse, a0, a1, a2, a3, a0, a1, a2, a3);
    cwrite<4>(out + index * 4 + 16 * 0, a0);
    cwrite<4>(out + index * 4 + 16 * 4, a1);
    cwrite<4>(out + index * 4 + 16 * 8, a2);
    cwrite<4>(out + index * 4 + 16 * 12, a3);
    butterfly4<N, inverse>(cfalse, a4, a5, a6, a7, a4, a5, a6, a7);
    cwrite<4>(out + index * 4 + 16 * 1, a4);
    cwrite<4>(out + index * 4 + 16 * 5, a5);
    cwrite<4>(out + index * 4 + 16 * 9, a6);
    cwrite<4>(out + index * 4 + 16 * 13, a7);
    butterfly4<N, inverse>(cfalse, a8, a9, a10, a11, a8, a9, a10, a11);
    cwrite<4>(out + index * 4 + 16 * 2, a8);
    cwrite<4>(out + index * 4 + 16 * 6, a9);
    cwrite<4>(out + index * 4 + 16 * 10, a10);
    cwrite<4>(out + index * 4 + 16 * 14, a11);
    butterfly4<N, inverse>(cfalse, a12, a13, a14, a15, a12, a13, a14, a15);
    cwrite<4>(out + index * 4 + 16 * 3, a12);
    cwrite<4>(out + index * 4 + 16 * 7, a13);
    cwrite<4>(out + index * 4 + 16 * 11, a14);
    cwrite<4>(out + index * 4 + 16 * 15, a15);
}

template <size_t index, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly16_multi_flip(complex<T>* out, const complex<T>* in)
{
    constexpr size_t N = 4;

    cvec<T, 4> a1  = cread<4>(in + index * 4 + 16 * 1);
    cvec<T, 4> a5  = cread<4>(in + index * 4 + 16 * 5);
    cvec<T, 4> a9  = cread<4>(in + index * 4 + 16 * 9);
    cvec<T, 4> a13 = cread<4>(in + index * 4 + 16 * 13);
    butterfly4<N, inverse>(cfalse, a1, a5, a9, a13, a1, a5, a9, a13);
    a5  = cmul_by_twiddle<1, 16, inverse>(a5);
    a9  = cmul_by_twiddle<2, 16, inverse>(a9);
    a13 = cmul_by_twiddle<3, 16, inverse>(a13);

    cvec<T, 4> a2  = cread<4>(in + index * 4 + 16 * 2);
    cvec<T, 4> a6  = cread<4>(in + index * 4 + 16 * 6);
    cvec<T, 4> a10 = cread<4>(in + index * 4 + 16 * 10);
    cvec<T, 4> a14 = cread<4>(in + index * 4 + 16 * 14);
    butterfly4<N, inverse>(cfalse, a2, a6, a10, a14, a2, a6, a10, a14);
    a6  = cmul_by_twiddle<2, 16, inverse>(a6);
    a10 = cmul_by_twiddle<4, 16, inverse>(a10);
    a14 = cmul_by_twiddle<6, 16, inverse>(a14);

    cvec<T, 4> a3  = cread<4>(in + index * 4 + 16 * 3);
    cvec<T, 4> a7  = cread<4>(in + index * 4 + 16 * 7);
    cvec<T, 4> a11 = cread<4>(in + index * 4 + 16 * 11);
    cvec<T, 4> a15 = cread<4>(in + index * 4 + 16 * 15);
    butterfly4<N, inverse>(cfalse, a3, a7, a11, a15, a3, a7, a11, a15);
    a7  = cmul_by_twiddle<3, 16, inverse>(a7);
    a11 = cmul_by_twiddle<6, 16, inverse>(a11);
    a15 = cmul_by_twiddle<9, 16, inverse>(a15);

    cvec<T, 16> w1 = concat(a1, a5, a9, a13);
    cvec<T, 16> w2 = concat(a2, a6, a10, a14);
    cvec<T, 16> w3 = concat(a3, a7, a11, a15);

    cvec<T, 4> a0  = cread<4>(in + index * 4 + 16 * 0);
    cvec<T, 4> a4  = cread<4>(in + index * 4 + 16 * 4);
    cvec<T, 4> a8  = cread<4>(in + index * 4 + 16 * 8);
    cvec<T, 4> a12 = cread<4>(in + index * 4 + 16 * 12);
    butterfly4<N, inverse>(cfalse, a0, a4, a8, a12, a0, a4, a8, a12);
    cvec<T, 16> w0 = concat(a0, a4, a8, a12);

    butterfly4<N * 4, inverse>(cfalse, w0, w1, w2, w3, w0, w1, w2, w3);

    w0 = digitreverse4<2>(w0);
    w1 = digitreverse4<2>(w1);
    w2 = digitreverse4<2>(w2);
    w3 = digitreverse4<2>(w3);

    transpose4(w0, w1, w2, w3);
    cwrite<16>(out + index * 64 + 16 * 0, cmul(w0, fixed_twiddle<T, 16, 256, 0, index * 4 + 0, inverse>()));
    cwrite<16>(out + index * 64 + 16 * 1, cmul(w1, fixed_twiddle<T, 16, 256, 0, index * 4 + 1, inverse>()));
    cwrite<16>(out + index * 64 + 16 * 2, cmul(w2, fixed_twiddle<T, 16, 256, 0, index * 4 + 2, inverse>()));
    cwrite<16>(out + index * 64 + 16 * 3, cmul(w3, fixed_twiddle<T, 16, 256, 0, index * 4 + 3, inverse>()));
}

template <size_t n2, size_t nnstep, size_t N, typename T>
KFR_INTRINSIC void apply_twiddles2(cvec<T, N>& a1)
{
    cvec<T, N> tw1 = fixed_twiddle<T, N, 64, n2 * nnstep * 1, nnstep * 1>();

    a1 = cmul(a1, tw1);
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw3r1()
{
    return static_cast<T>(-0.5 - 1.0);
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw3i1()
{
    return static_cast<T>(0.86602540378443864676372317075) * twiddleimagmask<T, N, inverse>();
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly3(cvec<T, N> a00, cvec<T, N> a01, cvec<T, N> a02, cvec<T, N>& w00,
                              cvec<T, N>& w01, cvec<T, N>& w02)
{

    const cvec<T, N> sum1 = a01 + a02;
    const cvec<T, N> dif1 = swap<2>(a01 - a02);
    w00                   = a00 + sum1;

    const cvec<T, N> s1 = w00 + sum1 * tw3r1<T, N, inverse>();

    const cvec<T, N> d1 = dif1 * tw3i1<T, N, inverse>();

    w01 = s1 + d1;
    w02 = s1 - d1;
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly3(cvec<T, N>& a0, cvec<T, N>& a1, cvec<T, N>& a2)
{
    butterfly3<N, inverse>(a0, a1, a2, a0, a1, a2);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly6(const cvec<T, N>& a0, const cvec<T, N>& a1, const cvec<T, N>& a2,
                              const cvec<T, N>& a3, const cvec<T, N>& a4, const cvec<T, N>& a5,
                              cvec<T, N>& w0, cvec<T, N>& w1, cvec<T, N>& w2, cvec<T, N>& w3, cvec<T, N>& w4,
                              cvec<T, N>& w5)
{
    cvec<T, N* 2> a03 = concat(a0, a3);
    cvec<T, N* 2> a25 = concat(a2, a5);
    cvec<T, N* 2> a41 = concat(a4, a1);
    butterfly3<N * 2, inverse>(a03, a25, a41, a03, a25, a41);
    cvec<T, N> t0, t1, t2, t3, t4, t5;
    split(a03, t0, t1);
    split(a25, t2, t3);
    split(a41, t4, t5);
    t3                = -t3;
    cvec<T, N* 2> a04 = concat(t0, t4);
    cvec<T, N* 2> a15 = concat(t1, t5);
    cvec<T, N * 2> w02, w35;
    butterfly2<N * 2>(a04, a15, w02, w35);
    split(w02, w0, w2);
    split(w35, w3, w5);

    butterfly2<N>(t2, t3, w1, w4);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly6(cvec<T, N>& a0, cvec<T, N>& a1, cvec<T, N>& a2, cvec<T, N>& a3, cvec<T, N>& a4,
                              cvec<T, N>& a5)
{
    butterfly6<N, inverse>(a0, a1, a2, a3, a4, a5, a0, a1, a2, a3, a4, a5);
}

template <typename T, bool inverse = false>
static constexpr KFR_INTRINSIC cvec<T, 1> tw9_1()
{
    return { T(0.76604444311897803520239265055541),
             (inverse ? -1 : 1) * T(-0.64278760968653932632264340990727) };
}
template <typename T, bool inverse = false>
static constexpr KFR_INTRINSIC cvec<T, 1> tw9_2()
{
    return { T(0.17364817766693034885171662676931),
             (inverse ? -1 : 1) * T(-0.98480775301220805936674302458952) };
}
template <typename T, bool inverse = false>
static constexpr KFR_INTRINSIC cvec<T, 1> tw9_4()
{
    return { T(-0.93969262078590838405410927732473),
             (inverse ? -1 : 1) * T(-0.34202014332566873304409961468226) };
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly9(const cvec<T, N>& a0, const cvec<T, N>& a1, const cvec<T, N>& a2,
                              const cvec<T, N>& a3, const cvec<T, N>& a4, const cvec<T, N>& a5,
                              const cvec<T, N>& a6, const cvec<T, N>& a7, const cvec<T, N>& a8,
                              cvec<T, N>& w0, cvec<T, N>& w1, cvec<T, N>& w2, cvec<T, N>& w3, cvec<T, N>& w4,
                              cvec<T, N>& w5, cvec<T, N>& w6, cvec<T, N>& w7, cvec<T, N>& w8)
{
    cvec<T, N* 3> a012 = concat(a0, a1, a2);
    cvec<T, N* 3> a345 = concat(a3, a4, a5);
    cvec<T, N* 3> a678 = concat(a6, a7, a8);
    butterfly3<N * 3, inverse>(a012, a345, a678, a012, a345, a678);
    cvec<T, N> t0, t1, t2, t3, t4, t5, t6, t7, t8;
    split(a012, t0, t1, t2);
    split(a345, t3, t4, t5);
    split(a678, t6, t7, t8);

    t4 = cmul(t4, tw9_1<T, inverse>());
    t5 = cmul(t5, tw9_2<T, inverse>());
    t7 = cmul(t7, tw9_2<T, inverse>());
    t8 = cmul(t8, tw9_4<T, inverse>());

    cvec<T, N* 3> t036 = concat(t0, t3, t6);
    cvec<T, N* 3> t147 = concat(t1, t4, t7);
    cvec<T, N* 3> t258 = concat(t2, t5, t8);

    butterfly3<N * 3, inverse>(t036, t147, t258, t036, t147, t258);
    split(t036, w0, w1, w2);
    split(t147, w3, w4, w5);
    split(t258, w6, w7, w8);
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly9(cvec<T, N>& a0, cvec<T, N>& a1, cvec<T, N>& a2, cvec<T, N>& a3, cvec<T, N>& a4,
                              cvec<T, N>& a5, cvec<T, N>& a6, cvec<T, N>& a7, cvec<T, N>& a8)
{
    butterfly9<N, inverse>(a0, a1, a2, a3, a4, a5, a6, a7, a8, a0, a1, a2, a3, a4, a5, a6, a7, a8);
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw7r1()
{
    return static_cast<T>(0.623489801858733530525004884 - 1.0);
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw7i1()
{
    return static_cast<T>(0.78183148246802980870844452667) * twiddleimagmask<T, N, inverse>();
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw7r2()
{
    return static_cast<T>(-0.2225209339563144042889025645 - 1.0);
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw7i2()
{
    return static_cast<T>(0.97492791218182360701813168299) * twiddleimagmask<T, N, inverse>();
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw7r3()
{
    return static_cast<T>(-0.90096886790241912623610231951 - 1.0);
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw7i3()
{
    return static_cast<T>(0.43388373911755812047576833285) * twiddleimagmask<T, N, inverse>();
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly7(cvec<T, N> a00, cvec<T, N> a01, cvec<T, N> a02, cvec<T, N> a03, cvec<T, N> a04,
                              cvec<T, N> a05, cvec<T, N> a06, cvec<T, N>& w00, cvec<T, N>& w01,
                              cvec<T, N>& w02, cvec<T, N>& w03, cvec<T, N>& w04, cvec<T, N>& w05,
                              cvec<T, N>& w06)
{
    const cvec<T, N> sum1 = a01 + a06;
    const cvec<T, N> dif1 = swap<2>(a01 - a06);
    const cvec<T, N> sum2 = a02 + a05;
    const cvec<T, N> dif2 = swap<2>(a02 - a05);
    const cvec<T, N> sum3 = a03 + a04;
    const cvec<T, N> dif3 = swap<2>(a03 - a04);
    w00                   = a00 + sum1 + sum2 + sum3;

    const cvec<T, N> s1 =
        w00 + sum1 * tw7r1<T, N, inverse>() + sum2 * tw7r2<T, N, inverse>() + sum3 * tw7r3<T, N, inverse>();
    const cvec<T, N> s2 =
        w00 + sum1 * tw7r2<T, N, inverse>() + sum2 * tw7r3<T, N, inverse>() + sum3 * tw7r1<T, N, inverse>();
    const cvec<T, N> s3 =
        w00 + sum1 * tw7r3<T, N, inverse>() + sum2 * tw7r1<T, N, inverse>() + sum3 * tw7r2<T, N, inverse>();

    const cvec<T, N> d1 =
        dif1 * tw7i1<T, N, inverse>() + dif2 * tw7i2<T, N, inverse>() + dif3 * tw7i3<T, N, inverse>();
    const cvec<T, N> d2 =
        dif1 * tw7i2<T, N, inverse>() - dif2 * tw7i3<T, N, inverse>() - dif3 * tw7i1<T, N, inverse>();
    const cvec<T, N> d3 =
        dif1 * tw7i3<T, N, inverse>() - dif2 * tw7i1<T, N, inverse>() + dif3 * tw7i2<T, N, inverse>();

    w01 = s1 + d1;
    w06 = s1 - d1;
    w02 = s2 + d2;
    w05 = s2 - d2;
    w03 = s3 + d3;
    w04 = s3 - d3;
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly7(cvec<T, N>& a0, cvec<T, N>& a1, cvec<T, N>& a2, cvec<T, N>& a3, cvec<T, N>& a4,
                              cvec<T, N>& a5, cvec<T, N>& a6)
{
    butterfly7<N, inverse>(a0, a1, a2, a3, a4, a5, a6, a0, a1, a2, a3, a4, a5, a6);
}

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11r1 = static_cast<T>(0.84125353283118116886181164892 - 1.0);

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11i1 =
    static_cast<T>(0.54064081745559758210763595432) * twiddleimagmask<T, N, inverse>();

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11r2 = static_cast<T>(0.41541501300188642552927414923 - 1.0);

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11i2 =
    static_cast<T>(0.90963199535451837141171538308) * twiddleimagmask<T, N, inverse>();

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11r3 = static_cast<T>(-0.14231483827328514044379266862 - 1.0);

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11i3 =
    static_cast<T>(0.98982144188093273237609203778) * twiddleimagmask<T, N, inverse>();

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11r4 = static_cast<T>(-0.65486073394528506405692507247 - 1.0);

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11i4 =
    static_cast<T>(0.75574957435425828377403584397) * twiddleimagmask<T, N, inverse>();

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11r5 = static_cast<T>(-0.95949297361449738989036805707 - 1.0);

template <typename T, size_t N, bool inverse>
static const cvec<T, N> tw11i5 =
    static_cast<T>(0.28173255684142969771141791535) * twiddleimagmask<T, N, inverse>();

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly11(cvec<T, N> a00, cvec<T, N> a01, cvec<T, N> a02, cvec<T, N> a03, cvec<T, N> a04,
                               cvec<T, N> a05, cvec<T, N> a06, cvec<T, N> a07, cvec<T, N> a08, cvec<T, N> a09,
                               cvec<T, N> a10, cvec<T, N>& w00, cvec<T, N>& w01, cvec<T, N>& w02,
                               cvec<T, N>& w03, cvec<T, N>& w04, cvec<T, N>& w05, cvec<T, N>& w06,
                               cvec<T, N>& w07, cvec<T, N>& w08, cvec<T, N>& w09, cvec<T, N>& w10)
{
    const cvec<T, N> sum1 = a01 + a10;
    const cvec<T, N> dif1 = swap<2>(a01 - a10);
    const cvec<T, N> sum2 = a02 + a09;
    const cvec<T, N> dif2 = swap<2>(a02 - a09);
    const cvec<T, N> sum3 = a03 + a08;
    const cvec<T, N> dif3 = swap<2>(a03 - a08);
    const cvec<T, N> sum4 = a04 + a07;
    const cvec<T, N> dif4 = swap<2>(a04 - a07);
    const cvec<T, N> sum5 = a05 + a06;
    const cvec<T, N> dif5 = swap<2>(a05 - a06);
    w00                   = a00 + sum1 + sum2 + sum3 + sum4 + sum5;

    const cvec<T, N> s1 = w00 + sum1 * tw11r1<T, N, inverse> + sum2 * tw11r2<T, N, inverse> +
                          sum3 * tw11r3<T, N, inverse> + sum4 * tw11r4<T, N, inverse> +
                          sum5 * tw11r5<T, N, inverse>;
    const cvec<T, N> s2 = w00 + sum1 * tw11r2<T, N, inverse> + sum2 * tw11r3<T, N, inverse> +
                          sum3 * tw11r4<T, N, inverse> + sum4 * tw11r5<T, N, inverse> +
                          sum5 * tw11r1<T, N, inverse>;
    const cvec<T, N> s3 = w00 + sum1 * tw11r3<T, N, inverse> + sum2 * tw11r4<T, N, inverse> +
                          sum3 * tw11r5<T, N, inverse> + sum4 * tw11r1<T, N, inverse> +
                          sum5 * tw11r2<T, N, inverse>;
    const cvec<T, N> s4 = w00 + sum1 * tw11r4<T, N, inverse> + sum2 * tw11r5<T, N, inverse> +
                          sum3 * tw11r1<T, N, inverse> + sum4 * tw11r2<T, N, inverse> +
                          sum5 * tw11r3<T, N, inverse>;
    const cvec<T, N> s5 = w00 + sum1 * tw11r5<T, N, inverse> + sum2 * tw11r1<T, N, inverse> +
                          sum3 * tw11r2<T, N, inverse> + sum4 * tw11r3<T, N, inverse> +
                          sum5 * tw11r4<T, N, inverse>;

    const cvec<T, N> d1 = dif1 * tw11i1<T, N, inverse> + dif2 * tw11i2<T, N, inverse> +
                          dif3 * tw11i3<T, N, inverse> + dif4 * tw11i4<T, N, inverse> +
                          dif5 * tw11i5<T, N, inverse>;
    const cvec<T, N> d2 = dif1 * tw11i2<T, N, inverse> - dif2 * tw11i3<T, N, inverse> -
                          dif3 * tw11i4<T, N, inverse> - dif4 * tw11i5<T, N, inverse> -
                          dif5 * tw11i1<T, N, inverse>;
    const cvec<T, N> d3 = dif1 * tw11i3<T, N, inverse> - dif2 * tw11i4<T, N, inverse> +
                          dif3 * tw11i5<T, N, inverse> + dif4 * tw11i1<T, N, inverse> +
                          dif5 * tw11i2<T, N, inverse>;
    const cvec<T, N> d4 = dif1 * tw11i4<T, N, inverse> - dif2 * tw11i5<T, N, inverse> +
                          dif3 * tw11i1<T, N, inverse> - dif4 * tw11i2<T, N, inverse> -
                          dif5 * tw11i3<T, N, inverse>;
    const cvec<T, N> d5 = dif1 * tw11i5<T, N, inverse> - dif2 * tw11i1<T, N, inverse> +
                          dif3 * tw11i2<T, N, inverse> - dif4 * tw11i3<T, N, inverse> +
                          dif5 * tw11i4<T, N, inverse>;

    w01 = s1 + d1;
    w10 = s1 - d1;
    w02 = s2 + d2;
    w09 = s2 - d2;
    w03 = s3 + d3;
    w08 = s3 - d3;
    w04 = s4 + d4;
    w07 = s4 - d4;
    w05 = s5 + d5;
    w06 = s5 - d5;
}

template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw5r1()
{
    return static_cast<T>(0.30901699437494742410229341718 - 1.0);
}
template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw5i1()
{
    return static_cast<T>(0.95105651629515357211643933338) * twiddleimagmask<T, N, inverse>();
}
template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw5r2()
{
    return static_cast<T>(-0.80901699437494742410229341718 - 1.0);
}
template <typename T, size_t N, bool inverse>
static constexpr KFR_INTRINSIC cvec<T, N> tw5i2()
{
    return static_cast<T>(0.58778525229247312916870595464) * twiddleimagmask<T, N, inverse>();
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly5(const cvec<T, N>& a00, const cvec<T, N>& a01, const cvec<T, N>& a02,
                              const cvec<T, N>& a03, const cvec<T, N>& a04, cvec<T, N>& w00, cvec<T, N>& w01,
                              cvec<T, N>& w02, cvec<T, N>& w03, cvec<T, N>& w04)
{
    const cvec<T, N> sum1 = a01 + a04;
    const cvec<T, N> dif1 = swap<2>(a01 - a04);
    const cvec<T, N> sum2 = a02 + a03;
    const cvec<T, N> dif2 = swap<2>(a02 - a03);
    w00                   = a00 + sum1 + sum2;

    const cvec<T, N> s1 = w00 + sum1 * tw5r1<T, N, inverse>() + sum2 * tw5r2<T, N, inverse>();
    const cvec<T, N> s2 = w00 + sum1 * tw5r2<T, N, inverse>() + sum2 * tw5r1<T, N, inverse>();

    const cvec<T, N> d1 = dif1 * tw5i1<T, N, inverse>() + dif2 * tw5i2<T, N, inverse>();
    const cvec<T, N> d2 = dif1 * tw5i2<T, N, inverse>() - dif2 * tw5i1<T, N, inverse>();

    w01 = s1 + d1;
    w04 = s1 - d1;
    w02 = s2 + d2;
    w03 = s2 - d2;
}

template <size_t N, bool inverse = false, typename T>
KFR_INTRINSIC void butterfly10(const cvec<T, N>& a0, const cvec<T, N>& a1, const cvec<T, N>& a2,
                               const cvec<T, N>& a3, const cvec<T, N>& a4, const cvec<T, N>& a5,
                               const cvec<T, N>& a6, const cvec<T, N>& a7, const cvec<T, N>& a8,
                               const cvec<T, N>& a9, cvec<T, N>& w0, cvec<T, N>& w1, cvec<T, N>& w2,
                               cvec<T, N>& w3, cvec<T, N>& w4, cvec<T, N>& w5, cvec<T, N>& w6, cvec<T, N>& w7,
                               cvec<T, N>& w8, cvec<T, N>& w9)
{
    cvec<T, N* 2> a05 = concat(a0, a5);
    cvec<T, N* 2> a27 = concat(a2, a7);
    cvec<T, N* 2> a49 = concat(a4, a9);
    cvec<T, N* 2> a61 = concat(a6, a1);
    cvec<T, N* 2> a83 = concat(a8, a3);
    butterfly5<N * 2, inverse>(a05, a27, a49, a61, a83, a05, a27, a49, a61, a83);
    cvec<T, N> t0, t1, t2, t3, t4, t5, t6, t7, t8, t9;
    split(a05, t0, t1);
    split(a27, t2, t3);
    split(a49, t4, t5);
    split(a61, t6, t7);
    split(a83, t8, t9);
    t5 = -t5;

    cvec<T, N * 2> t02, t13;
    cvec<T, N * 2> w06, w51;
    t02 = concat(t0, t2);
    t13 = concat(t1, t3);
    butterfly2<N * 2>(t02, t13, w06, w51);
    split(w06, w0, w6);
    split(w51, w5, w1);

    cvec<T, N * 2> t68, t79;
    cvec<T, N * 2> w84, w39;
    t68 = concat(t6, t8);
    t79 = concat(t7, t9);
    butterfly2<N * 2>(t68, t79, w84, w39);
    split(w84, w8, w4);
    split(w39, w3, w9);
    butterfly2<N>(t4, t5, w7, w2);
}

template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1, vec<T, N>& out0,
                             vec<T, N>& out1)
{
    butterfly2<N / 2>(in0, in1, out0, out1);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, vec<T, N>& out0, vec<T, N>& out1, vec<T, N>& out2)
{
    butterfly3<N / 2, inverse>(in0, in1, in2, out0, out1, out2);
}

template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, vec<T, N>& out0, vec<T, N>& out1,
                             vec<T, N>& out2, vec<T, N>& out3)
{
    butterfly4<N / 2, inverse>(cfalse, in0, in1, in2, in3, out0, out1, out2, out3);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             vec<T, N>& out0, vec<T, N>& out1, vec<T, N>& out2, vec<T, N>& out3,
                             vec<T, N>& out4)
{
    butterfly5<N / 2, inverse>(in0, in1, in2, in3, in4, out0, out1, out2, out3, out4);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             const vec<T, N>& in5, vec<T, N>& out0, vec<T, N>& out1, vec<T, N>& out2,
                             vec<T, N>& out3, vec<T, N>& out4, vec<T, N>& out5)
{
    butterfly6<N / 2, inverse>(in0, in1, in2, in3, in4, in5, out0, out1, out2, out3, out4, out5);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             const vec<T, N>& in5, const vec<T, N>& in6, vec<T, N>& out0, vec<T, N>& out1,
                             vec<T, N>& out2, vec<T, N>& out3, vec<T, N>& out4, vec<T, N>& out5,
                             vec<T, N>& out6)
{
    butterfly7<N / 2, inverse>(in0, in1, in2, in3, in4, in5, in6, out0, out1, out2, out3, out4, out5, out6);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             const vec<T, N>& in5, const vec<T, N>& in6, const vec<T, N>& in7,
                             vec<T, N>& out0, vec<T, N>& out1, vec<T, N>& out2, vec<T, N>& out3,
                             vec<T, N>& out4, vec<T, N>& out5, vec<T, N>& out6, vec<T, N>& out7)
{
    butterfly8<N / 2, inverse>(in0, in1, in2, in3, in4, in5, in6, in7, out0, out1, out2, out3, out4, out5,
                               out6, out7);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             const vec<T, N>& in5, const vec<T, N>& in6, const vec<T, N>& in7,
                             const vec<T, N>& in8, vec<T, N>& out0, vec<T, N>& out1, vec<T, N>& out2,
                             vec<T, N>& out3, vec<T, N>& out4, vec<T, N>& out5, vec<T, N>& out6,
                             vec<T, N>& out7, vec<T, N>& out8)
{
    butterfly9<N / 2, inverse>(in0, in1, in2, in3, in4, in5, in6, in7, in8, out0, out1, out2, out3, out4,
                               out5, out6, out7, out8);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             const vec<T, N>& in5, const vec<T, N>& in6, const vec<T, N>& in7,
                             const vec<T, N>& in8, const vec<T, N>& in9, vec<T, N>& out0, vec<T, N>& out1,
                             vec<T, N>& out2, vec<T, N>& out3, vec<T, N>& out4, vec<T, N>& out5,
                             vec<T, N>& out6, vec<T, N>& out7, vec<T, N>& out8, vec<T, N>& out9)
{
    butterfly10<N / 2, inverse>(in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, out0, out1, out2, out3,
                                out4, out5, out6, out7, out8, out9);
}
template <bool inverse, typename T, size_t N>
KFR_INTRINSIC void butterfly(cbool_t<inverse>, const vec<T, N>& in0, const vec<T, N>& in1,
                             const vec<T, N>& in2, const vec<T, N>& in3, const vec<T, N>& in4,
                             const vec<T, N>& in5, const vec<T, N>& in6, const vec<T, N>& in7,
                             const vec<T, N>& in8, const vec<T, N>& in9, const vec<T, N>& in10,
                             vec<T, N>& out0, vec<T, N>& out1, vec<T, N>& out2, vec<T, N>& out3,
                             vec<T, N>& out4, vec<T, N>& out5, vec<T, N>& out6, vec<T, N>& out7,
                             vec<T, N>& out8, vec<T, N>& out9, vec<T, N>& out10)
{
    butterfly11<N / 2, inverse>(in0, in1, in2, in3, in4, in5, in6, in7, in8, in9, in10, out0, out1, out2,
                                out3, out4, out5, out6, out7, out8, out9, out10);
}
template <bool transposed, typename T, size_t... N, size_t Nout = csum<size_t, N...>()>
KFR_INTRINSIC void cread_transposed(cbool_t<transposed>, const complex<T>* ptr, vec<T, N>&... w)
{
    vec<T, Nout> temp = read(cunaligned, csize<Nout>, ptr_cast<T>(ptr));
    if (transposed)
        temp = ctranspose<sizeof...(N)>(temp);
    split(temp, w...);
}

// Warning: Reads past the end. Use with care
KFR_INTRINSIC void cread_transposed(cbool_t<true>, const complex<f32>* ptr, cvec<f32, 4>& w0,
                                    cvec<f32, 4>& w1, cvec<f32, 4>& w2)
{
    cvec<f32, 4> w3;
    cvec<f32, 16> v16 = concat(cread<4>(ptr), cread<4>(ptr + 3), cread<4>(ptr + 6), cread<4>(ptr + 9));
    v16               = digitreverse4<2>(v16);
    split<f32, 32>(v16, w0, w1, w2, w3);
}

KFR_INTRINSIC void cread_transposed(cbool_t<true>, const complex<f32>* ptr, cvec<f32, 4>& w0,
                                    cvec<f32, 4>& w1, cvec<f32, 4>& w2, cvec<f32, 4>& w3, cvec<f32, 4>& w4)
{
    cvec<f32, 16> v16 = concat(cread<4>(ptr), cread<4>(ptr + 5), cread<4>(ptr + 10), cread<4>(ptr + 15));
    v16               = digitreverse4<2>(v16);
    split<f32, 32>(v16, w0, w1, w2, w3);
    w4 = cgather<4, 5>(ptr + 4);
}

template <bool transposed, typename T, size_t... N, size_t Nout = csum<size_t, N...>()>
KFR_INTRINSIC void cwrite_transposed(cbool_t<transposed>, complex<T>* ptr, vec<T, N>... args)
{
    auto temp = concat(args...);
    if (transposed)
        temp = ctransposeinverse<sizeof...(N)>(temp);
    write(ptr_cast<T>(ptr), temp);
}

template <size_t I, size_t radix, typename T, size_t N, size_t width = N / 2>
KFR_INTRINSIC vec<T, N> mul_tw(cbool_t<false>, const vec<T, N>& x, const complex<T>* twiddle)
{
    return I == 0 ? x : cmul(x, cread<width>(twiddle + width * (I - 1)));
}
template <size_t I, size_t radix, typename T, size_t N, size_t width = N / 2>
KFR_INTRINSIC vec<T, N> mul_tw(cbool_t<true>, const vec<T, N>& x, const complex<T>* twiddle)
{
    return I == 0 ? x : cmul_conj(x, cread<width>(twiddle + width * (I - 1)));
}

// Non-final
template <typename T, size_t width, size_t radix, bool inverse, size_t... I>
KFR_INTRINSIC void butterfly_helper(csizes_t<I...>, size_t i, csize_t<width>, csize_t<radix>,
                                    cbool_t<inverse>, complex<T>* out, const complex<T>* in,
                                    const complex<T>* tw, size_t stride)
{
    carray<cvec<T, width>, radix> inout;

    swallow{ (inout.get(csize_t<I>()) = cread<width>(in + i + stride * I))... };

    butterfly(cbool_t<inverse>(), inout.template get<I>()..., inout.template get<I>()...);

    swallow{ (
        cwrite<width>(out + i + stride * I,
                      mul_tw<I, radix>(cbool_t<inverse>(), inout.template get<I>(), tw + i * (radix - 1))),
        0)... };
}

// Final
template <typename T, size_t width, size_t radix, bool inverse, size_t... I>
KFR_INTRINSIC void butterfly_helper(csizes_t<I...>, size_t i, csize_t<width>, csize_t<radix>,
                                    cbool_t<inverse>, complex<T>* out, const complex<T>* in, size_t stride)
{
    carray<cvec<T, width>, radix> inout;

    //        swallow{ ( inout.get( csize<I> ) = infn( i, I, cvec<T, width>( ) ) )... };
    cread_transposed(ctrue, in + i * radix, inout.template get<I>()...);

    butterfly(cbool_t<inverse>(), inout.template get<I>()..., inout.template get<I>()...);

    swallow{ (cwrite<width>(out + i + stride * I, inout.get(csize_t<I>())), 0)... };
}

template <size_t width, size_t radix, typename... Args>
KFR_INTRINSIC void butterfly(size_t i, csize_t<width>, csize_t<radix>, Args&&... args)
{
    butterfly_helper(csizeseq_t<radix>(), i, csize_t<width>(), csize_t<radix>(), std::forward<Args>(args)...);
}

template <typename... Args>
KFR_INTRINSIC void butterfly_cycle(size_t&, size_t, csize_t<0>, Args&&...)
{
}
template <size_t width, typename... Args>
KFR_INTRINSIC void butterfly_cycle(size_t& i, size_t count, csize_t<width>, Args&&... args)
{
    CMT_LOOP_NOUNROLL
    for (; i < count / width * width; i += width)
        butterfly(i, csize_t<width>(), std::forward<Args>(args)...);
    butterfly_cycle(i, count, csize_t<width / 2>(), std::forward<Args>(args)...);
}

template <size_t width, typename... Args>
KFR_INTRINSIC void butterflies(size_t count, csize_t<width>, Args&&... args)
{
    CMT_ASSUME(count > 0);
    size_t i = 0;
    butterfly_cycle(i, count, csize_t<width>(), std::forward<Args>(args)...);
}

template <typename T, bool inverse, typename Tradix, typename Tstride>
KFR_INTRINSIC void generic_butterfly_cycle(csize_t<0>, Tradix, cbool_t<inverse>, complex<T>*,
                                           const complex<T>*, Tstride, size_t, size_t, const complex<T>*,
                                           size_t)
{
}

template <size_t width, bool inverse, typename T, typename Tradix, typename Thalfradix,
          typename Thalfradixsqr, typename Tstride>
KFR_INTRINSIC void generic_butterfly_cycle(csize_t<width>, Tradix radix, cbool_t<inverse>, complex<T>* out,
                                           const complex<T>* in, Tstride ostride, Thalfradix halfradix,
                                           Thalfradixsqr halfradix_sqr, const complex<T>* twiddle, size_t i)
{
    CMT_LOOP_NOUNROLL
    for (; i < halfradix / width * width; i += width)
    {
        const cvec<T, 1> in0 = cread<1>(in);
        cvec<T, width> sum0  = resize<2 * width>(in0);
        cvec<T, width> sum1  = sum0;

        for (size_t j = 0; j < halfradix; j++)
        {
            const cvec<T, 1> ina = cread<1>(in + (1 + j));
            const cvec<T, 1> inb = cread<1>(in + radix - (j + 1));
            cvec<T, width> tw    = cread<width>(twiddle);
            if (inverse)
                tw = negodd /*cconj*/ (tw);

            cmul_2conj(sum0, sum1, ina, inb, tw);
            twiddle += halfradix;
        }
        twiddle = twiddle - halfradix_sqr + width;

        //        if (inverse)
        //            std::swap(sum0, sum1);

        if (is_constant_val(ostride))
        {
            cwrite<width>(out + (1 + i), sum0);
            cwrite<width>(out + (radix - (i + 1)) - (width - 1), reverse<2>(sum1));
        }
        else
        {
            cscatter<width>(out + (i + 1) * ostride, ostride, sum0);
            cscatter<width>(out + (radix - (i + 1)) * ostride - (width - 1) * ostride, ostride,
                            reverse<2>(sum1));
        }
    }
    generic_butterfly_cycle(csize_t<width / 2>(), radix, cbool_t<inverse>(), out, in, ostride, halfradix,
                            halfradix_sqr, twiddle, i);
}

template <typename T>
KFR_INTRINSIC vec<T, 2> hcadd(vec<T, 2> value)
{
    return value;
}
template <typename T, size_t N, KFR_ENABLE_IF(N >= 4)>
KFR_INTRINSIC vec<T, 2> hcadd(vec<T, N> value)
{
    return hcadd(low(value) + high(value));
}

template <size_t width, typename T, bool inverse, typename Tstride = csize_t<1>>
KFR_INTRINSIC void generic_butterfly_w(size_t radix, cbool_t<inverse>, complex<T>* out, const complex<T>* in,
                                       const complex<T>* twiddle, Tstride ostride = Tstride{})
{
    CMT_ASSUME(radix > 0);
    {
        cvec<T, width> sum = T();
        size_t j           = 0;
        CMT_LOOP_NOUNROLL
        for (; j < radix / width * width; j += width)
        {
            sum += cread<width>(in + j);
        }
        cvec<T, 1> sums = T();
        CMT_LOOP_NOUNROLL
        for (; j < radix; j++)
        {
            sums += cread<1>(in + j);
        }
        cwrite<1>(out, hcadd(sum) + sums);
    }
    const auto halfradix = radix / 2;
    CMT_ASSUME(halfradix > 0);
    size_t i = 0;

    generic_butterfly_cycle(csize_t<width>(), radix, cbool_t<inverse>(), out, in, ostride, halfradix,
                            halfradix * halfradix, twiddle, i);
}

template <size_t width, size_t radix, typename T, bool inverse, typename Tstride = csize_t<1>>
KFR_INTRINSIC void spec_generic_butterfly_w(csize_t<radix>, cbool_t<inverse>, complex<T>* out,
                                            const complex<T>* in, const complex<T>* twiddle,
                                            Tstride ostride = Tstride{})
{
    {
        cvec<T, width> sum = T();
        size_t j           = 0;
        CMT_LOOP_UNROLL
        for (; j < radix / width * width; j += width)
        {
            sum += cread<width>(in + j);
        }
        cvec<T, 1> sums = T();
        CMT_LOOP_UNROLL
        for (; j < radix; j++)
        {
            sums += cread<1>(in + j);
        }
        cwrite<1>(out, hcadd(sum) + sums);
    }
    const size_t halfradix     = radix / 2;
    const size_t halfradix_sqr = halfradix * halfradix;
    CMT_ASSUME(halfradix > 0);
    size_t i = 0;

    generic_butterfly_cycle(csize_t<width>(), radix, cbool_t<inverse>(), out, in, ostride, halfradix,
                            halfradix_sqr, twiddle, i);
}

template <typename T, bool inverse, typename Tstride = csize_t<1>>
KFR_INTRINSIC void generic_butterfly(size_t radix, cbool_t<inverse>, complex<T>* out, const complex<T>* in,
                                     complex<T>*, const complex<T>* twiddle, Tstride ostride = {})
{
    cswitch(
        csizes_t<11, 13>(), radix,
        [&](auto radix_) CMT_INLINE_LAMBDA
        {
            constexpr size_t width = vector_width<T>;
            spec_generic_butterfly_w<width>(radix_, cbool_t<inverse>(), out, in, twiddle, ostride);
        },
        [&]() CMT_INLINE_LAMBDA
        {
            constexpr size_t width = vector_width<T>;
            generic_butterfly_w<width>(radix, cbool_t<inverse>(), out, in, twiddle, ostride);
        });
}

template <typename T, size_t N>
constexpr cvec<T, N> cmask08 = broadcast<N * 2, T>(T(), -T());

template <typename T, size_t N>
constexpr cvec<T, N> cmask0088 = broadcast<N * 4, T>(T(), T(), -T(), -T());

template <bool A = false, typename T, size_t N>
KFR_INTRINSIC void cbitreverse_write(complex<T>* dest, const vec<T, N>& x)
{
    cwrite<N / 2, A>(dest, bitreverse<2>(x));
}

template <bool A = false, typename T, size_t N>
KFR_INTRINSIC void cdigitreverse4_write(complex<T>* dest, const vec<T, N>& x)
{
    cwrite<N / 2, A>(dest, digitreverse4<2>(x));
}

template <size_t N, bool A = false, typename T>
KFR_INTRINSIC cvec<T, N> cbitreverse_read(const complex<T>* src)
{
    return bitreverse<2>(cread<N, A>(src));
}

template <size_t N, bool A = false, typename T>
KFR_INTRINSIC cvec<T, N> cdigitreverse4_read(const complex<T>* src)
{
    return digitreverse4<2>(cread<N, A>(src));
}

#if 1

template <>
KFR_INTRINSIC cvec<f64, 16> cdigitreverse4_read<16, false, f64>(const complex<f64>* src)
{
    return concat(cread<1>(src + 0), cread<1>(src + 4), cread<1>(src + 8), cread<1>(src + 12),
                  cread<1>(src + 1), cread<1>(src + 5), cread<1>(src + 9), cread<1>(src + 13),
                  cread<1>(src + 2), cread<1>(src + 6), cread<1>(src + 10), cread<1>(src + 14),
                  cread<1>(src + 3), cread<1>(src + 7), cread<1>(src + 11), cread<1>(src + 15));
}
template <>
KFR_INTRINSIC void cdigitreverse4_write<false, f64, 32>(complex<f64>* dest, const vec<f64, 32>& x)
{
    cwrite<1>(dest, part<16, 0>(x));
    cwrite<1>(dest + 4, part<16, 1>(x));
    cwrite<1>(dest + 8, part<16, 2>(x));
    cwrite<1>(dest + 12, part<16, 3>(x));

    cwrite<1>(dest + 1, part<16, 4>(x));
    cwrite<1>(dest + 5, part<16, 5>(x));
    cwrite<1>(dest + 9, part<16, 6>(x));
    cwrite<1>(dest + 13, part<16, 7>(x));

    cwrite<1>(dest + 2, part<16, 8>(x));
    cwrite<1>(dest + 6, part<16, 9>(x));
    cwrite<1>(dest + 10, part<16, 10>(x));
    cwrite<1>(dest + 14, part<16, 11>(x));

    cwrite<1>(dest + 3, part<16, 12>(x));
    cwrite<1>(dest + 7, part<16, 13>(x));
    cwrite<1>(dest + 11, part<16, 14>(x));
    cwrite<1>(dest + 15, part<16, 15>(x));
}
#endif
} // namespace intrinsics
} // namespace CMT_ARCH_NAME
} // namespace kfr

CMT_PRAGMA_MSVC(warning(pop))

CMT_PRAGMA_GNU(GCC diagnostic pop)
