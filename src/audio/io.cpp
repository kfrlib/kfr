/** @addtogroup audio
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
#include <kfr/audio/io.hpp>

namespace kfr
{

std::string to_string(audiofile_error err)
{
    switch (err)
    {
    case audiofile_error::unknown:
        return "unknown";
    case audiofile_error::io_error:
        return "io_error";
    case audiofile_error::format_error:
        return "format_error";
    case audiofile_error::internal_error:
        return "internal_error";
    case audiofile_error::too_large:
        return "too_large";
    case audiofile_error::end_of_file:
        return "end_of_file";
    case audiofile_error::abort:
        return "abort";
    case audiofile_error::not_implemented:
        return "not_implemented";
    case audiofile_error::invalid_argument:
        return "invalid_argument";
    case audiofile_error::closed:
        return "closed";
    case audiofile_error::empty_file:
        return "empty_file";
    case audiofile_error::not_found:
        return "not_found";
    case audiofile_error::access_denied:
        return "access_denied";
    default:
        return "(invalid audiofile_error: " + std::to_string(uint32_t(err)) + ")";
    }
}
audiofile_error from_error_code(std::error_code ec)
{
    switch (ec.value())
    {
        /* File not found */
#ifdef ENOENT
    case ENOENT: // POSIX, MSVCRT
        return audiofile_error::not_found;
#endif

        /* Permission denied */
#ifdef EACCES
    case EACCES: // POSIX, MSVCRT
        return audiofile_error::access_denied;
#endif
#ifdef EPERM
    case EPERM: // POSIX
        return audiofile_error::access_denied;
#endif

        /* Invalid argument */
#ifdef EINVAL
    case EINVAL: // POSIX, MSVCRT
        return audiofile_error::invalid_argument;
#endif

    /* Anything else: generic I/O error */
    default:
        return audiofile_error::io_error;
    }
}
} // namespace kfr
