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
#pragma once

#include "cpuid.hpp"

namespace kfr
{

namespace internal_generic
{

KFR_INTRINSIC cpu_t& cpu_v()
{
    static cpu_t v1 = cpu_t::native;
    return v1;
}

KFR_INTRINSIC char init_cpu_v()
{
    cpu_v() = detect_cpu<0>();
    return 0;
}

KFR_INTRINSIC char init_dummyvar()
{
    static char dummy = init_cpu_v();
    return dummy;
}

static char dummyvar = init_dummyvar();
} // namespace internal_generic

/**
 * @brief Returns the CPU instruction set selected for use at runtime.
 *
 * The value is initialized once at static-init time from
 * @ref internal_generic::detect_cpu and may afterwards be changed with
 * @ref override_cpu.
 */
inline cpu_t get_cpu() { return internal_generic::cpu_v(); }

/**
 * @brief Overrides the CPU instruction set used by KFR at runtime.
 *
 * @param cpu The instruction set level to install.
 * @return The previously selected instruction set level.
 *
 * Useful for forcing a lower (or higher, if the host supports it) target than
 * the auto-detected one. Dispatchers that query @ref get_cpu after this call
 * will observe the new value.
 */
inline cpu_t override_cpu(cpu_t cpu)
{
    cpu_t previous            = internal_generic::cpu_v();
    internal_generic::cpu_v() = cpu;
    return previous;
}

} // namespace kfr
