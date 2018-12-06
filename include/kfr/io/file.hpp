/** @addtogroup io
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
  This file is part of KFR

  KFR is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
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

#include "../base/function.hpp"
#include "../base/univector.hpp"
#include "../base/vec.hpp"
#include <cstdio>
#include <string>

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

inline FILE* fopen_portable(const filepath_char* path, const filepath_char* mode)
{
#ifdef CMT_OS_WIN
    FILE* f   = nullptr;
    errno_t e = _wfopen_s(&f, path, mode);
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

enum class seek_origin : int
{
    current = SEEK_CUR,
    begin   = SEEK_SET,
    end     = SEEK_END,
};

template <typename T = void>
struct abstract_stream
{
    virtual ~abstract_stream() {}
    virtual imax tell() const                          = 0;
    virtual bool seek(imax offset, seek_origin origin) = 0;
    bool seek(imax offset, int origin) { return seek(offset, static_cast<seek_origin>(origin)); }
};

template <typename T = void>
struct abstract_reader : abstract_stream<T>
{
    virtual size_t read(T* data, size_t size) = 0;
};

template <typename T = void>
struct abstract_writer : abstract_stream<T>
{
    virtual size_t write(const T* data, size_t size) = 0;
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

using binary_reader = abstract_reader<>;
using binary_writer = abstract_writer<>;
using byte_reader   = abstract_reader<u8>;
using byte_writer   = abstract_writer<u8>;
using f32_reader    = abstract_reader<f32>;
using f32_writer    = abstract_writer<f32>;

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

template <typename T = void>
struct file_reader : abstract_reader<T>
{
    file_reader(file_handle&& handle) : handle(std::move(handle)) {}
    ~file_reader() override {}
    size_t read(T* data, size_t size) final { return fread(data, element_size<T>(), size, handle.file); }

    imax tell() const final { return IO_TELL_64(handle.file); }
    bool seek(imax offset, seek_origin origin) final
    {
        return !IO_SEEK_64(handle.file, offset, static_cast<int>(origin));
    }
    file_handle handle;
};

template <typename T = void>
struct file_writer : abstract_writer<T>
{
    file_writer(file_handle&& handle) : handle(std::move(handle)) {}
    ~file_writer() override {}
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

template <typename T = void>
inline std::shared_ptr<file_reader<T>> open_file_for_reading(const filepath& path)
{
    return std::make_shared<file_reader<T>>(fopen_portable(path.c_str(), KFR_FILEPATH("rb")));
}

template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_writing(const filepath& path)
{
    return std::make_shared<file_writer<T>>(fopen_portable(path.c_str(), KFR_FILEPATH("wb")));
}

template <typename T = void>
inline std::shared_ptr<file_writer<T>> open_file_for_appending(const filepath& path)
{
    return std::make_shared<file_writer<T>>(fopen_portable(path.c_str(), KFR_FILEPATH("ab")));
}

namespace internal
{
struct expression_file_base
{
    expression_file_base()                            = delete;
    expression_file_base(const expression_file_base&) = delete;
    expression_file_base(expression_file_base&&)      = default;
    expression_file_base(FILE* file) : file(file) {}
    ~expression_file_base() { fclose(file); }
    bool ok() const { return file != nullptr; }
    FILE* file;
};

struct expression_sequential_file_writer : expression_file_base, output_expression
{
    using expression_file_base::expression_file_base;
    template <typename U, size_t N>
    void operator()(coutput_t, size_t, const vec<U, N>& value)
    {
        write(value);
    }
    template <typename U>
    void write(const U& value)
    {
        write(&value, 1);
    }
    template <typename U>
    void write(const U* value, size_t size)
    {
        fwrite(value, 1, sizeof(U) * size, file);
    }
};

struct expression_sequential_file_reader : expression_file_base, input_expression
{
    using expression_file_base::expression_file_base;
    template <typename U, size_t N>
    vec<U, N> operator()(cinput_t, size_t, vec_t<U, N>) const
    {
        vec<U, N> input = qnan;
        read(input);
        return input;
    }
    template <typename U>
    void read(U& value) const
    {
        fread(std::addressof(value), 1, sizeof(U), file);
    }
};

template <typename T>
struct expression_file_writer : expression_file_base, output_expression
{
    using expression_file_base::expression_file_base;
    template <size_t N>
    void operator()(coutput_t, size_t index, const vec<T, N>& value)
    {
        if (position != index)
            fseeko(file, static_cast<off_t>(index * sizeof(T)), SEEK_SET);
        const vec<T, N> output = value;
        fwrite(output.data(), sizeof(T), output.size(), file);
        position = index + N;
    }
    size_t position = 0;
};

template <typename T>
struct expression_file_reader : expression_file_base, input_expression
{
    using expression_file_base::expression_file_base;
    template <size_t N>
    vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        if (position != index)
            fseeko(file, static_cast<off_t>(index * sizeof(T)), SEEK_SET);
        vec<T, N> input = qnan;
        fread(input.data(), sizeof(T), input.size(), file);
        position = index + N;
        return input;
    }
    mutable size_t position = 0;
};
} // namespace internal

} // namespace kfr
