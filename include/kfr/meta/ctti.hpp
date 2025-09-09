/** @addtogroup meta
 *  @{
 */
#pragma once

#include "../meta.hpp"
#include "cstring.hpp"

namespace kfr
{
using pconstvoid = const void*;

struct type_id_t
{
    constexpr type_id_t(const void* id) KFR_NOEXCEPT : id(id) {}
    constexpr bool operator==(type_id_t other) const { return id == other.id; }
    constexpr bool operator!=(type_id_t other) const { return !(id == other.id); }
    const void* const id;
};

namespace details
{

template <typename T>
constexpr inline type_id_t typeident_impl() KFR_NOEXCEPT
{
    return type_id_t(pconstvoid(&typeident_impl<T>));
}

#if defined KFR_COMPILER_CLANG && KFR_COMPILER_MSVC
constexpr size_t typename_prefix  = sizeof("auto __cdecl kfr::ctype_name(void) [T = ") - 1;
constexpr size_t typename_postfix = sizeof("]") - 1;
#elif defined KFR_COMPILER_CLANG
constexpr size_t typename_prefix  = sizeof("auto kfr::ctype_name() [T = ") - 1;
constexpr size_t typename_postfix = sizeof("]") - 1;
#elif defined KFR_COMPILER_MSVC
constexpr size_t typename_prefix  = sizeof("auto __cdecl kfr::ctype_name<") - 1;
constexpr size_t typename_postfix = sizeof(">(void) noexcept") - 1;
#else // GCC
constexpr size_t typename_prefix  = sizeof("constexpr auto kfr::ctype_name() [with T = ") - 1;
constexpr size_t typename_postfix = sizeof("]") - 1;
#endif

template <size_t... indices, size_t Nout = 1 + sizeof...(indices)>
constexpr cstring<Nout> gettypename_impl(const char* str, csizes_t<indices...>) KFR_NOEXCEPT
{
    return cstring<Nout>{ { (str[indices])..., 0 } };
}
template <size_t... indices>
constexpr cstring<1> gettypename_impl(const char*, csizes_t<>) KFR_NOEXCEPT
{
    return cstring<1>{ { 0 } };
}
} // namespace details

#ifdef KFR_COMPILER_MSVC
#define KFR_CALL_CONV_SPEC __cdecl
#else
#define KFR_CALL_CONV_SPEC
#endif

template <typename T>
constexpr auto KFR_CALL_CONV_SPEC ctype_name() noexcept
{
    constexpr size_t length =
        sizeof(KFR_FUNC_SIGNATURE) - 1 > details::typename_prefix + details::typename_postfix
            ? sizeof(KFR_FUNC_SIGNATURE) - 1 - details::typename_prefix - details::typename_postfix
            : 0;
    static_assert(length > 0, "");
    return details::gettypename_impl(KFR_FUNC_SIGNATURE + (length > 0 ? details::typename_prefix : 0),
                                     csizeseq<length>);
}

/**
 * @brief Gets the fully qualified name of the type, including namespace and
 * template parameters (if any)
 * @tparam T    type
 * @return      name of the type
 */
template <typename T>
inline const char* type_name() KFR_NOEXCEPT
{
    static const auto name = ctype_name<T>();
    return name.c_str();
}

/**
 * @brief Gets the fully qualified name of the type, including namespace and
 * template parameters (if any)
 * @param x      value of specific type
 * @return      name of the type
 */
template <typename T>
inline const char* type_name(T x) KFR_NOEXCEPT
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
} // namespace kfr
