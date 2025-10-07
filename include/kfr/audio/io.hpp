/** @addtogroup audio
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

#include <cstdint>
#include <kfr/io/file.hpp>
#include <kfr/audio/data.hpp>

namespace kfr
{
/**
 * @enum audiofile_error
 * @brief Enumerates possible error codes for audio file operations.
 */
enum class audiofile_error : uint32_t
{
    unknown          = 0, ///< An unknown error occurred
    io_error         = 1, ///< An input/output error occurred.
    format_error     = 2, ///< The audio file format is invalid or unsupported.
    internal_error   = 3, ///< An internal error occurred.
    too_large        = 4, ///< data chunk too large for standard WAV
    end_of_file      = 5, ///< End of file reached
    abort            = 6, ///< Operation aborted
    not_implemented  = 7, ///< Not implemented
    invalid_argument = 8, ///< Invalid argument passed
    closed           = 9, ///< The audio file is closed
    empty_file       = 10, ///< The file is empty or does not contain any audio data
    not_found        = 11, ///< The file was not found
    access_denied    = 12, ///< Access to the file was denied
};

/**
 * @brief Converts a standard error code to an audiofile_error.
 *
 * This function maps a given std::error_code to the corresponding
 * audiofile_error enumeration value.
 *
 * @param ec The standard error code to convert.
 * @return The corresponding audiofile_error value.
 */
audiofile_error from_error_code(std::error_code ec);

/**
 * @brief Converts an audiofile_error to its string representation.
 *
 * This function provides a human-readable string representation of
 * the given audiofile_error value.
 *
 * @param err The audiofile_error value to convert.
 * @return A string describing the error.
 */
std::string to_string(audiofile_error err);

/**
 * @brief The default number of audio frames to read in one operation.
 *
 * This constant defines the default number of audio frames to process
 * in a single read operation.
 */
constexpr inline size_t default_audio_frames_to_read = 16384;

} // namespace kfr
