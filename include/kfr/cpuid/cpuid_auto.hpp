/** @addtogroup cpuid
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

#include "cpuid.hpp"

namespace kfr
{
namespace internal
{

CMT_INLINE cpu_t& cpu_v()
{
    static cpu_t v1 = cpu_t::native;
    return v1;
}

CMT_INLINE char init_cpu_v()
{
    cpu_v() = detect_cpu<0>();
    return 0;
}

CMT_INLINE char init_dummyvar()
{
    static char dummy = init_cpu_v();
    return dummy;
}

static char dummyvar = init_dummyvar();
}

/**
 * @brief Returns cpu instruction set detected at runtime.
 */
CMT_INLINE cpu_t get_cpu() { return internal::cpu_v(); }
}
