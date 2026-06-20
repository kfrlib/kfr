/** @addtogroup biquad
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

#include "../base/filter.hpp"
#include "../base/handle.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/operators.hpp"
#include "../base/state_holder.hpp"
#include "../simd/vec.hpp"
#include "../test/assert.hpp"

namespace kfr
{

/// Maximum supported IIR filter order.
constexpr inline size_t maximum_iir_order = 128;
/// Maximum number of cascaded biquad sections (maximum_iir_order / 2).
constexpr inline size_t maximum_biquad_count = maximum_iir_order / 2;

namespace internal_generic
{
constexpr inline auto biquad_sizes = csize<1> << csizeseq<ilog2(maximum_biquad_count) + 1>;
}

/// Type of biquad filter response used by coefficient design helpers.
enum class biquad_type
{
    lowpass, ///< Passes frequencies below the cutoff.
    highpass, ///< Passes frequencies above the cutoff.
    bandpass, ///< Passes frequencies inside a band.
    bandstop, ///< Rejects frequencies inside a band.
    peak, ///< Peaking equalizer filter.
    notch, ///< Notch (band-reject) filter.
    lowshelf, ///< Low-shelf equalizer filter.
    highshelf ///< High-shelf equalizer filter.
};

/**
 * @brief Structure for holding biquad filter coefficients.
 *
 * Represents a single second-order section in the Direct Form I/II representation:
 *   H(z) = (b0 + b1*z^-1 + b2*z^-2) / (a0 + a1*z^-1 + a2*z^-2)
 * The coefficients are stored unnormalized; use normalized_a0()/normalized_b0() to
 * obtain a section with a0 == 1 or b0 == 1 respectively.
 *
 * @tparam T Floating-point element type.
 */
template <typename T>
struct biquad_section
{
    /**
     * @brief Converting copy constructor.
     * @param bq Source section whose coefficients are cast to T.
     */
    template <typename U>
    constexpr biquad_section(const biquad_section<U>& bq) noexcept
        : a0(static_cast<T>(bq.a0)), a1(static_cast<T>(bq.a1)), a2(static_cast<T>(bq.a2)),
          b0(static_cast<T>(bq.b0)), b1(static_cast<T>(bq.b1)), b2(static_cast<T>(bq.b2))
    {
    }

    static_assert(std::is_floating_point_v<T>, "T must be a floating point type");
    /// Default constructor: produces a pass-through section (a0=b0=1, rest 0).
    constexpr biquad_section() noexcept : a0(1), a1(0), a2(0), b0(1), b1(0), b2(0) {}
    /**
     * @brief Construct from explicit coefficient values.
     * @param a0 Denominator coefficient z^0.
     * @param a1 Denominator coefficient z^-1.
     * @param a2 Denominator coefficient z^-2.
     * @param b0 Numerator coefficient z^0.
     * @param b1 Numerator coefficient z^-1.
     * @param b2 Numerator coefficient z^-2.
     */
    constexpr biquad_section(T a0, T a1, T a2, T b0, T b1, T b2) noexcept
        : a0(a0), a1(a1), a2(a2), b0(b0), b1(b1), b2(b2)
    {
    }
    T a0; ///< Denominator coefficient z^0
    T a1; ///< Denominator coefficient z^-1
    T a2; ///< Denominator coefficient z^-2
    T b0; ///< Numerator coefficient z^0
    T b1; ///< Numerator coefficient z^-1
    T b2; ///< Numerator coefficient z^-2
    /// Returns a section scaled so that a0 == 1.
    biquad_section<T> normalized_a0() const
    {
        vec<T, 5> v{ a1, a2, b0, b1, b2 };
        v = v / a0;
        return { T(1.0), v[0], v[1], v[2], v[3], v[4] };
    }
    /// Returns a section scaled so that b0 == 1.
    biquad_section<T> normalized_b0() const { return { a0, a1, a2, T(1.0), b1 / b0, b2 / b0 }; }
    /// Returns a section scaled so that both a0 == 1 and b0 == 1.
    biquad_section<T> normalized_all() const { return normalized_a0().normalized_b0(); }
};

/**
 * @brief Per-section delay-line state for a cascade of `filters` biquad sections.
 *
 * Holds the two Direct Form II transposed delay registers (s1, s2) and the
 * most recent output vector `out` (used as the inter-section delay line).
 *
 * @tparam T Floating-point element type.
 * @tparam filters Number of cascaded biquad sections.
 */
template <typename T, size_t filters>
struct biquad_state
{
    vec<T, filters> s1; ///< First delay register per section.
    vec<T, filters> s2; ///< Second delay register per section.
    vec<T, filters> out; ///< Last output vector, used as inter-section delay line.
    /// Default constructor: zero-initializes all registers.
    constexpr biquad_state() noexcept : s1(0), s2(0), out(0) {}
};

/**
 * @brief Fixed-size container of biquad coefficients stored as SoA vectors.
 *
 * Stores the a1, a2, b0, b1, b2 coefficients of `filters` sections as separate
 * vectors (a0 is assumed to be 1 after normalization). Used by the SIMD
 * biquad processing kernel.
 *
 * @tparam T Floating-point element type.
 * @tparam filters Number of cascaded biquad sections (tag_dynamic_vector for runtime size).
 */
template <typename T, size_t filters = tag_dynamic_vector>
struct iir_params
{
    vec<T, filters> a1; ///< Denominator coefficient z^-1 per section.
    vec<T, filters> a2; ///< Denominator coefficient z^-2 per section.
    vec<T, filters> b0; ///< Numerator coefficient z^0 per section.
    vec<T, filters> b1; ///< Numerator coefficient z^-1 per section.
    vec<T, filters> b2; ///< Numerator coefficient z^-2 per section.

    /// Default constructor: pass-through (b0=1, rest 0).
    constexpr iir_params() noexcept : a1(0), a2(0), b0(1), b1(0), b2(0) {}
    /**
     * @brief Build from an array of biquad_section.
     * @param bq Pointer to the first section.
     * @param count Number of sections to copy; remaining slots are filled with pass-through.
     */
    iir_params(const biquad_section<T>* bq, size_t count)
    {
        KFR_LOGIC_CHECK(count <= filters, "iir_params: too many biquad sections");
        count = std::min(filters, count);
        for (size_t i = 0; i < count; i++)
        {
            a1[i] = bq[i].a1;
            a2[i] = bq[i].a2;
            b0[i] = bq[i].b0;
            b1[i] = bq[i].b1;
            b2[i] = bq[i].b2;
        }
        for (size_t i = count; i < filters; i++)
        {
            a1[i] = T(0);
            a2[i] = T(0);
            b0[i] = T(1);
            b1[i] = T(0);
            b2[i] = T(0);
        }
    }

    /// Construct from a single biquad_section.
    iir_params(const biquad_section<T>& one) noexcept : iir_params(&one, 1) {}

    /**
     * @brief Construct from any container with data()/size().
     * @param cont Container of biquad_section values.
     */
    template <has_data_size Container>
    constexpr iir_params(Container&& cont) noexcept : iir_params(std::data(cont), std::size(cont))
    {
    }
};

/**
 * @brief Dynamic-size specialization of iir_params.
 *
 * Stores a runtime-sized vector of biquad_section values. The runtime size is
 * rounded up to the next power of two (up to maximum_biquad_count) when the
 * filter is instantiated.
 *
 * @tparam T Floating-point element type.
 */
template <typename T>
struct iir_params<T, tag_dynamic_vector> : public std::vector<biquad_section<T>>
{
    using base = std::vector<biquad_section<T>>;

    iir_params()                  = default;
    iir_params(const iir_params&) = default;
    iir_params(iir_params&&)      = default;

    /// Allocate storage for `count` default-constructed sections.
    iir_params(size_t count) : base(count) {}

    /// Build from a C array of `count` biquad_section values.
    iir_params(const biquad_section<T>* bq, size_t count) noexcept : base(bq, bq + count) {}

    /// Construct from a single biquad_section.
    iir_params(const biquad_section<T>& one) noexcept : iir_params(&one, 1) {}

    /// Move-construct from a vector of sections.
    iir_params(std::vector<biquad_section<T>>&& sections) noexcept : base(std::move(sections)) {}

    /**
     * @brief Construct from any container with data()/size().
     * @param cont Container of biquad_section values.
     */
    template <has_data_size Container>
    constexpr iir_params(Container&& cont) noexcept : iir_params(std::data(cont), std::size(cont))
    {
    }

    /**
     * @brief Convert a fixed-size iir_params to the dynamic-size form.
     * @param params Fixed-size parameters; a0 is set to 1 for each section.
     */
    template <size_t filters>
    iir_params(const iir_params<T, filters>& params) : base(filters)
    {
        for (size_t i = 0; i < filters; ++i)
        {
            this->operator[](i).a0 = T(1);
            this->operator[](i).a1 = params.a1[i];
            this->operator[](i).a2 = params.a2[i];
            this->operator[](i).b0 = params.b0[i];
            this->operator[](i).b1 = params.b1[i];
            this->operator[](i).b2 = params.b2[i];
        }
    }
};

template <typename T, size_t Size>
iir_params(const std::array<T, Size>&) -> iir_params<T, Size>;
template <typename T, size_t Size>
iir_params(const univector<T, Size>) -> iir_params<T, Size>;
template <typename T, size_t Size>
iir_params(const biquad_section<T> (&)[Size]) -> iir_params<T, Size>;
template <typename T>
iir_params(const biquad_section<T>&) -> iir_params<T, 1>;
template <typename T>
iir_params(const std::vector<biquad_section<T>>&) -> iir_params<T, tag_dynamic_vector>;
template <typename T>
iir_params(std::vector<biquad_section<T>>&&) -> iir_params<T, tag_dynamic_vector>;

/**
 * @brief Combined IIR filter parameters and runtime state.
 *
 * Holds the fixed-size iir_params together with the working biquad_state, a
 * saved copy of the state used for block-based (look-ahead) processing, and
 * `block_end` marking the end of the current processing block.
 *
 * @tparam T Floating-point element type.
 * @tparam filters Number of cascaded biquad sections (1..maximum_biquad_count).
 */
template <typename T, size_t filters>
struct iir_state
{
    static_assert(filters >= 1 && filters <= maximum_biquad_count, "Incorrect number of biquad filters");

    iir_params<T, filters> params;

    /**
     * @brief Forwarding constructor for the embedded iir_params.
     * @param args Arguments forwarded to iir_params<T, filters>.
     */
    template <typename... Args,
              std::enable_if_t<std::is_constructible_v<iir_params<T, filters>, Args...>>* = nullptr>
    iir_state(Args&&... args) : params(std::forward<Args>(args)...)
    {
    }

    biquad_state<T, filters> state; ///< Active delay-line state.
    biquad_state<T, filters> saved_state; ///< Snapshot used for block-based processing.
    size_t block_end = 0; ///< End index of the current processing block.
};

template <typename T, size_t filters>
iir_state(const iir_params<T, filters>&) -> iir_state<T, filters>;
template <typename T, size_t filters>
iir_state(iir_params<T, filters>&&) -> iir_state<T, filters>;

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Template expression applying a cascade of biquad sections (look-ahead variant, no latency
 * compensation).
 *
 * This is the look-ahead variant of the biquad cascade. Like expression_iir it uses a
 * look-ahead optimization (priming the filter with future samples) for faster block
 * processing, **but it does not compensate for the group delay introduced by the
 * cascade**. As a result the output is shifted earlier in time by `filters - 1` samples
 * relative to the input: the first `filters - 1` output samples correspond to the
 * filter's transient response to the look-ahead priming, not to the input at index 0.
 *
 * Use this variant only when the time shift is acceptable (e.g. when the result will be
 * re-aligned by the caller, or for internal/block-internal use). For a phase-aligned
 * output that matches the input indexing, use expression_iir instead.
 *
 * @tparam filters Number of cascaded biquad sections. The look-ahead priming uses
 *                 `filters - 1` samples.
 * @tparam T Floating-point element type.
 * @tparam E1 Type of the wrapped input expression.
 * @tparam Stateless If true, the state is held by reference (external ownership).
 */
template <size_t filters, typename T, typename E1, bool Stateless = false>
struct expression_iir_l : public expression_with_traits<E1>
{
    using value_type = T;

    /**
     * @brief Construct from an input expression and a state holder.
     * @param e1 Input expression.
     * @param state Filter state holder.
     */
    expression_iir_l(E1&& e1, state_holder<iir_state<T, filters>, Stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    mutable state_holder<iir_state<T, filters>, Stateless> state;
};

/**
 * @brief Template expression applying a cascade of biquad sections (look-ahead variant, with latency
 * compensation).
 *
 * This is the latency-compensated variant of the biquad cascade. It uses the same
 * look-ahead optimization as expression_iir_l (priming the filter with future samples
 * for faster block processing), **but it compensates for the cascade group delay** so
 * that output sample `i` corresponds to the filter's response to input sample `i`.
 *
 * The compensation works by:
 *  - begin_pass: priming the state with the first `filters - 1` input samples and
 *                recording `block_end`;
 *  - get_elements: reading the input at `index + (filters - 1)` (so the look-ahead
 *                window is consumed internally) and snapshotting the state at the
 *                block boundary;
 *  - end_pass: restoring the snapshotted state so the next block starts from the
 *                correct boundary conditions.
 *
 * The effective latency is `filters - 1` samples, which is absorbed internally and
 * not visible in the output indexing. Use this variant when the output must stay
 * aligned with the input (the common case). When the time shift is acceptable, the
 * non-compensated expression_iir_l may be slightly cheaper.
 *
 * @tparam filters Number of cascaded biquad sections. The compensated latency is
 *                 `filters - 1` samples.
 * @tparam T Floating-point element type.
 * @tparam E1 Type of the wrapped input expression.
 * @tparam Stateless If true, the state is held by reference (external ownership).
 */
template <size_t filters, typename T, typename E1, bool Stateless = false>
struct expression_iir : expression_with_traits<E1>
{
    using value_type = T;

    /**
     * @brief Construct from an input expression and a state holder.
     * @param e1 Input expression.
     * @param state Filter state holder.
     */
    expression_iir(E1&& e1, state_holder<iir_state<T, filters>, Stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    mutable state_holder<iir_state<T, filters>, Stateless> state;
};

namespace internal
{

template <size_t filters, typename T>
KFR_INTRINSIC T biquad_process(vec<T, filters>& out, const iir_params<T, filters>& bq,
                               biquad_state<T, filters>& state, std::type_identity_t<T> in0,
                               const vec<T, filters>& delayline)
{
    vec<T, filters> in = insertleft(in0, delayline);
    out                = bq.b0 * in + state.s1;
    state.s1           = state.s2 + bq.b1 * in - bq.a1 * out;
    state.s2           = bq.b2 * in - bq.a2 * out;
    return out[filters - 1];
}
template <size_t filters, typename T, size_t N>
KFR_INTRINSIC vec<T, N> biquad_process(iir_state<T, filters>& state, const vec<T, N>& in,
                                       size_t save_state_after = static_cast<size_t>(-1))
{
    vec<T, N> out;
    if (KFR_LIKELY(save_state_after == static_cast<size_t>(-1)))
    {
        KFR_LOOP_UNROLL
        for (size_t i = 0; i < N; i++)
        {
            out[i] = biquad_process(state.state.out, state.params, state.state, in[i], state.state.out);
        }
    }
    else
    {
        for (size_t i = 0; i < save_state_after; i++)
        {
            out[i] = biquad_process(state.state.out, state.params, state.state, in[i], state.state.out);
        }
        state.saved_state = state.state;
        for (size_t i = save_state_after; i < N; i++)
        {
            out[i] = biquad_process(state.state.out, state.params, state.state, in[i], state.state.out);
        }
    }
    return out;
}
} // namespace internal

/// Internal ADL-provided implementation for `expression_iir_l` expressions.
template <size_t filters, typename T, typename E1, bool Stateless, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_iir_l<filters, T, E1, Stateless>& self, shape<1> index,
                                     axis_params<0, N> t)
{
    const vec<T, N> in = get_elements(self.first(), index, t);
    return internal::biquad_process(*self.state, in);
}

/// Internal ADL-provided implementation for `expression_iir` expressions.
template <typename T, typename E1, bool Stateless>
KFR_INTRINSIC void begin_pass(const expression_iir<1, T, E1, Stateless>&, shape<1>, shape<1>)
{
}
/// Internal ADL-provided implementation for `expression_iir` expressions.
template <size_t filters, typename T, typename E1, bool Stateless>
KFR_INTRINSIC void begin_pass(const expression_iir<filters, T, E1, Stateless>& self, shape<1> start,
                              shape<1> stop)
{
    size_t size           = stop.front();
    self.state->block_end = size;
    vec<T, filters - 1> in;
    for (index_t i = 0; i < filters - 1; i++)
    {
        in[i] = i < size ? get_elements(self.first(), shape<1>{ i }, axis_params_v<0, 1>).front() : 0;
    }
    internal::biquad_process(*self.state, in);
}

/// Internal ADL-provided implementation for `expression_iir` expressions.
template <typename T, typename E1, bool Stateless>
KFR_INTRINSIC void end_pass(const expression_iir<1, T, E1, Stateless>&, shape<1>, shape<1>)
{
}
/// Internal ADL-provided implementation for `expression_iir` expressions.
template <size_t filters, typename T, typename E1, bool Stateless>
KFR_INTRINSIC void end_pass(const expression_iir<filters, T, E1, Stateless>& self, shape<1> start,
                            shape<1> stop)
{
    self.state->state = self.state->saved_state;
}

/// Internal ADL-provided implementation for `expression_iir` expressions.
template <typename T, typename E1, bool Stateless, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_iir<1, T, E1, Stateless>& self, shape<1> index,
                                     axis_params<0, N> t)
{
    const vec<T, N> in = get_elements(self.first(), index, t);
    return internal::biquad_process(*self.state, in);
}

/// Internal ADL-provided implementation for `expression_iir` expressions.
template <size_t filters, typename T, typename E1, bool Stateless, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_iir<filters, T, E1, Stateless>& self, shape<1> index,
                                     axis_params<0, N> t)
{
    using internal::biquad_process;
    index.front() += filters - 1;
    vec<T, N> out{};
    if (index.front() + N <= self.state->block_end)
    {
        const vec<T, N> in = get_elements(self.first(), shape<1>{ index.front() }, t);

        out = biquad_process(*self.state, in);
        if (index.front() + N == self.state->block_end)
            self.state->saved_state = self.state->state;
    }
    else if (index.front() >= self.state->block_end)
    {
        out = biquad_process(*self.state, vec<T, N>(0));
    }
    else
    {
        size_t save_at = std::min(N, self.state->block_end - static_cast<size_t>(index.front()));
        vec<T, N> in;
        for (size_t i = 0; i < save_at; ++i)
            in[i] =
                get_elements(self.first(), index.add_at(i, cval<index_t, 0>), axis_params_v<0, 1>).front();
        for (size_t i = save_at; i < N; ++i)
            in[i] = 0;
        out = biquad_process(*self.state, in, save_at);
    }
    return out;
}

/**
 * @brief Returns a template expression that applies a biquad filter cascade to the input.
 *
 * The returned expression owns its filter state. The number of sections is fixed at
 * compile time via `filters`.
 *
 * @tparam filters Number of cascaded biquad sections.
 * @tparam T Floating-point element type.
 * @tparam E1 Type of the input expression.
 * @param e1 Input expression.
 * @param params Biquad coefficients (iir_params).
 */
template <size_t filters, typename T, typename E1>
KFR_FUNCTION expression_iir<filters, T, E1> iir(E1&& e1, iir_params<T, filters> params)
{
    return expression_iir<filters, T, E1>(std::forward<E1>(e1), iir_state{ std::move(params) });
}

/**
 * @brief Returns a template expression that applies a runtime-sized biquad cascade to the input.
 *
 * The runtime section count is rounded up to the next power of two (bounded by
 * maximum_biquad_count) and dispatched to the matching fixed-size specialization.
 *
 * @tparam T Floating-point element type.
 * @tparam E1 Type of the input expression.
 * @param e1 Input expression.
 * @param params Dynamic-size biquad coefficients.
 */
template <typename T, typename E1>
KFR_FUNCTION expression_handle<T, 1> iir(E1&& e1, const iir_params<T, tag_dynamic_vector>& params)
{
    KFR_LOGIC_CHECK(next_poweroftwo(params.size()) <= maximum_biquad_count, "iir: too many biquad sections");
    return cswitch(
        internal_generic::biquad_sizes, next_poweroftwo(params.size()),
        [&](auto x)
        {
            constexpr size_t filters = x;
            return to_handle(expression_iir<filters, T, E1>(
                std::forward<E1>(e1), iir_state{ iir_params<T, filters>(params.data(), params.size()) }));
        },
        [&] { return to_handle(fixshape(zeros<T>(), fixed_shape<infinite_size>)); });
}

/**
 * @brief Returns a template expression that applies a biquad cascade using externally-owned state.
 *
 * The state is held by reference (Stateless = true), allowing the caller to share or
 * reset the filter state across multiple invocations.
 *
 * @tparam filters Number of cascaded biquad sections.
 * @tparam T Floating-point element type.
 * @tparam E1 Type of the input expression.
 * @param state Reference wrapper around an externally-owned iir_state.
 * @param e1 Input expression.
 */
template <size_t filters, typename T, typename E1>
KFR_FUNCTION expression_iir<filters, T, E1, true> iir(E1&& e1,
                                                      std::reference_wrapper<iir_state<T, filters>> state)
{
    return expression_iir<filters, T, E1, true>(std::forward<E1>(e1), state);
}

/**
 * @brief Applies forward and backward filtering to the input array using the given IIR filter parameters.
 *
 * This function performs zero-phase filtering by first applying the IIR filter in the forward direction
 * and then applying it again in the reverse direction. The result is a filtered signal with minimal phase
 * distortion.
 *
 * @tparam T The data type of the elements in the input array.
 * @tparam Tag The tag type associated with the input array.
 * @tparam Itag The tag type associated with the IIR filter parameters.
 *
 * @param arr The input array to be filtered. This array is modified in-place to store the filtered result.
 * @param params The IIR filter parameters used for filtering the input array.
 */
template <typename T, univector_tag Tag, size_t Itag>
KFR_FUNCTION void filtfilt(univector<T, Tag>& arr, const iir_params<T, Itag>& params)
{
    // Forward pass
    arr = iir(arr, params);
    // Backward pass
    process(reverse(arr), iir(reverse(arr), params));
}

/// Alias for expression_iir_l (look-ahead biquad cascade expression).
template <size_t filters, typename T, typename E1>
using expression_biquads_l = expression_iir_l<filters, T, E1>;

/// Alias for expression_iir (sample-by-sample biquad cascade expression).
template <size_t filters, typename T, typename E1>
using expression_biquads = expression_iir<filters, T, E1>;

} // namespace KFR_ARCH_NAME

/**
 * @brief Runtime IIR filter object usable as a streaming filter.
 *
 * Wraps an iir expression built from dynamic-size iir_params. The actual
 * expression is constructed via the multi-arch `create_iir_filter` helper
 * (see src/dsp/biquad.cpp), so when KFR's multi-architecture support is
 * enabled the best SIMD implementation for the running CPU is selected at
 * runtime (dynamic CPU dispatch). On modern CPUs this can make `iir_filter`
 * faster than manually calling `iir` or building an `expression_iir`
 * directly, since the latter are resolved at compile time against the
 * currently selected architecture only.
 *
 * @tparam T Floating-point element type.
 */
template <typename T>
class iir_filter : public expression_filter<T>
{
public:
    /**
     * @brief Construct the filter from biquad parameters.
     * @param params Biquad coefficients (dynamic-size iir_params).
     */
    iir_filter(const iir_params<T>& params);
};
} // namespace kfr
