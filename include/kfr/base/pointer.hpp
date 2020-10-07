/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016 D Levin (https://www.kfrlib.com)
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

#include "../simd/vec.hpp"
#include "basic_expressions.hpp"
#include <memory>

namespace kfr
{

template <typename T, bool enable_resource = true>
struct expression_pointer;

template <typename T>
constexpr size_t maximum_expression_width = vector_width_for<T, cpu_t::highest> * 2;

inline namespace CMT_ARCH_NAME
{

namespace internal
{

template <typename Expression, typename T, size_t key = 0>
KFR_INTRINSIC bool invoke_substitute(Expression& expr, expression_pointer<T>&& new_pointer,
                                     csize_t<key> = {});
}
} // namespace CMT_ARCH_NAME

template <typename T, size_t N = maximum_expression_width<T>>
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
        result = get_elements(*static_cast<Expression*>(instance), cinput, index, vec_shape<T, N>());
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
    expression_vtable(ctype_t<Expression>)
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
struct expression_resource_impl : expression_resource
{
    expression_resource_impl(E&& e) CMT_NOEXCEPT : e(std::move(e)) {}
    virtual ~expression_resource_impl() {}
    virtual void* instance() override final { return &e; }

public:
#ifdef __cpp_aligned_new
    static void operator delete(void* p, std::align_val_t al) noexcept { details::aligned_release(p); }
#endif

private:
    E e;
};

template <typename E>
KFR_INTRINSIC std::shared_ptr<expression_resource> make_resource(E&& e)
{
    using T = expression_resource_impl<decay<E>>;
    return std::static_pointer_cast<expression_resource>(std::shared_ptr<T>(
        new (aligned_allocate<T>()) T(std::move(e)), [](T* pi) { aligned_deallocate<T>(pi); }));
}

template <typename T, bool enable_resource>
struct expression_pointer : input_expression
{
    using value_type = T;

    expression_pointer() CMT_NOEXCEPT : instance(nullptr), vtable(nullptr) {}
    expression_pointer(const void* instance, const expression_vtable<T>* vtable,
                       std::shared_ptr<expression_resource> resource = nullptr)
        : instance(const_cast<void*>(instance)), vtable(vtable), resource(std::move(resource))
    {
    }
    template <size_t N, KFR_ENABLE_IF(N <= maximum_expression_width<T>)>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_pointer& self, cinput_t, size_t index,
                                                vec_shape<T, N>)
    {
        static_assert(is_poweroftwo(N), "N must be a power of two");
        vec<T, N> result;
        static_cast<const expression_vtable<T, N>*>(self.vtable)->get(self.instance, index, result);
        return result;
    }
    template <size_t N, KFR_ENABLE_IF(N > maximum_expression_width<T>)>
    friend KFR_INTRINSIC vec<T, N> get_elements(const expression_pointer& self, cinput_t cinput, size_t index,
                                                vec_shape<T, N>)
    {
        static_assert(is_poweroftwo(N), "N must be a power of two");
        const vec<T, N / 2> r1 = get_elements(self, cinput, index, vec_shape<T, N / 2>());
        const vec<T, N / 2> r2 = get_elements(self, cinput, index + N / 2, vec_shape<T, N / 2>());
        return concat(r1, r2);
    }
    KFR_MEM_INTRINSIC void begin_block(cinput_t, size_t size) const { vtable->begin_block(instance, size); }
    KFR_MEM_INTRINSIC void end_block(cinput_t, size_t size) const { vtable->end_block(instance, size); }
    KFR_MEM_INTRINSIC size_t size() const { return vtable->size(instance); }

    KFR_MEM_INTRINSIC bool substitute(expression_pointer<T>&& new_pointer, csize_t<0> = csize_t<0>{}) const
    {
        return vtable->substitute(instance, std::move(new_pointer));
    }

    explicit operator bool() const { return instance != nullptr; }

private:
    void* instance;
    const expression_vtable<T>* vtable;
    std::shared_ptr<expression_resource> resource;
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{

template <typename T, typename E>
KFR_INTRINSIC expression_vtable<T>* make_expression_vtable()
{
    static_assert(is_input_expression<E>, "E must be an expression");
    static expression_vtable<T> vtable{ ctype_t<decay<E>>{} };
    return &vtable;
}
} // namespace internal

/** @brief Converts the given expression into an opaque object.
 *  This overload takes reference to the expression.
 *  @warning Use with caution with local variables.
 */
template <typename E, typename T = value_type_of<E>>
KFR_INTRINSIC expression_pointer<T> to_pointer(E& expr)
{
    static_assert(is_input_expression<E>, "E must be an expression");
    return expression_pointer<T>(std::addressof(expr), internal::make_expression_vtable<T, E>());
}

/** @brief Converts the given expression into an opaque object.
 *  This overload takes ownership of the expression (Move semantics).
 *  @note Use std::move to force use of this overload.
 */
template <typename E, typename T = value_type_of<E>>
KFR_INTRINSIC expression_pointer<T> to_pointer(E&& expr)
{
    static_assert(is_input_expression<E>, "E must be an expression");
    std::shared_ptr<expression_resource> ptr = make_resource(std::move(expr));
    void* instance                           = ptr->instance();
    return expression_pointer<T>(instance, internal::make_expression_vtable<T, E>(), std::move(ptr));
}

template <typename T, size_t key>
class expression_placeholder : public input_expression
{
public:
    using value_type                      = T;
    expression_placeholder() CMT_NOEXCEPT = default;
    template <typename U, size_t N>
    friend KFR_INTRINSIC vec<U, N> get_elements(const expression_placeholder& self, cinput_t, size_t index,
                                                vec_shape<U, N>)
    {
        return self.pointer ? elemcast<U>(get_elements(self.pointer, cinput, index, vec_shape<T, N>())) : 0;
    }
    expression_pointer<T> pointer;
};

template <typename T, size_t key = 0>
KFR_INTRINSIC expression_placeholder<T, key> placeholder(csize_t<key> = csize_t<key>{})
{
    return expression_placeholder<T, key>();
}

template <typename... Args>
KFR_INTRINSIC bool substitute(input_expression&, Args&&...)
{
    return false;
}

namespace internal
{
template <typename... Args, typename T, size_t key, size_t... indices>
KFR_INTRINSIC bool substitute(internal::expression_with_arguments<Args...>& expr,
                              expression_pointer<T>&& new_pointer, csize_t<key>, csizes_t<indices...>);
}

template <typename T, size_t key = 0>
KFR_INTRINSIC bool substitute(expression_placeholder<T, key>& expr, expression_pointer<T>&& new_pointer,
                              csize_t<key> = csize_t<key>{})
{
    expr.pointer = std::move(new_pointer);
    return true;
}

template <typename... Args, typename T, size_t key = 0>
KFR_INTRINSIC bool substitute(internal::expression_with_arguments<Args...>& expr,
                              expression_pointer<T>&& new_pointer, csize_t<key> = csize_t<key>{})
{
    return internal::substitute(expr, std::move(new_pointer), csize_t<key>{}, indicesfor_t<Args...>{});
}

template <typename T, size_t key = 0>
KFR_INTRINSIC bool substitute(expression_pointer<T>& expr, expression_pointer<T>&& new_pointer,
                              csize_t<key> = csize_t<key>{})
{
    return expr.substitute(std::move(new_pointer), csize_t<key>{});
}

namespace internal
{

template <typename... Args, typename T, size_t key, size_t... indices>
KFR_INTRINSIC bool substitute(internal::expression_with_arguments<Args...>& expr,
                              expression_pointer<T>&& new_pointer, csize_t<key>, csizes_t<indices...>)
{
    return (substitute(std::get<indices>(expr.args), std::move(new_pointer), csize_t<key>()) || ...);
}

} // namespace internal

namespace internal
{

template <typename Expression, typename T, size_t key>
KFR_INTRINSIC bool invoke_substitute(Expression& expr, expression_pointer<T>&& new_pointer, csize_t<key>)
{
    return kfr::substitute(expr, std::move(new_pointer), csize_t<key>{});
}

} // namespace internal
} // namespace CMT_ARCH_NAME
} // namespace kfr
