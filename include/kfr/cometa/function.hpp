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
    CMT_INTRIN virtual ~virtual_function()      = default;
};

template <typename Fn, typename Result, typename... Args>
struct virtual_function_impl : virtual_function<Result, Args...>
{
public:
    CMT_INTRIN virtual_function_impl(const Fn& fn) : fn(fn) {}
    CMT_INTRIN Result operator()(Args... args) override final { return fn(args...); }
    CMT_INTRIN virtual_function<Result, Args...>* make_copy() const override final
    {
        return new virtual_function_impl{ fn };
    }
    CMT_INTRIN ~virtual_function_impl() {}

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
constexpr CMT_INTRIN T return_val() noexcept
{
    return {};
}

template <>
constexpr CMT_INTRIN void return_val<void>() noexcept
{
}
}

template <typename>
struct function;

/**
 * @brief std::function-like lightweight function wrapper
 * @code
 * function<int( float )> f = []( float x ){ return static_cast<int>( x ); };
 * CHECK( f( 3.4f ) == 3 )
 * @encode
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

    CMT_INTRIN function() : fn(nullptr) {}
    CMT_INTRIN function(std::nullptr_t) : fn(nullptr) {}
    template <typename Func>
    CMT_INTRIN function(const Func& x)
        : fn(new details::virtual_function_impl<typename details::func_filter<Func>::type, Result, Args...>(
              x))
    {
    }
    function(const this_t& other) : fn(other.fn ? other.fn->make_copy() : nullptr) {}
    CMT_INTRIN function& operator=(const this_t& other)
    {
        if ((&other != this) && (other.fn))
        {
            auto* temp = other.fn->make_copy();
            delete fn;
            fn = temp;
        }
        return *this;
    }
    CMT_INTRIN function& operator=(std::nullptr_t)
    {
        delete fn;
        fn = nullptr;
        return *this;
    }
    template <typename Fn>
    CMT_INTRIN function& operator=(const Fn& x)
    {
        using FnImpl =
            details::virtual_function_impl<typename details::func_filter<Fn>::type, Result, Args...>;
        FnImpl* temp = new FnImpl(x);
        delete fn;
        fn = temp;
        return *this;
    }
    CMT_INTRIN Result operator()(Args... args) const { return (*fn)(std::forward<Args>(args)...); }
    template <typename TResult>
    CMT_INTRIN Result call(TResult&& default_result, Args... args) const
    {
        return fn ? (*fn)(std::forward<Args>(args)...) : std::forward<TResult>(default_result);
    }
    CMT_INTRIN explicit operator bool() const noexcept { return !!fn; }

    CMT_INTRIN ~function() { delete fn; }
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
                   CMT_INLINE_MEMBER -> Ret { return fn(cval<T, v0>, std::forward<Args>(args)...); };
    }
    else
    {
        return cdispatch<Ret, Args...>(cvals_t<T, values...>(), value, std::forward<Fn>(fn),
                                       std::forward<DefFn>(deffn));
    }
}
}
