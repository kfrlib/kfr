/** @addtogroup dft
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

#ifdef FLOAT
#include <kfr/dft/fft.hpp>

namespace kfr
{
inline namespace CMT_ARCH_NAME
{
namespace impl
{
template void dft_initialize<FLOAT>(dft_plan<FLOAT>& plan);
template void dft_real_initialize<FLOAT>(dft_plan_real<FLOAT>& plan);
template void dft_execute<FLOAT>(const dft_plan<FLOAT>&, cbool_t<false>, complex<FLOAT>*,
                                 const complex<FLOAT>*, u8*);
template void dft_execute<FLOAT>(const dft_plan<FLOAT>&, cbool_t<true>, complex<FLOAT>*,
                                 const complex<FLOAT>*, u8*);
template void dft_initialize_transpose<FLOAT>(internal_generic::fn_transpose<FLOAT>& transpose);
} // namespace impl
} // namespace CMT_ARCH_NAME
} // namespace kfr

#endif
