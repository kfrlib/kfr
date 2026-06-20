/** @addtogroup basic_math
 *  @{
 */
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
#include "impl/bitshuffle.hpp"

namespace kfr
{

#if defined(KFR_ARCH_RVV) || defined(KFR_ARCH_AVX512)
#define KFR_DISABLE_BITSHUFFLE 1
#endif

inline namespace KFR_ARCH_NAME
{

template <size_t count, typename T, size_t N, typename V>
KFR_INTRINSIC void split_native(const vec<T, N>& in, V (&out)[count])
{
    static_assert(is_poweroftwo(count));
    static_assert(is_poweroftwo(N));
    constexpr size_t S = N / count;
    [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA
    { ((out[I] = static_cast<V>(slice<S * I, S>(in).v)), ...); }(csizeseq_t<count>{});
}

template <size_t count, typename T, size_t N, typename V>
KFR_INTRINSIC vec<T, N> concat_native_recursive(const V* in)
{
    if constexpr (count == 1)
        return vec<T, N>(in[0]);
    else
        return concat(concat_native_recursive<count / 2, T, N / 2, V>(in),
                      concat_native_recursive<count / 2, T, N / 2, V>(in + count / 2));
}

template <typename T, size_t N, typename V, size_t count>
KFR_INTRINSIC void concat_native(vec<T, N>& out, const V (&in)[count])
{
    static_assert(is_poweroftwo(count));
    static_assert(is_poweroftwo(N));
    out = concat_native_recursive<count, T, N, V>(in);
}

#ifndef KFR_DISABLE_BITSHUFFLE

struct bitperm_step
{
    int8_t op_idx       = -1;
    int8_t pull_src_idx = -1;
};

template <typename T>
struct bitperm_plan
{
    std::array<bitperm_step, intr::in_reg_bits<T>> steps{};
    uint8_t count = 0;
    bool found    = false;
    bitperm<6> final_perm{};
};

template <typename T, size_t N>
constexpr bitperm<N> apply(bitperm<N> s, int x_idx, int op_idx)
{
    // x_idx is where the pulled element currently lives (>= 3)
    // Bring it to position 3 (free swap)
    if (x_idx != intr::in_reg_bits<T>)
        std::swap(s[intr::in_reg_bits<T>], s[x_idx]);

    // Apply permutation to first 4 elements
    bitperm<N> next  = s;
    const auto& perm = intr::bitperm_ops<T>[op_idx].p;
    for (int j = 0; j < intr::in_reg_bits<T> + 1; ++j)
        next[j] = s[perm[j]];
    return next;
}

template <size_t z, size_t N>
constexpr bool prefix_equal(const bitperm<N>& current, const bitperm<N>& target_prefix)
{
    for (size_t i = 0; i < z; ++i)
    {
        if (current[i] != target_prefix[i])
            return false;
    }
    return true;
}

template <typename T, size_t N>
constexpr bool dls(bitperm<N> current, const bitperm<N>& target_prefix, int depth, int limit,
                   bitperm_plan<T>& plan)
{
    constexpr size_t z = intr::in_reg_bits<T>;
    // Only check first 3 positions for goal
    if (prefix_equal<z>(current, target_prefix))
    {
        plan.count = depth;

        std::iota(plan.final_perm.begin(), plan.final_perm.end(), 0);
        // Build permutation that will convert current to target for indices [z, N)
        for (size_t i = 0; i < N - z; ++i)
        {
            const uint8_t target = target_prefix[i + z];
            for (size_t j = 0; j < N - z; ++j)
            {
                if (current[j + z] == target)
                {
                    plan.final_perm[i] = static_cast<uint8_t>(j);
                    break;
                }
            }
        }
        return true;
    }
    if (depth >= limit)
        return false;

    for (int xi = z; xi < N; ++xi)
    {
        for (int op = 0; op < std::size(intr::bitperm_ops<T>); ++op)
        {
            plan.steps[depth] = { int8_t(op), int8_t(xi) };
            if (dls<T, N>(apply<T, N>(current, xi, op), target_prefix, depth + 1, limit, plan))
            {
                return true;
            }
        }
    }
    return false;
}

template <typename T, size_t N>
consteval bitperm_plan<T> find_min_plan(bitperm<N> target_prefix)
{
    bitperm<N> start{};
    std::iota(start.begin(), start.end(), 0);
    bitperm_plan<T> plan;
    for (int limit = 0; limit <= plan.steps.size(); ++limit)
        if (dls<T, N>(start, target_prefix, 0, limit, plan))
        {
            plan.found = true;
            return plan;
        }
    return plan;
}

template <bitperm_step step, typename T, size_t count>
KFR_INTRINSIC void bitpermute_step(typename native_vector_type<T>::type (&out)[count],
                                   const typename native_vector_type<T>::type (&in)[count])
{
    static_assert(is_poweroftwo(count));
    constexpr uint8_t register_bit_count = ilog2(count);
    constexpr uint8_t pull_bit           = step.pull_src_idx - intr::in_reg_bits<T>;

    static_assert(pull_bit < register_bit_count);

    [&]<size_t... I>(csizes_t<I...>) KFR_INLINE_LAMBDA
    {
        (
            [&]
            {
                constexpr size_t J0 = internal_generic::swap_bit_indices<0, pull_bit>(I * 2);
                constexpr size_t J1 = J0 | (1 << pull_bit);
                intr::bitpermute_pair<step.op_idx>(out[I * 2], out[I * 2 + 1], in[J0], in[J1]);
            }(),
            ...);
    }(csizeseq_t<count / 2>{});
}

#endif

template <size_t k, bitperm<k> perm, typename T, size_t N = 1u << k>
KFR_INTRINSIC vec<T, N> bitpermute(const vec<T, N>& w)
{
#ifdef KFR_DISABLE_BITSHUFFLE
    return vec<T, N>(intr::simd_shuffle(intr::simd_t<T, N>{}, w.v, internal_generic::to_elements<k, perm>(),
                                        overload_auto));
#else
#if defined KFR_ARCH_ARM && !defined(KFR_ARCH_NEON64)
    if constexpr (std::is_same_v<T, double>)
    {
        return vec<T, N>(intr::simd_shuffle(intr::simd_t<T, N>{}, w.v,
                                            internal_generic::to_elements<k, perm>(), overload_auto));
    }
    else
#endif
    {
        static_assert(ilog2(N) <= 8);
        constexpr auto plan = find_min_plan<T>(perm);
        static_assert(plan.found, "No plan found for this permutation");

        using V                   = typename native_vector_type<T>::type;
        constexpr size_t elements = sizeof(V) / sizeof(T);
        constexpr size_t count    = N / elements;

        V x[count];
        V y[count];
        if constexpr (plan.count % 2 == 0)
        {
            split_native(w, y);
        }
        else
        {
            split_native(w, x);
        }

        KFR_FOR(i, 0, plan.count)
        {
            if constexpr ((plan.count - 1 - i) % 2 == 0)
            {
                bitpermute_step<plan.steps[i], T>(y, x);
            }
            else
            {
                bitpermute_step<plan.steps[i], T>(x, y);
            }
        };

        KFR_FOR(i, 0, count) { x[i] = y[internal_generic::shuffle_bits(i, plan.final_perm)]; };

        vec<T, N> result;
        concat_native(result, x);
        return result;
    }
#endif
}

#ifdef KFR_DISABLE_OPTIMIZED_SHUFFLE
template <size_t... I, typename T, size_t N, size_t k = std::countr_zero(N)>
KFR_INTRINSIC vec<T, N> optimized_shuffle(const vec<T, N>& w, elements_t<I...>)
{
    static_assert(sizeof...(I) == N);
    constexpr auto perm = to_bit_indices<N>({ I... });
    if constexpr (N < 2 * vector_width<T> || perm == bitperm<k>{}) // Not a bit permutation
        return w.shuffle(elements<I...>);
    else
        return bitpermute<k, perm, T>(w);
}
#endif

} // namespace KFR_ARCH_NAME

} // namespace kfr
