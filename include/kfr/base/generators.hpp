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

#include "../math/log_exp.hpp"
#include "../math/sin_cos.hpp"
#include "../simd/complex.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/select.hpp"
#include "../simd/vec.hpp"
#include "expression.hpp"
#include "shape.hpp"

namespace kfr
{

inline namespace KFR_ARCH_NAME
{

namespace internal
{
/**
 * @brief Returns the vector width to use for a generator producing values of
 * type @p T, derived from the platform's vector capacity and a @p divisor that
 * accounts for the per-element work performed by the generator.
 * @tparam T Value (or complex) type produced by the generator.
 * @param divisor Per-element cost divisor used to scale the native vector width.
 */
template <typename T>
constexpr size_t generator_width(size_t divisor)
{
    return std::max(size_t(1), vector_capacity<deep_subtype<T>> / 8 / divisor);
}
} // namespace internal

/**
 * @brief CRTP base for stateful streaming generators that produce fixed-width
 * vectors of values on demand.
 *
 * A generator holds the current vector of values in @ref value and advances it
 * by calling `next()` on the derived class @p Class. The derived class must
 * implement `sync(start)` to reinitialize the state at a given starting point,
 * `next()` to advance the state by one vector, and `get_value()` to return the
 * current vector.
 *
 * Generators are expressions of infinite length and are not random-access.
 *
 * @tparam T Value type produced by the generator.
 * @tparam VecWidth Native vector width used by the generator.
 * @tparam Class Derived generator type (CRTP).
 * @tparam Twork Working type used to store the internal state; defaults to @p T
 *               but may differ (e.g. interleaved cos/sin pairs for `gen_sin`).
 */
template <typename T, size_t VecWidth, typename Class, typename Twork = T>
struct generator : public expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;
    constexpr static shape<1> get_shape(const Class&) { return infinite_size; }
    constexpr static shape<1> get_shape() { return infinite_size; }

    constexpr static inline bool random_access = false;

    constexpr static size_t width = VecWidth;

    /**
     * @brief Reinitializes the generator state so that the next produced value
     * corresponds to the given @p start.
     * @param start Starting value to synchronize the generator to.
     */
    void resync(T start) const { ptr_cast<Class>(this)->sync(start); }

    /**
     * @brief Produces a vector of @p N values from the generator and advances
     * its internal state.
     *
     * For @p N smaller than the native width, a narrow slice of the current
     * state is returned and the state is rotated so that subsequent calls
     * continue the sequence. For @p N larger than the native width, the request
     * is split into power-of-two sized sub-vectors that are concatenated.
     * @tparam N Number of values to produce.
     * @return Vector of @p N generated values.
     */
    template <size_t N>
    KFR_MEM_INTRINSIC vec<T, N> generate() const
    {
        if constexpr (N < width)
        {
            const vec<T, N> result           = narrow<N>(call_get_value());
            const vec<Twork, width> oldvalue = value;
            call_next();
            value = slice<N, width>(oldvalue, value);
            return result;
        }
        else if constexpr (N > width)
        {
            constexpr size_t Nlow = prev_poweroftwo(N - 1);
            const vec lo          = generate<Nlow>();
            const vec hi          = generate<N - Nlow>();
            return concat(lo, hi);
        }
        else // N == width
        {
            const vec<T, N> result = call_get_value();
            call_next();
            return result;
        }
    }
    mutable vec<Twork, width> value;

    /**
     * @brief Internal ADL-provided implementation for `generator` expressions.
     * Returns a vector of @p N generated values at the given @p index.
     */
    template <size_t N>
    friend KFR_INTRINSIC vec<T, N> get_elements(const generator& self, const shape<1>& index,
                                                const axis_params<0, N>&)
    {
        return self.template generate<N>();
    }

private:
    KFR_MEM_INTRINSIC void call_next() const { ptr_cast<Class>(this)->next(); }

    KFR_MEM_INTRINSIC vec<T, width> call_get_value() const { return ptr_cast<Class>(this)->get_value(); }

    template <typename U = T>
        requires(std::is_same_v<U, Twork>)
    KFR_MEM_INTRINSIC vec<T, width> get_value() const
    {
        return value;
    }
};

/**
 * @brief Generator that produces an arithmetic progression
 * \f$ x_i = start + i \cdot step \f$.
 *
 * The state is advanced in steps of `VecWidth * step` per vector so that each
 * generated vector is contiguous with the previous one.
 *
 * @tparam T Value type.
 * @tparam VecWidth Native vector width.
 */
template <typename T, size_t VecWidth = internal::generator_width<T>(1)>
struct generator_linear : public generator<T, VecWidth, generator_linear<T, VecWidth>>
{
    /**
     * @brief Constructs a linear generator.
     * @param start First value to produce.
     * @param step Increment between successive values.
     */
    generator_linear(T start, T step) noexcept : vstep{ step * VecWidth } { sync(start); }

    /**
     * @brief Reinitializes the generator so the next vector starts at @p start.
     * @param start Starting value.
     */
    KFR_MEM_INTRINSIC void sync(T start) const noexcept
    {
        this->value = start + enumerate(vec_shape<T, VecWidth>{}, vstep / VecWidth);
    }

    /**
     * @brief Advances the generator state by one native vector width.
     */
    KFR_MEM_INTRINSIC void next() const noexcept { this->value += vstep; }

    T vstep;
};

/**
 * @brief Generator that produces an exponential sequence
 * \f$ x_i = e^{ start + i \cdot step } \f$.
 *
 * The state is advanced multiplicatively using a precomputed per-vector growth
 * factor so that each generated vector is contiguous with the previous one.
 *
 * @tparam T Value type.
 * @tparam VecWidth Native vector width.
 */
template <typename T, size_t VecWidth = internal::generator_width<T>(1)>
struct generator_exp : public generator<T, VecWidth, generator_exp<T, VecWidth>>
{
    /**
     * @brief Constructs an exponential generator.
     * @param start Exponent of the first value to produce.
     * @param step Increment added to the exponent between successive values.
     */
    generator_exp(T start, T step) noexcept
        : step{ step }, vstep{ exp(make_vector(step * VecWidth)).front() - 1 }
    {
        this->resync(start);
    }

    /**
     * @brief Reinitializes the generator so the next vector starts at @p start.
     * @param start Exponent of the starting value.
     */
    KFR_MEM_INTRINSIC void sync(T start) const noexcept
    {
        this->value = exp(start + enumerate<T, VecWidth>() * step);
    }

    /**
     * @brief Advances the generator state by one native vector width using the
     * precomputed multiplicative growth factor.
     */
    KFR_MEM_INTRINSIC void next() const noexcept { this->value += this->value * vstep; }

protected:
    T step;
    T vstep;
};

/**
 * @brief Generator that produces a complex exponential
 * \f$ x_i = e^{ j ( start + i \cdot step ) } \f$.
 *
 * The complex rotation is advanced using a recurrence relation based on the
 * precomputed coefficients @ref alpha and @ref beta, avoiding repeated calls to
 * `sin`/`cos`.
 *
 * @tparam T Complex value type.
 * @tparam VecWidth Native vector width.
 */
template <typename T, size_t VecWidth = internal::generator_width<T>(2)>
struct generator_expj : public generator<T, VecWidth, generator_expj<T, VecWidth>>
{
    using ST = deep_subtype<T>;
    static_assert(std::is_same_v<complex<deep_subtype<T>>, T>, "generator_expj requires complex type");

    /**
     * @brief Constructs a complex exponential generator.
     * @param start_ Phase of the first value to produce.
     * @param step_ Phase increment between successive values.
     */
    generator_expj(ST start_, ST step_)
        : step(step_), alpha(2 * sqr(sin(VecWidth * step / 2))), beta(-sin(VecWidth * step))
    {
        this->resync(T(start_));
    }
    /**
     * @brief Reinitializes the generator so the next vector starts at phase
     * @p start.
     * @param start Starting phase (complex value).
     */
    KFR_MEM_INTRINSIC void sync(T start) const noexcept { this->value = init_cossin(step, start.real()); }

    /**
     * @brief Advances the complex rotation by one native vector width using the
     * recurrence relation.
     */
    KFR_MEM_INTRINSIC void next() const noexcept
    {
        this->value = ccomp(cdecom(this->value) -
                            subadd(alpha * cdecom(this->value), beta * swap<2>(cdecom(this->value))));
    }

protected:
    ST step;
    ST alpha;
    ST beta;
    /**
     * @brief Initializes a vector of complex exponentials with angular frequency
     * @p w and starting phase @p phase.
     * @param w Angular frequency increment per sample.
     * @param phase Starting phase.
     * @return Vector of complex exponentials.
     */
    KFR_NOINLINE static vec<T, VecWidth> init_cossin(ST w, ST phase)
    {
        return ccomp(cossin(dup(phase + enumerate<ST, VecWidth>() * w)));
    }
};

/**
 * @brief Generator that produces a base-2 exponential sequence
 * \f$ x_i = 2^{ start + i \cdot step } \f$.
 *
 * The state is advanced multiplicatively using a precomputed per-vector growth
 * factor so that each generated vector is contiguous with the previous one.
 *
 * @tparam T Value type.
 * @tparam VecWidth Native vector width.
 */
template <typename T, size_t VecWidth = internal::generator_width<T>(1)>
struct generator_exp2 : public generator<T, VecWidth, generator_exp2<T, VecWidth>>
{
    /**
     * @brief Constructs a base-2 exponential generator.
     * @param start Exponent of the first value to produce.
     * @param step Increment added to the exponent between successive values.
     */
    generator_exp2(T start, T step) noexcept
        : step{ step }, vstep{ exp2(make_vector(step * VecWidth))[0] - 1 }
    {
        this->resync(start);
    }

    /**
     * @brief Reinitializes the generator so the next vector starts at @p start.
     * @param start Exponent of the starting value.
     */
    KFR_MEM_INTRINSIC void sync(T start) const noexcept
    {
        this->value = exp2(start + enumerate(vec_shape<T, VecWidth>{}, step));
    }

    /**
     * @brief Advances the generator state by one native vector width using the
     * precomputed multiplicative growth factor.
     */
    KFR_MEM_INTRINSIC void next() const noexcept { this->value += this->value * vstep; }

protected:
    T step;
    T vstep;
};

/**
 * @brief Generator that produces interleaved cosine and sine values
 * \f[
 *   x_i = \begin{cases}
 *     \cos(start + i \cdot step), & \text{if } i \text{ is even}\\
 *     \sin(start + i \cdot step), & \text{otherwise}
 *   \end{cases}
 * \f]
 *
 * The rotation is advanced using a recurrence relation based on the
 * precomputed coefficients @ref alpha and @ref beta.
 *
 * @tparam T Value type.
 * @tparam VecWidth Native vector width (must be even).
 */
template <typename T, size_t VecWidth = internal::generator_width<T>(1)>
struct generator_cossin : public generator<T, VecWidth, generator_cossin<T, VecWidth>>
{
    static_assert(VecWidth % 2 == 0);
    /**
     * @brief Constructs an interleaved cosine/sine generator.
     * @param start Starting phase.
     * @param step Phase increment between successive values.
     */
    generator_cossin(T start, T step)
        : step(step), alpha(2 * sqr(sin(VecWidth / 2 * step / 2))), beta(-sin(VecWidth / 2 * step))
    {
        this->resync(start);
    }
    /**
     * @brief Reinitializes the generator so the next vector starts at phase
     * @p start.
     * @param start Starting phase.
     */
    KFR_MEM_INTRINSIC void sync(T start) const noexcept { this->value = init_cossin(step, start); }

    /**
     * @brief Advances the rotation by one native vector width using the
     * recurrence relation.
     */
    KFR_MEM_INTRINSIC void next() const noexcept
    {
        this->value = this->value - subadd(alpha * this->value, beta * swap<2>(this->value));
    }

protected:
    T step;
    T alpha;
    T beta;
    /**
     * @brief Initializes a vector of interleaved cosine/sine pairs with angular
     * frequency @p w and starting phase @p phase.
     * @param w Angular frequency increment per sample.
     * @param phase Starting phase.
     * @return Vector of interleaved cosine and sine values.
     */
    KFR_NOINLINE static vec<T, VecWidth> init_cossin(T w, T phase)
    {
        return cossin(dup(phase + enumerate(vec_shape<T, VecWidth / 2>{}, w)));
    }
};

/**
 * @brief Generator that produces sine values
 * \f$ x_i = \sin( start + i \cdot step ) \f$.
 *
 * Internally maintains interleaved cos/sin pairs (stored as `vec<T, 2>`
 * elements) and advances the rotation using a recurrence relation based on the
 * precomputed coefficients @ref alpha and @ref beta. Only the sine (odd)
 * components are returned by `get_value()`.
 *
 * @tparam T Value type.
 * @tparam VecWidth Native vector width.
 */
template <typename T, size_t VecWidth = internal::generator_width<T>(2)>
struct generator_sin : public generator<T, VecWidth, generator_sin<T, VecWidth>, vec<T, 2>>
{
    /**
     * @brief Constructs a sine generator.
     * @param start Starting phase.
     * @param step Phase increment between successive values.
     */
    generator_sin(T start, T step)
        : step(step), alpha(2 * sqr(sin(VecWidth * step / 2))), beta(sin(VecWidth * step))
    {
        this->resync(start);
    }
    /**
     * @brief Reinitializes the generator so the next vector starts at phase
     * @p start.
     * @param start Starting phase.
     */
    KFR_MEM_INTRINSIC void sync(T start) const noexcept
    {
        const vec<T, 2 * VecWidth> cs = cossin(dup(start + enumerate(vec_shape<T, VecWidth>{}, step)));
        this->value                   = vec<vec<T, 2>, VecWidth>::from_flatten(cs);
    }

    /**
     * @brief Advances the rotation by one native vector width using the
     * recurrence relation.
     */
    KFR_MEM_INTRINSIC void next() const noexcept
    {
        vec<T, 2 * VecWidth> cs = flatten(this->value);

        cs = cs - addsub(alpha * cs, beta * swap<2>(cs));

        this->value = vec<vec<T, 2>, VecWidth>::from_flatten(cs);
    }
    /**
     * @brief Returns the sine components of the current interleaved cos/sin
     * state.
     * @return Vector of sine values.
     */
    KFR_MEM_INTRINSIC vec<T, VecWidth> get_value() const { return odd(flatten(this->value)); }

protected:
    T step;
    T alpha;
    T beta;
};

/**
 * @brief Creates an expression that generates values starting from @p start
 * and using @p step as the increment between successive numbers.
 *
 * \f[
    x_i = start + i \cdot step
   \f]
 * @tparam T1 Type of the starting value.
 * @tparam T2 Type of the step value.
 * @param start First value to produce.
 * @param step Increment between successive values.
 * @return Linear generator expression.
 */
template <typename T1, typename T2, typename TF = ftype<std::common_type_t<T1, T2>>>
KFR_FUNCTION generator_linear<TF> gen_linear(T1 start, T2 step)
{
    return generator_linear<TF>(start, step);
}

/**
 * @brief Creates an expression that generates values using the following formula:
 * \f[
    x_i = e^{ start + i \cdot step }
   \f]
 * @tparam T1 Type of the starting exponent.
 * @tparam T2 Type of the step value.
 * @param start Exponent of the first value to produce.
 * @param step Increment added to the exponent between successive values.
 * @return Exponential generator expression.
 */
template <typename T1, typename T2, typename TF = ftype<std::common_type_t<T1, T2>>>
KFR_FUNCTION generator_exp<TF> gen_exp(T1 start, T2 step)
{
    return generator_exp<TF>(start, step);
}

/**
 * @brief Creates an expression that generates values using the following formula:
 * \f[
    x_i = e^{ j ( start + i \cdot step ) }
   \f]
 * @tparam T1 Type of the starting phase.
 * @tparam T2 Type of the step value.
 * @param start Phase of the first value to produce.
 * @param step Phase increment between successive values.
 * @return Complex exponential generator expression.
 */
template <typename T1, typename T2, typename TF = complex<ftype<std::common_type_t<T1, T2>>>>
KFR_FUNCTION generator_expj<TF> gen_expj(T1 start, T2 step)
{
    return generator_expj<TF>(start, step);
}

/**
 * @brief Creates an expression that generates values using the following formula:
 * \f[
    x_i = 2^{ start + i \cdot step }
   \f]
 * @tparam T1 Type of the starting exponent.
 * @tparam T2 Type of the step value.
 * @param start Exponent of the first value to produce.
 * @param step Increment added to the exponent between successive values.
 * @return Base-2 exponential generator expression.
 */
template <typename T1, typename T2, typename TF = ftype<std::common_type_t<T1, T2>>>
KFR_FUNCTION generator_exp2<TF> gen_exp2(T1 start, T2 step)
{
    return generator_exp2<TF>(start, step);
}

/**
 * @brief Creates an expression that generates values using the following formula:
 * \f[
    x_i=
    \begin{cases}
        \cos(start + i \cdot step),  & \text{if } i \text{ is even}\\
        \sin(start + i \cdot step),  & \text{otherwise}
    \end{cases}
   \f]
 * @tparam T1 Type of the starting phase.
 * @tparam T2 Type of the step value.
 * @param start Starting phase.
 * @param step Phase increment between successive values.
 * @return Interleaved cosine/sine generator expression.
 */
template <typename T1, typename T2, typename TF = ftype<std::common_type_t<T1, T2>>>
KFR_FUNCTION generator_cossin<TF> gen_cossin(T1 start, T2 step)
{
    return generator_cossin<TF>(start, step);
}

/**
 * @brief Creates an expression that generates values using the following formula:
 * \f[
    x_i = \sin( start + i \cdot step )
   \f]
 * @tparam T1 Type of the starting phase.
 * @tparam T2 Type of the step value.
 * @param start Starting phase.
 * @param step Phase increment between successive values.
 * @return Sine generator expression.
 */
template <typename T1, typename T2, typename TF = ftype<std::common_type_t<T1, T2>>>
KFR_FUNCTION generator_sin<TF> gen_sin(T1 start, T2 step)
{
    return generator_sin<TF>(start, step);
}
} // namespace KFR_ARCH_NAME
} // namespace kfr
