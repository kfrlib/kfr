/** @addtogroup dft
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

#ifdef FLOAT
#include <kfr/dft/fft.hpp>

namespace kfr
{
inline namespace KFR_ARCH_NAME
{
namespace impl
{
template size_t ngfft_twiddle_count<FLOAT>(ngfft_plan<FLOAT>&,
                                           cval_t<dft_algorithm, dft_algorithm::fourstep>);

template bool ngfft_initialize<FLOAT>(ngfft_plan<FLOAT>&, cval_t<dft_algorithm, dft_algorithm::fourstep>);

template void ngfft_execute<FLOAT>(const ngfft_plan<FLOAT>&, cval_t<dft_algorithm, dft_algorithm::fourstep>,
                                   cbool_t<false>, complex<FLOAT>*, const complex<FLOAT>*);

template void ngfft_execute<FLOAT>(const ngfft_plan<FLOAT>&, cval_t<dft_algorithm, dft_algorithm::fourstep>,
                                   cbool_t<true>, complex<FLOAT>*, const complex<FLOAT>*);

template void ngfft_real_execute<FLOAT>(const ngfft_plan<FLOAT>&,
                                        cval_t<dft_algorithm, dft_algorithm::fourstep>, complex<FLOAT>*,
                                        const FLOAT*);

template void ngfft_real_execute<FLOAT>(const ngfft_plan<FLOAT>&,
                                        cval_t<dft_algorithm, dft_algorithm::fourstep>, FLOAT*,
                                        const complex<FLOAT>*);
} // namespace impl
} // namespace KFR_ARCH_NAME
} // namespace kfr

#endif
