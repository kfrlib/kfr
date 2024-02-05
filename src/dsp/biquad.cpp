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
#include <kfr/multiarch.h>
#include <kfr/dsp/biquad.hpp>

namespace kfr
{

CMT_MULTI_PROTO(namespace impl {
    template <typename T>
    expression_handle<T, 1> create_iir_filter(const iir_params<T>& params);
} // namespace impl
)

inline namespace CMT_ARCH_NAME
{
namespace impl
{
template <typename T>
expression_handle<T, 1> create_iir_filter(const iir_params<T>& params)
{
    return iir(placeholder<T>(), params);
}
template expression_handle<float, 1> create_iir_filter<float>(const iir_params<float>& params);
template expression_handle<double, 1> create_iir_filter<double>(const iir_params<double>& params);
} // namespace impl
} // namespace CMT_ARCH_NAME

#ifdef CMT_MULTI_NEEDS_GATE

template <typename T>
iir_filter<T>::iir_filter(const iir_params<T>& params)
{
    CMT_MULTI_GATE(this->filter_expr = ns::impl::create_iir_filter<T>(params));
}

template iir_filter<float>::iir_filter(const iir_params<float>&);
template iir_filter<double>::iir_filter(const iir_params<double>&);

#endif

} // namespace kfr
