/** @addtogroup base
 *  @{
 */
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

#include "../meta/memory.hpp"
#include "../simd/vec.hpp"
#include "basic_expressions.hpp"
#include <memory>

namespace kfr
{

/** @brief Opaque type-erased handle to an expression (forward declaration).
 *  @see expression_handle
 */
template <typename T, index_t Dims>
struct expression_handle;

/** @brief Maximum vector width (in elements) supported by an expression handle for type @c T.
 *
 *  Twice the widest native vector for the highest available CPU target; get_elements/set_elements
 *  calls requesting more than this are split recursively.
 */
template <typename T>
constexpr size_t maximum_expression_width = vector_width_for<T, cpu_t::highest> * 2;

/** @brief Expands a @ref cvals_t pack into a template expecting non-type parameters.
 *  @tparam T       Value type of the pack.
 *  @tparam Tpl     Variadic value template to instantiate.
 *  @tparam Pack    A @ref cvals_t specialization.
 */
template <typename T, template <T...> typename Tpl, typename Pack>
struct expand_cvals;

/** @brief Specialization of @ref expand_cvals that instantiates @c Tpl with the pack values. */
template <typename T, template <T...> typename Tpl, T... vals>
struct expand_cvals<T, Tpl, cvals_t<T, vals...>>
{
    using type = Tpl<vals...>;
};

inline namespace KFR_ARCH_NAME
{

namespace internal
{

template <typename Expression, typename T, index_t Dims, size_t key = 0>
KFR_INTRINSIC bool invoke_substitute(Expression& expr, expression_handle<T, Dims> new_handle,
                                     csize_t<key> = {});
}
} // namespace KFR_ARCH_NAME

/** @brief Virtual table providing type-erased access to an expression of value type @c T
 *         and dimensionality @c Dims.
 *
 *  Stores function pointers for shape query, placeholder substitution, pass begin/end and
 *  get/set of power-of-two element blocks along each axis. One static instance is created
 *  per concrete expression type via the constructor taking @c ctype_t<Expression>.
 */
template <typename T, index_t Dims>
struct expression_vtable
{
    /** @brief Number of supported power-of-two block sizes (1 .. maximum_expression_width<T>). */
    constexpr static const size_t Nsizes = 1 + ilog2(maximum_expression_width<T>);
    /** @brief Largest supported block size. */
    constexpr static const size_t Nmax = 1 << Nsizes;

    /** @brief Signature of a get_elements callback: read @c N samples into @c dest. */
    using func_get = void (*)(void*, shape<Dims>, T*);
    /** @brief Signature of a set_elements callback: write @c N samples from @c src. */
    using func_set = void (*)(void*, shape<Dims>, const T*);
    /** @brief Signature of the shape query callback. */
    using func_shapeof = void (*)(void*, shape<Dims>&);
    /** @brief Signature of the placeholder substitution callback. */
    using func_substitute = bool (*)(void*, expression_handle<T, Dims>);
    /** @brief Signature of the begin_pass/end_pass callbacks. */
    using func_pass = void (*)(void*, shape<Dims>, shape<Dims>);

    func_shapeof fn_shapeof; ///< Shape query callback.
    func_substitute fn_substitute; ///< Placeholder substitution callback.
    func_pass fn_begin_pass; ///< begin_pass callback.
    func_pass fn_end_pass; ///< end_pass callback.
    std::array<std::array<func_get, Nsizes>, Dims> fn_get_elements; ///< get_elements callbacks per axis/size.
    std::array<std::array<func_set, Nsizes>, Dims> fn_set_elements; ///< set_elements callbacks per axis/size.

    /** @brief Build the vtable for concrete expression type @c Expression.
     *  @tparam Expression Concrete expression type to bind.
     *  @param t Type tag selecting the expression type.
     */
    template <typename Expression>
    KFR_MEM_INTRINSIC expression_vtable(ctype_t<Expression> t)
    {
        fn_shapeof    = &static_shapeof<Expression>;
        fn_substitute = &static_substitute<Expression>;
        fn_begin_pass = &static_begin_pass<Expression>;
        fn_end_pass   = &static_end_pass<Expression>;
        cforeach(csizeseq<Nsizes>,
                 [&](auto size_) KFR_INLINE_LAMBDA
                 {
                     cforeach(csizeseq<Dims>,
                              [&](auto axis_) KFR_INLINE_LAMBDA
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

    /** @brief get_elements trampoline: forwards to @c get_elements for @c Expression. */
    template <typename Expression, size_t N, index_t VecAxis>
    static void static_get_elements(void* instance, shape<Dims> index, T* dest)
    {
        if constexpr (is_input_expression<Expression>)
        {
            write(dest, get_elements(*static_cast<Expression*>(instance), index, axis_params_v<VecAxis, N>));
        }
    }
    /** @brief set_elements trampoline: forwards to @c set_elements for @c Expression. */
    template <typename Expression, size_t N, index_t VecAxis>
    static void static_set_elements(void* instance, shape<Dims> index, const T* src)
    {
        if constexpr (is_output_expression<Expression>)
        {
            set_elements(*static_cast<Expression*>(instance), index, axis_params_v<VecAxis, N>, read<N>(src));
        }
    }
    /** @brief shapeof trampoline: queries @c expression_traits::get_shape for @c Expression. */
    template <typename Expression>
    static void static_shapeof(void* instance, shape<Dims>& result)
    {
        result = expression_traits<Expression>::get_shape(*static_cast<Expression*>(instance));
    }
    /** @brief substitute trampoline: invokes @c kfr::substitute on @c Expression. */
    template <typename Expression>
    static bool static_substitute(void* instance, expression_handle<T, Dims> ptr)
    {
        return internal::invoke_substitute(*static_cast<Expression*>(instance), std::move(ptr));
    }
    /** @brief begin_pass trampoline: forwards to @c begin_pass for @c Expression. */
    template <typename Expression>
    static void static_begin_pass(void* instance, shape<Dims> start, shape<Dims> stop)
    {
        begin_pass(*static_cast<Expression*>(instance), start, stop);
    }
    /** @brief end_pass trampoline: forwards to @c end_pass for @c Expression. */
    template <typename Expression>
    static void static_end_pass(void* instance, shape<Dims> start, shape<Dims> stop)
    {
        end_pass(*static_cast<Expression*>(instance), start, stop);
    }
};

/** @brief Base class for owning the storage of a type-erased expression.
 *
 *  A handle that owns its expression holds a @c shared_ptr to an
 *  @ref expression_resource_impl specialization.
 */
struct expression_resource
{
    /** @brief Returns a pointer to the owned expression instance, or @c nullptr. */
    virtual ~expression_resource() {}
    virtual void* instance() { return nullptr; }
};

/** @brief @ref expression_resource that owns an expression of type @c E by value.
 *  @tparam E Concrete expression type stored (moved in). */
template <typename E>
struct expression_resource_impl : expression_resource
{
    /** @brief Take ownership of @c e by move. */
    expression_resource_impl(E&& e) noexcept : e(std::move(e)) {}
    virtual ~expression_resource_impl() {}
    KFR_INTRINSIC virtual void* instance() override final { return &e; }

public:
#ifdef __cpp_aligned_new
    /** @brief Aligned delete overload matching the aligned allocation in @ref make_resource. */
    static void operator delete(void* p, std::align_val_t al) noexcept { details::aligned_free(p); }
#endif

private:
    E e;
};

/** @brief Create an @ref expression_resource owning @c e (moved in), allocated with proper alignment.
 *  @tparam E Concrete expression type.
 *  @param e Expression to take ownership of.
 *  @return shared_ptr to the owning resource.
 */
template <typename E>
KFR_INTRINSIC std::shared_ptr<expression_resource> make_resource(E&& e)
{
    using T = expression_resource_impl<std::decay_t<E>>;
    return std::static_pointer_cast<expression_resource>(std::shared_ptr<T>(
        new (aligned_allocate<T>()) T(std::move(e)), [](T* pi) { aligned_deallocate<T>(pi); }));
}

/** @brief Type-erased reference to an expression of value type @c T and dimensionality @c Dims.
 *
 *  Holds a pointer to the concrete expression instance, a pointer to its vtable, and (optionally)
 *  a shared_ptr to a resource that owns the expression. When @c resource is null the handle is
 *  non-owning and the caller must keep the referenced expression alive.
 *
 *  @tparam T    Value type produced/consumed by the expression.
 *  @tparam Dims Number of dimensions (default 1).
 */
template <typename T, index_t Dims = 1>
struct expression_handle
{
    void* instance; ///< Pointer to the concrete expression instance.
    const expression_vtable<T, Dims>* vtable; ///< Pointer to the static vtable for the instance type.
    std::shared_ptr<expression_resource> resource; ///< Owning resource, or null for a non-owning handle.

    /** @brief Construct an empty (invalid) handle. */
    expression_handle() noexcept : instance(nullptr), vtable(nullptr) {}
    /** @brief Construct a handle referencing @c instance with vtable @c vtable.
     *  @param instance Pointer to the concrete expression instance (not owned unless @c resource is set).
     *  @param vtable   Vtable for the instance type.
     *  @param resource Optional owning resource; when set the handle keeps the instance alive. */
    expression_handle(const void* instance, const expression_vtable<T, Dims>* vtable,
                      std::shared_ptr<expression_resource> resource = nullptr)
        : instance(const_cast<void*>(instance)), vtable(vtable), resource(std::move(resource))
    {
    }

    /** @brief @c true if the handle references a valid instance. */
    explicit operator bool() const { return instance != nullptr; }

    /** @brief Replace the placeholder inside the referenced expression with @c new_handle.
     *  @param new_handle Handle to substitute into the placeholder slot.
     *  @return @c true if a placeholder was found and substituted.
     *  @see substitute(expression_handle&, expression_handle, csize_t) */
    bool substitute(expression_handle<T, Dims> new_handle)
    {
        return vtable->fn_substitute(instance, std::move(new_handle));
    }
};

/** @brief @ref expression_traits specialization for @ref expression_handle.
 *  @relates expression_handle
 */
template <typename T, index_t Dims>
struct expression_traits<expression_handle<T, Dims>> : expression_traits_defaults
{
    using value_type             = T; ///< Value type produced by the expression.
    constexpr static size_t dims = Dims; ///< Number of dimensions.
    /** @brief Query the shape of the referenced expression via its vtable. */
    constexpr static shape<dims> get_shape(const expression_handle<T, Dims>& self)
    {
        shape<dims> result;
        self.vtable->fn_shapeof(self.instance, result);
        return result;
    }
    /** @brief Static shape query: undefined for an unbound handle. */
    constexpr static shape<dims> get_shape() { return shape<dims>(undefined_size); }

    constexpr static inline bool random_access = false; ///< Handles are not random-access.
};

inline namespace KFR_ARCH_NAME
{

/** @brief Internal ADL-provided implementation for @ref expression_handle expressions. */
template <typename T, index_t NDims>
KFR_INTRINSIC void begin_pass(const expression_handle<T, NDims>& self, shape<NDims> start, shape<NDims> stop)
{
    self.vtable->fn_begin_pass(self.instance, start, stop);
}
/** @brief Internal ADL-provided implementation for @ref expression_handle expressions. */
template <typename T, index_t NDims>
KFR_INTRINSIC void end_pass(const expression_handle<T, NDims>& self, shape<NDims> start, shape<NDims> stop)
{
    self.vtable->fn_end_pass(self.instance, start, stop);
}

/** @brief Internal ADL-provided implementation for @ref expression_handle expressions.
 *
 *  Reads @c N samples starting at @c index along @c Axis. Block sizes larger than
 *  @ref expression_vtable::Nmax are split recursively into two half-size reads.
 */
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

/** @brief Internal ADL-provided implementation for @ref expression_handle expressions.
 *
 *  Writes @c N samples starting at @c index along @c Axis. Block sizes larger than
 *  @ref expression_vtable::Nmax are split recursively into two half-size writes.
 */
template <typename T, index_t NDims, index_t Axis, size_t N>
KFR_INTRINSIC void set_elements(const expression_handle<T, NDims>& self, const shape<NDims>& index,
                                const axis_params<Axis, N>& sh, const std::type_identity_t<vec<T, N>>& value)
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
} // namespace KFR_ARCH_NAME

inline namespace KFR_ARCH_NAME
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

} // namespace KFR_ARCH_NAME

/** @brief Converts the given expression into an opaque handle.
 *
 *  This overload takes a reference to the expression; the returned handle is non-owning
 *  and the caller must keep @c expr alive for the lifetime of the handle.
 *
 *  @tparam E    Concrete expression type.
 *  @tparam T    Value type (deduced from @c E).\n
 *  @tparam Dims Dimensionality (deduced from @c E).
 *  @param expr  Expression to wrap.
 *  @return Non-owning handle referencing @c expr.
 *  @warning Use with caution with local variables; the handle does not extend @c expr's lifetime.
 */
template <typename E, typename T = expression_value_type<E>, index_t Dims = expression_dims<E>>
KFR_INTRINSIC expression_handle<T, Dims> to_handle(E& expr)
{
    return expression_handle<T>(std::addressof(expr), internal::make_expression_vtable<T, Dims, E>());
}

/** @brief Converts the given expression into an opaque handle.
 *
 *  This overload takes ownership of the expression (move semantics): the handle holds a
 *  shared resource that keeps the moved expression alive. Pass an rvalue (e.g. via @c std::move)
 *  to select this overload.
 *
 *  @tparam E    Concrete expression type.
 *  @tparam T    Value type (deduced from @c E).
 *  @tparam Dims Dimensionality (deduced from @c E).
 *  @param expr  Expression to move into the handle.
 *  @return Owning handle that keeps @c expr alive.
 *  @note Use @c std::move to force use of this overload.
 */
template <typename E, typename T = expression_value_type<E>, index_t Dims = expression_dims<E>>
KFR_INTRINSIC expression_handle<T, Dims> to_handle(E&& expr)
{
    std::shared_ptr<expression_resource> ptr = make_resource(std::move(expr));
    void* instance                           = ptr->instance();
    return expression_handle<T, Dims>(instance, internal::make_expression_vtable<T, Dims, E>(),
                                      std::move(ptr));
}

/** @brief A placeholder expression that can be bound to a concrete expression at runtime.
 *
 *  Used to build an expression graph with a deferred input slot, then later bound via
 *  @ref substitute. Multiple distinct placeholders in one graph are distinguished by @c Key.
 *
 *  @tparam T    Value type of the placeholder.
 *  @tparam Dims Number of dimensions.
 *  @tparam Key  Integer key identifying this placeholder among several in the same graph.
 */
template <typename T, index_t Dims = 1, size_t Key = 0>
struct expression_placeholder
{
public:
    using value_type                  = T; ///< Value type produced by the placeholder.
    expression_placeholder() noexcept = default;
    /** @brief Handle to the bound expression, or empty if unbound. */
    expression_handle<T, Dims> handle;
};

/** @brief @ref expression_traits specialization for @ref expression_placeholder.
 *  @relates expression_placeholder
 */
template <typename T, index_t Dims, size_t Key>
struct expression_traits<expression_placeholder<T, Dims, Key>> : public expression_traits_defaults
{
    using value_type             = T; ///< Value type produced by the placeholder.
    constexpr static size_t dims = Dims; ///< Number of dimensions.
    /** @brief Shape of the bound expression, or @c infinite_size when unbound. */
    constexpr static shape<dims> get_shape(const expression_placeholder<T, Dims, Key>& self)
    {
        return self.handle ? ::kfr::get_shape(self.handle) : shape<dims>(infinite_size);
    }
    /** @brief Static shape query: undefined for an unbound placeholder. */
    constexpr static shape<dims> get_shape() { return shape<dims>(undefined_size); }
};

inline namespace KFR_ARCH_NAME
{

/** @brief Internal ADL-provided implementation for @ref expression_placeholder expressions.
 *
 *  Reads from the bound handle, or returns zeros when the placeholder is unbound.
 */
template <typename T, index_t Dims, size_t Key, index_t VecAxis, size_t N>
KFR_INTRINSIC vec<T, N> get_elements(const expression_placeholder<T, Dims, Key>& self, shape<Dims> index,
                                     axis_params<VecAxis, N> sh)
{
    return self.handle ? get_elements(self.handle, index, sh) : 0;
}
} // namespace KFR_ARCH_NAME

/** @brief Create an unbound @ref expression_placeholder with the given key.
 *  @tparam T    Value type of the placeholder.
 *  @tparam Dims Number of dimensions (default 1).
 *  @tparam Key  Integer key identifying this placeholder (default 0).
 *  @param key   Compile-time key tag.
 *  @return A new unbound placeholder.
 */
template <typename T, index_t Dims = 1, size_t Key = 0>
KFR_INTRINSIC expression_placeholder<T, Dims, Key> placeholder(csize_t<Key> = csize_t<Key>{})
{
    return expression_placeholder<T, Dims, Key>();
}

/** @brief Fallback @c substitute that accepts any expression and arguments and does nothing.
 *  @return Always @c false.
 */
template <typename... Args>
KFR_INTRINSIC bool substitute(const internal_generic::anything&, Args&&...)
{
    return false;
}

inline namespace KFR_ARCH_NAME
{
namespace internal
{
template <typename... Args, typename T, index_t Dims, size_t Key, size_t... indices>
KFR_INTRINSIC bool substitute_helper(expression_with_arguments<Args...>& expr,
                                     expression_handle<T, Dims> new_handle, csize_t<Key>,
                                     csizes_t<indices...>);
}
} // namespace KFR_ARCH_NAME

/** @brief Bind a placeholder to a concrete expression handle.
 *
 *  Replaces the placeholder's stored handle with @c new_handle. The @c Key template argument
 *  selects which placeholder to bind when several exist in the same graph.
 *
 *  @param expr       Placeholder to bind.
 *  @param new_handle Handle to the expression that will fill the placeholder slot.
 *  @param key        Compile-time key tag (must match @c Key).
 *  @return Always @c true (a placeholder always accepts a binding).
 */
template <typename T, index_t Dims, size_t Key = 0>
KFR_INTRINSIC bool substitute(expression_placeholder<T, Dims, Key>& expr,
                              expression_handle<T, Dims> new_handle, csize_t<Key> = csize_t<Key>{})
{
    expr.handle = std::move(new_handle);
    return true;
}

/** @brief Recursively substitute a placeholder inside a composite expression.
 *
 *  Walks the arguments of @c expr and substitutes @c new_handle into the first matching
 *  placeholder (selected by @c Key). Returns @c true if any argument accepted the substitution.
 *
 *  @param expr       Composite expression whose arguments are searched.
 *  @param new_handle Handle to substitute into a matching placeholder.
 *  @param key        Compile-time key tag selecting which placeholder to bind.
 *  @return @c true if a placeholder was found and substituted.
 */
template <typename... Args, typename T, index_t Dims, size_t Key = 0>
KFR_INTRINSIC bool substitute(expression_with_arguments<Args...>& expr, expression_handle<T, Dims> new_handle,
                              csize_t<Key> = csize_t<Key>{})
{
    return internal::substitute_helper(expr, std::move(new_handle), csize_t<Key>{}, indicesfor_t<Args...>{});
}

/** @brief Substitute the placeholder inside the expression referenced by @c expr.
 *
 *  Delegates to @ref expression_handle::substitute via the vtable. Only @c Key == 0 is
 *  supported because the handle hides the concrete expression type.
 *
 *  @param expr       Handle whose referenced expression contains a placeholder.
 *  @param new_handle Handle to substitute into the placeholder slot.
 *  @param key        Compile-time key tag (must be 0).
 *  @return @c true if a placeholder was found and substituted.
 */
template <typename T, index_t Dims, size_t Key = 0>
KFR_INTRINSIC bool substitute(expression_handle<T, Dims>& expr, expression_handle<T, Dims> new_handle,
                              csize_t<Key> = csize_t<Key>{})
{
    static_assert(Key == 0, "expression_handle supports only Key = 0");
    return expr.substitute(std::move(new_handle));
}

inline namespace KFR_ARCH_NAME
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

} // namespace KFR_ARCH_NAME

} // namespace kfr
