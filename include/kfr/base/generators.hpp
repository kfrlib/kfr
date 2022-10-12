/** @addtogroup generators
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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
#include "../simd/impl/function.hpp"
#include "../simd/select.hpp"
#include "../simd/vec.hpp"
#include "shape.hpp"

namespace kfr
{

template <typename T, size_t VecWidth, typename Class, typename Twork = T>
struct xgenerator
{
    constexpr static size_t width = VecWidth;

    void resync(T start) const { ptr_cast<Class>(this)->sync(start); }

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
        else if (N > width)
        {
            const vec lo = generate(low(x));
            const vec hi = generate(high(x));
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

private:
    KFR_MEM_INTRINSIC void call_next() const { ptr_cast<Class>(this)->next(); }

    KFR_MEM_INTRINSIC vec<T, width> call_get_value() const { return ptr_cast<Class>(this)->get_value(); }

    KFR_MEM_INTRINSIC std::enable_if_t<std::is_same_v<T, Twork>, vec<T, width>> get_value() const
    {
        return value;
    }
};

template <typename T, size_t VecWidth, typename Class>
struct expression_traits<xgenerator<T, VecWidth, Class>> : public expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = 1;
    constexpr static shape<1> shapeof(const T&) { return shape<1>(infinite_size); }
    constexpr static shape<1> shapeof() { return shape<1>(infinite_size); }

    constexpr static inline bool explicit_operand = true;
    constexpr static inline bool random_access    = false;
};

inline namespace CMT_ARCH_NAME
{
template <typename T, size_t VecWidth, typename Class, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xgenerator<T, VecWidth, Class>& self, const shape<1>& index,
                                     const axis_params<0, N>&)
{
    return self.template generate<N>();
}
} // namespace CMT_ARCH_NAME

template <typename T, size_t VecWidth = vector_capacity<T> / 8>
struct xgenlinear : public xgenerator<T, VecWidth, xgenlinear<T, VecWidth>>
{
    xgenlinear(T start, T step) CMT_NOEXCEPT : step{ step }, vstep{ step * VecWidth } { sync(start); }

    KFR_MEM_INTRINSIC void sync(T start) const CMT_NOEXCEPT
    {
        this->value = start + enumerate(vec_shape<T, VecWidth>{}, vstep / VecWidth);
    }

    KFR_MEM_INTRINSIC void next() const CMT_NOEXCEPT { this->value += vstep; }

    T vstep;
};

template <typename T, size_t VecWidth = vector_capacity<T> / 8>
struct xgenexp : public xgenerator<T, VecWidth, xgenexp<T, VecWidth>>
{
    xgenexp(T start, T step) CMT_NOEXCEPT : step{ step },
                                            vstep{ exp(make_vector(step * VecWidth)).front() - 1 }
    {
        this->resync(start);
    }

    KFR_MEM_INTRINSIC void sync(T start) const CMT_NOEXCEPT
    {
        this->value = exp(start + enumerate<T, VecWidth>() * step);
    }

    KFR_MEM_INTRINSIC void next() const CMT_NOEXCEPT { this->value += this->value * vstep; }

protected:
    T step;
    T vstep;
};

template <typename T, size_t VecWidth = vector_capacity<deep_subtype<T>> / 8 / 2>
struct xgenexpj : public xgenerator<T, VecWidth, xgenexpj<T, VecWidth>>
{
    using ST = deep_subtype<T>;
    static_assert(std::is_same_v<complex<deep_subtype<T>>, T>, "xgenexpj requires complex type");

    xgenexpj(ST start_, ST step_)
        : step(step_), alpha(2 * sqr(sin(VecWidth * step / 2))), beta(-sin(VecWidth * step))
    {
        this->resync(T(start_));
    }
    KFR_MEM_INTRINSIC void sync(T start) const CMT_NOEXCEPT { this->value = init_cossin(step, start.real()); }

    KFR_MEM_INTRINSIC void next() const CMT_NOEXCEPT
    {
        this->value = ccomp(cdecom(this->value) -
                            subadd(alpha * cdecom(this->value), beta * swap<2>(cdecom(this->value))));
    }

protected:
    ST step;
    ST alpha;
    ST beta;
    CMT_NOINLINE static vec<T, VecWidth> init_cossin(ST w, ST phase)
    {
        return ccomp(cossin(dup(phase + enumerate<ST, width>() * w)));
    }
};

template <typename T, size_t VecWidth = vector_capacity<T> / 8>
struct xgenexp2 : public xgenerator<T, VecWidth, xgenexp2<T, VecWidth>>
{
    xgenexp2(T start, T step) CMT_NOEXCEPT : step{ step }, vstep{ exp2(make_vector(step * VecWidth))[0] - 1 }
    {
        this->resync(start);
    }

    KFR_MEM_INTRINSIC void sync(T start) const CMT_NOEXCEPT
    {
        this->value = exp2(start + enumerate(vec_shape<T, VecWidth>, step));
    }

    KFR_MEM_INTRINSIC void next() const CMT_NOEXCEPT { this->value += this->value * vstep; }

protected:
    T step;
    T vstep;
};

template <typename T, size_t VecWidth = vector_capacity<T> / 8>
struct xgencossin : public xgenerator<T, VecWidth, xgencossin<T, VecWidth>>
{
    static_assert(VecWidth % 2 == 0);
    xgencossin(T start, T step)
        : step(step), alpha(2 * sqr(sin(VecWidth / 2 * step / 2))), beta(-sin(VecWidth / 2 * step))
    {
        this->resync(start);
    }
    KFR_MEM_INTRINSIC void sync(T start) const CMT_NOEXCEPT { this->value = init_cossin(step, start); }

    KFR_MEM_INTRINSIC void next() const CMT_NOEXCEPT
    {
        this->value = this->value - subadd(alpha * this->value, beta * swap<2>(this->value));
    }

protected:
    T step;
    T alpha;
    T beta;
    CMT_NOINLINE static vec<T, VecWidth> init_cossin(T w, T phase)
    {
        return cossin(dup(phase + enumerate(vec_shape<T, VecWidth / 2>, w)));
    }
};

template <typename T, size_t VecWidth = vector_capacity<T> / 8 / 2>
struct xgensin : public xgenerator<T, VecWidth, xgensin<T, VecWidth>, vec<T, 2>>
{
    xgensin(T start, T step)
        : step(step), alpha(2 * sqr(sin(VecWidth * step / 2))), beta(sin(VecWidth * step))
    {
        this->resync(start);
    }
    KFR_MEM_INTRINSIC void sync(T start) const CMT_NOEXCEPT
    {
        const vec<T, 2 * VecWidth> cs = cossin(dup(start + enumerate(vec_shape<T, VecWidth>, step)));
        this->value                   = vec<vec<T, 2>, VecWidth>::from_flatten(cs);
    }

    KFR_MEM_INTRINSIC void next() const CMT_NOEXCEPT
    {
        const vec<T, 2 * VecWidth> cs = flatten(this->value);

        cs = cs - addsub(alpha * cs, beta * swap<2>(cs));

        this->value = vec<vec<T, 2>, VecWidth>::from_flatten(cs);

        // const vec<T, VecWidth> c = even(flatten(this->value));
        // const vec<T, VecWidth> s = odd(flatten(this->value));
        // const vec<T, VecWidth> cc = alpha * c + beta * s;
        // const vec<T, VecWidth> ss = alpha * s - beta * c;
        // this->cos_value = c - cc;
        // this->value     = s - ss;
    }

protected:
    T step;
    T alpha;
    T beta;
};

inline namespace CMT_ARCH_NAME
{

/**
 * @brief Returns template expression that generates values starting from the start and using the step as the
 * increment between numbers.
 *
 * \f[
    x_i = start + i \cdot step
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_FUNCTION xgenlinear<TF> gen_linear(T1 start, T2 step)
{
    return xgenlinear<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = e^{ start + i \cdot step }
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_FUNCTION xgenexp<TF> gen_exp(T1 start, T2 step)
{
    return xgenexp<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = e^{ j ( start + i \cdot step ) }
   \f]
 */
template <typename T1, typename T2, typename TF = complex<ftype<common_type<T1, T2>>>>
KFR_FUNCTION xgenexpj<TF> gen_expj(T1 start, T2 step)
{
    return xgenexpj<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = 2^{ start + i \cdot step }
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_FUNCTION xgenexp2<TF> gen_exp2(T1 start, T2 step)
{
    return xgenexp2<TF>(start, step);
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
KFR_FUNCTION xgencossin<TF> gen_cossin(T1 start, T2 step)
{
    return xgencossin<TF>(start, step);
}

/**
 * @brief Returns template expression that generates values using the following formula:
 * \f[
    x_i = \sin( start + i \cdot step )
   \f]
 */
template <typename T1, typename T2, typename TF = ftype<common_type<T1, T2>>>
KFR_FUNCTION xgensin<TF> gen_sin(T1 start, T2 step)
{
    return xgensin<TF>(start, step);
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
