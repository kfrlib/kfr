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
#pragma once

#include "../base/complex.hpp"
#include "../base/constants.hpp"
#include "../base/memory.hpp"
#include "../base/read_write.hpp"
#include "../base/vec.hpp"

#include "cache.hpp"
#include "fft.hpp"

#pragma clang diagnostic push
#if CMT_HAS_WARNING("-Wshadow")
#pragma clang diagnostic ignored "-Wshadow"
#endif

namespace kfr
{

template <typename T, size_t Tag1, size_t Tag2>
KFR_INTRIN univector<T> convolve(const univector<T, Tag1>& src1, const univector<T, Tag2>& src2)
{
    const size_t size                = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<T>> src1padded = src1;
    univector<complex<T>> src2padded = src2;
    src1padded.resize(size, 0);
    src2padded.resize(size, 0);

    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype<T>, size);
    univector<u8> temp(dft->temp_size);
    dft->execute(src1padded, src1padded, temp);
    dft->execute(src2padded, src2padded, temp);
    src1padded = src1padded * src2padded;
    dft->execute(src1padded, src1padded, temp, true);
    const T invsize = reciprocal<T>(size);
    return truncate(real(src1padded), src1.size() + src2.size() - 1) * invsize;
}

template <typename T, size_t Tag1, size_t Tag2>
KFR_INTRIN univector<T> correlate(const univector<T, Tag1>& src1, const univector<T, Tag2>& src2)
{
    const size_t size                = next_poweroftwo(src1.size() + src2.size() - 1);
    univector<complex<T>> src1padded = src1;
    univector<complex<T>> src2padded = reverse(src2);
    src1padded.resize(size, 0);
    src2padded.resize(size, 0);
    dft_plan_ptr<T> dft = dft_cache::instance().get(ctype<T>, size);
    univector<u8> temp(dft->temp_size);
    dft->execute(src1padded, src1padded, temp);
    dft->execute(src2padded, src2padded, temp);
    src1padded = src1padded * src2padded;
    dft->execute(src1padded, src1padded, temp, true);
    const T invsize = reciprocal<T>(size);
    return truncate(real(src1padded), src1.size() + src2.size() - 1) * invsize;
}

template <typename T, size_t Tag1>
KFR_INTRIN univector<T> autocorrelate(const univector<T, Tag1>& src)
{
    univector<T> result = correlate(src, src);
    result              = result.slice(result.size() / 2);
    return result;
}
}
#pragma clang diagnostic pop
