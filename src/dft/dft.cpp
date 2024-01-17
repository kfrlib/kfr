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

#include <kfr/dft/fft.hpp>
#include <kfr/multiarch.h>

namespace kfr
{

CMT_MULTI_PROTO(namespace impl {
    template <typename T>
    void dft_initialize(dft_plan<T> & plan);
    template <typename T>
    void dft_real_initialize(dft_plan_real<T> & plan);
})

#ifdef CMT_MULTI_NEEDS_GATE

template <typename T>
void dft_initialize(dft_plan<T>& plan)
{
    CMT_MULTI_GATE(ns::impl::dft_initialize(plan));
}
template <typename T>
void dft_real_initialize(dft_plan_real<T>& plan)
{
    CMT_MULTI_GATE(ns::impl::dft_real_initialize(plan));
}

template void dft_initialize<float>(dft_plan<float>&);
template void dft_initialize<double>(dft_plan<double>&);
template void dft_real_initialize<float>(dft_plan_real<float>&);
template void dft_real_initialize<double>(dft_plan_real<double>&);

#endif

} // namespace kfr
