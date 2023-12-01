/** @addtogroup shuffle
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

#include "../simd/operators.hpp"
#include "../simd/read_write.hpp"
#include "expression.hpp"

namespace kfr
{

template <typename T>
void convert_endianess(T* data, size_t size)
{
    block_process(size, csizes<2 * vector_width<T>, 1>,
                  [&](size_t i, auto w)
                  {
                      constexpr size_t width = CMT_CVAL(w);
                      vec<T, width> value    = read<width>(data + i);
                      value                  = swapbyteorder(value);
                      write(data + i, value);
                  });
}
} // namespace kfr
