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

#include "base.hpp"

#include "dsp/biquad.hpp"
#include "dsp/biquad_design.hpp"
#include "dsp/dcremove.hpp"
#include "dsp/delay.hpp"
#include "dsp/ebu.hpp"
#include "dsp/fir.hpp"
#include "dsp/fir_design.hpp"
#include "dsp/fracdelay.hpp"
#include "dsp/goertzel.hpp"
#include "dsp/iir_design.hpp"
#include "dsp/mixdown.hpp"
#include "dsp/oscillators.hpp"
#include "dsp/sample_rate_conversion.hpp"
#include "dsp/speaker.hpp"
#include "dsp/special.hpp"
#include "dsp/units.hpp"
#include "dsp/waveshaper.hpp"
#include "dsp/weighting.hpp"
#include "dsp/window.hpp"
