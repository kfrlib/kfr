/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"
#include "memory.hpp"
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <type_traits>
#if CMT_HAS_EXCEPTIONS
#include <functional>
#endif

namespace cometa
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
constexpr CMT_INTRINSIC T return_val() CMT_NOEXCEPT
{
    return {};
}

template <>
constexpr CMT_INTRINSIC void return_val<void>() CMT_NOEXCEPT
{
}
} // namespace details

/**
 * @brief std::function-like lightweight function wrapper
 * @code
 * function<int( float )> f = []( float x ){ return static_cast<int>( x ); };
 * CHECK( f( 3.4f ) == 3 )
 * @endcode
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

template <typename R, typename... Args>
struct function<R(Args...)>
{
    function() noexcept = default;

    function(std::nullptr_t) noexcept {}

    template <typename Fn, typename = std::enable_if_t<std::is_invocable_r_v<R, Fn, Args...> &&
                                                       !std::is_same_v<std::decay_t<Fn>, function>>>
    function(Fn fn) : impl(new details::function_impl<std::decay_t<Fn>, R, Args...>(std::move(fn)))
    {
    }

    function(const function&) = default;

    function(function&&) noexcept = default;

    function& operator=(const function&) = default;

    function& operator=(function&&) noexcept = default;

    R operator()(Args... args) const
    {
#if CMT_HAS_EXCEPTIONS
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

    [[nodiscard]] explicit operator bool() const { return !!impl; }

    [[nodiscard]] bool empty() const { return !impl; }

    std::shared_ptr<details::function_abstract<R, Args...>> impl;

    bool operator==(const function& fn) const { return impl == fn.impl; }
    bool operator!=(const function& fn) const { return !operator==(fn); }
};

template <typename Ret, typename... Args, typename T, typename Fn, typename DefFn = fn_noop>
CMT_INLINE function<Ret(Args...)> cdispatch(cvals_t<T>, identity<T>, Fn&&, DefFn&& deffn = DefFn())
{
    return [=](Args... args) CMT_INLINE_LAMBDA -> Ret { return deffn(std::forward<Args>(args)...); };
}

template <typename Ret, typename... Args, typename T, T v0, T... values, typename Fn,
          typename DefFn = fn_noop>
inline function<Ret(Args...)> cdispatch(cvals_t<T, v0, values...>, identity<T> value, Fn&& fn,
                                        DefFn&& deffn = DefFn())
{
    if (value == v0)
    {
        return [=](Args... args) CMT_INLINE_LAMBDA -> Ret
        { return fn(cval_t<T, v0>(), std::forward<Args>(args)...); };
    }
    else
    {
        return cdispatch<Ret, Args...>(cvals_t<T, values...>(), value, std::forward<Fn>(fn),
                                       std::forward<DefFn>(deffn));
    }
}
} // namespace cometa
