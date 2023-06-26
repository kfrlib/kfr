/** @addtogroup shuffle
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
#include "constants.hpp"
#include "mask.hpp"
#include "types.hpp"
#include "vec.hpp"

#include <tuple>
#include <utility>

CMT_PRAGMA_MSVC(warning(push))
CMT_PRAGMA_MSVC(warning(disable : 5051))

namespace kfr
{

inline namespace CMT_ARCH_NAME
{

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec<T, Nout> low(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout>);
}

template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec_shape<T, Nout> low(vec_shape<T, N>)
{
    return {};
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec<T, Nout> high(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout, prev_poweroftwo(N - 1)>);
}

template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec_shape<T, Nout> high(vec_shape<T, N>)
{
    return {};
}

template <typename T, size_t... Ns>
KFR_INTRINSIC vec<T, csum<size_t, Ns...>()> concat(const vec<T, Ns>&... vs) CMT_NOEXCEPT
{
    return vec<T, csum<size_t, Ns...>()>(
        intrinsics::simd_concat<typename vec<T, 1>::scalar_type, vec<T, Ns>::scalar_size()...>(vs.v...));
}

template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, N1 + N2> concat2(const vec<T, N1>& x, const vec<T, N2>& y) CMT_NOEXCEPT
{
    return vec<T, csum<size_t, N1, N2>()>(
        intrinsics::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N1>::scalar_size(),
                                vec<T, N2>::scalar_size()>(x.v, y.v));
}

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N * 4> concat4(const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c,
                                    const vec<T, N>& d) CMT_NOEXCEPT
{
    return intrinsics::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N * 2>::scalar_size(),
                                   vec<T, N * 2>::scalar_size()>(
        intrinsics::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N>::scalar_size(),
                                vec<T, N>::scalar_size()>(a.v, b.v),
        intrinsics::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N>::scalar_size(),
                                vec<T, N>::scalar_size()>(c.v, d.v));
}

template <size_t count, typename T, size_t N, size_t Nout = N* count>
KFR_INTRINSIC vec<T, Nout> repeat(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout> % csize<N>);
}

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout != N)>
KFR_INTRINSIC vec<T, Nout> resize(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout> % csize<N>);
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout == N)>
constexpr KFR_INTRINSIC const vec<T, Nout>& resize(const vec<T, N>& x)
{
    return x;
}

namespace intrinsics
{

template <typename T, typename... Ts, size_t... indices, size_t Nin = sizeof...(Ts),
          size_t Nout = sizeof...(indices)>
KFR_INTRINSIC vec<T, Nout> broadcast_helper(csizes_t<indices...>, const Ts&... values)
{
    const std::tuple<Ts...> tup(values...);
    return vec<T, Nout>(std::get<indices % Nin>(tup)...);
}
} // namespace intrinsics

template <size_t Nout, typename... Ts, typename C = typename std::common_type<Ts...>::type>
KFR_INTRINSIC vec<C, Nout> broadcast(const Ts&... values)
{
    return intrinsics::broadcast_helper<C>(csizeseq<Nout>, values...);
}
KFR_FN(broadcast)

template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padhigh(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<N + Ncount>);
}
template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padhigh(const vec<T, N>& x, identity<T> newvalue)
{
    if constexpr (Ncount == 0)
        return x;
    else
        return concat(x, broadcast<Ncount, T>(newvalue));
}
KFR_FN(padhigh)

template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padlow(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<N + Ncount, 0 - Ncount>);
}
template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padlow(const vec<T, N>& x, identity<T> newvalue)
{
    if constexpr (Ncount == 0)
        return x;
    else
        return concat(broadcast<Ncount, T>(newvalue), x);
}
KFR_FN(padlow)

template <size_t Nout, typename T>
KFR_INTRINSIC vec<T, Nout> extend(const vec<T, 1>& x)
{
    return vec<T, Nout>(x.front());
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(N != Nout && N > 1)>
KFR_INTRINSIC vec<T, Nout> extend(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout>);
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(N == Nout && N > 1)>
constexpr KFR_INTRINSIC const vec<T, Nout>& extend(const vec<T, N>& x)
{
    return x;
}
KFR_FN(extend)

template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, count> slice(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<count, start>);
}
template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, count> slice(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq<count, start>);
}
KFR_FN(slice)

template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, N> replace(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq<N> + (csizeseq<N> >= csize<start> && csizeseq<N> < csize<start + count>)*N);
}
KFR_FN(replace)

template <size_t, typename T, size_t N>
KFR_INTRINSIC void split(const vec<T, N>&)
{
}
template <size_t start = 0, typename T, size_t N, size_t Nout, typename... Args>
KFR_INTRINSIC void split(const vec<T, N>& x, vec<T, Nout>& out, Args&&... args)
{
    out = x.shuffle(csizeseq<Nout, start>);
    split<start + Nout>(x, std::forward<Args>(args)...);
}
template <typename T, size_t N>
KFR_INTRINSIC void split(const vec<T, N>& x, vec<T, N / 2>& low, vec<T, N / 2>& high)
{
    low  = x.shuffle(csizeseq<N / 2, 0>);
    high = x.shuffle(csizeseq<N / 2, N / 2>);
}
template <typename T, size_t N>
KFR_INTRINSIC void split(const vec<T, N>& x, vec<T, N / 4>& w0, vec<T, N / 4>& w1, vec<T, N / 4>& w2,
                         vec<T, N / 4>& w3)
{
    w0 = x.shuffle(csizeseq<N / 4, 0>);
    w1 = x.shuffle(csizeseq<N / 4, N / 4>);
    w2 = x.shuffle(csizeseq<N / 4, 2 * N / 4>);
    w3 = x.shuffle(csizeseq<N / 4, 3 * N / 4>);
}
KFR_FN(split)

template <size_t total, size_t number, typename T, size_t N, size_t Nout = N / total>
KFR_INTRINSIC vec<T, Nout> part(const vec<T, N>& x)
{
    static_assert(N % total == 0, "N % total == 0");
    return x.shuffle(csizeseq<Nout, number * Nout>);
}
KFR_FN(part)

template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, count> concat_and_slice(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq<count, start>);
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2, KFR_ENABLE_IF(N1 > N2)>
KFR_INTRINSIC vec<T, count> concat_and_slice(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(y.shuffle(csizeseq<N1>), csizeseq<N1 * 2>).shuffle(csizeseq<count, start>);
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2, KFR_ENABLE_IF(N1 < N2)>
KFR_INTRINSIC vec<T, count> concat_and_slice(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(csizeseq<N2, -(N2 - N1)>)
        .shuffle(y, csizeseq<N2 * 2>)
        .shuffle(csizeseq<count, N2 - N1 + start>);
}

KFR_FN(concat_and_slice)

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout > N)>
KFR_INTRINSIC vec<T, Nout> widen(const vec<T, N>& x, identity<T> newvalue = T())
{
    static_assert(Nout > N, "Nout > N");
    return concat(x, broadcast<Nout - N>(newvalue));
}
template <size_t Nout, typename T, typename TS>
constexpr KFR_INTRINSIC const vec<T, Nout>& widen(const vec<T, Nout>& x, TS)
{
    return x;
}
KFR_FN(widen)

template <size_t Nout, typename T, size_t N>
KFR_INTRINSIC vec<T, Nout> narrow(const vec<T, N>& x)
{
    static_assert(Nout <= N, "Nout <= N");
    return slice<0, Nout>(x);
}
KFR_FN(narrow)

template <size_t group = 1, typename T, size_t N, size_t Nout = N / 2, KFR_ENABLE_IF(N >= 2 && (N & 1) == 0)>
KFR_INTRINSIC vec<T, Nout> even(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq<Nout / group, 0, 2>));
}
KFR_FN(even)

template <size_t group = 1, typename T, size_t N, size_t Nout = N / 2, KFR_ENABLE_IF(N >= 2 && (N & 1) == 0)>
KFR_INTRINSIC vec<T, Nout> odd(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq<Nout / group, 1, 2>));
}
KFR_FN(odd)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> dupeven(const vec<T, N>& x)
{
    static_assert(N % 2 == 0, "N must be even");
    return x.shuffle(csizeseq<N, 0, 1> & ~csize<1>);
}
KFR_FN(dupeven)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> dupodd(const vec<T, N>& x)
{
    static_assert(N % 2 == 0, "N must be even");
    return x.shuffle(csizeseq<N, 0, 1> | csize<1>);
}
KFR_FN(dupodd)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N * 2> duphalves(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<N * 2> % csize<N>);
}
KFR_FN(duphalves)

template <size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> shuffle(const vec<T, N>& x, const vec<T, N>& y,
                                elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, i[csizeseq_t<N>() % csize_t<sizeof...(Indices)>()] +
                            csizeseq_t<N>() / csize_t<count>() * csize_t<count>());
}
KFR_FN(shuffle)

template <size_t group, size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> shufflegroups(const vec<T, N>& x, const vec<T, N>& y,
                                      elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, scale<group>(i[csizeseq_t<N / group>() % csize_t<sizeof...(Indices)>()] +
                                     csizeseq_t<N / group>() / csize_t<count>() * csize_t<count>()));
}
KFR_FN(shufflegroups)

template <size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> permute(const vec<T, N>& x, elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(i[csizeseq_t<N>() % csize_t<count>()] +
                     csizeseq_t<N>() / csize_t<count>() * csize_t<count>());
}
KFR_FN(permute)

template <size_t group, size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> permutegroups(const vec<T, N>& x, elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(scale<group>(i[csizeseq_t<N / group>() % csize_t<sizeof...(Indices)>()] +
                                  csizeseq_t<N / group>() / csize_t<count>() * csize_t<count>()));
}
KFR_FN(permutegroups)

namespace internal
{

template <typename T, size_t Nout, typename Fn, size_t... Indices>
constexpr KFR_INTRINSIC vec<T, Nout> generate_vector(csizes_t<Indices...>)
{
    return make_vector<T>(static_cast<T>(Fn()(Indices))...);
}
} // namespace internal

template <typename T, size_t Nout, typename Fn>
constexpr KFR_INTRINSIC vec<T, Nout> generate_vector()
{
    return internal::generate_vector<T, Nout, Fn>(cvalseq_t<size_t, Nout>());
}
KFR_FN(generate_vector)

namespace internal
{
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> evenmask()
{
    return broadcast<N>(maskbits<T>(true), maskbits<T>(false));
}
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> oddmask()
{
    return broadcast<N>(maskbits<T>(false), maskbits<T>(true));
}
} // namespace internal

template <typename T, size_t N, size_t Nout = N * 2>
KFR_INTRINSIC vec<T, Nout> dup(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>() / csize_t<2>());
}
KFR_FN(dup)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> duplow(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() % csize_t<N / 2>());
}
KFR_FN(duplow)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> duphigh(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() % csize_t<N / 2>() + csize_t<N - N / 2>());
}
KFR_FN(duphigh)

template <size_t... Indices, typename T, size_t N>
KFR_INTRINSIC vec<T, N> blend(const vec<T, N>& x, const vec<T, N>& y,
                              elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, i[csizeseq_t<N>() % csize_t<sizeof...(Indices)>()] * csize_t<N>() + csizeseq_t<N>());
}
KFR_FN(blend)

template <size_t elements = 2, typename T, size_t N>
KFR_INTRINSIC vec<T, N> swap(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() ^ csize_t<elements - 1>());
}
CMT_FN_TPL((size_t elements), (elements), swap)

template <size_t shift, typename T, size_t N>
KFR_INTRINSIC vec<T, N> rotatetwo(const vec<T, N>& lo, const vec<T, N>& hi)
{
    return shift == 0 ? lo : (shift == N ? hi : hi.shuffle(lo, csizeseq_t<N, N - shift>()));
}

template <size_t amount, typename T, size_t N>
KFR_INTRINSIC vec<T, N> rotateright(const vec<T, N>& x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return x.shuffle(csizeseq_t<N, N - amount>() % csize_t<N>());
}
KFR_FN(rotateright)

template <size_t amount, typename T, size_t N>
KFR_INTRINSIC vec<T, N> rotateleft(const vec<T, N>& x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return x.shuffle(csizeseq_t<N, amount>() % csize_t<N>());
}
KFR_FN(rotateleft)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> insertright(T x, const vec<T, N>& y)
{
    return concat_and_slice<1, N>(y, vec<T, 1>(x));
}
KFR_FN(insertright)

template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> insertleft(T x, const vec<T, N>& y)
{
    return concat_and_slice<0, N>(vec<T, 1>(x), y);
}
KFR_FN(insertleft)

template <size_t side1, size_t group = 1, typename T, size_t N, size_t size = N / group,
          size_t side2 = size / side1, KFR_ENABLE_IF(size > 3)>
KFR_INTRINSIC vec<T, N> transpose(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
template <size_t side, size_t group = 1, typename T, size_t N, KFR_ENABLE_IF(N / group <= 3)>
KFR_INTRINSIC vec<T, N> transpose(const vec<T, N>& x)
{
    return x;
}
template <typename T, size_t N>
KFR_INTRINSIC vec<vec<T, N>, N> transpose(const vec<vec<T, N>, N>& x)
{
    return vec<vec<T, N>, N>::from_flatten(transpose<2>(x.flatten()));
}
KFR_FN(transpose)

template <size_t side2, size_t group = 1, typename T, size_t N, size_t size = N / group,
          size_t side1 = size / side2, KFR_ENABLE_IF(size > 3)>
KFR_INTRINSIC vec<T, N> transposeinverse(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
template <size_t side, size_t groupsize = 1, typename T, size_t N, KFR_ENABLE_IF(N / groupsize <= 3)>
KFR_INTRINSIC vec<T, N> transposeinverse(const vec<T, N>& x)
{
    return x;
}
KFR_FN(transposeinverse)

template <size_t side, typename T, size_t N>
KFR_INTRINSIC vec<T, N> ctranspose(const vec<T, N>& x)
{
    return transpose<side, 2>(x);
}
KFR_FN(ctranspose)

template <size_t side, typename T, size_t N>
KFR_INTRINSIC vec<T, N> ctransposeinverse(const vec<T, N>& x)
{
    return transposeinverse<side, 2>(x);
}
KFR_FN(ctransposeinverse)

template <size_t group = 1, typename T, size_t N, size_t Nout = N * 2, size_t size = Nout / group,
          size_t side2 = 2, size_t side1 = size / side2>
KFR_INTRINSIC vec<T, Nout> interleave(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                     csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(interleave)

template <typename T, size_t N1, size_t... Ns, size_t size = N1 + csum<size_t, Ns...>(),
          size_t side2 = 1 + sizeof...(Ns), size_t side1 = size / side2>
KFR_INTRINSIC vec<vec<T, side2>, side1> zip(const vec<T, N1>& x, const vec<T, Ns>&... y)
{
    static_assert(is_poweroftwo(1 + sizeof...(Ns)), "number of vectors must be power of two");
    return vec<vec<T, side2>, side1>::from_flatten(concat(x, y...).shuffle(scale<1>(
        csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() + csizeseq_t<size>() / csize_t<side2>())));
}
KFR_FN(zip)

template <size_t index, typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, N2> column(const vec<vec<T, N1>, N2>& x)
{
    static_assert(index < N1, "column index must be less than inner vector length");
    return x.flatten().shuffle(csizeseq_t<N2>() * csize_t<N1>() + csize_t<index>());
}

template <size_t group = 1, typename T, size_t N, size_t size = N / group, size_t side2 = 2,
          size_t side1 = size / side2>
KFR_INTRINSIC vec<T, N> interleavehalves(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(interleavehalves)

template <size_t group = 1, typename T, size_t N, size_t size = N / group, size_t side1 = 2,
          size_t side2 = size / side1>
KFR_INTRINSIC vec<T, N> splitpairs(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(splitpairs)

template <size_t group = 1, typename T, size_t N, KFR_ENABLE_IF(!is_vec<T>)>
KFR_INTRINSIC vec<T, N> reverse(const vec<T, N>& x)
{
    constexpr size_t size = N / group;
    return x.shuffle(scale<group>(csizeseq_t<size, size - 1, -1>()));
}
template <size_t group = 1, typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<vec<T, N1>, N2> reverse(const vec<vec<T, N1>, N2>& x)
{
    return swap<N1>(x.flatten()).v;
}
KFR_FN(reverse)

template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, N1> combine(const vec<T, N1>& x, const vec<T, N2>& y)
{
    static_assert(N2 <= N1, "N2 <= N1");
    return x.shuffle(extend<N1>(y), (csizeseq_t<N1>() < csize_t<N2>()) * csize_t<N1>() + csizeseq_t<N1>());
}
KFR_FN(combine)

namespace internal
{
template <size_t start, size_t stride>
struct generate_index
{
    KFR_INTRINSIC constexpr size_t operator()(size_t index) const { return start + index * stride; }
};
template <size_t start, size_t size, int on, int off>
struct generate_onoff
{
    KFR_INTRINSIC constexpr size_t operator()(size_t index) const
    {
        return index >= start && index < start + size ? on : off;
    }
};
} // namespace internal

template <typename T, size_t N, size_t start = 0, size_t stride = 1>
constexpr KFR_INTRINSIC vec<T, N> enumerate()
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
template <size_t start = 0, size_t stride = 1, typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> enumerate(vec_shape<T, N>)
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> enumerate(vec_shape<T, N> sh, identity<T> step)
{
    if constexpr (N == 1)
    {
        return czeros;
    }
    else if constexpr (!is_poweroftwo(N))
    {
        return slice<0, N>(enumerate(vec_shape<T, next_poweroftwo(N)>{}, step));
    }
    else
    {
        vec<T, N> vv = step;
        vec<T, N> zz(czeros);

        vec<T, N> acc = blend(zz, vv, csizeseq<N> % csize<2>);
        cfor(csize<0>, csize<ilog2(N) - 1>,
             [&](auto idx) CMT_INLINE_LAMBDA
             {
                 vv = vv + vv;
                 acc += blend(zz, vv, csizeseq<N> / (csize<2 << (idx)>) % csize<2>);
             });
        return acc;
    }
}

KFR_FN(enumerate)

template <typename T, size_t N, size_t start = 0, size_t size = 1, int on = 1, int off = 0>
constexpr KFR_INTRINSIC vec<T, N> onoff(cint_t<on> = cint_t<on>(), cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
template <size_t start = 0, size_t size = 1, int on = 1, int off = 0, typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> onoff(vec_shape<T, N>, cint_t<on> = cint_t<on>(),
                                        cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
KFR_FN(onoff)

} // namespace CMT_ARCH_NAME
} // namespace kfr
#define KFR_SHUFFLE_SPECIALIZATIONS 1
#include "impl/specializations.hpp"

CMT_PRAGMA_MSVC(warning(pop))
