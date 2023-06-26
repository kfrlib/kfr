/** @addtogroup dsp_extra
 *  @{
 */
/*
  Copyright (C) 2016-2023 Dan Cazarin (https://www.kfrlib.com)
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
#include "../math/log_exp.hpp"
#include "../simd/abs.hpp"
#include "../simd/vec.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

using sample_rate_t = double;

namespace intrinsics
{
template <typename T1, typename T2>
KFR_INTRINSIC std::common_type_t<T1, T2> fix_nans(const T1& val, const T2& replacement)
{
    return select(val != val, replacement, val);
}

template <typename T, typename TF = flt_type<T>>
KFR_INTRINSIC TF amp_to_dB(const T& amp)
{
    return fix_nans(log(static_cast<TF>(abs(amp))) * subtype<TF>(8.6858896380650365530225783783322),
                    -c_infinity<T>);
    // return T( 20.0 ) * log10( level );
}

template <typename T, typename TF = flt_type<T>>
KFR_INTRINSIC TF dB_to_amp(const T& dB)
{
    return exp(dB * subtype<TF>(0.11512925464970228420089957273422));
    // return exp10( dB / 20 );
}

template <typename T, typename TF = flt_type<T>>
KFR_INTRINSIC TF amp_to_dB(const T& amp, const T& offset)
{
    return fix_nans(
        log_fmadd(static_cast<TF>(abs(amp)), subtype<TF>(8.6858896380650365530225783783322), offset),
        -c_infinity<T>);
    // return T( 20.0 ) * log10( level );
}

template <typename T, typename TF = flt_type<T>>
KFR_INTRINSIC TF dB_to_amp(const T& dB, const T& offset)
{
    auto offs = -subtype<TF>(0.11512925464970228420089957273422) * offset;
    return exp_fmadd(dB, subtype<TF>(0.11512925464970228420089957273422), offs);
    // return exp10( dB / 20 );
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout power_to_dB(const T& x)
{
    return log(static_cast<Tout>(abs(x))) * (10 * c_recip_log_10<Tout>);
}

template <typename T, typename Tout = flt_type<T>>
KFR_INTRINSIC Tout dB_to_power(const T& x)
{
    if (CMT_UNLIKELY(x == -c_infinity<Tout>))
        return 0.0;
    else
        return exp(x * (c_log_10<Tout> / 10.0));
}

template <typename T, typename TF = flt_type<T>>
KFR_INTRINSIC TF note_to_hertz(const T& note)
{
    const subtype<TF> offset = 2.1011784386926213177653145771814;

    return intrinsics::exp_fmadd(note, subtype<TF>(0.05776226504666210911810267678818), offset);
}

template <typename T, typename TF = flt_type<T>>
KFR_INTRINSIC TF hertz_to_note(const T& hertz)
{
    const subtype<TF> offset = -36.376316562295915248836189714583;

    return intrinsics::log_fmadd(hertz, subtype<TF>(17.312340490667560888319096172023), offset);
}

template <typename T1, typename T2, typename T3, typename Tc = flt_type<std::common_type_t<T1, T2, T3, f32>>>
KFR_INTRINSIC Tc note_to_hertz(const T1& note, const T2& tunenote, const T3& tunehertz)
{
    const Tc offset = log(tunehertz) - tunenote * subtype<Tc>(0.05776226504666210911810267678818);

    return intrinsics::exp_fmadd(note, subtype<Tc>(0.05776226504666210911810267678818), offset);
}

template <typename T1, typename T2, typename T3, typename Tc = flt_type<std::common_type_t<T1, T2, T3, f32>>>
KFR_INTRINSIC Tc hertz_to_note(const T1& hertz, const T2& tunenote, const T3& tunehertz)
{
    const Tc offset = tunenote - log(tunehertz) * subtype<Tc>(17.312340490667560888319096172023);

    return intrinsics::log_fmadd(hertz, subtype<Tc>(17.312340490667560888319096172023), offset);
}
} // namespace intrinsics
KFR_I_FN(note_to_hertz)
KFR_I_FN(hertz_to_note)
KFR_I_FN(amp_to_dB)
KFR_I_FN(dB_to_amp)
KFR_I_FN(power_to_dB)
KFR_I_FN(dB_to_power)

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> note_to_hertz(const T1& x)
{
    return intrinsics::note_to_hertz(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::note_to_hertz, E1> note_to_hertz(E1&& x)
{
    return { fn::note_to_hertz(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> hertz_to_note(const T1& x)
{
    return intrinsics::hertz_to_note(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::hertz_to_note, E1> hertz_to_note(E1&& x)
{
    return { fn::hertz_to_note(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> amp_to_dB(const T1& x)
{
    return intrinsics::amp_to_dB(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::amp_to_dB, E1> amp_to_dB(E1&& x)
{
    return { fn::amp_to_dB(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> dB_to_amp(const T1& x)
{
    return intrinsics::dB_to_amp(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::dB_to_amp, E1> dB_to_amp(E1&& x)
{
    return { fn::dB_to_amp(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> power_to_dB(const T1& x)
{
    return intrinsics::power_to_dB(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::power_to_dB, E1> power_to_dB(E1&& x)
{
    return { fn::power_to_dB(), std::forward<E1>(x) };
}

template <typename T1, KFR_ENABLE_IF(is_numeric<T1>)>
KFR_FUNCTION flt_type<T1> dB_to_power(const T1& x)
{
    return intrinsics::dB_to_power(x);
}

template <typename E1, KFR_ACCEPT_EXPRESSIONS(E1)>
KFR_FUNCTION expression_function<fn::dB_to_power, E1> dB_to_power(E1&& x)
{
    return { fn::dB_to_power(), std::forward<E1>(x) };
}
} // namespace CMT_ARCH_NAME
} // namespace kfr
