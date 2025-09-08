/** @addtogroup meta
 *  @{
 */
#pragma once

#include "cident.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <random>
#include <type_traits>
#include <tuple>
#include <utility>

KFR_PRAGMA_GNU(GCC diagnostic push)
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wshadow")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wpragmas")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wunknown-warning-option")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wmaybe-uninitialized")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wdeprecated-declarations")

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4814))
KFR_PRAGMA_MSVC(warning(disable : 4308))
KFR_PRAGMA_MSVC(warning(disable : 4014))

namespace kfr
{

using std::ptrdiff_t;
using std::size_t;

using pvoid      = void*;
using pconstvoid = const void*;

constexpr size_t max_size_t = size_t(-1);

template <typename T1, typename T2>
using or_type = std::conditional_t<std::is_same_v<T1, void>, T2, T1>;

template <typename T1, typename T2 = void, typename... Ts>
constexpr size_t typeindex() noexcept
{
    return std::is_same_v<T1, T2>() ? 0 : 1 + typeindex<T1, Ts...>();
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

    KFR_MEM_INTRINSIC static constexpr const subtype& at(const T& value, size_t /*index*/) noexcept
    {
        return value;
    }
};

template <typename T>
constexpr size_t widthof(T) noexcept
{
    return compound_type_traits<T>::width;
}
template <typename T>
constexpr size_t widthof() noexcept
{
    return compound_type_traits<std::decay_t<T>>::width;
}

template <typename T>
constexpr inline bool is_compound_type = !compound_type_traits<std::decay_t<T>>::is_scalar;

template <typename T>
using subtype = typename compound_type_traits<T>::subtype;

template <typename T>
using deep_subtype = typename compound_type_traits<T>::deep_subtype;

template <typename T>
struct compound_type_traits<std::pair<T, T>>
{
    constexpr static size_t width      = 2;
    constexpr static size_t deep_width = width * compound_type_traits<T>::width;
    using subtype                      = T;
    using deep_subtype                 = kfr::deep_subtype<T>;
    constexpr static bool is_scalar    = false;
    constexpr static size_t depth      = kfr::compound_type_traits<T>::depth + 1;

    template <typename U>
    using rebind = std::pair<U, U>;
    template <typename U>
    using deep_rebind = std::pair<typename compound_type_traits<subtype>::template deep_rebind<U>,
                                  typename compound_type_traits<subtype>::template deep_rebind<U>>;

    KFR_MEM_INTRINSIC static constexpr const subtype& at(const std::pair<subtype, subtype>& value,
                                                         size_t index) noexcept
    {
        return index == 0 ? value.first : value.second;
    }
};

template <typename T, T val>
struct cval_t
{
    constexpr static T value = val;
    constexpr KFR_MEM_INTRINSIC cval_t() noexcept {}
    constexpr KFR_MEM_INTRINSIC cval_t(const cval_t&) noexcept = default;
    constexpr KFR_MEM_INTRINSIC cval_t(cval_t&&) noexcept      = default;
    using value_type                                           = T;
    using type                                                 = cval_t;
    constexpr KFR_MEM_INTRINSIC operator value_type() const noexcept { return value; }
    constexpr KFR_MEM_INTRINSIC value_type operator()() const noexcept { return value; }
};

template <typename T, T value>
constexpr KFR_INTRINSIC T val_of(cval_t<T, value>) noexcept
{
    return value;
}

template <typename T>
constexpr KFR_INTRINSIC T val_of(T value) noexcept
{
    return value;
}

#define KFR_CVAL(...) (decltype(__VA_ARGS__)::value)

template <typename T>
constexpr KFR_INTRINSIC bool is_constant_val(T) noexcept
{
    return false;
}

template <typename T, T value>
constexpr KFR_INTRINSIC bool is_constant_val(cval_t<T, value>) noexcept
{
    return true;
}

namespace details
{

template <typename T>
struct is_val_impl : std::false_type
{
};

template <typename T, T val>
struct is_val_impl<cval_t<T, val>> : std::true_type
{
};
} // namespace details

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

using cfalse_t = cbool_t<false>;
using ctrue_t  = cbool_t<true>;

constexpr inline ctrue_t ctrue{};
constexpr inline cfalse_t cfalse{};

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

template <size_t index, typename T>
struct get_nth_e;

template <size_t index, typename... Types>
struct get_nth_type
{
#if defined(__clang__) && __has_builtin(__type_pack_element)
    using type = __type_pack_element<index, Types...>;
#elif defined(__GNUC__) && __has_builtin(__builtin_type_pack_element)
    using type = __builtin_type_pack_element(index, Types...);
#else
    using type = std::tuple_element_t<index, std::tuple<Types...>>;
#endif
};

} // namespace details

template <typename T, T... values>
struct cvals_t
{
    constexpr KFR_MEM_INTRINSIC cvals_t() noexcept = default;

    using type = cvals_t<T, values...>;
    constexpr KFR_MEM_INTRINSIC static size_t size() noexcept { return sizeof...(values); }
    template <size_t index>
    constexpr KFR_MEM_INTRINSIC T operator[](csize_t<index>) const noexcept
    {
        return get(csize_t<index>());
    }
    template <size_t index>
    constexpr KFR_MEM_INTRINSIC static T get(csize_t<index> = csize_t<index>()) noexcept
    {
        return details::get_nth<index, T, values...>::value;
    }
    constexpr KFR_MEM_INTRINSIC static T front() noexcept { return get(csize_t<0>()); }
    constexpr KFR_MEM_INTRINSIC static T back() noexcept { return get(csize_t<size() - 1>()); }

    static KFR_MEM_INTRINSIC const T* begin() noexcept { return array(); }
    static KFR_MEM_INTRINSIC const T* end() noexcept { return array() + size(); }

    static KFR_MEM_INTRINSIC const T* array() noexcept
    {
        static const T arr[] = { values... };
        return &arr[0];
    }
    template <size_t... indices>
    constexpr KFR_MEM_INTRINSIC cvals_t<T, details::get_nth_e<indices, type>::value...> operator[](
        cvals_t<size_t, indices...>) const noexcept
    {
        return {};
    }

    // MSVC requires static_cast<T> here:
    template <typename Fn>
    constexpr KFR_MEM_INTRINSIC auto map(Fn&&) const noexcept -> cvals_t<T, static_cast<T>(Fn()(values))...>
    {
        return {};
    }

    constexpr KFR_MEM_INTRINSIC bool equal(cvals_t<T, values...>) const noexcept { return true; }
    template <T... values2>
    constexpr KFR_MEM_INTRINSIC bool equal(cvals_t<T, values2...>) const noexcept
    {
        return false;
    }

    template <T... values2>
    constexpr KFR_MEM_INTRINSIC bool notequal(cvals_t<T, values...> ind) const noexcept
    {
        return !equal(ind);
    }
};

template <typename T>
struct cvals_t<T>
{
    using type = cvals_t<T>;
    constexpr KFR_MEM_INTRINSIC static size_t size() noexcept { return 0; }

    static KFR_MEM_INTRINSIC const T* array() noexcept { return nullptr; }
};

template <typename T, bool... flags, T... values1, T... values2>
constexpr cvals_t<T, (flags ? values1 : values2)...> select(cvals_t<bool, flags...>, cvals_t<T, values1...>,
                                                            cvals_t<T, values2...>) noexcept
{
    return {};
}

namespace details
{
template <size_t index, typename T, T... vals>
struct get_nth_e<index, cvals_t<T, vals...>>
{
    constexpr static T value = get_nth<index, T, vals...>::value;
};
} // namespace details

template <bool... values>
using cbools_t = cvals_t<bool, values...>;

constexpr inline cbools_t<false, true> cfalse_true{};

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

template <typename T>
constexpr KFR_INTRINSIC T csum(cvals_t<T> = cvals_t<T>()) noexcept
{
    return 0;
}

template <typename T, T first, T... rest>
constexpr KFR_INTRINSIC T csum(cvals_t<T, first, rest...> = cvals_t<T, first, rest...>()) noexcept
{
    return first + csum(cvals_t<T, rest...>());
}

template <typename T>
constexpr KFR_INTRINSIC T cprod(cvals_t<T>) noexcept
{
    return 1;
}

template <typename T, T first, T... rest>
constexpr KFR_INTRINSIC T cprod(cvals_t<T, first, rest...>) noexcept
{
    return first * cprod(cvals_t<T, rest...>());
}

template <typename T>
struct ctype_t
{
#ifdef KFR_COMPILER_INTEL
    constexpr ctype_t() noexcept               = default;
    constexpr ctype_t(const ctype_t&) noexcept = default;
#endif
    using type = T;
};

template <typename T>
using type_of = typename T::type;

template <typename... Types>
struct ctypes_t
{
    constexpr static size_t size() noexcept { return sizeof...(Types); }

    template <size_t index>
    using nth = typename details::get_nth_type<index, Types...>::type;

    template <size_t index>
    constexpr static auto get(csize_t<index>) noexcept -> ctype_t<nth<index>>
    {
        return {};
    }
};

namespace details
{
template <typename T1, typename... Ts>
struct concat_impl;

template <typename T>
struct concat_impl<T>
{
    using type = T;
};

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

template <typename T1, typename T2, typename T3, typename... Ts>
struct concat_impl<T1, T2, T3, Ts...>
{
    using type = typename concat_impl<typename concat_impl<T1, T2>::type, T3, Ts...>::type;
};

} // namespace details
template <typename T1, typename... Ts>
using concat_lists = typename details::concat_impl<std::decay_t<T1>, std::decay_t<Ts>...>::type;

template <typename T1, typename... Ts>
constexpr KFR_INTRINSIC concat_lists<T1, Ts...> cconcat(T1, Ts...) noexcept
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
    using type     = std::conditional_t<flag, concat_lists<cvals_t<T, value>, filtered>, filtered>;
};
} // namespace details

template <typename Fn>
using function_arguments = typename details::function_arguments_impl<decltype(&Fn::operator())>::args;

template <typename Fn>
using function_result = typename details::function_arguments_impl<decltype(&Fn::operator())>::result;

template <typename T1, typename T2>
using cfilter_t = typename details::filter_impl<std::decay_t<T1>, std::decay_t<T2>>::type;

template <typename T, T... vals, bool... flags,
          typename Ret = cfilter_t<cvals_t<T, vals...>, cvals_t<bool, flags...>>>
constexpr KFR_INTRINSIC Ret cfilter(cvals_t<T, vals...>, cvals_t<bool, flags...>) noexcept
{
    return Ret{};
}

#define KFR_UN_OP(op)                                                                                        \
    template <typename T1, T1... vals1,                                                                      \
              typename Ret = cvals_t<decltype(op std::declval<T1>()), (op vals1)...>>                        \
    constexpr KFR_INTRINSIC Ret operator op(cvals_t<T1, vals1...>) noexcept                                  \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }                                                                                                        \
    template <typename T1, T1 val1, typename Ret = cval_t<decltype(op std::declval<T1>()), (op val1)>>       \
    constexpr KFR_INTRINSIC Ret operator op(cval_t<T1, val1>) noexcept                                       \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }

#define KFR_BIN_OP(op)                                                                                       \
    template <typename T1, T1... vals1, typename T2, T2... vals2,                                            \
              typename Ret =                                                                                 \
                  cvals_t<decltype(std::declval<T1>() op std::declval<T2>()), (vals1 op vals2)...>>          \
    constexpr KFR_INTRINSIC Ret operator op(cvals_t<T1, vals1...>, cvals_t<T2, vals2...>) noexcept           \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }                                                                                                        \
    template <typename T1, T1... vals1, typename T2, T2 val2,                                                \
              typename Ret =                                                                                 \
                  cvals_t<decltype(std::declval<T1>() op std::declval<T2>()), (vals1 op val2)...>>           \
    constexpr KFR_INTRINSIC Ret operator op(cvals_t<T1, vals1...>, cval_t<T2, val2>) noexcept                \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }                                                                                                        \
    template <typename T1, T1 val1, typename T2, T2... vals2,                                                \
              typename Ret =                                                                                 \
                  cvals_t<decltype(std::declval<T1>() op std::declval<T2>()), (val1 op vals2)...>>           \
    constexpr KFR_INTRINSIC Ret operator op(cval_t<T1, val1>, cvals_t<T2, vals2...>) noexcept                \
    {                                                                                                        \
        return Ret{};                                                                                        \
    }

// clang-format off
KFR_UN_OP(-)
KFR_UN_OP(+)
KFR_UN_OP(~)
KFR_UN_OP(!)

KFR_BIN_OP(&&)
KFR_BIN_OP(||)
KFR_BIN_OP(==)
KFR_BIN_OP(!=)
KFR_BIN_OP(<)
KFR_BIN_OP(>)
KFR_BIN_OP(<=)
KFR_BIN_OP(>=)
KFR_BIN_OP(+)
KFR_BIN_OP(-)
KFR_BIN_OP(*)
KFR_BIN_OP(/)
KFR_BIN_OP(%)
KFR_BIN_OP(<<)
KFR_BIN_OP(>>)
KFR_BIN_OP(&)
KFR_BIN_OP(|)
KFR_BIN_OP(^)
// clang-format on

namespace details
{

template <typename T, size_t Nsize, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl
    : concat_impl<typename cvalseq_impl<T, Nsize / 2, Nstart, Nstep>::type,
                  typename cvalseq_impl<T, Nsize - Nsize / 2,
                                        static_cast<T>(Nstart + static_cast<ptrdiff_t>(Nsize / 2) * Nstep),
                                        Nstep>::type>
{
};

template <typename T, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl<T, 0, Nstart, Nstep> : cvals_t<T>
{
};
template <typename T, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl<T, 1, Nstart, Nstep> : cvals_t<T, Nstart>
{
};
template <typename T, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl<T, 2, Nstart, Nstep> : cvals_t<T, Nstart, static_cast<T>(Nstart + Nstep)>
{
};

template <typename T, T Nstart, ptrdiff_t Nstep>
struct cvalseq_impl<T, 4, Nstart, Nstep>
    : cvals_t<T, static_cast<T>(Nstart), static_cast<T>(Nstart + Nstep),
              static_cast<T>(Nstart + Nstep + Nstep), static_cast<T>(Nstart + Nstep + Nstep + Nstep)>
{
};

template <typename T1, typename T2>
struct scale_impl;

template <size_t... Args1, size_t... Args2>
struct scale_impl<csizes_t<Args1...>, csizes_t<Args2...>>
{
    constexpr static size_t count1 = sizeof...(Args1);
    constexpr static size_t count2 = sizeof...(Args2);
    using type                     = csizes_t<>;
};

} // namespace details

template <typename T, size_t size, T start = T(), ptrdiff_t step = 1>
using cvalseq_t = typename details::cvalseq_impl<T, size, start, step>::type;

template <size_t size, size_t start = 0, ptrdiff_t step = 1>
using csizeseq_t = cvalseq_t<size_t, size, start, step>;

template <typename... List>
using indicesfor_t = cvalseq_t<size_t, sizeof...(List), 0>;

template <size_t group, size_t... indices, size_t N = group * sizeof...(indices)>
constexpr KFR_INTRINSIC auto scale(csizes_t<indices...>) noexcept
{
    using Tlist = typename details::concat_impl<csizeseq_t<group, group * indices>...>::type;
    return Tlist{};
}

template <size_t group, size_t... indices, size_t N = group * sizeof...(indices)>
constexpr KFR_INTRINSIC auto scale() noexcept
{
    using Tlist = typename details::concat_impl<csizeseq_t<group, group * indices>...>::type;
    return Tlist{};
}

namespace details
{

template <typename Ret, typename T, typename enable = std::void_t<>>
struct is_returning_type_impl : std::false_type
{
};

template <typename Ret, typename Fn, typename... Args>
struct is_returning_type_impl<Ret, Fn(Args...), std::void_t<std::invoke_result_t<Fn, Args...>>>
    : std::is_same<Ret, std::invoke_result_t<Fn, Args...>>
{
};

template <typename Fn, typename Args, typename enable = std::void_t<>>
struct is_callable_impl : std::false_type
{
};

template <typename Fn, typename... Args>
struct is_callable_impl<Fn, ctypes_t<Args...>, std::void_t<std::invoke_result_t<Fn, Args...>>>
    : std::true_type
{
};

template <typename T, typename enable = std::void_t<>>
struct is_enabled_impl : std::true_type
{
};

template <typename Fn>
struct is_enabled_impl<Fn, std::void_t<decltype(Fn::disabled)>> : std::integral_constant<bool, !Fn::disabled>
{
};

template <int N>
struct unique_enum_impl
{
    enum type : int
    {
        value = N
    };
};

#if defined KFR_COMPILER_MSVC && !defined KFR_COMPILER_CLANG
#define KFR_ENABLE_IF_IMPL(N, ...)                                                                           \
    bool enable_ = (__VA_ARGS__), typename enabled_ = typename ::std::enable_if<enable_>::type,              \
         typename kfr::details::unique_enum_impl<N>::type dummy_ =                                           \
             ::kfr::details::unique_enum_impl<N>::value

#else
#define KFR_ENABLE_IF_IMPL(N, ...)                                                                           \
    typename ::std::enable_if<(__VA_ARGS__), typename ::kfr::details::unique_enum_impl<N>::type>::type =     \
        ::kfr::details::unique_enum_impl<N>::value

#endif
#define KFR_ENABLE_IF(...) KFR_ENABLE_IF_IMPL(__LINE__, __VA_ARGS__)
} // namespace details

template <typename T>
constexpr inline bool is_enabled = details::is_enabled_impl<T>::value;

namespace details
{
template <typename Fn, KFR_ENABLE_IF(std::is_invocable_v<Fn>)>
KFR_INTRINSIC auto call_if_callable(Fn&& fn) noexcept
{
    return fn();
}

template <typename Fn, KFR_ENABLE_IF(!std::is_invocable_v<Fn>)>
KFR_INTRINSIC auto call_if_callable(Fn&& fn) noexcept
{
    return std::forward<Fn>(fn);
}
} // namespace details

template <typename Fn, typename... Args>
KFR_INTRINSIC auto bind_func(Fn&& fn, Args&&... args) noexcept
{
    return [=]() KFR_INLINE_LAMBDA { return fn(details::call_if_callable(std::forward<Args>(args))...); };
}

template <typename T>
constexpr KFR_INTRINSIC bool is_even(T x) noexcept
{
    return (x % 2) == 0;
}

template <typename T>
constexpr KFR_INTRINSIC bool is_odd(T x) noexcept
{
    return !is_even(x);
}

template <typename T>
constexpr KFR_INTRINSIC bool is_poweroftwo(T x) noexcept
{
    return ((x != 0) && !(x & (x - 1)));
}

template <typename T>
constexpr KFR_INTRINSIC unsigned ilog2(T n, unsigned p = 0) noexcept
{
    return (n <= 1) ? p : ilog2(n / 2, p + 1);
}

/// @brief Returns a nearest power of two that is greater or equal than n
template <typename T>
constexpr KFR_INTRINSIC T next_poweroftwo(T n) noexcept
{
    return n > 2 ? T(1) << (ilog2(n - 1) + 1) : n;
}

/// @brief Returns a nearest power of two that is less or equal than n
template <typename T>
constexpr KFR_INTRINSIC T prev_poweroftwo(T n) noexcept
{
    return n > 2 ? T(1) << (ilog2(n)) : n;
}

template <typename T>
constexpr KFR_INTRINSIC bool is_divisible(T x, T divisor) noexcept
{
    return x % divisor == 0;
}

/// @brief Greatest common divisor
template <typename T>
constexpr KFR_INTRINSIC T gcd(T a) noexcept
{
    return a;
}

/// @brief Greatest common divisor
template <typename T>
constexpr inline T gcd(T a, T b) noexcept
{
    return a < b ? gcd(b, a) : ((a % b == 0) ? b : gcd(b, a % b));
}

/// @brief Greatest common divisor
template <typename T, typename... Ts>
constexpr KFR_INTRINSIC T gcd(T a, T b, T c, Ts... rest) noexcept
{
    return gcd(a, gcd(b, c, rest...));
}

/// @brief Least common multiple
template <typename T>
constexpr KFR_INTRINSIC T lcm(T a) noexcept
{
    return a;
}

/// @brief Least common multiple
template <typename T>
constexpr KFR_INTRINSIC T lcm(T a, T b) noexcept
{
    return a * b / gcd(a, b);
}

/// @brief Least common multiple
template <typename T, typename... Ts>
constexpr KFR_INTRINSIC T lcm(T a, T b, T c, Ts... rest) noexcept
{
    return lcm(a, lcm(b, c, rest...));
}

KFR_INTRINSIC std::lldiv_t floor_div(long long a, long long b) noexcept
{
    std::lldiv_t d = std::lldiv(a, b);
    if (d.rem < 0)
    {
        d.rem += b;
        --d.quot;
    }
    return d;
}

namespace details
{

template <typename T>
constexpr inline char typekind = std::is_floating_point_v<T> ? 'f'
                                 : std::is_integral_v<T>     ? (std::is_unsigned_v<T> ? 'u' : 'i')
                                                             : '?';

template <char kind, size_t bits>
struct bits_to_type_impl;

template <>
struct bits_to_type_impl<'f', 32>
{
    using type = float;
    static_assert(sizeof(type) * 8 == 32, "float must represent IEEE single precision value");
};
template <>
struct bits_to_type_impl<'f', 64>
{
    using type = double;
    static_assert(sizeof(type) * 8 == 64, "double must represent IEEE double precision value");
};

template <>
struct bits_to_type_impl<'i', 8>
{
    using type = std::int8_t;
};
template <>
struct bits_to_type_impl<'i', 16>
{
    using type = std::int16_t;
};
template <>
struct bits_to_type_impl<'i', 32>
{
    using type = std::int32_t;
};
template <>
struct bits_to_type_impl<'i', 64>
{
    using type = std::int64_t;
};

template <>
struct bits_to_type_impl<'u', 8>
{
    using type = std::uint8_t;
};
template <>
struct bits_to_type_impl<'u', 16>
{
    using type = std::uint16_t;
};
template <>
struct bits_to_type_impl<'u', 32>
{
    using type = std::uint32_t;
};
template <>
struct bits_to_type_impl<'u', 64>
{
    using type = std::uint64_t;
};

template <char kind, size_t bits>
using bits_to_type = typename bits_to_type_impl<kind, bits>::type;

template <char kind, size_t bytes>
using bytes_to_type = typename bits_to_type_impl<kind, bytes * 8>::type;

template <int64_t min, int64_t max, typename... Types>
struct findinttype_impl
{
};
template <int64_t min, int64_t max, typename T, typename... Types>
struct findinttype_impl<min, max, T, Types...>
{
    using type =
        std::conditional_t<(std::numeric_limits<T>::min() <= min && std::numeric_limits<T>::max() >= max), T,
                           typename findinttype_impl<min, max, Types...>::type>;
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
} // namespace details

template <size_t bits>
using float_type = typename details::bits_to_type_impl<'f', bits>::type;
template <size_t bits>
using int_type = typename details::bits_to_type_impl<'i', bits>::type;
template <size_t bits>
using unsigned_type = typename details::bits_to_type_impl<'u', bits>::type;

template <int64_t min, int64_t max>
using findinttype = typename details::findinttype_impl<min, max, uint8_t, int8_t, uint16_t, int16_t, uint32_t,
                                                       int32_t, uint64_t, int64_t>::type;

template <typename T>
constexpr inline bool is_number = details::is_number_impl<std::decay_t<T>>::value;

template <typename T>
constexpr inline bool is_number_or_bool = is_number<T> || std::is_same_v<std::decay_t<T>, bool>;

template <typename... Ts>
constexpr inline bool is_numbers = (details::is_number_impl<std::decay_t<Ts>>::value && ...);

/// @brief Check if the type argument is a number or a vector of numbers
template <typename T>
constexpr inline bool is_numeric = is_number<deep_subtype<T>>;

/// @brief Check if the type arguments are a numbers or a vectors of numbers
template <typename... Ts>
constexpr inline bool is_numeric_args = (is_numeric<Ts> && ...);

/// @brief Check if the type argument is a number, bool or a vector of numbers of bool
template <typename T>
constexpr inline bool is_numeric_or_bool = is_number_or_bool<deep_subtype<T>>;

namespace details
{
template <typename T>
struct identity_impl
{
    using type = T;
};

template <typename T>
constexpr size_t elementsize() noexcept
{
    return sizeof(T);
}

template <>
constexpr size_t elementsize<void>() noexcept
{
    return 1;
}
} // namespace details

/// @brief Utility class to use in list-initialization context
struct swallow
{
    template <typename... T>
    KFR_MEM_INTRINSIC constexpr swallow(T&&...) noexcept
    {
    }
};

template <typename T, size_t N>
struct carray;

template <typename T>
struct carray<T, 1>
{
    KFR_MEM_INTRINSIC constexpr carray() noexcept = default;
    KFR_MEM_INTRINSIC constexpr carray(T val) noexcept : val(val) {}

    template <typename Fn, size_t index = 0, KFR_ENABLE_IF(std::is_invocable_v<Fn, csize_t<index>>)>
    KFR_MEM_INTRINSIC constexpr carray(Fn&& fn, csize_t<index> = csize_t<index>{}) noexcept
        : val(static_cast<T>(fn(csize_t<index>())))
    {
    }

    KFR_MEM_INTRINSIC constexpr carray(const carray&) noexcept = default;
    KFR_MEM_INTRINSIC constexpr carray(carray&&) noexcept      = default;
    KFR_MEM_INTRINSIC static constexpr size_t size() noexcept { return 1; }

    template <size_t index>
    KFR_MEM_INTRINSIC constexpr T& get(csize_t<index>) noexcept
    {
        static_assert(index == 0, "carray: Array index is out of range");
        return val;
    }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr const T& get(csize_t<index>) const noexcept
    {
        static_assert(index == 0, "carray: Array index is out of range");
        return val;
    }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr T& get() noexcept
    {
        return get(csize_t<index>());
    }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr const T& get() const noexcept
    {
        return get(csize_t<index>());
    }
    KFR_MEM_INTRINSIC constexpr const T* front() const noexcept { return val; }
    KFR_MEM_INTRINSIC constexpr T* front() noexcept { return val; }
    KFR_MEM_INTRINSIC constexpr const T* back() const noexcept { return val; }
    KFR_MEM_INTRINSIC constexpr T* back() noexcept { return val; }
    KFR_MEM_INTRINSIC constexpr const T* begin() const noexcept { return &val; }
    KFR_MEM_INTRINSIC constexpr const T* end() const noexcept { return &val + 1; }
    KFR_MEM_INTRINSIC constexpr T* begin() noexcept { return &val; }
    KFR_MEM_INTRINSIC constexpr T* end() noexcept { return &val + 1; }
    KFR_MEM_INTRINSIC constexpr const T* data() const noexcept { return begin(); }
    KFR_MEM_INTRINSIC constexpr T* data() noexcept { return begin(); }
    KFR_MEM_INTRINSIC constexpr bool empty() const noexcept { return false; }
    T val;
};

template <typename T, size_t N>
struct carray : carray<T, N - 1>
{
    template <typename... Ts>
    KFR_MEM_INTRINSIC constexpr carray(T first, Ts... list) noexcept : carray<T, N - 1>(list...), val(first)
    {
        static_assert(sizeof...(list) + 1 == N, "carray: Argument count is invalid");
    }

    template <typename Fn, size_t index = N - 1>
    KFR_MEM_INTRINSIC constexpr carray(Fn&& fn, csize_t<index> = csize_t<index>{}) noexcept
        : carray<T, N - 1>(std::forward<Fn>(fn), csize_t<index - 1>()),
          val(static_cast<T>(fn(csize_t<index>())))
    {
    }

    KFR_MEM_INTRINSIC constexpr carray() noexcept              = default;
    KFR_MEM_INTRINSIC constexpr carray(const carray&) noexcept = default;
    KFR_MEM_INTRINSIC constexpr carray(carray&&) noexcept      = default;
    KFR_MEM_INTRINSIC static constexpr size_t size() noexcept { return N; }
    KFR_MEM_INTRINSIC constexpr T& get(csize_t<N - 1>) noexcept { return val; }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr T& get(csize_t<index>) noexcept
    {
        return carray<T, N - 1>::get(csize_t<index>());
    }
    KFR_MEM_INTRINSIC constexpr const T& get(csize_t<N - 1>) const noexcept { return val; }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr const T& get(csize_t<index>) const noexcept
    {
        return carray<T, N - 1>::get(csize_t<index>());
    }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr T& get() noexcept
    {
        return get(csize_t<index>());
    }
    template <size_t index>
    KFR_MEM_INTRINSIC constexpr const T& get() const noexcept
    {
        return get(csize_t<index>());
    }
    KFR_MEM_INTRINSIC constexpr const T* front() const noexcept { return carray<T, N - 1>::front(); }
    KFR_MEM_INTRINSIC constexpr T* front() noexcept { return carray<T, N - 1>::front(); }
    KFR_MEM_INTRINSIC constexpr const T* back() const noexcept { return val; }
    KFR_MEM_INTRINSIC constexpr T* back() noexcept { return val; }
    KFR_MEM_INTRINSIC constexpr const T* begin() const noexcept { return carray<T, N - 1>::begin(); }
    KFR_MEM_INTRINSIC constexpr const T* end() const noexcept { return &val + 1; }
    KFR_MEM_INTRINSIC constexpr T* begin() noexcept { return carray<T, N - 1>::begin(); }
    KFR_MEM_INTRINSIC constexpr T* end() noexcept { return &val + 1; }
    KFR_MEM_INTRINSIC constexpr const T* data() const noexcept { return begin(); }
    KFR_MEM_INTRINSIC constexpr T* data() noexcept { return begin(); }
    KFR_MEM_INTRINSIC constexpr bool empty() const noexcept { return false; }

private:
    T val;
};

#define KFR_META_FN(fn)                                                                                      \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        KFR_INLINE_MEMBER decltype(fn(std::declval<Args>()...)) operator()(Args&&... args) const noexcept    \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

#define KFR_ESC(...) __VA_ARGS__

#define KFR_META_FN_TPL(tpl_list, tpl_args, fn)                                                              \
    template <KFR_ESC tpl_list>                                                                              \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        KFR_INLINE_MEMBER decltype(fn<KFR_ESC tpl_args>(std::declval<Args>()...)) operator()(                \
            Args&&... args) const noexcept                                                                   \
        {                                                                                                    \
            return fn<KFR_ESC tpl_args>(std::forward<Args>(args)...);                                        \
        }                                                                                                    \
    };

/// @brief Function that returns its first argument
template <typename T>
KFR_INTRINSIC constexpr T&& pass_through(T&& x) noexcept
{
    return std::forward<T>(x);
}

/// @brief Function that returns void and ignores all its arguments
template <typename... Ts>
KFR_INTRINSIC constexpr void noop(Ts&&...) noexcept
{
}

/// @brief Function that returns its first argument and ignores all other arguments
template <typename T1, typename... Ts>
KFR_INTRINSIC constexpr T1&& get_first(T1&& x, Ts&&...) noexcept
{
    return std::forward<T1>(x);
}

/// @brief Function that returns its second argument and ignores all other arguments
template <typename T1, typename T2, typename... Ts>
KFR_INTRINSIC constexpr T2&& get_second(T1, T2&& x, Ts&&...) noexcept
{
    return std::forward<T2>(x);
}

/// @brief Function that returns its third argument and ignores all other arguments
template <typename T1, typename T2, typename T3, typename... Ts>
KFR_INTRINSIC constexpr T3&& get_third(T1&&, T2&&, T3&& x, Ts&&...) noexcept
{
    return std::forward<T3>(x);
}

/// @brief Function that returns value-initialization of type T and ignores all its arguments
template <typename T, typename... Ts>
KFR_INTRINSIC constexpr T returns(Ts&&...) noexcept
{
    return T();
}

/// @brief Function that returns constant of type T and ignores all its arguments
template <typename T, T value, typename... Args>
KFR_INTRINSIC constexpr T return_constant(Args&&...) noexcept
{
    return value;
}

KFR_META_FN(pass_through)
KFR_META_FN(noop)
KFR_META_FN(get_first)
KFR_META_FN(get_second)
KFR_META_FN(get_third)
KFR_META_FN_TPL((typename T), (T), returns)

template <typename T, T value>
struct fn_return_constant
{
    template <typename... Args>
    constexpr T operator()(Args&&...) const noexcept
    {
        return value;
    }
};

template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_equal(const T1& x, const T2& y) noexcept(noexcept(x == y))
{
    return x == y;
}
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_notequal(const T1& x, const T2& y) noexcept(noexcept(x != y))
{
    return x != y;
}
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_less(const T1& x, const T2& y) noexcept(noexcept(x < y))
{
    return x < y;
}
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_greater(const T1& x, const T2& y) noexcept(noexcept(x > y))
{
    return x > y;
}
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_lessorequal(const T1& x, const T2& y) noexcept(noexcept(x <= y))
{
    return x <= y;
}
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_greaterorequal(const T1& x, const T2& y) noexcept(noexcept(x >= y))
{
    return x >= y;
}
template <typename T>
KFR_INTRINSIC constexpr bool is_between(T value, std::type_identity_t<T> min,
                                        std::type_identity_t<T> max) noexcept(noexcept(value >= min &&
                                                                                     value <= max))
{
    return value >= min && value <= max;
}
KFR_META_FN(is_equal)
KFR_META_FN(is_notequal)
KFR_META_FN(is_less)
KFR_META_FN(is_greater)
KFR_META_FN(is_lessorequal)
KFR_META_FN(is_greaterorequal)
KFR_META_FN(is_between)

namespace details
{
template <typename, typename = void>
struct has_begin_end_impl : std::false_type
{
};

template <typename T>
struct has_begin_end_impl<T,
                          std::void_t<decltype(std::declval<T>().begin()), decltype(std::declval<T>().end())>>
    : std::true_type
{
};

template <typename, typename = void>
struct has_value_type_impl : std::false_type
{
};

template <typename T>
struct has_value_type_impl<T, std::void_t<typename T::value_type>> : std::true_type
{
};

template <typename, typename = void>
struct has_data_size_impl : std::false_type
{
};

template <typename T>
struct has_data_size_impl<T,
                          std::void_t<decltype(std::declval<T>().size()), decltype(std::declval<T>().data())>>
    : std::true_type
{
};

template <typename, typename = void>
struct has_data_size_free_impl : std::false_type
{
};

template <typename T>
struct has_data_size_free_impl<
    T, std::void_t<decltype(std::size(std::declval<T>())), decltype(std::data(std::declval<T>()))>>
    : std::true_type
{
};

template <typename, typename Fallback, typename = void>
struct value_type_impl
{
    using type = Fallback;
};

template <typename T, typename Fallback>
struct value_type_impl<T, Fallback, std::void_t<typename T::value_type>>
{
    using type = typename T::value_type;
};
} // namespace details

template <typename T>
constexpr inline bool has_begin_end = details::has_begin_end_impl<std::decay_t<T>>::value;

template <typename T>
constexpr inline bool has_data_size = details::has_data_size_impl<std::decay_t<T>>::value;

#define KFR_HAS_DATA_SIZE(CONTAINER)                                                                         \
    std::enable_if_t<details::has_data_size_free_impl<CONTAINER>::value>* = nullptr

template <typename T>
using value_type_of = typename std::decay_t<T>::value_type;

#ifndef KFR_COMPILER_CLANG
namespace details
{
template <typename T, T value, typename Fn>
void cforeach_impl(Fn&& fn)
{
    fn(cval_t<T, value>());
}
} // namespace details
#endif

template <typename T, T... values, typename Fn>
KFR_INTRINSIC void cforeach(cvals_t<T, values...>, Fn&& fn)
{
#ifdef KFR_COMPILER_CLANG
    swallow{ (fn(cval_t<T, values>()), void(), 0)... };
#else
    swallow{ (details::cforeach_impl<T, values>(std::forward<Fn>(fn)), void(), 0)... };
#endif
}

template <typename T, typename Fn, KFR_ENABLE_IF(has_begin_end<T>)>
KFR_INTRINSIC void cforeach(T&& list, Fn&& fn)
{
    for (const auto& v : list)
    {
        fn(v);
    }
}

template <typename T, size_t N, typename Fn>
KFR_INTRINSIC void cforeach(const T (&array)[N], Fn&& fn)
{
    for (size_t i = 0; i < N; i++)
    {
        fn(array[i]);
    }
}

namespace details
{

template <size_t index, typename... types>
KFR_INTRINSIC auto get_type_arg(ctypes_t<types...>) noexcept
{
    return ctype_t<typename details::get_nth_type<index, types...>::type>();
}

template <typename T0, typename... types, typename Fn, size_t... indices>
KFR_INTRINSIC void cforeach_types_impl(ctypes_t<T0, types...> type_list, Fn&& fn, csizes_t<indices...>)
{
    swallow{ (fn(get_type_arg<indices>(type_list)), void(), 0)... };
}
template <typename Fn>
KFR_INTRINSIC void cforeach_types_impl(ctypes_t<>, Fn&&, csizes_t<>)
{
}
} // namespace details

template <typename... Ts, typename Fn>
KFR_INTRINSIC void cforeach(ctypes_t<Ts...> types, Fn&& fn)
{
    details::cforeach_types_impl(types, std::forward<Fn>(fn), csizeseq_t<sizeof...(Ts)>());
}

template <typename A0, typename A1, typename Fn>
KFR_INTRINSIC void cforeach(A0&& a0, A1&& a1, Fn&& fn)
{
    // Default capture causes ICE in Intel C++
    cforeach(std::forward<A0>(a0), //
             [&a1, &fn](auto v0) { //
                 cforeach(std::forward<A1>(a1), //
                          [&v0, &fn](auto v1) { fn(v0, v1); });
             });
}

template <typename A0, typename A1, typename A2, typename Fn>
KFR_INTRINSIC void cforeach(A0&& a0, A1&& a1, A2&& a2, Fn&& fn)
{
    // Default capture causes ICE in Intel C++
    cforeach(std::forward<A0>(a0), //
             [&a1, &a2, &fn](auto v0) { //
                 cforeach(std::forward<A1>(a1), //
                          [&v0, &a2, &fn](auto v1) { //
                              cforeach(std::forward<A2>(a2), //
                                       [&v0, &v1, &fn](auto v2) { //
                                           fn(v0, v1, v2);
                                       });
                          });
             });
}

template <typename A0, typename A1, typename A2, typename A3, typename Fn>
KFR_INTRINSIC void cforeach(A0&& a0, A1&& a1, A2&& a2, A3&& a3, Fn&& fn)
{
    // Default capture causes ICE in Intel C++
    cforeach(std::forward<A0>(a0), //
             [&a1, &a2, &a3, &fn](auto v0) { //
                 cforeach(std::forward<A1>(a1), //
                          [&v0, &a2, &a3, &fn](auto v1) { //
                              cforeach(std::forward<A2>(a2), //
                                       [&v0, &v1, &a3, &fn](auto v2) { //
                                           cforeach(std::forward<A3>(a3), //
                                                    [&v0, &v1, &v2, &fn](auto v3) //
                                                    { fn(v0, v1, v2, v3); });
                                       });
                          });
             });
}

template <typename TrueFn, typename FalseFn = fn_noop>
KFR_INTRINSIC decltype(auto) cif(cbool_t<true>, TrueFn&& truefn, FalseFn&& = FalseFn())
{
    return truefn(ctrue);
}

template <typename TrueFn, typename FalseFn = fn_noop>
KFR_INTRINSIC decltype(auto) cif(cbool_t<false>, TrueFn&&, FalseFn&& falsefn = FalseFn())
{
    return falsefn(cfalse);
}

template <typename T, T start, T stop, typename BodyFn>
KFR_INTRINSIC decltype(auto) cfor(cval_t<T, start>, cval_t<T, stop>, BodyFn&& bodyfn)
{
    return cforeach(cvalseq_t<T, stop - start, start>(), std::forward<BodyFn>(bodyfn));
}

template <typename T, T... vs, typename U, typename Function, typename Fallback = fn_noop>
KFR_INTRINSIC void cswitch(cvals_t<T, vs...>, const U& value, Function&& function,
                           Fallback&& fallback = Fallback())
{
    bool result = false;
    swallow{ (result = result || ((vs == value) ? (function(cval_t<T, vs>()), void(), true) : false), void(),
              0)... };
    if (!result)
        fallback();
}

template <typename T, typename Fn, typename DefFn = fn_noop, typename CmpFn = fn_is_equal>
KFR_INTRINSIC decltype(auto) cswitch(cvals_t<T>, std::type_identity_t<T>, Fn&&, DefFn&& deffn = DefFn(),
                                     CmpFn&& = CmpFn())
{
    return deffn();
}

template <typename T, T v0, T... values, typename Fn, typename DefFn = fn_noop, typename CmpFn = fn_is_equal>
KFR_INTRINSIC decltype(auto) cswitch(cvals_t<T, v0, values...>, std::type_identity_t<T> value, Fn&& fn,
                                     DefFn&& deffn = DefFn(), CmpFn&& cmpfn = CmpFn())
{
    if (cmpfn(value, v0))
    {
        return fn(cval_t<T, v0>());
    }
    else
    {
        return cswitch(cvals_t<T, values...>(), value, std::forward<Fn>(fn), std::forward<DefFn>(deffn),
                       std::forward<CmpFn>(cmpfn));
    }
}

namespace details
{
template <typename T, typename Fn1, typename Fn2, typename... Fns>
KFR_INTRINSIC decltype(auto) cmatch_impl(T&& value, Fn1&& first, Fn2&& second, Fns&&... rest);
template <typename T, typename Fn, typename... Ts>
KFR_INTRINSIC decltype(auto) cmatch_impl(T&& value, Fn&& last);

template <typename T, typename Fn, typename... Fns>
KFR_INTRINSIC decltype(auto) cmatch_impl2(cbool_t<true>, T&& value, Fn&& fn, Fns&&...)
{
    return fn(std::forward<T>(value));
}

template <typename T, typename Fn, typename... Fns>
KFR_INTRINSIC decltype(auto) cmatch_impl2(cbool_t<false>, T&& value, Fn&&, Fns&&... rest)
{
    return cmatch_impl(std::forward<T>(value), std::forward<Fns>(rest)...);
}

template <typename T, typename Fn1, typename Fn2, typename... Fns>
KFR_INTRINSIC decltype(auto) cmatch_impl(T&& value, Fn1&& first, Fn2&& second, Fns&&... rest)
{
    using first_arg        = typename function_arguments<Fn1>::template nth<0>;
    constexpr bool is_same = std::is_same_v<std::decay_t<T>, std::decay_t<first_arg>>;
    return cmatch_impl2(cbool_t<is_same>(), std::forward<T>(value), std::forward<Fn1>(first),
                        std::forward<Fn2>(second), std::forward<Fns>(rest)...);
}

template <typename T, typename Fn, typename... Ts>
KFR_INTRINSIC decltype(auto) cmatch_impl(T&& value, Fn&& last)
{
    return last(std::forward<T>(value));
}
} // namespace details

template <typename T, typename Fn, typename... Args>
KFR_INTRINSIC decltype(auto) cmatch(T&& value, Fn&& fn, Args... args)
{
    return details::cmatch_impl(std::forward<T>(value), std::forward<Fn>(fn), std::forward<Args>(args)...);
}

template <typename T, T... values>
KFR_INTRINSIC size_t cfind(cvals_t<T, values...>, std::type_identity_t<T> value)
{
    static constexpr T temp[]    = { values... };
    static constexpr size_t size = sizeof...(values);
    for (size_t i = 0; i < size; i++)
    {
        if (temp[i] == value)
            return i;
    }
    return size_t(-1);
}

template <typename Fn, typename... Args>
KFR_UNUSED KFR_NOINLINE static std::invoke_result_t<Fn, Args...> noinline(Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

template <typename Fn>
struct fn_noinline
{
    template <typename... Args>
    KFR_MEM_INTRINSIC std::invoke_result_t<Fn, Args...> operator()(Args&&... args) const
    {
        return noinline(Fn{}, std::forward<Args>(args)...);
    }
}; // namespace kfr

template <typename... Args, typename Fn, typename Ret = decltype(std::declval<Fn>()(std::declval<Args>()...)),
          typename NonMemFn = Ret (*)(Fn*, Args...)>
KFR_INTRINSIC NonMemFn make_nonmember(const Fn&)
{
    return [](Fn* fn, Args... args) -> Ret { return fn->operator()(std::forward<Args>(args)...); };
}

template <typename T>
constexpr KFR_INTRINSIC T choose_const() noexcept
{
    static_assert(sizeof(T) != 0, "T not found in the list of template arguments");
    return T();
}
template <typename T, typename C1>
constexpr KFR_INTRINSIC T choose_const_fallback(C1 c1) noexcept
{
    return static_cast<T>(c1);
}

/**
 * Selects constant of the specific type
 * @code
 * CHECK( choose_const<f32>( 32.0f, 64.0 ) == 32.0f );
 * CHECK( choose_const<f64>( 32.0f, 64.0 ) == 64.0 );
 * @endcode
 */
template <typename T, typename C1, typename... Cs, KFR_ENABLE_IF(std::is_same_v<T, C1>)>
constexpr KFR_INTRINSIC T choose_const(C1 c1, Cs...) noexcept
{
    return static_cast<T>(c1);
}
template <typename T, typename C1, typename... Cs, KFR_ENABLE_IF(!std::is_same_v<T, C1>)>
constexpr KFR_INTRINSIC T choose_const(C1, Cs... constants) noexcept
{
    return choose_const<T>(constants...);
}

template <typename T, typename C1, typename... Cs>
constexpr KFR_INTRINSIC T choose_const_fallback(C1 c1, Cs... constants) noexcept
{
    return std::is_same_v<T, C1> ? static_cast<T>(c1) : choose_const_fallback<T>(constants...);
}

template <typename Tfrom>
struct autocast_impl
{
    const Tfrom value;
    template <typename T>
    KFR_MEM_INTRINSIC constexpr operator T() const noexcept
    {
        return static_cast<T>(value);
    }
};

template <typename Tfrom>
KFR_INTRINSIC constexpr autocast_impl<Tfrom> autocast(const Tfrom& value) noexcept
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
struct signed_type_impl<T, std::void_t<std::enable_if_t<std::is_unsigned_v<T>>>>
{
    using type = findinttype<std::numeric_limits<T>::min(), std::numeric_limits<T>::max()>;
};
} // namespace details

template <typename T>
using signed_type = typename details::signed_type_impl<T>::type;

template <typename T>
constexpr KFR_INTRINSIC T align_down(T x, std::type_identity_t<T> alignment) noexcept
{
    return (x) & ~(alignment - 1);
}
template <typename T>
constexpr KFR_INTRINSIC T* align_down(T* x, size_t alignment) noexcept
{
    return reinterpret_cast<T*>(align_down(reinterpret_cast<size_t>(x), alignment));
}

template <typename T>
constexpr KFR_INTRINSIC T align_up(T x, std::type_identity_t<T> alignment) noexcept
{
    return (x + alignment - 1) & ~(alignment - 1);
}
template <typename T>
constexpr KFR_INTRINSIC T* align_up(T* x, size_t alignment) noexcept
{
    return reinterpret_cast<T*>(align_up(reinterpret_cast<size_t>(x), alignment));
}

template <typename T>
constexpr KFR_INTRINSIC T* advance(T* x, ptrdiff_t offset) noexcept
{
    return x + offset;
}
constexpr KFR_INTRINSIC void* advance(void* x, ptrdiff_t offset) noexcept
{
    return advance(static_cast<unsigned char*>(x), offset);
}

constexpr KFR_INTRINSIC ptrdiff_t distance(const void* x, const void* y) noexcept
{
    return static_cast<const unsigned char*>(x) - static_cast<const unsigned char*>(y);
}

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Wundefined-reinterpret-cast")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wundefined-reinterpret-cast")
#endif

template <typename T, typename U>
KFR_INTRINSIC constexpr static T& ref_cast(U& ptr) noexcept
{
    return reinterpret_cast<T&>(ptr);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static const T& ref_cast(const U& ptr) noexcept
{
    return reinterpret_cast<const T&>(ptr);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static T* ptr_cast(U* ptr) noexcept
{
    return reinterpret_cast<T*>(ptr);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static const T* ptr_cast(const U* ptr) noexcept
{
    return reinterpret_cast<const T*>(ptr);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static T* ptr_cast(U* ptr, ptrdiff_t offset) noexcept
{
    return ptr_cast<T>(ptr_cast<unsigned char>(ptr) + offset);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static T* derived_cast(U* ptr) noexcept
{
    return static_cast<T*>(ptr);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static const T* derived_cast(const U* ptr) noexcept
{
    return static_cast<const T*>(ptr);
}

template <typename T, typename U>
KFR_INTRINSIC constexpr static T implicit_cast(U&& value) noexcept
{
    return std::forward<T>(value);
}

namespace details
{
template <size_t start, size_t count>
constexpr KFR_INTRINSIC std::true_type test_sequence(csizeseq_t<count, start>) noexcept
{
    return {};
}

template <size_t, size_t>
constexpr KFR_INTRINSIC std::false_type test_sequence(...) noexcept
{
    return {};
}
} // namespace details

template <size_t number, size_t... numbers>
constexpr KFR_INTRINSIC bool is_sequence(csizes_t<number, numbers...>) noexcept
{
    return details::test_sequence<number, 1 + sizeof...(numbers)>(csizes_t<number, numbers...>()).value;
}

template <typename T, T val>
constexpr inline cval_t<T, val> cval{};

template <bool val>
constexpr inline cbool_t<val> cbool{};

template <int val>
constexpr inline cint_t<val> cint{};

template <unsigned val>
constexpr inline cuint_t<val> cuint{};

template <size_t val>
constexpr inline csize_t<val> csize{};

template <typename T, T... values>
constexpr inline cvals_t<T, values...> cvals{};

template <bool... vals>
constexpr inline cbools_t<vals...> cbools{};

template <int... vals>
constexpr inline cints_t<vals...> cints{};

template <char... vals>
constexpr inline cchars_t<vals...> cchars{};

template <unsigned... vals>
constexpr inline cuints_t<vals...> cuints{};

template <size_t... vals>
constexpr inline csizes_t<vals...> csizes{};

template <size_t... vals>
constexpr inline elements_t<vals...> elements{};

template <typename T>
constexpr inline ctype_t<T> ctype{};

template <typename... Ts>
constexpr inline ctypes_t<Ts...> ctypes{};

template <typename T, T begin, T end>
constexpr inline cvalseq_t<T, end - begin, begin> cvalrange{};

template <size_t begin, size_t end>
constexpr inline cvalseq_t<size_t, end - begin, begin> csizerange{};

template <int begin, int end>
constexpr inline cvalseq_t<int, end - begin, begin> cintrange{};

template <unsigned begin, unsigned end>
constexpr inline cvalseq_t<unsigned, end - begin, begin> cuintrange{};

template <typename T, size_t size, T start = T(), ptrdiff_t step = 1>
constexpr inline cvalseq_t<T, size, start, step> cvalseq{};

template <size_t size, size_t start = 0, ptrdiff_t step = 1>
constexpr inline cvalseq_t<size_t, size, start, step> csizeseq{};

template <size_t size, int start = 0, ptrdiff_t step = 1>
constexpr inline cvalseq_t<int, size, start, step> cintseq{};

template <size_t size, unsigned start = 0, ptrdiff_t step = 1>
constexpr inline cvalseq_t<unsigned, size, start, step> cuintseq{};
template <typename... List>
constexpr inline indicesfor_t<List...> indicesfor{};

template <typename T>
constexpr KFR_INTRINSIC T cminof(cvals_t<T>)
{
    return std::numeric_limits<T>::max();
}
template <typename T, T val, T... vals>
constexpr KFR_INTRINSIC T cminof(cvals_t<T, val, vals...>)
{
    T m = cminof(cvals<T, vals...>);
    return val < m ? val : m;
}
template <typename T>
constexpr KFR_INTRINSIC T cmaxof(cvals_t<T>)
{
    return std::numeric_limits<T>::min();
}
template <typename T, T val, T... vals>
constexpr KFR_INTRINSIC T cmaxof(cvals_t<T, val, vals...>)
{
    T m = cmaxof(cvals<T, vals...>);
    return val > m ? val : m;
}

template <int n = 10>
struct overload_priority : overload_priority<n - 1>
{
};

template <>
struct overload_priority<0>
{
};

constexpr inline overload_priority<> overload_auto{};

using overload_generic = overload_priority<0>;

#define KFR_GEN_LIST1(m, ...) m(0, __VA_ARGS__)
#define KFR_GEN_LIST2(m, ...) KFR_GEN_LIST1(m, __VA_ARGS__), m(1, __VA_ARGS__)
#define KFR_GEN_LIST3(m, ...) KFR_GEN_LIST2(m, __VA_ARGS__), m(2, __VA_ARGS__)
#define KFR_GEN_LIST4(m, ...) KFR_GEN_LIST3(m, __VA_ARGS__), m(3, __VA_ARGS__)
#define KFR_GEN_LIST5(m, ...) KFR_GEN_LIST4(m, __VA_ARGS__), m(4, __VA_ARGS__)
#define KFR_GEN_LIST6(m, ...) KFR_GEN_LIST5(m, __VA_ARGS__), m(5, __VA_ARGS__)
#define KFR_GEN_LIST7(m, ...) KFR_GEN_LIST6(m, __VA_ARGS__), m(6, __VA_ARGS__)
#define KFR_GEN_LIST8(m, ...) KFR_GEN_LIST7(m, __VA_ARGS__), m(7, __VA_ARGS__)
#define KFR_GEN_LIST9(m, ...) KFR_GEN_LIST8(m, __VA_ARGS__), m(8, __VA_ARGS__)
#define KFR_GEN_LIST10(m, ...) KFR_GEN_LIST9(m, __VA_ARGS__), m(9, __VA_ARGS__)

#define KFR_GEN_LIST11(m, ...) KFR_GEN_LIST10(m, __VA_ARGS__), m(10, __VA_ARGS__)
#define KFR_GEN_LIST12(m, ...) KFR_GEN_LIST11(m, __VA_ARGS__), m(11, __VA_ARGS__)
#define KFR_GEN_LIST13(m, ...) KFR_GEN_LIST12(m, __VA_ARGS__), m(12, __VA_ARGS__)
#define KFR_GEN_LIST14(m, ...) KFR_GEN_LIST13(m, __VA_ARGS__), m(13, __VA_ARGS__)
#define KFR_GEN_LIST15(m, ...) KFR_GEN_LIST14(m, __VA_ARGS__), m(14, __VA_ARGS__)
#define KFR_GEN_LIST16(m, ...) KFR_GEN_LIST15(m, __VA_ARGS__), m(15, __VA_ARGS__)
#define KFR_GEN_LIST17(m, ...) KFR_GEN_LIST16(m, __VA_ARGS__), m(16, __VA_ARGS__)
#define KFR_GEN_LIST18(m, ...) KFR_GEN_LIST17(m, __VA_ARGS__), m(17, __VA_ARGS__)
#define KFR_GEN_LIST19(m, ...) KFR_GEN_LIST18(m, __VA_ARGS__), m(18, __VA_ARGS__)
#define KFR_GEN_LIST20(m, ...) KFR_GEN_LIST19(m, __VA_ARGS__), m(19, __VA_ARGS__)

#define KFR_GEN_LIST21(m, ...) KFR_GEN_LIST20(m, __VA_ARGS__), m(20, __VA_ARGS__)
#define KFR_GEN_LIST22(m, ...) KFR_GEN_LIST21(m, __VA_ARGS__), m(21, __VA_ARGS__)
#define KFR_GEN_LIST23(m, ...) KFR_GEN_LIST22(m, __VA_ARGS__), m(22, __VA_ARGS__)
#define KFR_GEN_LIST24(m, ...) KFR_GEN_LIST23(m, __VA_ARGS__), m(23, __VA_ARGS__)
#define KFR_GEN_LIST25(m, ...) KFR_GEN_LIST24(m, __VA_ARGS__), m(24, __VA_ARGS__)
#define KFR_GEN_LIST26(m, ...) KFR_GEN_LIST25(m, __VA_ARGS__), m(25, __VA_ARGS__)
#define KFR_GEN_LIST27(m, ...) KFR_GEN_LIST26(m, __VA_ARGS__), m(26, __VA_ARGS__)
#define KFR_GEN_LIST28(m, ...) KFR_GEN_LIST27(m, __VA_ARGS__), m(27, __VA_ARGS__)
#define KFR_GEN_LIST29(m, ...) KFR_GEN_LIST28(m, __VA_ARGS__), m(28, __VA_ARGS__)
#define KFR_GEN_LIST30(m, ...) KFR_GEN_LIST29(m, __VA_ARGS__), m(29, __VA_ARGS__)

#define KFR_GEN_LIST31(m, ...) KFR_GEN_LIST30(m, __VA_ARGS__), m(30, __VA_ARGS__)
#define KFR_GEN_LIST32(m, ...) KFR_GEN_LIST31(m, __VA_ARGS__), m(31, __VA_ARGS__)
#define KFR_GEN_LIST33(m, ...) KFR_GEN_LIST32(m, __VA_ARGS__), m(32, __VA_ARGS__)
#define KFR_GEN_LIST34(m, ...) KFR_GEN_LIST33(m, __VA_ARGS__), m(33, __VA_ARGS__)
#define KFR_GEN_LIST35(m, ...) KFR_GEN_LIST34(m, __VA_ARGS__), m(34, __VA_ARGS__)
#define KFR_GEN_LIST36(m, ...) KFR_GEN_LIST35(m, __VA_ARGS__), m(35, __VA_ARGS__)
#define KFR_GEN_LIST37(m, ...) KFR_GEN_LIST36(m, __VA_ARGS__), m(36, __VA_ARGS__)
#define KFR_GEN_LIST38(m, ...) KFR_GEN_LIST37(m, __VA_ARGS__), m(37, __VA_ARGS__)
#define KFR_GEN_LIST39(m, ...) KFR_GEN_LIST38(m, __VA_ARGS__), m(38, __VA_ARGS__)
#define KFR_GEN_LIST40(m, ...) KFR_GEN_LIST39(m, __VA_ARGS__), m(39, __VA_ARGS__)

#define KFR_GEN_LIST41(m, ...) KFR_GEN_LIST40(m, __VA_ARGS__), m(40, __VA_ARGS__)
#define KFR_GEN_LIST42(m, ...) KFR_GEN_LIST41(m, __VA_ARGS__), m(41, __VA_ARGS__)
#define KFR_GEN_LIST43(m, ...) KFR_GEN_LIST42(m, __VA_ARGS__), m(42, __VA_ARGS__)
#define KFR_GEN_LIST44(m, ...) KFR_GEN_LIST43(m, __VA_ARGS__), m(43, __VA_ARGS__)
#define KFR_GEN_LIST45(m, ...) KFR_GEN_LIST44(m, __VA_ARGS__), m(44, __VA_ARGS__)
#define KFR_GEN_LIST46(m, ...) KFR_GEN_LIST45(m, __VA_ARGS__), m(45, __VA_ARGS__)
#define KFR_GEN_LIST47(m, ...) KFR_GEN_LIST46(m, __VA_ARGS__), m(46, __VA_ARGS__)
#define KFR_GEN_LIST48(m, ...) KFR_GEN_LIST47(m, __VA_ARGS__), m(47, __VA_ARGS__)
#define KFR_GEN_LIST49(m, ...) KFR_GEN_LIST48(m, __VA_ARGS__), m(48, __VA_ARGS__)
#define KFR_GEN_LIST50(m, ...) KFR_GEN_LIST49(m, __VA_ARGS__), m(49, __VA_ARGS__)

#define KFR_GEN_LIST51(m, ...) KFR_GEN_LIST50(m, __VA_ARGS__), m(50, __VA_ARGS__)
#define KFR_GEN_LIST52(m, ...) KFR_GEN_LIST51(m, __VA_ARGS__), m(51, __VA_ARGS__)
#define KFR_GEN_LIST53(m, ...) KFR_GEN_LIST52(m, __VA_ARGS__), m(52, __VA_ARGS__)
#define KFR_GEN_LIST54(m, ...) KFR_GEN_LIST53(m, __VA_ARGS__), m(53, __VA_ARGS__)
#define KFR_GEN_LIST55(m, ...) KFR_GEN_LIST54(m, __VA_ARGS__), m(54, __VA_ARGS__)
#define KFR_GEN_LIST56(m, ...) KFR_GEN_LIST55(m, __VA_ARGS__), m(55, __VA_ARGS__)
#define KFR_GEN_LIST57(m, ...) KFR_GEN_LIST56(m, __VA_ARGS__), m(56, __VA_ARGS__)
#define KFR_GEN_LIST58(m, ...) KFR_GEN_LIST57(m, __VA_ARGS__), m(57, __VA_ARGS__)
#define KFR_GEN_LIST59(m, ...) KFR_GEN_LIST58(m, __VA_ARGS__), m(58, __VA_ARGS__)
#define KFR_GEN_LIST60(m, ...) KFR_GEN_LIST59(m, __VA_ARGS__), m(59, __VA_ARGS__)

#define KFR_GEN_LIST61(m, ...) KFR_GEN_LIST60(m, __VA_ARGS__), m(60, __VA_ARGS__)
#define KFR_GEN_LIST62(m, ...) KFR_GEN_LIST61(m, __VA_ARGS__), m(61, __VA_ARGS__)
#define KFR_GEN_LIST63(m, ...) KFR_GEN_LIST62(m, __VA_ARGS__), m(62, __VA_ARGS__)
#define KFR_GEN_LIST64(m, ...) KFR_GEN_LIST63(m, __VA_ARGS__), m(63, __VA_ARGS__)
#define KFR_GEN_LIST65(m, ...) KFR_GEN_LIST64(m, __VA_ARGS__), m(64, __VA_ARGS__)
#define KFR_GEN_LIST66(m, ...) KFR_GEN_LIST65(m, __VA_ARGS__), m(65, __VA_ARGS__)
#define KFR_GEN_LIST67(m, ...) KFR_GEN_LIST66(m, __VA_ARGS__), m(66, __VA_ARGS__)
#define KFR_GEN_LIST68(m, ...) KFR_GEN_LIST67(m, __VA_ARGS__), m(67, __VA_ARGS__)
#define KFR_GEN_LIST69(m, ...) KFR_GEN_LIST68(m, __VA_ARGS__), m(68, __VA_ARGS__)
#define KFR_GEN_LIST70(m, ...) KFR_GEN_LIST69(m, __VA_ARGS__), m(69, __VA_ARGS__)

#define KFR_GEN_LIST(c, m, ...) KFR_GEN_LIST##c(m, __VA_ARGS__)

template <typename Tout, typename Tin>
KFR_INTRINSIC Tout bitcast_anything(const Tin& in)
{
    static_assert(sizeof(Tin) == sizeof(Tout), "Invalid arguments for bitcast_anything");
#if defined KFR_COMPILER_INTEL
    const union
    {
        const Tin in;
        Tout out;
    } u{ in };
    return u.out;
#else
    union
    {
        Tin in;
        Tout out;
    } u{ in };
    return u.out;
#endif
}

template <typename T>
KFR_INTRINSIC constexpr T dont_deduce(T x)
{
    return x;
}

template <typename Ty, typename T>
KFR_INTRINSIC constexpr T just_value(T value)
{
    return value;
}

template <typename Tout, typename>
KFR_INTRINSIC constexpr Tout pack_elements()
{
    return 0;
}

template <typename Tout, typename Arg, typename... Args>
KFR_INTRINSIC constexpr Tout pack_elements(Arg x, Args... args)
{
    return static_cast<typename std::make_unsigned<Arg>::type>(x) |
           (pack_elements<Tout, Arg>(args...) << (sizeof(Arg) * 8));
}

template <typename T, bool reference>
using value_or_ref = std::conditional_t<reference, const T&, T>;

enum class special_constant
{
    default_constructed,
    infinity,
    neg_infinity,
    min,
    max,
    neg_max,
    lowest,
    epsilon,
    integer,
    floating_point,
    random_bits,
};

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4700))
KFR_PRAGMA_MSVC(warning(disable : 4146))
struct special_value
{
    constexpr special_value(const special_value&) = default;
    constexpr special_value(special_constant c) : c(c), ll(0), d(0) {}
    constexpr special_value(double d) : c(special_constant::floating_point), ll(0), d(d) {}
    constexpr special_value(long long ll) : c(special_constant::integer), ll(ll), d(0) {}
    constexpr special_value(int i) : c(special_constant::integer), ll(i), d(0) {}

    template <typename T>
    constexpr T get() const noexcept
    {
        switch (c)
        {
        case special_constant::default_constructed:
            return T{};
        case special_constant::infinity:
            return std::numeric_limits<subtype<T>>::infinity();
        case special_constant::neg_infinity:
        {
            subtype<T> gg = std::numeric_limits<subtype<T>>::infinity();
            return -gg;
        }
        case special_constant::min:
            return std::numeric_limits<subtype<T>>::min();
        case special_constant::max:
            return std::numeric_limits<subtype<T>>::max();
        case special_constant::neg_max:
            return static_cast<T>(-std::numeric_limits<subtype<T>>::max());
        case special_constant::lowest:
            return std::numeric_limits<subtype<T>>::lowest();
        case special_constant::integer:
            return static_cast<T>(ll);
        case special_constant::floating_point:
            return static_cast<T>(d);
        case special_constant::random_bits:
            return random_bits<T>();
        case special_constant::epsilon:
            return std::numeric_limits<subtype<T>>::epsilon();
            // default:
            // return T{};
        }
        return T();
    }

    template <typename T>
    constexpr operator T() const noexcept
    {
        return get<T>();
    }
    special_constant c;
    long long ll;
    double d;

    static std::mt19937& random_generator()
    {
        static std::mt19937 rnd(1);
        return rnd;
    }

    template <typename T>
    static T random_bits()
    {
        union
        {
            uint32_t bits[(sizeof(T) + sizeof(uint32_t) - 1) / sizeof(uint32_t)];
            T value;
        } u;
        for (uint32_t& b : u.bits)
        {
            b = random_generator()();
        }
        return u.value;
    }
};
KFR_PRAGMA_MSVC(warning(pop))

KFR_PRAGMA_GNU(GCC diagnostic pop)
} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)

KFR_PRAGMA_MSVC(warning(pop))
