/** @addtogroup expressions
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

#include "function.hpp"
#include "log_exp.hpp"
#include "select.hpp"
#include "sin_cos.hpp"
#include "vec.hpp"

namespace kfr
{

namespace internal
{

template <typename T, size_t width_, typename Class>
struct generator : input_expression
{
    constexpr static size_t width = width_;
    using type                    = T;

    constexpr static bool is_incremental = true;

    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t, vec_t<U, N> t) const
    {
        return generate(t);
    }

    void resync(T start) const { ptr_cast<Class>(this)->sync(start); }

protected:
    void call_next() const { ptr_cast<Class>(this)->next(); }
    template <size_t N>
    void call_shift(csize_t<N>) const
    {
        ptr_cast<Class>(this)->shift(csize<N>);
    }

    template <size_t N>
    void shift(csize_t<N>) const
    {
        const vec<T, width> oldvalue = value;
        call_next();
        value = slice<N, width>(oldvalue, value);
    }

    template <size_t N, KFR_ENABLE_IF(N == width)>
    CMT_INLINE vec<T, N> generate(vec_t<T, N>) const
    {
        const vec<T, N> result = value;
        call_next();
        return result;
    }

    template <size_t N, KFR_ENABLE_IF(N < width)>
    CMT_INLINE vec<T, N> generate(vec_t<T, N>) const
    {
        const vec<T, N> result = narrow<N>(value);
        shift(csize<N>);
        return result;
    }

    template <size_t N, KFR_ENABLE_IF(N > width)>
    CMT_INLINE vec<T, N> generate(vec_t<T, N> x) const
    {
        const auto lo = generate(low(x));
        const auto hi = generate(high(x));
        return concat(lo, hi);
    }

    mutable vec<T, width> value;
};

template <typename T, size_t width = get_vector_width<T, cpu_t::native>(1, 2)>
struct generator_linear : generator<T, width, generator_linear<T, width>>
{
    constexpr generator_linear(T start, T step) noexcept : step(step), vstep(step* width)
    {
        this->resync(start);
    }

    CMT_INLINE void sync(T start) const noexcept { this->value = start + enumerate<T, width>() * step; }

    CMT_INLINE void next() const noexcept { this->value += vstep; }

protected:
    T step;
    T vstep;
};

template <typename T, size_t width = get_vector_width<T, cpu_t::native>(1, 2), KFR_ARCH_DEP>
struct generator_exp : generator<T, width, generator_exp<T, width>>
{
    generator_exp(T start, T step) noexcept : step(step), vstep(exp(make_vector(step* width))[0] - 1)
    {
        this->resync(start);
    }

    CMT_INLINE void sync(T start) const noexcept { this->value = exp(start + enumerate<T, width>() * step); }

    CMT_INLINE void next() const noexcept { this->value += this->value * vstep; }

protected:
    T step;
    T vstep;
};

template <typename T, size_t width = get_vector_width<T, cpu_t::native>(1, 2), KFR_ARCH_DEP>
struct generator_exp2 : generator<T, width, generator_exp2<T, width>>
{
    generator_exp2(T start, T step) noexcept : step(step), vstep(exp2(make_vector(step* width))[0] - 1)
    {
        this->resync(start);
    }

    CMT_INLINE void sync(T start) const noexcept { this->value = exp2(start + enumerate<T, width>() * step); }

    CMT_INLINE void next() const noexcept { this->value += this->value * vstep; }

protected:
    T step;
    T vstep;
};

template <typename T, size_t width = get_vector_width<T, cpu_t::native>(1, 2), KFR_ARCH_DEP>
struct generator_cossin : generator<T, width, generator_cossin<T, width>>
{
    generator_cossin(T start, T step)
        : step(step), alpha(2 * sqr(sin(width / 2 * step / 2))), beta(-sin(width / 2 * step))
    {
        this->resync(start);
    }
    CMT_INLINE void sync(T start) const noexcept { this->value = init_cossin(step, start); }

    CMT_INLINE void next() const noexcept
    {
        this->value = this->value - subadd(alpha * this->value, beta * swap<2>(this->value));
    }

protected:
    T step;
    T alpha;
    T beta;
    CMT_NOINLINE static vec<T, width> init_cossin(T w, T phase)
    {
        return cossin(dup(phase + enumerate<T, width / 2>() * w));
    }
};

template <typename T, size_t width = get_vector_width<T, cpu_t::native>(2, 4), KFR_ARCH_DEP>
struct generator_sin : generator<T, width, generator_sin<T, width>>
{
    generator_sin(T start, T step)
        : step(step), alpha(2 * sqr(sin(width * step / 2))), beta(sin(width * step))
    {
        this->resync(start);
    }
    CMT_INLINE void sync(T start) const noexcept
    {
        const vec<T, width* 2> cs = splitpairs(cossin(dup(start + enumerate<T, width>() * step)));
        this->cos_value = low(cs);
        this->value     = high(cs);
    }

    CMT_INLINE void next() const noexcept
    {
        const vec<T, width> c = this->cos_value;
        const vec<T, width> s = this->value;

        const vec<T, width> cc = alpha * c + beta * s;
        const vec<T, width> ss = alpha * s - beta * c;

        this->cos_value = c - cc;
        this->value     = s - ss;
    }

    template <size_t N>
    void shift(csize_t<N>) const noexcept
    {
        const vec<T, width> oldvalue    = this->value;
        const vec<T, width> oldcosvalue = this->cos_value;
        next();
        this->value     = slice<N, width>(oldvalue, this->value);
        this->cos_value = slice<N, width>(oldcosvalue, this->cos_value);
    }

protected:
    T step;
    T alpha;
    T beta;
    mutable vec<T, width> cos_value;
};
}

/**
 * @brief Returns template expression that generates values starting from the start and using the step as the
 * increment between numbers.
 *
 * \f[
    x_i = start + i \cdot step
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_SINTRIN internal::generator_linear<TF> gen_linear(T1 start, T2 step)
{
    return internal::generator_linear<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = e^{ start + i \cdot step }
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_SINTRIN internal::generator_exp<TF> gen_exp(T1 start, T2 step)
{
    return internal::generator_exp<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = 2^{ start + i \cdot step }
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_SINTRIN internal::generator_exp2<TF> gen_exp2(T1 start, T2 step)
{
    return internal::generator_exp2<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i=
    \begin{cases}
        \cos(start + i \cdot step),  & \text{if } i \text{ is even}\\
        \sin(start + i \cdot step),  & \text{otherwise}
    \end{cases}
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_SINTRIN internal::generator_cossin<TF> gen_cossin(T1 start, T2 step)
{
    return internal::generator_cossin<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = \sin( start + i \cdot step )
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_SINTRIN internal::generator_sin<TF> gen_sin(T1 start, T2 step)
{
    return internal::generator_sin<TF>(start, step);
}
}
