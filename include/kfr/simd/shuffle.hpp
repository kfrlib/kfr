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
#include "constants.hpp"
#include "mask.hpp"
#include "types.hpp"
#include "vec.hpp"
#include "bitshuffle.hpp"

#include <tuple>
#include <utility>

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 5051))
KFR_PRAGMA_MSVC(warning(disable : 4244))

namespace kfr
{

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Extracts the largest power-of-two-sized low subvector of @p x.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Output length, defaults to the largest power of two strictly less than @p N.
 * @param x Source vector.
 * @return A vector holding the first @p Nout elements of @p x.
 */
template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec<T, Nout> low(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout>);
}

/**
 * @brief Shape-only overload of low() for type deduction.
 * @related low
 */
template <typename T, size_t N, size_t Nout = prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec_shape<T, Nout> low(vec_shape<T, N>)
{
    return {};
}

/**
 * @brief Extracts the lower half of @p x.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Output length, defaults to @p N / 2.
 * @param x Source vector.
 * @return A vector holding the first @p Nout elements of @p x.
 */
template <typename T, size_t N, size_t Nout = N / 2>
KFR_INTRINSIC vec<T, Nout> lowhalf(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout>);
}

/**
 * @brief Shape-only overload of lowhalf() for type deduction.
 * @related lowhalf
 */
template <typename T, size_t N, size_t Nout = N / 2>
KFR_INTRINSIC vec_shape<T, Nout> lowhalf(vec_shape<T, N>)
{
    return {};
}

/**
 * @brief Extracts the high subvector of @p x with non-power-of-two-aware sizing.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Output length, defaults to @p N - prev_poweroftwo(N - 1).
 * @param x Source vector.
 * @return A vector holding the last @p Nout elements of @p x.
 */
template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec<T, Nout> high(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout, prev_poweroftwo(N - 1)>);
}

/**
 * @brief Shape-only overload of high() for type deduction.
 * @related high
 */
template <typename T, size_t N, size_t Nout = N - prev_poweroftwo(N - 1)>
KFR_INTRINSIC vec_shape<T, Nout> high(vec_shape<T, N>)
{
    return {};
}

/**
 * @brief Extracts the upper half of @p x.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Output length, defaults to @p N - N / 2.
 * @param x Source vector.
 * @return A vector holding the last @p Nout elements of @p x.
 */
template <typename T, size_t N, size_t Nout = N - N / 2>
KFR_INTRINSIC vec<T, Nout> highhalf(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout, N / 2>);
}

/**
 * @brief Shape-only overload of highhalf() for type deduction.
 * @related highhalf
 */
template <typename T, size_t N, size_t Nout = N - N / 2>
KFR_INTRINSIC vec_shape<T, Nout> highhalf(vec_shape<T, N>)
{
    return {};
}

/**
 * @brief Concatenates any number of vectors into a single vector.
 * @tparam T Element type of the vectors.
 * @tparam Ns Lengths of the input vectors.
 * @param vs Vectors to concatenate, in left-to-right order.
 * @return A vector of length `csum(Ns...)` containing all input elements.
 */
template <typename T, size_t... Ns>
KFR_INTRINSIC vec<T, csum<size_t, Ns...>()> concat(const vec<T, Ns>&... vs) noexcept
{
    return vec<T, csum<size_t, Ns...>()>(
        intr::simd_concat<typename vec<T, 1>::scalar_type, vec<T, Ns>::scalar_size()...>(vs.v...));
}

/**
 * @brief Concatenates two vectors into a single vector.
 * @tparam T Element type of the vectors.
 * @tparam N1 Length of the first vector.
 * @tparam N2 Length of the second vector.
 * @param x First vector.
 * @param y Second vector.
 * @return A vector of length @p N1 + @p N2 containing @p x followed by @p y.
 */
template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, N1 + N2> concat2(const vec<T, N1>& x, const vec<T, N2>& y) noexcept
{
    return vec<T, csum<size_t, N1, N2>()>(
        intr::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N1>::scalar_size(),
                          vec<T, N2>::scalar_size()>(x.v, y.v));
}

/**
 * @brief Concatenates four equal-length vectors into a single vector.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @param a First vector.
 * @param b Second vector.
 * @param c Third vector.
 * @param d Fourth vector.
 * @return A vector of length @p N * 4 containing @p a, @p b, @p c, @p d in order.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N * 4> concat4(const vec<T, N>& a, const vec<T, N>& b, const vec<T, N>& c,
                                    const vec<T, N>& d) noexcept
{
    return intr::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N * 2>::scalar_size(),
                             vec<T, N * 2>::scalar_size()>(
        intr::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N>::scalar_size(),
                          vec<T, N>::scalar_size()>(a.v, b.v),
        intr::simd_concat<typename vec<T, 1>::scalar_type, vec<T, N>::scalar_size(),
                          vec<T, N>::scalar_size()>(c.v, d.v));
}

/**
 * @brief Repeats the contents of @p x @p count times.
 * @tparam count Number of repetitions.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Output length, defaults to @p N * @p count.
 * @param x Source vector.
 * @return A vector of length @p Nout where the pattern of @p x is repeated @p count times.
 */
template <size_t count, typename T, size_t N, size_t Nout = N * count>
KFR_INTRINSIC vec<T, Nout> repeat(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout> % csize<N>);
}

/**
 * @brief Resizes a vector to length @p Nout.
 * @details When @p Nout > @p N the pattern of @p x is repeated cyclically; when @p Nout < @p N the
 *          vector is truncated. The @p Nout == @p N case is handled by a separate overload that
 *          returns the input unchanged.
 * @tparam Nout Desired output length.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p Nout.
 */
template <size_t Nout, typename T, size_t N>
    requires(Nout != N)
KFR_INTRINSIC vec<T, Nout> resize(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout> % csize<N>);
}

/**
 * @brief Identity overload of resize() for the @p Nout == @p N case.
 * @related resize
 */
template <size_t Nout, typename T, size_t N>
    requires(Nout == N)
constexpr KFR_INTRINSIC const vec<T, Nout>& resize(const vec<T, N>& x)
{
    return x;
}

namespace intr
{

/**
 * @brief Internal helper that broadcasts a pack of scalars into a vector using index mapping.
 * @tparam T Element type of the resulting vector.
 * @tparam Ts Input scalar types.
 * @tparam indices Compile-time index sequence selecting which input value feeds each output lane.
 * @tparam Nin Number of input values.
 * @tparam Nout Output vector length.
 * @param values Scalar values to broadcast from.
 * @return A vector of length @p Nout whose lanes are drawn from @p values via the index pattern.
 */
template <typename T, typename... Ts, size_t... indices, size_t Nin = sizeof...(Ts),
          size_t Nout = sizeof...(indices)>
KFR_INTRINSIC vec<T, Nout> broadcast_helper(csizes_t<indices...>, const Ts&... values)
{
    const std::tuple<Ts...> tup(values...);
    return vec<T, Nout>(std::get<indices % Nin>(tup)...);
}
} // namespace intr

/**
 * @brief Broadcasts a pack of scalar values into a vector of length @p Nout.
 * @details The supplied values are repeated cyclically to fill all @p Nout lanes. The common type
 *          of all @p values is used as the element type.
 * @tparam Nout Output vector length.
 * @tparam Ts Input scalar types.
 * @tparam C Common type of @p Ts, used as the element type.
 * @param values Scalar values to broadcast from.
 * @return A vector of length @p Nout filled by cycling through @p values.
 */
template <size_t Nout, typename... Ts, typename C = typename std::common_type<Ts...>::type>
KFR_INTRINSIC vec<C, Nout> broadcast(const Ts&... values)
{
    return intr::broadcast_helper<C>(csizeseq<Nout>, values...);
}
KFR_FN(broadcast)

/**
 * @brief Pads @p x with @p Ncount zero-initialized lanes on the high side.
 * @tparam Ncount Number of lanes to append.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N + @p Ncount with the appended lanes zeroed.
 */
template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padhigh(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<N + Ncount>);
}
/**
 * @brief Pads @p x with @p Ncount copies of @p newvalue on the high side.
 * @tparam Ncount Number of lanes to append.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @param newvalue Value to fill the appended lanes with.
 * @return A vector of length @p N + @p Ncount with the appended lanes set to @p newvalue.
 */
template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padhigh(const vec<T, N>& x, std::type_identity_t<T> newvalue)
{
    if constexpr (Ncount == 0)
        return x;
    else
        return concat(x, broadcast<Ncount, T>(newvalue));
}
KFR_FN(padhigh)

/**
 * @brief Pads @p x with @p Ncount zero-initialized lanes on the low side.
 * @tparam Ncount Number of lanes to prepend.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N + @p Ncount with the prepended lanes zeroed.
 */
template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padlow(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<N + Ncount, 0 - Ncount>);
}
/**
 * @brief Pads @p x with @p Ncount copies of @p newvalue on the low side.
 * @tparam Ncount Number of lanes to prepend.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @param newvalue Value to fill the prepended lanes with.
 * @return A vector of length @p N + @p Ncount with the prepended lanes set to @p newvalue.
 */
template <size_t Ncount, typename T, size_t N>
KFR_INTRINSIC vec<T, N + Ncount> padlow(const vec<T, N>& x, std::type_identity_t<T> newvalue)
{
    if constexpr (Ncount == 0)
        return x;
    else
        return concat(broadcast<Ncount, T>(newvalue), x);
}
KFR_FN(padlow)

/**
 * @brief Extends a single-element vector to length @p Nout by repeating its value.
 * @tparam Nout Desired output length.
 * @tparam T Element type of the vector.
 * @param x Single-element source vector.
 * @return A vector of length @p Nout whose lanes are all equal to @p x.front().
 */
template <size_t Nout, typename T>
KFR_INTRINSIC vec<T, Nout> extend(const vec<T, 1>& x)
{
    return repeat<Nout>(vec<T, 1>(x.front()));
}
/**
 * @brief Resizes a multi-element vector to length @p Nout by cyclic repetition or truncation.
 * @details Active when @p N != @p Nout and @p N > 1.
 * @tparam Nout Desired output length.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p Nout.
 */
template <size_t Nout, typename T, size_t N>
    requires(N != Nout && N > 1)
KFR_INTRINSIC vec<T, Nout> extend(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<Nout>);
}
/**
 * @brief Identity overload of extend() for the @p Nout == @p N case.
 * @related extend
 */
template <size_t Nout, typename T, size_t N>
    requires(N == Nout && N > 1)
constexpr KFR_INTRINSIC const vec<T, Nout>& extend(const vec<T, N>& x)
{
    return x;
}
KFR_FN(extend)

/**
 * @brief Extracts a contiguous subvector of @p count elements starting at @p start.
 * @tparam start Index of the first element to extract.
 * @tparam count Number of elements to extract.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p count holding elements [@p start, @p start + @p count).
 */
template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, count> slice(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<count, start>);
}
/**
 * @brief Extracts a contiguous subvector spanning the concatenation of @p x and @p y.
 * @details The two vectors are treated as a single concatenated sequence of length @p N + @p N.
 * @tparam start Index of the first element to extract, in the concatenated sequence.
 * @tparam count Number of elements to extract.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @param x First source vector.
 * @param y Second source vector.
 * @return A vector of length @p count drawn from the concatenated sequence.
 */
template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, count> slice(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq<count, start>);
}
KFR_FN(slice)

/**
 * @brief Replaces a contiguous range of @p x with the corresponding lanes of @p y.
 * @details Lanes of @p x in [@p start, @p start + @p count) are taken from @p y; all other lanes
 *          are taken from @p x.
 * @tparam start Index of the first lane to replace.
 * @tparam count Number of lanes to replace.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @param x Source vector providing the unchanged lanes.
 * @param y Source vector providing the replacement lanes.
 * @return A vector of length @p N with the selected range replaced.
 */
template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, N> replace(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq<N> + (csizeseq<N> >= csize<start> && csizeseq<N> < csize<start + count>)*N);
}
KFR_FN(replace)

/**
 * @brief Terminal overload of split() for variadic recursion.
 * @details No-op overload that terminates the recursive unpacking of output references.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector (unused).
 */
template <size_t, typename T, size_t N>
KFR_INTRINSIC void split(const vec<T, N>&)
{
}
/**
 * @brief Splits @p x into consecutive subvectors written into the supplied output references.
 * @details Each output reference receives the next @p Nout elements of @p x, starting at offset
 *          @p start. The function recurses on the remaining outputs.
 * @tparam start Index of the first element to write into @p out.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Length of the current output subvector.
 * @tparam Args Remaining output reference types.
 * @param x Source vector.
 * @param out Output reference receiving the current subvector.
 * @param args Remaining output references.
 */
template <size_t start = 0, typename T, size_t N, size_t Nout, typename... Args>
KFR_INTRINSIC void split(const vec<T, N>& x, vec<T, Nout>& out, Args&&... args)
{
    out = x.shuffle(csizeseq<Nout, start>);
    split<start + Nout>(x, std::forward<Args>(args)...);
}
/**
 * @brief Splits @p x into two equal halves.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be even.
 * @param x Source vector.
 * @param low Output receiving the first @p N/2 elements.
 * @param high Output receiving the last @p N/2 elements.
 */
template <typename T, size_t N>
KFR_INTRINSIC void split(const vec<T, N>& x, vec<T, N / 2>& low, vec<T, N / 2>& high)
{
    low  = x.shuffle(csizeseq<N / 2, 0>);
    high = x.shuffle(csizeseq<N / 2, N / 2>);
}
/**
 * @brief Splits @p x into four equal quarters.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be divisible by 4.
 * @param x Source vector.
 * @param w0 Output receiving the first quarter.
 * @param w1 Output receiving the second quarter.
 * @param w2 Output receiving the third quarter.
 * @param w3 Output receiving the fourth quarter.
 */
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

/**
 * @brief Extracts the @p number-th of @p total equal-sized parts of @p x.
 * @tparam total Number of equal parts the vector is divided into.
 * @tparam number Zero-based index of the part to extract.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be divisible by @p total.
 * @tparam Nout Output length, defaults to @p N / @p total.
 * @param x Source vector.
 * @return A vector of length @p Nout holding part @p number of @p x.
 */
template <size_t total, size_t number, typename T, size_t N, size_t Nout = N / total>
KFR_INTRINSIC vec<T, Nout> part(const vec<T, N>& x)
{
    static_assert(N % total == 0, "N % total == 0");
    return x.shuffle(csizeseq<Nout, number * Nout>);
}
KFR_FN(part)

/**
 * @brief Concatenates two equal-length vectors and extracts a slice of length @p count.
 * @details The slice is taken from the concatenated sequence of length @p 2 * @p N starting at
 *          offset @p start.
 * @tparam start Index of the first element to extract in the concatenated sequence.
 * @tparam count Number of elements to extract.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @param x First source vector.
 * @param y Second source vector.
 * @return A vector of length @p count drawn from the concatenated sequence.
 */
template <size_t start, size_t count, typename T, size_t N>
KFR_INTRINSIC vec<T, count> concat_and_slice(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, csizeseq<count, start>);
}

/**
 * @brief Concatenates two vectors of unequal length (first longer) and extracts a slice.
 * @details @p y is extended to length @p N1 before concatenation. Active when @p N1 > @p N2.
 * @tparam start Index of the first element to extract in the concatenated sequence.
 * @tparam count Number of elements to extract.
 * @tparam T Element type of the vectors.
 * @tparam N1 Length of the first (longer) input vector.
 * @tparam N2 Length of the second (shorter) input vector.
 * @param x First source vector.
 * @param y Second source vector.
 * @return A vector of length @p count drawn from the concatenated sequence.
 */
template <size_t start, size_t count, typename T, size_t N1, size_t N2>
    requires(N1 > N2)
KFR_INTRINSIC vec<T, count> concat_and_slice(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(y.shuffle(csizeseq<N1>), csizeseq<N1 * 2>).shuffle(csizeseq<count, start>);
}

/**
 * @brief Concatenates two vectors of unequal length (first shorter) and extracts a slice.
 * @details @p x is extended to length @p N2 before concatenation. Active when @p N1 < @p N2.
 * @tparam start Index of the first element to extract in the concatenated sequence.
 * @tparam count Number of elements to extract.
 * @tparam T Element type of the vectors.
 * @tparam N1 Length of the first (shorter) input vector.
 * @tparam N2 Length of the second (longer) input vector.
 * @param x First source vector.
 * @param y Second source vector.
 * @return A vector of length @p count drawn from the concatenated sequence.
 */
template <size_t start, size_t count, typename T, size_t N1, size_t N2>
    requires(N1 < N2)
KFR_INTRINSIC vec<T, count> concat_and_slice(const vec<T, N1>& x, const vec<T, N2>& y)
{
    return x.shuffle(csizeseq<N2, N1 - N2>)
        .shuffle(y, csizeseq<N2 * 2>)
        .shuffle(csizeseq<count, N2 - N1 + start>);
}

KFR_FN(concat_and_slice)

/**
 * @brief Widens @p x to length @p Nout by appending lanes set to @p newvalue.
 * @details Active when @p Nout > @p N. The appended lanes are filled with @p newvalue (default
 *          value of @p T if unspecified).
 * @tparam Nout Desired output length.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @param newvalue Value to fill the appended lanes with.
 * @return A vector of length @p Nout with the appended lanes set to @p newvalue.
 */
template <size_t Nout, typename T, size_t N>
    requires(Nout > N)
KFR_INTRINSIC vec<T, Nout> widen(const vec<T, N>& x, std::type_identity_t<T> newvalue = T())
{
    static_assert(Nout > N, "Nout > N");
    return concat(x, broadcast<Nout - N>(newvalue));
}
/**
 * @brief Identity overload of widen() for the @p Nout == input length case.
 * @related widen
 */
template <size_t Nout, typename T, typename TS>
constexpr KFR_INTRINSIC const vec<T, Nout>& widen(const vec<T, Nout>& x, TS)
{
    return x;
}
KFR_FN(widen)

/**
 * @brief Narrows @p x to length @p Nout by truncation.
 * @details Active when @p Nout <= @p N. The first @p Nout lanes of @p x are retained.
 * @tparam Nout Desired output length.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p Nout holding the first @p Nout elements of @p x.
 */
template <size_t Nout, typename T, size_t N>
KFR_INTRINSIC vec<T, Nout> narrow(const vec<T, N>& x)
{
    static_assert(Nout <= N, "Nout <= N");
    return slice<0, Nout>(x);
}
KFR_FN(narrow)

/**
 * @brief Extracts the even-indexed elements of @p x (with optional grouping).
 * @details Elements at indices 0, 2*@p group, 4*@p group, ... are gathered into the result.
 * @tparam group Stride between consecutive selected pairs.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be even and at least 2.
 * @tparam Nout Output length, defaults to @p N / 2.
 * @param x Source vector.
 * @return A vector of length @p Nout holding the even-indexed elements of @p x.
 */
template <size_t group = 1, typename T, size_t N, size_t Nout = N / 2>
    requires(N >= 2 && (N & 1) == 0)
KFR_INTRINSIC vec<T, Nout> even(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq<Nout / group, 0, 2>));
}
KFR_FN(even)

/**
 * @brief Extracts the odd-indexed elements of @p x (with optional grouping).
 * @details Elements at indices @p group, 3*@p group, 5*@p group, ... are gathered into the result.
 * @tparam group Stride between consecutive selected pairs.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be even and at least 2.
 * @tparam Nout Output length, defaults to @p N / 2.
 * @param x Source vector.
 * @return A vector of length @p Nout holding the odd-indexed elements of @p x.
 */
template <size_t group = 1, typename T, size_t N, size_t Nout = N / 2>
    requires(N >= 2 && (N & 1) == 0)
KFR_INTRINSIC vec<T, Nout> odd(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq<Nout / group, 1, 2>));
}
KFR_FN(odd)

/**
 * @brief Duplicates each even-indexed element into the following odd-indexed lane.
 * @details For each pair (i, i+1), lane i+1 is replaced with the value of lane i.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be even.
 * @param x Source vector.
 * @return A vector of length @p N where each odd lane equals its preceding even lane.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> dupeven(const vec<T, N>& x)
{
    static_assert(N % 2 == 0, "N must be even");
    return x.shuffle(csizeseq<N, 0, 1> & ~csize<1>);
}
KFR_FN(dupeven)

/**
 * @brief Duplicates each odd-indexed element into the preceding even-indexed lane.
 * @details For each pair (i, i+1), lane i is replaced with the value of lane i+1.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be even.
 * @param x Source vector.
 * @return A vector of length @p N where each even lane equals its following odd lane.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> dupodd(const vec<T, N>& x)
{
    static_assert(N % 2 == 0, "N must be even");
    return x.shuffle(csizeseq<N, 0, 1> | csize<1>);
}
KFR_FN(dupodd)

/**
 * @brief Duplicates the contents of @p x to form a vector twice as long.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N * 2 containing two copies of @p x.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N * 2> duphalves(const vec<T, N>& x)
{
    return x.shuffle(csizeseq<N * 2> % csize<N>);
}
KFR_FN(duphalves)

/**
 * @brief Shuffles the concatenation of two equal-length vectors using a repeating index pattern.
 * @details The index pattern @p i is tiled to cover all @p N output lanes, with each tile offset
 *          by its tile index times @p count. The result is drawn from the concatenated sequence
 *          of @p x and @p y.
 * @tparam Indices Compile-time index list defining the per-tile source selection.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @tparam count Number of indices in @p Indices.
 * @param x First source vector.
 * @param y Second source vector.
 * @param i Index pattern selector.
 * @return A vector of length @p N drawn from the concatenated sequence.
 */
template <size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> shuffle(const vec<T, N>& x, const vec<T, N>& y,
                                elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, i[csizeseq_t<N>() % csize_t<sizeof...(Indices)>()] +
                            csizeseq_t<N>() / csize_t<count>() * csize_t<count>());
}
KFR_FN(shuffle)

/**
 * @brief Group-wise variant of shuffle() that operates on blocks of @p group elements.
 * @details Same as shuffle() but the index pattern is applied per group of @p group consecutive
 *          lanes rather than per single lane.
 * @tparam group Number of lanes in each group.
 * @tparam Indices Compile-time index list defining the per-tile source selection.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @tparam count Number of indices in @p Indices.
 * @param x First source vector.
 * @param y Second source vector.
 * @param i Index pattern selector.
 * @return A vector of length @p N drawn from the concatenated sequence, shuffled by group.
 */
template <size_t group, size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> shufflegroups(const vec<T, N>& x, const vec<T, N>& y,
                                      elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, scale<group>(i[csizeseq_t<N / group>() % csize_t<sizeof...(Indices)>()] +
                                     csizeseq_t<N / group>() / csize_t<count>() * csize_t<count>()));
}
KFR_FN(shufflegroups)

/**
 * @brief Permutes the lanes of @p x using a repeating index pattern.
 * @details The index pattern @p i is tiled to cover all @p N output lanes, with each tile offset
 *          by its tile index times @p count.
 * @tparam Indices Compile-time index list defining the per-tile source selection.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam count Number of indices in @p Indices.
 * @param x Source vector.
 * @param i Index pattern selector.
 * @return A vector of length @p N with lanes permuted according to @p i.
 */
template <size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> permute(const vec<T, N>& x, elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(i[csizeseq_t<N>() % csize_t<count>()] +
                     csizeseq_t<N>() / csize_t<count>() * csize_t<count>());
}
KFR_FN(permute)

/**
 * @brief Group-wise variant of permute() that operates on blocks of @p group elements.
 * @details Same as permute() but the index pattern is applied per group of @p group consecutive
 *          lanes rather than per single lane.
 * @tparam group Number of lanes in each group.
 * @tparam Indices Compile-time index list defining the per-tile source selection.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam count Number of indices in @p Indices.
 * @param x Source vector.
 * @param i Index pattern selector.
 * @return A vector of length @p N with lanes permuted by group according to @p i.
 */
template <size_t group, size_t... Indices, typename T, size_t N, size_t count = sizeof...(Indices)>
KFR_INTRINSIC vec<T, N> permutegroups(const vec<T, N>& x, elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(scale<group>(i[csizeseq_t<N / group>() % csize_t<sizeof...(Indices)>()] +
                                  csizeseq_t<N / group>() / csize_t<count>() * csize_t<count>()));
}
KFR_FN(permutegroups)

namespace internal
{

/**
 * @brief Internal helper that generates a vector by invoking @p Fn at each compile-time index.
 * @related generate_vector
 */
template <typename T, size_t Nout, typename Fn, size_t... Indices>
constexpr KFR_INTRINSIC vec<T, Nout> generate_vector(csizes_t<Indices...>)
{
    return make_vector<T>(static_cast<T>(Fn()(Indices))...);
}
} // namespace internal

/**
 * @brief Generates a vector of length @p Nout by invoking @p Fn at each compile-time index.
 * @details @p Fn must be a default-constructible functor taking a `size_t` index and returning a
 *          value convertible to @p T.
 * @tparam T Element type of the resulting vector.
 * @tparam Nout Output vector length.
 * @tparam Fn Generator functor type.
 * @return A vector of length @p Nout where lane @p i equals `Fn()(i)`.
 */
template <typename T, size_t Nout, typename Fn>
constexpr KFR_INTRINSIC vec<T, Nout> generate_vector()
{
    return internal::generate_vector<T, Nout, Fn>(cvalseq_t<size_t, Nout>());
}
KFR_FN(generate_vector)

namespace internal
{
/**
 * @brief Internal helper returning a mask selecting even-indexed lanes.
 * @related even
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> evenmask()
{
    return mask<T, N>(broadcast<N>(maskbits<T>(true), maskbits<T>(false)));
}
/**
 * @brief Internal helper returning a mask selecting odd-indexed lanes.
 * @related odd
 */
template <typename T, size_t N>
KFR_INTRINSIC mask<T, N> oddmask()
{
    return mask<T, N>(broadcast<N>(maskbits<T>(false), maskbits<T>(true)));
}
} // namespace internal

/**
 * @brief Duplicates each element of @p x into a pair of adjacent lanes.
 * @details Element @p i of @p x becomes lanes @p 2*i and @p 2*i+1 of the result.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam Nout Output length, defaults to @p N * 2.
 * @param x Source vector.
 * @return A vector of length @p Nout with each input element duplicated into adjacent lanes.
 */
template <typename T, size_t N, size_t Nout = N * 2>
KFR_INTRINSIC vec<T, Nout> dup(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<Nout>() / csize_t<2>());
}
KFR_FN(dup)

/**
 * @brief Duplicates the lower half of @p x to fill the whole vector.
 * @details The first @p N/2 elements of @p x are repeated to occupy all @p N lanes.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N whose two halves are both copies of the lower half of @p x.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> duplow(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() % csize_t<N / 2>());
}
KFR_FN(duplow)

/**
 * @brief Duplicates the upper half of @p x to fill the whole vector.
 * @details The last @p N/2 elements of @p x are repeated to occupy all @p N lanes.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N whose two halves are both copies of the upper half of @p x.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> duphigh(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() % csize_t<N / 2>() + csize_t<N - N / 2>());
}
KFR_FN(duphigh)

/**
 * @brief Blends lanes from @p x and @p y according to a repeating selector pattern.
 * @details The index pattern @p i selects, for each output lane, whether to take from @p x or
 *          from @p y. A selector value of 0 picks @p x; a value of 1 picks @p y. The pattern is
 *          tiled across all @p N lanes.
 * @tparam Indices Compile-time index list defining the per-tile source selection (0 for @p x, 1 for @p y).
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @param x First source vector.
 * @param y Second source vector.
 * @param i Selector pattern.
 * @return A vector of length @p N blending lanes from @p x and @p y.
 */
template <size_t... Indices, typename T, size_t N>
KFR_INTRINSIC vec<T, N> blend(const vec<T, N>& x, const vec<T, N>& y,
                              elements_t<Indices...> i = elements_t<Indices...>())
{
    return x.shuffle(y, i[csizeseq_t<N>() % csize_t<sizeof...(Indices)>()] * csize_t<N>() + csizeseq_t<N>());
}
KFR_FN(blend)

/**
 * @brief Swaps adjacent groups of @p elements lanes.
 * @details For @p elements == 2, pairs of adjacent lanes are swapped. More generally, blocks of
 *          @p elements consecutive lanes are swapped pairwise.
 * @tparam elements Number of lanes in each block to swap.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N with adjacent @p elements-sized blocks swapped.
 */
template <size_t elements = 2, typename T, size_t N>
KFR_INTRINSIC vec<T, N> swap(const vec<T, N>& x)
{
    return x.shuffle(csizeseq_t<N>() ^ csize_t<elements - 1>());
}
namespace fn
{
KFR_META_FN_TPL((size_t elements), (elements), swap)
}

/**
 * @brief Combines two vectors @p lo and @p hi with a rotation by @p shift lanes.
 * @details The result is formed from the concatenation of @p lo (low) and @p hi (high), shifted
 *          right by @p shift positions. When @p shift == 0 the result is @p lo; when @p shift == @p N
 *          the result is @p hi.
 * @tparam shift Number of lanes to shift by, in [0, @p N].
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @param lo Low source vector.
 * @param hi High source vector.
 * @return A vector of length @p N drawn from the rotated concatenation of @p lo and @p hi.
 */
template <size_t shift, typename T, size_t N>
KFR_INTRINSIC vec<T, N> rotatetwo(const vec<T, N>& lo, const vec<T, N>& hi)
{
    return shift == 0 ? lo : (shift == N ? hi : hi.shuffle(lo, csizeseq_t<N, N - shift>()));
}

/**
 * @brief Rotates the lanes of @p x to the right by @p amount positions.
 * @details Lanes shifted off the high end wrap around to the low end.
 * @tparam amount Number of positions to rotate by, in [0, @p N).
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N rotated right by @p amount.
 */
template <size_t amount, typename T, size_t N>
KFR_INTRINSIC vec<T, N> rotateright(const vec<T, N>& x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return x.shuffle(csizeseq_t<N, N - amount>() % csize_t<N>());
}
KFR_FN(rotateright)

/**
 * @brief Rotates the lanes of @p x to the left by @p amount positions.
 * @details Lanes shifted off the low end wrap around to the high end.
 * @tparam amount Number of positions to rotate by, in [0, @p N).
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N rotated left by @p amount.
 */
template <size_t amount, typename T, size_t N>
KFR_INTRINSIC vec<T, N> rotateleft(const vec<T, N>& x, csize_t<amount> = csize_t<amount>())
{
    static_assert(amount >= 0 && amount < N, "amount >= 0 && amount < N");
    return x.shuffle(csizeseq_t<N, amount>() % csize_t<N>());
}
KFR_FN(rotateleft)

/**
 * @brief Inserts a scalar @p x at the high (right) end of @p y, shifting @p y one lane to the low (left) side.
 * @details The lowest lane of @p y (index 0) is discarded; @p x occupies the highest lane.
 * Given @p y = [y0, y1, ..., y(N-1)], the result is [y1, ..., y(N-1), x].
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Scalar value to insert at the high end.
 * @param y Source vector.
 * @return A vector of length @p N with @p x at the last lane and @p y shifted left by one.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> inserthigh(T x, const vec<T, N>& y)
{
    return concat_and_slice<1, N>(y, vec<T, 1>(x));
}
KFR_FN(inserthigh)

/**
 * @brief Inserts a scalar @p x at the low (left) end of @p y, shifting @p y one lane to the high (right) side.
 * @details The highest lane of @p y (index N-1) is discarded; @p x occupies the lowest lane.
 * Given @p y = [y0, y1, ..., y(N-1)], the result is [x, y0, ..., y(N-2)].
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Scalar value to insert at the low end.
 * @param y Source vector.
 * @return A vector of length @p N with @p x at lane 0 and @p y shifted right by one.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> insertlow(T x, const vec<T, N>& y)
{
    return concat_and_slice<0, N>(vec<T, 1>(x), y);
}
KFR_FN(insertlow)

/**
 * @brief Legacy alias for inserthigh().
 * @related inserthigh
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> insertright(T x, const vec<T, N>& y)
{
    return inserthigh(x, y);
}
KFR_FN(insertright)

/**
 * @brief Legacy alias for insertlow().
 * @related insertlow
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> insertleft(T x, const vec<T, N>& y)
{
    return insertlow(x, y);
}
KFR_FN(insertleft)

/**
 * @brief Transposes a vector viewed as a @p side1 x @p side2 matrix (row-major).
 * @details The vector is treated as a matrix of @p size = @p N / @p group elements with @p side1
 *          rows and @p side2 columns, where @p side2 = @p size / @p side1. The result is the
 *          matrix transpose. Active when @p size > 3.
 * @tparam side1 Number of rows in the interpreted matrix.
 * @tparam group Number of lanes in each element group.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam size Number of elements per group, equals @p N / @p group.
 * @tparam side2 Number of columns, equals @p size / @p side1.
 * @param x Source vector.
 * @return A vector of length @p N representing the transposed matrix.
 */
template <size_t side1, size_t group = 1, typename T, size_t N, size_t size = N / group,
          size_t side2 = size / side1>
    requires(size > 3)
KFR_INTRINSIC vec<T, N> transpose(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
/**
 * @brief Identity overload of transpose() for small vectors (size <= 3).
 * @details For matrices with 3 or fewer elements per group, transposition is a no-op.
 * @related transpose
 */
template <size_t side, size_t group = 1, typename T, size_t N>
    requires(N / group <= 3)
KFR_INTRINSIC vec<T, N> transpose(const vec<T, N>& x)
{
    return x;
}
/**
 * @brief Transposes a vector of vectors (matrix of vectors).
 * @details Each inner vector is treated as a row; the result swaps rows and columns at the inner
 *          vector granularity.
 * @tparam T Element type of the inner vector.
 * @tparam N Length of each inner vector and the outer vector.
 * @param x Source vector of vectors.
 * @return The transposed vector of vectors.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<vec<T, N>, N> transpose(const vec<vec<T, N>, N>& x)
{
    return vec<vec<T, N>, N>::from_flatten(transpose<N>(x.flatten()));
}
KFR_FN(transpose)

/**
 * @brief Inverse transpose of a vector viewed as a @p side1 x @p side2 matrix (row-major).
 * @details Interprets the vector as a matrix with @p side2 columns and @p side1 = @p size / @p side2
 *          rows, then transposes. Active when @p size > 3.
 * @tparam side2 Number of columns in the interpreted matrix.
 * @tparam group Number of lanes in each element group.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam size Number of elements per group, equals @p N / @p group.
 * @tparam side1 Number of rows, equals @p size / @p side2.
 * @param x Source vector.
 * @return A vector of length @p N representing the inverse-transposed matrix.
 */
template <size_t side2, size_t group = 1, typename T, size_t N, size_t size = N / group,
          size_t side1 = size / side2>
    requires(size > 3)
KFR_INTRINSIC vec<T, N> transposeinverse(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
/**
 * @brief Identity overload of transposeinverse() for small vectors (size <= 3).
 * @related transposeinverse
 */
template <size_t side, size_t groupsize = 1, typename T, size_t N>
    requires(N / groupsize <= 3)
KFR_INTRINSIC vec<T, N> transposeinverse(const vec<T, N>& x)
{
    return x;
}
KFR_FN(transposeinverse)

/**
 * @brief Complex transpose: transposes pairs of adjacent lanes viewed as complex (real, imag).
 * @details Equivalent to transpose<side, 2> when @p side is in (1, @p N/2); otherwise a no-op.
 * @tparam side Number of complex elements per row.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N with complex pairs transposed.
 */
template <size_t side, typename T, size_t N>
KFR_INTRINSIC vec<T, N> ctranspose(const vec<T, N>& x)
{
    if constexpr (side <= 1 || side >= N / 2)
        return x;
    else
        return transpose<side, 2>(x);
}
KFR_FN(ctranspose)

/**
 * @brief Inverse complex transpose.
 * @details Equivalent to transposeinverse<side, 2>.
 * @tparam side Number of complex elements per row.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N with complex pairs inverse-transposed.
 */
template <size_t side, typename T, size_t N>
KFR_INTRINSIC vec<T, N> ctransposeinverse(const vec<T, N>& x)
{
    return transposeinverse<side, 2>(x);
}
KFR_FN(ctransposeinverse)

namespace internal
{

/**
 * @brief Internal helper that gathers bits from @p x at positions @p A into a contiguous result.
 * @related shuffleindexbits
 */
template <size_t... A, size_t... I>
constexpr size_t shufflebits(size_t x, elements_t<A...>, csizes_t<I...>) noexcept
{
    return ((((x >> A) & 1u) << I) | ...);
}

/**
 * @brief Internal helper overload of shufflebits using a default sequential output index.
 * @related shuffleindexbits
 */
template <size_t... A>
constexpr size_t shufflebits(size_t x, elements_t<A...>) noexcept
{
    return shufflebits(x, elements<A...>, csizeseq<sizeof...(A)>);
}

/**
 * @brief Internal helper that generates a compile-time index sequence by bit-shuffling.
 * @related shuffleindexbits
 */
template <typename Indices, size_t... A>
struct indexbit_gen;

/**
 * @brief Internal specialization of indexbit_gen producing an elements_t of bit-shuffled indices.
 * @related shuffleindexbits
 */
template <size_t... I, size_t... A>
struct indexbit_gen<csizes_t<I...>, A...>
{
    using type = elements_t<shufflebits(I, elements<A...>, csizeseq<sizeof...(A)>)...>;
};
} // namespace internal

/**
 * @brief Shuffles lanes of @p x by permuting the bits of each lane index.
 * @details Each lane index @p i in [0, @p N / @p group) is decomposed into bits, and the bits are
 *          reordered: bit @p A[j] of @p i is moved to bit position @p j (LSB-first) of the new
 *          index. This implements an arbitrary bit-permutation of the index space, useful for
 *          FFT-style reordering.
 * @tparam group Number of lanes in each element group, must be a power of two.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length, must be a power of two.
 * @tparam A Bit position specifiers; the count must equal ilog2(@p N / @p group).
 * @param x Source vector.
 * @return A vector of length @p N with lanes reordered by the bit permutation.
 */
template <size_t group = 1, typename T, size_t N, size_t... A>
KFR_INTRINSIC vec<T, N> shuffleindexbits(const vec<T, N>& x, elements_t<A...>)
{
    static_assert(is_poweroftwo(group), "group must be a power of two");
    static_assert(is_poweroftwo(N), "N must be a power of two");
    static_assert(sizeof...(A) == ilog2(N / group), "Number of axes must be log2(N/group)");

    constexpr auto indices        = typename internal::indexbit_gen<csizeseq_t<N / group>, A...>::type{};
    constexpr auto scaled_indices = scale<group>(indices);

    return x.shuffle(scaled_indices);
}

/**
 * @brief Interleaves the lanes of two equal-length vectors @p x and @p y.
 * @details Produces a vector of length @p N * 2 where lanes from @p x and @p y alternate. With
 *          @p group > 1, groups of @p group consecutive lanes are interleaved as units.
 * @tparam group Number of lanes in each interleaved group.
 * @tparam T Element type of the vectors.
 * @tparam N Length of each input vector.
 * @tparam Nout Output length, defaults to @p N * 2.
 * @tparam size Number of groups, equals @p Nout / @p group.
 * @tparam side2 Number of sources, fixed at 2.
 * @tparam side1 Groups per source, equals @p size / @p side2.
 * @param x First source vector.
 * @param y Second source vector.
 * @return A vector of length @p Nout with lanes from @p x and @p y interleaved.
 */
template <size_t group = 1, typename T, size_t N, size_t Nout = N * 2, size_t size = Nout / group,
          size_t side2 = 2, size_t side1 = size / side2>
KFR_INTRINSIC vec<T, Nout> interleave(const vec<T, N>& x, const vec<T, N>& y)
{
    return x.shuffle(y, scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                     csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(interleave)

/**
 * @brief Zips multiple vectors into a vector of vectors.
 * @details The inputs are interleaved and regrouped into inner vectors of length @p side2 (the
 *          number of inputs). The number of inputs must be a power of two.
 * @tparam T Element type of the vectors.
 * @tparam N1 Length of the first input vector.
 * @tparam Ns Lengths of the remaining input vectors.
 * @tparam size Total number of scalar elements, equals @p N1 + csum(Ns...).
 * @tparam side2 Number of input vectors.
 * @tparam side1 Number of inner vectors in the result, equals @p size / @p side2.
 * @param x First source vector.
 * @param y Remaining source vectors.
 * @return A vector of @p side1 inner vectors, each of length @p side2.
 */
template <typename T, size_t N1, size_t... Ns, size_t size = N1 + csum<size_t, Ns...>(),
          size_t side2 = 1 + sizeof...(Ns), size_t side1 = size / side2>
KFR_INTRINSIC vec<vec<T, side2>, side1> zip(const vec<T, N1>& x, const vec<T, Ns>&... y)
{
    static_assert(is_poweroftwo(1 + sizeof...(Ns)), "number of vectors must be power of two");
    return vec<vec<T, side2>, side1>::from_flatten(concat(x, y...).shuffle(scale<1>(
        csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() + csizeseq_t<size>() / csize_t<side2>())));
}
KFR_FN(zip)

/**
 * @brief Extracts a single column from a vector-of-vectors.
 * @details Treats @p x as a matrix of @p N2 rows by @p N1 columns and returns the @p index-th column.
 * @tparam index Column index to extract, must be less than @p N1.
 * @tparam T Element type of the inner vector.
 * @tparam N1 Length of each inner vector (number of columns).
 * @tparam N2 Number of inner vectors (number of rows).
 * @param x Source vector of vectors.
 * @return A vector of length @p N2 holding the @p index-th element of each inner vector.
 */
template <size_t index, typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, N2> column(const vec<vec<T, N1>, N2>& x)
{
    static_assert(index < N1, "column index must be less than inner vector length");
    return x.flatten().shuffle(csizeseq_t<N2>() * csize_t<N1>() + csize_t<index>());
}

/**
 * @brief Interleaves the two halves of @p x.
 * @details Treats @p x as two halves and interleaves them lane-by-lane (or by @p group-sized blocks).
 * @tparam group Number of lanes in each interleaved block.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam size Number of groups, equals @p N / @p group.
 * @tparam side2 Number of halves, fixed at 2.
 * @tparam side1 Groups per half, equals @p size / @p side2.
 * @param x Source vector.
 * @return A vector of length @p N with the two halves interleaved.
 */
template <size_t group = 1, typename T, size_t N, size_t size = N / group, size_t side2 = 2,
          size_t side1 = size / side2>
KFR_INTRINSIC vec<T, N> interleavehalves(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(interleavehalves)

/**
 * @brief Deinterleaves adjacent pairs (inverse of interleavehalves with @p side1 == 2).
 * @details Groups adjacent lanes into pairs and reorders so that all first elements of each pair
 *          precede all second elements.
 * @tparam group Number of lanes in each block.
 * @tparam T Element type of the vector.
 * @tparam N Input vector length.
 * @tparam size Number of groups, equals @p N / @p group.
 * @tparam side1 Number of elements per pair, fixed at 2.
 * @tparam side2 Number of pairs, equals @p size / @p side1.
 * @param x Source vector.
 * @return A vector of length @p N with adjacent pairs split apart.
 */
template <size_t group = 1, typename T, size_t N, size_t size = N / group, size_t side1 = 2,
          size_t side2 = size / side1>
KFR_INTRINSIC vec<T, N> splitpairs(const vec<T, N>& x)
{
    return x.shuffle(scale<group>(csizeseq_t<size>() % csize_t<side2>() * csize_t<side1>() +
                                  csizeseq_t<size>() / csize_t<side2>()));
}
KFR_FN(splitpairs)

/**
 * @brief Reverses the order of lanes in @p x (with optional grouping).
 * @details When @p group > 1, groups of @p group consecutive lanes are reversed as units.
 * @tparam group Number of lanes in each reversed group.
 * @tparam T Element type of the vector, must not itself be a vector type.
 * @tparam N Input vector length.
 * @param x Source vector.
 * @return A vector of length @p N with lanes (or groups) in reverse order.
 */
template <size_t group = 1, typename T, size_t N>
    requires(!is_vec<T>)
KFR_INTRINSIC vec<T, N> reverse(const vec<T, N>& x)
{
    constexpr size_t size = N / group;
    return x.shuffle(scale<group>(csizeseq_t<size, size - 1, -1>()));
}
/**
 * @brief Reverses the order of inner vectors in a vector of vectors.
 * @details The outer order of inner vectors is reversed; the order within each inner vector is
 *          preserved.
 * @tparam T Element type of the inner vector.
 * @tparam N1 Length of each inner vector.
 * @tparam N2 Number of inner vectors.
 * @param x Source vector of vectors.
 * @return A vector of vectors with the outer order reversed.
 */
template <size_t group = 1, typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<vec<T, N1>, N2> reverse(const vec<vec<T, N1>, N2>& x)
{
    return swap<N1>(x.flatten()).v;
}
KFR_FN(reverse)

/**
 * @brief Combines two vectors by selecting lanes from @p x or @p y based on index.
 * @details For lane indices in [0, @p N2) the lane is taken from @p x; for indices >= @p N2 the
 *          lane is taken from @p y. @p y is extended to length @p N1 before the selection.
 * @tparam T Element type of the vectors.
 * @tparam N1 Length of the first (longer) input vector and the output.
 * @tparam N2 Length of the second (shorter) input vector, must be <= @p N1.
 * @param x First source vector.
 * @param y Second source vector.
 * @return A vector of length @p N1 combining lanes from @p x and @p y.
 */
template <typename T, size_t N1, size_t N2>
KFR_INTRINSIC vec<T, N1> combine(const vec<T, N1>& x, const vec<T, N2>& y)
{
    static_assert(N2 <= N1, "N2 <= N1");
    return x.shuffle(extend<N1>(y), (csizeseq_t<N1>() < csize_t<N2>()) * csize_t<N1>() + csizeseq_t<N1>());
}
KFR_FN(combine)

namespace internal
{
/**
 * @brief Internal functor generating an arithmetic sequence `start + index * stride`.
 * @related enumerate
 */
template <size_t start, size_t stride>
struct generate_index
{
    KFR_INTRINSIC constexpr size_t operator()(size_t index) const { return start + index * stride; }
};
/**
 * @brief Internal functor generating an on/off pattern over a contiguous range.
 * @related onoff
 */
template <size_t start, size_t size, int on, int off>
struct generate_onoff
{
    KFR_INTRINSIC constexpr size_t operator()(size_t index) const
    {
        return index >= start && index < start + size ? on : off;
    }
};
} // namespace internal

/**
 * @brief Generates a vector with an arithmetic sequence of values.
 * @details Lane @p i is set to `start + i * stride`.
 * @tparam T Element type of the resulting vector.
 * @tparam N Output vector length.
 * @tparam start Value of the first lane.
 * @tparam stride Difference between consecutive lanes.
 * @return A vector of length @p N holding the arithmetic sequence.
 */
template <typename T, size_t N, size_t start = 0, size_t stride = 1>
constexpr KFR_INTRINSIC vec<T, N> enumerate()
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
/**
 * @brief Shape-driven overload of enumerate() for type deduction.
 * @related enumerate
 */
template <size_t start = 0, size_t stride = 1, typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> enumerate(vec_shape<T, N>)
{
    return generate_vector<T, N, internal::generate_index<start, stride>>();
}
/**
 * @brief Generates a vector with an arithmetic sequence using a runtime step.
 * @details Lane @p i is set to `i * step`. Uses a doubling accumulation strategy for power-of-two
 *          @p N; non-power-of-two lengths are handled by slicing from the next power of two.
 * @tparam T Element type of the resulting vector.
 * @tparam N Output vector length.
 * @param sh Shape tag for type deduction.
 * @param step Difference between consecutive lanes.
 * @return A vector of length @p N holding the sequence `0, step, 2*step, ...`.
 */
template <typename T, size_t N>
KFR_INTRINSIC vec<T, N> enumerate(vec_shape<T, N> sh, std::type_identity_t<T> step)
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
             [&](auto idx) KFR_INLINE_LAMBDA
             {
                 vv = vv + vv;
                 acc += blend(zz, vv, csizeseq<N> / (csize<2 << (idx)>) % csize<2>);
             });
        return acc;
    }
}

KFR_FN(enumerate)

/**
 * @brief Generates a vector with an on/off pattern over a contiguous range.
 * @details Lanes in [@p start, @p start + @p size) are set to @p on; all others are set to @p off.
 * @tparam T Element type of the resulting vector.
 * @tparam N Output vector length.
 * @tparam start Index of the first "on" lane.
 * @tparam size Number of consecutive "on" lanes.
 * @tparam on Value for lanes inside the range.
 * @tparam off Value for lanes outside the range.
 * @return A vector of length @p N holding the on/off pattern.
 */
template <typename T, size_t N, size_t start = 0, size_t size = 1, int on = 1, int off = 0>
constexpr KFR_INTRINSIC vec<T, N> onoff(cint_t<on> = cint_t<on>(), cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
/**
 * @brief Shape-driven overload of onoff() for type deduction.
 * @related onoff
 */
template <size_t start = 0, size_t size = 1, int on = 1, int off = 0, typename T, size_t N>
constexpr KFR_INTRINSIC vec<T, N> onoff(vec_shape<T, N>, cint_t<on> = cint_t<on>(),
                                        cint_t<off> = cint_t<off>())
{
    return generate_vector<T, N, internal::generate_onoff<start, size, on, off>>();
}
KFR_FN(onoff)

} // namespace KFR_ARCH_NAME
} // namespace kfr
#define KFR_SHUFFLE_SPECIALIZATIONS 1
#include "impl/specializations.hpp"

KFR_PRAGMA_MSVC(warning(pop))
