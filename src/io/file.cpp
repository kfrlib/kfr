
/** @addtogroup binary_io
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
#include <kfr/io/file.hpp>

#ifdef KFR_OS_WIN
#include <windows.h>
#endif

namespace kfr
{

template <typename Char>
static const Char* mode_string(open_file_mode mode)
{
#define KFR_MODE_STRING(str)                                                                                 \
    if constexpr (std::is_same_v<Char, wchar_t>)                                                             \
        return L##str;                                                                                       \
    else                                                                                                     \
        return str;
    switch (mode)
    {
    case open_file_mode::read_existing:
        KFR_MODE_STRING("rb");
    case open_file_mode::write_new:
        KFR_MODE_STRING("wb");
    case open_file_mode::read_write_existing:
        KFR_MODE_STRING("r+b");
    case open_file_mode::read_write_new:
        KFR_MODE_STRING("w+b");
    case open_file_mode::append_existing:
        KFR_MODE_STRING("ab");
    default:
        KFR_MODE_STRING("");
    }
#undef KFR_MODE_STRING
}

expected<FILE*, std::error_code> fopen_path(const file_path& path, open_file_mode mode) noexcept
{
#ifdef KFR_OS_WIN
    FILE* f = nullptr;
    if (errno_t e = _wfopen_s(&f, path.c_str(), mode_string<wchar_t>(mode)); e != 0)
        return unexpected(std::error_code(e, std::generic_category()));
    if (!f)
        return unexpected(std::error_code(errno, std::generic_category()));
    return f;
#else
    FILE* f = fopen(path.c_str(), mode_string<char>(mode));
    if (!f)
        return unexpected(std::error_code(errno, std::generic_category()));
    return f;
#endif
}

#ifdef KFR_OS_WIN
namespace details
{
std::wstring utf8_to_wstring(std::string_view str)
{
    // Convert using Windows API
    if (str.empty())
        return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
    if (size_needed == 0)
        return {};
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}
} // namespace details
#endif

file_handle::file_handle(file_handle&& handle) : file(nullptr) { swap(handle); }
void file_handle::swap(file_handle& handle) { std::swap(file, handle.file); }
file_handle::file_handle(FILE* file) : file(file) {}
file_handle::~file_handle()
{
    if (file)
    {
        fclose(file);
    }
}
} // namespace kfr
