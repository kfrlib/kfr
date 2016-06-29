/**
 * Copyright (C) 2016 D Levin (http://www.kfrlib.com)
 * This file is part of KFR
 *
 * KFR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KFR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KFR.
 *
 * If GPL is not suitable for your project, you must purchase a commercial license to use KFR.
 * Buying a commercial license is mandatory as soon as you develop commercial activities without
 * disclosing the source code of your own applications.
 * See http://www.kfrlib.com for details.
 */
#pragma once

#include "../base/vec.hpp"
#include "basic.hpp"
#include <memory>

namespace kfr
{

constexpr size_t maximum_expression_width() { return bitness_const(16, 32); }

template <typename T, size_t maxwidth = maximum_expression_width()>
using expression_vtable = carray<void*, 2 + ilog2(maxwidth) + 1>;

struct dummy_content
{
};

struct expression_resource
{
    virtual ~expression_resource() {}
    virtual void* instance() { return nullptr; }
};
template <typename E>
struct expression_resource_impl : expression_resource
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
    return std::static_pointer_cast<expression_resource>(
        std::make_shared<expression_resource_impl<decay<E>>>(std::move(e)));
}

template <typename T, size_t maxwidth = maximum_expression_width()>
struct expression_pointer : input_expression
{
    using value_type = T;

    static_assert(is_poweroftwo(maxwidth), "N must be a power of two");
    expression_pointer() noexcept : instance(nullptr), vtable(nullptr) {}
    expression_pointer(void* instance, const expression_vtable<T, maxwidth>* vtable,
                       std::shared_ptr<expression_resource> resource = nullptr)
        : instance(instance), vtable(vtable), resource(std::move(resource))
    {
    }
    template <typename U, size_t N>
    KFR_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N>) const
    {
        using func_t = simd<T, N> (*)(void*, size_t);

        static_assert(is_poweroftwo(N), "N must be a power of two");
        constexpr size_t findex = ilog2(N);
        static_assert(N <= maxwidth, "N is greater than maxwidth");
        func_t func = reinterpret_cast<func_t>(vtable->get(csize<2 + findex>));
        vec<U, N> result = cast<U>(func(instance, index));
        return result;
    }
    KFR_INLINE void begin_block(size_t size) const
    {
        using func_t = void (*)(void*, size_t);
        func_t func  = reinterpret_cast<func_t>(vtable->get(csize<0>));
        func(instance, size);
    }
    KFR_INLINE void end_block(size_t size) const
    {
        using func_t = void (*)(void*, size_t);
        func_t func  = reinterpret_cast<func_t>(vtable->get(csize<1>));
        func(instance, size);
    }

private:
    void* instance;
    const expression_vtable<T, maxwidth>* vtable;
    std::shared_ptr<expression_resource> resource;
};

namespace internal
{
template <typename T, size_t N, typename Fn, typename Ret = simd<T, N>,
          typename NonMemFn = Ret (*)(Fn*, size_t, vec_t<T, N>)>
KFR_INLINE NonMemFn make_expression_func()
{
    return [](Fn* fn, size_t index, vec_t<T, N> x) { return *(fn->operator()(cinput, index, x)); };
}

template <typename Fn, typename NonMemFn = void (*)(Fn*, size_t)>
KFR_INLINE NonMemFn make_expression_begin_block()
{
    return [](Fn* fn, size_t size) { return fn->begin_block(size); };
}
template <typename Fn, typename NonMemFn = void (*)(Fn*, size_t)>
KFR_INLINE NonMemFn make_expression_end_block()
{
    return [](Fn* fn, size_t size) { return fn->end_block(size); };
}

template <typename T, size_t maxwidth, typename E>
expression_vtable<T, maxwidth> make_expression_vtable_impl()
{
    expression_vtable<T, maxwidth> result;
    constexpr size_t size = result.size() - 2;

    result.get(csize<0>) = reinterpret_cast<void*>(&internal::make_expression_begin_block<decay<E>>);
    result.get(csize<1>) = reinterpret_cast<void*>(&internal::make_expression_end_block<decay<E>>);

    cforeach(csizeseq<size>, [&](auto u) {
        constexpr size_t N = 1 << val_of(u);
        result.get(csize<2 + val_of(u)>) =
            reinterpret_cast<void*>(internal::make_expression_func<T, N, decay<E>>());
    });
    return result;
}

template <typename T, size_t maxwidth, typename E>
KFR_INLINE expression_vtable<T, maxwidth>* make_expression_vtable()
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    static expression_vtable<T, maxwidth> vtable = internal::make_expression_vtable_impl<T, maxwidth, E>();
    return &vtable;
}
}

template <typename E, typename T = value_type_of<E>, size_t maxwidth = maximum_expression_width()>
KFR_INLINE expression_pointer<T, maxwidth> to_pointer(E& expr)
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    return expression_pointer<T, maxwidth>(std::addressof(expr),
                                           internal::make_expression_vtable<T, maxwidth, E>());
}

template <typename E, typename T = value_type_of<E>, size_t maxwidth = maximum_expression_width()>
KFR_INLINE expression_pointer<T, maxwidth> to_pointer(E&& expr)
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    std::shared_ptr<expression_resource> ptr = make_resource(std::move(expr));
    return expression_pointer<T, maxwidth>(
        ptr->instance(), internal::make_expression_vtable<T, maxwidth, E>(), std::move(ptr));
}
}
