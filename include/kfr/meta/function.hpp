/** @addtogroup meta
 *  @{
 */
#pragma once

#include "../meta.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>
#if KFR_HAS_EXCEPTIONS
#include <functional>
#endif

namespace kfr
{

namespace details
{

template <typename Fn>
struct func_filter
{
    typedef Fn type;
};
template <typename Result, typename... Args>
struct func_filter<Result(Args...)>
{
    typedef Result (*type)(Args...);
};

template <typename T>
constexpr KFR_INTRINSIC T return_val() noexcept
{
    return {};
}

template <>
constexpr KFR_INTRINSIC void return_val<void>() noexcept
{
}
} // namespace details

/**
 * @brief std::function-like lightweight function wrapper
 * @code
 * function<int( float )> f = []( float x ){ return static_cast<int>( x ); };
 * CHECK( f( 3.4f ) == 3 )
 * @endcode
 *
 * This class is not intended to replace `std::function` in all cases. Its
 * primary goal is to allocate aligned memory for the stored callable, so that
 * SIMD values (such as `vec<>`) and other over-aligned types can be safely used
 * in lambda captures without violating alignment requirements.
 *
 * The primary template is declared but not defined; only the specialization
 * `function<R(Args...)>` is usable.
 *
 * @tparam F Function type of the form `R(Args...)`.
 */
template <typename F>
struct function;

namespace details
{
template <typename R, typename... Args>
struct function_abstract
{
    virtual ~function_abstract() {}
    virtual R operator()(Args... args) = 0;
};
template <typename Fn, typename R, typename... Args>
struct function_impl : public function_abstract<R, Args...>
{
    inline static void* operator new(size_t size) noexcept { return aligned_allocate(size, alignof(Fn)); }
    inline static void operator delete(void* ptr) noexcept { return aligned_deallocate(ptr); }

#ifdef __cpp_aligned_new
    inline static void* operator new(size_t size, std::align_val_t al) noexcept
    {
        return aligned_allocate(size, static_cast<size_t>(al));
    }
    inline static void operator delete(void* ptr, std::align_val_t al) noexcept
    {
        return aligned_deallocate(ptr);
    }
#endif

    template <typename Fn_>
    function_impl(Fn_ fn) : fn(std::forward<Fn_>(fn))
    {
    }
    ~function_impl() override {}
    R operator()(Args... args) override { return fn(std::forward<Args>(args)...); }
    Fn fn;
};
} // namespace details

/**
 * @brief Specialization of @ref function for the function type `R(Args...)`.
 *
 * Stores any callable convertible to `R(Args...)` on the heap through a
 * type-erased polymorphic holder and invokes it through `operator()`.
 *
 * @tparam R    Return type of the wrapped callable.
 * @tparam Args  Argument types of the wrapped callable.
 */
template <typename R, typename... Args>
struct function<R(Args...)>
{
    /** @brief Constructs an empty wrapper. */
    function() noexcept = default;

    /** @brief Constructs an empty wrapper from `nullptr`. */
    function(std::nullptr_t) noexcept {}

    /**
     * @brief Constructs the wrapper from a callable.
     * @tparam Fn  Callable type satisfying `std::is_invocable_r_v<R, Fn, Args...>`.
     * @param fn  Callable to wrap. Must be invocable with `Args...` and return `R`.
     */
    template <typename Fn>
        requires(std::is_invocable_r_v<R, Fn, Args...> && !std::is_same_v<std::decay_t<Fn>, function>)
    function(Fn fn) : impl(new details::function_impl<std::decay_t<Fn>, R, Args...>(std::move(fn)))
    {
    }

    /** @brief Copy-constructs; shares the underlying implementation. */
    function(const function&) = default;

    /** @brief Move-constructs; leaves the source empty. */
    function(function&&) noexcept = default;

    /** @brief Copy-assigns; shares the underlying implementation. */
    function& operator=(const function&) = default;

    /** @brief Move-assigns; leaves the source empty. */
    function& operator=(function&&) noexcept = default;

    /**
     * @brief Invokes the wrapped callable.
     * @param args Arguments forwarded to the stored callable.
     * @return Value returned by the callable.
     * @throw std::bad_function_call if `*this` is empty (only when exceptions are enabled).
     *
     * When exceptions are disabled, calling an empty wrapper dereferences a null
     * pointer; check @ref operator bool or @ref empty() first in that case.
     */
    R operator()(Args... args) const
    {
#if KFR_HAS_EXCEPTIONS
        if (impl)
        {
            return impl->operator()(std::forward<Args>(args)...);
        }
        throw std::bad_function_call();
#else
        // With exceptions disabled let it crash. To prevent this, check first
        return impl->operator()(std::forward<Args>(args)...);
#endif
    }

    /** @brief Returns `true` if the wrapper holds a callable. */
    [[nodiscard]] explicit operator bool() const { return !!impl; }

    /** @brief Returns `true` if the wrapper is empty. */
    [[nodiscard]] bool empty() const { return !impl; }

    /// Type-erased implementation holding the wrapped callable.
    std::shared_ptr<details::function_abstract<R, Args...>> impl;

    /** @brief Returns `true` if both wrappers share the same implementation. */
    bool operator==(const function& fn) const { return impl == fn.impl; }
    /** @brief Returns `true` if the wrappers do not share the same implementation. */
    bool operator!=(const function& fn) const { return !operator==(fn); }
};

/**
 * @brief Base case of compile-time dispatch: no candidate value matches.
 *
 * Returns a wrapper that invokes the default callable `deffn` for every call.
 *
 * @tparam Ret    Return type of the dispatched function.
 * @tparam Args   Argument types of the dispatched function.
 * @tparam T      Type of the dispatch value.
 * @tparam Fn     Callable type (unused in this overload).
 * @tparam DefFn  Default callable type invoked when no candidate matches.
 * @param deffn  Default callable invoked when no candidate matches.
 * @return A @ref function wrapping `deffn`.
 */
template <typename Ret, typename... Args, typename T, typename Fn, typename DefFn = fn_noop>
KFR_INLINE function<Ret(Args...)> cdispatch(cvals_t<T>, std::type_identity_t<T>, Fn&&,
                                            DefFn&& deffn = DefFn())
{
    return [=](Args... args) KFR_INLINE_LAMBDA -> Ret { return deffn(std::forward<Args>(args)...); };
}

/**
 * @brief Compile-time dispatch over a list of constant values.
 *
 * Compares the runtime `value` against each candidate in `cvals_t<T, v0, values...>`
 * and returns a @ref function bound to `fn` specialized with the matching
 * `cval_t<T, v0>`. If `value` matches none of the candidates, the default
 * callable `deffn` is used (see the base-case overload).
 *
 * @tparam Ret    Return type of the dispatched function.
 * @tparam Args   Argument types of the dispatched function.
 * @tparam T      Type of the dispatch value.
 * @tparam v0     First candidate value.
 * @tparam values Remaining candidate values.
 * @tparam Fn     Callable invoked with `cval_t<T, v0>` and the forwarded arguments on match.
 * @tparam DefFn  Default callable type invoked when no candidate matches.
 * @param value  Runtime value to dispatch on.
 * @param fn    Callable invoked on match as `fn(cval_t<T, v0>(), args...)`.
 * @param deffn Default callable invoked when no candidate matches.
 * @return A @ref function bound to the matching specialization of `fn`, or to `deffn`.
 */
template <typename Ret, typename... Args, typename T, T v0, T... values, typename Fn,
          typename DefFn = fn_noop>
inline function<Ret(Args...)> cdispatch(cvals_t<T, v0, values...>, std::type_identity_t<T> value, Fn&& fn,
                                        DefFn&& deffn = DefFn())
{
    if (value == v0)
    {
        return [=](Args... args) KFR_INLINE_LAMBDA -> Ret
        { return fn(cval_t<T, v0>(), std::forward<Args>(args)...); };
    }
    else
    {
        return cdispatch<Ret, Args...>(cvals_t<T, values...>(), value, std::forward<Fn>(fn),
                                       std::forward<DefFn>(deffn));
    }
}
} // namespace kfr
