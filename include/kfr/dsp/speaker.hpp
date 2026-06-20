/** @addtogroup dsp_extra
 *  @{
 */
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

#include <span>
#include <array>

namespace kfr
{

/**
 * @brief Speaker types (channel positions).
 *
 * Matches VST3 speaker type definitions. Several enumerators provide short aliases
 * (e.g. `M` for `Mono`, `L` for `Left`) that share the same numeric value.
 */
enum class speaker_type : int
{
    None          = -1, ///< No speaker / unspecified.
    Mono          = 0, ///< Mono (front center, single channel).
    M             = static_cast<int>(Mono), ///< Alias for Mono.
    Left          = 1, ///< Front left.
    L             = static_cast<int>(Left), ///< Alias for Left.
    Right         = 2, ///< Front right.
    R             = static_cast<int>(Right), ///< Alias for Right.
    Center        = 3, ///< Front center.
    C             = static_cast<int>(Center), ///< Alias for Center.
    Lfe           = 4, ///< Low-frequency effects (subwoofer).
    Ls            = 5, ///< Left surround.
    LeftSurround  = static_cast<int>(Ls), ///< Alias for Ls.
    Rs            = 6, ///< Right surround.
    RightSurround = static_cast<int>(Rs), ///< Alias for Rs.
    Lc            = 7, ///< Left center (front, between L and C).
    Rc            = 8, ///< Right center (front, between R and C).
    S             = 9, ///< Rear surround (single).
    Cs            = static_cast<int>(S), ///< Alias for S (center surround).
    Sl            = 10, ///< Side left.
    Sr            = 11, ///< Side right.
    Tm            = 12, ///< Top middle.
    Tfl           = 13, ///< Top front left.
    Tfc           = 14, ///< Top front center.
    Tfr           = 15, ///< Top front right.
    Trl           = 16, ///< Top rear left.
    Trc           = 17, ///< Top rear center.
    Trr           = 18, ///< Top rear right.
    Lfe2          = 19 ///< Second low-frequency effects channel.
};

/**
 * @brief Predefined speaker arrangements.
 *
 * Matches VST3 speaker arrangement definitions. Each value names a canonical
 * channel layout (e.g. `Stereo`, `Music51`, `Cine71`); see
 * @ref arrangement_speakers() for the channel sequence of each arrangement.
 */
enum class speaker_arrangement : int
{
    None           = -1, ///< No arrangement / unspecified.
    Mono           = 0, ///< Single mono channel (M).
    Stereo         = 1, ///< L, R.
    StereoSurround = 2, ///< Ls, Rs.
    StereoCenter   = 3, ///< Lc, Rc.
    StereoSide     = 4, ///< Sl, Sr.
    StereoCLfe     = 5, ///< C, Lfe.
    Cine30         = 6, ///< L, R, C (cinema 3.0).
    Music30        = 7, ///< L, R, S (music 3.0).
    Cine31         = 8, ///< L, R, C, Lfe (cinema 3.1).
    Music31        = 9, ///< L, R, S, Lfe (music 3.1).
    Cine40         = 10, ///< L, R, C, Cs (cinema 4.0).
    Music40        = 11, ///< L, R, Ls, Rs (music 4.0).
    Cine41         = 12, ///< L, R, C, Cs, Lfe (cinema 4.1).
    Music41        = 13, ///< L, R, Ls, Rs, Lfe (music 4.1).
    Arr50          = 14, ///< L, R, C, Ls, Rs (5.0).
    Arr51          = 15, ///< L, R, C, Lfe, Ls, Rs (5.1).
    Cine60         = 16, ///< L, R, C, Ls, Rs, Cs (cinema 6.0).
    Music60        = 17, ///< L, R, Ls, Rs, Sl, Sr (music 6.0).
    Cine61         = 18, ///< L, R, C, Lfe, Ls, Rs, Cs (cinema 6.1).
    Music61        = 19, ///< L, R, Lfe, Ls, Rs, Sl, Sr (music 6.1).
    Cine70         = 20, ///< L, R, C, Ls, Rs, Lc, Rc (cinema 7.0).
    Music70        = 21, ///< L, R, C, Ls, Rs, Sl, Sr (music 7.0).
    Cine71         = 22, ///< L, R, C, Lfe, Ls, Rs, Lc, Rc (cinema 7.1).
    Music71        = 23, ///< L, R, C, Lfe, Ls, Rs, Sl, Sr (music 7.1).
    Cine80         = 24, ///< L, R, C, Ls, Rs, Lc, Rc, Cs (cinema 8.0).
    Music80        = 25, ///< L, R, C, Ls, Rs, Sl, Sr, Cs (music 8.0).
    Cine81         = 26, ///< L, R, C, Lfe, Ls, Rs, Lc, Rc, Cs (cinema 8.1).
    Music81        = 27, ///< L, R, C, Lfe, Ls, Rs, Sl, Sr, Cs (music 8.1).
    Arr102         = 28 ///< L, R, C, Lfe, Ls, Rs, Lc, Rc, Tfl, Tfr, Trl, Trr (10.2).
};

/** @deprecated Use speaker_type instead. */
using Speaker [[deprecated("Use speaker_type instead")]] = speaker_type;
/** @deprecated Use speaker_arrangement instead. */
using SpeakerArrangement [[deprecated("Use speaker_arrangement instead")]] = speaker_arrangement;

/**
 * @brief Returns the canonical channel list for a speaker arrangement.
 *
 * Maps a speaker_arrangement to an ordered, immutable sequence of speaker_type values.
 * The returned span references static storage valid for the program lifetime and performs no allocations.
 *
 * @param arr The speaker arrangement to resolve.
 * @return Ordered channels for the arrangement, or an empty span if unsupported
 *         (e.g. speaker_arrangement::None).
 */
std::span<const speaker_type> arrangement_speakers(speaker_arrangement arr) noexcept;

/**
 * @brief Returns a predefined speaker arrangement for a given number of channels.
 *
 * Selects a canonical layout for the given channel count, preferring the
 * `Music`/`Arr` variants over the corresponding `Cine` variants where both exist
 * (e.g. 3 channels -> Music30, 8 channels -> Music71).
 *
 * @param count Number of channels in the layout.
 * @return A matching predefined speaker_arrangement, or speaker_arrangement::None
 *         if no predefined layout exists for the given count.
 */
speaker_arrangement arrangement_for_channels(size_t count) noexcept;

} // namespace kfr
