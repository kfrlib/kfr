/** @addtogroup base
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

#include "../cometa/memory.hpp"
#include "../simd/vec.hpp"
#include "basic_expressions.hpp"
#include <memory>

namespace kfr
{

template <typename T, index_t Dims>
struct expression_handle;

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

template <typename Expression, typename T, index_t Dims, size_t key = 0>
KFR_INTRINSIC bool invoke_substitute(Expression& expr, expression_handle<T, Dims> new_handle,
                                     csize_t<key> = {});
}
} // namespace CMT_ARCH_NAME

template <typename T, index_t Dims>
struct expression_vtable
{
    constexpr static const size_t Nsizes = 1 + ilog2(maximum_expression_width<T>);
    constexpr static const size_t Nmax   = 1 << Nsizes;

    using func_get        = void (*)(void*, shape<Dims>, T*);
    using func_set        = void (*)(void*, shape<Dims>, const T*);
    using func_shapeof    = void (*)(void*, shape<Dims>&);
    using func_substitute = bool (*)(void*, expression_handle<T, Dims>);
    using func_pass       = void (*)(void*, shape<Dims>, shape<Dims>);

    func_shapeof fn_shapeof;
    func_substitute fn_substitute;
    func_pass fn_begin_pass;
    func_pass fn_end_pass;
    std::array<std::array<func_get, Nsizes>, Dims> fn_get_elements;
    std::array<std::array<func_set, Nsizes>, Dims> fn_set_elements;

    template <typename Expression>
    KFR_MEM_INTRINSIC expression_vtable(ctype_t<Expression> t)
    {
        fn_shapeof    = &static_shapeof<Expression>;
        fn_substitute = &static_substitute<Expression>;
        fn_begin_pass = &static_begin_pass<Expression>;
        fn_end_pass   = &static_end_pass<Expression>;
        cforeach(csizeseq<Nsizes>,
                 [&](auto size_) CMT_INLINE_LAMBDA
                 {
                     cforeach(csizeseq<Dims>,
                              [&](auto axis_) CMT_INLINE_LAMBDA
                              {
                                  constexpr size_t size = decltype(size_)::value;
                                  constexpr size_t axis = decltype(axis_)::value;
                                  fn_get_elements[axis][size] =
                                      &static_get_elements<Expression, 1 << size, axis>;
                                  fn_set_elements[axis][size] =
                                      &static_set_elements<Expression, 1 << size, axis>;
                              });
                 });
    }

    template <typename Expression, size_t N, index_t VecAxis>
    static void static_get_elements(void* instance, shape<Dims> index, T* dest)
    {
        if constexpr (is_input_expression<Expression>)
        {
            write(dest, get_elements(*static_cast<Expression*>(instance), index, axis_params_v<VecAxis, N>));
        }
        else
        {
        }
    }
    template <typename Expression, size_t N, index_t VecAxis>
    static void static_set_elements(void* instance, shape<Dims> index, const T* src)
    {
        if constexpr (is_output_expression<Expression>)
        {
            set_elements(*static_cast<Expression*>(instance), index, axis_params_v<VecAxis, N>, read<N>(src));
        }
        else
        {
        }
    }
    template <typename Expression>
    static void static_shapeof(void* instance, shape<Dims>& result)
    {
        result = expression_traits<Expression>::get_shape(*static_cast<Expression*>(instance));
    }
    template <typename Expression>
    static bool static_substitute(void* instance, expression_handle<T, Dims> ptr)
    {
        return internal::invoke_substitute(*static_cast<Expression*>(instance), std::move(ptr));
    }
    template <typename Expression>
    static void static_begin_pass(void* instance, shape<Dims> start, shape<Dims> stop)
    {
        begin_pass(*static_cast<Expression*>(instance), start, stop);
    }
    template <typename Expression>
    static void static_end_pass(void* instance, shape<Dims> start, shape<Dims> stop)
    {
        end_pass(*static_cast<Expression*>(instance), start, stop);
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
    KFR_INTRINSIC virtual void* instance() override final { return &e; }

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
    using T = expression_resource_impl<std::decay_t<E>>;
    return std::static_pointer_cast<expression_resource>(std::shared_ptr<T>(
        new (aligned_allocate<T>()) T(std::move(e)), [](T* pi) { aligned_deallocate<T>(pi); }));
}

template <typename T, index_t Dims = 1>
struct expression_handle
{
    void* instance;
    const expression_vtable<T, Dims>* vtable;
    std::shared_ptr<expression_resource> resource;

    expression_handle() CMT_NOEXCEPT : instance(nullptr), vtable(nullptr) {}
    expression_handle(const void* instance, const expression_vtable<T, Dims>* vtable,
                      std::shared_ptr<expression_resource> resource = nullptr)
        : instance(const_cast<void*>(instance)), vtable(vtable), resource(std::move(resource))
    {
    }

    explicit operator bool() const { return instance != nullptr; }

    bool substitute(expression_handle<T, Dims> new_handle)
    {
        return vtable->fn_substitute(instance, std::move(new_handle));
    }
};

template <typename T, index_t Dims>
struct expression_traits<expression_handle<T, Dims>> : expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_handle<T, Dims>& self)
    {
        shape<dims> result;
        self.vtable->fn_shapeof(self.instance, result);
        return result;
    }
    constexpr static shape<dims> get_shape() { return shape<dims>(undefined_size); }

    constexpr static inline bool random_access = false;
};

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t NDims>
KFR_INTRINSIC void begin_pass(const expression_handle<T, NDims>& self, shape<NDims> start, shape<NDims> stop)
{
    self.vtable->fn_begin_pass(self.instance, start, stop);
}
template <typename T, index_t NDims>
KFR_INTRINSIC void end_pass(const expression_handle<T, NDims>& self, shape<NDims> start, shape<NDims> stop)
{
    self.vtable->fn_end_pass(self.instance, start, stop);
}

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_handle<T, NDims>& self, const shape<NDims>& index,
                                     const axis_params<Axis, N>& sh)
{
    static_assert(is_poweroftwo(N) && N >= 1);
    constexpr size_t Nsize = ilog2(N);
    if constexpr (Nsize >= expression_vtable<T, NDims>::Nsizes)
    {
        constexpr size_t Nhalf = N / 2;
        auto low               = get_elements(self, index, axis_params_v<Axis, Nhalf>);
        auto high = get_elements(self, index.add_at(Nhalf, cval<index_t, Axis>), axis_params_v<Axis, Nhalf>);
        return concat(low, high);
    }
    else
    {
        portable_vec<T, N> result;
        self.vtable->fn_get_elements[Axis][Nsize](self.instance, index, result.elem);
        return result;
    }
}

template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const expression_handle<T, NDims>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const identity<vec<T, N>>& value)
{
    static_assert(is_poweroftwo(N) && N >= 1);
    constexpr size_t Nsize = ilog2(N);
    if constexpr (Nsize >= expression_vtable<T, NDims>::Nsizes)
    {
        constexpr size_t Nhalf = N / 2;
        set_elements(self, index, axis_params_v<Axis, Nhalf>, slice<0, Nhalf>(value));
        set_elements(self, index.add_at(Nhalf, cval<index_t, Axis>), axis_params_v<Axis, Nhalf>,
                     slice<Nhalf, Nhalf>(value));
    }
    else
    {
        self.vtable->fn_set_elements[Axis][Nsize](self.instance, index, &value.front());
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
    static expression_vtable<T, Dims> vtable{ ctype_t<std::decay_t<E>>{} };
    return &vtable;
}
} // namespace internal

} // namespace CMT_ARCH_NAME

/** @brief Converts the given expression into an opaque object.
 *  This overload takes reference to the expression.
 *  @warning Use with caution with local variables.
 */
template <typename E, typename T = expression_value_type<E>, index_t Dims = expression_dims<E>>
KFR_INTRINSIC expression_handle<T, Dims> to_handle(E& expr)
{
    return expression_handle<T>(std::addressof(expr), internal::make_expression_vtable<T, Dims, E>());
}

/** @brief Converts the given expression into an opaque object.
 *  This overload takes ownership of the expression (Move semantics).
 *  @note Use std::move to force use of this overload.
 */
template <typename E, typename T = expression_value_type<E>, index_t Dims = expression_dims<E>>
KFR_INTRINSIC expression_handle<T, Dims> to_handle(E&& expr)
{
    std::shared_ptr<expression_resource> ptr = make_resource(std::move(expr));
    void* instance                           = ptr->instance();
    return expression_handle<T, Dims>(instance, internal::make_expression_vtable<T, Dims, E>(),
                                      std::move(ptr));
}

template <typename T, index_t Dims = 1, size_t Key = 0>
struct expression_placeholder
{
public:
    using value_type                      = T;
    expression_placeholder() CMT_NOEXCEPT = default;
    expression_handle<T, Dims> handle;
};

template <typename T, index_t Dims, size_t Key>
struct expression_traits<expression_placeholder<T, Dims, Key>> : public expression_traits_defaults
{
    using value_type             = T;
    constexpr static size_t dims = Dims;
    constexpr static shape<dims> get_shape(const expression_placeholder<T, Dims, Key>& self)
    {
        return self.handle ? ::kfr::get_shape(self.handle) : shape<dims>(infinite_size);
    }
    constexpr static shape<dims> get_shape() { return shape<dims>(undefined_size); }
};

inline namespace CMT_ARCH_NAME
{

template <typename T, index_t Dims, size_t Key, index_t VecAxis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_placeholder<T, Dims, Key>& self, shape<Dims> index,
                                     axis_params<VecAxis, N> sh)
{
    return self.handle ? get_elements(self.handle, index, sh) : 0;
}
} // namespace CMT_ARCH_NAME

template <typename T, index_t Dims = 1, size_t Key = 0>
KFR_INTRINSIC expression_placeholder<T, Dims, Key> placeholder(csize_t<Key> = csize_t<Key>{})
{
    return expression_placeholder<T, Dims, Key>();
}

template <typename... Args>
KFR_INTRINSIC bool substitute(const internal_generic::anything&, Args&&...)
{
    return false;
}

inline namespace CMT_ARCH_NAME
{
namespace internal
{
template <typename... Args, typename T, index_t Dims, size_t Key, size_t... indices>
KFR_INTRINSIC bool substitute_helper(expression_with_arguments<Args...>& expr,
                                     expression_handle<T, Dims> new_handle, csize_t<Key>,
                                     csizes_t<indices...>);
}
} // namespace CMT_ARCH_NAME

template <typename T, index_t Dims, size_t Key = 0>
KFR_INTRINSIC bool substitute(expression_placeholder<T, Dims, Key>& expr,
                              expression_handle<T, Dims> new_handle, csize_t<Key> = csize_t<Key>{})
{
    expr.handle = std::move(new_handle);
    return true;
}

template <typename... Args, typename T, index_t Dims, size_t Key = 0>
KFR_INTRINSIC bool substitute(expression_with_arguments<Args...>& expr, expression_handle<T, Dims> new_handle,
                              csize_t<Key> = csize_t<Key>{})
{
    return internal::substitute_helper(expr, std::move(new_handle), csize_t<Key>{}, indicesfor_t<Args...>{});
}

template <typename T, index_t Dims, size_t Key = 0>
KFR_INTRINSIC bool substitute(expression_handle<T, Dims>& expr, expression_handle<T, Dims> new_handle,
                              csize_t<Key> = csize_t<Key>{})
{
    static_assert(Key == 0, "expression_handle supports only Key = 0");
    return expr.substitute(std::move(new_handle));
}

inline namespace CMT_ARCH_NAME
{
namespace internal
{

template <typename... Args, typename T, index_t Dims, size_t Key, size_t... indices>
KFR_INTRINSIC bool substitute_helper(expression_with_arguments<Args...>& expr,
                                     expression_handle<T, Dims> new_handle, csize_t<Key>,
                                     csizes_t<indices...>)
{
    return (substitute(std::get<indices>(expr.args), std::move(new_handle), csize_t<Key>()) || ...);
}

template <typename Expression, typename T, index_t Dims, size_t Key>
KFR_INTRINSIC bool invoke_substitute(Expression& expr, expression_handle<T, Dims> new_handle, csize_t<Key>)
{
    return kfr::substitute(expr, std::move(new_handle), csize_t<Key>{});
}

} // namespace internal

} // namespace CMT_ARCH_NAME

} // namespace kfr
