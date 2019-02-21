/** @addtogroup dft
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

#ifdef FLOAT
#include "../fft.hpp"

namespace kfr
{
inline namespace CMT_ARCH_NAME
{

template dft_plan<FLOAT>::dft_plan(size_t, dft_order);
template void dft_plan<FLOAT>::init_fft(size_t, dft_order);
template dft_plan<FLOAT>::~dft_plan();
template void dft_plan<FLOAT>::dump() const;
template void dft_plan<FLOAT>::execute_dft(cometa::cbool_t<false>, kfr::complex<FLOAT>* out,
                                           const kfr::complex<FLOAT>* in, kfr::u8* temp) const;
template void dft_plan<FLOAT>::execute_dft(cometa::cbool_t<true>, kfr::complex<FLOAT>* out,
                                           const kfr::complex<FLOAT>* in, kfr::u8* temp) const;
template dft_plan_real<FLOAT>::dft_plan_real(size_t);
template void dft_plan_real<FLOAT>::from_fmt(kfr::complex<FLOAT>* out, const kfr::complex<FLOAT>* in,
                                             kfr::dft_pack_format fmt) const;
template void dft_plan_real<FLOAT>::to_fmt(kfr::complex<FLOAT>* out, kfr::dft_pack_format fmt) const;
} // namespace CMT_ARCH_NAME
} // namespace kfr

#endif
