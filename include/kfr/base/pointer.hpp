/** @addtogroup expressions
 *  @{
 */
/*
  Copyright (C) 2016-2022 Fractalium Ltd (https://www.kfrlib.com)
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

template <typename T, template <T...> typename Tpl, typename Pack>
struct expand_cvals;

template <typename T, template <T...> typename Tpl, T... vals>
struct expand_cvals<T, Tpl, cvals_t<T, vals...>>
{
    using type = Tpl<vals...>;
};

inline namespace CMT_ARCH_NAME
{

namespace internal
{

template <typename Expression, typename T, size_t key = 0>
KFR_INTRINSIC bool invoke_substitute(Expression& expr, expression_pointer<T>&& new_pointer,
                                     csize_t<key> = {});
}
} // namespace CMT_ARCH_NAME

template <typename T, index_t Dims>
struct expression_vtable
{
    constexpr static const size_t Nsizes = 1 + ilog2(maximum_expression_width<T>);

    using func_get        = void (*)(void*, shape<Dims>, T*);
    using func_set        = void (*)(void*, shape<Dims>, const T*);
    using func_shapeof    = void (*)(void*, shape<Dims>&);
    using func_substitute = bool (*)(void*, expression_pointer<T>&&);

    func_shapeof fn_shapeof;
    func_substitute fn_substitute;
    std::array<std::array<func_get, Nsizes>, Dims> fn_get_elements;
    std::array<std::array<func_set, Nsizes>, Dims> fn_set_elements;

    template <typename Expression>
    expression_vtable(ctype_t<Expression> t)
    {
        fn_shapeof    = &static_shapeof<Expression>;
        fn_substitute = &static_substitute<Expression>;
        cforeach(csizeseq<Nsizes>,
                 [&](size_t size) CMT_INLINE_LAMBDA
                 {
                     cforeach(csizeseq<Dims>,
                              [&](size_t axis) CMT_INLINE_LAMBDA
                              {
                                  fn_get_elements[axis][size] =
                                      &expression_vtable::static_get_elements<Expression, 1 << size, axis>;
                                  fn_set_elements[axis][size] =
                                      &expression_vtable::static_set_elements<Expression, 1 << size, axis>;
                              });
                 });
    }

    template <typename Expression, size_t N, index_t VecAxis>
    static void static_get_elements(void* instance, shape<Dims> index, T* dest)
    {
        write(dest, get_elements(*static_cast<Expression*>(instance), index, axis_params_v<VecAxis, N>));
    }
    template <typename Expression, size_t N, index_t VecAxis>
    static void static_set_elements(void* instance, shape<Dims> index, const T* src)
    {
        set_elements(*static_cast<Expression*>(instance), index, axis_params_v<VecAxis, N>, read<N>(src));
    }
    template <typename Expression>
    static void static_shapeof(void* instance, shape<Dims>& result)
    {
        result = expression_traits<Expression>::shapeof(*static_cast<Expression*>(instance));
    }
    template <typename Expression>
    static bool static_substitute(void* instance, expression_pointer<T> ptr)
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

template <typename T, index_t Dims>
struct xpointer
{
    void* instance;
    const expression_vtable<T, Dims>* vtable;
    std::shared_ptr<expression_resource> resource;

    xpointer() CMT_NOEXCEPT : instance(nullptr), vtable(nullptr) {}
    xpointer(const void* instance, const expression_vtable<T, Dims>* vtable,
             std::shared_ptr<expression_resource> resource = nullptr)
        : instance(const_cast<void*>(instance)), vtable(vtable), resource(std::move(resource))
    {
    }

    explicit operator bool() const { return instance != nullptr; }
};

template <typename T, index_t Dims>
struct expression_traits<xpointer<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> shapeof(const xpointer<T, Dims>& self)
    {
        shape<dims> result;
        self.vtable->fn_shapeof(self.instance, result);
    }
    constexpr static shape<dims> shapeof() { return shape<dims>(undefined_size); }

    constexpr static inline bool random_access = false;
};

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const xpointer<T, NDims>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    static_assert(is_poweroftwo(N) && N >= 1);
    if constexpr (N > expression_vtable<T, NDims>::Nmax)
    {
        constexpr size_t Nhalf = N / 2;
        return concat(get_elements(self, index, axis_params_v<Axis, Nhalf>),
                      get_elements(self, index.add_at(Axis, Nhalf), axis_params_v<Axis, Nhalf>));
    }
    else
    {
        portable_vec<T, N> result;
        self.vtable->fn_get_elements[Axis][ilog2(N)](self.instance, index, result.elem);
        return result;
    }
}

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const xpointer<T, NDims>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    static_assert(is_poweroftwo(N) && N >= 1);
    if constexpr (N > expression_vtable<T, NDims>::Nmax)
    {
        constexpr size_t Nhalf = N / 2;
        set_elements(self, index, axis_params_v<Axis, Nhalf>, slice<0, Nhalf>(value));
        set_elements(self, index.add_at(Axis, Nhalf), axis_params_v<Axis, Nhalf>, slice<Nhalf, Nhalf>(value));
    }
    else
    {
        self.vtable->fn_set_elements[Axis][ilog2(N)](self.instance, index, &value.front());
    }
}
} // namespace CMT_ARCH_NAME

inline namespace CMT_ARCH_NAME
{

namespace internal
{

template <typename T, index_t Dims, typename E>
KFR_INTRINSIC expression_vtable<T, Dims>* make_expression_vtable()
{
    static expression_vtable<T, Dims> vtable{ ctype_t<decay<E>>{} };
    return &vtable;
}
} // namespace internal

/** @brief Converts the given expression into an opaque object.
 *  This overload takes reference to the expression.
 *  @warning Use with caution with local variables.
 */
template <typename E, typename T = expression_value_type<E>, index_t Dims = expression_dims<E>>
KFR_INTRINSIC expression_pointer<T, Dims> to_pointer(E& expr)
{
    return expression_pointer<T>(std::addressof(expr), internal::make_expression_vtable<T, Dims, E>());
}

/** @brief Converts the given expression into an opaque object.
 *  This overload takes ownership of the expression (Move semantics).
 *  @note Use std::move to force use of this overload.
 */
template <typename E, typename T = expression_value_type<E>, index_t Dims = expression_dims<E>>
KFR_INTRINSIC expression_pointer<T, Dims> to_pointer(E&& expr)
{
    std::shared_ptr<expression_resource> ptr = make_resource(std::move(expr));
    void* instance                           = ptr->instance();
    return expression_pointer<T, Dims>(instance, internal::make_expression_vtable<T, E>(), std::move(ptr));
}

#if 0
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
#endif
} // namespace CMT_ARCH_NAME

}
