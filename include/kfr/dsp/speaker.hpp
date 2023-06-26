/** @addtogroup dsp_extra
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

namespace kfr
{

enum class Speaker : int
{
    None          = -1,
    Mono          = 0,
    M             = static_cast<int>(Mono),
    Left          = 1,
    L             = static_cast<int>(Left),
    Right         = 2,
    R             = static_cast<int>(Right),
    Center        = 3,
    C             = static_cast<int>(Center),
    Lfe           = 4,
    Ls            = 5,
    LeftSurround  = static_cast<int>(Ls),
    Rs            = 6,
    RightSurround = static_cast<int>(Rs),
    Lc            = 7,
    Rc            = 8,
    S             = 9,
    Cs            = static_cast<int>(S),
    Sl            = 10,
    Sr            = 11,
    Tm            = 12,
    Tfl           = 13,
    Tfc           = 14,
    Tfr           = 15,
    Trl           = 16,
    Trc           = 17,
    Trr           = 18,
    Lfe2          = 19
};

enum class SpeakerArrangement : int
{
    None           = -1,
    Mono           = 0,
    Stereo         = 1,
    StereoSurround = 2,
    StereoCenter   = 3,
    StereoSide     = 4,
    StereoCLfe     = 5,
    Cine30         = 6,
    Music30        = 7,
    Cine31         = 8,
    Music31        = 9,
    Cine40         = 10,
    Music40        = 11,
    Cine41         = 12,
    Music41        = 13,
    Arr50          = 14,
    Arr51          = 15,
    Cine60         = 16,
    Music60        = 17,
    Cine61         = 18,
    Music61        = 19,
    Cine70         = 20,
    Music70        = 21,
    Cine71         = 22,
    Music71        = 23,
    Cine80         = 24,
    Music80        = 25,
    Cine81         = 26,
    Music81        = 27,
    Arr102         = 28
};

} // namespace kfr
