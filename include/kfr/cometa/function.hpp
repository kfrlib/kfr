/** @addtogroup cometa
 *  @{
 */
#pragma once

#include "../cometa.hpp"

namespace cometa
{

namespace details
{

template <typename Result, typename... Args>
struct virtual_function
{
    virtual Result operator()(Args... args)     = 0;
    virtual virtual_function* make_copy() const = 0;
    virtual ~virtual_function()                 = default;
};

template <typename Fn, typename Result, typename... Args>
struct virtual_function_impl : virtual_function<Result, Args...>
{
public:
    CMT_MEM_INTRINSIC virtual_function_impl(const Fn& fn) : fn(fn) {}
    CMT_MEM_INTRINSIC Result operator()(Args... args) final { return fn(args...); }
    CMT_MEM_INTRINSIC virtual_function<Result, Args...>* make_copy() const final
    {
        return new virtual_function_impl{ fn };
    }
    CMT_MEM_INTRINSIC ~virtual_function_impl() {}

private:
    Fn fn;
};

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

template <typename>
struct function;

/**
 * @brief std::function-like lightweight function wrapper
 * @code
 * function<int( float )> f = []( float x ){ return static_cast<int>( x ); };
 * CHECK( f( 3.4f ) == 3 )
 * @endcode
 */
template <typename Result, typename... Args>
struct function<Result(Args...)>
{
    using this_t = function<Result(Args...)>;

    function(function&& other) : fn(other.fn) { other.fn = nullptr; }
    function& operator=(function&& other)
    {
        fn       = other.fn;
        other.fn = nullptr;
        return *this;
    }

    CMT_MEM_INTRINSIC function() : fn(nullptr) {}
    CMT_MEM_INTRINSIC function(std::nullptr_t) : fn(nullptr) {}
    template <typename Func>
    CMT_MEM_INTRINSIC function(const Func& x)
        : fn(new details::virtual_function_impl<typename details::func_filter<Func>::type, Result, Args...>(
              x))
    {
    }
    function(const this_t& other) : fn(other.fn ? other.fn->make_copy() : nullptr) {}
    CMT_MEM_INTRINSIC function& operator=(const this_t& other)
    {
        if ((&other != this) && (other.fn))
        {
            auto* temp = other.fn->make_copy();
            delete fn;
            fn = temp;
        }
        return *this;
    }
    CMT_MEM_INTRINSIC function& operator=(std::nullptr_t)
    {
        delete fn;
        fn = nullptr;
        return *this;
    }
    template <typename Fn>
    CMT_MEM_INTRINSIC function& operator=(const Fn& x)
    {
        using FnImpl =
            details::virtual_function_impl<typename details::func_filter<Fn>::type, Result, Args...>;
        FnImpl* temp = new FnImpl(x);
        delete fn;
        fn = temp;
        return *this;
    }
    CMT_MEM_INTRINSIC Result operator()(Args... args) const { return (*fn)(std::forward<Args>(args)...); }
    template <typename TResult>
    CMT_MEM_INTRINSIC Result call(TResult&& default_result, Args... args) const
    {
        return fn ? (*fn)(std::forward<Args>(args)...) : std::forward<TResult>(default_result);
    }
    CMT_MEM_INTRINSIC explicit operator bool() const CMT_NOEXCEPT { return !!fn; }

    CMT_MEM_INTRINSIC ~function() { delete fn; }

private:
    details::virtual_function<Result, Args...>* fn;
};

template <typename Ret, typename... Args, typename T, typename Fn, typename DefFn = fn_noop>
CMT_INLINE function<Ret(Args...)> cdispatch(cvals_t<T>, identity<T>, Fn&&, DefFn&& deffn = DefFn())
{
    return [=](Args... args) CMT_INLINE_MEMBER -> Ret { return deffn(std::forward<Args>(args)...); };
}

template <typename Ret, typename... Args, typename T, T v0, T... values, typename Fn,
          typename DefFn = fn_noop>
inline function<Ret(Args...)> cdispatch(cvals_t<T, v0, values...>, identity<T> value, Fn&& fn,
                                        DefFn&& deffn = DefFn())
{
    if (value == v0)
    {
        return [=](Args... args)
                   CMT_INLINE_MEMBER -> Ret { return fn(cval_t<T, v0>(), std::forward<Args>(args)...); };
    }
    else
    {
        return cdispatch<Ret, Args...>(cvals_t<T, values...>(), value, std::forward<Fn>(fn),
                                       std::forward<DefFn>(deffn));
    }
}
} // namespace cometa
