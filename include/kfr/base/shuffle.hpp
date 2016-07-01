/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
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

template <size_t index, typename T>
constexpr KFR_INLINE T broadcast_get_nth()
{
    return c_qnan<T>;
}

template <size_t index, typename T, typename... Ts>
constexpr KFR_INLINE T broadcast_get_nth(T x, Ts... rest)
{
    return index == 0 ? x : broadcast_get_nth<index - 1, T>(rest...);
}

template <typename T, typename... Ts, size_t... indices, size_t Nin = 1 + sizeof...(Ts),
          size_t Nout = sizeof...(indices)>
KFR_INLINE constexpr vec<T, Nout> broadcast_helper(csizes_t<indices...>, T x, Ts... rest)
{
    simd<T, Nout> result{ broadcast_get_nth<indices % Nin>(x, rest...)... };
    return result;
}
}

template <size_t Nout, typename T, typename... Ts>
constexpr KFR_INLINE vec<T, Nout> broadcast(T x, T y, Ts... rest)
{
    return internal::broadcast_helper(csizeseq<Nout>, x, y, rest...);
}
KFR_FN(broadcast)

template <size_t Ncount, typename T, size_t N>
KFR_INLINE vec<T, N + Ncount> padhigh(vec<T, N> x)
{
    return shufflevector<N + Ncount, internal::shuffle_index_extend<0, N>>(x);
}
KFR_FN(padhigh)

template <size_t Ncount, typename T, size_t N>
KFR_INLINE vec<T, N + Ncount> padlow(vec<T, N> x)
{
    return shufflevector<N + Ncount, internal::shuffle_index_extend<Ncount, N>>(x);
}
KFR_FN(padlow)

template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(N != Nout)>
KFR_INLINE vec<T, Nout> extend(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index_extend<0, N>>(x);
}
template <size_t Nout, typename T, size_t N, KFR_ENABLE_IF(N == Nout)>
constexpr KFR_INLINE vec<T, Nout> extend(vec<T, N> x)
{
    return x;
}
KFR_FN(extend)

template <size_t start, size_t count, typename T, size_t N>
KFR_INLINE vec<T, count> slice(vec<T, N> x)
{
    static_assert(start + count <= N, "start + count <= N");
    return shufflevector<count, internal::shuffle_index<start>>(x);
}
template <size_t start, size_t count, typename T, size_t N>
KFR_INLINE vec<T, count> slice(vec<T, N> x, vec<T, N> y)
{
    static_assert(start + count <= N * 2, "start + count <= N * 2");
    return shufflevector<count, internal::shuffle_index<start>>(x, y);
}
KFR_FN(slice)

template <size_t, typename T, size_t N>
KFR_INLINE void split(vec<T, N>)
{
}
template <size_t start = 0, typename T, size_t N, size_t Nout, typename... Args>
KFR_INLINE void split(vec<T, N> x, vec<T, Nout>& out, Args&&... args)
{
    out = slice<start, Nout>(x);
    split<start + Nout>(x, std::forward<Args>(args)...);
}
KFR_FN(split)

template <size_t total, size_t number, typename T, size_t N, size_t Nout = N / total>
KFR_INLINE vec<T, Nout> part(vec<T, N> x)
{
    static_assert(N % total == 0, "N % total == 0");
    return shufflevector<Nout, internal::shuffle_index<number * Nout>>(x);
}
KFR_FN(part)

template <size_t start, size_t count, typename T, size_t N1, size_t N2>
KFR_INLINE vec<T, count> concat_and_slice(vec<T, N1> x, vec<T, N2> y)
{
    return internal::concattwo<start, count>(x, y);
}
KFR_FN(concat_and_slice)

template <size_t Nout, typename T, size_t N>
KFR_INLINE vec<T, Nout> widen(vec<T, N> x, identity<T> newvalue = T())
{
    static_assert(Nout > N, "Nout > N");
    return concat(x, broadcast<Nout - N>(newvalue));
}
template <size_t Nout, typename T, typename TS>
constexpr KFR_INLINE vec<T, Nout> widen(vec<T, Nout> x, TS)
{
    return x;
}
KFR_FN(widen)

template <size_t Nout, typename T, size_t N>
KFR_INLINE vec<T, Nout> narrow(vec<T, N> x)
{
    static_assert(Nout <= N, "Nout <= N");
    return slice<0, Nout>(x);
}
KFR_FN(narrow)

template <size_t groupsize = 1, typename T, size_t N, size_t Nout = N / 2,
          KFR_ENABLE_IF(N >= 2 && (N & 1) == 0)>
KFR_INLINE vec<T, Nout> even(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index<0, 2>, groupsize>(x);
}
KFR_FNR(even, 2, 1)

template <size_t groupsize = 1, typename T, size_t N, size_t Nout = N / 2,
          KFR_ENABLE_IF(N >= 2 && (N & 1) == 0)>
KFR_INLINE vec<T, Nout> odd(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index<1, 2>, groupsize>(x);
}
KFR_FNR(odd, 2, 1)

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
KFR_INLINE vec<T, N> dupeven(vec<T, N> x)
{
    static_assert(N % 2 == 0, "N must be even");
    return shufflevector<N, internal::shuffle_index_dup<2, 0>>(x);
}
KFR_FN(dupeven)

template <typename T, size_t N>
KFR_INLINE vec<T, N> dupodd(vec<T, N> x)
{
    static_assert(N % 2 == 0, "N must be even");
    return shufflevector<N, internal::shuffle_index_dup<2, 1>>(x);
}
KFR_FN(dupodd)

template <typename T, size_t N>
KFR_INLINE vec<T, N * 2> duphalfs(vec<T, N> x)
{
    return concat(x, x);
}
KFR_FN(duphalfs)

namespace internal
{
template <size_t size, size_t... Indices>
struct shuffle_index_shuffle
{
    constexpr static size_t indexcount = sizeof...(Indices);

    template <size_t index>
    constexpr inline size_t operator()() const
    {
        constexpr int result = csizes_t<Indices...>::get(csize<index % indexcount>);
        return result + index / indexcount * indexcount;
    }
};
}

template <size_t... Indices, typename T, size_t N>
KFR_INLINE vec<T, N> shuffle(vec<T, N> x, vec<T, N> y, elements_t<Indices...> = elements_t<Indices...>())
{
    return shufflevector<N, internal::shuffle_index_shuffle<N, Indices...>>(x, y);
}
KFR_FN(shuffle)

template <size_t groupsize, size_t... Indices, typename T, size_t N>
KFR_INLINE vec<T, N> shufflegroups(vec<T, N> x, vec<T, N> y,
                                   elements_t<Indices...> = elements_t<Indices...>())
{
    return shufflevector<N, internal::shuffle_index_shuffle<N, Indices...>, groupsize>(x, y);
}
KFR_FN(shufflegroups)

namespace internal
{
template <size_t size, size_t... Indices>
struct shuffle_index_permute
{
    constexpr static size_t indexcount = sizeof...(Indices);

    template <size_t index>
    constexpr inline size_t operator()() const
    {
        constexpr size_t result = csizes_t<Indices...>::get(csize<index % indexcount>);
        static_assert(result < size, "result < size");
        return result + index / indexcount * indexcount;
    }
};
}

template <size_t... Indices, typename T, size_t N>
KFR_INLINE vec<T, N> permute(vec<T, N> x, elements_t<Indices...> = elements_t<Indices...>())
{
    return shufflevector<N, internal::shuffle_index_permute<N, Indices...>>(x);
}
KFR_FN(permute)

template <size_t groupsize, size_t... Indices, typename T, size_t N>
KFR_INLINE vec<T, N> permutegroups(vec<T, N> x, elements_t<Indices...> = elements_t<Indices...>())
{
    return shufflevector<N, internal::shuffle_index_permute<N, Indices...>, groupsize>(x);
}
KFR_FN(permutegroups)

namespace internal
{

template <typename T, size_t Nout, typename Fn, size_t... Indices>
constexpr KFR_INLINE vec<T, Nout> generate_vector(csizes_t<Indices...>)
{
    constexpr Fn fn{};
    return make_vector(static_cast<T>(fn(Indices))...);
}
}

template <typename T, size_t Nout, typename Fn>
constexpr KFR_INLINE vec<T, Nout> generate_vector()
{
    return internal::generate_vector<T, Nout, Fn>(csizeseq<Nout>);
}
KFR_FN(generate_vector)

namespace internal
{
template <typename T, size_t N, typename = u8[N > 1]>
constexpr KFR_INLINE mask<T, N> evenmask()
{
    return broadcast<N, T>(maskbits<T>(true), maskbits<T>(false));
}
template <typename T, size_t N, typename = u8[N > 1]>
constexpr KFR_INLINE mask<T, N> oddmask()
{
    return broadcast<N, T>(maskbits<T>(false), maskbits<T>(true));
}
}

template <typename T, size_t N, size_t Nout = N * 2>
KFR_INLINE vec<T, Nout> dup(vec<T, N> x)
{
    return shufflevector<Nout, internal::shuffle_index_dup1<2>>(x, x);
}
KFR_FNR(dup, 1, 2)

namespace internal
{
template <size_t count, size_t start = 0>
struct shuffle_index_duphalf
{
    constexpr inline size_t operator()(size_t index) const { return start + (index) % count; }
};
}

template <typename T, size_t N>
KFR_INLINE vec<T, N> duplow(vec<T, N> x)
{
    static_assert(N % 2 == 0, "N must be even");
    return shufflevector<N, internal::shuffle_index_duphalf<N / 2, 0>>(x);
}
KFR_FN(duplow)

template <typename T, size_t N>
KFR_INLINE vec<T, N> duphigh(vec<T, N> x)
{
    static_assert(N % 2 == 0, "N must be even");
    return shufflevector<N, internal::shuffle_index_duphalf<N / 2, N / 2>>(x);
}
KFR_FN(duphigh)

namespace internal
{
template <size_t size, size_t... Indices>
struct shuffle_index_blend
{
    constexpr static size_t indexcount = sizeof...(Indices);

    template <size_t index>
    constexpr inline size_t operator()() const
    {
        return (elements_t<Indices...>::get(csize<index % indexcount>) ? size : 0) + index % size;
    }
};
}

template <size_t... Indices, typename T, size_t N>
KFR_INLINE vec<T, N> blend(vec<T, N> x, vec<T, N> y, elements_t<Indices...> = elements_t<Indices...>())
{
    return shufflevector<N, internal::shuffle_index_blend<N, Indices...>, 1>(x, y);
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

template <size_t elements, typename T, size_t N>
KFR_INLINE vec<T, N> swap(vec<T, N> x)
{
    return shufflevector<N, internal::shuffle_index_swap<elements>>(x);
}
KFR_FN(swap)

template <size_t shift, typename T, size_t N>
KFR_INLINE vec<T, N> rotatetwo(vec<T, N> lo, vec<T, N> hi)
{
    return shift == 0 ? lo : (shift == N ? hi : shufflevector<N, internal::shuffle_index<N - shift>>(hi, lo));
}

template <size_t amount, typename T, size_t N>
KFR_INLINE vec<T, N> rotateright(vec<T, N> x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return shufflevector<N, internal::shuffle_index_wrap<N, N - amount>>(x);
}
KFR_FN(rotateright)

template <size_t amount, typename T, size_t N>
KFR_INLINE vec<T, N> rotateleft(vec<T, N> x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return shufflevector<N, internal::shuffle_index_wrap<N, amount>>(x);
}
KFR_FN(rotateleft)

template <typename T, size_t N>
KFR_INLINE vec<T, N> insertright(T x, vec<T, N> y)
{
    return concat_and_slice<1, N>(y, vec<T, 1>(x));
}
KFR_FN(insertright)

template <typename T, size_t N>
KFR_INLINE vec<T, N> insertleft(T x, vec<T, N> y)
{
    return concat_and_slice<0, N>(vec<T, 1>(x), y);
}
KFR_FN(insertleft)

template <typename T, size_t N, size_t N2>
KFR_INLINE vec<T, N> outputright(vec<T, N> x, vec<T, N2> y)
{
    return shufflevector<N, internal::shuffle_index_outputright<N2, N>>(x, extend<N>(y));
}
KFR_FN(outputright)

namespace internal
{
template <size_t size, size_t side1>
struct shuffle_index_transpose
{
    constexpr inline size_t operator()(size_t index) const
    {
        constexpr size_t side2 = size / side1;
        return index % side2 * side1 + index / side2;
    }
};
}

template <size_t side, size_t groupsize = 1, typename T, size_t N, KFR_ENABLE_IF(N / groupsize > 3)>
KFR_INLINE vec<T, N> transpose(vec<T, N> x)
{
    return shufflevector<N, internal::shuffle_index_transpose<N / groupsize, side>, groupsize>(x);
}
template <size_t side, size_t groupsize = 1, typename T, size_t N, KFR_ENABLE_IF(N / groupsize <= 3)>
KFR_INLINE vec<T, N> transpose(vec<T, N> x)
{
    return x;
}
KFR_FN(transpose)

template <size_t side, size_t groupsize = 1, typename T, size_t N, KFR_ENABLE_IF(N / groupsize > 3)>
KFR_INLINE vec<T, N> transposeinverse(vec<T, N> x)
{
    return shufflevector<N, internal::shuffle_index_transpose<N / groupsize, N / groupsize / side>,
                         groupsize>(x);
}
template <size_t side, size_t groupsize = 1, typename T, size_t N, KFR_ENABLE_IF(N / groupsize <= 3)>
KFR_INLINE vec<T, N> transposeinverse(vec<T, N> x)
{
    return x;
}
KFR_FN(transposeinverse)

template <size_t side, typename T, size_t N>
KFR_INLINE vec<T, N> ctranspose(vec<T, N> x)
{
    return transpose<side, 2>(x);
}
KFR_FN(ctranspose)

template <size_t side, typename T, size_t N>
KFR_INLINE vec<T, N> ctransposeinverse(vec<T, N> x)
{
    return transposeinverse<side, 2>(x);
}
KFR_FN(ctransposeinverse)

template <size_t groupsize = 1, typename T, size_t N, size_t Nout = N * 2>
KFR_INLINE vec<T, Nout> interleave(vec<T, N> x, vec<T, N> y)
{
    return shufflevector<Nout, internal::shuffle_index_transpose<Nout / groupsize, Nout / groupsize / 2>,
                         groupsize>(x, y);
}
KFR_FNR(interleave, 1, 2)

template <typename E1, typename E2, KFR_ENABLE_IF(is_input_expressions<E1, E2>::value)>
KFR_INLINE expr_func<fn_interleave, E1, E2> interleave(E1&& x, E2&& y)
{
    return { fn_interleave(), std::forward<E1>(x), std::forward<E2>(y) };
}

template <size_t groupsize = 1, typename T, size_t N>
KFR_INLINE vec<T, N> interleavehalfs(vec<T, N> x)
{
    return shufflevector<N, internal::shuffle_index_transpose<N / groupsize, N / groupsize / 2>, groupsize>(
        x);
}
KFR_FN(interleavehalfs)

template <size_t groupsize = 1, typename T, size_t N>
KFR_INLINE vec<T, N> splitpairs(vec<T, N> x)
{
    return shufflevector<N, internal::shuffle_index_transpose<N / groupsize, 2>, groupsize>(x);
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

template <size_t groupsize = 1, typename T, size_t N>
KFR_INLINE vec<T, N> reverse(vec<T, N> x)
{
    return shufflevector<N, internal::shuffle_index_reverse<N / groupsize>, groupsize>(x);
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
KFR_INLINE vec<T, N1> combine(vec<T, N1> x, vec<T, N2> y)
{
    static_assert(N2 <= N1, "N2 <= N1");
    return shufflevector<N1, internal::shuffle_index_combine<N1, N2>>(x, extend<N1>(y));
}
KFR_FN(combine)

namespace internal
{
template <size_t start, size_t stride>
struct generate_index
{
    constexpr size_t operator()(size_t index) const { return start + index * stride; }
};
template <size_t start, size_t size, int on, int off>
struct generate_onoff
{
    constexpr size_t operator()(size_t index) const
    {
        return index >= start && index < start + size ? on : off;
    }
};
}

template <typename T, size_t N, size_t start = 0, size_t stride = 1>
constexpr KFR_INLINE vec<T, N> enumerate()
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
template <size_t start = 0, size_t stride = 1, typename T, size_t N>
constexpr KFR_INLINE vec<T, N> enumerate(vec_t<T, N>)
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
KFR_FN(enumerate)

template <typename T, size_t N, size_t start = 0, size_t size = 1, int on = 1, int off = 0>
constexpr KFR_INLINE vec<T, N> onoff(cint_t<on> = cint_t<on>(), cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
template <size_t start = 0, size_t size = 1, int on = 1, int off = 0, typename T, size_t N>
constexpr KFR_INLINE vec<T, N> onoff(vec_t<T, N>, cint_t<on> = cint_t<on>(), cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
KFR_FN(onoff)
}
#define KFR_SHUFFLE_SPECIALIZATIONS
#include "specializations.i"
