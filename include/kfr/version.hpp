/** @addtogroup utility
 *  @{
 */
/*
  Copyright (C) 2016-2025 Dan Casarin (https://www.kfrlib.com)
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

#include "runtime/cpuid_auto.hpp"

namespace kfr
{
/// @brief Returns the string representation of the KFR library version, including target architecture.
/// @return A constant character pointer to the version string.
inline static const char* library_version() { return KFR_VERSION_FULL; }

/// @brief Returns the current CPU name at runtime.
/// @return A constant character pointer to the name of the current CPU.
inline static const char* cpu_runtime() { return cpu_name(get_cpu()); }

} // namespace kfr
