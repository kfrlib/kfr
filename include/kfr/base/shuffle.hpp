/** @addtogroup shuffle
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
#include "constants.hpp"
#include "expression.hpp"
#include "types.hpp"
#include "vec.hpp"

#include <utility>

namespace kfr
{

namespace internal
{

template <typename T, typename... Ts, size_t... indices, size_t Nin = sizeof...(Ts),
          size_t Nout = sizeof...(indices)>
CMT_GNU_CONSTEXPR CMT_INLINE vec<T, Nout> broadcast_helper(csizes_t<indices...>, const Ts&... values)
{
    const std::tuple<Ts...> tup(values...);
    return vec<T, Nout>(std::get<indices % Nin>(tup)...);
}
}

template <size_t Nout, typename... Ts, typename C = typename std::common_type<Ts...>::type>
CMT_GNU_CONSTEXPR CMT_INLINE vec<C, Nout> broadcast(const Ts&... values)
{
    return internal::broadcast_helper<C>(csizeseq_t<Nout>(), values...);
}
KFR_FN(broadcast)

template <size_t Ncount, typename T, size_t N>
CMT_INLINE vec<T, N + Ncount> padhigh(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N + Ncount>());
}
KFR_FN(padhigh)

template <size_t Ncount, typename T, size_t N>
CMT_INLINE vec<T, N + Ncount> padlow(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N + Ncount, 0 - Ncount>());
}
KFR_FN(padlow)

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(N != Nout)>
CMT_INLINE vec<T, Nout> extend(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>());
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(N == Nout)>
constexpr CMT_INLINE vec<T, Nout> extend(const vec<T, N>& x)
{
    return x;
}
KFR_FN(extend)

template <size_t start, size_t count, typename T, size_t N>
CMT_INLINE vec<T, count> slice(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<count, start>());
}
template <size_t start, size_t count, typename T, size_t N>
CMT_INLINE vec<T, count> slice(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq_t<count, start>());
}
KFR_FN(slice)

template <size_t start, size_t count, typename T, size_t N>
CMT_INLINE vec<T, N> replace(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(
        y, csizeseq_t<N>() +
               (csizeseq_t<N>() >= csize_t<start>() && csizeseq_t<N>() < csize_t<start + count>()) * N);
}
KFR_FN(replace)

template <size_t, typename T, size_t N>
CMT_INLINE void split(const vec<T, N>&)
{
}
template <size_t start = 0, typename T, size_t N, size_t Nout, typename... Args>
CMT_INLINE void split(const vec<T, N>& x, vec<T, Nout>& out, Args&&... args)
{
    out = x.shuffle(csizeseq_t<Nout, start>());
    split<start + Nout>(x, std::forward<Args>(args)...);
}
template <typename T, size_t N>
CMT_INLINE void split(const vec<T, N>& x, vec<T, N / 2>& low, vec<T, N / 2>& high)
{
    low  = x.shuffle(csizeseq_t<N / 2, 0>());
    high = x.shuffle(csizeseq_t<N / 2, N / 2>());
}
template <typename T, size_t N>
CMT_INLINE void split(const vec<T, N>& x, vec<T, N / 4>& w0, vec<T, N / 4>& w1, vec<T, N / 4>& w2,
                      vec<T, N / 4>& w3)
{
    w0 = x.shuffle(csizeseq_t<N / 4, 0>());
    w1 = x.shuffle(csizeseq_t<N / 4, N / 4>());
    w2 = x.shuffle(csizeseq_t<N / 4, 2 * N / 4>());
    w3 = x.shuffle(csizeseq_t<N / 4, 3 * N / 4>());
}
KFR_FN(split)

template <size_t total, size_t number, typename T, size_t N, size_t Nout = N / total>
CMT_INLINE vec<T, Nout> part(const vec<T, N>& x)
{
    static_assert(N % total == 0, "N % total == 0");
    return x.shuffle(csizeseq_t<Nout, number * Nout>());
}
KFR_FN(part)

template <size_t start, size_t count, typename T, size_t N>
CMT_INLINE vec<T, count> concat_and_slice(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq_t<count, start>());
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2, KFR_ENABLE_IF(N1 > N2)>
CMT_INLINE vec<T, count> concat_and_slice(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(y.shuffle(csizeseq_t<N1>()), csizeseq_t<N1 * 2>()).shuffle(csizeseq_t<count, start>());
}

template <size_t start, size_t count, typename T, size_t N1, size_t N2, KFR_ENABLE_IF(N1 < N2)>
CMT_INLINE vec<T, count> concat_and_slice(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(csizeseq_t<N2, -(N2 - N1)>())
        .shuffle(y, csizeseq_t<N2 * 2>())
        .shuffle(csizeseq_t<count, N2 - N1 + start>());
}

KFR_FN(concat_and_slice)

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(Nout > N)>
CMT_INLINE vec<T, Nout> widen(const vec<T, N>& x, identity<T> newvalue = T())
{
    static_assert(Nout > N, "Nout > N");
    return concat(x, broadcast<Nout - N>(newvalue));
}
template <size_t Nout, typename T, typename TS>
constexpr CMT_INLINE vec<T, Nout> widen(const vec<T, Nout>& x, TS)
{
    return x;
}
KFR_FN(widen)

template <size_t Nout, typename T, size_t N>
CMT_INLINE vec<T, Nout> narrow(const vec<T, N>& x)
{
    static_assert(Nout <= N, "Nout <= N");
    return slice<0, Nout>(x);
}
KFR_FN(narrow)

template <size_t group = 1, typename T, size_t N, size_t Nout = N / 2, KFR_ENABLE_IF(N >= 2 && (N & 1) == 0)>
CMT_INLINE vec<T, Nout> even(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<Nout / group, 0, 2>()));
}
KFR_FN(even)

template <size_t group = 1, typename T, size_t N, size_t Nout = N / 2, KFR_ENABLE_IF(N >= 2 && (N & 1) == 0)>
CMT_INLINE vec<T, Nout> odd(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<Nout / group, 1, 2>()));
}
KFR_FN(odd)

namespace internal
{
template <size_t groupsize = 2>
struct shuffle_index_dup1
{
    constexpr inline size_t operator()(size_t index) const { return index / groupsize; }
};

template <size_t groupsize = 2, size_t start = 0>
struct shuffle_index_dup
{
    constexpr inline size_t operator()(size_t index) const { return start + index / groupsize * groupsize; }
};
}

template <typename T, size_t N>
CMT_INLINE vec<T, N> dupeven(const vec<T, N>& x)
{
    static_assert(N % 2 == 0, "N must be even");
    return x.shuffle(csizeseq_t<N, 0, 1>() & ~csize_t<1>());
}
KFR_FN(dupeven)

template <typename T, size_t N>
CMT_INLINE vec<T, N> dupodd(const vec<T, N>& x)
{
    static_assert(N % 2 == 0, "N must be even");
    return x.shuffle(csizeseq_t<N, 0, 1>() | csize_t<1>());
}
KFR_FN(dupodd)

template <typename T, size_t N>
CMT_INLINE vec<T, N * 2> duphalfs(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N * 2>() % csize_t<N>());
}
KFR_FN(duphalfs)

namespace internal
{
template <size_t size, size_t... Indices>
struct shuffle_index_shuffle
{
    constexpr static size_t indexcount = sizeof...(Indices);

    template <size_t index>
    constexpr inline size_t operator()(csize_t<index>) const
    {
        return csizes_t<Indices...>::get(csize_t<index % indexcount>()) + index / indexcount * indexcount;
    }
};
}

template <size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
CMT_INLINE vec<T, N> shuffle(const vec<T, N>& x, const vec<T, N>& y,
                             elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, i[csizeseq_t<N>() % csize_t<sizeof...(Indices)>()] +
                            csizeseq_t<N>() / csize_t<count>() * csize_t<count>());
}
KFR_FN(shuffle)

template <size_t group, size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
CMT_INLINE vec<T, N> shufflegroups(const vec<T, N>& x, const vec<T, N>& y,
                                   elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, scale<group>(i[csizeseq_t<N / group>() % csize_t<sizeof...(Indices)>()] +
                                     csizeseq_t<N / group>() / csize_t<count>() * csize_t<count>()));
}
KFR_FN(shufflegroups)

namespace internal
{
template <size_t size, size_t... Indices>
struct shuffle_index_permute
{
    constexpr static size_t indexcount = sizeof...(Indices);

    template <size_t index>
    constexpr inline size_t operator()(csize_t<index>) const
    {
        return csizes_t<Indices...>::get(csize_t<index % indexcount>()) + index / indexcount * indexcount;
    }
};
}

template <size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
CMT_INLINE vec<T, N> permute(const vec<T, N>& x, elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(i[csizeseq_t<N>() % csize_t<count>()] +
                     csizeseq_t<N>() / csize_t<count>() * csize_t<count>());
}
KFR_FN(permute)

template <size_t group, size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
CMT_INLINE vec<T, N> permutegroups(const vec<T, N>& x, elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(scale<group>(i[csizeseq_t<N / group>() % csize_t<sizeof...(Indices)>()] +
                                  csizeseq_t<N / group>() / csize_t<count>() * csize_t<count>()));
}
KFR_FN(permutegroups)

namespace internal
{

template <typename T, size_t Nout, typename Fn, size_t... Indices>
constexpr CMT_INLINE vec<T, Nout> generate_vector(csizes_t<Indices...>)
{
    return make_vector(static_cast<T>(Fn()(Indices))...);
}
}

template <typename T, size_t Nout, typename Fn>
constexpr CMT_INLINE vec<T, Nout> generate_vector()
{
    return internal::generate_vector<T, Nout, Fn>(cvalseq_t<size_t, Nout>());
}
KFR_FN(generate_vector)

namespace internal
{
template <typename T, size_t N>
constexpr CMT_INLINE mask<T, N> evenmask()
{
    return broadcast<N, T>(maskbits<T>(true), maskbits<T>(false));
}
template <typename T, size_t N>
constexpr CMT_INLINE mask<T, N> oddmask()
{
    return broadcast<N, T>(maskbits<T>(false), maskbits<T>(true));
}
}

template <typename T, size_t N, size_t Nout = N * 2>
CMT_INLINE vec<T, Nout> dup(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>() / csize_t<2>());
}
KFR_FN(dup)

namespace internal
{
template <size_t count, size_t start = 0>
struct shuffle_index_duphalf
{
    constexpr inline size_t operator()(size_t index) const { return start + (index) % count; }
};
}

template <typename T, size_t N>
CMT_INLINE vec<T, N> duplow(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() % csize_t<N / 2>());
}
KFR_FN(duplow)

template <typename T, size_t N>
CMT_INLINE vec<T, N> duphigh(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() % csize_t<N / 2>() + csize_t<N - N / 2>());
}
KFR_FN(duphigh)

namespace internal
{
template <size_t size, size_t... Indices>
struct shuffle_index_blend
{
    constexpr static size_t indexcount = sizeof...(Indices);

    template <size_t index>
    constexpr inline size_t operator()(csize_t<index>) const
    {
        return (elements_t<Indices...>::get(csize_t<index % indexcount>()) ? size : 0) + index % size;
    }
};
}

template <size_t... Indices, typename T, size_t N>
CMT_INLINE vec<T, N> blend(const vec<T, N>& x, const vec<T, N>& y,
                           elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, i[csizeseq_t<N>() % csize_t<sizeof...(Indices)>()] * csize_t<N>() + csizeseq_t<N>());
}
KFR_FN(blend)

namespace internal
{
template <size_t elements>
struct shuffle_index_swap
{
    constexpr inline size_t operator()(size_t index) const
    {
        static_assert(is_poweroftwo(elements), "is_poweroftwo( elements )");
        return index ^ (elements - 1);
    }
};
template <size_t amount, size_t N>
struct shuffle_index_outputright
{
    constexpr inline size_t operator()(size_t index) const
    {
        return index < N - amount ? index : index + amount;
    }
};
}

template <size_t elements = 2, typename T, size_t N>
CMT_INLINE vec<T, N> swap(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() ^ csize_t<elements - 1>());
}
CMT_FN_TPL((size_t elements), (elements), swap)

template <size_t shift, typename T, size_t N>
CMT_INLINE vec<T, N> rotatetwo(const vec<T, N>& lo, const vec<T, N>& hi)
{
    return shift == 0 ? lo : (shift == N ? hi : hi.shuffle(lo, csizeseq_t<N, N - shift>()));
}

template <size_t amount, typename T, size_t N>
CMT_INLINE vec<T, N> rotateright(const vec<T, N>& x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return x.shuffle(csizeseq_t<N, N - amount>() % csize_t<N>());
}
KFR_FN(rotateright)

template <size_t amount, typename T, size_t N>
CMT_INLINE vec<T, N> rotateleft(const vec<T, N>& x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return x.shuffle(csizeseq_t<N, amount>() % csize_t<N>());
}
KFR_FN(rotateleft)

template <typename T, size_t N>
CMT_INLINE vec<T, N> insertright(T x, const vec<T, N>& y)
{
    return concat_and_slice<1, N>(y, vec<T, 1>(x));
}
KFR_FN(insertright)

template <typename T, size_t N>
CMT_INLINE vec<T, N> insertleft(T x, const vec<T, N>& y)
{
    return concat_and_slice<0, N>(vec<T, 1>(x), y);
}
KFR_FN(insertleft)

// template <typename T, size_t N, size_t N2>
// CMT_INLINE vec<T, N> outputright(const vec<T, N>& x, const vec<T, N2>& y)
//{
//    return shufflevector<N, internal::shuffle_index_outputright<N2, N>>(x, extend<N>(y));
//}
// KFR_FN(outputright)

namespace internal
{
template <size_t size, size_t side1>
struct shuffle_index_transpose
{
    constexpr inline size_t operator()(size_t index) const
    {
        return index % (size / side1) * side1 + index / (size / side1);
    }
};
}

template <size_t side1, size_t group = 1, typename T, size_t N, size_t size = N / group,
          size_t side2 = size / side1, KFR_ENABLE_IF(size > 3)>
CMT_INLINE vec<T, N> transpose(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
template <size_t side, size_t group = 1, typename T, size_t N, KFR_ENABLE_IF(N / group <= 3)>
CMT_INLINE vec<T, N> transpose(const vec<T, N>& x)
{
    return x;
}
template <typename T, size_t N>
CMT_INLINE vec<vec<T, N>, N> transpose(const vec<vec<T, N>, N>& x)
{
    return vec<vec<T, N>, N>(transpose<2>(x.flatten()));
}
KFR_FN(transpose)

template <size_t side2, size_t group = 1, typename T, size_t N, size_t size = N / group,
          size_t side1 = size / side2, KFR_ENABLE_IF(size > 3)>
CMT_INLINE vec<T, N> transposeinverse(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
template <size_t side, size_t groupsize = 1, typename T, size_t N, KFR_ENABLE_IF(N / groupsize <= 3)>
CMT_INLINE vec<T, N> transposeinverse(const vec<T, N>& x)
{
    return x;
}
KFR_FN(transposeinverse)

template <size_t side, typename T, size_t N>
CMT_INLINE vec<T, N> ctranspose(const vec<T, N>& x)
{
    return transpose<side, 2>(x);
}
KFR_FN(ctranspose)

template <size_t side, typename T, size_t N>
CMT_INLINE vec<T, N> ctransposeinverse(const vec<T, N>& x)
{
    return transposeinverse<side, 2>(x);
}
KFR_FN(ctransposeinverse)

template <size_t group = 1, typename T, size_t N, size_t Nout = N * 2, size_t size = Nout / group,
          size_t side2 = 2, size_t side1 = size / side2>
CMT_INLINE vec<T, Nout> interleave(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                     csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(interleave)

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
CMT_INLINE internal::expression_function<fn::interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return { fn::interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}

template <size_t group = 1, typename T, size_t N, size_t size = N / group, size_t side2 = 2,
          size_t side1 = size / side2>
CMT_INLINE vec<T, N> interleavehalfs(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(interleavehalfs)

template <size_t group = 1, typename T, size_t N, size_t size = N / group, size_t side1 = 2,
          size_t side2 = size / side1>
CMT_INLINE vec<T, N> splitpairs(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(splitpairs)

namespace internal
{
template <size_t size>
struct shuffle_index_reverse
{
    constexpr inline size_t operator()(size_t index) const { return size - 1 - index; }
};
}

template <size_t group = 1, typename T, size_t N, size_t size = N / group>
CMT_INLINE vec<T, N> reverse(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size, size - 1, -1>()));
}
template <typename T, size_t N1, size_t N2>
CMT_INLINE vec<vec<T, N1>, N2> reverse(const vec<vec<T, N1>, N2>& x)
{
    return vec<vec<T, N1>, N2>(swap<N1>(x.flatten()));
}
KFR_FN(reverse)

namespace internal
{
template <size_t N1, size_t N2>
struct shuffle_index_combine
{
    constexpr inline size_t operator()(size_t index) const { return index >= N2 ? index : N1 + index; }
};
}

template <typename T, size_t N1, size_t N2>
CMT_INLINE vec<T, N1> combine(const vec<T, N1>& x, const vec<T, N2>& y)
{
    static_assert(N2 <= N1, "N2 <= N1");
    return x.shuffle(extend<N1>(y), (csizeseq_t<N1>() < csize_t<N2>()) * csize_t<N1>() + csizeseq_t<N1>());
    //    return shufflevector<N1, internal::shuffle_index_combine<N1, N2>>(x, extend<N1>(y));
}
KFR_FN(combine)

namespace internal
{
template <size_t start, size_t stride>
struct generate_index
{
    CMT_INLINE constexpr size_t operator()(size_t index) const { return start + index * stride; }
};
template <size_t start, size_t size, int on, int off>
struct generate_onoff
{
    CMT_INLINE constexpr size_t operator()(size_t index) const
    {
        return index >= start && index < start + size ? on : off;
    }
};
}

template <typename T, size_t N, size_t start = 0, size_t stride = 1>
constexpr CMT_INLINE vec<T, N> enumerate()
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
template <size_t start = 0, size_t stride = 1, typename T, size_t N>
constexpr CMT_INLINE vec<T, N> enumerate(vec_t<T, N>)
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
KFR_FN(enumerate)

template <typename T, size_t N, size_t start = 0, size_t size = 1, int on = 1, int off = 0>
constexpr CMT_INLINE vec<T, N> onoff(cint_t<on> = cint_t<on>(), cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
template <size_t start = 0, size_t size = 1, int on = 1, int off = 0, typename T, size_t N>
constexpr CMT_INLINE vec<T, N> onoff(vec_t<T, N>, cint_t<on> = cint_t<on>(), cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
KFR_FN(onoff)
}
#define KFR_SHUFFLE_SPECIALIZATIONS 1
#include "specializations.i"
