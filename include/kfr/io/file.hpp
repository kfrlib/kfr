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
        fwrite(std::addressof(value), 1, sizeof(U), file);
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
}

inline internal::expression_sequential_file_reader sequential_file_reader(const std::string& file_name)
{
    return internal::expression_sequential_file_reader(fopen(file_name.c_str(), "rb"));
}
inline internal::expression_sequential_file_writer sequential_file_writer(const std::string& file_name)
{
    return internal::expression_sequential_file_writer(fopen(file_name.c_str(), "wb"));
}

template <typename T = u8>
internal::expression_file_reader<T> file_reader(const std::string& file_name)
{
    return internal::expression_file_reader<T>(fopen(file_name.c_str(), "rb"));
}
template <typename T = u8>
internal::expression_file_writer<T> file_writer(const std::string& file_name)
{
    return internal::expression_file_writer<T>(fopen(file_name.c_str(), "wb"));
}
}
