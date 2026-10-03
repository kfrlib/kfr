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

#include "basic_expressions.hpp"
#include "expression.hpp"
#include "handle.hpp"
#include "univector.hpp"

namespace kfr
{

/**
 * @brief Abstract base class for single-argument filters, mainly intended for
 * DSP processing.
 *
 * Concrete filters implement the protected `process_buffer` and
 * `process_expression` hooks; the public `apply` overloads dispatch to them
 * for several common container and expression source types.
 */
template <typename T>
class filter
{
public:
    /** @brief Destructor. */
    virtual ~filter() {}

    /**
     * @brief Resets the internal state (such as a delay line).
     *
     * The default implementation does nothing; subclasses that keep state
     * between calls should override this.
     */
    virtual void reset() {}

    /**
     * @brief Applies the filter in-place to a fixed-size C array.
     * @param buffer Array to filter; the result is written back into it.
     */
    template <size_t Size>
    void apply(T (&buffer)[Size])
    {
        process_buffer(buffer, buffer, Size);
    }

    /**
     * @brief Applies the filter to a fixed-size C array and writes the result
     * to another array.
     * @param dest Destination array.
     * @param src  Source array.
     */
    template <size_t Size>
    void apply(T (&dest)[Size], T (&src)[Size])
    {
        process_buffer(dest, src, Size);
    }

    /**
     * @brief Applies the filter in-place to a univector.
     * @param buffer Vector to filter; the result is written back into it.
     */
    template <univector_tag Tag>
    void apply(univector<T, Tag>& buffer)
    {
        process_buffer(buffer.data(), buffer.data(), buffer.size());
    }

    /**
     * @brief Applies the filter to a univector and writes the result to another
     * univector.
     * @param dest Destination vector. Resized to match @p src when empty.
     * @param src  Source vector.
     */
    template <univector_tag Tag1, univector_tag Tag2>
    void apply(univector<T, Tag1>& dest, const univector<T, Tag2>& src)
    {
        if (dest.empty())
            dest.resize(src.size());
        process_buffer(dest.data(), src.data(), std::min(dest.size(), src.size()));
    }

    /**
     * @brief Applies the filter in-place to a raw buffer.
     * @param buffer Pointer to the sample buffer.
     * @param size   Number of samples.
     */
    void apply(T* buffer, size_t size) { process_buffer(buffer, buffer, size); }

    /**
     * @brief Applies the filter to a raw buffer and writes the result to another
     * buffer.
     * @param dest Destination buffer.
     * @param src  Source buffer.
     * @param size Number of samples to process.
     */
    void apply(T* dest, const T* src, size_t size) { process_buffer(dest, src, size); }

    /**
     * @brief Feeds zero-valued samples to the filter and writes its tail to a fixed-size C array.
     * @param output Destination array for the tail samples.
     */
    template <size_t Size>
    void apply_zeros(T (&output)[Size])
    {
        apply_zeros(output, Size);
    }

    /**
     * @brief Feeds zero-valued samples to the filter and writes its tail to a univector.
     * @param output Destination vector for the tail samples.
     */
    template <univector_tag Tag>
    void apply_zeros(univector<T, Tag>& output)
    {
        apply_zeros(output.data(), output.size());
    }

    /**
     * @brief Feeds @p size zero-valued samples to the filter and writes the resulting tail.
     *
     * The input zeros are processed in bounded chunks, preserving the filter state between chunks.
     * @param output Destination buffer for the tail samples.
     * @param size   Number of tail samples to generate.
     */
    void apply_zeros(T* output, size_t size)
    {
        constexpr size_t zero_input_size = 1024 / sizeof(T);
        const T zero_input[zero_input_size]{};
        while (size > 0)
        {
            const size_t chunk_size = std::min(size, zero_input_size);
            process_buffer(output, zero_input, chunk_size);
            output += chunk_size;
            size -= chunk_size;
        }
    }

    /**
     * @brief Evaluates @p src into @p dest through the filter.
     * @param dest Destination univector.
     * @param src  Source expression handle.
     */
    template <univector_tag Tag>
    void apply(univector<T, Tag>& dest, const expression_handle<T, 1>& src)
    {
        process_expression(dest.data(), src, size_min(dest.size(), src.size()));
    }

    /**
     * @brief Evaluates @p src into a raw destination buffer through the filter.
     * @param dest Destination buffer.
     * @param src  Source expression handle.
     * @param size Number of samples to process.
     */
    void apply(T* dest, const expression_handle<T, 1>& src, size_t size)
    {
        process_expression(dest, src, size_min(size, src.size()));
    }

    /**
     * @brief Evaluates the 1-dimensional expression @p src into @p dest through
     * the filter.
     * @param dest Destination univector.
     * @param src  Source expression.
     */
    template <univector_tag Tag, input_expression Expr>
    void apply(univector<T, Tag>& dest, const Expr& src)
    {
        static_assert(expression_dims<Expr> == 1);
        process_expression(dest.data(), to_handle(src), size_min(dest.size(), get_shape(src).front()));
    }

    /**
     * @brief Evaluates the 1-dimensional expression @p src into a raw destination
     * buffer through the filter.
     * @param dest Destination buffer.
     * @param src  Source expression.
     * @param size Number of samples to process.
     */
    template <input_expression Expr>
    void apply(T* dest, const Expr& src, size_t size)
    {
        process_expression(dest, to_handle(src), size_min(size, src.size()));
    }

protected:
    /**
     * @brief Filters @p size samples from @p src into @p dest.
     * @param dest Destination buffer.
     * @param src  Source buffer.
     * @param size Number of samples to process.
     */
    virtual void process_buffer(T* dest, const T* src, size_t size) = 0;
    /**
     * @brief Evaluates @p size samples of the expression @p src into @p dest.
     * @param dest Destination buffer.
     * @param src  Source expression handle.
     * @param size Number of samples to process.
     */
    virtual void process_expression(T* dest, const expression_handle<T, 1>& src, size_t size) = 0;
};

/**
 * @brief Filter adapter that wraps an expression containing a placeholder.
 *
 * The wrapped expression is evaluated against the input on every call by
 * substituting the input (as a univector or expression handle) for the
 * placeholder and invoking `process`.
 */
template <typename T>
class expression_filter : public filter<T>
{
public:
    /**
     * @brief Constructs the filter from a placeholder expression handle.
     * @param filter_expr Expression containing a placeholder to be substituted
     *                    with the input on each call.
     */
    explicit expression_filter(expression_handle<T, 1> filter_expr) : filter_expr(std::move(filter_expr)) {}

    /** @brief Resets the state of the wrapped expression. */
    void reset() override
    {
        using kfr::reset;
        reset(filter_expr);
    }

protected:
    /** @brief Default-constructing constructor for subclasses. */
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

    /** @brief The wrapped placeholder expression. */
    expression_handle<T, 1> filter_expr;
};

inline namespace KFR_ARCH_NAME
{

/**
 * @brief Creates a filter from an expression containing a placeholder.
 *
 * The placeholder and the resulting filter share the same value type.
 * @param e Expression with a placeholder.
 * @return An `expression_filter` wrapping @p e.
 */
template <typename E, typename T = expression_value_type<E>>
KFR_INTRINSIC expression_filter<T> to_filter(E&& e)
{
    return expression_filter<T>(to_handle(std::move(e)));
}
} // namespace KFR_ARCH_NAME

/**
 * @brief Creates a filter from an expression handle containing a placeholder.
 *
 * The placeholder and the resulting filter share the same value type.
 * @param e Expression handle with a placeholder.
 * @return An `expression_filter` wrapping @p e.
 */
template <typename T, typename E>
KFR_INTRINSIC expression_filter<T> to_filter(expression_handle<T, 1>&& e)
{
    return expression_filter<T>(std::move(e));
}

} // namespace kfr
