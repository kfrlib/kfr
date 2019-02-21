/** @addtogroup io
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

#include "../audiofile.hpp"
CMT_PRAGMA_GNU(GCC diagnostic push)
CMT_PRAGMA_GNU(GCC diagnostic ignored "-Wimplicit-fallthrough")

#if defined(KFR_ENABLE_WAV) && KFR_ENABLE_WAV
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_IMPLEMENTATION
#include "../dr/dr_wav.h"
#endif
#if defined(KFR_ENABLE_FLAC) && KFR_ENABLE_FLAC
#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO
#include "../dr/dr_flac.h"
#endif

CMT_PRAGMA_GNU(GCC diagnostic pop)
