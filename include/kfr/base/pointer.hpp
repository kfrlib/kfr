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

template <typename T, bool enable_resource = true>
struct expression_pointer;

namespace internal
{

template <typename Expression, typename T, size_t key = 0>
KFR_SINTRIN bool invoke_substitute(Expression& expr, expression_pointer<T>&& new_pointer,
                                   csize_t<key> = csize_t<key>{});

}

template <typename T, size_t N = maximum_expression_width>
struct expression_vtable : expression_vtable<T, N / 2>
{
    using func_get = void (*)(void*, size_t, vec<T, N>&);
    func_get get;

    template <typename Expression>
    expression_vtable(ctype_t<Expression> t)
        : expression_vtable<T, N / 2>(t), get(&expression_vtable<T, N>::template static_get<Expression>)
    {
    }

    template <typename Expression>
    static void static_get(void* instance, size_t index, vec<T, N>& result)
    {
        result = static_cast<Expression*>(instance)->operator()(cinput, index, vec_t<T, N>());
    }
};

template <typename T>
struct expression_vtable<T, 0>
{
    using func_size        = size_t (*)(void* p);
    using func_begin_block = void (*)(void*, size_t);
    using func_end_block   = void (*)(void*, size_t);
    using func_substitute  = bool (*)(void*, expression_pointer<T>&&);

    func_size size;
    func_begin_block begin_block;
    func_end_block end_block;
    func_substitute substitute;

    template <typename Expression>
    expression_vtable(ctype_t<Expression> t)
        : size(&expression_vtable<T, 0>::template static_size<Expression>),
          begin_block(&expression_vtable<T, 0>::template static_begin_block<Expression>),
          end_block(&expression_vtable<T, 0>::template static_end_block<Expression>),
          substitute(&expression_vtable<T, 0>::template static_substitute<Expression>)
    {
    }

    template <typename Expression>
    static size_t static_size(void* instance)
    {
        return static_cast<Expression*>(instance)->size();
    }
    template <typename Expression>
    static void static_begin_block(void* instance, size_t size)
    {
        return static_cast<Expression*>(instance)->begin_block(cinput, size);
    }
    template <typename Expression>
    static void static_end_block(void* instance, size_t size)
    {
        return static_cast<Expression*>(instance)->end_block(cinput, size);
    }
    template <typename Expression>
    static bool static_substitute(void* instance, expression_pointer<T>&& ptr)
    {
        return internal::invoke_substitute(*static_cast<Expression*>(instance), std::move(ptr));
    }
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
KFR_SINTRIN std::shared_ptr<expression_resource> make_resource(E&& e)
{
    using T = expression_resource_impl<decay<E>>;
    return std::static_pointer_cast<expression_resource>(
        std::allocate_shared<T>(allocator<T>(), std::move(e)));
}

template <typename T, bool enable_resource>
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
        static_assert(is_poweroftwo(N), "N must be a power of two");
        vec<T, N> result;
        static_cast<const expression_vtable<T, N>*>(vtable)->get(instance, index, result);
        return result;
    }
    template <size_t N, KFR_ENABLE_IF(N > maximum_expression_width)>
    CMT_INLINE vec<T, N> operator()(cinput_t cinput, size_t index, vec_t<T, N>) const
    {
        return concat(operator()(cinput, index, vec_t<T, N / 2>()), operator()(cinput, index + N / 2,
                                                                               vec_t<T, N / 2>()));
    }
    CMT_INLINE void begin_block(cinput_t, size_t size) const { vtable->begin_block(instance, size); }
    CMT_INLINE void end_block(cinput_t, size_t size) const { vtable->end_block(instance, size); }
    CMT_INLINE size_t size() const { return vtable->size(instance); }

    CMT_INLINE bool substitute(expression_pointer<T>&& new_pointer, csize_t<0> = csize_t<0>{}) const
    {
        return vtable->substitute(instance, std::move(new_pointer));
    }

    explicit operator bool() const { return instance != nullptr; }

private:
    void* instance;
    const expression_vtable<T>* vtable;
    std::shared_ptr<expression_resource> resource;
};

namespace internal
{

template <typename T, typename E>
CMT_INLINE expression_vtable<T>* make_expression_vtable()
{
    static_assert(is_input_expression<E>::value, "E must be an expression");
    static expression_vtable<T> vtable{ ctype_t<decay<E>>{} };
    return &vtable;
}
} // namespace internal

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

template <typename T, size_t key>
class expression_placeholder : public input_expression
{
public:
    using value_type                  = T;
    expression_placeholder() noexcept = default;
    template <typename U, size_t N>
    CMT_INLINE vec<U, N> operator()(cinput_t, size_t index, vec_t<U, N>) const
    {
        return pointer ? cast<U>(pointer(cinput, index, vec_t<T, N>())) : 0;
    }
    expression_pointer<T> pointer;
};

template <typename T, size_t key = 0>
KFR_SINTRIN expression_placeholder<T, key> placeholder(csize_t<key> = csize_t<key>{})
{
    return expression_placeholder<T, key>();
}

template <typename... Args>
KFR_SINTRIN bool substitute(input_expression&, Args&&...)
{
    return false;
}

namespace internal
{
template <typename... Args, typename T, size_t key, size_t... indices>
KFR_SINTRIN bool substitute(internal::expression_base<Args...>& expr, expression_pointer<T>&& new_pointer,
                            csize_t<key>, csizes_t<indices...>);

}

template <typename T, size_t key = 0>
KFR_SINTRIN bool substitute(expression_placeholder<T, key>& expr, expression_pointer<T>&& new_pointer,
                            csize_t<key> = csize_t<key>{})
{
    expr.pointer = std::move(new_pointer);
    return true;
}

template <typename... Args, typename T, size_t key = 0>
KFR_SINTRIN bool substitute(internal::expression_base<Args...>& expr, expression_pointer<T>&& new_pointer,
                            csize_t<key> = csize_t<key>{})
{
    return internal::substitute(expr, std::move(new_pointer), csize_t<key>{},
                                indicesfor_t<Args...>{});
}

template <typename T, size_t key = 0>
KFR_SINTRIN bool substitute(expression_pointer<T>& expr, expression_pointer<T>&& new_pointer,
                            csize_t<key> = csize_t<key>{})
{
    return expr.substitute(std::move(new_pointer), csize_t<key>{});
}

namespace internal
{

KFR_SINTRIN bool var_or() { return false; }

template <typename... Args>
KFR_SINTRIN bool var_or(bool b, Args... args)
{
    return b || var_or(args...);
}

template <typename... Args, typename T, size_t key, size_t... indices>
KFR_SINTRIN bool substitute(internal::expression_base<Args...>& expr, expression_pointer<T>&& new_pointer,
                            csize_t<key>, csizes_t<indices...>)
{
    return var_or(substitute(std::get<indices>(expr.args), std::move(new_pointer), csize_t<key>())...);
}

} // namespace internal

namespace internal
{

template <typename Expression, typename T, size_t key>
KFR_SINTRIN bool invoke_substitute(Expression& expr, expression_pointer<T>&& new_pointer, csize_t<key>)
{
    return kfr::substitute(expr, std::move(new_pointer), csize_t<key>{});
}

} // namespace internal
} // namespace kfr
