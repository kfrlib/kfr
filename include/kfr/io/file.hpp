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
#include <span>
#include <string>
#include <vector>
#ifdef KFR_USE_STD_FILESYSTEM
#include <filesystem>
#endif
#include <kfr/thirdparty/expected/expected.hpp>

namespace kfr
{

using tl::expected;
using tl::unexpected;

enum class open_file_mode
{
    read_existing,
    write_new,
    read_write_existing,
    read_write_new,
    append_existing,
};

#ifdef KFR_USE_STD_FILESYSTEM
#define KFR_FILEPATH(s) (s)
using file_path = std::filesystem::path;
#else
#ifdef KFR_OS_WIN
#define KFR_FILEPATH_PREFIX_CONCAT(x, y) x##y
#define KFR_FILEPATH(s) (KFR_FILEPATH_PREFIX_CONCAT(L, s))
using file_path = std::wstring;

namespace details
{
std::wstring utf8_to_wstring(std::string_view str);
}

#else
#define KFR_FILEPATH(s) (s)
using file_path = std::string;
#endif
#endif

using filepath = file_path;

[[nodiscard]] expected<FILE*, std::error_code> fopen_path(const file_path& path,
                                                          open_file_mode mode) noexcept;

#if defined _MSC_VER // MSVC
#define KFR_IO_SEEK_64 _fseeki64
#define KFR_IO_TELL_64 _ftelli64
#elif defined _WIN32 // MingW
#define KFR_IO_SEEK_64 fseeko64
#define KFR_IO_TELL_64 ftello64
#else // macOS, Linux
#define KFR_IO_SEEK_64 fseeko
#define KFR_IO_TELL_64 ftello
#endif

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
    [[nodiscard]] virtual uint64_t tell() const                         = 0;
    [[nodiscard]] virtual bool seek(int64_t offset, seek_origin origin) = 0;
    [[nodiscard]] bool seek(int64_t offset, int origin)
    {
        return seek(offset, static_cast<seek_origin>(origin));
    }

    [[nodiscard]] std::optional<uint64_t> size()
    {
        uint64_t pos = tell();
        if (!seek(0, seek_origin::end))
            return std::nullopt;
        uint64_t size = tell();
        if (!seek(pos, seek_origin::begin))
            return std::nullopt;
        return size;
    }
};

/// @brief Base class for all typed readers
template <typename T = void>
struct abstract_reader : abstract_stream<T>
{
    [[nodiscard]] virtual size_t read(T* data, size_t size) = 0;

    [[nodiscard]] size_t read(std::convertible_to<std::span<T>> auto&& data)
        requires(!std::is_void_v<T>)
    {
        std::span<T> s(data);
        return read(s.data(), s.size());
    }

    [[nodiscard]] univector<T> read(size_t size)
    {
        univector<T> result(size);
        this->read(result);
        return result;
    }
    template <typename U = T>
        requires(!std::is_void_v<U>)
    [[nodiscard]] bool read(U& data)
    {
        return read(&data, 1) == 1;
    }
};

/// @brief Base class for all typed writers
template <typename T = void>
struct abstract_writer : abstract_stream<T>
{
    [[nodiscard]] virtual size_t write(const T* data, size_t size) = 0;

    [[nodiscard]] size_t write(std::convertible_to<std::span<const T>> auto&& data)
        requires(!std::is_void_v<T>)
    {
        std::span<const T> s(data);
        return write(s.data(), s.size());
    }

    template <typename U = T>
        requires(!std::is_void_v<U>)
    [[nodiscard]] bool write(const U& data)
    {
        return write(&data, 1) == 1;
    }
};

template <typename From, typename To>
struct reader_adapter : abstract_reader<To>
{
    static_assert(element_size<To>() > element_size<From>() &&
                  element_size<To>() % element_size<From>() == 0);
    constexpr static uint64_t scale = element_size<To>() / element_size<To>();

    reader_adapter(std::shared_ptr<abstract_reader<From>> reader) : reader(std::move(reader)) {}

    [[nodiscard]] size_t read(To* data, size_t size) final
    {
        return reader->read(reinterpret_cast<From*>(data), size * scale) / scale;
    }
    [[nodiscard]]
    bool seek(int64_t offset, seek_origin origin) final
    {
        return reader->seek(offset * scale, origin);
    }

    [[nodiscard]] uint64_t tell() const final { return reader->tell() / scale; }

    std::shared_ptr<abstract_reader<From>> reader;
};

template <typename From, typename To = void>
struct writer_adapter : abstract_writer<To>
{
    static_assert(element_size<To>() > element_size<From>() &&
                  element_size<To>() % element_size<From>() == 0);
    constexpr static uint64_t scale = element_size<To>() / element_size<To>();

    writer_adapter(std::shared_ptr<abstract_writer<From>> writer) : writer(std::move(writer)) {}

    [[nodiscard]] size_t write(const To* data, size_t size) final
    {
        return writer->write(reinterpret_cast<const From*>(data), size * scale) / scale;
    }

    [[nodiscard]] bool seek(int64_t offset, seek_origin origin) final
    {
        return writer->seek(offset * scale, origin);
    }

    [[nodiscard]] uint64_t tell() const final { return writer->tell() / scale; }

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
    file_handle(FILE* file);
    file_handle()                   = delete;
    file_handle(const file_handle&) = delete;
    file_handle(file_handle&& handle);
    ~file_handle();
    FILE* file;
    void swap(file_handle& handle);
};

/// @brief Typed file reader
template <typename T = void>
struct file_reader : abstract_reader<T>
{
    file_reader(file_handle&& handle) : handle(std::move(handle)) {}

    ~file_reader() override {}

    size_t read(T* data, size_t size) final { return fread(data, element_size<T>(), size, handle.file); }

    using abstract_reader<T>::read;

    uint64_t tell() const final { return KFR_IO_TELL_64(handle.file); }

    bool seek(int64_t offset, seek_origin origin) final
    {
        return !KFR_IO_SEEK_64(handle.file, offset, static_cast<int>(origin));
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

    uint64_t tell() const final { return KFR_IO_TELL_64(handle.file); }

    bool seek(int64_t offset, seek_origin origin) final
    {
        return !KFR_IO_SEEK_64(handle.file, offset, static_cast<int>(origin));
    }

    file_handle handle;
};

/// @brief Opens typed file for reading
template <typename T = void>
inline std::shared_ptr<file_reader<T>> open_file_for_reading(const filepath& path)
{
    auto f = fopen_path(path, open_file_mode::read_existing);
    return f ? std::make_shared<file_reader<T>>(*f) : nullptr;
}

/// @brief Opens typed file for writing
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_writing(const filepath& path)
{
    auto f = fopen_path(path, open_file_mode::write_new);
    return f ? std::make_shared<file_writer<T>>(*f) : nullptr;
}

/// @brief Opens typed file for appending
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_appending(const filepath& path)
{
    auto f = fopen_path(path, open_file_mode::append_existing);
    return f ? std::make_shared<file_writer<T>>(*f) : nullptr;
}

#if defined KFR_OS_WIN && !defined KFR_USE_STD_FILESYSTEM
/// @brief Opens typed file for reading
template <typename T = void>
inline std::shared_ptr<file_reader<T>> open_file_for_reading(const std::string& path)
{
    return open_file_for_reading<T>(details::utf8_to_wstring(path));
}

/// @brief Opens typed file for writing
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_writing(const std::string& path)
{
    return open_file_for_writing<T>(details::utf8_to_wstring(path));
}

/// @brief Opens typed file for appending
template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_appending(const std::string& path)
{
    return open_file_for_appending<T>(details::utf8_to_wstring(path));
}
#endif

} // namespace kfr
