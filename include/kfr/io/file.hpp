/** @addtogroup binary_io
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

#include "../base/univector.hpp"
#include "../simd/impl/function.hpp"
#include "../simd/vec.hpp"
#include <cstdio>
#include <string>
#include <vector>

namespace kfr
{

#ifdef CMT_OS_WIN
using filepath_char = wchar_t;
#define KFR_FILEPATH_PREFIX_CONCAT(x, y) x##y
#define KFR_FILEPATH(s) KFR_FILEPATH_PREFIX_CONCAT(L, s)
#else
using filepath_char = char;
#define KFR_FILEPATH(s) s
#endif

using filepath = std::basic_string<filepath_char>;

#if defined _MSC_VER // MSVC
#define IO_SEEK_64 _fseeki64
#define IO_TELL_64 _ftelli64
#elif defined _WIN32 // MingW
#define IO_SEEK_64 fseeko64
#define IO_TELL_64 ftello64
#else // macOS, Linux
#define IO_SEEK_64 fseeko
#define IO_TELL_64 ftello
#endif

/// @brief Opens file using portable path (char* on posix, wchar_t* on windows)
inline FILE* fopen_portable(const filepath_char* path, const filepath_char* mode)
{
#ifdef CMT_OS_WIN
    FILE* f   = nullptr;
    errno_t e = _wfopen_s(&f, path, mode);
    (void)e;
    return f;
#else
    return fopen(path, mode);
#endif
}

template <typename T = void>
constexpr inline size_t element_size()
{
    return sizeof(T);
}
template <>
constexpr inline size_t element_size<void>()
{
    return 1;
}

/// @brief Seek origin
enum class seek_origin : int
{
    current = SEEK_CUR, ///< From the current position
    begin   = SEEK_SET, ///< From the beginning
    end     = SEEK_END, ///< From the end
};

/// @brief Base class for all typed readers and writer
template <typename T = void>
struct abstract_stream
{
    virtual ~abstract_stream() {}
    virtual imax tell() const                          = 0;
    virtual bool seek(imax offset, seek_origin origin) = 0;
    bool seek(imax offset, int origin) { return seek(offset, static_cast<seek_origin>(origin)); }
};

namespace internal_generic
{
struct empty
{
};

} // namespace internal_generic

/// @brief Base class for all typed readers
template <typename T = void>
struct abstract_reader : abstract_stream<T>
{
    virtual size_t read(T* data, size_t size) = 0;

    template <univector_tag Tag>
    size_t read(univector<T, Tag>& data)
    {
        return read(data.data(), data.size());
    }
    size_t read(univector_ref<T>&& data) { return read(data.data(), data.size()); }

    univector<T> read(size_t size)
    {
        univector<T> result(size);
        this->read(result);
        return result;
    }
    bool read(std::conditional_t<std::is_void_v<T>, internal_generic::empty, T>& data)
    {
        return read(&data, 1) == 1;
    }
};

/// @brief Base class for all typed writers
template <typename T = void>
struct abstract_writer : abstract_stream<T>
{
    virtual size_t write(const T* data, size_t size) = 0;

    template <univector_tag Tag>
    size_t write(const univector<T, Tag>& data)
    {
        return write(data.data(), data.size());
    }
    size_t write(univector_ref<const T>&& data) { return write(data.data(), data.size()); }
    bool write(const std::conditional_t<std::is_void_v<T>, internal_generic::empty, T>& data)
    {
        return write(&data, 1) == 1;
    }
};

template <typename From, typename To = void>
struct reader_adapter : abstract_reader<To>
{
    static_assert(element_size<From>() % element_size<To>() == 0 ||
                      element_size<To>() % element_size<From>() == 0,
                  "From and To sizes must be compatible");
    reader_adapter(std::shared_ptr<abstract_reader<From>>&& reader) : reader(std::move(reader)) {}
    virtual size_t read(To* data, size_t size) final
    {
        return reader->read(reinterpret_cast<From*>(data), size * element_size<From>() / element_size<To>()) *
               element_size<To>() / element_size<From>();
    }
    std::shared_ptr<abstract_reader<From>> reader;
};

template <typename From, typename To = void>
struct writer_adapter : abstract_writer<To>
{
    static_assert(element_size<From>() % element_size<To>() == 0 ||
                      element_size<To>() % element_size<From>() == 0,
                  "From and To sizes must be compatible");
    writer_adapter(std::shared_ptr<abstract_writer<From>>&& writer) : writer(std::move(writer)) {}
    virtual size_t write(const To* data, size_t size) final
    {
        return writer->write(reinterpret_cast<const From*>(data),
                             size * element_size<From>() / element_size<To>()) *
               element_size<To>() / element_size<From>();
    }
    std::shared_ptr<abstract_writer<From>> writer;
};

/// @brief Binary reader
using binary_reader = abstract_reader<>;

/// @brief Binary writer
using binary_writer = abstract_writer<>;

/// @brief Byte reader
using byte_reader = abstract_reader<u8>;

/// @brief Byte writer
using byte_writer = abstract_writer<u8>;

/// @brief float reader
using f32_reader = abstract_reader<f32>;

/// @brief float writer
using f32_writer = abstract_writer<f32>;

struct file_handle
{
    file_handle(FILE* file) : file(file) {}
    file_handle()                   = delete;
    file_handle(const file_handle&) = delete;
    file_handle(file_handle&& handle) : file(nullptr) { swap(handle); }
    ~file_handle()
    {
        if (file)
        {
            fclose(file);
        }
    }
    FILE* file;
    void swap(file_handle& handle) { std::swap(file, handle.file); }
};

/// @brief Typed file reader
template <typename T = void>
struct file_reader : abstract_reader<T>
{
    file_reader(file_handle&& handle) : handle(std::move(handle)) {}
    ~file_reader() override {}
    size_t read(T* data, size_t size) final { return fread(data, element_size<T>(), size, handle.file); }

    using abstract_reader<T>::read;

    imax tell() const final { return IO_TELL_64(handle.file); }
    bool seek(imax offset, seek_origin origin) final
    {
        return !IO_SEEK_64(handle.file, offset, static_cast<int>(origin));
    }
    file_handle handle;
};

/// @brief Typed file writer
template <typename T = void>
struct file_writer : abstract_writer<T>
{
    file_writer(file_handle&& handle) : handle(std::move(handle)) {}
    ~file_writer() override {}

    using abstract_writer<T>::write;
    size_t write(const T* data, size_t size) final
    {
        return fwrite(data, element_size<T>(), size, handle.file);
    }
    imax tell() const final { return IO_TELL_64(handle.file); }
    bool seek(imax offset, seek_origin origin) final
    {
        return !IO_SEEK_64(handle.file, offset, static_cast<int>(origin));
    }
    file_handle handle;
};

/// @brief Opens typed file for reading
template <typename T = void>
inline std::shared_ptr<file_reader<T>> open_file_for_reading(const filepath& path)
{
    return std::make_shared<file_reader<T>>(fopen_portable(path.c_str(), KFR_FILEPATH("rb")));
}

/// @brief Opens typed file for writing
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_writing(const filepath& path)
{
    return std::make_shared<file_writer<T>>(fopen_portable(path.c_str(), KFR_FILEPATH("wb")));
}

/// @brief Opens typed file for appending
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_appending(const filepath& path)
{
    return std::make_shared<file_writer<T>>(fopen_portable(path.c_str(), KFR_FILEPATH("ab")));
}

#ifdef CMT_OS_WIN
/// @brief Opens typed file for reading
template <typename T = void>
inline std::shared_ptr<file_reader<T>> open_file_for_reading(const std::string& path)
{
    return std::make_shared<file_reader<T>>(fopen(path.c_str(), "rb"));
}

/// @brief Opens typed file for writing
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_writing(const std::string& path)
{
    return std::make_shared<file_writer<T>>(fopen(path.c_str(), "wb"));
}

/// @brief Opens typed file for appending
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_appending(const std::string& path)
{
    return std::make_shared<file_writer<T>>(fopen(path.c_str(), "ab"));
}
#endif

} // namespace kfr
