#pragma once

#include "cident.h"

#include <algorithm>
#include <array>
#include <tuple>
#include <type_traits>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"

namespace cometa
{

using std::size_t;
using std::ptrdiff_t;

using pvoid = void*;

template <typename...>
using void_t = void;

// Workaround for GCC 4.8
template <typename T>
constexpr const T& const_max(const T& x, const T& y)
{
    return x > y ? x : y;
}
template <typename T>
constexpr const T& const_min(const T& x, const T& y)
{
    return x < y ? x : y;
}

namespace details
{
constexpr inline bool args_or() { return false; }
template <typename... Ts>
constexpr inline bool args_or(bool x, Ts... rest)
{
    return x || args_or(rest...);
}

constexpr inline bool args_and() { return true; }
template <typename... Ts>
constexpr inline bool args_and(bool x, Ts... rest)
{
    return x && args_and(rest...);
}

template <typename T, typename Enable = void>
struct is_pod_impl : std::false_type
{
};

template <typename T>
struct is_pod_impl<T, void_t<decltype(T::is_pod)>> : std::integral_constant<bool, T::is_pod>
{
};
}

template <typename... Ts>
struct or_t : std::integral_constant<bool, details::args_or(Ts::value...)>
{
};

template <typename... Ts>
struct and_t : std::integral_constant<bool, details::args_and(Ts::value...)>
{
};

template <typename T>
struct not_t : std::integral_constant<bool, !T::value>
{
};

constexpr size_t max_size_t = size_t(-1);

template <typename... T>
using common_type = typename std::common_type<T...>::type;

template <typename T>
using result_of = typename std::result_of<T>::type;

template <bool Condition, typename Type = void>
using enable_if = typename std::enable_if<Condition, Type>::type;

template <bool Condition, typename T, typename F>
using conditional = typename std::conditional<Condition, T, F>::type;

template <typename T>
using remove_reference = typename std::remove_reference<T>::type;

template <typename T>
using remove_cv = typename std::remove_cv<T>::type;

template <typename T>
using remove_pointer = typename std::remove_pointer<T>::type;

template <typename T>
using remove_extent = typename std::remove_extent<T>::type;

template <typename T>
using remove_const = typename std::remove_const<T>::type;

template <typename T>
using underlying_type = typename std::underlying_type<T>::type;

template <typename T>
using is_pod = or_t<std::is_pod<T>, details::is_pod_impl<T>>;

template <typename T>
using is_class = std::is_class<T>;

template <typename T>
using is_const = std::is_const<T>;

template <typename T>
using is_pointer = std::is_pointer<T>;

template <typename T>
using is_array = std::is_array<T>;

template <typename T>
using is_void = std::is_void<T>;

template <typename T1, typename T2>
using is_same = std::is_same<T1, T2>;

template <typename T>
using is_template_arg = std::integral_constant<bool, std::is_integral<T>::value || std::is_enum<T>::value>;

template <typename T>
using decay = typename std::decay<T>::type;

template <typename... T>
using decay_common = decay<common_type<T...>>;

template <typename T1, typename T2 = void, typename... Ts>
constexpr size_t typeindex()
{
    return is_same<T1, T2>() ? 0 : 1 + typeindex<T1, Ts...>();
}

template <typename T>
struct compound_type_traits
{
    constexpr static size_t width      = 1;
    constexpr static size_t deep_width = width;
    using subtype                      = T;
    using deep_subtype                 = T;
    constexpr static size_t depth      = 0;
    constexpr static bool is_scalar    = true;

    template <typename U>
    using rebind = U;
    template <typename U>
    using deep_rebind = U;

    static constexpr const subtype& at(const T& value, size_t /*index*/) { return value; }
};

template <typename T>
using is_compound = std::integral_constant<bool, !compound_type_traits<decay<T>>::is_scalar>;

template <typename T>
using subtype = typename compound_type_traits<T>::subtype;

template <typename T>
using deep_subtype = typename compound_type_traits<T>::deep_subtype;

template <typename T, typename SubType>
using rebind = typename compound_type_traits<T>::template rebind<SubType>;

template <typename T, typename SubType>
using deep_rebind = typename compound_type_traits<T>::template deep_rebind<SubType>;

template <typename T>
struct compound_type_traits<std::pair<T, T>>
{
    constexpr static size_t width      = 2;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    using subtype                      = T;
    using deep_subtype                 = cometa::deep_subtype<T>;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = cometa::compound_type_traits<T>::depth + 1;

    template <typename U>
    using rebind = std::pair<U, U>;
    template <typename U>
    using deep_rebind = std::pair<cometa::deep_rebind<subtype, U>, cometa::deep_rebind<subtype, U>>;

    static constexpr const subtype& at(const std::pair<subtype, subtype>& value, size_t index)
    {
        return index == 0 ? value.first : value.second;
    }
};

namespace ops
{
struct empty
{
};
}

template <typename T, T val>
struct cval_t : ops::empty
{
    constexpr static T value                 = val;
    constexpr cval_t() noexcept              = default;
    constexpr cval_t(const cval_t&) noexcept = default;
    constexpr cval_t(cval_t&&) noexcept      = default;
    typedef T value_type;
    typedef cval_t type;
    constexpr operator value_type() const { return value; }
    constexpr value_type operator()() const { return value; }
};

template <typename T, T value>
constexpr inline T val_of(cval_t<T, value>)
{
    return value;
}

template <typename T>
constexpr inline T val_of(T value)
{
    return value;
}

template <typename T>
constexpr inline bool is_constant_val(T)
{
    return false;
}

template <typename T, T value>
constexpr inline bool is_constant_val(cval_t<T, value>)
{
    return true;
}

namespace details
{

template <typename T>
struct inherit : T
{
};

template <typename T, typename Enable = void>
struct is_inheritable_impl : std::false_type
{
};

template <typename T>
struct is_inheritable_impl<T, void_t<inherit<T>>> : std::true_type
{
};

template <typename T>
struct is_val_impl : std::false_type
{
};

template <typename T, T val>
struct is_val_impl<cval_t<T, val>> : std::true_type
{
};
}

template <typename T>
using is_inheritable = typename details::is_inheritable_impl<T>::type;

template <typename T>
using is_val_t = typename details::is_val_impl<T>::type;

template <bool val>
using cbool_t = cval_t<bool, val>;

template <int val>
using cint_t = cval_t<int, val>;

template <unsigned val>
using cuint_t = cval_t<unsigned, val>;

template <size_t val>
using csize_t = cval_t<size_t, val>;

template <typename T, T val>
constexpr cval_t<T, val> cval{};

template <bool val>
constexpr cbool_t<val> cbool{};

using cfalse_t = cbool_t<false>;
using ctrue_t  = cbool_t<true>;

constexpr ctrue_t ctrue{};
constexpr cfalse_t cfalse{};

template <int val>
constexpr cint_t<val> cint{};

template <unsigned val>
constexpr cuint_t<val> cuint{};

template <size_t val>
constexpr csize_t<val> csize{};

namespace details
{
template <size_t index, typename T, T first, T... rest>
struct get_nth : get_nth<index - 1, T, rest...>
{
};

template <typename T, T first, T... rest>
struct get_nth<0, T, first, rest...>
{
    constexpr static T value = first;
};

template <size_t index, typename... Types>
struct get_nth_type;

template <size_t index, typename first, typename... rest>
struct get_nth_type<index, first, rest...> : get_nth_type<index - 1, rest...>
{
};

template <typename first, typename... rest>
struct get_nth_type<0, first, rest...>
{
    using type = first;
};

template <size_t index>
struct get_nth_type<index>
{
};
}

template <typename T, T... values>
struct cvals_t : ops::empty
{
    using type = cvals_t<T, values...>;
    constexpr static size_t size() { return sizeof...(values); }
    template <size_t index>
    constexpr T operator[](csize_t<index>)
    {
        return get(csize<index>);
    }
    template <size_t index>
    constexpr static T get(csize_t<index> = csize_t<index>())
    {
        return details::get_nth<index, T, values...>::value;
    }
    constexpr static T front() { return get(csize<0>); }
    constexpr static T back() { return get(csize<size() - 1>); }

    static const T* begin() { return array(); }
    static const T* end() { return array() + size(); }

    static const T* array()
    {
        static const T arr[] = { values... };
        return &arr[0];
    }
    template <size_t... indices>
    constexpr cvals_t<T, details::get_nth<indices, T, values...>::value...> operator[](
        cvals_t<size_t, indices...>) const
    {
        return {};
    }
};

template <typename T>
struct cvals_t<T> : ops::empty
{
    using type = cvals_t<T>;
    constexpr static size_t size() { return 0; }
};

template <bool... values>
using cbools_t = cvals_t<bool, values...>;

template <int... values>
using cints_t = cvals_t<int, values...>;

template <char... values>
using cchars_t = cvals_t<char, values...>;

template <unsigned... values>
using cuints_t = cvals_t<unsigned, values...>;

template <size_t... values>
using csizes_t = cvals_t<size_t, values...>;

template <size_t... values>
using elements_t = cvals_t<size_t, values...>;

template <typename T, T... values>
constexpr cvals_t<T, values...> cvals{};

template <bool... vals>
constexpr cbools_t<vals...> cbools{};

constexpr cbools_t<false, true> cfalse_true{};

template <int... vals>
constexpr cints_t<vals...> cints{};

template <char... vals>
constexpr cchars_t<vals...> cchars{};

template <unsigned... vals>
constexpr cuints_t<vals...> cuints{};

template <size_t... vals>
constexpr csizes_t<vals...> csizes{};

template <size_t... vals>
constexpr elements_t<vals...> elements{};

template <typename T>
constexpr inline T csum(cvals_t<T>)
{
    return 0;
}

template <typename T, T first, T... rest>
constexpr inline T csum(cvals_t<T, first, rest...>)
{
    return first + csum(cvals<T, rest...>);
}

template <typename T>
constexpr inline T cprod(cvals_t<T>)
{
    return 1;
}

template <typename T, T first, T... rest>
constexpr inline T cprod(cvals_t<T, first, rest...>)
{
    return first * cprod(cvals<T, rest...>);
}

template <typename T>
struct ctype_t
{
    using type = T;
};

template <typename T>
using type_of = typename T::type;

template <typename T>
constexpr ctype_t<T> ctype{};

template <typename... Types>
struct ctypes_t
{
    constexpr static size_t size() { return sizeof...(Types); }

    template <size_t index>
    using nth = typename details::get_nth_type<index, Types...>::type;

    template <size_t index>
    constexpr static auto get(csize_t<index>) -> ctype_t<nth<index>>
    {
        return {};
    }
};

template <typename... Ts>
constexpr ctypes_t<Ts...> ctypes{};
namespace details
{
template <typename T1, typename T2>
struct concat_impl;

template <typename T, T... values1, T... values2>
struct concat_impl<cvals_t<T, values1...>, cvals_t<T, values2...>>
{
    using type = cvals_t<T, values1..., values2...>;
};
template <typename... types1, typename... types2>
struct concat_impl<ctypes_t<types1...>, ctypes_t<types2...>>
{
    using type = ctypes_t<types1..., types2...>;
};
}
template <typename T1, typename T2>
using concat_lists = typename details::concat_impl<T1, T2>::type;

template <typename T1, typename T2>
constexpr inline concat_lists<T1, T2> cconcat(T1, T2)
{
    return {};
}

namespace details
{

template <typename>
struct function_arguments_impl;

template <typename Ret, typename... Args>
struct function_arguments_impl<Ret (*)(Args...)>
{
    using result = Ret;
    using args   = ctypes_t<Args...>;
};

template <typename Class, typename Ret, typename... Args>
struct function_arguments_impl<Ret (Class::*)(Args...)>
{
    using result = Ret;
    using args   = ctypes_t<Args...>;
};

template <typename Class, typename Ret, typename... Args>
struct function_arguments_impl<Ret (Class::*)(Args...) const>
{
    using result = Ret;
    using args   = ctypes_t<Args...>;
};

template <typename T1, typename T2>
struct filter_impl;

template <typename T>
struct filter_impl<cvals_t<T>, cvals_t<bool>>
{
    using type = cvals_t<T>;
};

template <typename T, T value, T... values, bool flag, bool... flags>
struct filter_impl<cvals_t<T, value, values...>, cvals_t<bool, flag, flags...>>
{
    using filtered = typename filter_impl<cvals_t<T, values...>, cvals_t<bool, flags...>>::type;
    using type     = conditional<flag, concat_lists<cvals_t<T, value>, filtered>, filtered>;
};
}

template <typename Fn>
using function_arguments = typename details::function_arguments_impl<decltype(&Fn::operator())>::args;

template <typename Fn>
using function_result = typename details::function_arguments_impl<decltype(&Fn::operator())>::result;

template <typename T1, typename T2>
using cfilter_t = typename details::filter_impl<T1, T2>::type;

template <typename T, T... vals, bool... flags,
          typename Ret = cfilter_t<cvals_t<T, vals...>, cvals_t<bool, flags...>>>
constexpr inline Ret cfilter(cvals_t<T, vals...>, cvals_t<bool, flags...>)
{
    return Ret{};
}

#define CMT_UN_OP(op)                                                                                        \
    template <typename T1, T1... vals1,                                                                      \
              typename Ret = cvals_t<decltype(op std::declval<T1>()), (op vals1)...>>                        \
    constexpr inline Ret operator op(cvals_t<T1, vals1...>)                                                  \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }                                                                                                        \
    template <typename T1, T1 val1, typename Ret = cval_t<decltype(op std::declval<T1>()), (op val1)>>       \
    constexpr inline Ret operator op(cval_t<T1, val1>)                                                       \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }

#define CMT_BIN_OP(op)                                                                                       \
    template <typename T1, T1... vals1, typename T2, T2... vals2,                                            \
              typename Ret =                                                                                 \
                  cvals_t<decltype(std::declval<T1>() op std::declval<T2>()), (vals1 op vals2)...>>          \
    constexpr inline Ret operator op(cvals_t<T1, vals1...>, cvals_t<T2, vals2...>)                           \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }                                                                                                        \
    template <typename T1, T1... vals1, typename T2, T2 val2,                                                \
              typename Ret =                                                                                 \
                  cvals_t<decltype(std::declval<T1>() op std::declval<T2>()), (vals1 op val2)...>>           \
    constexpr inline Ret operator op(cvals_t<T1, vals1...>, cval_t<T2, val2>)                                \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }                                                                                                        \
    template <typename T1, T1 val1, typename T2, T2... vals2,                                                \
              typename Ret =                                                                                 \
                  cvals_t<decltype(std::declval<T1>() op std::declval<T2>()), (val1 op vals2)...>>           \
    constexpr inline Ret operator op(cval_t<T1, val1>, cvals_t<T2, vals2...>)                                \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }
namespace ops
{
// clang-format off
CMT_UN_OP(-)
CMT_UN_OP(+)
CMT_UN_OP(~)
CMT_UN_OP(!)

CMT_BIN_OP(&&)
CMT_BIN_OP(||)
CMT_BIN_OP(==)
CMT_BIN_OP(!=)
CMT_BIN_OP(<)
CMT_BIN_OP(>)
CMT_BIN_OP(<=)
CMT_BIN_OP(>=)
CMT_BIN_OP(+)
CMT_BIN_OP(-)
CMT_BIN_OP(*)
CMT_BIN_OP(/)
CMT_BIN_OP(%)
CMT_BIN_OP(<<)
CMT_BIN_OP(>>)
CMT_BIN_OP(&)
CMT_BIN_OP(|)
CMT_BIN_OP(^)
// clang-format on
}

namespace details
{
template <typename T, size_t Nsize, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl;

template <typename T, size_t Nsize, T Nstart, ptrdiff_t Nstep>
using cgen_seq = typename cvalseq_impl<T, Nsize, Nstart, Nstep>::type;

template <typename T, size_t Nsize, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl : concat_impl<cgen_seq<T, Nsize / 2, Nstart, Nstep>,
                                  cgen_seq<T, Nsize - Nsize / 2, Nstart + (Nsize / 2) * Nstep, Nstep>>
{
};

template <typename T, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl<T, 0, Nstart, Nstep> : cvals_t<T>
{
};
template <typename T, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl<T, 1, Nstart, Nstep> : cvals_t<T, static_cast<T>(Nstart)>
{
};
}

template <typename T, size_t size, T start = T(), ptrdiff_t step = 1>
using cvalseq_t = typename details::cvalseq_impl<T, size, start, step>::type;

template <typename T, T begin, T end>
constexpr cvalseq_t<T, end - begin, begin> cvalrange{};

template <size_t begin, size_t end>
constexpr cvalseq_t<size_t, end - begin, begin> csizerange{};

template <int begin, int end>
constexpr cvalseq_t<int, end - begin, begin> cintrange{};

template <unsigned begin, unsigned end>
constexpr cvalseq_t<unsigned, end - begin, begin> cuintrange{};

template <typename T, size_t size, T start = T(), ptrdiff_t step = 1>
constexpr cvalseq_t<T, size, start, step> cvalseq{};

template <size_t size, size_t start = 0, ptrdiff_t step = 1>
constexpr cvalseq_t<size_t, size, start, step> csizeseq{};

template <size_t size, int start = 0, ptrdiff_t step = 1>
constexpr cvalseq_t<int, size, start, step> cintseq{};

template <size_t size, unsigned start = 0, ptrdiff_t step = 1>
constexpr cvalseq_t<unsigned, size, start, step> cuintseq{};

template <typename... List>
using indicesfor_t = cvalseq_t<size_t, sizeof...(List), 0>;

template <typename... List>
constexpr indicesfor_t<List...> indicesfor{};

namespace details
{

template <typename Ret, typename T, typename enable = void_t<>>
struct is_returning_type_impl : std::false_type
{
};

template <typename Ret, typename Fn, typename... Args>
struct is_returning_type_impl<Ret, Fn(Args...), void_t<result_of<Fn(Args...)>>>
    : std::is_same<Ret, result_of<Fn(Args...)>>
{
};

template <typename Fn, typename Args, typename enable = void_t<>>
struct is_callable_impl : std::false_type
{
};

template <typename Fn, typename... Args>
struct is_callable_impl<Fn, ctypes_t<Args...>, void_t<result_of<Fn(Args...)>>> : std::true_type
{
};

template <typename T, typename enable = void_t<>>
struct is_enabled_impl : std::true_type
{
};

template <typename Fn>
struct is_enabled_impl<Fn, void_t<decltype(Fn::disabled)>> : std::integral_constant<bool, !Fn::disabled>
{
};

template <size_t N>
struct unique_enum_impl
{
    enum class type : size_t
    {
        value = N
    };
};
template <size_t N>
using unique_enum = typename unique_enum_impl<N>::type;

#define CMT_ENABLE_IF_IMPL(N, ...)                                                                           \
    typename ::std::enable_if<(__VA_ARGS__), ::cometa::details::unique_enum<N>>::type =                      \
        ::cometa::details::unique_enum<N>::value

#define CMT_ENABLE_IF(...) CMT_ENABLE_IF_IMPL(__LINE__, __VA_ARGS__)
}

template <typename T>
struct is_enabled : details::is_enabled_impl<T>
{
};

template <typename Fn, typename... Args>
struct is_callable : details::is_callable_impl<Fn, ctypes_t<Args...>>
{
};

template <typename Ret, typename T>
struct is_returning_type : details::is_returning_type_impl<Ret, T>
{
};

namespace details
{
template <typename Fn, CMT_ENABLE_IF(is_callable<Fn()>())>
inline auto call_if_callable(Fn&& fn)
{
    return fn();
}

template <typename Fn, CMT_ENABLE_IF(!is_callable<Fn()>())>
inline auto call_if_callable(Fn&& fn)
{
    return std::forward<Fn>(fn);
}
}

template <typename Fn, typename... Args>
inline auto bind_func(Fn&& fn, Args&&... args)
{
    return [=]() CMT_INLINE_LAMBDA { return fn(details::call_if_callable(std::forward<Args>(args))...); };
}

template <typename T>
constexpr inline bool is_even(T x)
{
    return (x % 2) == 0;
}

template <typename T>
constexpr inline bool is_odd(T x)
{
    return !is_even(x);
}

template <typename T>
constexpr inline bool is_poweroftwo(T x)
{
    return ((x != 0) && !(x & (x - 1)));
}

template <typename T>
constexpr inline unsigned ilog2(T n, unsigned p = 0)
{
    return (n <= 1) ? p : ilog2(n / 2, p + 1);
}

template <typename T>
constexpr inline T next_poweroftwo(T n)
{
    return n > 2 ? T(1) << (ilog2(n - 1) + 1) : n;
}

template <typename T>
constexpr inline T prev_poweroftwo(T n)
{
    return n > 2 ? T(1) << (ilog2(n)) : n;
}

template <typename T>
constexpr inline bool is_divisible(T x, T divisor)
{
    return x % divisor == 0;
}

template <typename T>
constexpr inline T gcd(T a)
{
    return a;
}

template <typename T>
constexpr inline T gcd(T a, T b)
{
    return a < b ? gcd(b, a) : ((a % b == 0) ? b : gcd(b, a % b));
}

template <typename T, typename... Ts>
constexpr inline T gcd(T a, T b, T c, Ts... rest)
{
    return gcd(a, gcd(b, c, rest...));
}

template <typename T>
constexpr inline T lcm(T a)
{
    return a;
}

template <typename T>
constexpr inline T lcm(T a, T b)
{
    return a * b / gcd(a, b);
}

template <typename T, typename... Ts>
constexpr inline T lcm(T a, T b, T c, Ts... rest)
{
    return lcm(a, lcm(b, c, rest...));
}

namespace details
{
template <int64_t min, int64_t max, typename... Types>
struct findinttype_impl
{
};
template <int64_t min, int64_t max, typename T, typename... Types>
struct findinttype_impl<min, max, T, Types...>
{
    using type = conditional<(std::numeric_limits<T>::min() <= min && std::numeric_limits<T>::max() >= max),
                             T, typename findinttype_impl<min, max, Types...>::type>;
};
template <int64_t min, int64_t max>
struct findinttype_impl<min, max>
{
    using type = void;
};

template <typename T>
using is_number_impl =
    std::integral_constant<bool, ((std::is_integral<T>::value) || (std::is_floating_point<T>::value)) &&
                                     !std::is_same<T, bool>::value>;
}

template <int64_t min, int64_t max>
using findinttype = typename details::findinttype_impl<min, max, uint8_t, int8_t, uint16_t, int16_t, uint32_t,
                                                       int32_t, uint64_t, int64_t>::type;

template <typename T>
using is_number = details::is_number_impl<decay<T>>;

template <typename... Ts>
using is_numbers = and_t<details::is_number_impl<decay<Ts>>...>;

namespace details
{
template <typename T>
struct identity_impl
{
    using type = T;
};

template <typename T>
constexpr size_t elementsize = sizeof(T);

template <>
constexpr size_t elementsize<void> = 1;
}

template <typename T>
using identity = typename details::identity_impl<T>::type;

struct swallow
{
    template <typename... T>
    CMT_INTRIN constexpr swallow(T&&...) noexcept
    {
    }
};

template <typename T, size_t N>
struct carray;

template <typename T>
struct carray<T, 1>
{
    constexpr carray() noexcept = default;
    constexpr carray(T val) noexcept : val(val) {}

    template <typename Fn, size_t index = 0, CMT_ENABLE_IF(is_callable<Fn, csize_t<index>>::value)>
    constexpr carray(Fn&& fn, csize_t<index> = csize_t<index>{}) noexcept
        : val(static_cast<T>(fn(csize<index>)))
    {
    }

    constexpr carray(const carray&) noexcept = default;
    constexpr carray(carray&&) noexcept      = default;
    static constexpr size_t size() noexcept { return 1; }

    template <size_t index>
    CMT_INTRIN constexpr T& get(csize_t<index>) noexcept
    {
        static_assert(index == 0, "carray: Array index is out of range");
        return val;
    }
    template <size_t index>
    CMT_INTRIN constexpr const T& get(csize_t<index>) const noexcept
    {
        static_assert(index == 0, "carray: Array index is out of range");
        return val;
    }
    template <size_t index>
    CMT_INTRIN constexpr T& get() noexcept
    {
        return get(csize<index>);
    }
    template <size_t index>
    CMT_INTRIN constexpr const T& get() const noexcept
    {
        return get(csize<index>);
    }
    constexpr const T* front() const noexcept { return val; }
    constexpr T* front() noexcept { return val; }
    constexpr const T* back() const noexcept { return val; }
    constexpr T* back() noexcept { return val; }
    constexpr const T* begin() const noexcept { return &val; }
    constexpr const T* end() const noexcept { return &val + 1; }
    constexpr T* begin() noexcept { return &val; }
    constexpr T* end() noexcept { return &val + 1; }
    constexpr const T* data() const noexcept { return begin(); }
    constexpr T* data() noexcept { return begin(); }
    constexpr bool empty() const noexcept { return false; }
    T val;
};

template <typename T, size_t N>
struct carray : carray<T, N - 1>
{
    template <typename... Ts>
    constexpr carray(T first, Ts... list) noexcept : carray<T, N - 1>(list...), val(first)
    {
        static_assert(sizeof...(list) + 1 == N, "carray: Argument count is invalid");
    }

    template <typename Fn, size_t index = N - 1>
    constexpr carray(Fn&& fn, csize_t<index> = csize_t<index>{}) noexcept
        : carray<T, N - 1>(std::forward<Fn>(fn), csize<index - 1>),
          val(static_cast<T>(fn(csize<index>)))
    {
    }

    constexpr carray() noexcept              = default;
    constexpr carray(const carray&) noexcept = default;
    constexpr carray(carray&&) noexcept      = default;
    static constexpr size_t size() noexcept { return N; }
    CMT_INTRIN constexpr T& get(csize_t<N - 1>) noexcept { return val; }
    template <size_t index>
    CMT_INTRIN constexpr T& get(csize_t<index>) noexcept
    {
        return carray<T, N - 1>::get(csize<index>);
    }
    template <size_t index>
    CMT_INTRIN constexpr T& get() noexcept
    {
        return get(csize<index>);
    }
    CMT_INTRIN constexpr const T& get(csize_t<N - 1>) const noexcept { return val; }
    template <size_t index>
    CMT_INTRIN constexpr const T& get(csize_t<index>) const noexcept
    {
        return carray<T, N - 1>::get(csize<index>);
    }
    template <size_t index>
    CMT_INTRIN constexpr const T& get() const noexcept
    {
        return get(csize<index>);
    }
    CMT_INTRIN constexpr const T* front() const noexcept { return carray<T, N - 1>::front(); }
    CMT_INTRIN constexpr T* front() noexcept { return carray<T, N - 1>::front(); }
    CMT_INTRIN constexpr const T* back() const noexcept { return val; }
    CMT_INTRIN constexpr T* back() noexcept { return val; }
    CMT_INTRIN constexpr const T* begin() const noexcept { return carray<T, N - 1>::begin(); }
    CMT_INTRIN constexpr const T* end() const noexcept { return &val + 1; }
    CMT_INTRIN constexpr T* begin() noexcept { return carray<T, N - 1>::begin(); }
    CMT_INTRIN constexpr T* end() noexcept { return &val + 1; }
    CMT_INTRIN constexpr const T* data() const noexcept { return begin(); }
    CMT_INTRIN constexpr T* data() noexcept { return begin(); }
    CMT_INTRIN constexpr bool empty() const noexcept { return false; }
private:
    T val;
};

#define CMT_FN(fn)                                                                                           \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(fn(std::declval<Args>()...)) operator()(Args&&... args) const             \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

#define CMT_ESC(...) __VA_ARGS__

#define CMT_FN_TPL(tpl_list, tpl_args, fn)                                                                   \
    template <CMT_ESC tpl_list>                                                                              \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        CMT_INLINE_MEMBER decltype(fn<CMT_ESC tpl_args>(std::declval<Args>()...)) operator()(                \
            Args&&... args) const                                                                            \
        {                                                                                                    \
            return fn<CMT_ESC tpl_args>(std::forward<Args>(args)...);                                        \
        }                                                                                                    \
    };

template <typename T>
inline auto pass_through(T&& x) noexcept
{
    return x;
}

template <typename... Ts>
inline void noop(Ts...) noexcept
{
}

template <typename T1, typename... Ts>
constexpr inline T1&& get_first(T1&& x, Ts...) noexcept
{
    return std::forward<T1>(x);
}

template <typename T1, typename T2, typename... Ts>
constexpr inline T2&& get_second(T1, T2&& x, Ts...) noexcept
{
    return std::forward<T2>(x);
}

template <typename T1, typename T2, typename T3, typename... Ts>
constexpr inline T3&& get_third(T1, T2, T3&& x, Ts...) noexcept
{
    return std::forward<T3>(x);
}
template <typename T, typename... Ts>
constexpr inline T returns(Ts...)
{
    return T();
}

CMT_FN(pass_through)
CMT_FN(noop)
CMT_FN(get_first)
CMT_FN(get_second)
CMT_FN(get_third)
CMT_FN_TPL((typename T), (T), returns)

template <typename T1, typename T2>
inline bool is_equal(const T1& x, const T2& y)
{
    return x == y;
}
template <typename T1, typename T2>
inline bool is_notequal(const T1& x, const T2& y)
{
    return x != y;
}
template <typename T1, typename T2>
inline bool is_less(const T1& x, const T2& y)
{
    return x < y;
}
template <typename T1, typename T2>
inline bool is_greater(const T1& x, const T2& y)
{
    return x > y;
}
template <typename T1, typename T2>
inline bool is_lessorequal(const T1& x, const T2& y)
{
    return x <= y;
}
template <typename T1, typename T2>
inline bool is_greaterorequal(const T1& x, const T2& y)
{
    return x >= y;
}
CMT_FN(is_equal)
CMT_FN(is_notequal)
CMT_FN(is_less)
CMT_FN(is_greater)
CMT_FN(is_lessorequal)
CMT_FN(is_greaterorequal)

namespace details
{
template <typename, typename = void>
struct has_begin_end_impl : std::false_type
{
};

template <typename T>
struct has_begin_end_impl<T, void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>>
    : std::true_type
{
};

template <typename, typename = void>
struct has_value_type_impl : std::false_type
{
};

template <typename T>
struct has_value_type_impl<T, void_t<typename T::value_type>> : std::true_type
{
};

template <typename, typename = void>
struct has_data_size_impl : std::false_type
{
};

template <typename T>
struct has_data_size_impl<T, void_t<decltype(std::declval<T>().size()), decltype(std::declval<T>().data())>>
    : std::true_type
{
};

template <typename, typename Fallback, typename = void>
struct value_type_impl
{
    using type = Fallback;
};

template <typename T, typename Fallback>
struct value_type_impl<T, Fallback, void_t<typename T::value_type>>
{
    using type = typename T::value_type;
};
}

template <typename T>
using has_begin_end = details::has_begin_end_impl<decay<T>>;

template <typename T>
using has_data_size = details::has_data_size_impl<decay<T>>;

template <typename T>
using value_type_of = typename decay<T>::value_type;

template <typename T, typename Fn>
CMT_INTRIN void cforeach(cvals_t<T>, Fn&&)
{
}

template <typename T, T v0, T... values, typename Fn>
CMT_INTRIN void cforeach(cvals_t<T, v0, values...>, Fn&& fn)
{
    fn(cval<T, v0>);
    cforeach(cvals_t<T, values...>(), std::forward<Fn>(fn));
}

template <typename T, typename Fn, CMT_ENABLE_IF(has_begin_end<T>::value)>
CMT_INTRIN void cforeach(T&& list, Fn&& fn)
{
    for (const auto& v : list)
    {
        fn(v);
    }
}

template <typename T, size_t N, typename Fn>
CMT_INTRIN void cforeach(const T (&array)[N], Fn&& fn)
{
    for (size_t i = 0; i < N; i++)
    {
        fn(array[i]);
    }
}

namespace details
{
template <typename... Ts, typename Fn, size_t... indices>
CMT_INTRIN void cforeach_tuple_impl(const std::tuple<Ts...>& tuple, Fn&& fn, csizes_t<indices...>)
{
    swallow{ (fn(std::get<indices>(tuple)), void(), 0)... };
}
template <typename T0, typename... types, typename Fn, size_t... indices>
CMT_INTRIN void cforeach_types_impl(ctypes_t<T0, types...>, Fn&& fn, csizes_t<indices...>)
{
    swallow{ (fn(ctype<type_of<details::get_nth_type<indices, T0, types...>>>), void(), 0)... };
}
}

template <typename... Ts, typename Fn>
CMT_INTRIN void cforeach(ctypes_t<Ts...> types, Fn&& fn)
{
    details::cforeach_types_impl(types, std::forward<Fn>(fn), csizeseq<sizeof...(Ts)>);
}

template <typename... Ts, typename Fn>
CMT_INTRIN void cforeach(const std::tuple<Ts...>& tuple, Fn&& fn)
{
    details::cforeach_tuple_impl(tuple, std::forward<Fn>(fn), csizeseq<sizeof...(Ts)>);
}

template <typename A0, typename A1, typename Fn>
CMT_INTRIN void cforeach(A0&& a0, A1&& a1, Fn&& fn)
{
    cforeach(std::forward<A0>(a0),
             [&](auto v0) { cforeach(std::forward<A1>(a1), [&](auto v1) { fn(v0, v1); }); });
}

template <typename A0, typename A1, typename A2, typename Fn>
CMT_INTRIN void cforeach(A0&& a0, A1&& a1, A2&& a2, Fn&& fn)
{
    cforeach(std::forward<A0>(a0), [&](auto v0) {
        cforeach(std::forward<A1>(a1),
                 [&](auto v1) { cforeach(std::forward<A2>(a2), [&](auto v2) { fn(v0, v1, v2); }); });
    });
}

template <typename T, typename Fn, typename DefFn = fn_noop, typename CmpFn = fn_is_equal>
CMT_INTRIN decltype(auto) cswitch(cvals_t<T>, identity<T>, Fn&&, DefFn&& deffn = DefFn(), CmpFn&& = CmpFn())
{
    return deffn();
}

template <typename T, T v0, T... values, typename Fn, typename DefFn = fn_noop, typename CmpFn = fn_is_equal>
CMT_INTRIN decltype(auto) cswitch(cvals_t<T, v0, values...>, identity<T> value, Fn&& fn,
                                  DefFn&& deffn = DefFn(), CmpFn&& cmpfn = CmpFn())
{
    if (cmpfn(value, v0))
    {
        return fn(cval<T, v0>);
    }
    else
    {
        return cswitch(cvals_t<T, values...>(), value, std::forward<Fn>(fn), std::forward<DefFn>(deffn),
                       std::forward<CmpFn>(cmpfn));
    }
}

template <typename TrueFn, typename FalseFn = fn_noop>
CMT_INTRIN decltype(auto) cif(cbool_t<true>, TrueFn&& truefn, FalseFn&& = FalseFn())
{
    return truefn(cbool<true>);
}

template <typename TrueFn, typename FalseFn = fn_noop>
CMT_INTRIN decltype(auto) cif(cbool_t<false>, TrueFn&&, FalseFn&& falsefn = FalseFn())
{
    return falsefn(cbool<false>);
}

template <typename T, T start, T stop, typename BodyFn>
CMT_INTRIN decltype(auto) cfor(cval_t<T, start>, cval_t<T, stop>, BodyFn&& bodyfn)
{
    return cforeach(cvalrange<T, start, stop>, std::forward<BodyFn>(bodyfn));
}

namespace details
{

template <typename T, typename Fn1, typename Fn2, typename... Fns>
inline decltype(auto) cmatch_impl(T&& value, Fn1&& first, Fn2&& second, Fns&&... rest);
template <typename T, typename Fn, typename... Ts>
inline decltype(auto) cmatch_impl(T&& value, Fn&& last);

template <typename T, typename Fn, typename... Fns>
inline decltype(auto) cmatch_impl2(cbool_t<true>, T&& value, Fn&& fn, Fns&&...)
{
    return fn(std::forward<T>(value));
}

template <typename T, typename Fn, typename... Fns>
inline decltype(auto) cmatch_impl2(cbool_t<false>, T&& value, Fn&&, Fns&&... rest)
{
    return cmatch_impl(std::forward<T>(value), std::forward<Fns>(rest)...);
}

template <typename T, typename Fn1, typename Fn2, typename... Fns>
inline decltype(auto) cmatch_impl(T&& value, Fn1&& first, Fn2&& second, Fns&&... rest)
{
    using first_arg        = typename function_arguments<Fn1>::template nth<0>;
    constexpr bool is_same = std::is_same<decay<T>, decay<first_arg>>::value;
    return cmatch_impl2(cbool<is_same>, std::forward<T>(value), std::forward<Fn1>(first),
                        std::forward<Fn2>(second), std::forward<Fns>(rest)...);
}

template <typename T, typename Fn, typename... Ts>
inline decltype(auto) cmatch_impl(T&& value, Fn&& last)
{
    return last(std::forward<T>(value));
}
}

template <typename T, typename Fn, typename... Args>
inline decltype(auto) cmatch(T&& value, Fn&& fn, Args... args)
{
    return details::cmatch_impl(std::forward<T>(value), std::forward<Fn>(fn), std::forward<Args>(args)...);
}

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
    CMT_INTRIN Result operator()(Args... args) const
    {
        if (fn)
            return (*fn)(args...);
        else
            return details::return_val<Result>();
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

template <typename T, T... values>
inline size_t cfind(cvals_t<T, values...>, identity<T> value)
{
    static const T temp[] = { values... };
    return static_cast<size_t>(
        std::distance(std::begin(temp), std::find(std::begin(temp), std::end(temp), value)));
}

template <typename Fn, typename... Args>
CMT_NOINLINE static result_of<Fn(Args...)> noinline(Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn>
struct fn_noinline
{
    template <typename... Args>
    CMT_INTRIN result_of<Fn(Args...)> operator()(Args&&... args) const
    {
        return noinline(Fn{}, std::forward<Args>(args)...);
    }
};

template <typename... Args, typename Fn, typename Ret = decltype(std::declval<Fn>()(std::declval<Args>()...)),
          typename NonMemFn = Ret (*)(Fn*, Args...)>
CMT_INTRIN NonMemFn make_nonmember(const Fn&)
{
    return [](Fn* fn, Args... args) -> Ret { return fn->operator()(std::forward<Args>(args)...); };
}

using type_id_t = const void*;

namespace details
{

constexpr inline size_t strlen(const char* str) { return *str ? 1 + cometa::details::strlen(str + 1) : 0; }

template <size_t... indices, size_t Nout = 1 + sizeof...(indices)>
constexpr inline std::array<char, Nout> gettypename_impl(const char* str, csizes_t<indices...>)
{
    std::array<char, Nout> arr{ { str[indices]..., 0 } };
    return arr;
}

template <typename T>
constexpr inline const void* typeident_impl() noexcept
{
    return type_id_t(&typeident_impl<T>);
}
}

/**
 * @brief Gets the fully qualified name of the type, including namespace and template parameters (if any)
 * @tparam T    type
 * @return      name of the type
 */
template <typename T>
inline const char* type_name() noexcept
{
    constexpr size_t prefix  = details::strlen("const char *cometa::type_name() [T = ");
    constexpr size_t postfix = details::strlen("]");
    constexpr size_t length  = sizeof(CMT_FUNC_SIGNATURE) - 1 - prefix - postfix;
    static const std::array<char, length + 1> name =
        details::gettypename_impl(CMT_FUNC_SIGNATURE + prefix, csizeseq<length>);
    return name.data();
}

/**
 * @brief Gets the fully qualified name of the type, including namespace and template parameters (if any)
 * @param x      value of specific type
 * @return      name of the type
 */
template <typename T>
inline const char* type_name(T x) noexcept
{
    (void)x;
    return type_name<T>();
}

/**
 * @brief Gets unique value associated with the type
 * @tparam T    type
 * @return      value of type that supports operator== and operator!=
 */
template <typename T>
constexpr inline type_id_t ctypeid()
{
    return details::typeident_impl<T>();
}
/**
 * @brief Gets unique value associated with the type
 * @param x     value of specific type
 * @return      value of type that supports operator== and operator!=
 */
template <typename T>
constexpr inline type_id_t ctypeid(T x)
{
    (void)x;
    return details::typeident_impl<T>();
}

template <typename T>
struct array_ref
{
public:
    using value_type             = T;
    using pointer                = value_type*;
    using const_pointer          = const value_type*;
    using reference              = value_type&;
    using const_reference        = const value_type&;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<pointer>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;

    constexpr array_ref() noexcept : m_data(nullptr), m_size(0) {}
    constexpr array_ref(const array_ref&) noexcept = default;
    constexpr array_ref(array_ref&&) noexcept      = default;
    constexpr array_ref& operator=(const array_ref&) noexcept = default;
    constexpr array_ref& operator=(array_ref&&) noexcept = default;

    template <size_t N>
    constexpr array_ref(value_type (&arr)[N]) noexcept : m_data(arr), m_size(N)
    {
    }
    template <size_t N>
    constexpr array_ref(const std::array<T, N>& arr) noexcept : m_data(arr.data()), m_size(N)
    {
    }
    template <size_t N>
    constexpr array_ref(std::array<T, N>& arr) noexcept : m_data(arr.data()), m_size(N)
    {
    }
    template <typename... Ts>
    constexpr array_ref(const std::vector<T, Ts...>& vec) noexcept : m_data(vec.data()), m_size(vec.size())
    {
    }
    template <typename... Ts, CMT_ENABLE_IF(sizeof...(Ts), is_const<T>::value)>
    constexpr array_ref(const std::vector<remove_const<T>, Ts...>& vec) noexcept : m_data(vec.data()),
                                                                                   m_size(vec.size())
    {
    }
    template <typename... Ts>
    constexpr array_ref(std::vector<T, Ts...>& vec) noexcept : m_data(vec.data()), m_size(vec.size())
    {
    }
    template <typename InputIter>
    constexpr array_ref(InputIter first, InputIter last) noexcept : m_data(std::addressof(*first)),
                                                                    m_size(std::distance(first, last))
    {
    }
    constexpr array_ref(T* data, size_type size) noexcept : m_data(data), m_size(size) {}

    constexpr reference front() const noexcept { return m_data[0]; }
    constexpr reference back() const noexcept { return m_data[m_size - 1]; }
    constexpr iterator begin() const noexcept { return m_data; }
    constexpr iterator end() const noexcept { return m_data + m_size; }
    constexpr const_iterator cbegin() const noexcept { return m_data; }
    constexpr const_iterator cend() const noexcept { return m_data + m_size; }
    constexpr pointer data() const noexcept { return m_data; }
    constexpr std::size_t size() const noexcept { return m_size; }
    constexpr bool empty() const noexcept { return !m_size; }
    constexpr reference operator[](std::size_t index) const { return m_data[index]; }

private:
    pointer m_data;
    size_type m_size;
};

template <typename T>
constexpr inline T choose_const()
{
    static_assert(sizeof(T) != 0, "T not found in the list of template arguments");
    return T();
}

/**
 * Selects constant of the specific type
 * @code
 * CHECK( choose_const<f32>( 32.0f, 64.0 ) == 32.0f );
 * CHECK( choose_const<f64>( 32.0f, 64.0 ) == 64.0 );
 * @endcode
 */
template <typename T, typename C1, typename... Cs>
constexpr inline T choose_const(C1 c1, Cs... constants)
{
    return std::is_same<T, C1>::value ? static_cast<T>(c1) : choose_const<T>(constants...);
}

template <typename T, std::size_t size>
inline array_ref<T> make_array_ref(T (&data)[size])
{
    return array_ref<T>(data);
}

template <typename T>
inline array_ref<T> make_array_ref(T* data, std::size_t size)
{
    return array_ref<T>(data, data + size);
}

template <typename Container, CMT_ENABLE_IF(has_data_size<Container>::value),
          typename T = remove_pointer<decltype(std::declval<Container>().data())>>
inline array_ref<T> make_array_ref(Container& cont)
{
    return array_ref<T>(cont.data(), cont.size());
}

template <typename Container, CMT_ENABLE_IF(has_data_size<Container>::value),
          typename T = remove_pointer<decltype(std::declval<Container>().data())>>
inline array_ref<const T> make_array_ref(const Container& cont)
{
    return array_ref<const T>(cont.data(), cont.size());
}

template <typename T>
inline array_ref<T> make_array_ref(std::vector<T>& cont)
{
    return array_ref<T>(cont.data(), cont.size());
}
template <typename T>
inline array_ref<const T> make_array_ref(const std::vector<T>& cont)
{
    return array_ref<const T>(cont.data(), cont.size());
}

template <typename Type, typename ErrEnum, ErrEnum OkValue = static_cast<ErrEnum>(0)>
struct result
{
    using value_type      = Type;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;

    using error_type = ErrEnum;

    constexpr static error_type ok_value = OkValue;

    constexpr result(const result&)     = default;
    constexpr result(result&&) noexcept = default;

    constexpr result(ErrEnum error) noexcept : m_error(error) {}

    template <typename ValueInit, CMT_ENABLE_IF(std::is_constructible<value_type, ValueInit>::value)>
    constexpr result(ValueInit&& value) noexcept : m_value(std::forward<ValueInit>(value)), m_error(OkValue)
    {
    }

    constexpr result(const Type& value) noexcept : m_value(value), m_error(OkValue) {}
    constexpr result(Type&& value) noexcept : m_value(std::move(value)), m_error(OkValue) {}

    constexpr explicit operator bool() const { return m_error == OkValue; }
    constexpr const_reference operator*() const { return m_value; }
    constexpr reference operator*() { return m_value; }
    constexpr const_pointer operator->() const { return &m_value; }
    constexpr pointer operator->() { return &m_value; }

    constexpr const_reference value() const { return m_value; }
    constexpr reference value() { return m_value; }
    constexpr ErrEnum error() const { return m_error; }
    constexpr bool ok() const { return m_error == OkValue; }
private:
    Type m_value;
    ErrEnum m_error;
};

template <typename Tfrom>
struct autocast_impl
{
    const Tfrom value;
    template <typename T>
    CMT_INTRIN constexpr operator T() const noexcept
    {
        return static_cast<T>(value);
    }
};

template <typename Tfrom>
CMT_INTRIN constexpr autocast_impl<Tfrom> autocast(const Tfrom& value) noexcept
{
    return { value };
}

inline void stop_constexpr() {}

namespace details
{
template <typename T, typename = void>
struct signed_type_impl
{
    using type = T;
};
template <typename T>
struct signed_type_impl<T, void_t<enable_if<std::is_unsigned<T>::value>>>
{
    using type = findinttype<std::numeric_limits<T>::min(), std::numeric_limits<T>::max()>;
};
}

template <typename T>
using signed_type = typename details::signed_type_impl<T>::type;

template <typename T>
struct range
{
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using diff_type       = decltype(std::declval<T>() - std::declval<T>());

    constexpr range(value_type begin, value_type end, diff_type step) noexcept : value_begin(begin),
                                                                                 value_end(end),
                                                                                 step(step)
    {
    }

    struct iterator
    {
        value_type value;
        diff_type step;
        const_reference operator*() const { return value; }
        const_pointer operator->() const { return &value; }
        iterator& operator++()
        {
            value += step;
            return *this;
        }
        iterator operator++(int)
        {
            iterator copy = *this;
            ++(*this);
            return copy;
        }
        bool operator!=(const iterator& other) const
        {
            return step > 0 ? value < other.value : value > other.value;
        }
    };
    value_type value_begin;
    value_type value_end;
    diff_type step;
    iterator begin() const { return iterator{ value_begin, step }; }
    iterator end() const { return iterator{ value_end, step }; }
};

template <typename T>
range<T> make_range(T begin, T end)
{
    return range<T>(begin, end, end > begin ? 1 : -1);
}

template <typename T, typename diff_type = decltype(std::declval<T>() - std::declval<T>())>
range<T> make_range(T begin, T end, diff_type step)
{
    return range<T>(begin, end, step);
}

template <typename T>
struct named_arg
{
    T value;
    const char* name;
};

struct named
{
    constexpr named(const char* name) noexcept : name(name) {}

    template <typename T>
    constexpr named_arg<T> operator=(T&& value)
    {
        return named_arg<T>{ std::forward<T>(value), name };
    }
    const char* name;
};

inline named operator""_arg(const char* name, size_t) { return name; }
}

#pragma clang diagnostic pop
