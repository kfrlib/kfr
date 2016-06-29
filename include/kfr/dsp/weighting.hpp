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

#include "../base/sqrt.hpp"
#include "../base/vec.hpp"
#include "units.hpp"

namespace kfr
{
namespace internal
{

template <cpu_t c = cpu_t::native, cpu_t cc = c>
struct in_weight : in_sqrt<cc>, in_dsp_units<cc>
{
private:
    using in_dsp_units<cc>::amp_to_dB;

public:
    template <typename T>
    KFR_SINTRIN T weight_a_unnorm(T f)
    {
        const T f2  = pow2(f);
        const T nom = pow2(12200) * pow4(f);
        const T den =
            (f2 + pow2(20.6)) * (sqrt((f2 + pow2(107.7)) * (f2 + pow2(737.9)))) * (f2 + pow2(12200));
        return nom / den;
    }

    template <typename T>
    constexpr static T weight_a_gain = reciprocal(weight_a_unnorm(T(1000.0)));

    template <typename T>
    KFR_SINTRIN T aweighting(T f)
    {
        return weight_a_unnorm(f) * weight_a_gain<subtype<T>>;
    }

    template <typename T>
    KFR_SINTRIN T weight_b_unnorm(T f)
    {
        const T f2  = pow2(f);
        const T nom = pow2(12200) * pow3(f);
        const T den = (f2 + pow2(20.6)) * (sqrt((f2 + pow2(158.5)))) * (f2 + pow2(12200));

        return nom / den;
    }

    template <typename T>
    constexpr static T weight_b_gain = reciprocal(weight_b_unnorm(T(1000.0)));

    template <typename T>
    KFR_SINTRIN T bweighting(T f)
    {
        return weight_b_unnorm(f) * weight_b_gain<subtype<T>>;
    }

    template <typename T>
    KFR_SINTRIN T weight_c_unnorm(T f)
    {
        const T f2  = pow2(f);
        const T nom = pow2(12200) * f2;
        const T den = (f2 + pow2(20.6)) * (f2 + pow2(12200));

        return nom / den;
    }

    template <typename T>
    constexpr static T weight_c_gain = reciprocal(weight_c_unnorm(T(1000.0)));

    template <typename T>
    KFR_SINTRIN T cweighting(T f)
    {
        return weight_c_unnorm(f) * weight_c_gain<subtype<T>>;
    }

    template <typename T>
    KFR_SINTRIN T aweightingdB(T f)
    {
        return amp_to_dB(aweighting(f));
    }
    template <typename T>
    KFR_SINTRIN T bweightingdB(T f)
    {
        return amp_to_dB(bweighting(f));
    }
    template <typename T>
    KFR_SINTRIN T cweightingdB(T f)
    {
        return amp_to_dB(cweighting(f));
    }

    KFR_SPEC_FN(in_weight, aweighting)
    KFR_SPEC_FN(in_weight, bweighting)
    KFR_SPEC_FN(in_weight, cweighting)
    KFR_SPEC_FN(in_weight, aweightingdB)
    KFR_SPEC_FN(in_weight, bweightingdB)
    KFR_SPEC_FN(in_weight, cweightingdB)
};
}
}
