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
#include <bit>
#include <concepts>
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

/// Pointer to void
using pvoid = void*;
/// Pointer to const void
using pconstvoid = const void*;

/// Maximum value representable by size_t
constexpr size_t max_size_t = size_t(-1);

/// If `T1` is `void`, yields `T2`; otherwise yields `T1`
template <typename T1, typename T2>
using or_type = std::conditional_t<std::is_same_v<T1, void>, T2, T1>;

/// Returns the zero-based index of type `T1` within the list `T2, Ts...`
template <typename T1, typename T2 = void, typename... Ts>
constexpr size_t typeindex() noexcept
{
    return std::is_same_v<T1, T2>() ? 0 : 1 + typeindex<T1, Ts...>();
}

/// @brief Traits describing how a type composes scalar elements.
/// Specialized for compound types (e.g. `std::pair`) to expose width, subtype and accessors.
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

/// Returns the width (number of scalar components) of `T`
template <typename T>
constexpr size_t widthof(T) noexcept
{
    return compound_type_traits<T>::width;
}
/// Returns the width (number of scalar components) of `T`
template <typename T>
constexpr size_t widthof() noexcept
{
    return compound_type_traits<std::decay_t<T>>::width;
}

/// `true` if `T` is a compound (non-scalar) type
template <typename T>
constexpr inline bool is_compound_type = !compound_type_traits<std::decay_t<T>>::is_scalar;

/// The immediate subtype of a compound type `T`
template <typename T>
using subtype = typename compound_type_traits<T>::subtype;

/// The deepest scalar subtype of a (possibly nested) compound type `T`
template <typename T>
using deep_subtype = typename compound_type_traits<T>::deep_subtype;

/// @brief Specialization of compound_type_traits for `std::pair<T, T>`
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

/// @brief Compile-time constant wrapper carrying a value of type `T`.
/// Usable as a non-type template parameter and convertible to its underlying value.
template <typename T, T val>
struct cval_t
{
    /// The wrapped constant value
    constexpr static T value = val;
    constexpr KFR_MEM_INTRINSIC cval_t() noexcept {}
    constexpr KFR_MEM_INTRINSIC cval_t(const cval_t&) noexcept = default;
    constexpr KFR_MEM_INTRINSIC cval_t(cval_t&&) noexcept      = default;
    using value_type                                           = T;
    using type                                                 = cval_t;
    /// Implicit conversion to the underlying value
    constexpr KFR_MEM_INTRINSIC operator value_type() const noexcept { return value; }
    /// Returns the wrapped value
    constexpr KFR_MEM_INTRINSIC value_type operator()() const noexcept { return value; }
};

/// Extracts the value from a cval_t constant
/// @tparam T underlying value type
/// @tparam value the wrapped value
template <typename T, T value>
constexpr KFR_INTRINSIC T val_of(cval_t<T, value>) noexcept
{
    return value;
}

/// Returns the value itself (overload for runtime values)
template <typename T>
constexpr KFR_INTRINSIC T val_of(T value) noexcept
{
    return value;
}

/// Macro that extracts the value from a cval_t expression
#define KFR_CVAL(...) (decltype(__VA_ARGS__)::value)

/// Returns `false` for runtime values
template <typename T>
constexpr KFR_INTRINSIC bool is_constant_val(T) noexcept
{
    return false;
}

/// Returns `true` if the argument is a cval_t constant
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

/// Compile-time boolean constant
/// @tparam val the boolean value
template <bool val>
using cbool_t = cval_t<bool, val>;

/// Compile-time int constant
/// @tparam val the int value
template <int val>
using cint_t = cval_t<int, val>;

/// Compile-time unsigned constant
/// @tparam val the unsigned value
template <unsigned val>
using cuint_t = cval_t<unsigned, val>;

/// Compile-time size_t constant
/// @tparam val the size_t value
template <size_t val>
using csize_t = cval_t<size_t, val>;

/// Compile-time false value type
using cfalse_t = cbool_t<false>;
/// Compile-time true value type
using ctrue_t = cbool_t<true>;

/// Constant instance of ctrue_t
constexpr inline ctrue_t ctrue{};
/// Constant instance of cfalse_t
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
#if KFR_HAS_BUILTIN(__type_pack_element)
    using type = __type_pack_element<index, Types...>;
#elif KFR_HAS_BUILTIN(__builtin_type_pack_element)
    using type = __builtin_type_pack_element(index, Types...);
#else
    using type = std::tuple_element_t<index, std::tuple<Types...>>;
#endif
};

} // namespace details

/// @brief Compile-time list of constant values of type `T`.
/// Supports indexing, slicing, mapping and equality comparison at compile time.
/// @tparam T element value type
/// @tparam values the packed constant values
template <typename T, T... values>
struct cvals_t
{
    constexpr KFR_MEM_INTRINSIC cvals_t() noexcept = default;

    using type = cvals_t<T, values...>;
    /// Number of values in the list
    constexpr KFR_MEM_INTRINSIC static size_t size() noexcept { return sizeof...(values); }
    /// Returns the value at `index`
    template <size_t index>
    constexpr KFR_MEM_INTRINSIC T operator[](csize_t<index>) const noexcept
    {
        return get(csize_t<index>());
    }
    /// Returns the value at `index`
    template <size_t index>
    constexpr KFR_MEM_INTRINSIC static T get(csize_t<index> = csize_t<index>()) noexcept
    {
        return details::get_nth<index, T, values...>::value;
    }
    /// Returns the first value
    constexpr KFR_MEM_INTRINSIC static T front() noexcept { return get(csize_t<0>()); }
    /// Returns the last value
    constexpr KFR_MEM_INTRINSIC static T back() noexcept { return get(csize_t<size() - 1>()); }

    /// Iterator to the first value
    static KFR_MEM_INTRINSIC const T* begin() noexcept { return array(); }
    /// Iterator past the last value
    static KFR_MEM_INTRINSIC const T* end() noexcept { return array() + size(); }

    /// Pointer to a static array holding all values
    static KFR_MEM_INTRINSIC const T* array() noexcept
    {
        static const T arr[] = { values... };
        return &arr[0];
    }
    /// Returns a sub-list containing the values at `indices`
    template <size_t... indices>
    constexpr KFR_MEM_INTRINSIC cvals_t<T, details::get_nth_e<indices, type>::value...> operator[](
        cvals_t<size_t, indices...>) const noexcept
    {
        return {};
    }

    template <typename U>
    struct subscript_t
    {
    };
    template <size_t... indices>
    struct subscript_t<cvals_t<size_t, indices...>>
    {
        using type = cvals_t<T, details::get_nth_e<indices, cvals_t<T, values...>>::value...>;
    };

    template <typename U>
    using subscript = typename subscript_t<U>::type;

    // MSVC requires static_cast<T> here:
    /// Returns a new list with `Fn` applied to each value
    template <typename Fn>
    constexpr KFR_MEM_INTRINSIC auto map(Fn&&) const noexcept -> cvals_t<T, static_cast<T>(Fn()(values))...>
    {
        return {};
    }

    /// Returns `true` if the other list holds the same values
    constexpr KFR_MEM_INTRINSIC bool equal(cvals_t<T, values...>) const noexcept { return true; }
    /// Returns `false` if the other list holds different values
    template <T... values2>
    constexpr KFR_MEM_INTRINSIC bool equal(cvals_t<T, values2...>) const noexcept
    {
        return false;
    }

    /// Returns `true` if the other list holds different values
    template <T... values2>
    constexpr KFR_MEM_INTRINSIC bool notequal(cvals_t<T, values...> ind) const noexcept
    {
        return !equal(ind);
    }
};

/// @brief Empty specialization of cvals_t.
template <typename T>
struct cvals_t<T>
{
    using type = cvals_t<T>;
    /// Number of values (always 0)
    constexpr KFR_MEM_INTRINSIC static size_t size() noexcept { return 0; }

    /// Pointer to the (empty) array (always nullptr)
    static KFR_MEM_INTRINSIC const T* array() noexcept { return nullptr; }
};

/// @brief Element-wise selection between two value lists based on a flag list.
/// For each position, returns `values1[i]` if `flags[i]` is `true`, otherwise `values2[i]`.
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

/// Compile-time list of boolean values
/// @tparam values the boolean values
template <bool... values>
using cbools_t = cvals_t<bool, values...>;

/// Constant instance of `{false, true}`
constexpr inline cbools_t<false, true> cfalse_true{};

/// Compile-time list of int values
/// @tparam values the int values
template <int... values>
using cints_t = cvals_t<int, values...>;

/// Compile-time list of char values
/// @tparam values the char values
template <char... values>
using cchars_t = cvals_t<char, values...>;

/// Compile-time list of unsigned values
/// @tparam values the unsigned values
template <unsigned... values>
using cuints_t = cvals_t<unsigned, values...>;

/// Compile-time list of size_t values
/// @tparam values the size_t values
template <size_t... values>
using csizes_t = cvals_t<size_t, values...>;

/// Compile-time list of element indices (size_t values)
/// @tparam values the indices
template <size_t... values>
using elements_t = cvals_t<size_t, values...>;

/// Returns the sum of all values in the list (0 for an empty list)
template <typename T>
constexpr KFR_INTRINSIC T csum(cvals_t<T> = cvals_t<T>()) noexcept
{
    return 0;
}

/// Returns the sum of all values in the list
/// @tparam T element type
/// @tparam first first value
/// @tparam rest remaining values
template <typename T, T first, T... rest>
constexpr KFR_INTRINSIC T csum(cvals_t<T, first, rest...> = cvals_t<T, first, rest...>()) noexcept
{
    return first + csum(cvals_t<T, rest...>());
}

/// Returns the product of all values in the list (1 for an empty list)
template <typename T>
constexpr KFR_INTRINSIC T cprod(cvals_t<T>) noexcept
{
    return 1;
}

/// Returns the product of all values in the list
/// @tparam T element type
/// @tparam first first value
/// @tparam rest remaining values
template <typename T, T first, T... rest>
constexpr KFR_INTRINSIC T cprod(cvals_t<T, first, rest...>) noexcept
{
    return first * cprod(cvals_t<T, rest...>());
}

/// @brief Compile-time wrapper for a single type.
/// Used to pass types as values in metaprogramming contexts.
/// @tparam T the wrapped type
template <typename T>
struct ctype_t
{
#ifdef KFR_COMPILER_INTEL
    constexpr ctype_t() noexcept               = default;
    constexpr ctype_t(const ctype_t&) noexcept = default;
#endif
    /// The wrapped type
    using type = T;
};

/// Extracts the wrapped type from a ctype_t-like wrapper
template <typename T>
using type_of = typename T::type;

/// @brief Compile-time list of types.
/// @tparam Types the packed types
template <typename... Types>
struct ctypes_t
{
    /// Number of types in the list
    constexpr static size_t size() noexcept { return sizeof...(Types); }

    /// The type at position `index`
    template <size_t index>
    using nth = typename details::get_nth_type<index, Types...>::type;

    /// Returns a ctype_t wrapping the type at position `index`
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
/// Concatenates two or more cvals_t or ctypes_t lists into a single list
template <typename T1, typename... Ts>
using concat_lists = typename details::concat_impl<std::decay_t<T1>, std::decay_t<Ts>...>::type;

/// Returns a value of the concatenated list type
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

/// Extracts the argument types of a functor's `operator()` as a ctypes_t
template <typename Fn>
using function_arguments = typename details::function_arguments_impl<decltype(&Fn::operator())>::args;

/// Extracts the return type of a functor's `operator()`
template <typename Fn>
using function_result = typename details::function_arguments_impl<decltype(&Fn::operator())>::result;

/// Filters a cvals_t list, keeping only elements whose corresponding flag is `true`
template <typename T1, typename T2>
using cfilter_t = typename details::filter_impl<std::decay_t<T1>, std::decay_t<T2>>::type;

/// Returns a filtered cvals_t containing only elements whose flag is `true`
/// @tparam T element type
/// @tparam vals source values
/// @tparam flags selection flags (must match `vals` in length)
template <typename T, T... vals, bool... flags,
          typename Ret = cfilter_t<cvals_t<T, vals...>, cvals_t<bool, flags...>>>
constexpr KFR_INTRINSIC Ret cfilter(cvals_t<T, vals...>, cvals_t<bool, flags...>) noexcept
{
    return Ret{};
}

/// @brief Defines unary `operator op` for cvals_t and cval_t returning a new constant list/value.
/// @param op the unary operator to define
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

/// @brief Defines binary `operator op` for combinations of cvals_t and cval_t returning a new constant list.
/// @param op the binary operator to define
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

/// @brief Compile-time arithmetic sequence of `size` values starting at `start` with step `step`.
/// @tparam T element value type
/// @tparam size number of elements
/// @tparam start first value (default `T()`)
/// @tparam step difference between consecutive values (default 1)
template <typename T, size_t size, T start = T(), ptrdiff_t step = 1>
using cvalseq_t = typename details::cvalseq_impl<T, size, start, step>::type;

/// Compile-time sequence of size_t values
/// @tparam size number of elements
/// @tparam start first value (default 0)
/// @tparam step difference between consecutive values (default 1)
template <size_t size, size_t start = 0, ptrdiff_t step = 1>
using csizeseq_t = cvalseq_t<size_t, size, start, step>;

/// Returns a sequence of indices `[0, 1, ..., sizeof...(List)-1]` for the given type list
template <typename... List>
using indicesfor_t = cvalseq_t<size_t, sizeof...(List), 0>;

/// Returns a list where each index `i` is expanded into the range `[group*i, group*i + group)`
/// @tparam group size of each expanded group
/// @tparam indices source indices
template <size_t group, size_t... indices, size_t N = group * sizeof...(indices)>
constexpr KFR_INTRINSIC auto scale(csizes_t<indices...>) noexcept
{
    using Tlist = typename details::concat_impl<csizeseq_t<group, group * indices>...>::type;
    return Tlist{};
}

/// Returns a list where each index `i` is expanded into the range `[group*i, group*i + group)`
/// @tparam group size of each expanded group
/// @tparam indices source indices
template <size_t group, size_t... indices, size_t N = group * sizeof...(indices)>
constexpr KFR_INTRINSIC auto scale() noexcept
{
    using Tlist = typename details::concat_impl<csizeseq_t<group, group * indices>...>::type;
    return Tlist{};
}

namespace details
{

template <int N>
struct unique_enum_impl
{
    enum type : int
    {
        value = N
    };
};

#if defined KFR_COMPILER_MSVC && !defined KFR_COMPILER_CLANG
/// @brief MSVC-specific implementation of KFR_ENABLE_IF using a default template argument.
/// @param N unique line-based identifier
/// @param ... boolean condition
#define KFR_ENABLE_IF_IMPL(N, ...)                                                                           \
    bool enable_ = (__VA_ARGS__), typename enabled_ = typename ::std::enable_if<enable_>::type,              \
         typename kfr::details::unique_enum_impl<N>::type dummy_ =                                           \
             ::kfr::details::unique_enum_impl<N>::value

#else
/// @brief Standard implementation of KFR_ENABLE_IF using a default template argument.
/// @param N unique line-based identifier
/// @param ... boolean condition
#define KFR_ENABLE_IF_IMPL(N, ...)                                                                           \
    typename ::std::enable_if<(__VA_ARGS__), typename ::kfr::details::unique_enum_impl<N>::type>::type =     \
        ::kfr::details::unique_enum_impl<N>::value

#endif
/// @brief Inserts an enable_if constraint with a unique tag derived from the source line.
/// @param ... boolean condition that must hold for the overload to be selected
#define KFR_ENABLE_IF(...) KFR_ENABLE_IF_IMPL(__LINE__, __VA_ARGS__)
} // namespace details

namespace details
{
template <typename Fn>
KFR_INTRINSIC auto call_if_callable(Fn&& fn) noexcept
{
    if constexpr (std::is_invocable_v<Fn>)
        return fn();
    else
        return std::forward<Fn>(fn);
}
} // namespace details

/// Returns a thunk that invokes `fn` with each argument, calling it if it is callable or forwarding it
/// otherwise
/// @tparam fn target function
/// @tparam args arguments to bind (may be thunks themselves)
template <typename Fn, typename... Args>
KFR_INTRINSIC auto bind_func(Fn&& fn, Args&&... args) noexcept
{
    return [=]() KFR_INLINE_LAMBDA { return fn(details::call_if_callable(std::forward<Args>(args))...); };
}

/// Returns `true` if `x` is even
template <typename T>
constexpr KFR_INTRINSIC bool is_even(T x) noexcept
{
    return (x % 2) == 0;
}

/// Returns `true` if `x` is odd
template <typename T>
constexpr KFR_INTRINSIC bool is_odd(T x) noexcept
{
    return !is_even(x);
}

/// Returns `true` if `x` is an exact power of two (returns `false` for `0`)
template <std::unsigned_integral T>
constexpr KFR_INTRINSIC bool is_poweroftwo(T x) noexcept
{
    return std::has_single_bit(x);
}

/// Returns the base-2 logarithm of `n`, truncated toward zero; `ilog2(0)` and `ilog2(1)` return 0
template <std::unsigned_integral T>
constexpr KFR_INTRINSIC unsigned ilog2(T n) noexcept
{
    return (n <= 1) ? 0 : std::bit_width(n) - 1;
}

/// @brief Returns a nearest power of two that is greater or equal than n
template <std::unsigned_integral T>
constexpr KFR_INTRINSIC T next_poweroftwo(T n) noexcept
{
    return std::bit_ceil(n);
}

/// @brief Returns a nearest power of two that is less or equal than n
template <std::unsigned_integral T>
constexpr KFR_INTRINSIC T prev_poweroftwo(T n) noexcept
{
    return std::bit_floor(n);
}

/// Returns `true` if `x` is evenly divisible by `divisor`
/// @tparam T integer type
/// @param x dividend
/// @param divisor divisor (must be non-zero)
template <typename T>
constexpr KFR_INTRINSIC bool is_divisible(T x, T divisor) noexcept
{
    return x % divisor == 0;
}

/// Performs floor division of `a` by `b`; the remainder is always non-negative
/// @param a dividend
/// @param b divisor (must be non-zero)
/// @return quotient and remainder with `rem` in `[0, b)`
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

/// Maps a bit width to the corresponding floating-point type (32 -> float, 64 -> double)
/// @tparam bits bit width (32 or 64)
/// @remarks Any value other than 32 or 64 results in a compile error: no specialization of
/// bits_to_type_impl exists, so the primary template is an incomplete type with no `type` member.
template <size_t bits>
using float_type = typename details::bits_to_type_impl<'f', bits>::type;
/// Maps a bit width to the corresponding signed integer type
/// @tparam bits bit width (8, 16, 32 or 64)
/// @remarks Any other value results in a compile error: no specialization of bits_to_type_impl
/// exists, so the primary template is an incomplete type with no `type` member.
template <size_t bits>
using int_type = typename details::bits_to_type_impl<'i', bits>::type;
/// Maps a bit width to the corresponding unsigned integer type
/// @tparam bits bit width (8, 16, 32 or 64)
/// @remarks Any other value results in a compile error: no specialization of bits_to_type_impl
/// exists, so the primary template is an incomplete type with no `type` member.
template <size_t bits>
using unsigned_type = typename details::bits_to_type_impl<'u', bits>::type;

/// Finds the smallest integer type able to represent values in `[min, max]`
/// @tparam min minimum value to represent
/// @tparam max maximum value to represent
template <int64_t min, int64_t max>
using findinttype = typename details::findinttype_impl<min, max, uint8_t, int8_t, uint16_t, int16_t, uint32_t,
                                                       int32_t, uint64_t, int64_t>::type;

/// `true` if `T` is an integral or floating-point type (but not `bool`)
template <typename T>
constexpr inline bool is_number = details::is_number_impl<std::decay_t<T>>::value;

/// `true` if `T` is a number or `bool`
template <typename T>
constexpr inline bool is_number_or_bool = is_number<T> || std::is_same_v<std::decay_t<T>, bool>;

/// `true` if all `Ts` are numbers
/// @tparam Ts types to check
template <typename... Ts>
constexpr inline bool is_numbers = (details::is_number_impl<std::decay_t<Ts>>::value && ...);

/// @brief Check if the type argument is a number or a compound type of numbers
template <typename T>
constexpr inline bool is_numeric = is_number<deep_subtype<T>>;

/// @brief Check if the type arguments are numbers or compound types of numbers
template <typename... Ts>
constexpr inline bool is_numeric_args = (is_numeric<Ts> && ...);

/// @brief Check if the type argument is a number, bool or a compound type of numbers or bool
template <typename T>
constexpr inline bool is_numeric_or_bool = is_number_or_bool<deep_subtype<T>>;

/// Concept satisfied by numeric types (numbers or compound types of numbers)
template <typename T>
concept numeric = is_numeric<T>;

namespace details
{
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
/// Accepts any number of arguments and discards them; useful for pack expansion side effects.
struct swallow
{
    /// Constructs from any arguments, discarding them
    template <typename... T>
    KFR_MEM_INTRINSIC constexpr swallow(T&&...) noexcept
    {
    }
};

/// @brief Defines a functor `fn_##fn` that forwards to the free function `fn`.
/// Used to wrap a function as a callable type for use in metaprogramming.
/// @param fn name of the function to wrap
#define KFR_META_FN(fn)                                                                                      \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        KFR_INLINE_MEMBER constexpr decltype(fn(std::declval<Args>()...)) operator()(                        \
            Args&&... args) const noexcept                                                                   \
        {                                                                                                    \
            return fn(std::forward<Args>(args)...);                                                          \
        }                                                                                                    \
    };

/// Helper macro that expands to its arguments unchanged (used to defer macro expansion)
#define KFR_ESC(...) __VA_ARGS__

/// @brief Defines a templated functor `fn_##fn` that forwards to the template function `fn<tpl_args>`.
/// @param tpl_list template parameter list for the functor
/// @param tpl_args template arguments to pass to `fn`
/// @param fn name of the template function to wrap
#define KFR_META_FN_TPL(tpl_list, tpl_args, fn)                                                              \
    template <KFR_ESC tpl_list>                                                                              \
    struct fn_##fn                                                                                           \
    {                                                                                                        \
        template <typename... Args>                                                                          \
        KFR_INLINE_MEMBER constexpr decltype(fn<KFR_ESC tpl_args>(std::declval<Args>()...)) operator()(      \
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
/// @tparam T type to value-initialize
/// @tparam Ts ignored argument types
template <typename T, typename... Ts>
KFR_INTRINSIC constexpr T returns(Ts&&...) noexcept
{
    return T();
}

/// @brief Function that returns constant of type T and ignores all its arguments
/// @tparam T type of the constant
/// @tparam value the constant value to return
/// @tparam Args ignored argument types
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

/// @brief Functor returning a fixed constant of type `T`, ignoring all arguments.
/// @tparam T type of the constant
/// @tparam value the constant value
template <typename T, T value>
struct fn_return_constant
{
    /// Returns the wrapped constant
    template <typename... Args>
    constexpr T operator()(Args&&...) const noexcept
    {
        return value;
    }
};

/// Returns `true` if `x == y`
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_equal(const T1& x, const T2& y) noexcept(noexcept(x == y))
{
    return x == y;
}
/// Returns `true` if `x != y`
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_notequal(const T1& x, const T2& y) noexcept(noexcept(x != y))
{
    return x != y;
}
/// Returns `true` if `x < y`
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_less(const T1& x, const T2& y) noexcept(noexcept(x < y))
{
    return x < y;
}
/// Returns `true` if `x > y`
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_greater(const T1& x, const T2& y) noexcept(noexcept(x > y))
{
    return x > y;
}
/// Returns `true` if `x <= y`
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_lessorequal(const T1& x, const T2& y) noexcept(noexcept(x <= y))
{
    return x <= y;
}
/// Returns `true` if `x >= y`
template <typename T1, typename T2>
KFR_INTRINSIC constexpr bool is_greaterorequal(const T1& x, const T2& y) noexcept(noexcept(x >= y))
{
    return x >= y;
}
/// Returns `true` if `min <= value <= max` (inclusive on both ends)
/// @tparam T value type
/// @param value value to test
/// @param min lower bound (inclusive)
/// @param max upper bound (inclusive)
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

/// Concept satisfied by types that support `std::begin` and `std::end`
template <typename T>
concept has_begin_end = requires(T t) {
    std::begin(t);
    std::end(t);
};

/// Concept satisfied by types that support `std::data` and `std::size`
template <typename T>
concept has_data_size = requires(T t) {
    std::data(t);
    std::size(t);
};

/// Extracts the `value_type` nested typedef of `T`
template <typename T>
using value_type_of = typename std::decay_t<T>::value_type;

#ifndef KFR_COMPILER_CLANG
namespace details
{
template <typename T, T value, typename Fn>
constexpr void cforeach_impl(Fn&& fn)
{
    fn(cval_t<T, value>());
}
} // namespace details
#endif

/// Invokes `fn` once for each constant value in the list, passing a `cval_t<T, value>`
/// @tparam T element value type
/// @tparam values the constant values
/// @param fn callable invoked with each value as a cval_t
template <typename T, T... values, typename Fn>
KFR_INTRINSIC constexpr void cforeach(cvals_t<T, values...>, Fn&& fn)
{
#ifdef KFR_COMPILER_CLANG
    swallow{ (fn(cval_t<T, values>()), void(), 0)... };
#else
    swallow{ (details::cforeach_impl<T, values>(std::forward<Fn>(fn)), void(), 0)... };
#endif
}

/// Invokes `fn` once for each element of a range-based-for compatible container
/// @tparam T container type
/// @param fn callable invoked with each element
template <has_begin_end T, typename Fn>
KFR_INTRINSIC constexpr void cforeach(T&& list, Fn&& fn)
{
    for (const auto& v : list)
    {
        fn(v);
    }
}

namespace details
{

template <size_t index, typename... types>
KFR_INTRINSIC constexpr auto get_type_arg(ctypes_t<types...>) noexcept
{
    return ctype_t<typename details::get_nth_type<index, types...>::type>();
}

template <typename T0, typename... types, typename Fn, size_t... indices>
KFR_INTRINSIC constexpr void cforeach_types_impl(ctypes_t<T0, types...> type_list, Fn&& fn,
                                                 csizes_t<indices...>)
{
    swallow{ (fn(get_type_arg<indices>(type_list)), void(), 0)... };
}
template <typename Fn>
KFR_INTRINSIC constexpr void cforeach_types_impl(ctypes_t<>, Fn&&, csizes_t<>)
{
}
} // namespace details

/// Invokes `fn` once for each type in the list, passing a `ctype_t<T>`
/// @tparam Ts the types
/// @param fn callable invoked with each type as a ctype_t
template <typename... Ts, typename Fn>
KFR_INTRINSIC constexpr void cforeach(ctypes_t<Ts...> types, Fn&& fn)
{
    details::cforeach_types_impl(types, std::forward<Fn>(fn), csizeseq_t<sizeof...(Ts)>());
}

/// Invokes `fn` for each pair `(v0, v1)` from the Cartesian product of two lists
/// @tparam A0 first list type
/// @tparam A1 second list type
template <typename A0, typename A1, typename Fn>
KFR_INTRINSIC constexpr void cforeach(A0&& a0, A1&& a1, Fn&& fn)
{
    // Default capture causes ICE in Intel C++
    cforeach(std::forward<A0>(a0), //
             [&a1, &fn](auto v0) { //
                 cforeach(std::forward<A1>(a1), //
                          [&v0, &fn](auto v1) { fn(v0, v1); });
             });
}

/// Invokes `fn` for each triple `(v0, v1, v2)` from the Cartesian product of three lists
template <typename A0, typename A1, typename A2, typename Fn>
KFR_INTRINSIC constexpr void cforeach(A0&& a0, A1&& a1, A2&& a2, Fn&& fn)
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

/// Invokes `fn` for each 4-tuple `(v0, v1, v2, v3)` from the Cartesian product of four lists
template <typename A0, typename A1, typename A2, typename A3, typename Fn>
KFR_INTRINSIC constexpr void cforeach(A0&& a0, A1&& a1, A2&& a2, A3&& a3, Fn&& fn)
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

/// Compile-time conditional: invokes `truefn` when the condition is `true`
/// @tparam TrueFn callable invoked with ctrue on the true branch
/// @tparam FalseFn callable invoked with cfalse on the false branch
template <typename TrueFn, typename FalseFn = fn_noop>
KFR_INTRINSIC constexpr decltype(auto) cif(cbool_t<true>, TrueFn&& truefn, FalseFn&& = FalseFn())
{
    return truefn(ctrue);
}

/// Compile-time conditional: invokes `falsefn` when the condition is `false`
template <typename TrueFn, typename FalseFn = fn_noop>
KFR_INTRINSIC constexpr decltype(auto) cif(cbool_t<false>, TrueFn&&, FalseFn&& falsefn = FalseFn())
{
    return falsefn(cfalse);
}

/// Compile-time for loop over `[start, stop)` invoking `bodyfn` with each `cval_t<T, i>`
/// @tparam T index value type
/// @tparam start first index (inclusive)
/// @tparam stop past-the-last index (exclusive)
template <typename T, T start, T stop, typename BodyFn>
KFR_INTRINSIC constexpr decltype(auto) cfor(cval_t<T, start>, cval_t<T, stop>, BodyFn&& bodyfn)
{
    return cforeach(cvalseq_t<T, stop - start, start>(), std::forward<BodyFn>(bodyfn));
}

/// Switch over a constant list: invokes `function` with `cval_t<T, vs>` for the matching value, or `fallback`
/// if none match
/// @tparam T element value type
/// @tparam vs candidate values
/// @param value runtime value to match against
/// @param function invoked on match with the corresponding cval_t
/// @param fallback invoked when no candidate matches
template <typename T, T... vs, typename U, typename Function, typename Fallback = fn_noop>
KFR_INTRINSIC constexpr void cswitch(cvals_t<T, vs...>, const U& value, Function&& function,
                                     Fallback&& fallback = Fallback())
{
    bool result = false;
    swallow{ (result = result || ((vs == value) ? (function(cval_t<T, vs>()), void(), true) : false), void(),
              0)... };
    if (!result)
        fallback();
}

/// Switch over an empty list: always invokes the default handler
/// @param deffn invoked since no candidates exist
template <typename T, typename Fn, typename DefFn = fn_noop, typename CmpFn = fn_is_equal>
KFR_INTRINSIC constexpr decltype(auto) cswitch(cvals_t<T>, std::type_identity_t<T>, Fn&&,
                                               DefFn&& deffn = DefFn(), CmpFn&& = CmpFn())
{
    return deffn();
}

/// Switch over a constant list with a custom comparator: invokes `fn` on the first match, otherwise `deffn`
/// @tparam T element value type
/// @tparam v0 first candidate value
/// @tparam values remaining candidate values
/// @param value runtime value to match
/// @param fn invoked on match with the corresponding cval_t
/// @param deffn invoked when no candidate matches
/// @param cmpfn equality predicate invoked as `cmpfn(value, candidate)`
template <typename T, T v0, T... values, typename Fn, typename DefFn = fn_noop, typename CmpFn = fn_is_equal>
KFR_INTRINSIC constexpr decltype(auto) cswitch(cvals_t<T, v0, values...>, std::type_identity_t<T> value,
                                               Fn&& fn, DefFn&& deffn = DefFn(), CmpFn&& cmpfn = CmpFn())
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
KFR_INTRINSIC constexpr decltype(auto) cmatch_impl(T&& value, Fn1&& first, Fn2&& second, Fns&&... rest);
template <typename T, typename Fn, typename... Ts>
KFR_INTRINSIC constexpr decltype(auto) cmatch_impl(T&& value, Fn&& last);

template <typename T, typename Fn, typename... Fns>
KFR_INTRINSIC constexpr decltype(auto) cmatch_impl2(cbool_t<true>, T&& value, Fn&& fn, Fns&&...)
{
    return fn(std::forward<T>(value));
}

template <typename T, typename Fn, typename... Fns>
KFR_INTRINSIC constexpr decltype(auto) cmatch_impl2(cbool_t<false>, T&& value, Fn&&, Fns&&... rest)
{
    return cmatch_impl(std::forward<T>(value), std::forward<Fns>(rest)...);
}

template <typename T, typename Fn1, typename Fn2, typename... Fns>
KFR_INTRINSIC constexpr decltype(auto) cmatch_impl(T&& value, Fn1&& first, Fn2&& second, Fns&&... rest)
{
    using first_arg        = typename function_arguments<Fn1>::template nth<0>;
    constexpr bool is_same = std::is_same_v<std::decay_t<T>, std::decay_t<first_arg>>;
    return cmatch_impl2(cbool_t<is_same>(), std::forward<T>(value), std::forward<Fn1>(first),
                        std::forward<Fn2>(second), std::forward<Fns>(rest)...);
}

template <typename T, typename Fn, typename... Ts>
KFR_INTRINSIC constexpr decltype(auto) cmatch_impl(T&& value, Fn&& last)
{
    return last(std::forward<T>(value));
}
} // namespace details

/// Dispatches `value` to the first callable in `fn, args...` whose first argument type matches `T`
/// @tparam T type of the value being dispatched
/// @param fn first candidate callable
/// @param args remaining candidate callables (last one is the fallback)
template <typename T, typename Fn, typename... Args>
KFR_INTRINSIC constexpr decltype(auto) cmatch(T&& value, Fn&& fn, Args... args)
{
    return details::cmatch_impl(std::forward<T>(value), std::forward<Fn>(fn), std::forward<Args>(args)...);
}

/// Returns the index of `value` within the constant list, or `size_t(-1)` if not found
/// @tparam T element value type
/// @tparam values candidate values
/// @param value value to search for
template <typename T, T... values>
KFR_INTRINSIC constexpr size_t cfind(cvals_t<T, values...>, std::type_identity_t<T> value)
{
    constexpr T temp[]    = { values... };
    constexpr size_t size = sizeof...(values);
    for (size_t i = 0; i < size; i++)
    {
        if (temp[i] == value)
            return i;
    }
    return size_t(-1);
}

/// Invokes `fn` with `args`, forcing the call to be non-inlined
/// @tparam Fn callable type
/// @tparam Args argument types
template <typename Fn, typename... Args>
KFR_UNUSED KFR_NOINLINE static std::invoke_result_t<Fn, Args...> noinline(Fn&& fn, Args&&... args)
{
    return fn(std::forward<Args>(args)...);
}

/// @brief Functor wrapper that invokes `Fn` through `noinline`.
/// @tparam Fn wrapped callable type
template <typename Fn>
struct fn_noinline
{
    /// Invokes the wrapped callable non-inlined
    template <typename... Args>
    KFR_MEM_INTRINSIC std::invoke_result_t<Fn, Args...> operator()(Args&&... args) const
    {
        return noinline(Fn{}, std::forward<Args>(args)...);
    }
}; // namespace kfr

/// Returns a free function pointer that forwards to a functor's `operator()`
/// @tparam Args argument types of the functor
/// @tparam Fn functor type
/// @tparam Ret return type
/// @tparam NonMemFn pointer-to-function type used as the result
template <typename... Args, typename Fn, typename Ret = decltype(std::declval<Fn>()(std::declval<Args>()...)),
          typename NonMemFn = Ret (*)(Fn*, Args...)>
KFR_INTRINSIC NonMemFn make_nonmember(const Fn&)
{
    return [](Fn* fn, Args... args) -> Ret { return fn->operator()(std::forward<Args>(args)...); };
}

/// Single-argument fallback: converts `c1` to `T`
template <typename T, typename C1>
constexpr KFR_INTRINSIC T choose_const_fallback(C1 c1) noexcept
{
    return static_cast<T>(c1);
}

/**
 * @brief Selects the constant of the specific type from a list of candidates.
 * Returns the candidate whose type matches `T`; triggers a static_assert if `T` is not present.
 * @code
 * CHECK( choose_const<f32>( 32.0f, 64.0 ) == 32.0f );
 * CHECK( choose_const<f64>( 32.0f, 64.0 ) == 64.0 );
 * @endcode
 */
template <typename T>
constexpr KFR_INTRINSIC T choose_const() noexcept
{
    static_assert(sizeof(T) != 0, "T not found in the list of template arguments");
    return T();
}
/// Returns `c1` when its type matches `T`
template <typename T, typename C1, typename... Cs>
constexpr KFR_INTRINSIC T choose_const(C1 c1, Cs...) noexcept
    requires std::is_same_v<T, C1>
{
    return static_cast<T>(c1);
}
/// Recursively skips candidates whose type does not match `T`
template <typename T, typename C1, typename... Cs>
constexpr KFR_INTRINSIC T choose_const(C1, Cs... constants) noexcept
    requires(!std::is_same_v<T, C1>)
{
    return choose_const<T>(constants...);
}

/// Returns the candidate whose type matches `T`, converting the last candidate if none match
/// @tparam T target type
/// @tparam C1 first candidate type
/// @tparam Cs remaining candidate types
template <typename T, typename C1, typename... Cs>
constexpr KFR_INTRINSIC T choose_const_fallback(C1 c1, Cs... constants) noexcept
{
    return std::is_same_v<T, C1> ? static_cast<T>(c1) : choose_const_fallback<T>(constants...);
}

/// @brief Helper returned by `autocast` that converts to any type via `static_cast`.
/// @tparam Tfrom source value type
template <typename Tfrom>
struct autocast_impl
{
    const Tfrom value;
    /// Converts the stored value to `T` via `static_cast`
    template <typename T>
    KFR_MEM_INTRINSIC constexpr operator T() const noexcept
    {
        return static_cast<T>(value);
    }
};

/// Returns a proxy that implicitly converts to any type via `static_cast`
/// @tparam Tfrom source value type
/// @param value value to convert
template <typename Tfrom>
KFR_INTRINSIC constexpr autocast_impl<Tfrom> autocast(const Tfrom& value) noexcept
{
    return { value };
}

/// Non-constexpr function used to halt constant evaluation in `if constexpr` branches
inline void stop_constexpr() {}

/// Rounds `x` down to the nearest multiple of `alignment` (power of two)
/// @tparam T integer type
/// @param x value to align
/// @param alignment alignment (must be a power of two)
template <typename T>
constexpr KFR_INTRINSIC T align_down(T x, std::type_identity_t<T> alignment) noexcept
{
    return (x) & ~(alignment - 1);
}
/// Rounds the pointer `x` down to the nearest multiple of `alignment` bytes
/// @tparam T pointee type
/// @param x pointer to align
/// @param alignment alignment in bytes (must be a power of two)
template <typename T>
constexpr KFR_INTRINSIC T* align_down(T* x, size_t alignment) noexcept
{
    return reinterpret_cast<T*>(align_down(reinterpret_cast<uintptr_t>(x), alignment));
}

/// Rounds `x` up to the nearest multiple of `alignment` (power of two)
/// @tparam T integer type
/// @param x value to align
/// @param alignment alignment (must be a power of two)
template <typename T>
constexpr KFR_INTRINSIC T align_up(T x, std::type_identity_t<T> alignment) noexcept
{
    return (x + alignment - 1) & ~(alignment - 1);
}
/// Rounds the pointer `x` up to the nearest multiple of `alignment` bytes
/// @tparam T pointee type
/// @param x pointer to align
/// @param alignment alignment in bytes (must be a power of two)
template <typename T>
constexpr KFR_INTRINSIC T* align_up(T* x, size_t alignment) noexcept
{
    return reinterpret_cast<T*>(align_up(reinterpret_cast<uintptr_t>(x), alignment));
}

/// Returns a pointer advanced by `offset` elements of type `T`
/// @tparam T pointee type
/// @param x base pointer
/// @param offset number of elements to advance
template <typename T>
constexpr KFR_INTRINSIC T* advance(T* x, ptrdiff_t offset) noexcept
{
    return x + offset;
}
/// Returns a `void*` advanced by `offset` bytes
/// @param x base pointer
/// @param offset number of bytes to advance
constexpr KFR_INTRINSIC void* advance(void* x, ptrdiff_t offset) noexcept
{
    return advance(static_cast<unsigned char*>(x), offset);
}

/// Returns the byte distance from `y` to `x` (i.e. `x - y`)
/// @param x first pointer
/// @param y second pointer
constexpr KFR_INTRINSIC ptrdiff_t distance(const void* x, const void* y) noexcept
{
    return static_cast<const unsigned char*>(x) - static_cast<const unsigned char*>(y);
}

KFR_PRAGMA_GNU(GCC diagnostic push)
#if KFR_HAS_WARNING("-Wundefined-reinterpret-cast")
KFR_PRAGMA_GNU(GCC diagnostic ignored "-Wundefined-reinterpret-cast")
#endif

/// Reinterprets a reference of type `U` as a reference of type `T`
/// @tparam T target reference type
/// @tparam U source reference type
template <typename T, typename U>
KFR_INTRINSIC constexpr static T& ref_cast(U& ptr) noexcept
{
    return reinterpret_cast<T&>(ptr);
}

/// Reinterprets a const reference of type `U` as a const reference of type `T`
template <typename T, typename U>
KFR_INTRINSIC constexpr static const T& ref_cast(const U& ptr) noexcept
{
    return reinterpret_cast<const T&>(ptr);
}

/// Reinterprets a pointer of type `U*` as a pointer of type `T*`
/// @tparam T target pointee type
/// @tparam U source pointee type
template <typename T, typename U>
KFR_INTRINSIC constexpr static T* ptr_cast(U* ptr) noexcept
{
    return reinterpret_cast<T*>(ptr);
}

/// Reinterprets a const pointer of type `U*` as a const pointer of type `T*`
template <typename T, typename U>
KFR_INTRINSIC constexpr static const T* ptr_cast(const U* ptr) noexcept
{
    return reinterpret_cast<const T*>(ptr);
}

/// Reinterprets `ptr` as `T*` and advances it by `offset` bytes
/// @tparam T target pointee type
/// @tparam U source pointee type
/// @param ptr base pointer
/// @param offset byte offset to apply
template <typename T, typename U>
KFR_INTRINSIC constexpr static T* ptr_cast(U* ptr, ptrdiff_t offset) noexcept
{
    return ptr_cast<T>(ptr_cast<unsigned char>(ptr) + offset);
}

/// Down-casts a pointer from `U*` to `T*` using `static_cast` (for related types)
/// @tparam T target derived pointee type
/// @tparam U source base pointee type
template <typename T, typename U>
KFR_INTRINSIC constexpr static T* derived_cast(U* ptr) noexcept
{
    return static_cast<T*>(ptr);
}

/// Down-casts a const pointer from `U*` to `T*` using `static_cast`
template <typename T, typename U>
KFR_INTRINSIC constexpr static const T* derived_cast(const U* ptr) noexcept
{
    return static_cast<const T*>(ptr);
}

/// Performs an implicit conversion from `U` to `T` (useful to suppress template argument deduction)
/// @tparam T target type
/// @tparam U source type
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

/// Returns `true` if the list forms a contiguous ascending sequence `[number, number+1, ...]`
/// @tparam number first value
/// @tparam numbers remaining values
template <size_t number, size_t... numbers>
constexpr KFR_INTRINSIC bool is_sequence(csizes_t<number, numbers...>) noexcept
{
    return details::test_sequence<number, 1 + sizeof...(numbers)>(csizes_t<number, numbers...>()).value;
}

/// Constant instance of cval_t<T, val>
template <typename T, T val>
constexpr inline cval_t<T, val> cval{};

template <bool val>
constexpr inline cbool_t<val> cbool{};

/// Constant instance of cint_t<val>
template <int val>
constexpr inline cint_t<val> cint{};

/// Constant instance of cuint_t<val>
template <unsigned val>
constexpr inline cuint_t<val> cuint{};

/// Constant instance of csize_t<val>
template <size_t val>
constexpr inline csize_t<val> csize{};

/// Constant instance of cvals_t<T, values...>
template <typename T, T... values>
constexpr inline cvals_t<T, values...> cvals{};

/// Constant instance of cbools_t<vals...>
template <bool... vals>
constexpr inline cbools_t<vals...> cbools{};

/// Constant instance of cints_t<vals...>
template <int... vals>
constexpr inline cints_t<vals...> cints{};

/// Constant instance of cchars_t<vals...>
template <char... vals>
constexpr inline cchars_t<vals...> cchars{};

/// Constant instance of cuints_t<vals...>
template <unsigned... vals>
constexpr inline cuints_t<vals...> cuints{};

/// Constant instance of csizes_t<vals...>
template <size_t... vals>
constexpr inline csizes_t<vals...> csizes{};

/// Constant instance of elements_t<vals...>
template <size_t... vals>
constexpr inline elements_t<vals...> elements{};

/// Constant instance of ctype_t<T>
template <typename T>
constexpr inline ctype_t<T> ctype{};

/// Constant instance of ctypes_t<Ts...>
template <typename... Ts>
constexpr inline ctypes_t<Ts...> ctypes{};

/// Constant instance of the value range `[begin, end)`
template <typename T, T begin, T end>
constexpr inline cvalseq_t<T, end - begin, begin> cvalrange{};

/// Constant instance of the size_t range `[begin, end)`
template <size_t begin, size_t end>
constexpr inline cvalseq_t<size_t, end - begin, begin> csizerange{};

/// Constant instance of the int range `[begin, end)`
template <int begin, int end>
constexpr inline cvalseq_t<int, end - begin, begin> cintrange{};

/// Constant instance of the unsigned range `[begin, end)`
template <unsigned begin, unsigned end>
constexpr inline cvalseq_t<unsigned, end - begin, begin> cuintrange{};

/// Constant instance of cvalseq_t with `size` elements starting at `start` with step `step`
template <typename T, size_t size, T start = T(), ptrdiff_t step = 1>
constexpr inline cvalseq_t<T, size, start, step> cvalseq{};

/// Constant instance of csizeseq_t with `size` elements starting at `start` with step `step`
template <size_t size, size_t start = 0, ptrdiff_t step = 1>
constexpr inline cvalseq_t<size_t, size, start, step> csizeseq{};

/// @brief Compile-time for loop helper that unrolls iteration over `[start, stop)`.
///
/// Assigning a lambda to a `cfor_t` instance invokes the lambda once per index `i`
/// in the range `[start, stop)`, passing `csize<i>` as the argument. The loop is
/// fully unrolled at compile time via pack expansion.
///
/// When `conditional` is `true`, iteration short-circuits as soon as the body
/// returns a falsy value (logical AND fold). When `false`, all iterations are
/// executed unconditionally (comma fold).
///
/// @tparam stop      past-the-last index (exclusive)
/// @tparam start     first index (inclusive, default 0)
/// @tparam conditional if `true`, stop on the first falsy body result
///
/// @code
/// // `i` is a compile-time constant inside the body; the loop is unrolled.
/// // Note: a semicolon is required after the closing brace (unlike a regular
/// // for loop), because the macro expands to an assignment expression.
/// KFR_FOR(i, 0, 3)
/// {
///     constexpr size_t j = i;
///     std::printf("%zu\n", j);  // prints 0, 1, 2
/// };
///
/// // Conditional: stops at the first index where the body returns false
/// KFR_FORC(i, 0, 5)
/// {
///     return i < 2;  // only i == 0 and i == 1 are processed
/// };
/// @endcode
template <size_t stop, size_t start, bool conditional = false>
struct cfor_t
{
    /// @brief Invokes `fn` for each index in `[start, stop)`, passing i as template argument.
    /// @tparam Fn body callable; return value is ignored unless `conditional` is `true`.
    template <typename Fn>
    constexpr KFR_MEM_INTRINSIC void operator=(Fn&& fn) const
    {
        [&]<size_t... i>(std::index_sequence<i...>) KFR_INLINE_LAMBDA
        {
            if constexpr (conditional)
                std::ignore = (... && fn.template operator()<start + i>());
            else
                (fn.template operator()<start + i>(), ...);
        }(std::make_index_sequence<stop - start>{});
    }
};

/// @brief Constant instance of `cfor_t<stop, start, conditional>` used as the
///        left-hand side of `KFR_FOR` / `KFR_FORC`.
/// @relates cfor_t
template <size_t stop, size_t start, bool conditional = false>
constexpr inline cfor_t<stop, start, conditional> cfor_v{};

/// @brief Compile-time for loop: `KFR_FOR(var, init, stop) { body }`
///        Unrolls `body` for `var` in `[init, stop)`. The macro supplies the
///        capture block, template argument list and argument list, so the body
///        is just a brace-enclosed block in which `var` is a compile-time constant.
/// @param var  loop variable name (a size_t non-type template parameter)
/// @param init first index (inclusive)
/// @param stop past-the-last index (exclusive)
/// @code
/// KFR_FOR(i, 0, 4)
/// {
///     constexpr size_t j = i;  // i is usable as a constant expression
/// };  // semicolon required: macro expands to an assignment expression
/// @endcode
#define KFR_FOR(var, init, stop) cfor_v<stop, init> = [&]<size_t var>() KFR_INLINE_LAMBDA

/// @brief Conditional compile-time for loop: `KFR_FORC(var, init, stop) { body }`
///        Like @ref KFR_FOR, but stops as soon as `body` returns a falsy value.
///        The macro supplies the capture block, template argument list and
///        argument list; the body is a brace-enclosed block returning a value
///        convertible to `bool`.
/// @param var  loop variable name (a size_t non-type template parameter)
/// @param init first index (inclusive)
/// @param stop past-the-last index (exclusive)
/// @code
/// KFR_FORC(i, 0, 8)
/// {
///     return i < 3;  // stops after i == 0, 1, 2
/// };  // semicolon required: macro expands to an assignment expression
/// @endcode
#define KFR_FORC(var, init, stop) cfor_v<stop, init, true> = [&]<size_t var>() KFR_INLINE_LAMBDA

/// Constant instance of an int sequence with `size` elements starting at `start` with step `step`
template <size_t size, int start = 0, ptrdiff_t step = 1>
constexpr inline cvalseq_t<int, size, start, step> cintseq{};

/// Constant instance of an unsigned sequence with `size` elements starting at `start` with step `step`
template <size_t size, unsigned start = 0, ptrdiff_t step = 1>
constexpr inline cvalseq_t<unsigned, size, start, step> cuintseq{};

/// Constant instance of indicesfor_t for the given type list
template <typename... List>
constexpr inline indicesfor_t<List...> indicesfor{};

/// Returns the minimum value in the list (max of `T` for an empty list)
template <typename T>
constexpr KFR_INTRINSIC T cminof(cvals_t<T>)
{
    return std::numeric_limits<T>::max();
}
/// Returns the minimum value in the list
/// @tparam T element type
/// @tparam val first value
/// @tparam vals remaining values
template <typename T, T val, T... vals>
constexpr KFR_INTRINSIC T cminof(cvals_t<T, val, vals...>)
{
    T m = cminof(cvals<T, vals...>);
    return val < m ? val : m;
}
/// Returns the maximum value in the list (min of `T` for an empty list)
template <typename T>
constexpr KFR_INTRINSIC T cmaxof(cvals_t<T>)
{
    return std::numeric_limits<T>::min();
}
/// Returns the maximum value in the list
/// @tparam T element type
/// @tparam val first value
/// @tparam vals remaining values
template <typename T, T val, T... vals>
constexpr KFR_INTRINSIC T cmaxof(cvals_t<T, val, vals...>)
{
    T m = cmaxof(cvals<T, vals...>);
    return val > m ? val : m;
}

/// @brief Tag type forming a linear inheritance chain used to rank overload candidates.
/// `overload_priority<n>` derives from `overload_priority<n-1>`, so larger `n` is a better match.
/// @tparam n priority level (default 10)
template <int n = 10>
struct overload_priority : overload_priority<n - 1>
{
};

/// Lowest priority tag, base of the overload_priority chain
template <>
struct overload_priority<0>
{
};

/// Convenience instance of the highest-priority tag for use in overload resolution
constexpr inline overload_priority<> overload_auto{};

/// Alias for the lowest-priority tag, used as a fallback overload
using overload_generic = overload_priority<0>;

/// @brief Generates a comma-separated list by invoking macro `m` with indices `0..N-1`.
/// Each `KFR_GEN_LISTN(m, ...)` expands to `m(0,...), m(1,...), ..., m(N-1,...)`.
/// @param m macro invoked as `m(index, __VA_ARGS__)`
/// @param ... extra arguments forwarded to `m`
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

/// @brief Dispatcher macro: expands `KFR_GEN_LIST##c` to generate a list of length `c`.
/// @param c count of elements (1..70)
/// @param m macro invoked as `m(index, __VA_ARGS__)`
/// @param ... extra arguments forwarded to `m`
#define KFR_GEN_LIST(c, m, ...) KFR_GEN_LIST##c(m, __VA_ARGS__)

/// Reinterprets the bits of `in` as a value of type `Tout` (requires equal sizes)
/// @tparam Tout target type
/// @tparam Tin source type
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

/// Returns `x` unchanged while preventing template argument deduction for `T`
/// @tparam T value type
template <typename T>
KFR_INTRINSIC constexpr T dont_deduce(T x)
{
    return x;
}

/// Returns `value` unchanged; the `Ty` template parameter can be used for SFINAE without affecting deduction
/// @tparam Ty tag type used for SFINAE
/// @tparam T value type
template <typename Ty, typename T>
KFR_INTRINSIC constexpr T just_value(T value)
{
    return value;
}

/// Returns the identity value (0) for an empty pack
/// @tparam Tout result type
template <typename Tout, typename>
KFR_INTRINSIC constexpr Tout pack_elements()
{
    return 0;
}

/// Packs the integer arguments into a single value of type `Tout` by OR-ing them at increasing byte offsets
/// @tparam Tout result type
/// @tparam Arg first argument type
/// @tparam Args remaining argument types
/// @param x first value (least significant)
/// @param args remaining values (each shifted left by the size of `Arg`)
template <typename Tout, typename Arg, typename... Args>
KFR_INTRINSIC constexpr Tout pack_elements(Arg x, Args... args)
{
    return static_cast<typename std::make_unsigned<Arg>::type>(x) |
           (pack_elements<Tout, Arg>(args...) << (sizeof(Arg) * 8));
}

/// Yields `const T&` when `reference` is `true`, otherwise `T`
/// @tparam T value type
/// @tparam reference whether to use a reference
template <typename T, bool reference>
using value_or_ref = std::conditional_t<reference, const T&, T>;

/// @brief Enumeration of special numeric values selectable through `special_value`.
enum class special_constant
{
    default_constructed, ///< value-initialized `T{}`
    infinity, ///< positive infinity
    neg_infinity, ///< negative infinity
    min, ///< minimum positive normalized value
    max, ///< maximum representable value
    neg_max, ///< negated maximum representable value
    lowest, ///< most negative representable value
    epsilon, ///< machine epsilon
    integer, ///< integer literal carried by `special_value`
    floating_point, ///< floating-point literal carried by `special_value`
    random_bits, ///< value filled with random bits
};

KFR_PRAGMA_MSVC(warning(push))
KFR_PRAGMA_MSVC(warning(disable : 4700))
KFR_PRAGMA_MSVC(warning(disable : 4146))
/// @brief Type-erased constant convertible to any numeric type `T`.
/// Holds either a `special_constant` tag, an integer literal, or a floating-point literal,
/// and converts to `T` according to the active variant.
struct special_value
{
    constexpr special_value(const special_value&) = default;
    /// Constructs from a special_constant tag
    constexpr special_value(special_constant c) : c(c), ll(0), d(0) {}
    /// Constructs from a double literal
    constexpr special_value(double d) : c(special_constant::floating_point), ll(0), d(d) {}
    /// Constructs from a long long literal
    constexpr special_value(long long ll) : c(special_constant::integer), ll(ll), d(0) {}
    /// Constructs from an int literal
    constexpr special_value(int i) : c(special_constant::integer), ll(i), d(0) {}

    /// Returns the value converted to type `T` according to the active variant
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

    /// Converts to type `T` via `get<T>()`
    template <typename T>
    constexpr operator T() const noexcept
    {
        return get<T>();
    }
    special_constant c; ///< Active variant tag
    long long ll; ///< Stored integer literal
    double d; ///< Stored floating-point literal

    /// Returns a reference to the shared mt19937 random generator (seeded with 1)
    static std::mt19937& random_generator()
    {
        static std::mt19937 rnd(1);
        return rnd;
    }

    /// Returns a value of type `T` whose bits are filled with random data
    /// @tparam T target type
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

/// @brief Compile-time index transformer used to build shuffle/permutation tables.
///
/// Given a length `size` and a sequence of unary callables `Fn...`, `map_indices_impl`
/// produces a `csizes_t` of length `size` whose element at position `i` is obtained by
/// applying the functions to `i`.
///
/// The functions are applied in **reverse declaration order**: the last listed callable
/// is applied first (innermost), and the first listed callable is applied last
/// (outermost). For a pack `F0, F1, ..., Fn-1` the element at index `i` is
/// `F0(F1(...(Fn-1(i))))`. This lets call sites list transforms in the order they are
/// conceptually composed (leftmost = final/outermost), matching how the result is read.
///
/// Each `Fn` must be a constexpr-invocable entity (e.g. a captureless function pointer
/// or a stateless functor) convertible to `size_t(size_t)`, since it is stored in a
/// `constexpr std::tuple` and invoked during constant evaluation.
///
/// @tparam size number of output indices; the input sequence is `[0, size)`.
/// @tparam Fn... unary callables `size_t -> size_t` applied right-to-left.
///
/// @code
/// // Square then negate-listed-last-applied-first: result is {0, 1, 4, 9}
/// using sq = map_indices_t<4, fn_return_constant<size_t, 0>>; // identity-like example
/// @endcode
///
/// @see map_indices_t
/// @see map_indices
template <size_t size, auto... Fn>
struct map_indices_impl
{
    using input_type = csizeseq_t<size>;

#if defined KFR_COMPILER_IS_MSVC
    // MSVC workaround: a named static function is used instead of a local lambda so the
    // fold expression over `J` can be expanded inside a constexpr member.
    template <size_t... J>
    static constexpr size_t apply_impl(size_t x, csizes_t<J...>) noexcept
    {
        constexpr auto fns = std::tuple{ Fn... };
        // J = 0..n-1, so `sizeof...(Fn) - 1 - J` selects functions from last to first;
        // the left-to-right comma fold therefore applies them in reverse declaration order.
        ((x = std::get<sizeof...(Fn) - 1 - J>(fns)(x)), ...);
        return x;
    }

    template <size_t... I>
    static constexpr auto helper(csizes_t<I...>)
    {
        return csizes_t<apply_impl(I, csizeseq_t<sizeof...(Fn)>{})...>{};
    }
#else
    template <size_t... I>
    static constexpr auto helper(csizes_t<I...>)
    {
        // For each input index `I`, build the output by threading `I` through the
        // function pack in reverse order (last declared = first applied).
        constexpr auto apply = [](size_t x)
        {
            constexpr auto fns = std::tuple{ Fn... };
            [&]<size_t... J>(csizes_t<J...>)
            { ((x = std::get<sizeof...(Fn) - 1 - J>(fns)(x)), ...); }(csizeseq_t<sizeof...(Fn)>{});
            return x;
        };

        return csizes_t<apply(I)...>{};
    }
#endif

    using type = decltype(helper(input_type{}));
};

/// @brief Alias for the `csizes_t` produced by `map_indices_impl<size, Fn...>`.
///
/// Yields a compile-time list of `size` values where element `i` equals
/// `F0(F1(...(Fn-1(i))))` — i.e. the functions are composed right-to-left so that the
/// order written at the call site matches the order of conceptual application
/// (leftmost transform is the outermost/last one applied).
///
/// Commonly used to build SIMD shuffle indices by composing primitive permute functions
/// such as `interleave_permute`, `split_permute`, `ctranspose_permute` and `br_permute`
/// (see `ngfft.hpp`'s `cread_group2` / `cwrite_group2`).
///
/// @tparam size length of the resulting index list.
/// @tparam Fn... unary `size_t -> size_t` callables, applied right-to-left.
///
/// @code
/// // Compose two permutes: br_permute is applied first, then split_permute, then
/// // interleave_permute (leftmost = outermost).
/// using Indices = map_indices_t<2 * R * N,
///                               interleave_permute<in_split_width, U>,
///                               split_permute<split_format ? N : 1, U>,
///                               br_permute<R, N, bitrev>>;
/// @endcode
template <size_t size, auto... Fn>
using map_indices_t = typename map_indices_impl<size, Fn...>::type;

/// @brief Returns a value of `map_indices_t<size, Fn...>` for use in non-type contexts.
///
/// Convenience function form of @ref map_indices_t; equivalent to
/// `map_indices_t<size, Fn...>{}`. Useful when an instance (rather than just the type)
/// is required, e.g. to pass the computed index list to a function expecting a
/// `csizes_t` value.
///
/// @tparam size length of the resulting index list.
/// @tparam Fn... unary `size_t -> size_t` callables, applied right-to-left.
/// @return a default-constructed `map_indices_t<size, Fn...>`.
template <size_t size, auto... Fn>
constexpr auto map_indices() -> map_indices_t<size, Fn...>
{
    return {};
}

/// @brief Empty type tagged with a unique integer `tag`.
///
/// Because distinct specializations (`empty_struct<0>`, `empty_struct<1>`, ...)
/// are different types, each occupies a separate address. This lets
/// `[[no_unique_address]]` (KFR_NO_UNIQUE_ADDRESS) collapse multiple empty
/// fields of a struct to zero size, while still allowing several such fields
/// to coexist without aliasing one another.
///
/// @tparam tag unique identifier distinguishing one empty field from another
///
/// @code
/// struct S {
///     KFR_NO_UNIQUE_ADDRESS empty_struct<0> a; // 0 bytes
///     KFR_NO_UNIQUE_ADDRESS empty_struct<1> b; // 0 bytes
/// };
/// static_assert(sizeof(S) == 1); // both empty fields compacted away
/// @endcode
template <int tag>
struct empty_struct
{
};

KFR_PRAGMA_MSVC(warning(pop))

KFR_PRAGMA_GNU(GCC diagnostic pop)
} // namespace kfr

KFR_PRAGMA_GNU(GCC diagnostic pop)

KFR_PRAGMA_MSVC(warning(pop))
