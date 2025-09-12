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
#pragma once

#include <cstdint>
#ifdef KFR_USE_STD_FILESYSTEM
#include <filesystem>
#endif
#include <kfr/thirdparty/expected/expected.hpp>
#include <kfr/audio/data.hpp>

namespace kfr
{

using tl::expected;
using tl::unexpected;

enum class audiofile_error : uint32_t
{
    unknown         = 0,
    io_error        = 1,
    format_error    = 2,
    internal_error  = 3,
    too_large_error = 4, // data chunk too large for standard WAV
    end_of_file     = 5,
    abort           = 6,
};

inline std::string to_string(audiofile_error err)
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
    case audiofile_error::too_large_error:
        return "too_large_error";
    case audiofile_error::end_of_file:
        return "end_of_file";
    case audiofile_error::abort:
        return "abort";
    default:
        return "(invalid audiofile_error)";
    }
}

constexpr inline size_t default_audio_frames_to_read = 65536;

enum class open_file_mode
{
    read_existing,
    write_new,
    read_write_existing,
    read_write_new,
    append_existing,
};

#ifdef KFR_USE_STD_FILESYSTEM
using file_path = std::filesystem::path;
#else
#ifdef KFR_OS_WIN
using file_path = std::wstring;

namespace details
{
std::wstring utf8_to_wstring(std::string_view str);
}

#else
using file_path = std::string;
#endif
#endif

[[nodiscard]] expected<FILE*, std::error_code> fopen_path(const file_path& path,
                                                          open_file_mode mode) noexcept;

} // namespace kfr
