/** @addtogroup fir
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

#include "../base/basic_expressions.hpp"
#include "../base/filter.hpp"
#include "../base/memory.hpp"
#include "../base/reduce.hpp"
#include "../base/simd_expressions.hpp"
#include "../base/state_holder.hpp"
#include "../base/univector.hpp"
#include "../simd/vec.hpp"

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4244))

namespace kfr
{

/**
 * @brief Alias for a fixed-size vector of FIR filter coefficients.
 * @tparam T    Coefficient/value type.
 * @tparam Size Number of taps.
 */
template <typename T, size_t Size>
using fir_taps = univector<T, Size>;

/**
 * @brief State for a short FIR filter (tap count in range 2..33).
 *
 * Stores the (zero-padded) taps and a delay line of `tapcount - 1` samples.
 * The taps are stored in natural order; the convolution reverses them
 * implicitly via the `concat_and_slice` logic in `expression_short_fir`.
 *
 * @tparam tapcount Number of taps (rounded up to the next valid internal size).
 * @tparam T        Coefficient type.
 * @tparam U        Sample/value type (defaults to T). May differ from T, e.g.
 *                  for complex-valued samples with real taps.
 */
template <size_t tapcount, typename T, typename U = T>
struct short_fir_state
{
    /**
     * @brief Construct from a runtime-sized tap vector.
     * @tparam N Size of the input tap vector.
     * @param taps Filter coefficients (zero-padded to `tapcount`).
     */
    template <size_t N>
    short_fir_state(const univector<T, N>& taps)
        : taps(widen<tapcount>(read<N>(taps.data()), T(0))), delayline(0)
    {
    }
    /**
     * @brief Construct from a runtime-sized tap vector (const data variant).
     * @tparam N Size of the input tap vector.
     * @param taps Filter coefficients (zero-padded to `tapcount`).
     */
    template <size_t N>
    short_fir_state(const univector<const T, N>& taps)
        : taps(widen<tapcount>(read<N>(taps.data()), T(0))), delayline(0)
    {
    }
    vec<T, tapcount> taps; ///< Filter coefficients (zero-padded).
    vec<U, tapcount - 1> delayline; ///< Delay line holding the last `tapcount - 1` samples.
};

/**
 * @brief Parameters of a generic FIR filter: the tap vector.
 *
 * The taps are stored in **reversed** order so that the convolution can be
 * performed as a dot product against the most recent samples first.
 *
 * @tparam T Coefficient type.
 */
template <typename T>
struct fir_params
{
    univector<T> taps; ///< Filter coefficients (reversed order).

    fir_params(const fir_params&)            = default;
    fir_params(fir_params&&)                 = default;
    fir_params& operator=(const fir_params&) = default;
    fir_params& operator=(fir_params&&)      = default;

    /**
     * @brief Construct from a raw pointer and size (taps are reversed internally).
     * @param data Pointer to the tap coefficients.
     * @param size Number of taps.
     */
    fir_params(const T* data, size_t size) : taps(reverse(make_univector(data, size))) {}

    /**
     * @brief Construct by moving a tap vector (taps are reversed internally).
     * @param taps Filter coefficients (moved in, then reversed).
     */
    fir_params(univector<T>&& taps) : taps(std::move(taps))
    {
        std::reverse(this->taps.begin(), this->taps.end());
    }

    /**
     * @brief Construct from any container satisfying `has_data_size`.
     * @tparam Cont Container type.
     * @param taps Container with the filter coefficients.
     */
    template <has_data_size Cont>
    fir_params(Cont&& taps) : fir_params(std::data(taps), std::size(taps))
    {
    }
};

/**
 * @brief Deduction guide for `fir_params` from an arbitrary container.
 */
template <typename Cont>
fir_params(Cont&&) -> fir_params<std::remove_const_t<container_value_type<Cont>>>;

/**
 * @brief State of a generic FIR filter: parameters plus a ring-buffer delay line.
 *
 * The delay line is a ring buffer addressed through `delayline_cursor`.
 * Complex sample types are supported (e.g. `U = complex<float>`).
 *
 * @tparam T Coefficient type.
 * @tparam U Sample/value type (defaults to T). May be a complex type.
 */
template <typename T, typename U = T>
struct fir_state
{
    fir_state(const fir_state&)            = default;
    fir_state(fir_state&&)                 = default;
    fir_state& operator=(const fir_state&) = default;
    fir_state& operator=(fir_state&&)      = default;

    /**
     * @brief Construct from `fir_params` (taps are taken as-is, already reversed).
     * @param params Filter parameters.
     */
    fir_state(fir_params<T> params)
        : params(std::move(params)), delayline(this->params.taps.size(), U(0)), delayline_cursor(0)
    {
    }
    /**
     * @brief Construct from a tap container (taps are reversed by `fir_params`).
     * @tparam Cont Container type.
     * @param taps Filter coefficients.
     */
    template <has_data_size Cont>
    fir_state(Cont&& taps) : params(std::move(taps)), delayline(params.taps.size(), U(0)), delayline_cursor(0)
    {
    }
    /**
     * @brief Append samples to the delay line ring buffer.
     * @tparam Cont Container type.
     * @param state Samples to push (most recent last).
     */
    template <has_data_size Cont>
    void push_delayline(Cont&& state)
    {
        delayline.ringbuf_write(delayline_cursor, std::data(state), std::size(state));
    }
    fir_params<T> params; ///< Filter parameters (reversed taps).
    univector<U> delayline; ///< Ring-buffer delay line of length `taps.size()`.
    size_t delayline_cursor; ///< Current write cursor into `delayline`.
};

/**
 * @brief Deduction guide for `fir_state` from an arbitrary container.
 */
template <typename Cont>
fir_state(Cont&&) -> fir_state<container_value_type<Cont>>;

/**
 * @brief State for a moving-sum (rectangular window) filter with a
 *        compile-time-sized delay line.
 *
 * @tparam U   Sample/value type.
 * @tparam Tag `univector_tag` describing the storage kind (default dynamic).
 */
template <typename U, univector_tag Tag = tag_dynamic_vector>
struct moving_sum_state
{
    moving_sum_state() : delayline({ 0 }), head_cursor(0), tail_cursor(1) {}
    univector<U, Tag> delayline; ///< Ring buffer holding the window contents.
    size_t head_cursor; ///< Write cursor (newest sample).
    size_t tail_cursor; ///< Read cursor (oldest sample).
};
/**
 * @brief Specialization of `moving_sum_state` for a runtime-sized window.
 *
 * @tparam U Sample/value type.
 */
template <typename U>
struct moving_sum_state<U, tag_dynamic_vector>
{
    /**
     * @brief Construct a moving-sum state with the given window length.
     * @param sum_length Number of samples in the summation window.
     */
    moving_sum_state(size_t sum_length) : delayline(sum_length, U(0)), head_cursor(0), tail_cursor(1) {}
    univector<U> delayline; ///< Ring buffer holding the window contents.
    size_t head_cursor; ///< Write cursor (newest sample).
    size_t tail_cursor; ///< Read cursor (oldest sample).
};

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Template expression applying a short FIR filter (2..33 taps).
 *
 * Uses SIMD-friendly fixed-size vectors for taps and delay line. The filter
 * state may be owned (`stateless == false`) or referenced (`stateless == true`).
 *
 * @tparam tapcount  Internal (padded) tap count.
 * @tparam T         Coefficient type.
 * @tparam U         Sample/value type (may be complex).
 * @tparam E1        Input expression type.
 * @tparam stateless If `true`, the state is held by reference.
 */
template <size_t tapcount, typename T, typename U, typename E1, bool stateless = false>
struct expression_short_fir : expression_with_traits<E1>
{
    using value_type = U; // override value_type

    static_assert(expression_traits<E1>::dims == 1, "expression_short_fir requires input with dims == 1");
    constexpr static inline bool random_access = false;

    expression_short_fir(E1&& e1, state_holder<short_fir_state<tapcount, T, U>, stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    /**
     * @brief Internal ADL-provided implementation for `expression_short_fir` expressions.
     */
    template <size_t N>
    KFR_INTRINSIC friend vec<U, N> get_elements(const expression_short_fir& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        vec<U, N> in = get_elements(self.first(), index, sh);

        vec<U, N> out = in * self.state->taps.front();
        cforeach(csizeseq<tapcount - 1, 1>,
                 [&](auto I) {
                     out = out + concat_and_slice<tapcount - 1 - I, N>(self.state->delayline, in) *
                                     self.state->taps[I];
                 });
        self.state->delayline = concat_and_slice<N, tapcount - 1>(self.state->delayline, in);

        return out;
    }
    mutable state_holder<short_fir_state<tapcount, T, U>, stateless> state;
};

/**
 * @brief Template expression applying a generic FIR filter of arbitrary length.
 *
 * Uses a ring-buffer delay line and per-sample dot products. The filter state
 * may be owned (`stateless == false`) or referenced (`stateless == true`).
 * Complex sample types are supported.
 *
 * @tparam T         Coefficient type.
 * @tparam U         Sample/value type (may be complex).
 * @tparam E1        Input expression type.
 * @tparam stateless If `true`, the state is held by reference.
 */
template <typename T, typename U, typename E1, bool stateless = false>
struct expression_fir : expression_with_traits<E1>
{
    using value_type = U; // override value_type

    static_assert(expression_traits<E1>::dims == 1, "expression_fir requires input with dims == 1");
    constexpr static inline bool random_access = false;

    expression_fir(E1&& e1, state_holder<fir_state<T, U>, stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    /**
     * @brief Internal ADL-provided implementation for `expression_fir` expressions.
     */
    template <size_t N>
    KFR_INTRINSIC friend vec<U, N> get_elements(const expression_fir& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const size_t tapcount = self.state->params.taps.size();
        const vec<U, N> input = get_elements(self.first(), index, sh);

        vec<U, N> output;
        size_t cursor = self.state->delayline_cursor;
        KFR_LOOP_NOUNROLL
        for (size_t i = 0; i < N; i++)
        {
            self.state->delayline.ringbuf_write(cursor, input[i]);
            U v = dotproduct(self.state->params.taps.slice(0, tapcount - cursor),
                             self.state->delayline.slice(cursor));
            if (cursor > 0)
                v = v + dotproduct(self.state->params.taps.slice(tapcount - cursor),
                                   self.state->delayline.slice(0, cursor));
            output[i] = v;
        }
        self.state->delayline_cursor = cursor;
        return output;
    }
    mutable state_holder<fir_state<T, U>, stateless> state;
};

/**
 * @brief Template expression performing a moving sum (rectangular window)
 *        over the input.
 *
 * Maintains a running sum updated incrementally as samples enter and leave
 * the window. The state may be owned or referenced.
 *
 * @tparam U         Sample/value type.
 * @tparam E1        Input expression type.
 * @tparam STag      `univector_tag` of the delay line storage.
 * @tparam stateless If `true`, the state is held by reference.
 */
template <typename U, typename E1, univector_tag STag, bool stateless = false>
struct expression_moving_sum : expression_with_traits<E1>
{
    using value_type = U; // override value_type

    static_assert(expression_traits<E1>::dims == 1, "expression_moving_sum requires input with dims == 1");
    constexpr static inline bool random_access = false;

    expression_moving_sum(E1&& e1, state_holder<moving_sum_state<U, STag>, stateless> state)
        : expression_with_traits<E1>(std::forward<E1>(e1)), state(std::move(state))
    {
    }

    /**
     * @brief Internal ADL-provided implementation for `expression_moving_sum` expressions.
     */
    template <size_t N>
    KFR_INTRINSIC friend vec<U, N> get_elements(const expression_moving_sum& self, shape<1> index,
                                                axis_params<0, N> sh)
    {
        const vec<U, N> input = get_elements(self.first(), index, sh);

        vec<U, N> output;
        size_t wcursor = self.state->head_cursor;
        size_t rcursor = self.state->tail_cursor;

        // initial summation
        self.state->delayline.ringbuf_write(wcursor, input[0]);
        auto s    = sum(self.state->delayline);
        output[0] = s;

        KFR_LOOP_NOUNROLL
        for (size_t i = 1; i < N; i++)
        {
            U nextout;
            self.state->delayline.ringbuf_read(rcursor, nextout);
            const U nextin = input[i];
            self.state->delayline.ringbuf_write(wcursor, nextin);
            s += nextin - nextout;
            output[i] = s;
        }
        self.state->delayline.ringbuf_step(rcursor, 1);
        self.state->head_cursor = wcursor;
        self.state->tail_cursor = rcursor;
        return output;
    }
    mutable state_holder<moving_sum_state<U, STag>, stateless> state;
};

/**
 * @brief Returns template expression that applies FIR filter to the input.
 * @deprecated Use `fir(expr, fir_params{taps})` instead.
 * @param e1   An input expression.
 * @param taps Filter coefficients (taken by value).
 */
template <typename E1, typename U = expression_value_type<E1>, typename Taps,
          typename T = std::remove_cv_t<container_value_type<Taps>>>
[[deprecated("fir(expr, taps) is deprecated. Use fir(expr, fir_params{taps})")]] KFR_INTRINSIC expression_fir<
    T, U, E1, false>
fir(E1&& e1, Taps&& taps)
{
    return expression_fir<T, U, E1, false>(std::forward<E1>(e1), fir_state<T, U>{ std::forward<Taps>(taps) });
}

/**
 * @brief Returns template expression that applies FIR filter to the input.
 *
 * The filter state is owned by the returned expression. Complex sample types
 * are supported when `U` is a complex type.
 *
 * @param e1    An input expression.
 * @param state Filter parameters (taps are reversed internally).
 */
template <typename T, typename E1, typename U = expression_value_type<E1>>
KFR_INTRINSIC expression_fir<T, U, E1, false> fir(E1&& e1, fir_params<T> state)
{
    return expression_fir<T, U, E1, false>(std::forward<E1>(e1), fir_state<T, U>{ std::move(state) });
}

/**
 * @brief Returns template expression that applies FIR filter to the input.
 *
 * The filter state is referenced by the returned expression; the caller must
 * keep `state` alive for the lifetime of the expression. This allows reuse of
 * the same state across multiple calls (streaming filtering).
 *
 * @param e1    An input expression.
 * @param state Filter state (taken by reference, ensure proper lifetime).
 */
template <typename T, typename E1, typename U>
KFR_INTRINSIC expression_fir<T, U, E1, true> fir(E1&& e1, std::reference_wrapper<fir_state<T, U>> state)
{
    static_assert(std::is_same_v<U, expression_value_type<E1>>, "fir: type mismatch");
    return expression_fir<T, U, E1, true>(std::forward<E1>(e1), state);
}

/**
 * @brief Returns template expression that applies FIR filter to the input.
 * @deprecated Use `fir(expr, std::ref(state))` instead.
 * @param state FIR filter state (state is referenced, ensure proper lifetime).
 * @param e1    An input expression.
 */
template <typename T, typename U, typename E1>
[[deprecated("fir(state, expr) is deprecated. Use fir(expr, std::ref(state))")]] KFR_INTRINSIC expression_fir<
    T, U, E1, true>
fir(fir_state<T, U>& state, E1&& e1)
{
    return fir(std::forward<E1>(e1), std::reference_wrapper<fir_state<T, U>>(state));
}

/**
 * @brief Returns template expression that performs moving sum on the input.
 * @param e1         An input expression.
 * @param sum_length Number of samples in the summation window.
 */
template <typename E1>
KFR_INTRINSIC expression_moving_sum<expression_value_type<E1>, E1, tag_dynamic_vector> moving_sum(
    E1&& e1, size_t sum_length)
{
    return expression_moving_sum<expression_value_type<E1>, E1, tag_dynamic_vector>(
        std::forward<E1>(e1), moving_sum_state<expression_value_type<E1>, tag_dynamic_vector>{ sum_length });
}

/**
 * @brief Returns template expression that performs moving sum on the input.
 * @deprecated Use `moving_sum(expr, len)` instead.
 * @tparam sum_length Number of samples in the summation window.
 * @param e1          An input expression.
 */
template <size_t sum_length, typename E1>
[[deprecated("moving_sum<len> is deprecated. Use moving_sum(expr, len) instead")]] KFR_INTRINSIC
    expression_moving_sum<expression_value_type<E1>, E1, tag_dynamic_vector>
    moving_sum(E1&& e1)
{
    return expression_moving_sum<expression_value_type<E1>, E1, tag_dynamic_vector>(
        std::forward<E1>(e1), moving_sum_state<expression_value_type<E1>, tag_dynamic_vector>{ sum_length });
}

/**
 * @brief Returns template expression that performs moving sum on the input.
 *
 * The state is referenced by the returned expression; the caller must keep
 * `state` alive for the lifetime of the expression.
 *
 * @param e1    An input expression.
 * @param state Moving sum state (taken by reference).
 */
template <typename E1, typename U, size_t Tag>
KFR_INTRINSIC expression_moving_sum<U, E1, Tag, true> moving_sum(
    E1&& e1, std::reference_wrapper<moving_sum_state<U, Tag>> state)
{
    return expression_moving_sum<expression_value_type<E1>, E1, Tag, true>(std::forward<E1>(e1), state);
}

/**
 * @brief Returns template expression that performs moving sum on the input.
 * @deprecated Use `moving_sum(expr, std::ref(state))` instead.
 * @param state Moving sum state.
 * @param e1    An input expression.
 */
template <typename U, typename E1, univector_tag STag>
[[deprecated("moving_sum(state, expr) is deprecated. Use moving_sum(expr, std::ref(state)) "
             "instead")]] KFR_INTRINSIC expression_moving_sum<U, E1, STag, true>
moving_sum(moving_sum_state<U, STag>& state, E1&& e1)
{
    return moving_sum(std::forward<E1>(e1), std::ref(state));
}

/**
 * @brief Returns template expression that applies a short FIR filter to the
 *        input (tap count must be in range 2..33).
 *
 * The filter state is owned by the returned expression. `short_fir` is
 * optimized for small tap counts and uses SIMD-friendly fixed-size vectors.
 *
 * @tparam T              Coefficient type.
 * @tparam TapCount       Number of taps (2..33).
 * @tparam E1             Input expression type.
 * @tparam InternalTapCount Padded internal tap count (auto-derived).
 * @tparam U              Sample/value type (may be complex).
 * @param e1   An input expression.
 * @param taps Filter coefficients.
 */
template <typename T, size_t TapCount, typename E1,
          size_t InternalTapCount = next_poweroftwo(TapCount - 1) + 1, typename U = expression_value_type<E1>>
KFR_INTRINSIC expression_short_fir<InternalTapCount, T, U, E1, false> short_fir(
    E1&& e1, const univector<T, TapCount>& taps)
{
    static_assert(TapCount >= 2 && TapCount <= 33, "Use short_fir only for small FIR filters");
    return expression_short_fir<InternalTapCount, T, U, E1, false>(
        std::forward<E1>(e1), short_fir_state<InternalTapCount, T, U>{ taps });
}
/**
 * @brief Returns template expression that applies a short FIR filter to the
 *        input (tap count must be in range 2..33).
 *
 * The filter state is referenced by the returned expression; the caller must
 * keep `state` alive for the lifetime of the expression.
 *
 * @tparam T         Coefficient type.
 * @tparam TapCount  Number of taps (2..33).
 * @tparam E1        Input expression type.
 * @tparam U         Sample/value type (may be complex).
 * @param e1    An input expression.
 * @param state Filter state (state is referenced, ensure proper lifetime).
 */
template <typename T, size_t TapCount, typename E1, typename U>
KFR_INTRINSIC expression_short_fir<TapCount, T, U, E1, true> short_fir(
    E1&& e1, std::reference_wrapper<short_fir_state<TapCount, T, U>> state)
{
    static_assert(std::is_same_v<U, expression_value_type<E1>>, "short_fir: type mismatch");
    static_assert(TapCount >= 2 && TapCount <= 33, "Use short_fir only for small FIR filters");
    return expression_short_fir<TapCount, T, U, E1, true>(std::forward<E1>(e1), state);
}

/**
 * @brief Returns template expression that applies a short FIR filter to the
 *        input (tap count must be in range 2..33).
 * @deprecated Use `short_fir(expr, std::ref(state))` instead.
 * @tparam TapCount          Number of taps.
 * @tparam InternalTapCount  Padded internal tap count (auto-derived).
 * @tparam T                 Coefficient type.
 * @tparam U                 Sample/value type.
 * @tparam E1                Input expression type.
 * @param state Filter state.
 * @param e1    An input expression.
 */
template <size_t TapCount, size_t InternalTapCount = next_poweroftwo(TapCount - 1) + 1, typename T,
          typename U, typename E1>
[[deprecated("short_fir(state, expr) is deprecated, use short_fir(expr, std::ref(state))")]] KFR_INTRINSIC
    expression_short_fir<InternalTapCount, T, expression_value_type<E1>, E1, true>
    short_fir(short_fir_state<InternalTapCount, T, U>& state, E1&& e1)
{
    static_assert(InternalTapCount == next_poweroftwo(TapCount - 1) + 1, "short_fir: TapCount mismatch");
    return short_fir(std::forward<E1>(e1), std::ref(state));
}

} // namespace KFR_ARCH_NAME

/**
 * @brief Runtime FIR filter implementing the `filter<U>` interface.
 *
 * This class is the recommended way to apply an FIR filter to a buffer when
 * streaming or when an opaque `filter` interface is required. When KFR's
 * multi-architecture (dynamic CPU dispatch) support is enabled globally, the
 * implementation of `process_buffer`/`process_expression` is selected at
 * runtime for the host CPU and may be faster than manually calling the
 * `fir` free function or using `expression_fir` directly, especially on
 * modern CPUs with wide SIMD units.
 *
 * Complex sample types are supported (e.g. `fir_filter<float, complex<float>>`).
 *
 * @note For long filters, the KFR DFT module provides `convolve_filter`,
 *       which performs the same convolution math using FFT-based overlap-add
 *       and is significantly faster for large tap counts.
 *
 * @tparam T Coefficient type.
 * @tparam U Sample/value type (defaults to T; may be complex).
 */
template <typename T, typename U = T>
class fir_filter : public filter<U>
{
public:
    /**
     * @brief Construct from a `fir_state`.
     * @param state Filter state (moved in).
     */
    fir_filter(fir_state<T, U> state) : state(std::move(state)) {}

    /// Replace the filter taps (resets parameters; delay line is unchanged until next process).
    void set_taps(fir_params<T> params) { state = std::move(params); }
    /// Replace the filter parameters (alias of `set_taps`).
    void set_params(fir_params<T> params) { state = std::move(params); }

    /// Reset internal filter state
    void reset() final
    {
        state.delayline        = scalar(0);
        state.delayline_cursor = 0;
    }

protected:
    void process_buffer(U* dest, const U* src, size_t size) final;
    void process_expression(U* dest, const expression_handle<U, 1>& src, size_t size) final;

    fir_state<T, U> state;
};

/// Alias for `fir_filter`.
template <typename T, typename U = T>
using filter_fir = fir_filter<T, U>;

} // namespace kfr

KFR_PRAGMA_MSVC(warning(pop))
