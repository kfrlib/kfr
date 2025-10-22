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

#include "dsp.hpp"

#include "audio/data.hpp"
#include "audio/decoder.hpp"
#include "audio/encoder.hpp"
#include "audio/io.hpp"

namespace kfr
{
const char* library_version_audio();
/**
 * @brief Returns a comma-separated list of enabled audio codecs.
 *
 * Example: "wav,mp3,w64,aiff,caf,rf64,bw64,raw,flac,alac"
 *
 * @return const char*
 */
const char* library_version_codecs();
} // namespace kfr
