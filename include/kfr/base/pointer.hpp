/** @addtogroup expressions
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

#include "basic_expressions.hpp"
#include "vec.hpp"
#include <memory>

namespace kfr
{

constexpr size_t maximum_expression_width = platform<float>::vector_capacity / 4;

constexpr size_t expression_vtable_size = 3 + ilog2(maximum_expression_width) + 1;

template <typename T>
using expression_vtable = std::array<void*, expression_vtable_size>;

struct dummy_content
{
};

struct expression_resource
{
    virtual ~expression_resource() {}
    virtual void* instance() { return nullptr; }
};
template <typename E>
struct alignas(const_max(size_t(8), alignof(E))) expression_resource_impl : expression_resource
{
    expression_resource_impl(E&& e) noexcept : e(std::move(e)) {}
    virtual ~expression_resource_impl() {}
    virtual void* instance() override final { return &e; }
private:
    E e;
};

template <typename E>
std::shared_ptr<expression_resource> make_resource(E&& e)
{
    using T = expression_resource_impl<decay<E>>;
    return std::static_pointer_cast<expression_resource>(
        std::allocate_shared<T>(allocator<T>(), std::move(e)));
}

template <typename T, bool enable_resource = true>
struct expression_pointer : input_expression
{
    using value_type = T;

    expression_pointer() noexcept : instance(nullptr), vtable(nullptr) {}
    expression_pointer(void* instance, const expression_vtable<T>* vtable,
                       std::shared_ptr<expression_resource> resource = nullptr)
        : instance(instance), vtable(vtable), resource(std::move(resource))
    {
    }
    template <size_t N, KFR_ENABLE_IF(N <= maximum_expression_width)>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        using func_t = void (*)(void*, size_t, vec<T, N>&);

        static_assert(is_poweroftwo(N), "N must be a power of two");
        constexpr size_t findex = ilog2(N);
        func_t func             = reinterpret_cast<func_t>((*vtable)[3 + findex]);
        vec<T, N> result;
        func(instance, index, result);
        return result;
    }
    template <size_t N, KFR_ENABLE_IF(N > maximum_expression_width)>
    CMT_INLINE vec<T, N> operator()(cinput_t cinput, size_t index, vec_t<T, N>) const
    {
        return concat(operator()(cinput, index, vec_t<T, N / 2>()),
                      operator()(cinput, index + N / 2, vec_t<T, N / 2>()));
    }
    CMT_INLINE void begin_block(cinput_t, size_t size) const
    {
        using func_t = void (*)(void*, size_t);
        func_t func  = reinterpret_cast<func_t>((*vtable)[0]);
        func(instance, size);
    }
    CMT_INLINE void end_block(cinput_t, size_t size) const
    {
        using func_t = void (*)(void*, size_t);
        func_t func  = reinterpret_cast<func_t>((*vtable)[1]);
        func(instance, size);
    }
    CMT_INLINE size_t size() const
    {
        using func_t = size_t (*)(void*);
        func_t func  = reinterpret_cast<func_t>((*vtable)[2]);
        return func(instance);
    }

private:
    void* instance;
    const expression_vtable<T>* vtable;
    std::shared_ptr<expression_resource> resource;
};

template <typename T>
struct expression_pointer<T, false> : input_expression
{
    using value_type = T;

    expression_pointer() noexcept : instance(nullptr), vtable(nullptr) {}
    expression_pointer(void* instance, const expression_vtable<T>* vtable)
        : instance(instance), vtable(vtable)
    {
    }
    template <size_t N, KFR_ENABLE_IF(N <= maximum_expression_width)>
    CMT_INLINE vec<T, N> operator()(cinput_t, size_t index, vec_t<T, N>) const
    {
        using func_t = void (*)(void*, size_t, vec<T, N>&);

        static_assert(is_poweroftwo(N), "N must be a power of two");
        constexpr size_t findex = ilog2(N);
        func_t func             = reinterpret_cast<func_t>((*vtable)[3 + findex]);
        vec<T, N> result;
        func(instance, index, result);
        return result;
    }
    template <size_t N, KFR_ENABLE_IF(N > maximum_expression_width)>
    CMT_INLINE vec<T, N> operator()(cinput_t input, size_t index, vec_t<T, N>) const
    {
        return concat(operator()(cinput, index, vec_t<T, N / 2>()),
                      operator()(cinput, index + N / 2, vec_t<T, N / 2>()));
    }
    CMT_INLINE void begin_block(cinput_t, size_t size) const
    {
        using func_t = void (*)(void*, size_t);
        func_t func  = reinterpret_cast<func_t>((*vtable)[0]);
        func(instance, size);
    }
    CMT_INLINE void end_block(cinput_t, size_t size) const
    {
        using func_t = void (*)(void*, size_t);
        func_t func  = reinterpret_cast<func_t>((*vtable)[1]);
        func(instance, size);
    }
    CMT_INLINE size_t size() const
    {
        using func_t = size_t (*)(void*);
        func_t func  = reinterpret_cast<func_t>((*vtable)[2]);
        return func(instance);
    }

private:
    void* instance;
    const expression_vtable<T>* vtable;
};

namespace internal
{
template <typename T, size_t N, typename Fn, typename Ret = vec<T, N>,
          typename NonMemFn = void (*)(void*, size_t, Ret&)>
CMT_INLINE NonMemFn make_expression_func()
{
    return [](void* fn, size_t index, Ret& result) {
        result = (reinterpret_cast<Fn*>(fn)->operator()(cinput, index, vec_t<T, N>()));
    };
}

template <typename Fn, typename NonMemFn = void (*)(void*, size_t)>
CMT_INLINE NonMemFn make_expression_begin_block()
{
    return [](void* fn, size_t size) { reinterpret_cast<Fn*>(fn)->begin_block(cinput, size); };
}
template <typename Fn, typename NonMemFn = void (*)(void*, size_t)>
CMT_INLINE NonMemFn make_expression_end_block()
{
    return [](void* fn, size_t size) { reinterpret_cast<Fn*>(fn)->end_block(cinput, size); };
}
template <typename Fn, typename NonMemFn = size_t (*)(void*)>
CMT_INLINE NonMemFn make_expression_size()
{
    return [](void* fn) -> size_t { return reinterpret_cast<Fn*>(fn)->size(); };
}

template <typename T, typename E>
expression_vtable<T> make_expression_vtable_impl()
{
    expression_vtable<T> result;

    result[0] = reinterpret_cast<void*>(internal::make_expression_begin_block<decay<E>>());
    result[1] = reinterpret_cast<void*>(internal::make_expression_end_block<decay<E>>());
    result[2] = reinterpret_cast<void*>(internal::make_expression_size<decay<E>>());

    cforeach(csizeseq_t<expression_vtable_size - 3>(), [&](auto u) {
        constexpr size_t N = 1 << val_of(decltype(u)());
        result[3 + val_of(decltype(u)())] =
            reinterpret_cast<void*>(internal::make_expression_func<T, N, decay<E>>());
    });
    return result;
}

template <typename T, typename E>
CMT_INLINE expression_vtable<T>* make_expression_vtable()
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    static expression_vtable<T> vtable = internal::make_expression_vtable_impl<T, E>();
    return &vtable;
}
}

/** @brief Converts the given expression into an opaque object.
 *  This overload takes reference to the expression.
 *  @warning Use with caution with local variables.
 */
template <typename E, typename T = value_type_of<E>>
CMT_INLINE expression_pointer<T> to_pointer(E& expr)
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    return expression_pointer<T>(std::addressof(expr), internal::make_expression_vtable<T, E>());
}

/** @brief Converts the given expression into an opaque object.
 *  This overload takes ownership of the expression (Move semantics).
 *  @note Use std::move to force use of this overload.
 */
template <typename E, typename T = value_type_of<E>>
CMT_INLINE expression_pointer<T> to_pointer(E&& expr)
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    std::shared_ptr<expression_resource> ptr = make_resource(std::move(expr));
    return expression_pointer<T>(ptr->instance(), internal::make_expression_vtable<T, E>(), std::move(ptr));
}
}
