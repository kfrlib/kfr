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

#include "../base/function.hpp"
#include "../base/operators.hpp"
#include "../base/vec.hpp"
#include <cmath>

#pragma clang diagnostic push
#if CID_HAS_WARNING("-Winaccessible-base")
#pragma clang diagnostic ignored "-Winaccessible-base"
#endif

namespace kfr
{

enum class biquad_type
{
    lowpass,
    highpass,
    bandpass,
    bandstop,
    peak,
    notch,
    lowshelf,
    highshelf
};

template <typename T>
struct biquad_params
{
    constexpr static bool is_pod = true;

    static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
    constexpr biquad_params() noexcept : a0(1), a1(0), a2(0), b0(1), b1(0), b2(0) {}
    constexpr biquad_params(T a0, T a1, T a2, T b0, T b1, T b2) noexcept : a0(a0),
                                                                           a1(a1),
                                                                           a2(a2),
                                                                           b0(b0),
                                                                           b1(b1),
                                                                           b2(b2)
    {
    }
    T a0;
    T a1;
    T a2;
    T b0;
    T b1;
    T b2;
    biquad_params<T> normalized_a0() const
    {
        vec<T, 5> v{ a1, a2, b0, b1, b2 };
        v = v / a0;
        return { T(1.0), v[0], v[1], v[2], v[3], v[4] };
    }
    biquad_params<T> normalized_b0() const { return { a0, a1, a2, T(1.0), b1 / b0, b2 / b0 }; }
    biquad_params<T> normalized_all() const { return normalized_a0().normalized_b0(); }
};

template <typename T>
KFR_INLINE biquad_params<T> biquad_allpass(T frequency, T Q)
{
    const T alpha = std::sin(frequency) / 2.0 * Q;
    const T cs    = std::cos(frequency);

    const T b0 = 1.0 / (1.0 + alpha);
    const T b1 = -2.0 * cs * b0;
    const T b2 = (1.0 - alpha) * b0;
    const T a0 = (1.0 - alpha) * b0;
    const T a1 = -2.0 * cs * b0;
    const T a2 = (1.0 + alpha) * b0;
    return { b0, b1, b2, a0, a1, a2 };
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_lowpass(T frequency, T Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = K2 * norm;
    const T a1   = 2 * a0;
    const T a2   = a0;
    const T b1   = 2 * (K2 - 1) * norm;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_highpass(T frequency, T Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = 1 * norm;
    const T a1   = -2 * a0;
    const T a2   = a0;
    const T b1   = 2 * (K2 - 1) * norm;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_bandpass(T frequency, T Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = K / Q * norm;
    const T a1   = 0;
    const T a2   = -a0;
    const T b1   = 2 * (K2 - 1) * norm;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_notch(T frequency, T Q)
{
    const T K    = std::tan(c_pi<T, 1> * frequency);
    const T K2   = K * K;
    const T norm = 1 / (1 + K / Q + K2);
    const T a0   = (1 + K2) * norm;
    const T a1   = 2 * (K2 - 1) * norm;
    const T a2   = a0;
    const T b1   = a1;
    const T b2   = (1 - K / Q + K2) * norm;
    return { 1.0, b1, b2, a0, a1, a2 };
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_peak(T frequency, T Q, T gain)
{
    biquad_params<T> result;
    const T K  = std::tan(c_pi<T, 1> * frequency);
    const T K2 = K * K;
    const T V  = std::exp(std::abs(gain) * (1.0 / 20.0) * c_log_10<T>);

    if (gain >= 0)
    { // boost
        const T norm = 1 / (1 + 1 / Q * K + K2);
        const T a0   = (1 + V / Q * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - V / Q * K + K2) * norm;
        const T b1   = a1;
        const T b2   = (1 - 1 / Q * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    else
    { // cut
        const T norm = 1 / (1 + V / Q * K + K2);
        const T a0   = (1 + 1 / Q * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - 1 / Q * K + K2) * norm;
        const T b1   = a1;
        const T b2   = (1 - V / Q * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    return result;
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_lowshelf(T frequency, T gain)
{
    biquad_params<T> result;
    const T K  = std::tan(c_pi<T, 1> * frequency);
    const T K2 = K * K;
    const T V  = std::exp(std::fabs(gain) * (1.0 / 20.0) * c_log_10<T>);

    if (gain >= 0)
    { // boost
        const T norm = 1 / (1 + c_sqrt_2<T> * K + K2);
        const T a0   = (1 + std::sqrt(2 * V) * K + V * K2) * norm;
        const T a1   = 2 * (V * K2 - 1) * norm;
        const T a2   = (1 - std::sqrt(2 * V) * K + V * K2) * norm;
        const T b1   = 2 * (K2 - 1) * norm;
        const T b2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    else
    { // cut
        const T norm = 1 / (1 + std::sqrt(2 * V) * K + V * K2);
        const T a0   = (1 + c_sqrt_2<T> * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        const T b1   = 2 * (V * K2 - 1) * norm;
        const T b2   = (1 - std::sqrt(2 * V) * K + V * K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    return result;
}

template <typename T>
KFR_INLINE biquad_params<T> biquad_highshelf(T frequency, T gain)
{
    biquad_params<T> result;
    const T K  = std::tan(c_pi<T, 1> * frequency);
    const T K2 = K * K;
    const T V  = std::exp(std::fabs(gain) * (1.0 / 20.0) * c_log_10<T>);

    if (gain >= 0)
    { // boost
        const T norm = 1 / (1 + c_sqrt_2<T> * K + K2);
        const T a0   = (V + std::sqrt(2 * V) * K + K2) * norm;
        const T a1   = 2 * (K2 - V) * norm;
        const T a2   = (V - std::sqrt(2 * V) * K + K2) * norm;
        const T b1   = 2 * (K2 - 1) * norm;
        const T b2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    else
    { // cut
        const T norm = 1 / (V + std::sqrt(2 * V) * K + K2);
        const T a0   = (1 + c_sqrt_2<T> * K + K2) * norm;
        const T a1   = 2 * (K2 - 1) * norm;
        const T a2   = (1 - c_sqrt_2<T> * K + K2) * norm;
        const T b1   = 2 * (K2 - V) * norm;
        const T b2   = (V - std::sqrt(2 * V) * K + K2) * norm;
        result       = { 1.0, b1, b2, a0, a1, a2 };
    }
    return result;
}

namespace internal
{
template <cpu_t cpu = cpu_t::native>
struct in_biquad
{
private:
public:
    template <typename T, size_t filters>
    struct biquad_block
    {
        vec<T, filters> s1;
        vec<T, filters> s2;
        vec<T, filters> a1;
        vec<T, filters> a2;
        vec<T, filters> b0;
        vec<T, filters> b1;
        vec<T, filters> b2;

        vec<T, filters> out;
        biquad_block() : s1(0), s2(0), a1(0), a2(0), b0(1), b1(0), b2(0), out(0) {}
        biquad_block(const biquad_params<T>* bq, size_t count) : s1(0), s2(0), out(0)
        {
            count = count > filters ? filters : count;
            for (size_t i = 0; i < count; i++)
            {
                a1(i) = bq[i].a1;
                a2(i) = bq[i].a2;
                b0(i) = bq[i].b0;
                b1(i) = bq[i].b1;
                b2(i) = bq[i].b2;
            }
            for (size_t i = count; i < filters; i++)
            {
                a1(i) = T(0);
                a2(i) = T(0);
                b0(i) = T(1);
                b1(i) = T(0);
                b2(i) = T(0);
            }
        }

        template <size_t count>
        biquad_block(const biquad_params<T> (&bq)[count]) : biquad_block(bq, count)
        {
            static_assert(count <= filters, "count > filters");
        }
    };

    template <size_t filters, typename T, typename E1>
    struct expression_biquads : public expression<E1>
    {
        using value_type = T;

        template <cpu_t newcpu>
        using retarget_this =
            typename in_biquad<newcpu>::template expression_biquads<filters, T, retarget<E1, newcpu>>;

        expression_biquads(const biquad_block<T, filters>& bq, E1&& e1)
            : expression<E1>(std::forward<E1>(e1)), bq(bq)
        {
        }
        template <size_t width>
        KFR_INTRIN vec<T, width> operator()(cinput_t, size_t index, vec_t<T, width> t) const
        {
            const vec<T, width> in = this->argument_first(index, t);
            vec<T, width> out;

            KFR_LOOP_UNROLL
            for (size_t i = 0; i < width; i++)
            {
                bq.out = process(bq, insertleft(in[i], bq.out));
                out(i) = bq.out[filters - 1];
            }

            return out;
        }
        KFR_SINTRIN vec<T, filters> process(biquad_block<T, filters>& bq, vec<T, filters> in)
        {
            const vec<T, filters> out = bq.b0 * in + bq.s1;
            bq.s1 = bq.s2 + bq.b1 * in - bq.a1 * out;
            bq.s2 = bq.b2 * in - bq.a2 * out;
            return out;
        }
        mutable biquad_block<T, filters> bq;
    };
};
}

template <typename T, typename E1>
KFR_INLINE internal::in_biquad<>::expression_biquads<1, T, internal::arg<E1>> biquad(const biquad_params<T>& bq,
                                                                                 E1&& e1)
{
    const biquad_params<T> bqs[1] = { bq };
    return internal::in_biquad<>::expression_biquads<1, T, internal::arg<E1>>(bqs, std::forward<E1>(e1));
}
template <size_t filters, typename T, typename E1>
KFR_INLINE internal::in_biquad<>::expression_biquads<filters, T, internal::arg<E1>> biquad(
    const biquad_params<T> (&bq)[filters], E1&& e1)
{
    return internal::in_biquad<>::expression_biquads<filters, T, internal::arg<E1>>(bq, std::forward<E1>(e1));
}
}

#pragma clang diagnostic pop
