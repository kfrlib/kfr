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
#include <kfr/dsp/fir.hpp>
#include <kfr/multiarch.h>

namespace kfr
{

CMT_MULTI_PROTO(namespace impl {
    template <typename T, typename U>
    class fir_filter : public kfr::fir_filter<T, U>
    {
    public:
        using kfr::fir_filter<T, U>::fir_filter;

        void process_buffer_impl(U* dest, const U* src, size_t size);
        void process_expression_impl(U* dest, const expression_handle<U, 1>& src, size_t size);
    };
} // namespace impl
)

inline namespace CMT_ARCH_NAME
{
namespace impl
{

template <typename T, typename U>
void fir_filter<T, U>::process_buffer_impl(U* dest, const U* src, size_t size)
{
    make_univector(dest, size) = fir(this->state, make_univector(src, size));
}
template <typename T, typename U>
void fir_filter<T, U>::process_expression_impl(U* dest, const expression_handle<U, 1>& src, size_t size)
{
    make_univector(dest, size) = fir(this->state, src);
}

template class fir_filter<float, float>;
template class fir_filter<double, double>;
template class fir_filter<float, double>;
template class fir_filter<double, float>;
template class fir_filter<float, complex<float>>;
template class fir_filter<double, complex<double>>;

} // namespace impl
} // namespace CMT_ARCH_NAME

#ifdef CMT_MULTI_NEEDS_GATE

template <typename T, typename U>
void fir_filter<T, U>::process_buffer(U* dest, const U* src, size_t size)
{
    make_univector(dest, size) = fir(this->state, make_univector(src, size));
}
template <typename T, typename U>
void fir_filter<T, U>::process_expression(U* dest, const expression_handle<U, 1>& src, size_t size)
{
    make_univector(dest, size) = fir(this->state, src);
}
template class fir_filter<float, float>;
template class fir_filter<double, double>;
template class fir_filter<float, double>;
template class fir_filter<double, float>;
template class fir_filter<float, complex<float>>;
template class fir_filter<double, complex<double>>;

#endif

} // namespace kfr
