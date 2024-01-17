/** @addtogroup filter
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

#include "basic_expressions.hpp"
#include "expression.hpp"
#include "handle.hpp"
#include "univector.hpp"

namespace kfr
{

/// @brief Abstract base class for filters with one argument. Mainly for DSP
template <typename T>
class filter
{
public:
    virtual ~filter() {}

    /// @brief Resets internal state (such as delay line)
    virtual void reset() {}

    /// @brief Applies filter to a static array
    template <size_t Size>
    void apply(T (&buffer)[Size])
    {
        process_buffer(buffer, buffer, Size);
    }

    /// @brief Applies filter to a static array and writes the result to another array
    template <size_t Size>
    void apply(T (&dest)[Size], T (&src)[Size])
    {
        process_buffer(dest, src, Size);
    }

    /// @brief Applies filter to a univector
    template <univector_tag Tag>
    void apply(univector<T, Tag>& buffer)
    {
        process_buffer(buffer.data(), buffer.data(), buffer.size());
    }

    /// @brief Applies filter to a univector and writes the result to another univector
    template <univector_tag Tag1, univector_tag Tag2>
    void apply(univector<T, Tag1>& dest, const univector<T, Tag2>& src)
    {
        if (dest.empty())
            dest.resize(src.size());
        process_buffer(dest.data(), src.data(), std::min(dest.size(), src.size()));
    }

    void apply(T* buffer, size_t size) { process_buffer(buffer, buffer, size); }

    void apply(T* dest, const T* src, size_t size) { process_buffer(dest, src, size); }

    template <univector_tag Tag>
    void apply(univector<T, Tag>& dest, const expression_handle<T, 1>& src)
    {
        process_expression(dest.data(), src, size_min(dest.size(), src.size()));
    }

    void apply(T* dest, const expression_handle<T, 1>& src, size_t size)
    {
        process_expression(dest, src, size_min(size, src.size()));
    }

    template <univector_tag Tag, typename Expr, KFR_ENABLE_IF(is_input_expression<Expr>)>
    void apply(univector<T, Tag>& dest, const Expr& src)
    {
        static_assert(expression_dims<Expr> == 1);
        process_expression(dest.data(), to_handle(src), size_min(dest.size(), get_shape(src).front()));
    }

    template <typename Expr, KFR_ENABLE_IF(is_input_expression<Expr>)>
    void apply(T* dest, const Expr& src, size_t size)
    {
        process_expression(dest, to_handle(src), size_min(size, src.size()));
    }

protected:
    virtual void process_buffer(T* dest, const T* src, size_t size)                           = 0;
    virtual void process_expression(T* dest, const expression_handle<T, 1>& src, size_t size) = 0;
};

template <typename T>
class expression_filter : public filter<T>
{
public:
    explicit expression_filter(expression_handle<T, 1> filter_expr) : filter_expr(std::move(filter_expr)) {}

protected:
    expression_filter() = default;
    void process_buffer(T* dest, const T* src, size_t size) override
    {
        substitute(filter_expr, to_handle(make_univector(src, size)));
        process(make_univector(dest, size), filter_expr, shape<1>(0), shape<1>(size));
    }
    void process_expression(T* dest, const expression_handle<T, 1>& src, size_t size) override
    {
        substitute(filter_expr, src);
        process(make_univector(dest, size), filter_expr, shape<1>(0), shape<1>(size));
    }

    expression_handle<T, 1> filter_expr;
};

inline namespace CMT_ARCH_NAME
{

/// @brief Converts expression with placeholder to filter. Placeholder and filter must have the same type
template <typename E, typename T = expression_value_type<E>>
KFR_INTRINSIC expression_filter<T> to_filter(E&& e)
{
    return expression_filter<T>(to_handle(std::move(e)));
}
} // namespace CMT_ARCH_NAME

/// @brief Converts expression with placeholder to filter. Placeholder and filter must have the same type
template <typename T, typename E>
KFR_INTRINSIC expression_filter<T> to_filter(expression_handle<T, 1>&& e)
{
    return expression_filter<T>(std::move(e));
}

} // namespace kfr
