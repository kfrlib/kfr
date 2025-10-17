/** @addtogroup dft
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
#include <kfr/cident.h>
#if !defined KFR_SKIP_IF_NON_X86 || defined(KFR_ARCH_X86)

#include <kfr/dsp/speaker.hpp>

namespace kfr
{

std::span<const speaker_type> arrangement_speakers(speaker_arrangement arr) noexcept
{
    using enum speaker_type;

    switch (arr)
    {
    case speaker_arrangement::Mono:
    {
        static constexpr std::array speakers{ M };
        return speakers;
    }
    case speaker_arrangement::Stereo:
    {
        static constexpr std::array speakers{ L, R };
        return speakers;
    }
    case speaker_arrangement::StereoSurround:
    {
        static constexpr std::array speakers{ Ls, Rs };
        return speakers;
    }
    case speaker_arrangement::StereoCenter:
    {
        static constexpr std::array speakers{ Lc, Rc };
        return speakers;
    }
    case speaker_arrangement::StereoSide:
    {
        static constexpr std::array speakers{ Sl, Sr };
        return speakers;
    }
    case speaker_arrangement::StereoCLfe:
    {
        static constexpr std::array speakers{ C, Lfe };
        return speakers;
    }
    case speaker_arrangement::Cine30:
    {
        static constexpr std::array speakers{ L, R, C };
        return speakers;
    }
    case speaker_arrangement::Music30:
    {
        static constexpr std::array speakers{ L, R, S };
        return speakers;
    }
    case speaker_arrangement::Cine31:
    {
        static constexpr std::array speakers{ L, R, C, Lfe };
        return speakers;
    }
    case speaker_arrangement::Music31:
    {
        static constexpr std::array speakers{ L, R, S, Lfe };
        return speakers;
    }
    case speaker_arrangement::Cine40:
    {
        static constexpr std::array speakers{ L, R, C, Cs };
        return speakers;
    }
    case speaker_arrangement::Music40:
    {
        static constexpr std::array speakers{ L, R, Ls, Rs };
        return speakers;
    }
    case speaker_arrangement::Cine41:
    {
        static constexpr std::array speakers{ L, R, C, Cs, Lfe };
        return speakers;
    }
    case speaker_arrangement::Music41:
    {
        static constexpr std::array speakers{ L, R, Ls, Rs, Lfe };
        return speakers;
    }
    case speaker_arrangement::Arr50:
    {
        static constexpr std::array speakers{ L, R, C, Ls, Rs };
        return speakers;
    }
    case speaker_arrangement::Arr51:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs };
        return speakers;
    }
    case speaker_arrangement::Cine60:
    {
        static constexpr std::array speakers{ L, R, C, Ls, Rs, Cs };
        return speakers;
    }
    case speaker_arrangement::Music60:
    {
        static constexpr std::array speakers{ L, R, Ls, Rs, Sl, Sr };
        return speakers;
    }
    case speaker_arrangement::Cine61:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs, Cs };
        return speakers;
    }
    case speaker_arrangement::Music61:
    {
        static constexpr std::array speakers{ L, R, Lfe, Ls, Rs, Sl, Sr };
        return speakers;
    }
    case speaker_arrangement::Cine70:
    {
        static constexpr std::array speakers{ L, R, C, Ls, Rs, Lc, Rc };
        return speakers;
    }
    case speaker_arrangement::Music70:
    {
        static constexpr std::array speakers{ L, R, C, Ls, Rs, Sl, Sr };
        return speakers;
    }
    case speaker_arrangement::Cine71:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs, Lc, Rc };
        return speakers;
    }
    case speaker_arrangement::Music71:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs, Sl, Sr };
        return speakers;
    }
    case speaker_arrangement::Cine80:
    {
        static constexpr std::array speakers{ L, R, C, Ls, Rs, Lc, Rc, Cs };
        return speakers;
    }
    case speaker_arrangement::Music80:
    {
        static constexpr std::array speakers{ L, R, C, Ls, Rs, Sl, Sr, Cs };
        return speakers;
    }
    case speaker_arrangement::Cine81:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs, Lc, Rc, Cs };
        return speakers;
    }
    case speaker_arrangement::Music81:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs, Sl, Sr, Cs };
        return speakers;
    }
    case speaker_arrangement::Arr102:
    {
        static constexpr std::array speakers{ L, R, C, Lfe, Ls, Rs, Lc, Rc, Tfl, Tfr, Trl, Trr };
        return speakers;
    }
    default:
    {
        static constexpr std::array<speaker_type, 0> speakers{};
        return speakers;
    }
    }
}

speaker_arrangement arrangement_for_channels(size_t count) noexcept
{
    switch (count)
    {
    case 1:
        return speaker_arrangement::Mono;
    case 2:
        return speaker_arrangement::Stereo;
    case 3:
        return speaker_arrangement::Music30;
    case 4:
        return speaker_arrangement::Music40;
    case 5:
        return speaker_arrangement::Arr50;
    case 6:
        return speaker_arrangement::Arr51;
    case 7:
        return speaker_arrangement::Music61;
    case 8:
        return speaker_arrangement::Music71;
    case 9:
        return speaker_arrangement::Music81;
    case 12:
        return speaker_arrangement::Arr102;
    default:
        return speaker_arrangement::None;
    }
}
} // namespace kfr

#endif
